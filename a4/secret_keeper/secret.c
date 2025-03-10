#include <minix/drivers.h>
#include <minix/driver.h>
#include <stdio.h>
#include <stdlib.h>
#include <minix/ds.h>
#include "secret.h"
#include <sys/ioc_secret.h>

/*
 * Function prototypes for the secret driver.
 */
FORWARD _PROTOTYPE( char * secret_name,   (void) );
FORWARD _PROTOTYPE( int secret_open,      (struct driver *d, message *m) );
FORWARD _PROTOTYPE( int secret_close,     (struct driver *d, message *m) );
FORWARD _PROTOTYPE( struct device * secret_prepare, (int device) );
FORWARD _PROTOTYPE( int secret_transfer,  (int procnr, int opcode,
                                          u64_t position, iovec_t *iov,
                                          unsigned nr_req) );
FORWARD _PROTOTYPE( void secret_geometry, (struct partition *entry) );
FORWARD _PROTOTYPE( int secret_ioctl,    (struct driver *d, message *m) );

/* SEF functions and variables. */
FORWARD _PROTOTYPE( void sef_local_startup, (void) );
FORWARD _PROTOTYPE( int sef_cb_init, (int type, sef_init_info_t *info) );
FORWARD _PROTOTYPE( int sef_cb_lu_state_save, (int) );
FORWARD _PROTOTYPE( int lu_state_restore, (void) );

/* Entry points to the secret driver. */
PRIVATE struct driver secret_tab =
{
    secret_name,
    secret_open,
    secret_close,
    secret_ioctl,
    secret_prepare,
    secret_transfer,
    nop_cleanup,
    secret_geometry,
    nop_alarm,
    nop_cancel,
    nop_select,
    nop_ioctl,
    do_nop,
};

/** Represents the /dev/secret device. */
PRIVATE struct device secret_device;

/** State variable to count the number of times the device has been opened. */
PRIVATE int open_counter;

/** Driver Global Variables */
PRIVATE int owned;
PRIVATE uid_t owner_uid;
PRIVATE int read_flag;
PRIVATE int fd_open_counter;
PRIVATE int write_index;
PRIVATE int read_index;

PRIVATE char buffer[SECRET_SIZE];


PRIVATE char * secret_name(void)
{
    printf("secret_name()\n");
    return "secret";
}

PRIVATE int secret_open(d, m)
    struct driver *d;
    message *m;
{
    struct ucred creds;
    int op_read, op_write;

    getnucred(m->IO_ENDPT, &creds);
    op_read = m->COUNT & R_BIT;
    op_write = m->COUNT & W_BIT;

    if (!owned) {
        /* Secret has not been claimed */
        if (op_read && op_write) {
            return EACCES; /* Cannot read-write at same time */
        } else if (op_write) {
            read_flag = 1;  /* Going to write, readable after writing */
        } else {
            /* Trying to read */
            read_flag = 0;  /* Secret will have been read */
        }
        owner_uid = creds.uid;
        owned = 1;

    } else {
        /* Secret has been claimed */
        if (op_write){
            return ENOSPC;  /* Cannot write with secret already inside */
        } else {
            if (creds.uid != owner_uid){
                return EACCES; /* Cannot open a secret someone else owns */
            }
            read_flag = 0;  /* Read secret */
        }
    }

    fd_open_counter++;
    return OK;
}

PRIVATE int secret_close(d, m)
    struct driver *d;
    message *m;
{
    fd_open_counter--;  /* Closing a file descriptor*/

    if (fd_open_counter == 0 && !read_flag){
        owned = 0;  /* Reset secret drive */
        owner_uid = NO_ID;
        memset(buffer, 0, SECRET_SIZE); /* Reset buffer */
        write_index = 0;    /* Reset read and write buffer positions */
        read_index = 0;
    }
    return OK;
}

PRIVATE int secret_ioctl(d, m)
    struct driver *d;
    message *m;
{
    /* Transfer ownership of secret */
    uid_t new_uid;
    int ret;

    if (m->REQUEST != SSGRANT) {
        return ENOTTY;
    }

    ret = sys_safecopyfrom(m->IO_ENDPT, (vir_bytes)m->IO_GRANT, 
                            0, (vir_bytes)&new_uid, sizeof(new_uid), D);

    if (ret == -1) {
        return ret;
    }

    owner_uid = new_uid;
    return ret;
}

PRIVATE struct device * secret_prepare(dev)
    int dev;
{
    secret_device.dv_base.lo = 0;
    secret_device.dv_base.hi = 0;
    secret_device.dv_size.lo = 0;
    secret_device.dv_size.hi = 0;
    return &secret_device;
}

PRIVATE int secret_transfer(proc_nr, opcode, position, iov, nr_req)
    int proc_nr;
    int opcode;
    u64_t position;
    iovec_t *iov;
    unsigned nr_req;
{
    int bytes, ret;

    switch (opcode)
    {
        case DEV_GATHER_S:
            /* Reading from device */
            bytes = write_index - read_index < iov->iov_size ?
            write_index - read_index : iov->iov_size;

            if (bytes <= 0) {
                return OK;
            }

            ret = sys_safecopyto(proc_nr, iov->iov_addr, 0,
                                (vir_bytes) (buffer + read_index),
                                 bytes, D);
            
            if (ret != OK) {
                return ret;
            }

            iov->iov_size -= bytes;
            read_index += bytes;
            break;

        case DEV_SCATTER_S:
            /* Writing to device */
            bytes = SECRET_SIZE - write_index < iov->iov_size ?
            SECRET_SIZE - write_index : iov->iov_size;

            if (bytes <= 0) {
                return ENOSPC;
            }

            ret = sys_safecopyfrom(proc_nr, iov->iov_addr, 0,
                                (vir_bytes) (buffer + write_index), 
                                 bytes, D);

            if (ret != OK) {
                return ret;
            }
            
            iov->iov_size -= bytes;
            write_index += bytes;
            break;

        default:
            return EINVAL;
    }
    return ret;
}

PRIVATE void secret_geometry(entry)
    struct partition *entry;
{
    printf("secret_geometry()\n");
    entry->cylinders = 0;
    entry->heads     = 0;
    entry->sectors   = 0;
    return;
}

PRIVATE int sef_cb_lu_state_save(int state) {
/* Save the state. */
    struct secret_info save;

    save.fd_counter = fd_open_counter;
    save.uid = owner_uid;
    save.owned = owned;
    save.write_index = write_index;
    save.read_index = read_index;
    memcpy(save.buffer, buffer, SECRET_SIZE);

    ds_publish_mem("secret_save", (char *) &save, 
                    sizeof(struct secret_info), DSF_OVERWRITE);

    return OK;
}

PRIVATE int lu_state_restore() {
/* Restore the state. */
    struct secret_info saved;
    size_t len;

    ds_retrieve_mem("secret_save", (char *) &saved, &len);
    fd_open_counter = saved.fd_counter;
    owner_uid = saved.uid;
    owned = saved.owned;
    write_index = saved.write_index;
    read_index = saved.read_index;
    memcpy(buffer, saved.buffer, SECRET_SIZE);

    ds_delete_mem("secret_save");

    return OK;
}

PRIVATE void sef_local_startup()
{
    /*
     * Register init callbacks. Use the same function for all event types
     */
    sef_setcb_init_fresh(sef_cb_init);
    sef_setcb_init_lu(sef_cb_init);
    sef_setcb_init_restart(sef_cb_init);

    /*
     * Register live update callbacks.
     */
    /* - Agree to update immediately when LU is requested in a valid state. */
    sef_setcb_lu_prepare(sef_cb_lu_prepare_always_ready);
    /* - Support live update starting from any standard state. */
    sef_setcb_lu_state_isvalid(sef_cb_lu_state_isvalid_standard);
    /* - Register a custom routine to save the state. */
    sef_setcb_lu_state_save(sef_cb_lu_state_save);

    /* Let SEF perform startup. */
    sef_startup();
}

PRIVATE int sef_cb_init(int type, sef_init_info_t *info)
{
/* Initialize the secret driver. */
    int do_announce_driver = TRUE;

    open_counter = 0;
    switch(type) {
        case SEF_INIT_FRESH:
            fd_open_counter = 0;
            read_flag = 0;
            owned = 0;
            owner_uid = NO_ID;
            read_index = 0;
            write_index = 0;
            memset(buffer, 0, SECRET_SIZE);
            break;

        case SEF_INIT_LU:
            /* Restore the state. */
            lu_state_restore();
            do_announce_driver = FALSE;

            printf("Secret Driver: new version!\n");
        break;

        case SEF_INIT_RESTART:
            printf("Secret Driver: restarted!\n");
        break;
    }

    /* Announce we are up when necessary. */
    if (do_announce_driver) {
        driver_announce();
    }

    /* Initialization completed successfully. */
    return OK;
}

PUBLIC int main(int argc, char **argv)
{
    /*
     * Perform initialization.
     */
    sef_local_startup();

    /*
     * Run the main loop.
     */
    driver_task(&secret_tab, DRIVER_STD);
    return OK;
}


#ifndef __SECRET_H
#define __SECRET_H


#ifndef SECRET_SIZE /* Only define if not already defined */
#define SECRET_SIZE (8192) /* Default buffer size */
#endif

/** The Hello, World! message. */
#define HELLO_MESSAGE "Hello, World!\n"
#define NO_ID   (-1)

struct secret_info {
    int fd_counter;
    uid_t uid;
    int owned;
    int write_index;
    int read_index;
    char buffer[SECRET_SIZE];
};

#endif /* __SECRET_H */

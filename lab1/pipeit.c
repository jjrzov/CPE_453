#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define PERMISSIONS 0666

int main(int argc, char *argv[]){
    if (argc != 1){ // Input error checking
        perror("Invalid Input");
        exit(EXIT_FAILURE);
    }

    int pipefd[2];
    // Open file with permissions
    int outfd = open("outfile", O_CREAT | O_WRONLY | O_TRUNC, PERMISSIONS);
    if (outfd == -1){
        perror("Open Failed");
        exit(EXIT_FAILURE);
    }
    
    if (pipe(pipefd) == -1){    // Create the pipe
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork to create 2 child processes
    pid_t child_ls, child_sort;

    child_ls = fork();
    if (child_ls < 0){
        perror("Fork for ls Failed");
        exit(EXIT_FAILURE);
    
    } else if (child_ls == 0){  // LS child writes to the pipe
        close(pipefd[0]);   // Close unused read end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1){ // Redirect STDOUT to write end of pipe
            perror("dup2 Error for ls");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);   // Close write end

        char *my_argv[] = {"ls", NULL};
        execvp("ls", my_argv);  // Run ls process
        perror("Execvp for ls Failed"); // Will only reach here if execvp fails
        exit(EXIT_FAILURE);
    } 

    child_sort = fork();
    if (child_sort < 0){
        perror("Fork for Sort Failed");
        exit(EXIT_FAILURE);
    
    } else if (child_sort == 0){  // Sort child reads from the pipe
        close(pipefd[1]);   // Close unused write end

        if (dup2(pipefd[0], STDIN_FILENO) == -1){ // Redirect STDIN to read end of pipe
            perror("dup2 Error for Sort");
            exit(EXIT_FAILURE);
        }
        
        if (dup2(outfd, STDOUT_FILENO) == -1){ // Redirect STDOUT to outfile
            perror("dup2 Error for Sort");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);   // Close read end

        char *my_argv[] = {"sort", "-r", NULL};
        execvp("sort", my_argv);    // Run sort process
        perror("Execvp for Sort Failed"); // Will only reach here if execvp fails
        exit(EXIT_FAILURE);
    } 

    // Parent Process
    close(pipefd[0]);   // Close pipe
    close(pipefd[1]);

    // Wait for child processes to finish
    int status1, status2;
    wait(&status1);
    if (!WIFEXITED(status1)){
        perror("first exit failed");
        exit(EXIT_FAILURE);
    }
    wait(&status2);
    if (!WIFEXITED(status2)){
        perror("second exit failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#define MAX_LINE 80 /* The maximum length command */

void empty(char *args[], int start, int end) {
        for(int i = start; i<end; i++){
                args[i]=NULL;
        }
}

int main(void) {
        int should_run = 1;
        while(should_run) {
        char *args[MAX_LINE/2 + 1]; /* command line arguments */
        char command[MAX_LINE];
        int argc = 0;
        int len, file;
        int is_ampersand = 0;
        int pipe_index = 0;
        int input_redirect = 0;
        int output_redirect = 0;
        pid_t pid;
        printf("osh>");
        fflush(stdout);
        fgets(&command[0], MAX_LINE, stdin);
        len = strlen(&command[0]);
        command[len-1] = '\0';
        int start = 0;
        //command parsing
        for(int i = 0; i<len; i++) {
                if(command[i]=='|') {
                        pipe_index = argc;
                }
                if(command[i]=='>'){
                        output_redirect = argc;
                }
                if(command[i]=='<') {
                        input_redirect = argc;
                }
                if(command[i]==' '){
                        command[i] = '\0';
                        args[argc] = &command[start];
                        start=i+1;
                        argc++;
                }
                if(command[i]=='&'){
                        break;
                }
                if(i==len-1){
                        args[argc] = &command[start];
                        argc++;
                }
        }

        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included &
        */
        pid = fork();
        if(strcmp(args[0], "exit")==0) {
                should_run = 0;
                break;
        }
        if(pid == 0) {
            if(input_redirect!=0) {
                    file = open(args[input_redirect+1], O_RDONLY);
                    if(file == -1) {
                            printf("error\n");
                    }
                    empty(args, input_redirect, argc);
                    int p_pid = fork();
                    if(p_pid == 0) {
                            dup2(file, STDIN_FILENO);
                            execvp(args[0], args);
                    }
                    wait(NULL);
                    close(STDIN_FILENO);
                    close(file);
                    break;
            }
            else if(output_redirect!=0) {
                    file = open(args[output_redirect+1], O_WRONLY | O_CREAT, 0644);
                    if(file == -1) {
                            printf("error\n");
                    }
                    empty(args, output_redirect, argc);
                    int p_pid = fork();
                    if(p_pid>0){
                            dup2(file, STDOUT_FILENO);
                            execvp(args[0], args);
                    }
                    close(STDOUT_FILENO);
                    close(file);
                    break;
            }
            else if(pipe_index!=0) {
                    int fd[2];
                    if(pipe(fd)==-1) {
                            fprintf(stderr, "Pipe Failed\n");
                            return 1;
                    }
                    char *before[pipe_index+1];
                    char *after[argc-pipe_index];
                    before[pipe_index] = NULL;
                    after[argc-pipe_index-1] = NULL;
                    for(int j = 0; j<argc; j++){
                            if(j<pipe_index) before[j] = args[j];
                            else if(j==pipe_index) continue;
                            else after[j-pipe_index-1] = args[j];
                    }
                    int p_pid =  fork();
                    if(p_pid == 0) {
                            close(fd[0]);
                            dup2(fd[1], STDOUT_FILENO);
                            execvp(before[0], before);
                            close(fd[1]);
                    }
                    else if(p_pid>0) {
                            wait(NULL);
                            close(fd[1]);
                            dup2(fd[0], STDIN_FILENO);
                            execvp(after[0], after);
                            close(fd[0]);
                    }

            }
            else {
                    execvp(args[0], args);
            }
        }


        else {
                if(is_ampersand == 0) wait(NULL);
        }

    }
}



#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
char *args[MAX_LINE/2 + 1]; /* command line arguments */
char command[MAX_LINE];
int argc;
int chk; //check whether the operator is, default : 0, '<' : 1, '>' : 2, '|' : 3
int is_ampersand; //whether ampersand is, if & is, the value is 1
int op; //index of operator('>' or '<' or '|')


//when osh prompt runs new command, reset memories.
void reset(){
        for(int i = 0; i<argc; i++){
                args[i]=NULL;
        }
        argc = 0;
        chk = 0;
        is_ampersand = 0;
        op = 0;
}

//command parsing
void parse() {
        int len = strlen(&command[0]);
        command[len-1] = '\0';
        int start = 0;
        //command parsing
        for(int i = 0; i<len; i++) {
                if(command[i]=='|') {
                        op = argc;
                        chk=3;
                }
                if(command[i]=='>'){
                        op = argc;
                        chk=2;
                }
                if(command[i]=='<') {
                        op = argc;
                        chk=1;
                }
                if(command[i]==' '){
                        command[i] = '\0';
                        args[argc] = &command[start];
                        start=i+1;
                        argc++;
                }
                if(command[i]=='&'){
                        is_ampersand = 0;
                        break;
                }
                if(i==len-1){
                        args[argc] = &command[start];
                        argc++;
                }
        }
}

//input_redirection execution function
void in_red(){
        int file;
        file = open(args[op+1], O_RDONLY);
        if(file == -1) {
                printf("error\n");
        }
        args[op] = NULL;

        int p_pid = fork();
        if(p_pid == 0) {
                dup2(file, STDIN_FILENO);
                execvp(args[0], args);
        }
        wait(NULL);
        close(STDIN_FILENO);
        close(file);
}

//output_redirection execution function
void out_red(){
        int file;
        file = open(args[op+1], O_WRONLY | O_CREAT, 0644);
        if(file == -1) {
                printf("error\n");
        }
        args[op] = NULL;
        int p_pid = fork();
        if(p_pid>0){
                dup2(file, STDOUT_FILENO);
                execvp(args[0], args);
        }
        close(STDOUT_FILENO);
        close(file);
}

//pipe execution function
void is_pipe(){
        int fd[2];
        char *before[op+1];
        char *after[argc-op];
        if(pipe(fd)==-1) {
                fprintf(stderr, "Pipe Failed\n");
                return 1;
        }
        before[op] = NULL;
        after[argc-op-1] = NULL;
        for(int j = 0; j<argc; j++){
                if(j<op) before[j] = args[j];
                else if(j==op) continue;
                else after[j-op-1] = args[j];
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

int main(void) {
        int should_run = 1;
        while(should_run) {
        reset();
        pid_t pid;
        printf("osh>");
        fflush(stdout);
        fgets(&command[0], MAX_LINE, stdin);
        parse();
        /**
        * After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included &
        */   
        if(strcmp(args[0], "exit")==0) {
                should_run = 0;
                break;
        }
        pid = fork();
        if(pid == 0) {
            if(chk==1) {
                    in_red();
                    break;
            }
            else if(chk==2) {
                    out_red();
                    break;
            }
            else if(chk==3) {
                    is_pipe();
                        break;
            }
            else {
                    execvp(args[0], args);
            }
        }
        else {
                if(is_ampersand == 0) wait(NULL);
        }

    }
        return 0;
}




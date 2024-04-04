/*
    COMP3511 Spring 2024
    PA1: Simplified Linux Shell (MyShell)

    Your name: ZHANG Haoyue
    Your ITSC email: hzhangex@connect.ust.hk

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks.

*/

/*
    Header files for MyShell
    Necessary header files are included.
    Do not include extra header files
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h>    // For open/read/write/close syscalls
#include <ctype.h>

#define MYSHELL_WELCOME_MESSAGE "COMP3511 PA1 Myshell (Spring 2024)"

// Define template strings so that they can be easily used in printf
//
// Usage: assume pid is the process ID
//
//  printf(TEMPLATE_MYSHELL_START, pid);
//
#define TEMPLATE_MYSHELL_START "Myshell (pid=%d) starts\n"
#define TEMPLATE_MYSHELL_END "Myshell (pid=%d) ends\n"
#define TEMPLATE_MYSHELL_CD_ERROR "Myshell cd command error\n"

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LENGTH 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters:
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements,
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
// We also need to add an extra NULL item to be used in execvp
// Thus: 8 + 1 = 9
//
// Example:
//   echo a1 a2 a3 a4 a5 a6 a7
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT];
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9
#define MAX_TOTAL_ARGU MAX_ARGUMENTS_PER_SEGMENT*MAX_PIPE_SEGMENTS

// Define the standard file descriptor IDs here
#define STDIN_FILENO 0  // Standard input
#define STDOUT_FILENO 1 // Standard output

/* global */  
char curPath[MAX_CMDLINE_LENGTH];                         // current shell pwd
char *argv[MAX_TOTAL_ARGU];
int argc=0;
char backupCmdLine[MAX_CMDLINE_LENGTH]; 

// This function will be invoked by main()
void show_prompt(char *prompt, char *path)
{
    printf("%s %s> ", prompt, path);
}

// This function will be invoked by main()
// This function is given
int get_cmd_line(char *cmdline)
{
    int i;
    int n;
    if (!fgets(cmdline, MAX_CMDLINE_LENGTH, stdin))
        return -1;
    // Ignore the newline character
    n = strlen(cmdline);
    cmdline[--n] = '\0';
    i = 0;
    while (i < n && cmdline[i] == ' ')
    {
        ++i;
    }
    if (i == n)
    {
        // Empty command
        return -1;
    }
    return 0;
}

void parse(char *line) {
    // for "ls -a", argv[0]="ls" and argv[1]="-a")
    strcpy(backupCmdLine, line);
    int flg = 0;
    for (int i = 0; backupCmdLine[i] != '\0'; i++) {
        if (flg == 0 && !isspace(backupCmdLine[i])) {
            flg = 1;
            argv[argc] = backupCmdLine + i;
            argc++;
        } else if (flg == 1 && isspace(backupCmdLine[i])) {
            flg = 0;
            backupCmdLine[i] = '\0';
        }
    }
}

// parse_arguments function is given
// This function helps you parse the command line
//
// Suppose the following variables are defined:
//
// char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
// int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
// char cmdline[MAX_CMDLINE_LENGTH]; // The input command line
//
// Sample usage:
//
//  parse_arguments(pipe_segments, cmdline, &num_pipe_segments, "|");
//
void parse_arguments(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
    if (argc > MAX_PIPE_SEGMENTS){
        printf("Too Much PIPE_SEGMENTS!\n");
    }
    argv[argc] = NULL;
}

int exeCD() {
    int flag = 1;   // 1:suceessed, 0:failed 
    if (argc > 2) {
        printf("Myshell cd commond error!\n");
        printf("Too many arguments.\n");
    } else if (argc == 1) {
        return flag;
    }
    else {
        int ret = chdir(argv[1]);
        if (ret) {
            printf("Myshell cd commond error!\n");
            printf("No such file or directory.\n");
        }
    }

    if (flag) {
        char* res = getcwd(curPath, MAX_CMDLINE_LENGTH);
        if (res == NULL) {
            printf("Myshell cd commond error!\n");
            printf("No such file or directory.\n");
        }
        return flag;
    }
    return 0;
}

int InputOutputRedirection(char *cmdline, int input_Redire_flag, int output_Redire_flag) {
    char inFile[MAX_CMDLINE_LENGTH];
    char outFile[MAX_CMDLINE_LENGTH];
    memset(inFile, 0x00, MAX_CMDLINE_LENGTH);
    memset(outFile, 0x00, MAX_CMDLINE_LENGTH);

    // find input file name and "<" loc; find output file name and ">" loc
    int divideLoc_input = MAX_TOTAL_ARGU;
    int divideLoc_output = MAX_TOTAL_ARGU;
    char *argvBackUp[MAX_TOTAL_ARGU];
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0) {
            if (i + 1 < argc) {
                strcpy(inFile, argv[i+1]);
                divideLoc_input = i;
            } else {
                printf("Lack of input!");
                return 0;
            }
        } else if (strcmp(argv[i], ">") == 0 ){
            if (i + 1 < argc) {
                strcpy(outFile, argv[i+1]);
                divideLoc_output = i;
            } else {
                printf("Lack of output!");
                return 0;
            }
        }   
    }
    // divide command in argvBackUp
    for (int i = 0; i < argc; i++) {
        if (i < divideLoc_input && i < divideLoc_output) {
            argvBackUp[i] = argv [i];
        }
        else {
            argvBackUp[i] = NULL;
        }
    }

    // process redirection
    pid_t pid;
    switch(pid = fork()) {
        case -1: {
            printf("Fail in creating child process!");
            return 0;
        }
        case 0: {
            // only input redirection
            if (input_Redire_flag == 1 && output_Redire_flag == 0) {  
                int fd = open(inFile, O_RDONLY);
                if (fd < 0) {
                    printf("Fail in opening file: %s\n",inFile);
                    exit(1);
                }
                dup2(fd, 0);
                close(fd);
                int stdout_backup = dup(1);
                close(1);
                dup(stdout_backup);
                close(stdout_backup);
                execvp(argvBackUp[0], argvBackUp);
                exit(0);
            } else if (input_Redire_flag == 0 && output_Redire_flag == 1) {  // only output redirection
                int fd = open(outFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                if (fd < 0) {
                    printf("Fail in opening file: %s\n",outFile);
                    exit(1);
                }
                dup2(fd, 1);
                close(fd);
                execvp(argvBackUp[0], argvBackUp);
                exit(0);
            } else { // input and output redirection
                if (divideLoc_input < divideLoc_output) {   // first input then output
                    int fd = open(inFile, O_RDONLY);
                    if (fd < 0) {
                        printf("Fail in opening file: %s\n",inFile);
                        exit(1);
                    }
                    dup2(fd, 0);
                    close(fd);

                    int output_fd = open(outFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                    if (output_fd < 0) {
                        printf("Fail in opening file: %s\n",outFile);
                        exit(1);
                    }
                    dup2(output_fd, 1);
                    close(output_fd);
                    execvp(argvBackUp[0], argvBackUp);
                    exit(0);
                } else {   // first output then input
                    int output_fd = open(outFile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
                    if (output_fd < 0) {
                        printf("Fail in opening file: %s\n",outFile);
                        exit(1);
                    }
                    dup2(output_fd, 1);
                    close(output_fd);

                    int fd = open(inFile, O_RDONLY);
                    if (fd < 0) {
                        printf("Fail in opening file: %s\n",inFile);
                        exit(1);
                    }
                    dup2(fd, 0);
                    close(fd);
                    execvp(argvBackUp[0], argvBackUp);
                    exit(0);
                }
            }
        }
        default: {
            wait(0);       // wait child
        } 
    }                        
}

int process_pipe(char **pipe_segments, int num_pipe_segments)
{
    int pipefd[MAX_PIPE_SEGMENTS - 1][2]; 
    pid_t pids[MAX_PIPE_SEGMENTS]; 
    char* commands[MAX_PIPE_SEGMENTS];
    int num_command = 0;
    char space[2] = {' ', '\t'};

    //create pipe
    for (int i = 0; i < num_pipe_segments - 1; i++) {
        if (pipe(pipefd[i]) == -1) {
            printf("Fail in creating pipe !\n");
            return 0;
        }
    }
    for (int j=0;j<MAX_PIPE_SEGMENTS;j++) {
        commands[j] = NULL;
    }

    // fork and exe
    for (int i = 0; i < num_pipe_segments; i++) {
        pids[i] = fork();
        num_command = 0;
        if (pids[i] < 0) {
            printf("Fail in creating child process!");
            return 0;
        } else if (pids[i] == 0) {
            // for (int j=0;j<MAX_PIPE_SEGMENTS;j++) {
            //     printf("%d:,%s\n",i,pipe_segments[j]);
            // }
            parse_arguments(commands, pipe_segments[i], &num_command, space);
            if ( i == 0 ){
                // first child process，write in pipe
                dup2(pipefd[i][1], STDOUT_FILENO); // output redirection
                // dup(pipefd[i][1]);
                close(pipefd[i][0]);
                // close(pipefd[i][1]);
            } else if (i == num_pipe_segments - 1) {
                // final child process，read in pipe
                dup2(pipefd[i-1][0], STDIN_FILENO); // input redirection
                // dup(pipefd[i-1][0]);
                close(pipefd[i-1][0]);
                close(pipefd[i-1][1]);
            } else {
                // midle child process，read from i-1 pipe and write to i pipe
                dup2(pipefd[i-1][0], STDIN_FILENO); // input redirection
                dup2(pipefd[i][1], STDOUT_FILENO); // output redirection

                close(pipefd[i-1][0]);
                close(pipefd[i-1][1]);
                close(pipefd[i][0]);
                // close(pipefd[i][1]);
            }
            for (int j = 0; j < num_pipe_segments - 1; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }
            // for (int j=0;j<MAX_ARGUMENTS;j++) {
            //     printf("i=%d,j=%d,%s\n",i,j,commands[j]);
            // }

            execvp(commands[0], commands);// execute command
            exit(0);
        } 
    }

    for (int j = 0; j < num_pipe_segments - 1; j++) {
        close(pipefd[j][0]);
        close(pipefd[j][1]);
    }

    for (int i = 0; i < num_pipe_segments; i++) {
        waitpid(pids[i], NULL, 0);
    }

    return 0;
}

void process_cmd(char *cmdline)
{
    // Uncomment this line to show the content of the cmdline
    // printf("cmdline is: %s\n", cmdline);
    
    // input and output redirection
    int input_Redire_flag = 0;
    int output_Redire_flag = 0;
    int pip_flag = 0;

    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
    for (int i=0;i<MAX_PIPE_SEGMENTS;i++) {
        pipe_segments[i] = NULL;
    }

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "<") == 0) {
            // printf("input redi:%s\n",cmdline);
            input_Redire_flag = 1;
        }
        if (strcmp(argv[i], ">") == 0) {
            // printf("output redi:%s\n",cmdline);
            output_Redire_flag = 1;
        }
    }
    if (input_Redire_flag == 1 || output_Redire_flag == 1){
        InputOutputRedirection(cmdline,input_Redire_flag,output_Redire_flag);
    }

    for (int i = 0; i < strlen(cmdline); i++) {
        if (cmdline[i] == '|') {
            pip_flag = 1;
            break;
        }
    }

    if (pip_flag == 1) {
        // printf("run pip:%s\n",cmdline);
        parse_arguments(pipe_segments, cmdline, &num_pipe_segments, "|");
        // for (int i=0;i<MAX_PIPE_SEGMENTS;i++) {
        //     printf("%d:,%s\n",i,pipe_segments[i]);
        // }
        int res = process_pipe(pipe_segments, num_pipe_segments);
    }

    if (input_Redire_flag == 0 && output_Redire_flag == 0 && pip_flag == 0){
        if (argc > MAX_ARGUMENTS){
            printf("Too Much Arguments!\n");
            exit(0);
        }
        execvp(argv[0],argv);
    }

    exit(0); // ensure the process cmd is finished
}

/* The main function implementation */
int main()
{
    char *prompt = "hzhangex";
    char cmdline[MAX_CMDLINE_LENGTH];
    char path[256]; // assume path has at most 256 characters
    // inatialize argv and argc

    printf("%s\n\n", MYSHELL_WELCOME_MESSAGE);
    printf(TEMPLATE_MYSHELL_START, getpid());

    // The main event loop
    while (1)
    {
        getcwd(path, 256);
        show_prompt(prompt, path);
        argc = 0;
        for(int i=0;i<MAX_TOTAL_ARGU;i++){
            argv[i] = NULL;
        }

        if (get_cmd_line(cmdline) == -1)
            continue; // empty line handling, continue and do not run process_cmd

        // parse_arguments(argv, cmdline, &numTokens, " ");
        parse(cmdline);

        // (1) Handle the exit command
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }
        // (2) Handle the cd command
        if (strcmp(argv[0], "cd") == 0) {
            exeCD();
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            // the child process handles the command
            process_cmd(cmdline);
            exit(0);
        }
        else
        {
            // the parent process simply wait for the child and do nothing
            wait(0);
        }
    }

    return 0;
}

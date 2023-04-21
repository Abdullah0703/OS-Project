#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_NUM_ARGS 10

// Function to handle signals
void sigintHandler(int signum) {
     signal(SIGINT, sigintHandler);
}

int main() {
    printf("Unix Shell by K21-3843, K20-1901, and K21-3886\n");
    signal(SIGINT, sigintHandler);

    while (1) {
        char input[MAX_COMMAND_LENGTH];
        char *args[MAX_NUM_ARGS];
        int inputRedirect = 0; 
        int outputRedirect = 0; 
        char inputFile[MAX_COMMAND_LENGTH]; 
        char outputFile[MAX_COMMAND_LENGTH];

        printf("Shell> ");
        fgets(input, MAX_COMMAND_LENGTH, stdin);
	int num_args = 0;
        char *token = strtok(input, " \n"); 
        while (token != NULL && num_args < MAX_NUM_ARGS - 1) {
            if (strcmp(token, "<") == 0) {
                token = strtok(NULL, " \n");
                strcpy(inputFile, token);
                inputRedirect = 1;
            } else if (strcmp(token, ">") == 0) {
                token = strtok(NULL, " \n");
                strcpy(outputFile, token);
                outputRedirect = 1;
            } else {
                args[num_args++] = token;
            }
            token = strtok(NULL, " \n");
        }
        args[num_args] = NULL;

        if (num_args == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            exit(0);
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (inputRedirect) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) {
                    printf("Error opening input file: %s\n", inputFile);
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }

            if (outputRedirect) {
                int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) {
                    printf("Error opening output file: %s\n", outputFile);
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
	    int i;
            int pipeIndex = -1; 
            for (i = 0; i < num_args; i++) {
                if (strcmp(args[i], "|") == 0) {
                    pipeIndex = i;
                    break;
                }
            }
            if (pipeIndex >= 0) {
                args[pipeIndex] = NULL; 
            int pipefd[2];

            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(1);
            }
		pid_t pid1 = fork();
            if (pid1 == 0) {
                close(pipefd[0]); 
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(1);
                }
            } else if (pid1 > 0) {
                pid_t pid2 = fork();
                if (pid2 == 0) {

                    close(pipefd[1]); 
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]); 
			if (execvp(args[pipeIndex + 1], args + pipeIndex + 1) == -1) {
                        perror("execvp");
                        exit(1);
                    }
                } else if (pid2 > 0) {
                    close(pipefd[0]); 
                    close(pipefd[1]);
                    waitpid(pid1, NULL, 0);
                    waitpid(pid2, NULL, 0);
                } else {
                    perror("fork");
                    exit(1);
                }
            } else {
                perror("fork");
                exit(1);
            }
        } else {
            if (execvp(args[0], args) == -1) {
                perror("execvp");
                exit(1);
            }
        }
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
        exit(1);
    }
}

return 0;

}


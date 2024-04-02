
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<errno.h>
#include<fcntl.h>

char prompt[256];
char *currentPath;
char* PATH  = "/usr/bin:/bin:/sbin";
char cwd[256];
           
pid_t running_pid = -1;

#define MAX_ARG 10
#define HISTORY_SIZE 10
char *history[HISTORY_SIZE];
int history_count = 0;

void add_to_history(char *command) {
    if (history_count < HISTORY_SIZE) {
        history[history_count++] = strdup(command);
    } else {
        free(history[0]);
        for (int i = 0; i < HISTORY_SIZE - 1; ++i) {
            history[i] = history[i + 1];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}

void print_history() {
    printf("Command history:\n");
    for (int i = 0; i < history_count; ++i) {
        printf("%d. %s\n", i + 1, history[i]);
    }
}

void sigint_handler(int signum) {
    if (running_pid != -1) {
        kill(running_pid, SIGKILL); 
        printf("\n");
        //printf("\nProcess terminated.\n");
        fflush(stdout); 
    } else {
        printf("\n"); 
        printf("%s", prompt); 
        fflush(stdout); 
    }
}
void execute_command(char* command, char *argv[]){
     printf("Adding command to history: %s\n", command);
     add_to_history(command);

       
    int num_pipes = 0;
    int pipe_positions[MAX_ARG];
    pipe_positions[0] = -1;

    
    for (int i = 0; argv[i] != NULL; ++i) {
        if (strcmp(argv[i], "|") == 0) {
            pipe_positions[++num_pipes] = i;
        }
    }
    pipe_positions[++num_pipes] = -1;

 
    int pipes[MAX_ARG - 1][2];
    for (int i = 0; i < num_pipes; ++i) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

        
    /*   if(pid==0){
       
             char *args[128];
             char *token;
      	     
             int i = 0;
             token = strtok(command, " ");
		while (token != NULL) {
		    if (strcmp(token, ">") == 0) {
		        
		        token = strtok(NULL, " ");  
		        int fd = open(token, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		        if (fd == -1) {
		            perror("Error in output redirection");
		            exit(EXIT_FAILURE);
		        }
		        dup2(fd, STDOUT_FILENO);
		        close(fd);
		        break;
		    } else if (strcmp(token, "<") == 0) {
		        
		        token = strtok(NULL, " ");  
		        int fd = open(token, O_RDONLY);
		        if (fd == -1) {
		            perror("Error in input redirection");
		            exit(EXIT_FAILURE);
		        }
		        dup2(fd, STDIN_FILENO);
		        close(fd);
		        break;
		    } else {
		        args[i++] = token;
		        token = strtok(NULL, " ");
		    }
		}
        
               char * path2 = strdup(PATH);
               char* directory = strtok(path2, ":");
             
               while(directory != NULL){
                   char arg[128];
                   snprintf(arg, sizeof(arg), "%s/%s", directory, argv[0]);
                   execv(arg, argv);
                   directory = strtok(NULL, ":");
               }
               if (execv(argv[0], argv) == -1) {
           	   fprintf(stderr, "Command not found: %s\n", argv[0]);
            	   free(path2);
            	   exit(EXIT_FAILURE);
               }
         
       }
       else if(pid>0){
            running_pid = pid; 
            wait(NULL);
            running_pid = -1;
       }
       else{
             perror("Fork Failed");
             exit(EXIT_FAILURE);
       }*/
        pid_t pid;
    int prev_pipe_read = -1;
    for (int i = 0; i < num_pipes + 1; ++i) {
        pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { 
            if (i > 0) { 
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
            }
            if (i < num_pipes) { 
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][1]);
            }

            
            for (int j = 0; j < num_pipes; ++j) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *cmd[MAX_ARG];
            int cmd_start = (i == 0) ? 0 : pipe_positions[i - 1] + 1;
            int cmd_end = (i == num_pipes) ? MAX_ARG - 1 : pipe_positions[i];
            for (int j = cmd_start; j < cmd_end; ++j) {
                cmd[j - cmd_start] = argv[j];
            }
            cmd[cmd_end - cmd_start] = NULL;
            execvp(cmd[0], cmd);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { 
            running_pid = pid;
            if (i > 0) { 
                close(pipes[i - 1][0]);
            }
            //waitpid(pid, NULL, 0);
            //running_pid = -1;
            //exit(EXIT_SUCCESS);
        }
    }

    for (int i = 0; i < num_pipes; ++i) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }


    for (int i = 0; i < num_pipes + 1; ++i) {
        wait(NULL);
    }
    running_pid = -1;
}

void changePrompt(char* prompt2){
      strcpy(prompt, prompt2);
}

void changeDirectory(char *newDir) {
    while (*newDir == ' ') {
        newDir++;
    }
    char *end = newDir + strlen(newDir) - 1;
    while (end > newDir && *end == ' ') {
        end--;
    }
    end[1] = '\0';

    if (chdir(newDir) != 0) {
        perror("chdir");
    }
}

void setPath(char *newPath) {
    PATH = newPath;
}

int main(){
       //int pid;
       signal(SIGINT, sigint_handler);
      
       char command[128];
       char* arguments[10];
       
       char *currentPath = getcwd(cwd, sizeof(cwd));
       if (currentPath == NULL) {
            perror("getcwd");
            return EXIT_FAILURE;
       }

       snprintf(prompt, sizeof(prompt), "%s> ", currentPath);
       
       while(1){
            
            printf("%s", prompt);
            fgets(command, sizeof(command), stdin);
            
            command[strcspn(command, "\n")] = '\0';
            if (strcmp(command, "exit") == 0) {
               add_to_history(command);
            	printf("Exiting shell.\n");
            	break;
            }
            if (strcmp(command, "history") == 0) {
               add_to_history(command);
               print_history();
               continue;
            }

            if(strncmp(command, "PS1=", 4) == 0){
                add_to_history(command);
                if(strcmp(command+4, "\\w$") == 0){
                       char *currentPath = getcwd(cwd, sizeof(cwd));
		       if (currentPath == NULL) {
			    perror("getcwd");
			    return EXIT_FAILURE;
		       }

		       snprintf(prompt, sizeof(prompt), "%s> ", currentPath);
                }
                else{
                
                    changePrompt(command+4);
                }
                continue;
            }
            
            if(strncmp(command, "echo $PATH", 10) == 0){
                add_to_history(command);
                char* path_env = getenv("PATH");
                printf("%s\n", path_env);
                continue;
            }
            
            if (strncmp(command, "PATH=", 5) == 0) {
               add_to_history(command);
            	setPath(command + 5);
            	continue;
            }
            
            if (strncmp(command, "cd", 2) == 0) {
               add_to_history(command);
    		char *newDir = strtok(command + 2, " \n");
    		if (newDir != NULL) {
        		changeDirectory(newDir);
        	       char *currentPath = getcwd(cwd, sizeof(cwd));
		       if (currentPath == NULL) {
			    perror("getcwd");
			    return EXIT_FAILURE;
		       }

		       snprintf(prompt, sizeof(prompt), "%s> ", currentPath);
        		continue;
    	    	} else {
        		fprintf(stderr, "Usage: cd <directory>\n");
            	}
            }
            
            int i = 0;
            arguments[i] = strtok(command, " ");
            i++;
            while (i < MAX_ARG - 1 && arguments[i-1] != NULL) {
            	arguments[i] = strtok(NULL, " ");
            	i++;
            }
            
            arguments[i] = NULL;
       
            execute_command(command, arguments);
        }
        
        return 0;

}

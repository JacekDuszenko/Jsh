#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <ctype.h>
#include<sys/wait.h>
#include <unistd.h>
#include<errno.h>
#include<signal.h>

#define MAX_DIR_LENGTH 512
#define MAX_LINE_LENGTH 1024 //buffer size while fetching the entire line
#define MAX_ARGS_COUNT 64 //buffer size while parsing word count 
#define DELIMITERS " \t\r\n\a"
#define ZOMBIES_SIZE 64

int lsh_cd(char ** args);
int lsh_exit(char ** args);

int has_pipe(char ** args) 
{
    char ** tmp = args;
    int i=0;
    while(tmp[i] != NULL)
    {
        if(strcmp(tmp[i],"|")==0) {
        return 1;
    }
    i++;
    }
    return 0;
}

void print_words(char **args)
{
    char ** tmp = args;
    int ctr = 0;
    while(tmp[ctr] != NULL)
    {
            printf("%s \n", tmp[ctr]);
            ctr ++;        
    }
}

char * builtin_str[] = 
{
    "cd",
    "exit"
};

int (*builtin_func[]) (char **) =  //very complicated stuff
{
    &lsh_cd,
    &lsh_exit,
};

int lsh_num_builtins() 
{
    return sizeof(builtin_str) / sizeof(char *);
}

int lsh_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "lsh: zapomniales katalogu!\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("lsh");
    }
  }
  return 1;
}

int lsh_exit(char ** args)
{
    return 0;
}

char *lsh_read_line(void)
{
  char *line = NULL;
  size_t bufsize = MAX_LINE_LENGTH;
  int handle_EOF = getline(&line, &bufsize, stdin);

  if(handle_EOF == -1)
  {
      exit(0);
  }
  return line;
}

char ** lsh_parse(char * line) 
{
    int bufsize = MAX_ARGS_COUNT;
    int position = 0;

    char **arguments = malloc(bufsize * sizeof(char*));
    char * argument;

    argument = strtok(line, DELIMITERS);
    while(argument != NULL)
    {
        arguments[position] = argument;
        position++;
        argument = strtok(NULL, DELIMITERS);
    }

    arguments[position] = NULL;
    return arguments;
}

int find_size(char **args)
{
    char ** tmp = args;
    int ctr = 0;
    while(1)
    {
        if(tmp[ctr] == NULL)
        {
            return ctr;
        }
        ctr ++;
    }
}

int find_index_of_input_redirect(char ** args)
{
    char ** tmp = args;
    int i=0;
    for(i;i<find_size(args);i++) 
    {
        if(strcmp(tmp[i],"<") == 0)
        {
            return i;
        }
        i++;
    }
}

int lsh_launch( char **args, int is_in, int is_out, int is_err)
{
    pid_t pid, wpid;
    int status;
    pid = fork();
    int size = find_size(args);
    int isAmpersand = strcmp(args[size-1], "&") == 0 ? 1 : 0; 

    if(isAmpersand) 
    {
       args[size-1] = NULL;
    }
   
    if(pid == 0) //CHILD WITHOUT OR WITH AMPERSAND 
    {
        if(is_out) {
            freopen(args[size-1], "w+", stdout);
            args[size-1] = NULL; //delete file
            args[size-2] = NULL; //delete ">"
           
        }
         else if(is_err) {
            freopen(args[size-1], "w+", stderr);
            args[size-1] = NULL;  //delete file
            args[size-2] = NULL; // delete "2>"
        }

         if(is_in) {             //copy everything except first element
           int idx_of_sign = find_index_of_input_redirect(args);           
            freopen(args[idx_of_sign+1], "r", stdin);
            char ** arguments = malloc(MAX_ARGS_COUNT * sizeof(char *));
          
            arguments[0] = args[0];     //this needs to be done because of copy condition. I couldn't figure out anything smarter at 4 a.m.
            int pos = 1;
            for(int i = 1;i<find_size(args)-1;i++)
            {
                if((strcmp(args[i],"<") != 0) && (strcmp(args[i-1], "<") != 0))                {
                    arguments[pos] = args[i];
                    pos++;
                }
            }

            execvp(arguments[0],arguments);
        } else {
            execvp(args[0],args);
        }
        
     if(execvp(args[0],args) == -1)
        {
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
     else if(pid < 0) //ERROR
    {
        printf("error forking");
        perror("lsh");
    } else if(pid > 0 && isAmpersand) //PARENT WITH AMPERSAND - skip waitpid
    {
        return 1;
    }
    else
    { //PARENT WITHOUT AMPERSAND
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

void clean_zombies() 
{
    waitpid(-1, NULL, WNOHANG);    //-1 every child, WNOHANG - proceed if not zombie yet
}

int count_pipes(char ** args) {

char ** tmp = args;

int i=0;
int ctr = 0;
while(tmp[i] != NULL) {

    if(strcmp(tmp[i], "|") == 0) {
    ctr++;
    }
    i++;
    }
    return ctr;
}

int has_stdout_red(char ** args) {        //redirect means  output goes for example to file
char ** tmp = args;
    int i=0;
    while(tmp[i] != NULL)
    {
        if(strcmp(tmp[i],">")==0) {
        return 1;
    }
    i++;
    }
    return 0;
}

int has_stdin_red(char ** args) {     
char ** tmp = args;
    int i=0;
    while(tmp[i] != NULL)
    {
        if(strcmp(tmp[i],"<")==0) {
        return 1;
    }
    i++;
    }
    return 0;
}

int has_stderr_red(char ** args) 
{
    char ** tmp = args;
    int i=0;
    while(tmp[i] != NULL)
    {
        if(strcmp(tmp[i],"2>") == 0) {
        return 1;
    }
    i++;
    }
    return 0;
}

char ** format_args_taking_inout(char ** args, int input, int output) {
int size = find_size(args);
if(output) {
    args[size-1] = NULL;
    args[size-2] = NULL;
}
if (input) {
        int idx_of_sign = find_index_of_input_redirect(args);           
            char ** arguments = malloc(MAX_ARGS_COUNT * sizeof(char *));
          
            arguments[0] = args[0];     //this needs to be done because of copy condition. I couldn't figure out anything smarter at 4 a.m.
            int pos = 1;
            for(int i = 1;i<find_size(args)-1;i++)
            {
                if((strcmp(args[i],"<") != 0) && (strcmp(args[i-1], "<") != 0))                {
                    arguments[pos] = args[i];
                    pos++;
                }
            }
        return arguments;
}
return args;
}


int lsh_pipe_handler(char **args, int input_red, int output_red) {

char * output_file;
char * input_file;

if(output_red) {
int size = find_size(args);
output_file = args[size-1];
}                                                                               //determine output/input file for pipe

if(input_red) {
    int idx_of_sign = find_index_of_input_redirect(args); 
    input_file = args[idx_of_sign+1];
}

char ** list_of_commands = format_args_taking_inout(args, input_red, output_red);

	int fd1[2], fd2[2];
	int num_of_commands = 0;
	char *command[256];
	pid_t pid;
	int boolean_end = 0;
	
    int MAIN_CLOCK = 0;
	int place = 0;
	int position = 0;
	int counter = 0;
	
	while (list_of_commands[counter] != NULL) {
		if (strcmp(list_of_commands[counter], "|") == 0)
			num_of_commands++;
		counter++;
	}
	num_of_commands++;
	
	while (list_of_commands[place] != NULL && boolean_end != 1)
	{
		position = 0;
		while (strcmp(list_of_commands[place],"|") != 0)
		{
			command[position] = list_of_commands[place];
			place++;	
			if (list_of_commands[place] == NULL)
			{
                boolean_end = 1;
				position++;
				break;
			}
			position++;
		}
	
		command[position] = NULL;
		place++;		
		

		if (MAIN_CLOCK% 2 != 0) {
			pipe(fd1); 		//for odd create pipe on first fd array
		}
		else {
			pipe(fd2); 		//for even create pipe on second fd array
		}
		
		pid=fork();
		
		if(pid==-1) {			
			if (MAIN_CLOCK!= num_of_commands - 1) {
				if (MAIN_CLOCK % 2 != 0)
					close(fd1[1]); 
				else
					close(fd2[1]); 
			}
			return 1;
		}

		if(pid==0)
		{
            if(input_red == 1 && MAIN_CLOCK ==0 ) {
                freopen(input_file,"r", stdin);
            }

            if(output_red == 1 && MAIN_CLOCK == counter) {
                freopen(output_file,"w+", stdout);
            }

            if(num_of_commands)
			if (MAIN_CLOCK == 0)
				dup2(fd2[1], STDOUT_FILENO);
			else if (MAIN_CLOCK== num_of_commands - 1)
			{
				if (num_of_commands % 2 != 0)
					dup2(fd1[0], STDIN_FILENO);
				else
		
					dup2(fd2[0], STDIN_FILENO);
			}
			else {
				if (MAIN_CLOCK % 2 != 0) {
					dup2(fd2[0], STDIN_FILENO); 
					dup2(fd1[1], STDOUT_FILENO);
				}
				else {
					dup2(fd1[0], STDIN_FILENO); 
					dup2(fd2[1], STDOUT_FILENO);					
				} 
			}
			if (execvp(command[0], command) == -1) {
				kill(getpid(), SIGTERM);
			}		
		}
				
		if (MAIN_CLOCK== 0)
			close(fd2[1]);
		else if (MAIN_CLOCK == num_of_commands - 1) {
			if (num_of_commands % 2 != 0)
				close(fd1[0]);
			else
				close(fd2[0]);
		}
		else {
			if (MAIN_CLOCK % 2 != 0) {					
				close(fd2[0]);
				close(fd1[1]);
			}
			else {					
				close(fd1[0]);
				close(fd2[1]);
			}
		}		
		waitpid (pid, NULL, 0);		
		MAIN_CLOCK++;	
	}
    return 1;
}

int lsh_execute(char ** args)
{
    int stdin_red = has_stdin_red(args);
    int stdout_red = has_stdout_red(args);
    int stderr_red = has_stderr_red(args);

    int i;

    if(args[0] == NULL) 
    {
        return 1;
    }

    for(int i=0;i<lsh_num_builtins();i++)
    {
        if(strcmp(args[0], builtin_str[i]) == 0) //strings match
        {
            return (*builtin_func[i])(args);
        }
    }

    if(count_pipes(args) <= 0) {
        return lsh_launch(args, stdin_red, stdout_red, stderr_red);
    } else {
        return lsh_pipe_handler(args, stdin_red, stdout_red);
    }

}

 void lsh_loop()
 {
     char * line;
     char ** args;
     int status;
    char cwd[MAX_DIR_LENGTH];
   

     do {
         getcwd(cwd, sizeof(cwd));
         printf("\e[32mGreenJacek's shell: %s $", cwd);
         line = lsh_read_line();
         args = lsh_parse(line);
         status = lsh_execute(args);

         clean_zombies();
         free(line);
         free(args);
     } while(status);
 }

void intHandler(int state) {
    fprintf(stderr, "push ctr-d to exit");
    lsh_loop();
}

 int main() {
     signal(SIGINT, intHandler);
     lsh_loop();
 }



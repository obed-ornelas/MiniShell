#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "ourhdr.h"
#include <stdbool.h>
#include <dirent.h>

#define INPUT_END 0
#define OUTPUT_END 1
#define FIRST_COMMAND 0

extern int makeargv(char *, char * , char ***);
int getNumOfPipes(int, char**);
int findPipes(int,char **, int*);
bool isBuiltinCMD(char **, int);
void execBuiltinCMD(char **);

int main()  
{
    char **argArray; // THE POINTER TO THE ARRAY OF POINTERS TO BE FILLED BY
    int i,NUM_COMMANDS,pipeCnt,status, numOfTokens;
    int LAST_COMMAND_LOC, LAST_COMMAND;
    char buf[80]; // command line
    pid_t pid; // process ids
    int leftPipe[2], rightPipe[2];
    
    write(1,"\nMyShellPrompt--> ",18);

    while(strcmp(fgets(buf, 80, stdin), "exit\n")!=0)  // USER COMMAND
    { 
        numOfTokens = makeargv(buf," \n",&argArray); // get NUMBER of TOKENS
        pipeCnt = getNumOfPipes(numOfTokens,argArray);
        NUM_COMMANDS = pipeCnt + 1;       
        LAST_COMMAND = NUM_COMMANDS - 1;
        
        int pipeLocArray[pipeCnt]; 
        LAST_COMMAND_LOC = findPipes(numOfTokens,argArray,pipeLocArray);
        
        pipe(leftPipe);
        pipe(rightPipe);   
        
        if(isBuiltinCMD(argArray,NUM_COMMANDS))//only built in command is CD
        {
            execBuiltinCMD(argArray);
        }
        else
        {
            for(i=0; i < NUM_COMMANDS; i++)
            {
                printf("Loop counter : %i\n",i);
                if((pid = fork()) == -1)
                err_sys("\nfork failed");
                    
                if(pid == 0)
                {
                // replace stdout with write part of 1st pipe
                //Write to process ouputrightPipe[OUTPUT_END]
                    if(i == FIRST_COMMAND && NUM_COMMANDS > 1)//no need to pipe for 1 command only
                    {//1st command has no left pipe. 
                        close(leftPipe[INPUT_END]);
                        dup2(leftPipe[OUTPUT_END],1);//We write to pipe to the right of 1st command
                        //which happens to be named leftPipe
                    }
                
                    if(i == LAST_COMMAND)
                    {//last command will only have one pipe to the left
                        if((i+1) == NUM_COMMANDS)
                        {
                            close(leftPipe[OUTPUT_END]);
                            dup2(leftPipe[INPUT_END],0);//read input to exec
                            close(rightPipe[OUTPUT_END]);
                            close(rightPipe[INPUT_END]);
                        }
                    }
                
                    if(i!= FIRST_COMMAND && i!= LAST_COMMAND )//middle commands
                    {//link pipes
                        close(leftPipe[OUTPUT_END]);
                        dup2(leftPipe[INPUT_END], 0);//read input of pipe to left of command                    
                        close(rightPipe[INPUT_END]);
                        dup2(rightPipe[OUTPUT_END], 1);//write to output of pipe to right of command
                    }

                    if(i == FIRST_COMMAND)
                    {
                        execvp(argArray[0], &argArray[0]);
                        err_sys("1st exec fd up");
                    }  
                    else if(i == LAST_COMMAND)
                    {
                        execvp(argArray[LAST_COMMAND_LOC],&argArray[LAST_COMMAND_LOC]);
                        err_sys("other exec fd up");
                    }
                    else//execute middle commands
                    {
                        execvp(argArray[pipeLocArray[i-1]+1],&argArray[pipeLocArray[i-1]+1]);
                        err_sys("middle exec fd up");
                    }
                    exit(1);
                }
                if(pid > 0)
                {
                    printf("in parent\n");
                    if(i != FIRST_COMMAND)//first command will not have a pipe to the left                    
                    {
                        close(leftPipe[INPUT_END]);
                        close(leftPipe[OUTPUT_END]);
                    }
                    //last command will not have a pipe to the right
                    if(i != LAST_COMMAND && i != FIRST_COMMAND)//for middle commands
                    {
                        //pipe to the right of previous command will now be left
                        //pipe of new middle command 
                        leftPipe[INPUT_END] = rightPipe[INPUT_END]; 
                        leftPipe[OUTPUT_END] = rightPipe[OUTPUT_END];
                    }                                   

                    waitpid(pid,&status,0);
                }
            }//end of (not built in command)
        }//out of for loop        
        write(1,"\nMyShellPrompt--> ",18);           
    }//out of while loop
}

int getNumOfPipes(int numOfArgs, char **argArray)
{
    int i = 0; int numOfPipes = 0;
    
    for (;i < numOfArgs;i++)
    {
        if(strncmp("|",argArray[i],1) == 0)
        {
	    argArray[i] = '\0';
	    numOfPipes++;
        }
    }    
    return numOfPipes;
}

int findPipes(int numOfArgs,char **cmdArray, int *pipeLoc)
{
    int lastCMD = 0;
    int i,j = 0;
    for(i=0; i < numOfArgs; i++)
    {		
	if(cmdArray[i] == '\0')
	{
            pipeLoc[j] = i;
            j++;
	}
    }
    lastCMD = pipeLoc[j-1]+1;
    return lastCMD;
}

bool isBuiltinCMD(char **cmd_array, int num_cmds)
{
    if(num_cmds != 1)
    {
        return false;
    }
    else
    {
        if(strncmp("cd",cmd_array[0],1) == 0)
            return true;
        else
            return false;
    }    
}

void execBuiltinCMD(char **cmd_array)
{
    DIR *dp;
    
    if ((dp = opendir(cmd_array[1])) == NULL)
    err_sys("can't open %s\n", cmd_array[1]); // now we know we have a directory

    chdir(cmd_array[1]); // This process now has a different "current working directory"
    printf("\nIn DIRECTORY -> /%s\n",cmd_array[1]);
}

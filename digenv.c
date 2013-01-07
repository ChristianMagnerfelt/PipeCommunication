/*
 *	Author:			Christian Magnerfelt
 *	Date:			2012.01.05
 *
 *	Brief:			Prints, sorts and filter enviroment variables for viewing.
 *
 *	Description:	The program digenv basically simulates they behaivor of executing
 *					the commands:
 *			
 *						printenv | sort | less
 *				
 *					on command line. If parameters to digenv is provided the following is executed:
 *			
 *						printenv | grep [args] | sort | less	
 *	
 *					If the command 'less' is not available in the OS the program will default to 'more'
 *
 *	Usage:			digenv [args]
 *					
 *		
 *	See:			printenv(1), grep(1), sort(1), less(1)
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <sys/wait.h>
#include <unistd.h>

char g_pager [255];
bool g_useGrep;
int g_numChilds;

int g_printEnvFD[2] = {-1, -1};
int g_grepFD 	[2] = {-1, -1};
int g_sortFD 	[2] = {-1, -1};
int g_lessFD 	[2] = {-1, -1};

/* checks if status is OK, non-negative = OK */
void checkStatus(int status, char * text);
/* Creates a new child process and runs a program in it */
int executeFork(int in, int out, char * prog, char * argv []);
/* Closes all file descriptors */
void closeAll();

/* Program entry point */
int main(int argc, char ** argv, char ** envp)
{
	/* get environment variable called PAGER */
	if(getenv("PAGER") != NULL)
	{
		strcpy(g_pager, getenv("PAGER"));
	}
	else
	{
		strcpy(g_pager, "less");
	}

	/* Use grep if we have command line parameters */
	if(argc > 1)
	{
		g_useGrep = true;
		g_numChilds = 4;
	}
	else
	{
		g_useGrep = false;
		g_numChilds = 3;
	}
	
	/* Construct pipes */
	checkStatus(pipe(g_printEnvFD), "pipe");
	checkStatus(pipe(g_grepFD), "pipe");
	checkStatus(pipe(g_sortFD), "pipe");
	checkStatus(pipe(g_lessFD), "pipe");
	
	/* Execute the forks */
	executeFork(STDIN_FILENO, g_printEnvFD[1], "printenv", NULL);
	if(g_useGrep)
	{
		executeFork(g_printEnvFD[0], g_grepFD[1], "grep", argv);
		executeFork(g_grepFD[0], g_sortFD[1], "sort", NULL);
	}
	else
	{
		executeFork(g_printEnvFD[0], g_sortFD[1], "sort", NULL);
	}
	executeFork(g_sortFD[0], STDOUT_FILENO, g_pager, NULL);
	
	// close all file descriptors as they are not used anymore
	closeAll();
		
	/* Wait for all childs to finish */
	int status;
	int exitStatus;
	int i;
	for(i = 0; i < g_numChilds; ++i)
	{
		/* Wait for the next child finish */
		do{
			wait(&status);
		} while(!WIFEXITED(status) && !WIFSIGNALED(status));
		
		if(WIFSIGNALED(status))
		{
			kill(0, SIGTERM);
			exitStatus = -1;
		}
		else
		{
			/* Child exited successfully */
		}
	}
	
	if(exitStatus != 0)
		exit(exitStatus);
	
	return 0; /* exit(EXIT_SUCCESS) */
}
/* checks if status is OK, non-negative = OK */
void checkStatus(int status, char * text)
{
	if(status < 0)
	{
		perror(text);
		exit(-1);
	}
}
/* Creates a new child process and runs a program in it */
int executeFork(int in, int out, char * prog, char * argv [])
{
	pid_t pid = fork();
	
	if(pid < 0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if(pid == 0)
	{
		/* This code runs in the child process */
		/* Create copies of file descriptors in and out for stdin and stdout */
		checkStatus(dup2(in, STDIN_FILENO), "dup2 stdin");
		checkStatus(dup2(out, STDOUT_FILENO), "dup2 stdout");
		
		/* Close all file descriptors */
		closeAll();
		
		/* Execute program with arguments if they are provided */
		if(argv == NULL)
		{
			char * args[2] = {prog, (char*) 0};
			execvp(prog, args);
		}
		else
		{
			execvp(prog, argv);
		}
		/* execvp returned which means something went wrong */
		/* default to more if less doesn't work */
		if(strcmp(prog, "less"))
		{
			char * args[2] = {"more", (char*) 0};
			execvp("more", args);
		}
			
		perror("execvp");
		exit(EXIT_FAILURE);
	}
	return 0;
}
/* Closes all file descriptors */
void closeAll()
{
	checkStatus(close(g_printEnvFD[0]), "close READ");
	checkStatus(close(g_printEnvFD[1]), "close WRITE");
	
	checkStatus(close(g_grepFD[0]), "close READ");
	checkStatus(close(g_grepFD[1]), "close WRITE");		

	checkStatus(close(g_sortFD[0]), "close READ");
	checkStatus(close(g_sortFD[1]), "close WRITE");

	checkStatus(close(g_lessFD[0]), "close READ");
	checkStatus(close(g_lessFD[1]), "close WRITE");		
}

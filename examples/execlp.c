#include <stdio.h> 
#include <unistd.h> 
#include <string.h> 
#include <stdlib.h> 
#include <errno.h>
#include <sys/wait.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

int
main (int argc, int * argv[])
{
    pid_t child_pid ;

	child_pid = fork() ;
	if (child_pid == 0) {
		int fd = open("ls.out", O_WRONLY | O_CREAT, 0644) ;
		dup2(fd, 1 /* STDOUT*/) ;
		close(fd) ;

		execlp("ls", "ls", "-al", NULL) ;
		// use execvp
	}

    int exit_code ;
    pid_t term_pid = wait(&exit_code) ;
    printf("Execution end: %d %d\n", term_pid, exit_code) ;

    return 0 ;
}
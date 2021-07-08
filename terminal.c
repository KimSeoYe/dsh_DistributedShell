#include <stdio.h> 
#include <unistd.h>
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h> 
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "network.h"

int sock_fd ;

char * cmd ;

char **
parse_cmd(char * cmd)
{
    char * ptr = 0x0 ;
    char * next_ptr = 0x0 ; 
    char ** cmd_argv = (char **) malloc(sizeof(char *) * 1) ;
  
    ptr = strtok_r(cmd, " ", &next_ptr) ;
    for (int i = 0; ptr; i++) {
        cmd_argv = (char**) realloc(cmd_argv, sizeof(char *) * (i + 1)) ;
        cmd_argv[i] = strdup(ptr) ;
        ptr = strtok_r(NULL, " ", &next_ptr) ;
    }

    return cmd_argv ;
}

int pipes[2] ;

void
child_proc (char ** cmd_argv)
{
    close(pipes[0]) ;
    dup2(pipes[1], 1) ;
    execvp(cmd_argv[0], cmd_argv) ;
    close(pipes[1]) ;
    exit(0) ; 
}

void
parent_proc ()
{
    close(pipes[1]) ;

    char buf[1024] ;
    int s ;
    while ((s = read(pipes[0], buf, 1023)) > 0) {
        buf[s] = 0x0 ;
        printf("%s", buf) ;
        // send...
        send_string(sock_fd, buf) ;
	}
    close(pipes[0]) ;

    send_string(sock_fd, "$end$") ;
}

void *
listen_cmd ()
{  
    while(1) {
        int cmd_len = recv_int(sock_fd) ;
        cmd = recv_n_data(sock_fd, cmd_len) ; 
        printf("> cmd: %s\n", cmd) ;
    }
}

void *
worker ()
{
    while (1) {
        // char * cmd = recv_string(sock_fd) ;
        char ** cmd_argv = parse_cmd(cmd) ;

        if (pipe(pipes) != 0) {
            perror("Error") ;
            exit(1) ;
        }

        pid_t child_pid = fork() ;
        if (child_pid < 0) {
            perror("fork") ;
            exit(1) ;
        }
        else if (child_pid == 0) { 
            child_proc(cmd_argv) ;
        }
        else {
            parent_proc() ;
        }
        int exit_code ;
        pid_t term_pid = wait(&exit_code) ;
        printf("Execution end: %d %d\n", term_pid, exit_code) ;
    }
}

void
handler (int sig)
{
    if (sig == SIGINT) {
        close(sock_fd) ;
        exit(0) ;
    }
}

int 
main(int argc, char const *argv[]) 
{ 
    signal(SIGINT, handler) ;

	struct sockaddr_in serv_addr; 
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0) ;
	if (sock_fd <= 0) {
		perror("socket failed : ") ;
		exit(1) ;
	} 
	
	int option = 1 ;
	if (setsockopt(sock_fd, SOL_TCP, TCP_NODELAY, &option, sizeof(option)) < 0) {
		perror("setsockopt: ") ;
		exit(1) ;
	} 

	memset(&serv_addr, 0, sizeof(serv_addr)); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(8989); 
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		exit(1) ;
	} 

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(1) ;
	}

    pthread_t cmd_thr ;
    if (pthread_create(&cmd_thr, NULL, listen_cmd, NULL) != 0) {
        perror("pthread_create: ") ;
        exit(1) ;
    }

    // pthread_t w_thr ;
    // if (pthread_create(&w_thr, NULL, worker, NULL) != 0) {
    //     perror("pthread_create: ") ;
    //     exit(1) ;
    // }

    pthread_join(cmd_thr, NULL) ;
    // pthread_join(w_thr, NULL) ;

} 

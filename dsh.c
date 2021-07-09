#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <string.h> 
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "network.h"

#define SOCK_MAX 16

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t c = PTHREAD_COND_INITIALIZER ;

int turn = -1 ;

void *
ui_worker ()
{
	while (turn == -1) ;

	printf("> turn: %d\n", turn) ;

	while (1) {
		char buffer[1024] = {0} ;
		char c ;
		int i ;
		for (i = 0; (c = getc(stdin)) != '\n'; i++) {
			buffer[i] = c ;
		}
		buffer[i] = 0x0 ;
		int len = i + 1 ;
		printf("> cmd: %s\n", buffer) ;

		// Q. lock..? -> 여기서 m을 잡으면 recv하는 thread와 같은 lock을 잡아서 deadlock에 걸리는 것 같다.
		// 그냥 보내는게 안전한가.......?
		// pthread_mutex_lock(&m) ;
		send_int(turn, len) ;
		send_n_data(turn, buffer, len) ;
		// pthread_mutex_unlock(&m) ;
	}

}

void recv_and_print (int sock)
{
    char buffer[1024] ;
	int len ;
    int s ;

	while (1) {
		s = recv(sock, buffer, 1024, 0) ;
		buffer[s] = 0x0 ;
		if (s > 0) {
			printf("%s", buffer) ;
		}
	}
}

void *
worker (void * ptr)
{
	int conn = * ((int *) ptr) ;

	// Todo. 
	pthread_mutex_lock(&m) ;
	turn = conn ;
	pthread_mutex_unlock(&m) ;

    while (1) {
		pthread_mutex_lock(&m) ;
        recv_and_print(turn) ;
		pthread_mutex_unlock(&m) ;
    }

    close(conn) ;
    free(ptr) ;
}

int 
main(int argc, char const *argv[]) 
{ 
	int listen_fd, new_socket ; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 

	char buffer[1024] = {0}; 

	listen_fd = socket(AF_INET /*IPv4*/, SOCK_STREAM /*TCP*/, 0 /*IP*/) ;
	if (listen_fd == 0)  { 
		perror("socket failed : "); 
		exit(1); 
	}
	
	memset(&address, 0, sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY /* the localhost*/ ; 
	address.sin_port = htons(8989); 
	if (bind(listen_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		perror("bind failed : "); 
		exit(1); 
	} 

	/* create ui_thr */
	pthread_t ui_thr ;
	if (pthread_create(&ui_thr, NULL, ui_worker, NULL) != 0) {
		perror("ERROR - pthread_create: ") ;
			exit(1) ;
	}

	while (1) {
		if (listen(listen_fd, SOCK_MAX /* the size of waiting queue*/) < 0) { 
			perror("listen failed : ") ; 
			exit(1) ; 
		} 

		new_socket = accept(listen_fd, (struct sockaddr *) &address, (socklen_t*)&addrlen) ;
		if (new_socket < 0) {
			perror("accept") ; 
			exit(1) ; 
		} 

		int option = 1 ;
		if (setsockopt(new_socket, SOL_TCP, TCP_NODELAY, &option, sizeof(option)) < 0) {
			perror("setsockopt: ") ;
			exit(1) ;
		}
	
		int * sock_addr = (int *) malloc(sizeof(int) * 1) ;
		*sock_addr = new_socket ;
		pthread_t thr ;
		if (pthread_create(&thr, NULL, worker, (void *) sock_addr) != 0) {
			perror("ERROR - pthread_create: ") ;
			exit(1) ;
		}
	}
} 
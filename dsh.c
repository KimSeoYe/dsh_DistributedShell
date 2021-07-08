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

void recv_and_print (int sock)
{
    char buffer[1024] ;
    int s ;

	while (1) {
		s = recv(sock, buffer, 1024, 0) ;
		if (strcmp(buffer, "$end$") == 0) break ;	// Todo. check...?
		if (s > 0) {
			printf("%s", buffer) ;
		}
	}
}

void *
worker (void * ptr)
{
	int conn = * ((int *) ptr) ;

    while (1) {
        char buffer[1024] = {0} ;
        char c ;
        int i = 0 ;
        for (i; (c = getc(stdin)) != '\n'; i++) {
            buffer[i] = c ;
        }
        buffer[i] = 0x0 ;

		// send_int(conn, i) ;
		int len = i + 1 ;
		send_int(conn, len) ;
		// send_string(conn, buffer) ;
		send_n_data(conn, buffer, len) ;
        // recv_and_print(conn) ;
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
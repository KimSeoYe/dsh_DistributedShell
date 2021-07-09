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
#include <signal.h>

#include "network.h"

// #define DEBUG

#define SOCK_MAX 16

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;
pthread_cond_t c = PTHREAD_COND_INITIALIZER ;

typedef struct _node {
	int fd ;
	struct _node * next ;
} Node ;

int listen_fd ;

Node clients = {0, 0x0};

int turn = -1 ;

void 
print_clients () 
{
    Node* itr = 0x0;
    printf("============ clients ==============\n");
    for(itr = clients.next; itr != 0x0; itr = itr->next) {
        printf("%d\n", itr->fd);
    }
	printf("0 to exit\n") ;
    printf("===================================\n");
}

int
is_exist (int fd)
{
    int exist = 0 ;
    Node * itr = 0x0 ;

    pthread_mutex_lock(&m) ;
    for (itr = clients.next; itr != 0x0; itr = itr->next) {
        if (itr->fd == fd) {
            exist = 1 ;
            break ;
        }
    }
    pthread_mutex_unlock(&m) ;

    return exist ;
}

void
append (int fd)
{
	Node * new_node = (Node *) malloc(sizeof(Node) * 1) ;
	new_node->next = 0x0 ;
	new_node->fd = fd ;

	Node* itr = 0x0, *curr = 0x0;
    
	pthread_mutex_lock(&m);

    for (itr = &clients ; itr != 0x0 ; itr = itr->next) {
        curr = itr;
    }
    curr->next = new_node;

    pthread_mutex_unlock(&m);
}

// Todo. check if the connection w/ client failed -> rm target
void
remove_target (int fd)
{
	Node * itr = 0x0 ;

	Node * prev = &clients ;
	for(itr = clients.next; itr != 0x0; itr = itr->next) {
		if(itr->fd == fd) {
			prev->next = itr->next ;
			// free(itr) ;	Todo.
			break ;
		}
		prev = itr ;
    }
}

void
close_all_sock ()
{
	Node * itr = 0x0 ;

    pthread_mutex_lock(&m) ;
    for (itr = clients.next; itr != 0x0; itr = itr->next) {
        close(itr->fd) ; 
    }
    pthread_mutex_unlock(&m) ;
	close(listen_fd) ;
}

void *
command_mode() 
{
#ifdef DEBUG
	printf(">> Enter ID: ") ;
#endif
	int id ;
	scanf("%d", &id) ;
	printf("> ID: %d\n", id) ;
	
	if (id == 0) {
		close_all_sock() ;
		exit(0) ;
	}
	else if (! is_exist(id)) {
		printf("Invalid input\n") ;
	}
	else {
		pthread_mutex_lock(&m) ;
		turn = id ;
		pthread_mutex_unlock(&m) ;
	}
}

void
handler(int sig)
{
	if (sig == SIGINT) {
		print_clients() ;
		pthread_mutex_lock(&m) ;
		turn = -1 ;
#ifdef DEBUG
		printf("> turn in handler: %d\n", turn) ;
#endif
		pthread_mutex_unlock(&m) ;
		// command_mode() ;
	}
}

void *
ui_worker ()
{
	// while (turn == -1) ;
	// printf("> turn: %d\n", turn) ;

	while (1) {

	#ifdef DEBUG
		printf("> turn in us_worker: %d\n", turn) ;
	#endif
		if (turn == -1) {

	#ifdef DEBUG
			printf("command mode\n") ;
	#endif
			command_mode() ;
		}
		else {
	#ifdef DEBUG
			printf("user mode\n") ;
			printf(">> Enter cmd: ") ;
	#endif
			char buffer[1024] = {0} ;
			char c ;
			int i ;
			for (i = 0; (c = getc(stdin)) != '\n'; i++) {
				buffer[i] = c ;
			}
			buffer[i] = 0x0 ;
			int len = i + 1 ;
	#ifdef DEBUG
			printf("> cmd: %s\n", buffer) ;
	#endif

			pthread_mutex_lock(&m) ;
			send_int(turn, len) ;
			send_n_data(turn, buffer, len) ;
			pthread_mutex_unlock(&m) ;

		}
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

	printf("> %d connected\n", conn) ;
	append(conn) ;

    while (1) {
		if (conn == turn) 
			recv_and_print(conn) ;
    }

    close(conn) ;
    free(ptr) ;
}

int 
main(int argc, char const *argv[]) 
{ 
	signal(SIGINT, handler) ;

	// int listen_fd ;	// Todo. listen_fd -> global?
	int new_socket ; 
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
	address.sin_port = htons(8999); 
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
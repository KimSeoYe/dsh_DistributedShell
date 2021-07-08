// Partly taken from https://www.geeksforgeeks.org/socket-programming-cc/

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

#define SOCK_MAX 16

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;

typedef struct _node {
	int fd ;
	struct _node * next ;
} Node ;

Node clients = {0, 0x0};

void 
print_clients () 
{
    Node* itr = 0x0;
    printf("============ clients ==============\n");
    pthread_mutex_lock(&m);
    for(itr = clients.next; itr != 0x0; itr = itr->next) {
        printf("%d\n", itr->fd);
    }
    pthread_mutex_unlock(&m);
    printf("===================================\n");
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

void
remove_target (int fd)
{
	Node * itr = 0x0 ;

	Node * prev = &clients ;
	for(itr = clients.next; itr != 0x0; itr = itr->next) {
		if(itr->fd == fd) {
			prev->next = itr->next ;
			// free(itr) ;
			break ;
		}
		prev = itr ;
    }
}

void
send_n_data (int conn, char * data, int n)
{
	int s ;

	while (1) {
		s = send(conn, data, n, MSG_NOSIGNAL) ;	// connection이 없을 경우 SIGPIPE signal을 보내지 않게...
		if (s == -1) {
			remove_target(conn) ;
			break ;
		}
		else if (n > 0 && s > 0) {
			data += s ;
			n -= s ;
		}
		else break ;	
	}
}

void
broadcast (char * data, int n) 
{
	Node * itr = 0x0 ;
	pthread_mutex_lock(&m) ;	
	for (itr = clients.next; itr != 0x0; itr = itr->next) {
		send_n_data(itr->fd, data, n) ;
	}
	pthread_mutex_unlock(&m) ;
}

void *
worker (void * ptr)
{
	int conn = * ((int *) ptr) ;	

	append(conn) ;

	while (1) { 
		char buf[1024] ;
		char * data = 0x0 ;
		int len = 0 ;
		
		
		int s = recv(conn, buf, 1023, 0) ;
		if (s > 0) {
			buf[s] = 0x0 ;
			data = strdup(buf) ;
			len = s ;
		}
		if (s == 0) { // Q. & Todo. s == -1 && errno == ENOTCONN ?
			// -> recv는 client가 closesocket()함수를 호출해 접속을 종료하면 0을 리턴한다 (정상종료 - normal close)
			// https://m.blog.naver.com/PostView.naver?isHttpsRedirect=true&blogId=iamchancokr&logNo=60195311628 
			// pthread_exit() ?
			printf("> %d disconnected\n", conn) ;
			goto thr_end ; 
		}

		if (data != 0x0) {
			printf("> %s\n", data) ;
			broadcast(data, len) ;
			// print_clients() ;
		}
	}

thr_end:
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
	address.sin_port = htons(8888); 
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

		// Q. position..?
		// Q. blocking: I/O 작업이 진행되는 동안 다른 작업을 중단한 채 대기.
		// -> Non blocking: I/O를 요청한 다음 진행상황과 상관 없이 바로 결과를 반환. (다른 작업을 중단하지 않음)
		// if(fcntl(new_socket, F_SETFL, fcntl(new_socket, F_GETFL, 0) | O_NONBLOCK) == -1) {
		// 	perror("fcntl") ;
		// 	exit(1) ;
		// }
	
		int * sock_addr = (int *) malloc(sizeof(int) * 1) ;
		*sock_addr = new_socket ;
		pthread_t thr ;
		if (pthread_create(&thr, NULL, worker, (void *) sock_addr) != 0) {
			perror("ERROR - pthread_create: ") ;
			exit(1) ;
		}
	}
} 


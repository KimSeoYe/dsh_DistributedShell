#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h> 
#include <pthread.h>
#include <fcntl.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER ;

void * 
send_message (void * ptr) 
{
	int sock_fd = * ((int *) ptr) ;	

	int s, len ;
	char buffer[1024] = {0} ; 
	char * data ;
	 
	while (1) { 
		char c ;
		int i = 0 ;

		pthread_mutex_lock(&m) ;
		for (i; (c = getc(stdin)) != '\n'; i++) {
			buffer[i] = c ;
		}
		buffer[i] = 0x0 ;
		pthread_mutex_unlock(&m) ;

		data = buffer ;
		len = strlen(buffer) ;

		s = 0 ;
		while (len > 0 && (s = send(sock_fd, data, len, 0)) > 0) {
			data += s ;
			len -= s ;
		}
		
	}

	exit(0) ;
}

void * 
recv_message (void * ptr)
{
	int sock_fd = *((int *) ptr) ;

	while (1) {
		int s, len ;
		char * data ;

		char buffer[1024] ;
		data = 0x0 ;
		len = 0 ;

		// 만약 메세지를 한번에 다 받지 못하면...? 메세지가 깨져서 오면...?
		// 메세지의 크기를 먼저 주고 size check를 해야 할 것 같다...
		// 메세지가 동시에 여러개 들어오면.....? -> 겹칠 일은 없다!

		if ((s = recv(sock_fd, buffer, 1023, 0)) > 0) {
			buffer[s] = 0x0 ;
			data = strdup(buffer) ;
			len = s ;
		}
		
		if (data != 0x0) {
			// pthread_mutex_lock(&m) ;
			printf("> %s\n", data) ; 
			// pthread_mutex_unlock(&m) ;
		}
	}

	exit(0) ;
}

int 
main(int argc, char const *argv[]) 
{ 
	struct sockaddr_in serv_addr; 
	int sock_fd ;
	
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
	serv_addr.sin_port = htons(8888); 
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
		perror("inet_pton failed : ") ; 
		exit(1) ;
	} 

	if (connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect failed : ") ;
		exit(1) ;
	}

	// Q. position...?
	// if(fcntl(sock_fd, F_SETFL, fcntl(sock_fd, F_GETFL, 0) | O_NONBLOCK) == -1) {
	// 	perror("fcntl") ;
	// 	exit(1) ;
	// }

	pthread_t s_thr, r_thr;
	if (pthread_create(&s_thr, NULL, send_message, (void *) &sock_fd) != 0) {
		perror("ERROR - pthread_create: ") ;
		exit(1) ;
	}
	if (pthread_create(&r_thr, NULL, recv_message, (void *) &sock_fd) != 0) {
		perror("ERROR - pthread_create: ") ;
		exit(1) ;
	}

	pthread_join(s_thr, NULL);
	pthread_join(r_thr, NULL);
} 


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <linux/limits.h>
#include <pthread.h>

#include "network.h"

#define DEBUG

void
recv_n_data (int sock, char * data, int n)
{
    char buf[1024] ;
    int is_first = 1 ;
    int len = 0 ;
    int s ;
    while (len < n && (s = recv(sock, buf, n, 0)) > 0) {
        if (is_first) {
            strncpy(data, buf, s) ;
            len = s ;
            is_first = 0 ;
        }
        else {
            data = realloc(data, len + s + 1) ;
            strncpy(data + len, buf, s) ;
            data[len + s] = 0x0 ;
            len += s ;
        }
    }
}

void
send_n_data (int sock, char * data, size_t len)
{
 	size_t s;
	while(len > 0 && (s = send(sock, data, len, 0)) > 0){
		data += s;
		len -= s;
	}
}


void
send_int (int sock, int h)  
{
    int s = 0 ;
    int len = 4 ;
    char * head_p = (char *) &h ;
    while (len > 0 && (s = send(sock, head_p, len, 0)) > 0) {
        head_p += s ;
        len -= s ;
    }
}

int
recv_int (int sock)
{   
    int s = 0 ; 
    int len = 4 ;
    int header ;
    char * head_p = (char *) & header ;

    while (len > 0) {
        s = recv(sock, head_p, len, 0) ;
        if (s == 0) {
            return -1 ;
        }
        head_p += s ;
        len -= s ;
    }

    return header ;
}

void
send_string (int sock, char * data)
{
	int s ;
    int n = strlen(data) ; // include 0x0

	while (1) {
		s = send(sock, data, n, MSG_NOSIGNAL) ;	
		if (s == -1) {
            printf("> connection failed\n") ;
			break ;
		}
		else if (n > 0 && s > 0) {
			data += s ;
			n -= s ;
		}
		else break ;	
	}
}

char *
recv_string (int sock)
{
    int s;
    char * data ;

    char buffer[1024] ;
    data = 0x0 ;

    if ((s = recv(sock, buffer, 1024, 0)) > 0) {
        buffer[s] = 0x0 ;
        data = strdup(buffer) ;
    }

    return data ;
}

char * 
recv_n_chars (int sock, int n)
{
    char buf[1024] ;
    char * data = 0x0;
    int len = 0 ;
    int s ;
    if (len < n && (s = recv(sock, buf, n, 0)) > 0) {
        // buf[s] = 0x0 ;
        if (data == 0x0) {
            data = strdup(buf) ;
            len = s ;
        }
        else {
            data = realloc(data, len + s + 1) ;
            strncpy(data + len, buf, s) ;
            data[len + s] = 0x0 ;
            len += s ;
        }
    }
    
    return data ;
}
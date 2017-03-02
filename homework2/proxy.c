#include <stdio.h>
#include <sys/socket.h>
#include <strings.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "proxylab-handout/csapp.h"

const char program_name[] = "proxy.c"; //global constants are fine, global variables are trouble

void err_exit() { //prints error and exits fuction
	perror(program_name); //by convention, the string is the name of the program
	exit(2);
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
int format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char* lhost, 
		      char *uri, int size)
{	
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;
    printf("test\n");
    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */

    int status;
    struct addrinfo hints;
	struct addrinfo *res;

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
	hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

	if (lhost == NULL) {
		printf("%s\n", uri);
		if ((status = getaddrinfo(uri, NULL, &hints, &res)) != 0) {
	        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	        return 2;
	    }
	} else {
		printf("%s\n", lhost);
		if ((status = getaddrinfo(lhost, NULL, &hints, &res)) != 0) {
	        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
	        return 2;
	    }
	}

    char ipstr[INET6_ADDRSTRLEN];
    struct addrinfo* p;
    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            continue;
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);

        host = ntohl((*(uint32_t*)addr));
    }

    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    return sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}

//function that handles client requests
void clientReq(char* uri, char* url, FILE* logfd, int cfd) {
	int port = 80; //user defines ports

	int fd = Open_clientfd(url, port); //must now send the get request. write to this file descriptor
	int reqLen = 10000;
	char req[reqLen];

	//char* lhost = "127.0.0.1";

	sprintf(req, "GET %s HTTP/1.1\r\nHost: %s:80\r\n\r\n", uri, url);

	printf("%s\n", req);

	int nw = 0;

	if ((nw = write(fd, req, reqLen)) < 0) {
		
		err_exit();
	}


	/* READING NEEDS TO HAPPEN ON THE SERVER */

	char recvline[MAXLINE + 1];
	int nr = 0;

	int rnr = 0;

	while ((nr = read(fd, recvline, MAXLINE)) > 0) {
		recvline[nr] = '\0';	/* null terminate */
		if (fputs(recvline, stdout) == EOF) {
			printf("error at line: %i\n", __LINE__);
			err_exit();
		}
		if (( rnr = fwrite(recvline, 1, MAXLINE, logfd)) < 0) {
			err_exit();
		}
		if(send(cfd, recvline, nr+1, MSG_NOSIGNAL) < 0) {
			err_exit();
		}
	}
	if (nr < 0) {
		printf("error at line: %i\n", __LINE__);
		err_exit();
	}
	
	return;
}


//function for server requests
void reqHandle(int cfd, struct sockaddr_in *addr) {

	char* logFile = "proxy.log"; //stores http info to be passed to web

	FILE* fp = fopen(logFile, "r+");
	if(fp == NULL)
		err_exit();

	//read request

	char reqbuf[10000];
	int reqlen = 0;
	if ( (reqlen = read(cfd, reqbuf, sizeof(reqbuf))) < 0)
		err_exit();

	printf("%s\n", reqbuf);

	//GET pathname HTTP/1.0

	if (strncmp(reqbuf, "GET", 3) != 0) {//strncmp allows for comparison of 2 strings up to N characters
		//if == 0, then get is found (inverted logic)
		//NOT a system-level error, so perror does not work here
		fprintf(stderr, "Not a GET request\n"); //by convention, error messages should be sent to "stderr"
		exit(2);
	}

	//dont know length of pathname, need to parse based on a delimiter, in this case " "
	char* s = reqbuf;
	char* command = strtok_r(reqbuf, " ", &s); //first time we call, we get "GET"
	char* uri = strtok_r(NULL, " ", &s); //modified &s to point to second token, which is the pathname

	//chech protocol and version

	char* proto = strtok_r(NULL, "\r\n", &s); //use end of line token to get last part of string

	strtok_r(NULL, " ", &s); //skip "Host:" portion of string

	char* url = strtok_r(NULL, ":", &s); //get ip address

	char* port = strtok_r(NULL, "\r\n", &s);

	printf("%s %s %s %s %s\n", command, uri, proto, url, port);
	
	//write request

	clientReq(uri, url, fp, cfd);

	//open file

	//read content

	char buf[10000];
	int logSize = format_log_entry(buf, addr, url, uri, 0);

	//the lone carrage return indicates the end of the meta data

	// //write the file recieved to the web client
	// int logLen = 0;
	// rewind(fp);
	// fseek(fp, 0L, SEEK_END);
	// logLen = ftell(fp);
	// rewind(fp);

	// int logBuf[logLen];
	// int logCount = 0;

	// logCount = fread(logBuf, 1, logLen, fp);
	// if ((logCount = write(cfd, logBuf, logLen)) < 0) {
	// 	err_exit();
	// }

	printf("%s\n", buf);

	if (fclose(fp) != 0)
		err_exit();
}

int main(int argc, char* argv[]) {
	//hanlde inputs
	int port = 0; //user defines ports
	char* pstoi;
	if (!argv[1]) {
		port = 80;
	}
	else if((port = (int)strtol(argv[1], &pstoi, 10)) < 1024) {
		err_exit();
	}

	//declare socket file descripter
	int sfd = 0;

	if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		err_exit();
	}

	//bind socket to this IP

	struct sockaddr_in servaddr;

	bzero(&servaddr, sizeof(&servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(sfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
		err_exit();
	}

	//configures server to listen for client
	if (listen(sfd, 5) < 0) {
		err_exit();
	}

	socklen_t buf[sizeof(struct sockaddr_in)]; //for storing the ip address retruned by accept

	while(1) {
		//accept the next request, this function is blocking I/O
		int cfd = 0;
		if((cfd = accept(sfd, NULL, buf)) < 0) {
			err_exit();
		}

		//use getnameinfo to get the buffer values

		//begin reading from client
		reqHandle(cfd, &servaddr);

		//close socket
		if (close(cfd) < 0) {
			err_exit();
		}
	}



	return 0;
}
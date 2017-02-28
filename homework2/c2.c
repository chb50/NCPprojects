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
#include <netdb.h>

#include "proxylab-handout/csapp.h"


const char* program_name = "c2.c";

void err_exit() { //prints error and exits fuction
	perror(program_name); //by convention, the string is the name of the program
	exit(2);
}

int main(int  argc, char* argv[]) {
	if(!argv[1]) {
		printf("no input for url\n");
		exit(0);
	}

	int port = 0; //user defines ports
	char* pstoi;
	if (!argv[2]) { //default to port 80
		port = 80;
	} else if((port = (int)strtol(argv[2], &pstoi, 10)) < 1024 && port != 80) {
		printf("error at line: %i\n", __LINE__);
		printf("%i\n", port);
		err_exit();
	}

	char* url = argv[1];

	int fd = Open_clientfd(url, port); //must now send the get request. write to this file descriptor
	int reqLen = 10000;
	char req[reqLen];

	char* lhost = "127.0.0.1";

	sprintf(req, "GET http://%s/ HTTP/1.1\r\nHost: %s:80\r\n\r\n", url, lhost);

	printf("%s", req);

	int nw = 0;

	if ((nw = write(fd, req, reqLen)) < 0) {
		
		err_exit();
	}

	printf("%i\n", nw);

	char recvline[MAXLINE + 1];
	int nr = 0;

	while ((nr = read(fd, recvline, MAXLINE)) > 0) {
		recvline[nr] = 0;	/* null terminate */
		if (fputs(recvline, stdout) == EOF) {
			printf("error at line: %i\n", __LINE__);
			err_exit();
		}
	}
	if (nr < 0) {
		printf("error at line: %i\n", __LINE__);
		err_exit();
	}
	//printf("SUCCESS: %s\n", recvline);
	char* reqLog = "req.log";

	FILE* rfd = fopen(reqLog, "r+");
	int rnr = 0;

	if (( rnr = fwrite(recvline, 1, MAXLINE, rfd)) < 0) {
		
		err_exit();
	}

	if(fclose(rfd) < 0) {
		err_exit();
	} 
	
	if (close(fd) < 0) {
			
		err_exit();
	}
	return 0;
}
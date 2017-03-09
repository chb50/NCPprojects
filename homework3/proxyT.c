/*
 * Threaded variant of proxy.c
 *
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
#include <pthread.h>

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"
#define THREAD_COUNT 100

typedef struct threadArgs {
    /* one port can handle multiple requests, because different IP addresses indicate different sockets, which mean different connections */
    //serverfd will be made within the context of the threads
    struct sockaddr_in clientaddr;  /* Clinet address structure*/  
    int clientlen;                  /* Size in bytes of the client socket address */
    int connfd; //the tread keeps track of the fd that accepts requests
} threadArgs;

/* Undefine this if you don't want debugging output */
#define DEBUG

/*
 * Globals
 */ 
FILE *log_file; /* Log file with one line per HTTP request */
int request_count = 0;    /* Number of requests received so far */

/*
 * Locks
 */

pthread_mutex_t lock_log_file;
pthread_mutex_t lock_open_clientfd;
/*
 * Functions not provided to the students
 */
int open_clientfd(char *hostname, int port); 
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen); 
void Rio_writen_w(int fd, void *usrbuf, size_t n);

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);

/* thread function for handling client requests */
void* threadRequest(void* args) {

    threadArgs tArgs = *((threadArgs*)args);

    char *request;                  /* HTTP request from client */
    char *request_uri;              /* Start of URI in first HTTP request header line */
    char *request_uri_end;          /* End of URI in first HTTP request header line */
    char *rest_of_request;          /* Beginning of second HTTP request header line */
    int request_len;                /* Total size of HTTP request */
    int response_len;               /* Total size in bytes of response from end server */
    int i, n;                       /* General index and counting variables */
    int realloc_factor;             /* Used to increase size of request buffer if necessary */

    int serverfd;                   /* Socket descriptor for talking with end server */ 

    char hostname[MAXLINE];         /* Hostname extracted from request URI */
    char pathname[MAXLINE];         /* Content pathname extracted from request URI */
    int serverport;                 /* Port number extracted from request URI (default 80) */
    char log_entry[MAXLINE];        /* Formatted log entry */

    rio_t rio;                      /* Rio buffer for calls to buffered rio_readlineb routine */
    char buf[MAXLINE];              /* General I/O buffer */

    int connfd = tArgs.connfd;
    int clientlen = tArgs.clientlen;
    struct sockaddr_in clientaddr = tArgs.clientaddr;

    //Used to fix a bug
    int error = 0;                  /* Used to detect error in reading requests */

    /* 
     * Read the entire HTTP request into the request buffer, one line
     * at a time.
     */


    request = (char *)Malloc(MAXLINE);
    request[0] = '\0';
    realloc_factor = 2;
    request_len = 0;
    Rio_readinitb(&rio, connfd);
    while (1) {
        if ((n = Rio_readlineb_w(&rio, buf, MAXLINE)) <= 0) {
            error = 1;  //Used to fix a bug
            printf("process_request: client issued a bad request (1).\n");
            close(connfd);
            free(request);
            return;
        }

        /* If not enough room in request buffer, make more room */
        if (request_len + n + 1 > MAXLINE)
            Realloc(request, MAXLINE*realloc_factor++);

        strcat(request, buf);
        request_len += n;

        /* An HTTP requests is always terminated by a blank line */
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }

    /*
     * Used to fix a bug
     * if a bad request has been issued then start over
     */
    if (error)
       return;

    /* 
     * Make sure that this is indeed a GET request
     */
    if (strncmp(request, "GET ", strlen("GET "))) {
        printf("process_request: Received non-GET request\n");
        close(connfd);
        free(request);
        return;
    }
    request_uri = request + 4;

    /* 
     * Extract the URI from the request
     */
    request_uri_end = NULL;
    for (i = 0; i < request_len; i++) {
        if (request_uri[i] == ' ') {
            request_uri[i] = '\0';
            request_uri_end = &request_uri[i];
            break;
        }
    }

    /* 
     * If we hit the end of the request without encountering a
     * terminating blank, then there is something screwy with the
     * request
     */
    if ( i == request_len ) {
        printf("process_request: Couldn't find the end of the URI\n");
        close(connfd);
        free(request);
        return;
    }

    /*
     * Make sure that the HTTP version field follows the URI 
     */
    if (strncmp(request_uri_end + 1, "HTTP/1.0\r\n", strlen("HTTP/1.0\r\n")) &&
        strncmp(request_uri_end + 1, "HTTP/1.1\r\n", strlen("HTTP/1.1\r\n"))) {
        printf("process_request: client issued a bad request (4).\n");
        close(connfd);
        free(request);
        return;
    }

    /*
     * We'll be forwarding the remaining lines in the request
     * to the end server without modification
     */
    rest_of_request = request_uri_end + strlen("HTTP/1.0\r\n") + 1;

    /* 
     * Parse the URI into its hostname, pathname, and port components.
     * Since the recipient is a proxy, the browser will always send
     * a URI consisting of a full URL "http://hostname:port/pathname"
     */  
    if (parse_uri(request_uri, hostname, pathname, &serverport) < 0) {
        printf("process_request: cannot parse uri\n");
        close(connfd);
        free(request);
        return;
    }      

    /*
     * Forward the request to the end server
     */ 
    pthread_mutex_lock(&lock_open_clientfd); //open_clientfd is NOT thread safe
    if ((serverfd = open_clientfd(hostname, serverport)) < 0) { //TODO: this is not working, on either this script or the one provided
        printf("process_request: Unable to connect to end server.\n");
        free(request);
        pthread_mutex_unlock(&lock_open_clientfd);
        return;
    }
    pthread_mutex_unlock(&lock_open_clientfd);

    Rio_writen_w(serverfd, "GET /", strlen("GET /"));
    Rio_writen_w(serverfd, pathname, strlen(pathname));
    Rio_writen_w(serverfd, " HTTP/1.0\r\n", strlen(" HTTP/1.0\r\n"));
    Rio_writen_w(serverfd, rest_of_request, strlen(rest_of_request));
  

    /*
     * Receive reply from server and forward on to client
     */
    Rio_readinitb(&rio, serverfd);
    response_len = 0;
    while( (n = Rio_readn_w(serverfd, buf, MAXLINE)) > 0 ) {
        response_len += n;
        Rio_writen_w(connfd, buf, n);
        #if defined(DEBUG)  
            printf("Forwarded %d bytes from end server to client\n", n);
            fflush(stdout);
        #endif
            bzero(buf, MAXLINE);
    }

    /*
     * Log the request to disk
     */

    /*** THIS WILL NEED TO BE SYNCHRONIZED ***/
    pthread_mutex_lock(&lock_log_file);
    format_log_entry(log_entry, &clientaddr, request_uri, response_len);  
    fprintf(log_file, "%s %d\n", log_entry, response_len);
    fflush(log_file);
    pthread_mutex_unlock(&lock_log_file);

    /* Clean up to avoid memory leaks and then return */
    close(connfd);
    close(serverfd);
    free(request);

    return;
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd;             /* The proxy's listening descriptor */
    int port;                 /* The port the proxy is listening on */

    pthread_t threads[THREAD_COUNT]; /* Array that stores thread contexts */
    int tid = 0;

    pthread_mutex_init(&lock_log_file, NULL);
    pthread_mutex_init(&lock_open_clientfd, NULL);

    int i; //iterator

    /* Check arguments */
    if (argc != 2) {
    	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    	exit(0);
    }

    /* 
     * Ignore any SIGPIPE signals elicited by writing to a connection
     * that has already been closed by the peer process.
     */
    signal(SIGPIPE, SIG_IGN);

    /* Create a listening descriptor */
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    /* Inititialize */
    log_file = Fopen(PROXY_LOG, "a");
  
    /* Wait for and process client connections */
    while (1) { 
        //thread arguements
        struct sockaddr_in clientaddr;  /* Clinet address structure*/  
        int clientlen;                  /* Size in bytes of the client socket address */
        int connfd;                     /* socket desciptor for talking wiht client*/

        threadArgs tArgs;

        tArgs.connfd = connfd;
        tArgs.clientaddr = clientaddr;
        tArgs.clientlen = clientlen;

    	clientlen = sizeof(clientaddr);  
    	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        /*** THREAD HERE ***/
        //we will need to declared some variables that are declared above within the context of the thread instead
        //this is so the variables are not shared amongst the threads
        //for example, each thread can have a serverfd, rather than every thread accessing 1 serverfd

        Pthread_create(&threads[tid], NULL, threadRequest, (void*)&tArgs);
        tid++;

        //printf("tid %i\n", tid);


        //lazy code: make sure the thread limit has not been reached. If it has, clear all threads
        if (tid == THREAD_COUNT) {
            printf("Thread limit reached, waiting for threads to die\n");
            for(i = 0; i < THREAD_COUNT; ++i) {
                Pthread_join(threads[i], NULL);
            }
            tid = 0;
        }
    }

    /* Control never reaches here */
    exit(0);
}


/*
 * Rio_readn_w - A wrapper function for rio_readn (csapp.c) that
 * prints a warning message when a read fails instead of terminating
 * the process.
 */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) 
{
    ssize_t n;
  
    if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
    	printf("Warning: rio_readn failed\n");
    	return 0;
    }    
    return n;
}

/*
 * Rio_readlineb_w - A wrapper for rio_readlineb (csapp.c) that
 * prints a warning when a read fails instead of terminating 
 * the process.
 */
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
    	printf("Warning: rio_readlineb failed\n");
    	return 0;
    }
    return rc;
} 

/*
 * Rio_writen_w - A wrapper function for rio_writen (csapp.c) that
 * prints a warning when a write fails, instead of terminating the
 * process.
 */
void Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n) {
	   printf("Warning: rio_writen failed.\n");
    }	   
}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
    	hostname[0] = '\0';
    	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	   *port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	   pathname[0] = '\0';
    }
    else {
    	pathbegin++;	
    	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


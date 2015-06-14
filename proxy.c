/*
 * proxy.c - CS:APP Web proxy
 *
 * Student ID:  2013-11394
 *       Name:  Kim Hyung Mo
 *
 * Proxy acts as server to echo client, and as client to echo server.
 *
 * 1. To act as server, the proxy opens listenfd to accept echoclients.
 * 2. When connection request accepted, the proxy make a worker thread to
 *    serve echoclient.
 * 3. Then the proxy opens clientfd to request to echoserver to act as client.
 * 4. The proxy carrys out send/recieve performance, and return it back to
 *    echoclient.
 * 5. At the moment, the proxy saves log for interaction.
 */

#include "csapp.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/* thread argument */
typedef struct{
  int logfd;
  int connfd;
  struct sockaddr_in clientaddr;
} TA;

/*
 * Functions to define
 */
void * process_request(void *);
int open_clientfd_ts(char *, int, sem_t *);
ssize_t Rio_readn_w(int, void *, size_t);
ssize_t Rio_readlineb_w(rio_t *, void *, size_t);
void Rio_writen_w(int, void *, size_t);

/*
 * Helper functions
 */
int connect_proxy(int, char (*)[], sem_t *);

/* Global static variables */
static sem_t copy_sem;
static sem_t log_sem;
static sem_t conn_sem;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
  TA t_arg;
  pthread_t thread;
  int port, clientlen, listenfd;

  /* check arguments number */
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
    exit(0);
  }

  /* keep proxy from caching */
  Signal(SIGPIPE, SIG_IGN);

  /* initialize semaphores */
  Sem_init(&copy_sem, 0, 1);
  Sem_init(&log_sem, 0, 1);
  Sem_init(&conn_sem, 0, 1);

  /* initialize as server of echo_client */
  port = atoi(argv[1]);
  clientlen = sizeof(SA);
  listenfd = Open_listenfd(port);
  t_arg.logfd = Open(PROXY_LOG, O_CREAT|O_RDWR|O_APPEND, 0677);

  /* accept concurrent clients */
  while (1)
  {
    /* aquire lock protecting the clientaddr until copy done */
    P(&copy_sem);
    t_arg.connfd = Accept(listenfd, (SA *) &t_arg.clientaddr, &clientlen);
    Pthread_create(&thread, NULL, process_request, (void *) &t_arg);
  }

  Close(t_arg.logfd);
  exit(0);
}

/* thread routine */
void * process_request(void * vargp)
{
  rio_t rio;
  int nbytes;
  time_t timer;
  char * hostaddrp;
  struct tm * time_now;
  struct sockaddr_in clientaddr;
  int logfd, connfd_c, connfd_s, clientport;

  /* local buffers */
  char log[MAXLINE];
  char logbuf[MAXLINE];
  char rdwrbuf[MAXLINE];

  /* setup as server of echo_client */
  logfd = ((TA *) vargp)->logfd;
  connfd_c = ((TA *) vargp)->connfd;
  clientaddr = ((TA *) vargp)->clientaddr;
  hostaddrp = inet_ntoa(clientaddr.sin_addr);
  clientport = ntohs(clientaddr.sin_port);

  /* detach thread & release lock */
  pthread_detach(Pthread_self());
  V(&copy_sem);

  /* work as client of echo_server */
  for (nbytes = 0; ;)
  {
    /* check connection error */
    if ((connfd_s = connect_proxy(connfd_c, &rdwrbuf, &conn_sem)) < 0)
    {
      if (connfd_s == -4)
      {
        fprintf(stderr, "Disconnect: %d\n", connfd_c);
        break;
      }
      else
      {
        sprintf(rdwrbuf, "proxy usage: <host> <port> <message>\n");
        Rio_writen_w(connfd_c, rdwrbuf, strlen(rdwrbuf));
        continue;
      }
    }

    /* read/write with echo_server */
    Rio_readinitb(&rio, connfd_s);
    Rio_writen_w(connfd_s, rdwrbuf, strlen(rdwrbuf));
    nbytes += Rio_readlineb_w(&rio, rdwrbuf, MAXLINE);

    /* wite back to echo_client */
    Rio_writen_w(connfd_c, rdwrbuf, strlen(rdwrbuf));
    Close(connfd_s);

    /* save log */
    time(&timer);
    time_now = localtime(&timer);
    strftime(logbuf, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", time_now);
    sprintf(log, "%s: %s %d %d %s",
        logbuf, hostaddrp, clientport, nbytes, rdwrbuf);

    /* write log to proxy.log */
    P(&log_sem);
    Write(logfd, log, strlen(log));
    V(&log_sem);
  }

  Close(connfd_c);
  return NULL;
}

/*
 * connect_proxy - Parse input (<host> <port> <message>) and
 * make connection with proper echo server.
 */
int connect_proxy(int fd, char (* usrbuf)[MAXLINE], sem_t * mutexp)
{
  rio_t rio;
  size_t nbytes;
  char buf[MAXLINE];
  char * host, * args, * port;

  Rio_readinitb(&rio, fd);

  /* read and check if disconnected */
  if (!(nbytes = Rio_readlineb_w(&rio, buf, MAXLINE)))
    return -4;

  /* parse message */
  if (!(host = strtok_r(buf, " ", &args)))
    return -1;
  if (!(port = strtok_r(NULL, " ", &args)))
    return -1;

  /* open socket as client of echo_server */
  fd = open_clientfd_ts(host, atoi(port), &conn_sem);

  strcpy(*usrbuf, args);
  return fd;
}

int open_clientfd_ts(char * hostname, int port, sem_t * mutexp)
{
  int fd;

  /* lock the opening of client fd */
  P(mutexp);
  fd = open_clientfd(hostname, port);
  V(mutexp);

  return fd;
}

ssize_t Rio_readn_w(int fd, void * ptr, size_t nbytes)
{
  ssize_t ncnt;
  if ((ncnt = rio_readn(fd, ptr, nbytes)) < 0)
  {
    printf("Rio_readn_w error\n");
    fflush(stdout);
    ncnt = 0;
  }

  return ncnt;
}

ssize_t Rio_readlineb_w(rio_t * rp, void * usrbuf, size_t maxlen)
{
  ssize_t rcnt;
  if ((rcnt = rio_readlineb(rp, usrbuf, maxlen)) < 0)
  {
    printf("Rio_readlineb_w error\n");
    fflush(stdout);
    rcnt = 0;
  }

  return rcnt;
}

void Rio_writen_w(int fd, void * usrbuf, size_t n)
{
  if (rio_writen(fd, usrbuf, n) != n)
  {
    printf("Rio_writen_w error\n");
    fflush(stdout);
  }
}


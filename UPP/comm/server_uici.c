#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "uici.h"

#define BLKSIZE 1024

void usr1handler(int s)
{
  fprintf(stderr, "SIGUSR1 signal caught by server (%d)\n", s);
}

void usr2handler(int s)
{
  fprintf(stderr, "SIGUSR2 signal caught by server (%d)\n", s);
}

void installusrhandlers()
{
  struct sigaction newact;
  newact.sa_handler = usr1handler;
  sigemptyset(&newact.sa_mask);
  newact.sa_flags = 0;
  if ( (sigaction(SIGUSR1, &newact, (struct sigaction *) NULL)) == -1) {
    perror("Could not install SIGUSR1 signal handler");
    return;
  }
  newact.sa_handler = usr2handler;
  if ( (sigaction(SIGUSR2, &newact, (struct sigaction *) NULL)) == -1) {
    perror("Could not install SIGUSR2 signal handler");
    return;
  }
  fprintf(stderr, "Server process %ld set to use SIGUSR1 and SIGUSR2\n",
	  (long) getpid());
}

int main(int argc, char *argv[])
{
  unsigned short portnumber;
  int listenfd;
  int communfd;
  ssize_t bytesread;
  ssize_t byteswritten;
  char buf[BLKSIZE];
  char remote[MAX_CANON];

  if (argc != 2) {
    fprintf(stderr, "Usage: %s port\n", argv[0]);
    exit(1);
  }

  installusrhandlers();

  portnumber = (unsigned short) atoi(argv[1]);

  if ((listenfd = u_open(portnumber)) < 0) {
    u_error("Unable to establish a port connection");
    exit(1);
  }

  if ((communfd = u_listen(listenfd, remote)) < 0) {
    u_error("Failure to listen on server");
    exit(1);
  }

  fprintf(stderr, "Connection has been made to %s\n", remote);

  while ( (bytesread = u_read(communfd, buf, BLKSIZE)) > 0) {
    byteswritten = write(STDOUT_FILENO, buf, bytesread);
    if (bytesread != byteswritten) {
      fprintf(stderr, "Error writting %ld bytes, %ld bytes written\n",
	      (long) bytesread, (long)byteswritten);
      break;
    }
  }
  u_close(listenfd);
  u_close(communfd);
  exit(0);
}



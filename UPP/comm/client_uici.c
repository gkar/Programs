#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
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
  int outfd;
  ssize_t bytesread;
  ssize_t byteswritten;
  char buf[BLKSIZE];

  if (argc != 3) {
    fprintf(stderr, "Usage: %s host port\n", argv[0]);
    exit(1);
  }

  installusrhandlers();

  portnumber = (unsigned short) atoi(argv[2]);

  if ((outfd = u_connect(portnumber, argv[1])) < 0) {
    u_error("Unable to establish an internet connection");
    exit(1);
  }

  fprintf(stderr, "Connection has been made to %s\n", argv[1]);

  for ( ; ; ) {
    bytesread = read(STDIN_FILENO, buf, BLKSIZE);
    if ( (bytesread == -1) && (errno == EINTR) )
      fprintf(stderr, "Client restarting read\n");
    else if (bytesread <= 0) break;
    else {
      byteswritten = u_write(outfd, buf, bytesread);
      if (byteswritten != bytesread) {
	fprintf(stderr, "Error writing %ld bytes, %ld bytes written\n",
		(long)bytesread, (long)byteswritten);
	break;
      }
    }
  }
  u_close(outfd);

  exit(0);
}



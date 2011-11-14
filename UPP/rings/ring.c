#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wait.h>
#include <sys/types.h>

int main (int argc, char *argv[])
{
  int i;
  int childpid;
  int nprocs;
  int fd[2];
  int error;

  if ( (argc != 2) || ((nprocs = atoi (argv[1])) <= 0)) {
    fprintf(stderr, "Usage: %s <nprocs>\n", argv[0]);
    exit(1);
  }

  /* se conecta stdin con stdout mediante pipes */

  if (pipe (fd) == -1) {
    perror("Could not create pipe");
    exit (1);
  }

  if ((dup2 (fd[0], STDIN_FILENO) == -1) ||
      (dup2 (fd[1], STDOUT_FILENO) == -1)) {
    perror("Could not dup pipes");
    exit (1);
  }

  if ((close (fd[0]) == -1) || (close (fd[1]) == -1)) {
    perror("Could not close extra descriptors");
    exit (1);
  }

  /* se crean los demas procesos con sus respectivos pipes */

  for (i = 1; i < nprocs; i++) {
    if (pipe (fd) == -1) {
      fprintf (stderr, "Could not create pipe %d: %s\n",
	       i, strerror(errno));
      exit (1);
    }
    if ((childpid = fork()) == -1) {
      fprintf (stderr, "Could not create child %d: %s\n",
	       i, strerror(errno));
      exit (1);
    }

    /* para el proceso padre se reasigna stdout */
    if (childpid > 0) {
      error = dup2 (fd[1], STDOUT_FILENO);
    } else {
      error = dup2 (fd[0], STDIN_FILENO);
    }

    if (error == -1) {
      fprintf (stderr, "Could not dup pipes for iteration %d: %s\n", 
	      i, strerror(errno));
      exit (1);
    }
    if ((close(fd[0]) == -1) || (close(fd[1]) == -1)) {
      fprintf (stderr, "Could not close extra descriptors %d: %s\n",
	       i, strerror(errno));
      exit (1);
    }
    if (childpid)
      break;
  }

  fprintf (stderr, "This is process %d with ID %d and parent ID %d\n",
	   i, (int)getpid(), (int)getppid());

  exit(0);
}

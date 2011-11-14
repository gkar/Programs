#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wait.h>
#include <sys/types.h>

int main (int argc, char *argv[])
{
  int i,k;
  int childpid;
  int nprocs;
  int next[2], prev[2], new_next[2], new_prev[2];
  int error;
  int child_status;
  int next_ID, prev_ID;

  if ( (argc != 2) || ((nprocs = atoi (argv[1])) <= 0)) {
    fprintf(stderr, "Usage: %s <nprocs>\n", argv[0]);
    exit(1);
  }

  /* se conecta stdin con stdout mediante pipes */

  if ( (pipe(next) == -1) || (pipe(prev) == -1) ) {
    perror("Could not create pipes");
    exit (1);
  }

  /* se crean los demas procesos con sus respectivos pipes */

  for (i = 1; i < nprocs; i++) {
    if ( (pipe(new_next) == -1) || (pipe(new_prev) == -1) )  {
      fprintf (stderr, "Could not create pipes %d: %s\n",
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
      error = dup2 (new_next[1], next[1]);
      error = dup2 (new_prev[0], prev[0]);
    } else {
      error = dup2 (new_next[0], next[0]);
      error = dup2 (new_prev[1], prev[1]);
    }

    if (error == -1) {
      fprintf (stderr, "Could not dup pipes for iteration %d: %s\n", 
	       i, strerror(errno));
      exit (1);
    }
    if ((close(new_next[0]) == -1) || (close(new_next[1]) == -1) ||
	(close(new_prev[0]) == -1) || (close(new_prev[1]) == -1) ) {
      fprintf (stderr, "Could not close extra descriptors %d: %s\n",
	       i, strerror(errno));
      exit (1);
    }
    if (childpid)
      break;
  }

  int IDp[nprocs], IDn[nprocs];
  IDn[0] = next_ID = (int) getpid();
  IDp[0] = prev_ID = (int) getpid();

  for (k = 1; k < nprocs; k++) {
    write(next[1], &next_ID, sizeof(next_ID));
    write(prev[1], &prev_ID, sizeof(prev_ID));

    read(next[0], &next_ID, sizeof(next_ID));
    read(prev[0], &prev_ID, sizeof(prev_ID));

    IDn[k] = next_ID;
    IDp[k] = prev_ID;
  }

  wait(&child_status);
  if (!WIFEXITED (child_status)) {
    fprintf (stderr,"the child process exited abnormally, with exit code %d\n",
	     WEXITSTATUS(child_status));
  }

  // print previous
  fprintf(stderr, "p[%d]", IDp[0]);
  for (k = 1; k < nprocs; k++)
    fprintf(stderr,":%d", IDp[k]);
  fprintf(stderr,"\n");

  // print next
  fprintf(stderr, "n[%d]", IDn[0]);
  for (k = 1; k < nprocs; k++)
    fprintf(stderr,":%d", IDn[k]);
  fprintf(stderr,"\n");

  exit(0);
}

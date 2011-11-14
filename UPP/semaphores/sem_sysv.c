#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define PERMS S_IRUSR | S_IWUSR
#define SET_SIZE 2

void set_sembuf_struct(struct sembuf *s, int num, int op, int flg)
{
  s->sem_num = (short) num;
  s->sem_op = op;
  s->sem_flg = flg;
  return;
}

int remove_semaphore(int semid)
{
  return semctl(semid, 0, IPC_RMID);
}

int main(int argc, char **argv) 
{
  char buffer[MAX_CANON];
  char *c;
  int i, n;
  pid_t childpid;
  int semid;
  int semop_ret;
  struct sembuf semwait[1];
  struct sembuf semsignal[1];
  int status;

  if ( (argc != 2) || ((n = atoi(argv[1])) <= 0) ) {
    fprintf(stderr,"Usage: %s <number_of_processes>\n", argv[0]);
    exit (1);
  }

  if ( (semid = semget(IPC_PRIVATE, SET_SIZE, PERMS)) == -1 ) {
    fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	    (long)getpid(), strerror(errno));
    exit(1);
  }

  set_sembuf_struct (semwait, 0, -1, 0);
  set_sembuf_struct (semsignal, 0, 1, 0);

  if ( (semop(semid, semsignal, 1) == -1) ) {
    fprintf(stderr,"[%ld]: Semaforo no valido el acceso - %s\n",
	    (long)getpid(), strerror(errno));
    if ( (remove_semaphore(semid) == -1) )
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	      (long)getpid(), strerror(errno));
    exit(1);
  }

  for (i = 1; i < n; i++)
    if ((childpid = fork()))
      break;

  sprintf(buffer, "i:%d process ID:%ld parent ID:%ld child ID:%ld",
	  i, (long)getpid(), (long)getppid(), (long)childpid);

  c = buffer;

  /*** Seccion de Entrada ***/
  while ( ((semop_ret = semop (semid, semwait, 1)) == -1) &&
	  (errno == EINTR) )
    ;

  if (semop_ret == -1) {
    fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	    (long)getpid(), strerror(errno));
  } else {

    /*** Inicio de Seccion Critica ***/
    while (*c != '\0') {
      fputc(*c, stderr);
      c++;
    }
    fputc('\n', stderr);
    /*** Fin de Seccion Critica ***/

    /*** Seccion de Salida ***/
    while ( ((semop_ret = semop (semid, semsignal, 1)) == -1) &&
	    (errno == EINTR) )
      ;

    if (semop_ret == -1) {
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	      (long)getpid(), strerror(errno));
    }
  }

  while ( (wait(&status) == -1) && (errno == EINTR) )
    ;

  if (i == 1)
    if ( (remove_semaphore(semid) == -1) )
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	      (long)getpid(), strerror(errno));

  /*** Seccion remanente ***/
  exit(0);
}

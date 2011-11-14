#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <semaphore.h>

// sem_t my_lock;

int main(int argc, char **argv) 
{
  char buffer[MAX_CANON];
  char *c;
  int i, n;
  pid_t childpid;
  sem_t my_lock;

  if ( (argc != 2) || ((n = atoi(argv[1])) <= 0) ) {
    fprintf(stderr,"Usage: %s <number_of_processes>\n", argv[0]);
    exit (1);
  }

  if (sem_init(&my_lock, 1, 1) == -1) {
    perror("No puede iniciar el semaforo my_lock");
    exit(1);
  }

  for (i = 1; i < n; i++)
    if ((childpid = fork()))
      break;

  sprintf(buffer, "i:%d process ID:%ld parent ID:%ld child ID:%ld",
	  i, (long)getpid(), (long)getppid(), (long)childpid);

  c = buffer;

  /*** Seccion de Entrada ***/
  if (sem_wait(&my_lock) == -1) {
    perror("Semaforo no valido");
    exit(1);
  }

  /*** Inicio de Seccion Critica ***/
  while (*c != '\0') {
    fputc(*c, stderr);
    c++;
  }
  fputc('\n', stderr);
  /*** Fin de Seccion Critica ***/

  /*** Seccion de Salida ***/
  if (sem_post(&my_lock) == -1) {
    perror("Semaforo ejecutado");
    exit(1);
  }

  /*** Seccion remanente ***/
  exit(0);
}

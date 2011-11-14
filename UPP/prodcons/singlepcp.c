/*
 * singlepcp.c
 *
 * Programa que inicia el problema del productor-consumidor
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "shared.h"

/* producer: produce una serie de numeros al azar, teniendo el cuidado de no
 * repetir ningun numero.
 */
void producer(int n)
{
  int i;

  for (i = 0; i < n; i++)
    if (buff_put(i) == -1)
      fprintf(stderr, "Error producer:buff_put\n");
    else {
      fprintf(stdout,"p:%d\n", i);
      bprint("p");
    }

  fprintf(stderr, "producer ends\n");
}

/* consumer: consume los numeros producidos por producer, y los almacena en una
 * lista de ordenada.
 */
void consumer(int n)
{
  int i, e;

  for (i = 0; i < n; i++)
    if ((e = buff_get()) == -1)
      fprintf(stderr, "Error consumer:buff_get\n");
    else
      fprintf(stdout,"c:%d\n", e);

  fprintf(stderr, "consumer ends\n");
}

/* init_struct: inicia las estructuras usadas para resolver el problema
 */
void init_struct(int n, int b)
{
  int child_status;
  int ppid = getpid();

  if ((buff_init(b, 0777 | IPC_CREAT)) == 0) {
    fprintf(stderr, "Buffer created!\n");
  } else {
    fprintf(stderr, "Buffer not created!\n");
    exit(1);
  }


  if (fork() == 0) producer(n);

  if (ppid == getpid() && fork() == 0) consumer(n);

  while (wait(&child_status) != -1) {
    if (!WIFEXITED (child_status)) {
      fprintf (stderr,"the child process %d exited abnormally, with exit code %d\n",
	       getpid(), WEXITSTATUS(child_status));
    }
  }

  if (ppid == getpid()) {
    if ((buff_close()) == 0) {
      fprintf(stderr, "Buffer closed!\n");
    } else {
      fprintf(stderr, "Buffer not closed\n");
      exit(1);
    }
  }
}

int main (int argc, char *argv[])
{
  int n, b;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <number of elements> <size of buffer>\n", argv[0]);
    exit (1);
  }

  n = atoi(argv[1]);
  b = atoi(argv[2]);

  init_struct(n, b);

  return (0);
}

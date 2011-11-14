/*
 * singlepcp.c
 *
 * Programa que inicia el problema del productor-consumidor
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "shared.h"

/* find_and_set: busca a 'e' en el arreglo 'a' y si no lo encuentra, fija su
 * contenido en la siguiente posición libre, suma 1 al contador 'c' y devuelve
 * 1 si encontro el valor y 0 en caso contrario.
 */
int find_and_set(int a[], int n, int *c, int e)
{
  int i;
  int found = 0;

  for (i = 0; (i < n) && (i < *c); i++)
    if (a[i] == e) 
      found = 1;

  if (!found && i < n) {
    a[i] = e;
    (*c)++;
  }

  return(found);
}


/* producer: produce una serie de numeros al azar, teniendo el cuidado de no
 * repetir ningun numero.
 */
void producer(int n)
{
  int e;
  int count = 0;
  int a[n];
  int seed = (unsigned int) time(NULL);

  srand(seed);

  while (count < n) {
    e = rand() % n;
    if (!find_and_set(a, n, &count, e)) {
      if (buff_put(e) == -1)
	fprintf(stderr, "Error producer:buff_put\n");
      else {
	fprintf(stdout,"p:%d\n", e);
	bprint("p");
      }
    }
  }

  fprintf(stderr, "producer ends\n");
}

void insertion_sort(int a[], int n, int *c, int e)
{
  int i;

  if (*c > n) return;

  for (i = (*c)-1; (i >= 0) && (e < a[i]); i--)
    a[i+1] = a[i];

  a[++i] = e;
  (*c)++;
}

/* consumer: consume los numeros producidos por producer, y los almacena en una
 * lista de ordenada.
 */
void consumer(int n)
{
  int i, e;
  int count = 0;
  int a[n];

  while (count < n) {
    if ((e = buff_get()) == -1)
      fprintf(stderr, "Error consumer:buff_get\n");
    else {
      insertion_sort(a, n, &count, e);
      fprintf(stdout,"c:%d\n", e);
    }
  }

  fprintf(stdout, "Output: ");
  for (i = 0; i < n; i++)
    fprintf(stdout, "%d ", a[i]);
  fprintf(stdout, "\n");

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

/*
 * pcp102.c
 *
 * Implementacion incorrecta del problema de productores-consumidores. Los
 * hilos productores y consumidores no estan sincronizados.
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "buffer.h"

#define SUMSIZE 100

int sum = 0;

void *producer(void *arg1)
{
  int i;

  for(i = 1; i <= SUMSIZE; i++)
    put_item(i*i);
  return NULL;
}

void *consumer(void *arg2)
{
  int i, myitem;

  for(i = 1; i <= SUMSIZE; i++) {
    get_item(&myitem);
    sum += myitem;
  }

  return NULL;
}

int main(void)
{
  pthread_t prodid;
  pthread_t consid;
  int i, total;

  /* comprobar valor */
  total = 0;
  for(i = 1; i <= SUMSIZE; i++)
    total += i*i;
  printf("the actual sum should be %d\n", total);

  /* crear hilos */
  if(pthread_create(&consid, NULL, consumer, NULL))
    perror("Could not create consumer");
  else if(pthread_create(&prodid, NULL, producer, NULL))
    perror("Could not create producer");

  /* esperar que los hilos terminen */
  pthread_join(prodid, NULL);
  pthread_join(consid, NULL);

  printf("The threads producer the sum %d\n", sum);

  exit(0);
}

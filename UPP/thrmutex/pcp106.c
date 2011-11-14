/*
 * pcp106.c
 *
 * Una segunda solucion incorrecta al problema de productores-consumidores
 * controlado por el productor
 *
 * Nota: Existe la posibilidad de que 'producer' finalice con la suficiente
 * antelacion como para notificar esto a 'consumer' antes de terminar de leer
 * los elementos faltantes del buffer circular, por lo que total > sum.
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "buffer.h"

#define SUMSIZE 100
#define BUFSIZE 8

int sum = 0;

pthread_cond_t slots = PTHREAD_COND_INITIALIZER;
pthread_cond_t items = PTHREAD_COND_INITIALIZER;
pthread_mutex_t slot_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t item_lock = PTHREAD_MUTEX_INITIALIZER;

int nslots = BUFSIZE;
int producer_done = 0;
int nitems = 0;

void *producer(void *arg1)
{
  int i;

  for(i = 1; i <= SUMSIZE; i++) {
    /* adquirir derecho a una ranura */
    pthread_mutex_lock(&slot_lock);
    {
      while (nslots <= 0)
	pthread_cond_wait(&slots, &slot_lock);
      nslots--;
    }
    pthread_mutex_unlock(&slot_lock);

    put_item(i*i);

    /* renuncia derecho a un elemento */
    pthread_mutex_lock(&item_lock);
    {
      nitems++;
      pthread_cond_signal(&items);
    }
    pthread_mutex_unlock(&item_lock);
  }

  pthread_mutex_lock(&item_lock);
  {
    producer_done = 1;
    pthread_cond_broadcast(&items);
  }
  pthread_mutex_unlock(&item_lock);

  return NULL;
}

void *consumer(void *arg2)
{
  int myitem;

  for( ; ; ) {
    /* adquirir el derecho a un elemento */
    pthread_mutex_lock(&item_lock);
    {
      while((nitems <= 0) && (!producer_done))
	pthread_cond_wait(&items, &item_lock);

      if((nitems <= 0) && (producer_done)) {
	pthread_mutex_unlock(&item_lock);
	break;
      }
      nitems--;
    }
    pthread_mutex_unlock(&item_lock);
    get_item(&myitem);
    sum += myitem;      

    /* renuncia derecho a unra ranura */    
    pthread_mutex_lock(&slot_lock);
    {
      nslots++;
      pthread_cond_signal(&slots);
    }
    pthread_mutex_unlock(&slot_lock);
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
  printf("the checksum should is %d\n", total);

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

/*
 * pcp105.c
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
#include <semaphore.h>
#include "buffer.h"

#define SUMSIZE 100
#define MAXCONSUMERS 1
#define BUFSIZE 8

int sum = 0;

sem_t items;
sem_t slots;
pthread_mutex_t my_lock = PTHREAD_MUTEX_INITIALIZER;

int producer_done = 0;

void *producer(void *arg1)
{
  int i;

  for(i = 1; i <= SUMSIZE; i++) {
    sem_wait(&slots);
    put_item(i*i);
    sem_post(&items);
  }

  pthread_mutex_lock(&my_lock);
  producer_done = 1;
  for(i = 0; i < MAXCONSUMERS; i++)
    sem_post(&items);
  pthread_mutex_unlock(&my_lock);

  return NULL;
}

void *consumer(void *arg2)
{
  int myitem;

  for( ; ; ) {
    sem_wait(&items);
    pthread_mutex_lock(&my_lock);
    if(!producer_done) {
      pthread_mutex_unlock(&my_lock);
      get_item(&myitem);
      sem_post(&slots);
      sum += myitem;      
    } else {
      pthread_mutex_unlock(&my_lock);
      break;
    }
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

  /* inicia los semaforos */
  sem_init(&items, 0, 0);
  sem_init(&slots, 0, BUFSIZE);

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

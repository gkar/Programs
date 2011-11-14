/*
 * pcp107.c
 *
 * Estrategia de controlador de señales para resolver el problema del buffer
 * acotado controlado por señales.
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "buffer.h"

#define SUMSIZE 1861
#define BUFSIZE 8
#define SPIN 10000

int sum = 0;

pthread_cond_t slots = PTHREAD_COND_INITIALIZER;
pthread_cond_t items = PTHREAD_COND_INITIALIZER;
pthread_mutex_t slot_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t item_lock = PTHREAD_MUTEX_INITIALIZER;

int nslots = BUFSIZE;
int producer_done = 0;
int nitems = 0;
int totalproduced = 0;
int producer_shutdown = 0;

/* spinit: entra en un ciclo para hacer tiempo
 */
void spinit(void)
{
  int i;
  for(i = 0; i < SPIN; i++)
    ;
}

/* controlador de señales para la desactivacion
 */
void catch_sigusr1(int signo)
{
  pthread_mutex_lock(&slot_lock);
  {
    producer_shutdown = 1;
  }
  pthread_mutex_unlock(&slot_lock);
}

void *producer(void *arg1)
{
  int i;
  sigset_t intmask;

  sigemptyset(&intmask);
  sigaddset(&intmask, SIGUSR1);

  for(i = 1; ; i++) {
    spinit();
    pthread_sigmask(SIG_BLOCK, &intmask, NULL);

    /* adquirir derecho a una ranura */
    pthread_mutex_lock(&slot_lock);
    {
      spinit();
      while ((nslots <= 0) && (!producer_shutdown))
	pthread_cond_wait(&slots, &slot_lock);
      if(producer_shutdown) {
	pthread_mutex_unlock(&slot_lock);
	break;
      }
      nslots--;
    }
    pthread_mutex_unlock(&slot_lock);
    pthread_sigmask(SIG_UNBLOCK, &intmask, NULL);

    spinit();
    put_item(i*i);

    /* renuncia al derecho a un elemento */
    pthread_mutex_lock(&item_lock);
    {
      nitems++;
      totalproduced++;
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
    spinit();
    get_item(&myitem);
    spinit();
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
  double total;
  double tp;
  struct sigaction act;
  sigset_t set;

  fprintf(stderr, "Process ID id %ld\n", (long) getpid());
  sleep(5);

  /* instalar controlador de señales y bloquear */
  act.sa_handler = catch_sigusr1;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  sigaction(SIGUSR1, &act, NULL);
  sigemptyset(&set);
  sigaddset(&set, SIGUSR1);
  sigprocmask(SIG_BLOCK, &set, NULL);

  /* crear hilos */
  if(pthread_create(&consid, NULL, consumer, NULL))
    perror("Could not create consumer");
  else if(pthread_create(&prodid, NULL, producer, NULL))
    perror("Could not create producer");
  fprintf(stderr, "Producer ID = %ld, Consumer ID = %ld\n",
	  (long) prodid, (long) consid);

  /* esperar que los hilos terminen */
  pthread_join(prodid, NULL);
  pthread_join(consid, NULL);

  printf("The threads produced the sum %d\n", sum);

  /* Mostrar los valores correctos */
  total = 0.0;
  tp = (double) totalproduced;
  total = tp*(tp+1)*(2*tp+1)/6.0;
  if(tp > SUMSIZE)
    fprintf(stderr, "*** Overflow ocurrs for more than %d items\n",
	    SUMSIZE);
  printf("The checksum for %4d items is %1.0f\n",
	 totalproduced, total);
  exit(0);
}

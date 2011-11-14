/*
 * buffer.c
 *
 * buffer circular protegido con candados mutex.
 *
 */

#include <pthread.h>
#include "buffer.h"

#define BUFSIZE 8

static int buffer[BUFSIZE];
static int bufin  = 0;
static int bufout = 0;

static pthread_mutex_t buffer_lock = PTHREAD_MUTEX_INITIALIZER;

/* obtener el siguiente elemento del buffer y colocarlo en *itemp. */
void get_item(int *item)
{
  pthread_mutex_lock(&buffer_lock);
  {
    *item = buffer[bufout];
    bufout = (bufout + 1) % BUFSIZE;
  }
  pthread_mutex_unlock(&buffer_lock);
  return;
}

/* colocar un elemento en el buffer en la posicion bufin y actualizar bufin */
void put_item(int item)
{
  pthread_mutex_lock(&buffer_lock);
  {
    buffer[bufin] = item;
    bufin = (bufin + 1) % BUFSIZE;
  }
  pthread_mutex_unlock(&buffer_lock);
  return;
}

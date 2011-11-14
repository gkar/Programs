/*
 * shared.c
 *
 * Implementacion de la cola o buffer limitado
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared.h"

#define FTOK_PATHNAME_SHM "/bin/sh"
#define FTOK_PATHNAME_SEM "/bin/ls"
#define FTOK_PROJID 77
#define EMPTY_SLOT -2
#define NEXT_NULL -1

typedef struct {
  int head;
  int tail;
  int num;
  int max;
} buff_data_t;

typedef struct {
  int data;
  int next;
} buff_item_t;

/* shared memory structures */
buff_data_t *buff_data = NULL;
buff_item_t *buff_list = NULL;
int shmid = 0;
void *shmaddr = NULL;

/* semaphores structures */
enum semaphores {sem_slots, sem_items, sem_buff};
struct sembuf semwait[3];
struct sembuf semsignal[3];
struct sembuf seminit[3];
int semid = 0;

/* fija los valores de los elementos de sembuf */
void set_sembuf_struct(struct sembuf *s, int num, int op, int flg)
{
  s[num].sem_num = (short) num;
  s[num].sem_op = op;
  s[num].sem_flg = flg;

  return;
}

/* buff_init: crea un buffer de tamaño n usando los flag especificados.
 */
int buff_init(int n, int flag)
{
  int i;
  key_t key;

  if ((key = ftok(FTOK_PATHNAME_SHM, FTOK_PROJID)) == -1) {
    fprintf(stderr, "Key not assigned for shared memory\n");
    return(-1);
  }

  if ((shmid = shmget(key, sizeof(buff_data) + (n * sizeof(buff_item_t)), flag)) == -1) {
    fprintf(stderr, "Shared memory not found\n");
    return(-1);
  }

  if ((shmaddr = (void*) shmat(shmid, NULL, 0)) == NULL) {
    fprintf(stderr, "Shared memory not attached\n");
    return(-1);
  }

  buff_data = (buff_data_t *) shmaddr;
  buff_list = (buff_item_t *) ((void *) buff_data + sizeof(buff_data_t));

  if (flag != 0777) {
    fprintf(stderr, "Initializing buffer...");
    buff_data->head = 0;
    buff_data->tail = -1;
    buff_data->num  = 0;
    buff_data->max = n;

    for (i = 0; i < buff_data->max; i++) {
      buff_list[i].data = 0;
      buff_list[i].next = EMPTY_SLOT;
    }
    fprintf(stderr, "done!\n");
  } else {
    fprintf(stderr, "Buffer Assigned\n");
  }

  if ((key = ftok(FTOK_PATHNAME_SEM, FTOK_PROJID)) == -1) {
    fprintf(stderr, "Key not assigned for semaphores\n");
    return(-1);
  }

  if ((semid = semget(key, 3, 0777 | IPC_CREAT)) == -1) {
    fprintf(stderr, "Semaphores not created!\n");
    return(-1);
  }
  
  // sem_slots
  set_sembuf_struct(seminit, sem_slots, buff_data->max, 0);
  set_sembuf_struct(semwait, sem_slots, -1, 0);
  set_sembuf_struct(semsignal, sem_slots, 1, 0);

  // sem_items
  set_sembuf_struct(seminit, sem_items, 0, 0);
  set_sembuf_struct(semwait, sem_items, -1, 0);
  set_sembuf_struct(semsignal, sem_items, 1, 0);

  // sem_buff
  set_sembuf_struct(seminit, sem_buff, 1, 0);
  set_sembuf_struct(semwait, sem_buff, -1, 0);
  set_sembuf_struct(semsignal, sem_buff, 1, 0);

  semop(semid, &seminit[sem_slots], 1);
  semop(semid, &seminit[sem_items], 1);
  semop(semid, &seminit[sem_buff], 1);

  return(0);
}

int buff_put(int e)
{
  int i;
  int semop_ret;

  // sem_wait(&slots);
  while ( ((semop_ret = semop (semid, &semwait[sem_slots], 1)) == -1) &&
          (errno == EINTR) )
    ;
  if (semop_ret == -1) {
    fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
            (long)getpid(), strerror(errno));
    return(-1);
  } else {
  
    /* INI: seccion critica */
    while ( ((semop_ret = semop (semid, &semwait[sem_buff], 1)) == -1) &&
	    (errno == EINTR) )
      ;
    if (semop_ret == -1) {
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	      (long)getpid(), strerror(errno));
      return(-1);
    } else {
      /* busca una posicion libre en el buffer */
      for (i = 0; i < buff_data->max && buff_list[i].next != EMPTY_SLOT; i++)
	;
      buff_list[i].data = e;
      buff_list[i].next = NEXT_NULL;
      if (buff_data->num == 0)
	buff_data->head = i;
      else
	buff_list[buff_data->tail].next = i;

      buff_data->num++;
      buff_data->tail = i;
    
      /* FIN: seccion critica */
      while ( ((semop_ret = semop (semid, &semsignal[sem_buff], 1)) == -1) &&
	      (errno == EINTR) )
	;
      if (semop_ret == -1) {
	fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
		(long)getpid(), strerror(errno));
	return(-1);
      }
    }
    // sem_post(&items);
    while ( ((semop_ret = semop (semid, &semsignal[sem_items], 1)) == -1) &&
            (errno == EINTR) )
      ;

    if (semop_ret == -1) {
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
              (long)getpid(), strerror(errno));
      return(-1);
    }

    return(0);
  }

  return(-1);
}

int buff_get(void)
{
  int i, e;
  int semop_ret;

  // sem_wait(&items);
  while ( ((semop_ret = semop (semid, &semwait[sem_items], 1)) == -1) &&
          (errno == EINTR) )
    ;
  if (semop_ret == -1) {
    fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
            (long)getpid(), strerror(errno));
    return(-1);
  } else {

    /* INI: seccion critica */
    while ( ((semop_ret = semop (semid, &semwait[sem_buff], 1)) == -1) &&
	    (errno == EINTR) )
      ;
    if (semop_ret == -1) {
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
	      (long)getpid(), strerror(errno));
      return(-1);
    } else {

      i = buff_data->head;
      e = buff_list[i].data;
      buff_data->head = buff_list[i].next;
      buff_list[i].next = EMPTY_SLOT;
      buff_data->num--;

      /* FIN: seccion critica */
      while ( ((semop_ret = semop (semid, &semsignal[sem_buff], 1)) == -1) &&
	      (errno == EINTR) )
	;
      if (semop_ret == -1) {
	fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
		(long)getpid(), strerror(errno));
	return(-1);
      }
    }

    // sem_post(&slots);
    while ( ((semop_ret = semop (semid, &semsignal[sem_slots], 1)) == -1) &&
            (errno == EINTR) )
      ;
    if (semop_ret == -1) {
      fprintf(stderr,"[%ld]: Semaforo no valido el acceso: %s\n",
              (long)getpid(), strerror(errno));
      return(-1);
    }

    return(e);
  }
}

void bprint(char *id)
{
  int i;

  fprintf(stdout, "%s:", id);
  for (i = 0; i < buff_data->max; i++)
    fprintf(stdout, "[%d,%d,%d]", i, buff_list[i].data, buff_list[i].next);
  fprintf(stdout, "\n");
}

int buff_close(void)
{
  if ((shmdt(shmaddr) == -1)) {
    perror("Shared memory not detached");
    return(-1);
  }

  buff_data = NULL;
  buff_list = NULL;
  shmid = 0;
  shmaddr = NULL;

  semctl(semid, 0, IPC_RMID);

  return(0);
}

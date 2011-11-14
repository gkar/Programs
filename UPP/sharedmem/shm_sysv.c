#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <sys/shm.h>

int get_shm_mem(void **a, int n, int flag)
{		
  key_t key;
  int shmid;

  if ((key = ftok("/bin/ls", 33)) == -1) {
    fprintf(stderr, "Key not found for shared memory\n");
    exit(1);
  }

  if ((shmid = shmget(key, n*sizeof(int), flag)) == -1) {
    fprintf(stderr, "ID not found for shared memory\n");
    exit(1);
  }

  if ((*a = (int *) shmat (shmid, NULL, 0)) == NULL) {
    fprintf(stderr, "Shared memory not assigned\n");
    exit(1);
  }

  return shmid;
}

int main (int argc, char *argv[])
{
  int i, j = 0, n;
  int shmid, child_status;
  int *array = NULL;
  int ppid;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <number of processes>\n", argv[0]);
    exit(1);
  }

  ppid = getpid();
  n = atoi(argv[1]);

  fprintf(stdout,"*array = %p\n", array);
  shmid = get_shm_mem((void*) &array, n, 0777 | IPC_CREAT);
  fprintf(stdout,"*array = %p\n", array);

  for (i = 0; i < n; i++)
    array[i] = 0;

  for (i = 0; i < n-1; i++)
    if (fork())
      break;

  shmid = get_shm_mem((void*) &array, n, 0777);

  fprintf(stdout, "INI ADD [%d][%d][%d]\n", getpid(), i, j);
  for (j = 0; j < n; j++)
    array[i]++;
  fprintf(stdout, "END ADD [%d][%d][%d]\n", getpid(), i, j);


  wait(&child_status);
  if (!WIFEXITED (child_status)) {
    fprintf (stderr,"the child process %d exited abnormally, with exit code %d\n",
	     getpid(), WEXITSTATUS(child_status));
  }

  if (getpid() == ppid) {
    fprintf(stdout,"Array: ");
    for(i = 0; i < n; i++)
      fprintf(stdout,"%d ", array[i]);
    fprintf(stdout,"\n");
    shmdt(&shmid);
    shmctl (shmid, IPC_RMID, NULL);
  } else {
    shmdt(&shmid);
  }

  return(0);
}

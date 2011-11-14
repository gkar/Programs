#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include "uici.h"

#define MAXBACKLOG 5

/* devolver 1 si hay error, 0 si OK */
int u_ignore_sigpipe()
{
  struct sigaction act;

  if (sigaction(SIGPIPE, (struct sigaction *)NULL, &act) < 0)
    return 1;
  if (act.sa_handler == SIG_DFL) {
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &act, (struct sigaction *) NULL) < 0)
      return 1;
  }
  return 0;
}

/*
 * u_open
 *
 * Devolver un descriptor de archivo vinculado con el puerto dado.
 *
 * parametro:
 * port = numero de puerto con el cual vincularse
 *
 * retorna: descriptor de archivo si hay exito y -1 si hay error
 *
 */
int u_open (u_port_t port)
{
  int sock;
  struct sockaddr_in server;

  if ( (u_ignore_sigpipe() != 0) ||
       ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) )
    return -1;

  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons((short) port);

  if ( (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) ||
       (listen(sock, MAXBACKLOG) < 0) )
    return -1;

  return sock;
}

/*
 * u_listen
 *
 * Escucha para detectar solicitud del nodo especificado en el puerto
 * especificado.
 *
 * parametros:
 * fd = descriptor de archivo previamente vinculado al puerto en el que se
 * escucha.
 * hostn = nombre del nodo que se va a escuchar.
 *
 * retorna: descriptor de archivo para comunicacion o -1 si hay error
 *
 */
int u_listen(int fd, char *hostn)
{
  struct sockaddr_in net_client;
  socklen_t len = sizeof(struct sockaddr);
  int retval;
  struct hostent *hostptr;

  while ( ((retval=accept(fd, (struct sockaddr *)(&net_client), &len)) == -1) &&
	  (errno == EINTR) )
    ;

  if (retval == -1)
    return retval;

  hostptr = gethostbyaddr((char *) &(net_client.sin_addr.s_addr), 4, AF_INET);
  if (hostptr == NULL)
    strcpy (hostn, "unknown");
  else
    strcpy (hostn, (*hostptr).h_name);

  return retval;
}

/*
 * u_connect
 *
 * Iniciar la comunicacion con un servidor remoto.
 *
 * parametros:
 * port = puerto bien conocido en servidor remoto.
 * hostn = cadena de caracteres con el nombre Internet de la maquina remota.
 *
 * retorna: descriptor de archivo para comunicacion o -1 si hay error
 *
 */
int u_connect(u_port_t port, char *hostn)
{
  struct sockaddr_in server;
  struct hostent *hp;
  int sock;
  int retval;

  if ( (u_ignore_sigpipe() != 0) ||
       !(hp = gethostbyname(hostn)) ||
       ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) )
    return -1;

  memcpy ((char *) &server.sin_addr, hp->h_addr_list[0], hp->h_length);

  server.sin_port = htons((short) port);
  server.sin_family = AF_INET;

  while( ((retval=connect(sock, (struct sockaddr *) &server, sizeof(server))) == -1) &&
	 (errno == EINTR) )
    ;

  if (retval == -1) {
    close (sock);
    return -1;
  }

  return sock;
}

/*
 * u_close
 *
 * Cerrar la comunicacion para el descriptor de archivos dado.
 *
 * parametro:
 * fd = descriptor de archivo de la conexion de socket por cerrar.
 *
 * retorna: un valor negativo si hay error.
 *
 */
int u_close(int fd)
{
  return (close(fd));
}

/*
 * u_read
 *
 * Recuperar informacion de un descriptor de archivos abierto con u_open.
 *
 * parametros:
 * fd = descriptor de archivo de la conexion de socket.
 * buf = buffer para la lectura.
 * size = numero de bytes por leer.
 *
 * retorna: el numero de bytes leidos o un valor negativo si hay error.
 *
 */

ssize_t u_read(int fd, void *buf, size_t size)
{
  ssize_t retval;

  while ( ((retval = read(fd, buf, size)) == -1) && (errno == EINTR) )
    ;

  return retval;
}

/*
 * u_write
 *
 * Enviar informacion a un descriptor de archivo abierto con u_open.
 *
 * parametros:
 * fd = descriptor de archivo de la conexion de socket.
 * buf = buffer para la escritura.
 * size = numero de bytes por escribir.
 *
 * retorna: el numero de bytes escritos o un valor negativo si hay error
 *
 */
ssize_t u_write(int fd, void *buf, size_t size)
{
  ssize_t retval;

  while ( ((retval = write(fd, buf, size)) == -1) && (errno == EINTR) )
    ;

  return retval;
}

/*
 * u_error
 *
 * Exhibir mensaje de error como hace perror.
 *
 * parametro:
 * s = cadena que precede al mensaje de error del sistema.
 *
 * retorna: nada
 *
 */
void u_error(char *s)
{
  perror(s);
}

/*
 * u_sync
 *
 * Esta funcion debe invocarse despues de exec o dup para anexar un descriptor
 * de archivo abierto en implementaciones sencillas de UICI. No se necesita
 * cuando se usan sockets. Se conserva por compatibilidad
 *
 * parametro:
 * fd = descriptor de archivo de la conexion de socket.
 *
 * retorna: 0 si exito, -1 si hay error.
 */
int u_sync(int fd)
{
  return fd * 0;
}

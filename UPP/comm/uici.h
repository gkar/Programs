/* Diversas definiciones necesarias para funciones de UICI */

// types
typedef unsigned short u_port_t;

// Functions
int u_open (u_port_t port);
int u_listen (int fd, char *hostn);
int u_connect (u_port_t port, char *inetp);
int u_close (int fd);
ssize_t u_read (int fd, void *buf, size_t nbyte);
ssize_t u_write (int fd, void *buf, size_t nbyte);
void u_error (char *s);
int u_sync (int fd);

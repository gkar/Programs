/*
 * shared.h
 *
 * Conoce y administra las estructuras e informacion compartida por todas las
 * aplicaciones que hacen uso de ellas.
 *
 */

int buff_init(int n, int flag);

int buff_get(void);

int buff_put(int e);

void bprint(char *id);

int buff_close(void);

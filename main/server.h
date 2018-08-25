#ifndef SERVER_H
#define SERVER_H

/*************************************************************************/

/* Externs */

extern int server_quit; 

/*************************************************************************/

extern void read_config(void);
extern void write_config(void);
extern void server_main(void *arg);

/*************************************************************************/

#endif	/* SERVER_H */

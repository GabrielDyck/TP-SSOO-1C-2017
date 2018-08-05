#ifndef UTILS_H
#define UTILS_H
#include <stddef.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>

/*
 * Dado una ip y un puerto, levanta un socket cliente
 * y se conecta a servidor en ip y puerto dado.
 * Retorna el socket cliente conectado.
 */
int conectarseAServidor(char* ip, char* puerto);
void readConfigFile(char *path) ;
void configLog(char *path,char * process) ;
char * assignString(char * string) ;
int crearSocketEscucha(char *puerto, int backlog);
int conectarClienteDesdeServidor(int socketEscucha);
int getClientType(int socket);
#endif /* UTILS_H */

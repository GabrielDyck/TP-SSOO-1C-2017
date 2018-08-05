/*
 * Al usar esto tenes que definir en tu .c
 * int tienePermiso(char* autentificacion){} y retornar 0 si no tiene y 1 si tiene permiso
 */
#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int conectar(int puerto,char* ip);
int autentificar(int conexion, char* autor);
int esperarConfirmacion(int conexion);
void enviarMensaje(int conexion, char* mensaje);
char* esperarMensaje(int conexion);
char* header(int numero);	
int recibirProtocolo(int conexion);
void enviarMensajeConProtocolo(int conexion, char* mensaje, int protocolo);
#endif								


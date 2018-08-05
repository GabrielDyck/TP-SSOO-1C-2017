#ifndef CPU_H_
#define CPU_H_

#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../Funciones/sockets.h"
#include <commons/log.h>
#include <commons/config.h>

int servidor;

int conectarseAlServidor();
void procesarPeticion();

t_log *archivoLog;

void logInfo(char* mensaje) { log_info(archivoLog, mensaje);}
void logDebug(char* mensaje) { log_debug(archivoLog, mensaje);}
void logWarning(char* mensaje) { log_warning(archivoLog, mensaje);}
void logError(char* mensaje) { log_error(archivoLog, mensaje);}


#endif /* CPU_H_ */

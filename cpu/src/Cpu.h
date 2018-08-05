#ifndef CPU_H_
#define CPU_H_

#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../Funciones/sockets.h"
#include "../Funciones/json.h"
#include <commons/log.h>
#include <commons/config.h>
#include <parser/parser.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

#define FUNC_GET_BYTES 1
#define FUNC_SAVE_BYTES 2
#define RECIBIR_PCB 0
#define PROTOCOLO_MORTAL 9

#define FIN_EJECUCION -10

int kernel;
int memoria;

int status = 1;
int finalizado = 0;
int flag = false;

char* IP_KERNEL;
char* PUERTO_KERNEL;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char RUTA_CONFIG[20] = "Cpu.conf";
t_log *LOGGER;
t_config* archivoConfiguracion;
int leerConfiguracion();
void conectarseAlKernel();
void conectarseMemoria();
void procesarPeticion();

int IS_EXECUTING = 0;
pthread_mutex_t mutex_ejecutando;

char* getStringConf(char * configName) {
	return config_get_string_value(archivoConfiguracion,configName);
}

int recvInt(int socket);
int saveBytesOnMemory(int socket, int pid, int page, int offset, int size, void *buffer);
int sendSaveBytesParams(int socket, int pid, int page, int offset, int size, void *buffer);
int getBytesFromMemory(void *buffer, int socket, int pid, int page, int offset, int size);
int recvSaveBytesStatus(int socket);
int sendGetBytesParams(int socket, int pid, int page, int offset, int size);
int recvBytes(void *buffer, int socket);
int conectarseAServidor(char* ip, char* puerto);
void testEscribirEnMemoria();
void testLeerDeMemoria();

void crearHiloSignal();
void hiloSignal();
void cerrarCPU(int senial);
void parsear(char* instruccion);

// Memoria
int executeMemoryAction(int memoriaSocket, int actionId, int *buffer, int pid, int page, int offset, int size);
void procesarPeticionKernel();
char* pedirLineaAMemoria();
void procesarCodigo(int quantum, int quantum_sleep);
void enviarMensajeMemoriaAsignacion(int pag, int off, int size, int proceso, int valor);

///ANSISOP
t_puntero definirVariable(t_nombre_variable identificador_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable nombre_variable);
t_valor_variable dereferenciar(t_puntero pagina);
void asignar(t_puntero pagina, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida	variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida	variable, t_valor_variable valor);
void finalizar();
void retornar(t_valor_variable retorno);
void llamarConRetorno(t_nombre_etiqueta	etiqueta, t_puntero	donde_retornar);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void wait(t_nombre_semaforo identificador_semaforo);
void signalAnsi(t_nombre_semaforo identificador_semaforo);
t_puntero reservar(t_valor_variable espacio);
void liberar(t_puntero puntero);
t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags);
void borrar(t_descriptor_archivo descriptor_archivo);
void cerrar(t_descriptor_archivo descriptor_archivo);
void moverCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void escribir(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);



AnSISOP_funciones funciones = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
		.AnSISOP_dereferenciar 			= dereferenciar,
		.AnSISOP_asignar				= asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel				= irAlLabel,
		.AnSISOP_llamarSinRetorno		= llamarSinRetorno,
		.AnSISOP_llamarConRetorno 		= llamarConRetorno,
		.AnSISOP_finalizar				= finalizar,
		.AnSISOP_retornar				= retornar
};


AnSISOP_kernel kernelParser = {
		.AnSISOP_wait = wait,
		.AnSISOP_signal = signalAnsi,
		.AnSISOP_reservar = reservar,
		.AnSISOP_liberar = liberar,
		.AnSISOP_abrir = abrir,
		.AnSISOP_borrar = borrar,
		.AnSISOP_cerrar = cerrar,
		.AnSISOP_moverCursor = moverCursor,
		.AnSISOP_escribir = escribir,
		.AnSISOP_leer = leer
};

// Stack

PCB* pcb;
int TAMANIO_PAGINA;
Stack* obtenerStack();
Stack* inicializarStack();


int siguientePosicion();
int siguientePosicion();
Variable * crearVariable ( t_nombre_variable variable );


// Helpers
void sumarEnLasVariables(Variable* var);
int sendInt(int socket, int number);



#endif /* CPU_H_ */

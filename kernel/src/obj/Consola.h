#ifndef SRC_OBJ_CONSOLA_H_
#define SRC_OBJ_CONSOLA_H_

#include "../Kernel.h"

#define INICIALIZAR_PROGRAMA 0
#define FINALIZAR_PROGRAMA 1
#define DESCONECTAR_CONSOLA 2
#define FIN -1

#define FINALIZO_OKEY 0
#define RECHAZAR_CONSOLA 1
#define ARCHIVO_NO_EXISTENTE 2
#define LEER_ARCHIVO_SIN_PERMISOS 3
#define ESCRIBIR_ARCHIV_SIN_PERMISOS 4
#define ERROR_DE_MEMORIA 5
#define ALOCAR_MAS_DE_UNA_PAGINA 8
#define NO_HAY_MAS_PAGINAS 9
#define ERROR_SIN_DEFINICION 20
#define PID_EXPULSADO 21
#define PRINT 6

void iniciarlizarConsolas() {
	pthread_mutex_init(&mutex_consolas, NULL);
	consolas = list_create();
	pthread_mutex_init(&mutex_abortar_programas, NULL);
	abortarProgramas = list_create();
}

void noSePuedoCrearElArhivo(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -2;
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, ARCHIVO_NO_EXISTENTE);
	free(mensaje);
}

void agregarConsola(int clientSocket) {
	send(clientSocket, &SUCCESS, sizeof(SUCCESS), 0);
	pthread_mutex_lock(&mutex_consolas);
	list_add(consolas, (void*) clientSocket);
	pthread_mutex_unlock(&mutex_consolas);
}

struct_consola* newConsola(char* codigo, int socket) {
	struct_consola* consola = malloc(sizeof(struct_consola));
	consola->socket = socket;
	consola->codigo = string_duplicate(codigo);
	consola->pid = newProceso(socket);
	return consola;
}

void imprimirEnConsola(int pid, char* dato) {
	int socket = getProceso(pid)->socket;
	enviarMensajeConProtocolo(socket, dato, PRINT);
	char* response = esperarMensaje(socket);
	free(response);
}

void escribirSinPermisos(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -4;
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje,
			ESCRIBIR_ARCHIV_SIN_PERMISOS);
	free(mensaje);
}

void leerSinPermisos(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -3;
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje,
			LEER_ARCHIVO_SIN_PERMISOS);
	free(mensaje);
}

void muerteCPU(int pid) {
	struct_proceso* proceso = getProceso(pid);
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, ERROR_SIN_DEFINICION);
	free(mensaje);
}

void freeConsola(int socketConsola) {
	int i;
	pthread_mutex_lock(&mutex_consolas);
	for (i = 0; i < list_size(consolas); i++) {
		int socket = (int) list_get(consolas, i);
		if (socket == socketConsola) {
			list_remove(consolas, i);
			pthread_mutex_unlock(&mutex_consolas);
			break;
		}
	}
	FD_CLR(socketConsola, &descriptores);
	close(socketConsola);
}

void maximoDescriptorConsolas(int* maximo) {
	int i;
	for (i = 0; i < list_size(consolas); i++) {
		int socket = (int) list_get(consolas, i);
		FD_SET(socket, &descriptores);
		if (socket > *maximo)
			*maximo = socket;
	}
}

bool abortarPidBloquado(int pid) {
	int cantSem;
	int index;
	log_info(archivoLog, "(MAIN) Buscando Pid en Bloqueado: %d", pid);
	pthread_mutex_lock(&mutex_bloqueado);
	for (cantSem = 0; cantSem < list_size(semaforosGlobales); cantSem++) {
		struct_semaforo_global* semaforo = (struct_semaforo_global*) list_get(
				semaforosGlobales, cantSem);
		for (index = 0; index < queue_size(semaforo->cola); index++) {
			char* pcb = (char*) queue_pop(semaforo->cola);
			char *charId = toSubString(pcb, 0, 3);
			int id = atoi(charId);
			if (pid == id) {
				log_info(archivoLog, "(MAIN) Pid Encontrado");
				semaforo->valor++;
				pthread_mutex_unlock(&mutex_bloqueado);
				return true;
			}
			queue_push(semaforo->cola, pcb);
		}
	}
	pthread_mutex_unlock(&mutex_bloqueado);
	log_info(archivoLog, "(MAIN) Pid No Encontrado");
	return false;
}

void abortarPid(int pid) {
	if (abortarPidBloquado(pid)) {
		struct_proceso* proceso = getProceso(pid);
		int liberoTodoOkey = proceso->cantidadDeLiberaciones
				== proceso->cantidadDeAlocamientos;
		char* mensaje = toStringInt(liberoTodoOkey);
		enviarMensajeConProtocolo(proceso->socket, mensaje, PID_EXPULSADO);

		free(mensaje);
		agregarAExit(proceso);
		logInfo("(MAIN) Proceso abortado en Bloquados");
	} else {
		logInfo("(MAIN) Agregarndo en programa para abortar");
		pthread_mutex_lock(&mutex_abortar_programas);
		list_add(abortarProgramas, (void*) pid);
		pthread_mutex_unlock(&mutex_abortar_programas);
		logInfo("(MAIN) Agregado Ok");
	}
}

void abortarConsola(int socket) {
	log_info(archivoLog, "(MAIN) Abortar Consola");
	char* pidChar = esperarMensaje(socket);
	log_info(archivoLog, "(MAIN) Consola me mando %s ", pidChar);
	int pid = atoi(pidChar);
	free(pidChar);
	getProceso(pid)->exitCode = -7;
	abortarPid(pid);
}

void desconectarConsolaForsosa(int socket) {
	char* pidChar = esperarMensaje(socket);
	int pid = atoi(pidChar);
	free(pidChar);
	getProceso(pid)->exitCode = -6;
	abortarPid(pid);
}

void revisarActividadConsolas() {
	int i;
	for (i = 0; i < list_size(consolas); i++) {
		int consola = (int) list_get(consolas, i);
		if (FD_ISSET(consola, &descriptores)) {
			int protocolo = recibirProtocolo(consola);
			switch (protocolo) {
			case INICIALIZAR_PROGRAMA:
				iniciarPrograma(consola);
				break;
			case FINALIZAR_PROGRAMA:
				abortarConsola(consola);
				break;
			case DESCONECTAR_CONSOLA:
				desconectarConsolaForsosa(consola);
				break;
			case FIN:
				freeConsola(consola);
				break;
			}
		}
	}
}

void errorSinDefinicion(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -20;
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, ERROR_SIN_DEFINICION);
	free(mensaje);
}

int filtrarDesconeccionConsola(int pid) {
	int i;
	int desconectado = 0;
	pthread_mutex_lock(&mutex_abortar_programas);
	for (i = 0; i < list_size(abortarProgramas); i++) {
		if (((int) list_get(abortarProgramas, i)) == pid) {
			list_remove(abortarProgramas, i);
			desconectado = 1;
			struct_proceso* proceso = getProceso(pid);
			int liberoTodoOkey = proceso->cantidadDeLiberaciones
					== proceso->cantidadDeAlocamientos;
			char* mensaje = toStringInt(liberoTodoOkey);
			enviarMensajeConProtocolo(proceso->socket, mensaje, PID_EXPULSADO);
			free(mensaje);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_abortar_programas);
	return desconectado;
}

void rechazarConsola(int pid) {
	struct_proceso* proceso = getProceso(pid);
	enviarProtocolo(proceso->socket, RECHAZAR_CONSOLA);
	proceso->exitCode = -1;
	agregarAExit(proceso);
}

struct_new* crearNew(char* codigo, int socket) {
	int pid = newProceso(socket);
	struct_new* new = malloc(sizeof(struct_new));
	new->codigo = codigo;
	new->pid = pid;
	return new;
}

void iniciarPrograma(int socket) {
	char* codigo = esperarMensaje(socket);
	logInfo(codigo);
	pthread_mutex_lock(&mutex_news);
	struct_new* new = crearNew(codigo, socket);
	queue_push(news, new);
	pthread_mutex_unlock(&mutex_news);
	char* pidChar = toStringInt(new->pid);
	enviarMensaje(socket, pidChar);
	free(pidChar);
	sem_post(&sem_news);
}

void finalizarPrograma(int pid) {
	removerDeEjecutados(pid);
	logInfo("(MAIN) Liberando Memoria ...");
	liberarRecursos(pid);
	logInfo("(MAIN) Insertando En Terminados ...");
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = 0;
	agregarAExit(proceso);
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, FINALIZO_OKEY);
	free(mensaje);
	logInfo("(MAIN) Eliminando Consola...");
}

void tamanioDeAlocamientoIncorrecto(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -8;
	agregarAExit(proceso);
	liberarRecursos(pid);
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje,
			ALOCAR_MAS_DE_UNA_PAGINA);
	free(mensaje);
}

void faltaDePaginasParaElHeap(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -9;
	agregarAExit(proceso);
	liberarRecursos(pid);
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, NO_HAY_MAS_PAGINAS);
	free(mensaje);
}

void informarErrorDeMemoria(int pid) {
	struct_proceso* proceso = getProceso(pid);
	proceso->exitCode = -5;
	agregarAExit(proceso);
	liberarRecursos(pid);
	int liberoTodoOkey = proceso->cantidadDeLiberaciones
			== proceso->cantidadDeAlocamientos;
	char* mensaje = toStringInt(liberoTodoOkey);
	enviarMensajeConProtocolo(proceso->socket, mensaje, ERROR_DE_MEMORIA);
	free(mensaje);
}

#endif /* SRC_OBJ_CONSOLA_H_ */

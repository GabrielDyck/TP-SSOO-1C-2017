#ifndef SRC_OBJ_CPU_H_
#define SRC_OBJ_CPU_H_

#include "../Kernel.h"

#define FIN_QUANTUM 5
#define FIN_EJECUCION 1
#define OBTENER_VARIABLE_GLOBAL 2
#define SET_VARIABLE_GLOBAL 3
#define WAIT_SEMAFORO 4
#define SIGNAL_SEMAFORO 6
#define ALOCAR_MEMORIA 7
#define LIBERAR_MEMORIA 8
#define FINALIZAR_PROGRAMA_CPU 9
#define FIN_CPU_ERROR_DE_MEMORIA 10
#define MUERTE_SUBITA -1

#define ABRIR_ARCHIVO 11
#define BORRAR_ARCHIVO 12
#define CERRAR_ARCHIVO 13
#define MOVER_CURSOR 14
#define ESCRIBIR_ARCHIVO 15
#define LEER_ARCHIVO 16

void iniciarlizarCPU() {
	logInfo("(MAIN) Inicializar CPU");
	cpus = list_create();
	sem_init(&sem_cpu, 0, 0);
	pthread_mutex_init(&mutex_cpus, NULL);
	pthread_mutex_init(&mutex_mensaje_cpu, NULL);
}

void agregarCPU(int socket) {
	logInfo("(MAIN) Agregar CPU");
	struct_cpu* newCpu = malloc(sizeof(struct_cpu));
	newCpu->socket = socket;
	newCpu->disponible = 1;
	enviarProtocolo(socket, tamanioPagina);
	pthread_mutex_lock(&mutex_cpus);
	list_add(cpus, (void*) newCpu);
	pthread_mutex_unlock(&mutex_cpus);
	sem_post(&sem_cpu);
}

int obtenerValorVariableGlobal(t_list* variables, char* nombre) {
	int i;
	for (i = 0; i < list_size(variables); i++) {
		struct_variable_global * variableGlobal = list_get(variables, i);
		if (string_equals_ignore_case(nombre, variableGlobal->nombre)) {
			return variableGlobal->valor;
		}
	}
	logError("(MAIN) NO SE ENCONTRO LA VARIABLE GLOBAL");
	return 0;
}

void setValorVariableGlobal(t_list* variables, char* nombre, int valor) {
	int i;
	for (i = 0; i < list_size(variables); i++) {
		struct_variable_global * variableGlobal = list_get(variables, i);
		if (string_equals_ignore_case(nombre, variableGlobal->nombre)) {
			variableGlobal->valor = valor;
			return;
		}
	}
	logError("(MAIN) NO SE ENCONTRO LA VARIABLE GLOBAL");
}

void recibirVariableGlobal(int cpu) {
	logInfo("(MAIN) Esperando Setear Variable Global");
	char* mensaje = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) CPU envia : %s", mensaje);
	char* valor = toSubString(mensaje, 0, 3);
	log_info(archivoLog, "(MAIN) Valor: %s", valor);
	char* nombre = toSubString(mensaje, 4, string_length(mensaje) - 1);
	char* nombreReal = string_new();
	string_append(&nombreReal, "!");
	string_append(&nombreReal, nombre);
	log_info(archivoLog, "(MAIN) Nombre: %s", nombreReal);
	setValorVariableGlobal(variablesGlobales, nombreReal, atoi(valor));
	free(mensaje);
	free(valor);
	free(nombre);
	free(nombreReal);
	logInfo("(MAIN) Seteado");
	enviarMensaje(cpu, "0001");
}

void enviarVariableGlobal(int cpu) {
	logInfo("(MAIN) Esperando nombre de la Variable Global .....");
	char* variable = esperarMensaje(cpu);
	char* nombreReal = string_new();
	string_append(&nombreReal, "!");
	string_append(&nombreReal, variable);
	log_info(archivoLog, "(MAIN) Enviando Variable Global : %s ....", nombreReal);
	int valor = obtenerValorVariableGlobal(variablesGlobales, nombreReal);
	char* mensaje = toStringInt(valor);
	log_info(archivoLog, "Enviando valor: %s", mensaje);
	enviarMensaje(cpu, mensaje);
	free(mensaje);
	free(variable);
	free(nombreReal);

}

void renovarCPU(int socket, char * mensaje) {
	eliminarCPUDelProceso(socket);
	logInfo("(MAIN) Renovar CPU..");
	struct_cpu* cpu = buscarCpu(socket);
	char* continuaChar = toSubString(mensaje, 0, 3);
	int continua = atoi(continuaChar);
	free(continuaChar);
	if (continua) {
		logInfo("(MAIN) CPU Renovada");
		cpu->disponible = 1;
		sem_post(&sem_cpu);
	} else {
		removerCPU(cpu);
	}
}
void bloquear(char* pcb, struct_semaforo_global* semaforo) {
	char* charPid = toSubString(pcb, 0, 3);
	int pid = atoi(charPid);
	free(charPid);
	removerDeEjecutados(pid);
	if (!filtrarDesconeccionConsola(pid)) {
		pthread_mutex_lock(&mutex_bloqueado);
		queue_push(semaforo->cola, pcb);
		pthread_mutex_unlock(&mutex_bloqueado);
		logInfo("(MAIN) Bloquado Ok");
	} else {
		semaforo->valor ++;
		logInfo("(MAIN) Pid Abortado");
		agregarAExit(getProceso(pid));
		liberarRecursos(pid);
	}
}

struct_semaforo_global* obtenerSemaforo(char* nombreSemaforo) {
	int i;
	for (i = 0; i < list_size(semaforosGlobales); i++) {
		struct_semaforo_global * semaforo = list_get(semaforosGlobales, i);
		if (string_equals_ignore_case(nombreSemaforo, semaforo->nombre)) {
			return semaforo;
		}
	}
	return 0;
}

void wait(char* nombreSemaforo, int cpu) {
	struct_semaforo_global* semaforo = obtenerSemaforo(nombreSemaforo);
	semaforo->valor--;
	log_info(archivoLog, "(MAIN) Semaforo en %d", semaforo->valor);
	if (semaforo->valor < 0) {
		logInfo("(MAIN) Bloquando Proceso..");
		enviarMensaje(cpu, "0000");
		char* mensaje = esperarMensaje(cpu);
		log_info(archivoLog, "(MAIN) Mensaje CPU: %s", mensaje);
		renovarCPU(cpu, mensaje);
		char* pcb = toSubString(mensaje, 4, string_length(mensaje));
		free(mensaje);
		bloquear(pcb, semaforo);
	} else {
		enviarMensaje(cpu, "0001");
		logInfo("(MAIN) Sin Bloqueo");
	}

}

void waitSemaforo(int cpu) {
	logInfo("(MAIN) Wait Semaforo");
	char* semaforo = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) semaforo: %s", semaforo);
	wait(semaforo, cpu);
	free(semaforo);
	logInfo("(MAIN) Wait Semaforo Ok");
}

void signalSemaforo(int cpu) {
	logInfo("(MAIN) Signal");
	char* semaforo = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) Cpu envia: %s", semaforo);
	signal(semaforo);
	enviarMensaje(cpu, "0001");
	free(semaforo);
}

void abrirArchivoAnSISOP(int cpu) {
	logInfo("(MAIN) Abrindo archivo ...");

	char* mensajeCpu = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) La CPU me Envio : %s", mensajeCpu);
	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);

	char* lectura = toSubString(mensajeCpu, 4, 7);
	char* escritura = toSubString(mensajeCpu, 8, 11);
	char* creacion = toSubString(mensajeCpu, 12, 15);

	t_banderas flags = { .lectura = atoi(lectura), .escritura = atoi(escritura),
			.creacion = atoi(creacion) };

	free(lectura);
	free(escritura);
	free(creacion);

	char* direccionArchivo = toSubString(mensajeCpu, 16,
			string_length(mensajeCpu) - 1);

	free(mensajeCpu);
	int fileDescriptor = abrirArchivo(direccionArchivo, flags, pid);

	free(direccionArchivo);

	if (fileDescriptor) {
		log_info(archivoLog, "(MAIN) Archivo Abierto, FD: %d", fileDescriptor);
		char* fileDescriptorChar = toStringInt(fileDescriptor);
		enviarMensaje(cpu, fileDescriptorChar);
		free(fileDescriptorChar);
	} else {
		log_warning(archivoLog, "(MAIN) Fallo al abrir el Archivo");
		enviarMensaje(cpu, "0000");
		removerDeEjecutados(pid);
		agregarAExit(getProceso(pid));
		char* cpuStatus = esperarMensaje(cpu);
		log_info(archivoLog, "(MAIN) La CPU me Envio : %s", cpuStatus);
		renovarCPU(cpu, cpuStatus);
		free(cpuStatus);
	}
}

void eliminarArchivoAnSISOP(int cpu) {
	logInfo("(MAIN) Eliminar archivo");

	char* mensajeCpu = esperarMensaje(cpu);
	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);
	char* fileDescriptorChar = toSubString(mensajeCpu, 4, 7);
	int fileDescriptor = atoi(fileDescriptorChar);
	free(fileDescriptorChar);
	free(mensajeCpu);

	int confirmacion = eliminarArchivo(pid, fileDescriptor);

	if (confirmacion) {
		enviarMensaje(cpu, "0001");
	} else {
		enviarMensaje(cpu, "0000");
		removerDeEjecutados(pid);
		agregarAExit(getProceso(pid));
		char* cpuStatus = esperarMensaje(cpu);
		renovarCPU(cpu, cpuStatus);
		free(cpuStatus);
	}

}

void cerrarArchivoAnSISOP(int cpu) {
	logInfo("(MAIN) Cerrando archivo ...");

	char* mensajeCpu = esperarMensaje(cpu);

	log_info(archivoLog, "(MAIN) La CPU me envio : %s", mensajeCpu);

	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);
	char* fileDescriptorChar = toSubString(mensajeCpu, 4, 7);
	int fileDescriptor = atoi(fileDescriptorChar);
	free(fileDescriptorChar);
	free(mensajeCpu);

	cerrarArchivo(pid, fileDescriptor);

	enviarMensaje(cpu, "0001");
	logInfo("(MAIN) Archivo Cerrado");

}

void moverCursorAnSISOP(int cpu) {
	logInfo("(MAIN) Mover cursor en archivo");

	char* mensajeCpu = esperarMensaje(cpu);

	log_info(archivoLog, "(MAIN) Recibi de la CPU : %s", mensajeCpu);

	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);

	char* fileDescriptorChar = toSubString(mensajeCpu, 4, 7);
	int fileDescriptor = atoi(fileDescriptorChar);
	free(fileDescriptorChar);

	char* posicionChar = toSubString(mensajeCpu, 8, 11);
	int posicion = atoi(posicionChar);
	free(posicionChar);

	free(mensajeCpu);

	moverCursor(pid, fileDescriptor, posicion);
	enviarMensaje(cpu, "0000");
}

void escribirAnSISOP(int cpu) {
	logInfo("(MAIN) Escribir archivo");

	char* mensajeCpu = esperarMensaje(cpu);
	send(cpu, &cpu, sizeof(int), 0);
	log_info(archivoLog, "(MAIN) Recibi de la CPU : %s", mensajeCpu);

	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);

	char* fileDescriptorChar = toSubString(mensajeCpu, 4, 7);
	int fileDescriptor = atoi(fileDescriptorChar);
	free(fileDescriptorChar);

	char* tamanioChar = toSubString(mensajeCpu, 8, 11);
	int tamanio = atoi(tamanioChar);
	free(tamanioChar);
	free(mensajeCpu);

	void* dato = malloc(tamanio);

	recv(cpu, dato, tamanio, MSG_WAITALL);

	int confirmacion = 1;

	if (fileDescriptor == 1)
		imprimirEnConsola(pid, dato);
	else
		confirmacion = escribirArchivo(pid, fileDescriptor, dato, tamanio);

	free(dato);

	if (confirmacion) {
		enviarMensaje(cpu, "0001");
	} else {
		log_error(archivoLog, "(MAIN) Error al escribir, FS responde: %d", confirmacion);
		enviarMensaje(cpu, "0000");
		removerDeEjecutados(pid);
		agregarAExit(getProceso(pid));
		char* cpuStatus = esperarMensaje(cpu);
		renovarCPU(cpu, cpuStatus);
		free(cpuStatus);
	}
}

void leerAnSISOP(int cpu) {
	logInfo("(MAIN) Leer archivo");

	char* mensajeCpu = esperarMensaje(cpu);

	log_info(archivoLog, "(MAIN) La CPU me envio : %s", mensajeCpu);

	char* pidChar = toSubString(mensajeCpu, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);

	char* fileDescriptorChar = toSubString(mensajeCpu, 4, 7);
	int fileDescriptor = atoi(fileDescriptorChar);
	free(fileDescriptorChar);

	char* tamanioChar = toSubString(mensajeCpu, 8, 11);
	int tamanio = atoi(tamanioChar);
	free(tamanioChar);

	free(mensajeCpu);

	void * lectura = leerArchivo(pid, fileDescriptor, tamanio);

	if (lectura != NULL) {
		enviarMensaje(cpu, "0001");
		esperarConfirmacion(cpu);
		send(cpu, lectura, tamanio, 0);
		free(lectura);
	} else {
		enviarMensaje(cpu, "0000");
		removerDeEjecutados(pid);
		agregarAExit(getProceso(pid));
		char* cpuStatus = esperarMensaje(cpu);
		renovarCPU(cpu, cpuStatus);
		free(cpuStatus);
	}

}

void signal(char* nombreSemaforo) {
	struct_semaforo_global* semaforo = obtenerSemaforo(nombreSemaforo);
	if (semaforo->valor < 0) {
		logInfo("(MAIN) Desbloquear Proceso");
		char* pcb = queue_pop(semaforo->cola);
		agregarAReady(pcb);
	}
	semaforo->valor++;
}

void finScript(int cpu) {
	logInfo("(MAIN) Fin Script");
	logInfo("(MAIN) Esperando mensaje ...");
	char* mensaje = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) Mensaje resivido : %s", mensaje);
	renovarCPU(cpu, mensaje);
	char* pidChar = toSubString(mensaje, 4, 7);
	int pid = atoi(pidChar);
	free(pidChar);
	removerDeEjecutados(pid);
	finalizarPrograma(pid);
	free(mensaje);
}

void finDeQuantum(int socket) {
	logInfo("(MAIN) Fin de quantum esperando mensaje...");
	char* mensaje = esperarMensaje(socket);
	log_info(archivoLog, "(MAIN) CPU me envia: %s", mensaje);
	char * pcb = toSubString(mensaje, 4, string_length(mensaje) - 1);
	char * pidChar = toSubString(pcb, 0, 3);
	int pid = atoi(pidChar);
	free(pidChar);
	logInfo("(MAIN) Insertando en Ready");
	removerDeEjecutados(pid);
	agregarAReady(pcb);
	renovarCPU(socket, mensaje);
	free(mensaje);
}

void liberarMemoriaReservada(int cpu) {
	logInfo("(MAIN) Liberando Memoria Reservada");
	char* mensaje = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) Resivi de la CPU : %s", mensaje);
	char* pidChar = toSubString(mensaje, 0, 3);
	char* paginaChar = toSubString(mensaje, 4, 7);
	char* offsetChar = toSubString(mensaje, 8, 11);
	liberarDelHeap(atoi(pidChar), atoi(paginaChar), atoi(offsetChar));
	logInfo("(MAIN) Liberado");
	enviarMensaje(cpu, "0001");
	free(mensaje);
	free(paginaChar);
	free(offsetChar);
}

void matarCPU(struct_cpu* cpu) {
	logWarning("(MAIN) Matar CPU");
	eliminarCPUDelProceso(cpu->socket);
	removerCPU(cpu);
	logInfo("(MAIN) CPU Destruida");
}

void finalizarProgramaCPU(struct_cpu* cpu) {
	logInfo("(MAIN) Finaliza la CPU");
	eliminarCPUDelProceso(cpu->socket);
	char* pidChar = esperarMensaje(cpu->socket);
	log_info(archivoLog, "(MAIN) CPU me mando : %s", pidChar);
	int pid = atoi(pidChar);
	removerDeEjecutados(pid);
	struct_proceso* proceso = getProceso(pid);
	agregarAExit(proceso);
	errorSinDefinicion(pid);
}

void errorDeMemoria(int cpu) {
	logInfo("(MAIN) Error De Memoria");
	char* mensaje = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) La CPU me dijo %s", mensaje);
	renovarCPU(cpu, mensaje);
	char* statusChar = toSubString(mensaje, 0, 3);
	char* pidChar = toSubString(mensaje, 4, 7);
	int pid = atoi(pidChar);
	free(statusChar);
	free(pidChar);
	removerDeEjecutados(pid);
	informarErrorDeMemoria(pid);
}

void revisarActividadCpus() {
	int i;
	pthread_mutex_lock(&mutex_mensaje_cpu);
	for (i = 0; i < list_size(cpus); i++) {
		pthread_mutex_lock(&mutex_planificacion);
		pthread_mutex_unlock(&mutex_planificacion);
		struct_cpu* cpu = (struct_cpu*) list_get(cpus, i);
		if ((FD_ISSET(cpu->socket, &descriptores))) {
			int socket = cpu->socket;
			int protocolo = recibirProtocolo(socket);
			switch (protocolo) {
			case FIN_QUANTUM:
				finDeQuantum(socket);
				break;
			case FIN_EJECUCION:
				finScript(socket);
				break;
			case OBTENER_VARIABLE_GLOBAL:
				aumentarsysCalls(socket);
				enviarVariableGlobal(socket);
				break;
			case SET_VARIABLE_GLOBAL:
				aumentarsysCalls(socket);
				recibirVariableGlobal(socket);
				break;
			case WAIT_SEMAFORO:
				aumentarsysCalls(socket);
				waitSemaforo(socket);
				break;
			case SIGNAL_SEMAFORO:
				aumentarsysCalls(socket);
				signalSemaforo(socket);
				break;
			case ALOCAR_MEMORIA:
				aumentarsysCalls(socket);
				reservarMemoria(socket);
				break;
			case LIBERAR_MEMORIA:
				aumentarsysCalls(socket);
				liberarMemoriaReservada(socket);
				break;
			case FINALIZAR_PROGRAMA_CPU:
				finalizarProgramaCPU(cpu);
				break;
			case FIN_CPU_ERROR_DE_MEMORIA:
				errorDeMemoria(socket);
				break;
			case MUERTE_SUBITA:
				matarCPU(cpu);
				break;
			case ABRIR_ARCHIVO:
				aumentarsysCalls(socket);
				abrirArchivoAnSISOP(socket);
				break;
			case BORRAR_ARCHIVO:
				aumentarsysCalls(socket);
				eliminarArchivoAnSISOP(socket);
				break;
			case CERRAR_ARCHIVO:
				aumentarsysCalls(socket);
				cerrarArchivoAnSISOP(socket);
				break;
			case MOVER_CURSOR:
				aumentarsysCalls(socket);
				moverCursorAnSISOP(socket);
				break;
			case ESCRIBIR_ARCHIVO:
				aumentarsysCalls(socket);
				escribirAnSISOP(socket);
				break;
			case LEER_ARCHIVO:
				aumentarsysCalls(socket);
				leerAnSISOP(socket);
				break;
			default:
				log_error(archivoLog, "(MAIN) CPU me envio le protocolo : %d ",
						protocolo);
				break;
			}
			break;
		}
	}
	pthread_mutex_unlock(&mutex_mensaje_cpu);
}

struct_cpu* buscarCpu(int socket) {
	int i;
	for (i = 0; i < list_size(cpus); i++) {
		struct_cpu* cpu = (struct_cpu*) list_get(cpus, i);
		if (cpu->socket == socket)
			return cpu;
	}
	return NULL;
}

int buscarCpuDisponible() {
	sem_wait(&sem_cpu);
	pthread_mutex_lock(&mutex_cpus);
	int i;
	for (i = 0; i < list_size(cpus); i++) {
		struct_cpu* cpu = (struct_cpu*) list_get(cpus, i);
		if (cpu->disponible) {
			cpu->disponible = 0;
			pthread_mutex_unlock(&mutex_cpus);
			return cpu->socket;
		}
	}
	pthread_mutex_unlock(&mutex_cpus);
	return -1;
}

void removerCPU(struct_cpu* cpu) {
	logWarning("(MAIN) Remover CPU");
	int i;
	pthread_mutex_lock(&mutex_cpus);
	for (i = 0; i < list_size(cpus); i++) {
		struct_cpu* cpuAcutal = (struct_cpu*) list_get(cpus, i);
		if (cpuAcutal->socket == cpu->socket) {
			list_remove(cpus, i);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_cpus);
	FD_CLR(cpu->socket, &descriptores);
	close(cpu->socket);
	free(cpu);
}

void maximoDescriptorCpus(int* maximo) {
	int i;
	for (i = 0; i < list_size(cpus); i++) {
		struct_cpu* cpu = (struct_cpu*) list_get(cpus, i);
		int socket = cpu->socket;
		FD_SET(socket, &descriptores);
		if (socket > *maximo)
			*maximo = socket;
	}
}

void enviarPCB(char* pcb) {
	logInfo("(PCP) Buscando CPUS...");
	int cpu = buscarCpuDisponible();
	logInfo("(PCP) CPU OK");
	char* mensaje = string_new();
	char* quantumChar;
	if (string_equals_ignore_case(CONF_DATA.algoritmo, "FIFO")) {
		quantumChar = string_new();
		string_append(&quantumChar, "0000");
	} else
		quantumChar = toStringInt(CONF_DATA.quantum);
	char* quantumSleepChar = toStringInt(CONF_DATA.quantumSleep);
	string_append(&mensaje, quantumChar);
	string_append(&mensaje, quantumSleepChar);
	string_append(&mensaje, pcb);
	log_info(archivoLog, "(PCP) Eviando a PCU: %d : %s",cpu , mensaje);
	pthread_mutex_lock(&mutex_mensaje_cpu);
	enviarMensaje(cpu, mensaje);
	esperarConfirmacion(cpu);
	pthread_mutex_unlock(&mutex_mensaje_cpu);
	char* pidChar = toSubString(pcb, 0, 3);
	int pid = atoi(pidChar);
	struct_proceso* proceso = getProceso(pid);
	proceso->cantidadDeRafagas++;
	proceso->cpu = cpu;
	free(quantumChar);
	free(quantumSleepChar);
	free(mensaje);
	logInfo("(PCP) Confirmo CPU");
}

void reservarMemoria(int cpu) {
	logInfo("(MAIN) Reservar Memoria");
	char* mensaje = esperarMensaje(cpu);
	log_info(archivoLog, "(MAIN) Resivi de CPU el mensaje: %s", mensaje);
	char* pidChar = toSubString(mensaje, 0, 3);
	char* sizeChar = toSubString(mensaje, 4, 7);
	free(mensaje);
	int sizeReal = atoi(sizeChar) + 5;
	free(sizeChar);
	int pid = atoi(pidChar);
	free(pidChar);

	if (sizeReal > tamanioPagina) {
		logError("(MAIN) Se intento reservar mas de una Pagina");
		alocamientoFail(cpu, pid);
		tamanioDeAlocamientoIncorrecto(pid);
		return;
	} else {
		char* response = alocarMemoria(cpu, pid, sizeReal);
		if (response != NULL) {
			enviarMensaje(cpu, response);
		} else {
			alocamientoFail(cpu, pid);
			faltaDePaginasParaElHeap(pid);
		}
	}
}

void alocamientoFail(int cpu, int pid) {
	char* response = toStringInt(ALOCAMIENTO_FAIL);
	enviarMensaje(cpu, response);
	free(response);
	char* statusChar = esperarMensaje(cpu);
	renovarCPU(cpu, statusChar);
	free(statusChar);
	removerDeEjecutados(pid);
}

#endif

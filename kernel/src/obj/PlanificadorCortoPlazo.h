#ifndef SRC_OBJ_PLANIFICADORCORTOPLAZO_H_
#define SRC_OBJ_PLANIFICADORCORTOPLAZO_H_

#include "../Kernel.h"

void inicializarPlanificadorCortoPlazo() {
	ready = queue_create();
	exec = list_create();
	sem_init(&sem_ready, 0, 0);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
	pthread_create(&planificadorCortoPlazo_h, NULL,
			(void*) planificadorCortoPlazo, NULL);
}

void planificadorCortoPlazo() {
	logInfo("(PCP) Iniciar Planificador Corto Plazo");
	while (1) {
		logInfo("(PCP) Esperando nuevo Candidato");
		sem_wait(&sem_ready);
		pthread_mutex_lock(&mutex_planificacion);
		pthread_mutex_unlock(&mutex_planificacion);
		logInfo("(PCP) Nuevo candidato encontrado");
		pthread_mutex_lock(&mutex_ready);
		char* pcb = (char*) queue_pop(ready);
		pthread_mutex_unlock(&mutex_ready);
		char* pidChar = toSubString(pcb, 0, 3);
		int pid = atoi(pidChar);
		free(pidChar);
		if (!filtrarDesconeccionConsola(pid)) {
			logInfo("(PCP) Enviando PCB ...");
			enviarPCB(pcb);
			pthread_mutex_lock(&mutex_exec);
			list_add(exec, (void*) pid);
			pthread_mutex_unlock(&mutex_exec);
			logInfo("(PCP) En Ejecucion");
		} else {
			agregarAExit(getProceso(pid));
			liberarRecursos(pid);
		}
		free(pcb);
		logInfo("(PCP) PCB Limpio");
	}
}

void agregarAReady(char* pcb) {
	pthread_mutex_lock(&mutex_ready);
	queue_push(ready, pcb);
	pthread_mutex_unlock(&mutex_ready);
	sem_post(&sem_ready);
}

void removerDeEjecutados(int pid) {
	int i;
	pthread_mutex_lock(&mutex_exec);
	for (i = 0; i < list_size(exec); i++) {
		int pidExc =(int) list_get(exec, i);
		if (pidExc == pid) {
			list_remove(exec, i);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_exec);
}


#endif

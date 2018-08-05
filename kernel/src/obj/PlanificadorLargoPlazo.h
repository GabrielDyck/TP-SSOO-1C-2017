/*
 * PlanificadorLargoPlazo.h
 *
 *  Created on: 9/7/2017
 *      Author: utnso
 */

#ifndef SRC_OBJ_PLANIFICADORLARGOPLAZO_H_
#define SRC_OBJ_PLANIFICADORLARGOPLAZO_H_

#include <stddef.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <parser/metadata_program.h>
#include "Consola.h"

void inicializarPlanificadorLargoPlazo() {
	news = queue_create();
	sem_init(&sem_news, 0, 0);
	sem_init(&sem_multiprogramacion, 0, CONF_DATA.gradoMultiprog);
	pthread_mutex_init(&mutex_news, NULL);

	pthread_create(&planificadorLargoPlazo_h, NULL,
			(void*) planificadorLargoPlazo, NULL);
}

struct_new* obetenerNew() {
	pthread_mutex_lock(&mutex_news);
	struct_new* consola = (struct_new*) queue_pop(news);
	pthread_mutex_unlock(&mutex_news);
	return consola;
}

char * createPCB(struct_new* new) {
	PCB* pcb = malloc(sizeof(PCB));
	t_metadata_program* metadata = metadata_desde_literal(new->codigo);
	pcb->indices = metadata;
	pcb->paginas_codigo = cantidadDePaginas(new->codigo);
	pcb->stack = list_create();
	pcb->id = new->pid;
	pcb->pc = 0;
	char* pcbString = toStringPCB(*pcb);
	liberarPCB(pcb);
	return pcbString;
}

void planificadorLargoPlazo() {
	logInfo("(PLP)Inicial Planificador Largo Plazo");
	struct_new* new;
	while (1) {
		sem_wait(&sem_news);
		int permisosDeMultiprogramacion = 0;
		logInfo("(PLP) Esperando la Multiprogramacion ...");
		while (!permisosDeMultiprogramacion) {
			pthread_mutex_lock(&mutex_planificacion);
			pthread_mutex_unlock(&mutex_planificacion);
			sem_wait(&sem_multiprogramacion);
			pthread_mutex_lock(&mutex_multiprogramacion);
			if (cantidadEnEjecucion < CONF_DATA.gradoMultiprog) {
				cantidadEnEjecucion++;
				pthread_mutex_unlock(&mutex_multiprogramacion);
				logInfo("(PLP) Esperando candidatos...");
				pthread_mutex_lock(&mutex_planificacion);
				pthread_mutex_unlock(&mutex_planificacion);
				new = obetenerNew();
				logInfo("(PLP) Candidato Encontrado");

				int cantPaginas = cantidadDePaginas(new->codigo)
						+ CONF_DATA.stackSize;
				logInfo("(PLP) Reservando Memoria ...");

				int aprobado = reservarEnMemoria(new->pid, cantPaginas);

				permisosDeMultiprogramacion = 1;
				if (!aprobado) {
					logInfo("(PLP) Rechazado");
					rechazarConsola(new->pid);
					logInfo("(PLP) Proceso Eliminado");
				} else {
					logInfo("(PLP) Insertando codigo en Memoria ... ");
					insertarEnMemoria(new->codigo, new->pid);
					logInfo("(PLP) Creando PCB ...");
					char* pcb = createPCB(new);
					logInfo("(PLP) Cargando en Ready el PCB ...");
					agregarAReady(pcb);
					logInfo("(PLP) Ok");
				}
			} else
				pthread_mutex_unlock(&mutex_multiprogramacion);
		}
		logInfo("(PLP)  Eliminando Residuos...");
		free(new->codigo);
		free(new);
		logInfo("(PLP) Ok");
	}
}

#endif /* SRC_OBJ_PLANIFICADORLARGOPLAZO_H_ */

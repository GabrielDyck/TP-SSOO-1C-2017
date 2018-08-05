#ifndef SRC_OBJ_PROCESO_H_
#define SRC_OBJ_PROCESO_H_

#include "../Kernel.h"

void inicializarProceso() {
	procesos = list_create();
}

struct_proceso* getProceso(int pid) {
	return (struct_proceso* )list_get(procesos, pid);
}

int newProceso(int socket) {
	int pid = list_size(procesos);
	struct_proceso* proceso = malloc(sizeof(struct_proceso));
	proceso->pid = pid;
	proceso->socket = socket;
	proceso->cantidadDeAlocamientos = 0;
	proceso->cantidadDeAlocamientosSize = 0;
	proceso->cantidadDeLiberaciones = 0;
	proceso->cantidadDeLiberacionesSize = 0;
	proceso->cantidadDeRafagas = 0;
	proceso->cantidadDeSysCalls = 0;
	proceso->cpu = 0;
	list_add(procesos, proceso);
	return pid;
}

void aumentarsysCalls(int cpu) {
	int i;
	for (i = 0; i < list_size(procesos); i++ ) {
		struct_proceso* proceso = list_get(procesos, i);
		if (proceso->cpu == cpu)
			proceso->cantidadDeSysCalls++;
	}
}

void eliminarCPUDelProceso(int cpu) {
	int i;
	for (i = 0; i < list_size(procesos); i++ ) {
		struct_proceso* proceso = list_get(procesos, i);
		if (proceso->cpu == cpu)
			proceso->cpu = 0;
	}
}
#endif

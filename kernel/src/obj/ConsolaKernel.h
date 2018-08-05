#ifndef SRC_OBJ_CONSOLAKERNEL_H_
#define SRC_OBJ_CONSOLAKERNEL_H_

#include "../Kernel.h"

#define LISTAR_PROCESOS 0
#define ARCHIVOS_ABIERTOS_PID 2
#define TABLA_ARCHIVOS 6
#define MODIFICAR_MULTIPROGRAMACION 7
#define FINALIZAR_PROCESO_KERNEL 8
#define DETENER_PLANIFICACION 9
#define ESTADO_PID 10
#define ACTIVAR_PLANIFICACION 11
#define CANTIDAD_RAFAGAS 12
#define CANTIDAD_DE_SYSCALLS 13
#define CANTIDAD_DE_ALOCACIONES 14
#define CANTIDAD_DE_LIBERACIONES 15
#define INICIAR_PLANIFICACION 16

void inicializarConsolaKernel() {
	pthread_create(&consolaKernel_h, NULL, (void*) crearConsolaKernel, NULL);
	pthread_mutex_init(&mutex_planificacion, NULL);
}

void cantidadDeRafagas(int pid) {
	printf("Cantidad de rafagas del pid %d son %d\n", pid,
			getProceso(pid)->cantidadDeRafagas);
}

void cantidadDeSysCalls(int pid) {
	printf("Cantidad de syscalls del pid %d son %d\n", pid,
			getProceso(pid)->cantidadDeSysCalls);
}

void cantidadDeAlocaciones(int pid) {
	printf("Cantidad de Alocamientos del pid %d son %d con %d bytes\n", pid,
			getProceso(pid)->cantidadDeAlocamientos,
			getProceso(pid)->cantidadDeAlocamientosSize);
}

void cantidadDeLiberaciones(int pid) {
	printf("Cantidad de frees del pid %d son %d con %d bytes\n", pid,
			getProceso(pid)->cantidadDeLiberaciones,
			getProceso(pid)->cantidadDeLiberacionesSize);
}

void mostrarVariablesGlobales() {
	int i;
	for (i = 0; i < list_size(variablesGlobales); i++) {
		struct_variable_global* variable = list_get(variablesGlobales, i);
		printf("%s:%d\n", variable->nombre, variable->valor);
	}
	for (i = 0; i < list_size(semaforosGlobales); i++) {
		struct_semaforo_global* variable = list_get(semaforosGlobales, i);
		printf("%s:%d\n", variable->nombre, variable->valor);
	}
}

int parsear(char * comando, int * value) {
	if (string_equals_ignore_case(comando, "obtenerTablaGlobal"))
		return TABLA_ARCHIVOS;
	if (string_equals_ignore_case(comando, "detenerPlanificacion"))
		return DETENER_PLANIFICACION;
	if (string_equals_ignore_case(comando, "iniciarPlanificacion"))
		return INICIAR_PLANIFICACION;
	if (string_equals_ignore_case(comando, "sudo 4215"))
		return -17;

	if (string_length(comando) == 16) {
		char * subComando = toSubString(comando, 0, 13);
		if (string_equals_ignore_case(subComando, "listarProcesos")) {
			char* charValue = toSubString(comando, 15, 15);
			*value = atoi(charValue);
			free(charValue);
			free(subComando);
			return LISTAR_PROCESOS;
		}
		free(subComando);
	}

	if (string_length(comando) > 27) {
		char* subComando = toSubString(comando, 0, 25);
		if (string_equals_ignore_case(subComando,
				"modificarMultiprogramacion")) {
			char* valor = toSubString(comando, 27, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return MODIFICAR_MULTIPROGRAMACION;
		}
		free(subComando);
	}

	if (string_length(comando) > 18) {
		char* subComando = toSubString(comando, 0, 16);
		if (string_equals_ignore_case(subComando, "cantidadDeRafagas")) {
			char* valor = toSubString(comando, 18, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return CANTIDAD_RAFAGAS;
		}
		free(subComando);
	}

	if (string_length(comando) > 17) {
		char* subComando = toSubString(comando, 0, 15);
		if (string_equals_ignore_case(subComando, "finalizarProceso")) {
			char* valor = toSubString(comando, 17, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return FINALIZAR_PROCESO_KERNEL;
		}
		free(subComando);
	}

	if (string_length(comando) > 12) {
		char* subComando = toSubString(comando, 0, 10);
		if (string_equals_ignore_case(subComando, "alocaciones")) {
			char* valor = toSubString(comando, 12, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return CANTIDAD_DE_ALOCACIONES;
		}
		free(subComando);
	}

	if (string_length(comando) > 9) {
		char* subComando = toSubString(comando, 0, 7);
		if (string_equals_ignore_case(subComando, "sysCalls")) {
			char* valor = toSubString(comando, 9, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return CANTIDAD_DE_SYSCALLS;
		}
		free(subComando);
	}

	if (string_length(comando) > 6) {
		char* subComando = toSubString(comando, 0, 4);
		if (string_equals_ignore_case(subComando, "frees")) {
			char* valor = toSubString(comando, 6, string_length(comando) - 1);
			*value = atoi(valor);
			free(valor);
			free(subComando);
			return CANTIDAD_DE_LIBERACIONES;
		}
		free(subComando);
	}
	return -1;
}

void detenerPlanificacion() {
	printf("Deteniendo la Planificacion!\n");
	pthread_mutex_lock(&mutex_planificacion);
}

void iniciarPlanificacion() {
	printf("Iniciando la Planificacion!\n");
	pthread_mutex_unlock(&mutex_planificacion);
}

void finalizarProceso(int pid) {
	printf("Finalizar Proceso\n");
	getProceso(pid)->exitCode = -20;
	abortarPid(pid);
}

void mostrarTablaGlobarDeArchivos() {
	printf("Mostrar tabla Global\n");
	pthread_mutex_lock(&mutex_tabla_global);
	int i;
	for (i = 0; i < list_size(tabla_global_archivos); i++) {
		struct_global_archivos* archivo = list_get(tabla_global_archivos, i);
		printf("Path: %s , Conexiones: %d\n", archivo->file, archivo->open);
	}
	pthread_mutex_unlock(&mutex_tabla_global);
}

void archivosAbiertos(int pid) {
	pthread_mutex_lock(&mutex_tabla_archivo_proceso);

	t_list * archivos = dictionary_get(tabla_archivos_procesos,
			string_itoa(pid));
	int i;
	for (i = 0; i < list_size(archivos); i++) {
		struct_archivo_proceso* descriptor = list_get(archivos, i);
		printf("Archivo abierto: %s\n", descriptor->fileDescriptorGlobal->file);
	}

	pthread_mutex_unlock(&mutex_tabla_archivo_proceso);

}
void crearConsolaKernel() {

	while (1) {
		char* comando = escucharComando();
		int valor;
		int codigo = parsear(comando, &valor);
		switch (codigo) {
		case LISTAR_PROCESOS:
			listarProcesos(valor);
			break;
		case CANTIDAD_DE_ALOCACIONES:
			cantidadDeAlocaciones(valor);
			break;
		case ARCHIVOS_ABIERTOS_PID:
			archivosAbiertos(valor);
			break;
		case CANTIDAD_DE_LIBERACIONES:
			cantidadDeLiberaciones(valor);
			break;
		case MODIFICAR_MULTIPROGRAMACION:
			modificarMultiprogramacion(valor);
			break;
		case FINALIZAR_PROCESO_KERNEL:
			finalizarProceso(valor);
			break;
		case DETENER_PLANIFICACION:
			detenerPlanificacion();
			break;
		case CANTIDAD_DE_SYSCALLS:
			cantidadDeSysCalls(valor);
			break;
		case CANTIDAD_RAFAGAS:
			cantidadDeRafagas(valor);
			break;
		case INICIAR_PLANIFICACION:
			iniciarPlanificacion();
			break;
		case TABLA_ARCHIVOS:
			mostrarTablaGlobarDeArchivos();
			break;
		case -17:
			mostrarVariablesGlobales();
			break;
		default:
			printf("Error: Comando no reconocido\n");
			break;
		}
		free(comando);
	}
}

void modificarMultiprogramacion(int valor) {
	printf("Modificar El Nivel de Multipragramacion de : %d, a %d\n",
			CONF_DATA.gradoMultiprog, valor);
	pthread_mutex_lock(&mutex_multiprogramacion);
	printf("Modificando ...\n");
	if (valor > CONF_DATA.gradoMultiprog) {
		int i;
		for (i = 0; i < (valor - CONF_DATA.gradoMultiprog); i++) {
			sem_post(&sem_multiprogramacion);
		}
	}
	CONF_DATA.gradoMultiprog = valor;
	printf("Ok\n");
	pthread_mutex_unlock(&mutex_multiprogramacion);
}

char* escucharComando() {
	char* comando = string_new();
	char* caracter = malloc(2 * sizeof(char));
	caracter[1] = '\0';
	do {
		caracter[0] = getchar();
		string_append(&comando, caracter);
	} while (caracter[0] != '\n');
	comando[string_length(comando) - 1] = '\0';
	free(caracter);
	return comando;
}

void listarProcesos(int valor) {
	switch (valor) {
	case (0):
		listarNews();
		listarEnEjecucion();
		listarEnReady();
		listarEnBloqueado();
		listarEnExits();
		break;
	case (1):
		listarNews();
		break;
	case (2):
		listarEnEjecucion();
		break;
	case (3):
		listarEnReady();
		break;
	case (4):
		listarEnBloqueado();
		break;
	case (5):
		listarEnExits();
		break;
	default:
		printf("Error se esperaba un valor de 0 - 5 y recibi : %d\n", valor);
		break;
	}

}

void listarEnBloqueado() {
	int index;
	int cantSem;
	printf("Lista de Bloquados\n");
	pthread_mutex_lock(&mutex_bloqueado);
	for (cantSem = 0; cantSem < list_size(semaforosGlobales); cantSem++) {
		struct_semaforo_global* semaforo = (struct_semaforo_global*) list_get(
				semaforosGlobales, cantSem);
		for (index = 0; index < queue_size(semaforo->cola); index++) {
			char* pcb = (char*) queue_pop(semaforo->cola);
			char *charId = toSubString(pcb, 0, 3);
			int id = atoi(charId);
			free(charId);
			printf("%d\n", id);
			queue_push(semaforo->cola, pcb);
		}
	}
	pthread_mutex_unlock(&mutex_bloqueado);
}

void listarNews() {
	int index;
	printf("Lista de News\n");
	pthread_mutex_lock(&mutex_news);
	for (index = 0; index < queue_size(news); index++) {
		struct_new* new = (struct_new*) queue_pop(news);
		printf("%d\n", new->pid);
		queue_push(news, new);
	}
	pthread_mutex_unlock(&mutex_news);
}

void listarEnEjecucion() {
	int id;
	int index;
	printf("En Ejecucion\n");
	pthread_mutex_lock(&mutex_exec);
	for (index = 0; index < list_size(exec); index++) {
		id = (int) list_get(exec, index);
		printf("%d\n", id);
	}
	pthread_mutex_unlock(&mutex_exec);
}

void listarEnReady() {
	int id;
	int index;
	printf("En Ready\n");
	pthread_mutex_lock(&mutex_ready);
	for (index = 0; index < queue_size(ready); index++) {
		char* pcbSerializado = (char*) queue_pop(ready);
		char* idSerializado = toSubString(pcbSerializado, 0, 3);
		id = atoi(idSerializado);
		free(idSerializado);
		printf("%d\n", id);
		queue_push(ready, pcbSerializado);
	}
	pthread_mutex_unlock(&mutex_ready);
}

void listarEnExits() {
	int index;
	printf("En Exit\n");
	pthread_mutex_lock(&mutex_exits);
	for (index = 0; index < queue_size(exits); index++) {
		struct_proceso* proceso = (struct_proceso*) queue_pop(exits);
		printf("%d con exit code: %d\n", proceso->pid, proceso->exitCode);
		queue_push(exits, proceso);
	}
	pthread_mutex_unlock(&mutex_exits);
}

#endif /* SRC_OBJ_CONSOLAKERNEL_H_ */

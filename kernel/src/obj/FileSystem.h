#ifndef SRC_OBJ_FILESYSTEM_H_
#define SRC_OBJ_FILESYSTEM_H_

#include "../Kernel.h"

#define VALIDAR_ARCHIVO 0
#define CREAR_ARCHIVO 1
#define BORRAR_ARCHIVO_FS 2
#define OBTENER_DATOS 3
#define GUARDAR_DATOS 4

// FLAGS - PERMISOS
#define LECTURA "L"
#define ESCRITURA "E"
#define CREACION "C"

// SUCCESS CODE
#define SUCCESS 1
#define ERROR 0

// Definicion de funciones
int crearEntradaEnLasTablas(char* path, char* mode, int pid);
int sendIntTo(int socket, int number);
int todoOKFs(int confirmacion);
char * flagsToMode(t_banderas banderas);
void eliminarDeLaTablaGlobal(struct_global_archivos* fileDescriptorGlobal);

void inicializarFileSystem() {
	tabla_global_archivos = list_create();
	tabla_archivos_procesos = dictionary_create();
	pthread_mutex_init(&mutex_tabla_global, NULL);
	pthread_mutex_init(&mutex_tabla_archivo_proceso, NULL);
}

int conectarAFileSystem() {
	logInfo("Conectando al File System....");

	fileSystem = conectar(CONF_DATA.puertoFs, CONF_DATA.ipFs);

	if (fileSystem < 0) {
		logError("(MAIN) Conexion a File System failed ");
		return 0;
	}
	logInfo("(MAIN) Conexion a File System ok ");
	return 1;
}

char* checkearPermisos(char * modo, char * flag) {
	log_info(archivoLog,"(MAIN) Permiso: %s .FLAG: %s",modo,flag );

	return string_contains(modo, flag);
}

int existeArchivo(char* path) {
	enviarMensajeConProtocolo(fileSystem, path, VALIDAR_ARCHIVO);

	return esperarConfirmacion(fileSystem);
}

int performEliminarArchivo(char * path) {
	enviarMensajeConProtocolo(fileSystem, path, BORRAR_ARCHIVO_FS);
	return esperarConfirmacion(fileSystem);
}

int abrirArchivo(char* path, t_banderas flags, int pid) {
	log_info(archivoLog, "(MAIN) Abriendo archivo : %s", path);

	char * mode = flagsToMode(flags);
	int fileDescriptor;
	if (existeArchivo(path)) {
		log_info(archivoLog, "(MAIN) Exite el archivo : %s", path);

		fileDescriptor = crearEntradaEnLasTablas(path, mode, pid);
	} else if (checkearPermisos(mode, CREACION)) {
		log_info(archivoLog, "(MAIN) El archivo  %s no existe pero tiene permisos de creacion", path);

		crearArchivo(path);
		fileDescriptor = crearEntradaEnLasTablas(path, mode, pid);
	} else {
		log_info(archivoLog, "(MAIN) El archivo  %s no existe y no tiene permisos de creacion", path);

		noSePuedoCrearElArhivo(pid);
		fileDescriptor = ERROR;
	}
	free(mode);
	return fileDescriptor;
}

struct_global_archivos* crearEntradaEnLaTablaGlobal(char* path) {
	int i;
	pthread_mutex_lock(&mutex_tabla_global);
	for (i = 0; i < list_size(tabla_global_archivos); i++) {
		struct_global_archivos* archivo = list_get(tabla_global_archivos, i);
		if (string_equals_ignore_case(archivo->file, path)) {
			archivo->open = archivo->open + 1;
			pthread_mutex_unlock(&mutex_tabla_global);
			return archivo;
		}
	}

	struct_global_archivos* archivo = malloc(sizeof(struct_global_archivos));
	archivo->file = string_duplicate(path);
	archivo->open = 1;

	list_add(tabla_global_archivos, archivo);
	pthread_mutex_unlock(&mutex_tabla_global);
	return archivo;
}

t_list* getProcesosDelArchivo(int pid) {
	char* pidChar = string_itoa(pid);
	t_list * archivos = dictionary_get(tabla_archivos_procesos, pidChar);

	if (archivos != NULL) {
		free(pidChar);
		return archivos;
	} else {
		t_list* newList = list_create();
		dictionary_put(tabla_archivos_procesos, pidChar, newList);
		return newList;
	}
}

int crearEntradaEnLaTablaPorProceso(int pid, char* mode,
		struct_global_archivos* fileDescriptorGlobal) {
	pthread_mutex_lock(&mutex_tabla_archivo_proceso);
	t_list* procesosDelArchivo = getProcesosDelArchivo(pid);
	int fileDescriptor = list_size(procesosDelArchivo) + 3;
	struct_archivo_proceso* archivo = malloc(sizeof(struct_archivo_proceso));
	archivo->flag = string_duplicate(mode);
	archivo->fileDescriptorGlobal = fileDescriptorGlobal;
	archivo->fileDescriptor = fileDescriptor;
	archivo->cursor = 0;
	list_add(procesosDelArchivo, archivo);
	pthread_mutex_unlock(&mutex_tabla_archivo_proceso);

	return fileDescriptor;
}

int crearEntradaEnLasTablas(char* path, char* mode, int pid) {
	struct_global_archivos* fileDescriptorGlobal = crearEntradaEnLaTablaGlobal(
			path);
	int fileDescriptorLocal = crearEntradaEnLaTablaPorProceso(pid, mode,
			fileDescriptorGlobal);
	return fileDescriptorLocal;
}

char * leerArchivo(int pid, int fileDescriptorProceso, int tamanio) {
	char* pidChar = string_itoa(pid);
	t_list * archivosDelProcesoActual = dictionary_get(tabla_archivos_procesos,	pidChar);
	free(pidChar);

	if (archivosDelProcesoActual != NULL) {
		int i;
		for (i = 0; i < list_size(archivosDelProcesoActual); i++) {
			struct_archivo_proceso * archivoProceso = list_get(archivosDelProcesoActual, i);
			if ((archivoProceso->fileDescriptor == fileDescriptorProceso) && (archivoProceso->fileDescriptorGlobal != NULL)){
				if (checkearPermisos(archivoProceso->flag, LECTURA)) {
					char *path = archivoProceso->fileDescriptorGlobal->file;

					char* mensajeReal = string_new();
					char* funcionId = toStringInt(OBTENER_DATOS);
					char* sizePath = toStringInt(string_length(path));
					char* offsetSent = toStringInt(archivoProceso->cursor);
					char* tamanioSent = toStringInt(tamanio);

					string_append(&mensajeReal, funcionId);
					string_append(&mensajeReal, sizePath);
					string_append(&mensajeReal, path);
					string_append(&mensajeReal, offsetSent);
					string_append(&mensajeReal, tamanioSent);
					log_debug(archivoLog, "(MAIN) Enviando a FS: %s", mensajeReal);
					send(fileSystem, mensajeReal, strlen(mensajeReal), 0);
					free(mensajeReal);
					free(funcionId);
					free(sizePath);
					free(offsetSent);
					free(tamanioSent);

					if(esperarConfirmacion(fileSystem)) {
						send(fileSystem, &fileSystem, sizeof(int), 0);
						void * buffer = malloc(tamanio);
						recv(fileSystem, buffer, tamanio, MSG_WAITALL);
						return buffer;
					}
					break;
				} else {
					leerSinPermisos(pid);
					return NULL;
				}
			}
		}
	}
	errorSinDefinicion(pid);
	return NULL;
}

void crearArchivo(char * path) {
	char* mensajeReal = string_new();

	uint32_t funcionId = CREAR_ARCHIVO;

	uint32_t sizePath = string_length(path);
	string_append(&mensajeReal, header(funcionId));
	string_append(&mensajeReal, header(sizePath));
	string_append(&mensajeReal, path);

	send(fileSystem, mensajeReal, strlen(mensajeReal), 0);
	recibirProtocolo(fileSystem);
}

int escribirArchivo(int pid, int fileDescriptorProceso, void* dato, int tamanio) {
	log_info(archivoLog, "(MAIN) Por escribir %s, para el pid %d y el fileDescriptor : %d",dato,pid,fileDescriptorProceso);

	char* pidChar = string_itoa(pid);
	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);

	if (archivoDelProcesoActual != NULL) {
		log_info(archivoLog, "(MAIN) Se encontro la lista de archivos par el pid %d",pid);

		int i;
		for (i = 0;	i < list_size(archivoDelProcesoActual);	i++) {
			struct_archivo_proceso * archivoProceso = list_get(archivoDelProcesoActual, i);

			if (archivoProceso->fileDescriptor == fileDescriptorProceso) {
				log_info(archivoLog, "(MAIN) Se encontro la tabla entrada para el fileDescriptor %d",fileDescriptorProceso);


				if (checkearPermisos(archivoProceso->flag,ESCRITURA) != NULL) {
					log_info(archivoLog, "(MAIN) Tiene permisos de escritura. Por escribir");
					struct_global_archivos * archivoGlobal = archivoProceso->fileDescriptorGlobal;
					if (archivoGlobal != NULL) {
						char * path = string_duplicate(archivoGlobal->file);

						char* mensajeReal = string_new();
						char* offsetSent = toStringInt(archivoProceso->cursor);

						char* sizeBuffer = toStringInt(tamanio);

						string_append(&mensajeReal, offsetSent);
						free(offsetSent);
						string_append(&mensajeReal, sizeBuffer);
						free(sizeBuffer);
						string_append(&mensajeReal, path);
						free(path);

						log_info(archivoLog, "(MAIN) Enviando a fileSystem  %s",mensajeReal);
						enviarMensajeConProtocolo(fileSystem, mensajeReal, GUARDAR_DATOS);
						free(mensajeReal);
						esperarConfirmacion(fileSystem);
						send(fileSystem, dato, tamanio, 0);
						int confirmacion = recibirProtocolo(fileSystem);
						log_debug(archivoLog, "File System Respondio: %d", confirmacion);
						if (confirmacion) {
							return confirmacion;
						}

					}
				} else {
					logError("(MAIN) No se tiene permisos para Escribir");

					escribirSinPermisos(pid);
					return ERROR;
				}
			}
		}
	}
	logError("(MAIN) No se pudo escirbir el archivo");
	errorSinDefinicion(pid);
	return ERROR;
}

void liberarFileSistem(int pid) {
	char* pidChar = string_itoa(pid);
	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);
	if (archivoDelProcesoActual != NULL) {
		int i;
		for (i = 0; i < list_size(archivoDelProcesoActual); i++) {
			struct_archivo_proceso * fileDescriptorBuscado = list_get(archivoDelProcesoActual, i);
			cerrarArchivo(pid, fileDescriptorBuscado->fileDescriptor);
		}
	}
}

int getCursor(int pid, int fileDescriptorProceso) {
	char* pidChar = string_itoa(pid);
	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);

	if (archivoDelProcesoActual != NULL) {
		int i;
		for (i = 0; i < list_size(archivoDelProcesoActual); i++) {
			struct_archivo_proceso * fileDescriptorBuscado = list_get(archivoDelProcesoActual, i);
			if (fileDescriptorBuscado->fileDescriptor == fileDescriptorProceso) {
				return fileDescriptorBuscado->cursor;
			}
		}
	}
	return -1;
}

int eliminarArchivo(int pid, int fileDescriptorProceso) {
	int indexArchivos;

	char* pidChar = string_itoa(pid);
	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);

	for (indexArchivos = 0; indexArchivos < list_size(archivoDelProcesoActual);
			indexArchivos++) {
		struct_archivo_proceso * fileDescriptorBuscado = list_get(
				archivoDelProcesoActual, indexArchivos);
		if (fileDescriptorBuscado->fileDescriptor == fileDescriptorProceso) {
			if (fileDescriptorBuscado->fileDescriptorGlobal->open == 1) {
				char* path = string_duplicate(
						fileDescriptorBuscado->fileDescriptorGlobal->file);
				eliminarDeLaTablaGlobal(
						fileDescriptorBuscado->fileDescriptorGlobal);

				int confirmacion = performEliminarArchivo(path);
				free(path);
				fileDescriptorBuscado->fileDescriptorGlobal = NULL;

				if (confirmacion >= 0)
					return SUCCESS;
				else {
					errorSinDefinicion(pid);
					return ERROR;
				}
			}
		}
	}
	errorSinDefinicion(pid);
	return ERROR;
}

struct_global_archivos* eliminarProcesoLocal(int pid, int fileDescriptor) {
	char* pidChar = string_itoa(pid);
	pthread_mutex_lock(&mutex_tabla_archivo_proceso);

	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);

	if (archivoDelProcesoActual != NULL) {
		int i;
		for (i = 0; i < list_size(archivoDelProcesoActual); i++) {
			struct_archivo_proceso* proceso = list_get(archivoDelProcesoActual,
					i);
			if (proceso->fileDescriptor == fileDescriptor) {
				list_remove(archivoDelProcesoActual, i);
				struct_global_archivos* fileDescriptorGlobal =
						proceso->fileDescriptorGlobal;
				free(proceso->flag);
				free(proceso);
				pthread_mutex_unlock(&mutex_tabla_archivo_proceso);

				return fileDescriptorGlobal;
			}
		}
		pthread_mutex_unlock(&mutex_tabla_archivo_proceso);

		return NULL;
	} else {
		pthread_mutex_unlock(&mutex_tabla_archivo_proceso);

		return NULL;

	}
}

void eliminarDeLaTablaGlobal(struct_global_archivos* fileDescriptorGlobal) {
	fileDescriptorGlobal->open = fileDescriptorGlobal->open - 1;
	if (fileDescriptorGlobal->open <= 0) {
		int i;
		pthread_mutex_lock(&mutex_tabla_global);
		for (i = 0; i < list_size(tabla_global_archivos); i++) {
			struct_global_archivos* archivo = list_get(tabla_global_archivos,
					i);
			if (archivo == fileDescriptorGlobal) {
				list_remove(tabla_global_archivos, i);
				free(archivo->file);
				free(archivo);
				break;
			}
		}
		pthread_mutex_unlock(&mutex_tabla_global);
	}
}

void cerrarArchivo(int pid, int fileDescriptorProceso) {
	struct_global_archivos* fileDescriptorGlobal = eliminarProcesoLocal(pid,
			fileDescriptorProceso);
	if (fileDescriptorGlobal != NULL)
		eliminarDeLaTablaGlobal(fileDescriptorGlobal);
}

void moverCursor(int pid, int fileDescriptorProceso, int posicion) {
	int i;
	char* pidChar = string_itoa(pid);
	t_list * archivoDelProcesoActual = dictionary_get(tabla_archivos_procesos,
			pidChar);
	free(pidChar);
	if (archivoDelProcesoActual != NULL) {
		for (i = 0; i < list_size(archivoDelProcesoActual); i++) {
			struct_archivo_proceso * archivoProceso = list_get(
					archivoDelProcesoActual, i);
			if (archivoProceso->fileDescriptor == fileDescriptorProceso) {
				archivoProceso->cursor = posicion;
			}
		}
	}
}

int recvString(char **str, int fd) {
	int32_t strLenght = recibirProtocolo(fd);
	if (strLenght < 0) {
		log_error(archivoLog, "Fallo al recibir largo de string");
		return -1;
	}
	*str = malloc(strLenght + 1);
	if (recv(fd, *str, strLenght, 0) < strLenght) {
		log_error(archivoLog, "Fallo al recibir string");
		return -1;
	}
	(*str)[strLenght] = '\0';
	return 0;
}

int todoOKFs(int confirmacion) {
	return confirmacion >= 0 ? SUCCESS : ERROR;
}

char * flagsToMode(t_banderas banderas) {

	char * mode = string_new();
	if (banderas.creacion) {
		string_append(&mode, CREACION);
	}
	if (banderas.lectura) {
		string_append(&mode, LECTURA);
	}
	if (banderas.escritura) {
		string_append(&mode, ESCRITURA);
	}
	return mode;
}
#endif

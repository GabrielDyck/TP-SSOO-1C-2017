#include "consola.h"

int main(int argc, char *argv[]) {
	config(argc, argv);

	init();

	return EXIT_SUCCESS;
}

void config(int argc, char *argv[]) {
	if (getcwd(CURRENT_WORK_DIRECTORY, 200) != NULL) {
		printf("[PATH] %s\n", CURRENT_WORK_DIRECTORY);
	} else {
		perror("getcwd() error");
	}

	initializeLogger();
	if (readConfigFile(argv[1])) {
		log_info(LOGGER, "Consola inicializada");
	} else {
		log_warning(LOGGER,
				"No se ha podido inicializar consola, ver logs anteriores");
	}

}

int readConfigFile(char *path) {
	t_config *configFile = config_create(path);
	int success = false;

	if (config_has_property(configFile, "PUERTO_KERNEL")) {
		CONF_DATA.puerto = config_get_int_value(configFile, "PUERTO_KERNEL");

		success = true;
	} else {
		log_warning(LOGGER, "No se ha encontrado la config de PUERTO_KERNEL");
	}

	if (config_has_property(configFile, "IP_KERNEL")) {
		CONF_DATA.ip_kernel = config_get_string_value(configFile, "IP_KERNEL");

		char *ip_kernel;
		ip_kernel = config_get_string_value(configFile, "IP_KERNEL");

		size_t size_ip_kernel;
		size_ip_kernel = strlen(ip_kernel);

		CONF_DATA.ip_kernel = malloc(size_ip_kernel + 1);
		memcpy(CONF_DATA.ip_kernel, ip_kernel, size_ip_kernel);
		CONF_DATA.ip_kernel[size_ip_kernel] = '\0';
		success = true;
	} else {
		log_warning(LOGGER, "No se ha encontrado la config de IP_KERNEL");
		success = false;
	}

	config_destroy(configFile);
	log_info(LOGGER, "Configuracion leida: PUERTO_KERNEL %d, IP_CONFIG %s",
			CONF_DATA.puerto, CONF_DATA.ip_kernel);
	return success;
}

int init() {
	int commandToExecute;
	int args = 0;
	openSockets = list_create();

	char line[MAX_LINE_SIZE];
	char **input;
	char *parameters;

	while (1) {
		int len = 0;

		if ((len = get_line(line, MAX_LINE_SIZE)) > 0) {

			input = string_split(line, " ");
			args = countWords(line);
			commandToExecute = commandParser(input, &args);
			parameters = input[2] ? string_duplicate(input[2]) : NULL;
			free(input[0]);
			free(input[1]);
			free(input[2]);
			free(input);

			if (commandToExecute >= 0) {
				switch (commandToExecute) {
				case INICIAR_PROGRAMA:
					iniciarPrograma(parameters);
					break;
				case FINALIZAR_PROGRAMA:
					finalizarPrograma(parameters);
					break;
				case LIMPIAR_MENSAJES:
					limpiarMensajes();
					break;
				case DESCONECTAR_CONSOLA:
					desconectarConsola();
					break;
				case LISTAR:
					listar();
					break;
				default:
					log_error(LOGGER, "Comando no reconocido");
				}
				free(parameters);
			}
		}
	}
}

void listar() {
	printf("---- PIDS ACTIVOS EN ESTA CONSOLA ----\n");
	void print(new_process * socketFromList) {
		if (socketFromList->pid >= 0) {
			printf("PID #%d \n", socketFromList->pid);
		}
	}

	list_iterate(openSockets, (void*)print);
}

void desconectarConsola() {
	log_info(LOGGER, "---- CERRANDO PROCESOS ----");

	void disconnect(new_process * socketFromList) {
		if (socketFromList->pid >= 0 && socketFromList->pid <= 9999) {
			log_info(LOGGER, "[#%d] Pid desconectado", socketFromList->pid);
			enviarMensajeConProtocolo(socketFromList->socketLocal, header(socketFromList->pid), DESCONECTAR_CONSOLA);
			//eliminarPid(socketFromList);
		}
	}

	list_iterate(openSockets, (void*)disconnect);
	log_info(LOGGER, "---- SOPRANO DESCONECTADO ----");
}

int commandParser(char **parameters, int *args) {
	int i = -1;

	if (strcmp(APP_NAME, parameters[APP]) == 0) {
		int result = -1;
		(*args)--; // La palabra clave soprano
		for (i = 0; i < TOTAL_COMMANDS; i++) {
			if (strcmp(parameters[1], acceptedCommands[i]) == 0) {
				result = 1;
				(*args)--;

				return i;
			}
		}
		if (result < 0) {
			log_error(LOGGER, "Comando no reconocido");
			return -1;
		}
	} else {
		log_error(LOGGER, "Los comandos deben iniciar con \"soprano\"");
		return -1;
	}
	return 0;
}

char * fileToString(char *path) {
	FILE* fd = fopen(path, "r");
	char * script;
	long length;
	if (fd != NULL) {
		fseek(fd, 0, SEEK_END);
		length = ftell(fd);
		fseek(fd, 0, SEEK_SET);
		script = malloc((length+1) * sizeof(char));
		fread(script, sizeof(char), length, fd);
		fclose(fd);
		script[length] = '\0';
		return script;
	} else {
		log_error(LOGGER, "No se encontro el archivo");
		return NULL;
	}
}

void limpiarMensajes() {
	int i;
	for (i = 0; i < 10; i++)
		printf("\n\n\n\n\n");
}

void iniciarPrograma(char *argument) {
	char *ansisopFilePath = string_new();
	string_append(&ansisopFilePath, CURRENT_WORK_DIRECTORY);
	string_append(&ansisopFilePath, "/script/");
	string_append(&ansisopFilePath, argument);

	new_process *processArgv = malloc(sizeof(new_process));

	log_info(LOGGER, "Enviando info de: %s", ansisopFilePath);
	char * script = fileToString(ansisopFilePath);
	free(ansisopFilePath);
	if (script != NULL) {
		processArgv->socketLocal = conectarKernelConHandshake();

		list_add(openSockets, processArgv);

		enviarMensajeConProtocolo(processArgv->socketLocal, script,	INICIAR_PROGRAMA);
		guardarTiempo(processArgv);
		free(script);

		pthread_create(&processArgv->hilo, NULL, (void*) processThread,
				processArgv);
	}
}


void guardarTiempo(new_process *process) {
	process->horaInicio = malloc(sizeof(struct tm));


	time_t log_time;
	if ((log_time = time(NULL)) == -1) {
		error_show("Error getting date!");
		return;
	}

	localtime_r(&log_time, process->horaInicio );
	ftime( &(process->msInicio) );

	return;
}

void processThread(new_process * processData) {
	processData->impresiones = 0;
	char* pidChar = esperarMensaje(processData->socketLocal);
	processData->pid = atoi(pidChar);
	free(pidChar);
	if (processData->pid < 0) {
		log_error(LOGGER, "Pid recibido invalido");
		log_error(LOGGER, "El kernel ha rechazado la ejecucion del proceso");
		return;
	}

	printf("[#%d]: El kernel ha aceptado el proceso, PID #%d \n",
			processData->pid, processData->pid);

	bool finalizado = false;
	while (!finalizado) {
		if(!processData) {
			return;
		}
		int protocolo = recibirProtocolo(processData->socketLocal);

		char* mensaje;

		switch (protocolo) {

		case PID_EXIT_SUCCESS:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID finalizado exitosamente, se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID finalizado exitosamente, no se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}

			finalizado = true;
			break;

		case PID_EXPULSADO:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, no se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}

			finalizado = true;
			break;

		case PID_EXIT_RECHAZADO:
			printf("[#%d]: PID rechazado por el kernel \n",
					processData->pid);
			finalizado = true;
			break;

		case PID_MESSAGE:
			mensaje = esperarMensaje(processData->socketLocal);
			printf("[#%d]: %s \n", processData->pid, mensaje);
			processData->impresiones++;
			enviarMensaje(processData->socketLocal, "1");
			free(mensaje);
			break;

		case PID_EXIT_ALOCAR_MAS_DE_UNA_PAGINA:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, se intento alocar mas de una pag. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, se intento alocar mas de una pag. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			finalizado = true;
			free(mensaje);
			break;

		case PID_EXIT_ARCHIVO_NO_EXISTENTE:

			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, archivo no existente. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, archivo no existente. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			finalizado = true;
			free(mensaje);
			break;

		case PID_EXIT_ERROR_DE_MEMORIA:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado. Error de Memoria. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado. Error de Memoria. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			free(mensaje);
			finalizado = true;
			break;

		case PID_EXIT_ERROR_SIN_DEFINICION:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, error no definido. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, error no definido. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			free(mensaje);
			finalizado = true;
			break;

		case PID_EXIT_LEER_ARCHIVO_SIN_PERMISOS:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, se intento leer archivo sin permisos. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, se intento leer archivo sin permisos. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			free(mensaje);
			finalizado = true;
			break;

		case PID_EXIT_NO_HAY_MAS_PAGINAS:
			mensaje = esperarMensaje(processData->socketLocal);
			if (atoi(mensaje) > 0) {
				printf("[#%d]: PID expulsado, no hay mas paginas disponibles para alocar. Se libero todo lo alocado \n",
						processData->pid);
			} else {
				printf("[#%d]: PID expulsado, no hay mas paginas disponibles para alocar. No se pudo liberar toda la memoria alocada \n",
						processData->pid);
			}
			free(mensaje);
			finalizado = true;
			break;
		}
	}
	eliminarPid(processData);
}

void eliminarPid(new_process * process) {
	int i;
	for (i = 0; i < list_size(openSockets); i++) {
		new_process * desiredProcess = list_get(openSockets, i);
		if (desiredProcess == process) {
			printf("[%d] Total de impresiones del proceso: %d \n", process->pid, process->impresiones);
			printf("[%d] Hora de inicio del proceso: %s \n", process->pid, get_hora_inicio(process));
			printf("[%d] Hora de finalizacion del proceso: %s \n", process->pid, temporal_get_string_time());
			printf("[%d] Tiempo del proceso en ejecucion: %s \n", process->pid, get_time_diff(process));
			list_remove(openSockets, i);
			break;
		}
	}

	close(process->socketLocal);
	free(process);
}

int recvInt(int socket) {
	int32_t number;
	if (recv(socket, &number, sizeof(number), 0) < 0) {
		log_error(LOGGER, "Fallo al recibir int");
		return -1;
	}
	return number;
}

void finalizarPrograma(char *argument) {
	printf("Enviando solicitud de finalizacion \n");
	if (argument == NULL) {
		log_warning(LOGGER, "No se ha especificado un pid para finalizar");
		return;
	}

	int pidFromInput = atoi(argument);

	int i;
	for (i = 0; i < list_size(openSockets); i++) {
		new_process * processToEnd = list_get(openSockets, i);
		if (processToEnd->pid == pidFromInput) {
			printf("Finalizando pid: %d \n", processToEnd->pid);
			char* charPid = header(processToEnd->pid);
			enviarMensajeConProtocolo(processToEnd->socketLocal, charPid, FINALIZAR_PROGRAMA);
			free(charPid);
			return;
		}
	}

	log_warning(LOGGER, "No se encuentra en ejecucion el pid: %d",
			pidFromInput);
}

unsigned countWords(char *str) {
	int state = 1;
	unsigned wc = 0;  // word count
	int out = 1, in = 0;

	while (*str) {
		if (*str == ' ' || *str == 'n' || *str == 't')
			state = out;

		else if (state == out) {
			state = in;
			++wc;
		}

		++str;
	}

	return wc;
}

int conectarKernelConHandshake() {
	int socketKernel;
	socketKernel = conectar(CONF_DATA.puerto, CONF_DATA.ip_kernel);
	socketKernel > 0 ?
			log_info(LOGGER, "Socket cliente creado en puerto %d",
					CONF_DATA.puerto) :
			NULL;

	uint32_t code;
	code = ID_CONSOLA;

	int sendStatus = send(socketKernel, &code, sizeof(code), 0); // Handshake

	sendStatus > 0 ?
			log_info(LOGGER, "Handshake request realizada con %s",
					CONF_DATA.ip_kernel) :
			NULL;

	recv(socketKernel, &code, sizeof(code), 0);
	sendStatus > 0 ?
			log_info(LOGGER,
					"Handshake realizado con exito, conexion establecida con kernel") :
			NULL;

	return socketKernel;
}

void initializeLogger() {
	LOGGER = log_create(NULL, "Consola", true, LOG_LEVEL_TRACE);
	log_info(LOGGER, "Logger inicializado");
}

int get_line(char s[], int lim) {
	int c, i, l;
	for (i = 0, l = 0; (c = getchar()) != EOF && c != '\n'; ++i)
		if (i < lim - 1)
			s[l++] = c;
	s[l] = '\0';
	return l;
}


char *get_time_diff(new_process * process) {
	struct tm *diff_tm = malloc(sizeof(struct tm));
	char *str_time = string_duplicate("hh:mm:ss:mmmm");
	struct timeb tmili;
	time_t log_time;
	if ((log_time = time(NULL)) == -1) {
		error_show("Error getting date!");
		return 0;
	}

	localtime_r(&log_time, diff_tm);
	if (ftime(&tmili)) {
		error_show("Error getting time!");
		return 0;
	}

	diff_tm->tm_hour -= abs(process->horaInicio->tm_hour);
	diff_tm->tm_min -= abs(process->horaInicio->tm_min);
	diff_tm->tm_sec -= abs(process->horaInicio->tm_sec);
	char *partial_time = string_duplicate("hh:mm:ss");
	strftime(partial_time, 127, "%H:%M:%S", diff_tm);
	sprintf(str_time, "%s:%hu", partial_time, abs(tmili.millitm - process->msInicio.millitm) );
	free(partial_time);
	free(diff_tm);
	//Adjust memory allocation
	str_time = realloc(str_time, strlen(str_time) + 1);
	return str_time;
}

char *get_hora_inicio(new_process * process) {
	char *str_time = string_duplicate("hh:mm:ss:mmmm");
	struct timeb tmili;
	time_t log_time;
	if ((log_time = time(NULL)) == -1) {
		error_show("Error getting date!");
		return 0;
	}

	char *partial_time = string_duplicate("hh:mm:ss");
	strftime(partial_time, 127, "%H:%M:%S", process->horaInicio);
	sprintf(str_time, "%s:%hu", partial_time, abs(process->msInicio.millitm) );
	free(partial_time);
	//Adjust memory allocation
	str_time = realloc(str_time, strlen(str_time) + 1);
	return str_time;
}



#include "Memoria.h"

int main() {

	if (config() < 0) return -1;
	
	pthread_t serverThreadId;
	pthread_create(&serverThreadId, NULL, (void*) &initServer, NULL);

	initConsole();
	
	finalize();
	return EXIT_SUCCESS;
}

int config() {
	if (configLog() < 0) return -1;

	if (readConfigFile() < 0) return -1;

	sem_init(&SEM_PAGES_TABLE, 0, 1);
	sem_init(&SEM_CACHE, 0, 1);
	MEMORY_BLOCK = malloc(CONF_DATA.marcos * CONF_DATA.marcoSize);

	ADMIN_PAGES_COUNT = configPagesTable(MEMORY_BLOCK);
	log_info(LOGGER, "Cantidad de paginas administrativas: %d", ADMIN_PAGES_COUNT);
	
	MEMORY_SIZE = (CONF_DATA.marcos - ADMIN_PAGES_COUNT) * CONF_DATA.marcoSize;
	
	configCache();
	log_info(LOGGER, "Memoria configurada");
	
	return 0;
}

int configLog() {
	char cwd[1000];
	if (getcwd(cwd, 1000) != NULL) {
		memcpy(cwd+strlen(cwd), "/log/log\0", 9);
		printf("Log file path %s\n", cwd);
	}else{
		perror("getcwd() error");
		return -1;
	}
	LOGGER = log_create(cwd, "Memoria", false, LOG_LEVEL_TRACE);
	log_info(LOGGER, "Logger inicializado");
	return 0;
}

int readConfigFile() {
	char cwd[1000];
	if (getcwd(cwd, 1000) != NULL) {
		memcpy(cwd+strlen(cwd), "/conf/conf.properties\0", 22);
		log_info(LOGGER, "Properties file path %s", cwd);
	}else{
		perror("getcwd() error");
		return -1;
	}

	t_config *configFile = config_create(cwd);

	if (config_has_property(configFile, "PUERTO")) {
		char *puerto = config_get_string_value(configFile, "PUERTO");

		size_t sizePuerto = strlen(puerto);

		CONF_DATA.puerto = malloc(sizePuerto + 1);
		memcpy(CONF_DATA.puerto, puerto, sizePuerto);
		CONF_DATA.puerto[sizePuerto] = '\0';
	}
	if (config_has_property(configFile, "MARCOS")) {
		CONF_DATA.marcos = config_get_int_value(configFile, "MARCOS");
	}
	if (config_has_property(configFile, "MARCO_SIZE")) {
		CONF_DATA.marcoSize = config_get_int_value(configFile, "MARCO_SIZE");
	}
	if (config_has_property(configFile, "ENTRADAS_CACHE")) {
		CONF_DATA.entradasCache = config_get_int_value(configFile, "ENTRADAS_CACHE");
	}
	if (config_has_property(configFile, "CACHE_X_PROC")) {
		CONF_DATA.cacheXProc = config_get_int_value(configFile, "CACHE_X_PROC");
	}
	if (config_has_property(configFile, "REEMPLAZO_CACHE")) {
		char *algoritmo = config_get_string_value(configFile, "REEMPLAZO_CACHE");

		size_t sizeAlgoritmo = strlen(algoritmo);

		CONF_DATA.reemplazoCache = malloc(sizeAlgoritmo + 1);
		memcpy(CONF_DATA.reemplazoCache, algoritmo, sizeAlgoritmo);
		CONF_DATA.reemplazoCache[sizeAlgoritmo] = '\0';
	}
	if (config_has_property(configFile, "RETARDO_MEMORIA")) {
		CONF_DATA.retardoMemoria = config_get_int_value(configFile, "RETARDO_MEMORIA");
	}
	config_destroy(configFile);
	log_info(LOGGER, "Configuracion leida: PUERTO %s, MARCOS %d, MARCO_SIZE %d, ENTRADAS_CACHE %d, CACHE_X_PROC %d, REEMPLAZO_CACHE %s, RETARDO_MEMORIA %d",
			CONF_DATA.puerto, CONF_DATA.marcos, CONF_DATA.marcoSize, CONF_DATA.entradasCache,
			CONF_DATA.cacheXProc, CONF_DATA.reemplazoCache, CONF_DATA.retardoMemoria);
	return 0;
}

int configPagesTable(void *memory) {
	PAGES_TABLE = memory; 

	int size = CONF_DATA.marcos * sizeof(t_pages_table_row); 
	
	int pages_count = (size % CONF_DATA.marcoSize) == 0 ? size / CONF_DATA.marcoSize : (size / CONF_DATA.marcoSize) + 1;
	
	int i;
	for (i = 0; i < pages_count; i++) {
		PAGES_TABLE[i].pid = -2;
		PAGES_TABLE[i].page = -2;
	}
	for (; i < CONF_DATA.marcos; i++) {
		eraseFrame(i);
	}
	
	return pages_count;
}

int initServer() {
	SERVER_FD = crearServidor(atoi(CONF_DATA.puerto));
	if (SERVER_FD < 0) {
		log_error(LOGGER, "No se pudo crear el socket servidor");
		return -1;
	}
	log_info(LOGGER, "Socket servidor creado en puerto %s", CONF_DATA.puerto);

	fd_set readFds;

	int conexiones = 1;
	while (1) {
		FD_ZERO(&readFds);
		FD_SET(SERVER_FD, &readFds);

		int selectState = select(SERVER_FD + conexiones, &readFds, NULL, NULL, NULL);

		if (selectState != -1 && FD_ISSET(SERVER_FD, &readFds)) {
			conexiones++;
			int clientSocket = conectarClienteDesdeServidor(SERVER_FD);

			int clientType = getClientType(clientSocket);

			if (clientType == CLIENT_TYPE_KERNEL) {
				if (IS_KERNEL_CONNECTED == 0) {
					IS_KERNEL_CONNECTED = 1;
					log_info(LOGGER, "Conexion recibida de un Kernel.");
					pthread_t kernelThreadId;
					pthread_create(&kernelThreadId, NULL, (void*) &kernelManager, &clientSocket);					
				} else {
					log_warning(LOGGER, "El kernel ya esta conectado.");
					sendIntTo(clientSocket, -1);
				}
			} else if (clientType == CLIENT_TYPE_CPU) {
				log_info(LOGGER, "Conexion recibida de un CPU.");
				pthread_t cpuThreadId;
				pthread_create(&cpuThreadId, NULL, (void*) &cpuManager, &clientSocket);
			} else if (clientType < 0) {
				log_error(LOGGER, "No se pudo recibir el tipo de cliente");
			}
			sleep(1);
		}
	}
}

int getClientType(int socket) {
	int code;
	int recvLenght = recv(socket, &code, sizeof(int), MSG_WAITALL);
	if (recvLenght <= 0) { return -1; }

	/*
	 * code=1 para KERNEL
	 * code=2 para CPU
	 */
	return code;
}

void finalize() {
	log_destroy(LOGGER);
	free(CONF_DATA.reemplazoCache);
	free(CONF_DATA.puerto);
	free(MEMORY_BLOCK);
	free(CACHE);
	free(CACHE_TABLE);
	sem_destroy(&SEM_PAGES_TABLE);
	sem_destroy(&SEM_CACHE);
}

void kernelManager(int *s) {
	int fdKernel;
	memcpy(&fdKernel, s, sizeof(fdKernel));
	log_info(LOGGER, "KERNEL conectado en fd %d", fdKernel);

	if (sendPageSize(fdKernel) < 0) return;

	performKernelFunctions(fdKernel);
	
	IS_KERNEL_CONNECTED = 0;

	log_info(LOGGER, "KERNEL desconectado de fd %d", fdKernel);
}

void performKernelFunctions(int socketKernel) {
	int status = -1;
	while(1) {
		int functionId = recvInt(socketKernel);
		switch(functionId) {
			case FUNC_INIT_PROGRAM:
				status = performInitProgramFunction(socketKernel);
				break;
			case FUNC_GET_BYTES:
				status = performGetBytesFunction(socketKernel);
				break;
			case FUNC_SAVE_BYTES:
				status = performSaveBytesFunction(socketKernel);
				break;
			case FUNC_ASSIGN_PAGES:
				status = performAssignPagesToProgramFunction(socketKernel);
				break;
			case FUNC_FINISH:
				status = performFinishProgramFunction(socketKernel);
				break;
			case FUNC_DELETE_PAGE:
				status = performDeletePageFunction(socketKernel);
				break;
			default:
				log_error(LOGGER, "Codigo %d no machea con nunguna funcion de Kernel", functionId);
				return;
		}
		
		if (status < 0) return;
	}
}

int sendPageSize(int socket) {
	int result = sendIntTo(socket, CONF_DATA.marcoSize);
	if(result < 0) log_error(LOGGER, "Fallo al enviar tamanio de paginas al Kernel");
	log_info(LOGGER, "Enviado tamanio de paginas %d", CONF_DATA.marcoSize);
	return result;
}

int recvInt(int socket) {
	int32_t number;
	if (recv(socket, &number, sizeof(number), 0) < 0) {
		log_error(LOGGER,"Fallo al recibir int");
		return -1;
	}
	return number;
}

void cpuManager(int *s) {
	int fd;
	memcpy(&fd, s, sizeof(fd));
	log_info(LOGGER, "CPU conectado en fd %d", fd);
	
	int functionId;
	int status = -1;
	while (1) {
		functionId = recvFunctionIdFromCpu(fd);
		
		switch (functionId) {
			case FUNC_GET_BYTES:
				status = performGetBytesFunction(fd);
				break;
			case FUNC_SAVE_BYTES:
				status = performSaveBytesFunction(fd);
				break;
			default:
				log_error(LOGGER, "Condigo %d no machea con ninguan funcion de CPU", functionId);
				return;
		}
		
		if (status < 0) return;
	}
	log_warning(LOGGER, "CPU conectado en fd %d desconectado", fd);
}

int recvFunctionIdFromCpu(int fd) {
	int functionId;
	
	int bytesCount = recv(fd, &functionId, sizeof(int), MSG_WAITALL);
	if (bytesCount <= 0) {
		log_error(LOGGER, "No se pudo recibir codigo de funcion de CPU fd %d", fd);
		return -1;
	}
	
	return functionId;
}

int performInitProgramFunction(int fd) {
	log_info(LOGGER, "Atendiendo pedido de iniciar programa");
	int pid = recvInt(fd);
	if (pid < 0) return -1;
	int pagesCount = recvInt(fd);
	if (pagesCount < 0) return -1;
	
	int operationStatus = initProgram(pid, pagesCount) < 0 ? 0 : 1;
	
	return sendIntTo(fd, operationStatus);
}

int performAssignPagesToProgramFunction(int fd) {
	log_info(LOGGER, "Atendiendo pedido de asignar paginas");

	int pid = recvInt(fd);
	if (pid < 0) return -1;
	int pagesCount = recvInt(fd);
	if (pagesCount < 0) return -1;
	
	int firstPageNumber = assignPagesToProgram(pid, pagesCount);
	
	return sendIntTo(fd, firstPageNumber < 0 ? 0 : firstPageNumber);
}

int performFinishProgramFunction(int fd) {
	log_info(LOGGER, "Atendiendo pedido de finalizar programa");

	int pid = recvInt(fd);
	if (pid < 0) return -1;
	
	int operationStatus = finishProgram(pid);

	return sendIntTo(fd, operationStatus);
}

int performDeletePageFunction(int fd) {
	log_info(LOGGER, "Atendiendo pedido de borrar pagina");

	int pid = recvInt(fd);
	if (pid < 0) return -1;
	int page = recvInt(fd);
	if (page < 0) return -1;

	int operationStatus = deletePage(pid, page);

	return sendIntTo(fd, operationStatus);
}

int performGetBytesFunction(int fd) {
	log_info(LOGGER, "Atendiendo pedido de obtener bytes");

	t_function_get_bytes_params params;
	
	if (recvGetBytesFunctionParams(&params, fd) < 0) return -1;
	
	if (isNotValidSize(params.size)) {
		log_error(LOGGER, "Size invalido %d (>0)", params.size);
		return sendIntTo(fd, 0);
	}
	
	void* buffer = malloc(params.size);		
	
	if (getBytesOfPage(buffer, params) < 0) {
		free(buffer);
		log_error(LOGGER, "No se pudieron leer las paginas");
		return sendIntTo(fd, 0);
	}
	
	int sendStatus = sendBytes(fd, params.size, buffer);
	
	free(buffer);
	
	if (sendStatus < 0) {
		log_error(LOGGER, "No se pudieron enviar las paginas");
		return -1;
	}
	
	return 0;
}

int performSaveBytesFunction(int socket) {
	log_info(LOGGER, "Atendiendo pedido de guardar bytes");

	t_function_save_bytes_params params;

	if (recvSaveBytesFunctionParams(&params, socket) < 0) return -1;

	int operationStatus;
	if (saveBytesInPage(params) < 0) {
		operationStatus = STATUS_ERROR;
	} else {
		operationStatus = STATUS_OK;
	}


	free(params.buffer);

	return sendIntTo(socket, operationStatus);
}

int recvGetBytesFunctionParams(t_function_get_bytes_params *params, int fd) {
	
	recv(fd, &(params->pid), sizeof(int), MSG_WAITALL);
	recv(fd, &(params->page), sizeof(int), MSG_WAITALL);
	recv(fd, &(params->offset), sizeof(int), MSG_WAITALL);
	recv(fd, &(params->size), sizeof(int), MSG_WAITALL);
	
	log_info(LOGGER, "Parametros recibidos para obtener bytes: pid %d, page %d, offset %d, size %d.",
			params->pid, params->page, params->offset, params->size);
	return 0;
}

int sendBytes(int socket, int countBytes, void *bytes) {
	int32_t size = countBytes;
	
	size_t bufferSize = size + sizeof(size);
	
	void *buffer = malloc(bufferSize);
	
	memcpy(buffer, &size, sizeof(size));
	
	memcpy(buffer + sizeof(size), bytes, size);
	
	int sendStatus = send(socket, buffer, bufferSize, 0);
	
	free(buffer);
	
	if (sendStatus != bufferSize) {
		return -1;
	}
	
	return sendStatus;
}

int recvSaveBytesFunctionParams(t_function_save_bytes_params *params, int socket) {
	size_t bufferSize = sizeof(int32_t) * 4;
	
	void *buffer = malloc(bufferSize);
	
	int bytesCount = recv(socket, buffer, bufferSize, 0);
	if (bytesCount <= 0) {
		free(buffer);
		return -1;
	}
	
	size_t sizeToCopy = sizeof(int32_t);
	
	memcpy(&(params->pid), buffer, sizeToCopy);
	
	int offsetToCopy = sizeToCopy;
	memcpy(&(params->page), buffer + offsetToCopy, sizeToCopy);
	
	offsetToCopy += sizeToCopy;
	memcpy(&(params->offset), buffer + offsetToCopy, sizeToCopy);
	
	offsetToCopy += sizeToCopy;
	memcpy(&(params->size), buffer + offsetToCopy, sizeToCopy);
	
	free(buffer);
	
	if (params->size <= 0) {
		return -1;
	}
	
	params->buffer = malloc(params->size);
	
	bytesCount = recv(socket, params->buffer, params->size, 0);
	if (bytesCount <= 0) {
		free(params->buffer);
		return -1;
	}
	
	log_info(LOGGER, "Parametros recibidos para guardar bytes: pid %d, page %d, offset %d, size %d.",
			params->pid, params->page, params->offset, params->size);

	return 0;
}


int sendIntTo(int socket, int number) {
	int32_t n = number;
	return send(socket, &n, sizeof(n), 0);
}


int crearSocketEscucha(char *puerto, int backlog) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, puerto, &hints, &serverInfo);

	int socketEscucha = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int estadoBind = bind(socketEscucha, serverInfo->ai_addr, serverInfo->ai_addrlen);
	if (estadoBind < 0) {	return -1; }//tratar de otra forma

	freeaddrinfo(serverInfo);

	int estadoListen = listen(socketEscucha, backlog);
	if (estadoListen < 0) { return -1; }//tratar de otra forma

	return socketEscucha;
}

int levantarServidorYConectarCliente(char* puerto, int backlog) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(NULL, puerto, &hints, &serverInfo);

	int socketEscucha = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int estadoBind = bind(socketEscucha,serverInfo->ai_addr, serverInfo->ai_addrlen);
	if (estadoBind < 0) {	return estadoBind; }//tratar de otra forma

	freeaddrinfo(serverInfo);

	int estadoListen = listen(socketEscucha, backlog);
	if (estadoListen < 0) { return estadoListen; }//tratar de otra forma

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socket = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
	close(socketEscucha);

	return socket;
}

int conectarseAServidor(char* ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &serverInfo);

	int socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

	int estadoConnect = connect(socketServidor, serverInfo->ai_addr, serverInfo->ai_addrlen);
	if (estadoConnect < 0) { return estadoConnect; }

	freeaddrinfo(serverInfo);

	return socketServidor;
}

int conectarClienteDesdeServidor(int socketEscucha) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);

	return socketCliente;
}

/*
 * MEMORY OPERATIONS
 * INTERFACE
 */

int initProgram(int pid, int requiredPagesCount) {
	log_info(LOGGER, "Iniciando programa pid: %d, paginas: %d", pid, requiredPagesCount);

	if (isNotValidPid(pid) || isNotValidPageCount(requiredPagesCount)) {
		log_error(LOGGER, "Parametros invalidos para iniciar PID %d, paginas requeridas %d.", pid, requiredPagesCount);
		return -1;
	}
	
	sem_wait(&SEM_PAGES_TABLE);
	
	if (isPresentPid(pid)) {
		log_error(LOGGER, "PID %d ya existe. No se pudo iniciar programa.", pid);
		sem_post(&SEM_PAGES_TABLE);
		return -2;
	}

	int frame = getEmptyFrame(pid, 0);

	if (isNotValidFrame(frame)) {
		log_error(LOGGER, "No hay paginas disponibles para iniciar PID %d.", pid);
		sem_post(&SEM_PAGES_TABLE);
		return -3;
	}

	int i;
	for (i = 0; i < requiredPagesCount; i++) {
		frame = getNextEmptyFrame(frame);
		if (frame < 0) {
			log_error(LOGGER, "No hay suficientes paginas disponibles para iniciar PID %d.", pid);
			eraseFramesOf(pid);
			sem_post(&SEM_PAGES_TABLE);
			return -2;
		} else {
			PAGES_TABLE[frame].pid = pid;
			PAGES_TABLE[frame].page = i;
		}
		frame++;
	}
	
	sem_post(&SEM_PAGES_TABLE);
	log_info(LOGGER, "PID %d iniciado.", pid);
	return 0;
}

int getBytesOfPage(void *buffer, t_function_get_bytes_params params) {
	log_info(LOGGER, "Obteniendo bytes pid: %d, page: %d, offset: %d, size: %d", params.pid, params.page, params.offset, params.size);

	if (isNotValidGetBytesParams(params) || isNotValidPageOffsetAndSize(params.offset, params.size)) return -1;
	
	if (IS_CACHE_ACTIVE) {
		return getBytesFromCache(buffer, params);
	} else {
		return getBytesFromMemory(buffer, params);		
	}
		
}

int saveBytesInPage(t_function_save_bytes_params params) {
	log_info(LOGGER, "Guardando bytes pid: %d, page: %d, offset: %d, size: %d", params.pid, params.page, params.offset, params.size);

	if (isNotValidSaveBytesParams(params)) return -1;
	
	if (IS_CACHE_ACTIVE) {
		return saveBytesInCache(params);
	} else {
		return saveBytesInMemory(params);	
	}
}

int assignPagesToProgram(int pid, int pagesCount) {
	log_info(LOGGER, "Asignando paginas pid: %d, paginas: %d", pid, pagesCount);

	if (isNotValidPid(pid) || isNotValidPageCount(pagesCount)) {
		log_error(LOGGER, "No se pudo asignar paginas a PID %d (PID>=0), paginas requeridas %d (paginas>0).", pid, pagesCount);
		return -1;
	}
	
	sem_wait(&SEM_PAGES_TABLE);
	
	int currentLastPage = getLastPageOf(pid);
	if (currentLastPage < 0) {
		log_error(LOGGER, "PID %d no fue inicializado.", pid);
		sem_post(&SEM_PAGES_TABLE);
		return -2;
	}

	int frame = getEmptyFrame(pid, currentLastPage + 1);

	if (frame < 0) {
		log_error(LOGGER, "No hay paginas disponibles para asignar a PID %d.", pid);
		sem_post(&SEM_PAGES_TABLE);
		return -3;
	}
		
	int i;
	for (i = currentLastPage + 1; i <= currentLastPage + pagesCount; i++) {
		frame = getNextEmptyFrame(frame);
		if (frame < 0) {
			log_error(LOGGER, "No hay suficientes paginas disponibles para asignar a PID %d.", pid);
			dereferencePages(pid, currentLastPage + 1, currentLastPage + pagesCount + 1);
			sem_post(&SEM_PAGES_TABLE);
			return -3;
		} else {
			PAGES_TABLE[frame].pid = pid;
			PAGES_TABLE[frame].page = i;
		}
	}

	log_info(LOGGER, "%d paginas asignadas a PID %d.", pagesCount, pid);
	sem_post(&SEM_PAGES_TABLE);
	return currentLastPage + 1;
}

int finishProgram(int pid) {
	log_info(LOGGER, "Finalizando programa pid: %d", pid);

	if (isNotValidPid(pid)) {
		log_error(LOGGER, "No se pudo finalizar programa, PID invalido %d.", pid);
		return -1;
	}
	
	if (IS_CACHE_ACTIVE) {
		erasePagesOfPidInCache(pid);
	}

	sem_wait(&SEM_PAGES_TABLE);	
	eraseFramesOf(pid);
	sem_post(&SEM_PAGES_TABLE);

	log_info(LOGGER, "PID %d finalizando.", pid);
	return 0;
}

int deletePage(int pid, int page) {
	log_info(LOGGER, "Borrando pagina %d de pid: %d", page, pid);

	if (isNotValidPid(pid)) {
		log_error(LOGGER, "No se pudo borrar pagina, PID invalido %d.", pid);
		return -1;
	}

	if (IS_CACHE_ACTIVE) {
		erasePageOfPidInCache(pid, page);
	}

	sem_wait(&SEM_PAGES_TABLE);
	int status = erasePageFromFrame(pid, page);
	sem_post(&SEM_PAGES_TABLE);

	if (status < 0) {
		log_error(LOGGER, "No se encontro la pagina %d de PID %d.", page, pid);
	} else {
		log_info(LOGGER, "Pagina %d de PID %d borrada.", page, pid);
	}
	return status;
}

/*
 * MEMORY OPERATIONS
 * INTERFACE
 */

int getBytesFromMemory(void *buffer, t_function_get_bytes_params params) {
	sem_wait(&SEM_PAGES_TABLE);
	
	int frame = getFrame(params.pid, params.page);
	
	if (frame < 0) {
		sem_post(&SEM_PAGES_TABLE);
		log_error(LOGGER, "No se encontro la pagina %d de PID %d.", params.page, params.pid);
		return -2;
	}
	
	sleepMillis(CONF_DATA.retardoMemoria);
	int getStatus = getFromFrame(buffer, frame, params.offset, params.size);
	
	sem_post(&SEM_PAGES_TABLE);
	
	if (getStatus < 0) {
		log_error(LOGGER, "No se pudieron obtener los bytes de PID %d, pagina %d.",
				params.pid, params.page);
		return -2;
	}
	
	return 0;
}

int getPageFromMemory(void *buffer, int pid, int page) {
	t_function_get_bytes_params params;
	params.pid = pid;
	params.page = page;
	params.offset = 0;
	params.size = CONF_DATA.marcoSize;
	return getBytesFromMemory(buffer, params);
}

int saveBytesInMemory(t_function_save_bytes_params params) {
	sem_wait(&SEM_PAGES_TABLE);
	
	int frame = getFrame(params.pid, params.page);
	if (frame < 0) {
		log_error(LOGGER, "No se encontro la pagina %d de PID %d.", params.page, params.pid);
		return -2;
	}
	
	int writeStatus = writeOnFrame(frame, params.offset, params.size, params.buffer);

	sem_post(&SEM_PAGES_TABLE);
	
	if (writeStatus < 0) {
		log_error(LOGGER, "No se escribieron los bytes de PID %d.", params.pid);
		return -2;
	}
	
	log_info(LOGGER, "Bytes guardados pid: %d, page: %d, offset: %d, size: %d", params.pid, params.page, params.offset, params.size);
	return 0;
}

int getEmptyFrame(int pid, int page) {
	if (IS_HASHING_ACTIVE) {
		int frame = hashOf(pid, page);

		if (isNotValidFrame(frame)) {
			frame = ADMIN_PAGES_COUNT;
		}

		return getNextEmptyFrame(frame);
	} else {
		int frame;
		for (frame = ADMIN_PAGES_COUNT; frame < CONF_DATA.marcos; frame++) {
			if (isEmptyFrame(frame)) {
				return frame;
			}
		}
	}
	return -1;
}

int hashOf(int pid, int page) {
	int hash = ADMIN_PAGES_COUNT + pid * HASH_NUMBER + page;
	return isNotValidFrame(hash) ? ADMIN_PAGES_COUNT : hash;
}

int getNextEmptyFrame(int frame) {
	int pageTableFramesCount = CONF_DATA.marcos - ADMIN_PAGES_COUNT;
	int i;
	for (i = 0; i < pageTableFramesCount; i++, frame++) {
		if (isNotValidFrame(frame)) {
			frame = ADMIN_PAGES_COUNT;
		}
		if (isEmptyFrame(frame)) {
			return frame;
		}
	}

	return -1;
}

bool isEmptyFrame(int frame) {
	if (isNotValidFrame(frame)) return false;
	return PAGES_TABLE[frame].pid == -1;
}

bool isNotEmptyFrame(int frame) {
	return !isEmptyFrame(frame);
}

void eraseFrame(int frame) {
	if (frame < 0 || frame > CONF_DATA.marcos) {
		log_error(LOGGER, "Error al borrar frame %d (>=0 && <= %d).", frame, CONF_DATA.marcos);
		return;
	}
	
	PAGES_TABLE[frame].pid = -1;
	PAGES_TABLE[frame].page = -1;
}

void eraseFramesOf(int pid) {
	if (isNotValidPid(pid)) {
		log_error(LOGGER, "Error al borrar frames de PID %d (>=0).", pid);
		return;
	}
	
	int i;
	for (i = ADMIN_PAGES_COUNT; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid == pid) {
			eraseFrame(i);
		}
	}
}

int erasePageFromFrame(int pid, int page) {
	if (isNotValidPid(pid)) {
		log_error(LOGGER, "Error al borrar pagina %d de frame de PID %d (>=0).", page, pid);
		return -1;
	}

	int i;
	for (i = ADMIN_PAGES_COUNT; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid == pid && PAGES_TABLE[i].page == page) {
			eraseFrame(i);
			return 0;
		}
	}

	return -1;
}

int getLastPageOf(int pid) {
	if (isNotValidPid(pid)) {
		log_error(LOGGER, "Error al obtener ultima pagina de pid %d (>=0).", pid);
		return -1;
	}
	
	int lastPage = -1;
	
	int i;
	for (i = ADMIN_PAGES_COUNT; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid == pid) {
			if (PAGES_TABLE[i].page > lastPage) {
				lastPage = PAGES_TABLE[i].page;
			}
		}
	}
	
	return lastPage;
}

void dereferencePages(int pid, int fromPage, int toPage) {
	int i;
	for (i = 0; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid == pid) {
			int page;
			page = PAGES_TABLE[i].page; 
			if (page >= fromPage && page <= toPage) {
				eraseFrame(i);
			}
		}
	}	
}

bool isPresentPid(int pid) {
	if (isNotValidPid(pid)) {
		log_error(LOGGER, "Error al buscar pid %d (>=0).", pid);
		return false;
	}
	
	int frame = hashOf(pid, 0);
	int pageTableFramesCount = CONF_DATA.marcos - ADMIN_PAGES_COUNT;
	int i;
	for (i = 0; i < pageTableFramesCount; i++, frame++) {
		if (isNotValidFrame(frame)) {
			frame = ADMIN_PAGES_COUNT;
		}
		if (PAGES_TABLE[frame].pid == pid) {
			return true;
		}
	}

	return false;
}

int getFrame(int pid, int page) {
	if (isNotValidPid(pid) || isNotValidPageCount(page)) {
		log_error(LOGGER, "Error al obtener frame, pid %d (>=0), page %d (>=0).", pid, page);
		return -1;
	}
	
	int frame = hashOf(pid, page);
	int pageTableFramesCount = CONF_DATA.marcos - ADMIN_PAGES_COUNT;
	int i;
	for (i = 0; i < pageTableFramesCount; i++, frame++) {
		if (isNotValidFrame(frame)) {
			frame = ADMIN_PAGES_COUNT;
		}
		if (PAGES_TABLE[frame].pid == pid && PAGES_TABLE[frame].page == page) {
			return frame;
		}
	}

	return -1;
}

int writeOnFrame(int frame, int offset, int size, void *buffer) {
	if (isNotValidPageOffsetAndSize(offset, size)) {
		log_error(LOGGER, "Parametros invalidos para escribir frame, offset %d (>=0), size %d (>0).",
				offset, size);
		return -1;
	}
	
	int memoryOffset = getOffsetOf(frame, offset);
	
	if (memoryOffset < 0) {
		log_error(LOGGER, "Error al escribir frame %d.", frame);
		return -3;
	}
	
	sleepMillis(CONF_DATA.retardoMemoria);

	memcpy(MEMORY_BLOCK + memoryOffset, buffer, size);
	
	return 0;
}

int getFromFrame(void *buffer, int frame, int offset, int size) {
	if (isNotValidPageOffsetAndSize(offset, size)) return -1;
	
	int memoryOffset = getOffsetOf(frame, offset);
	
	if (memoryOffset < 0) {
		log_error(LOGGER, "Error al leer frame %d.", frame);
		return -3;
	}

	memcpy(buffer, MEMORY_BLOCK + memoryOffset, size);
	
	return 0;
}

int getOffsetOf(int frame, int offset) {
	if (isNotValidFrame(frame) || isNotValidOffset(offset)) return -1;
	
	return frame * CONF_DATA.marcoSize + offset;
}



/*
 * CHACHE
 */

void configCache() {
	CACHE = malloc(CONF_DATA.entradasCache * CONF_DATA.marcoSize);
	CACHE_TABLE = malloc(CONF_DATA.entradasCache * sizeof(t_cache_row));

	eraseCache();
}

void eraseCache() {
	sem_wait(&SEM_CACHE);
	int i;
	for (i = 0; i < CONF_DATA.entradasCache; i++) {
		CACHE_TABLE[i].pid = -1;
		CACHE_TABLE[i].page = -1;
		CACHE_TABLE[i].content = CACHE + i * CONF_DATA.marcoSize;
	}
	sem_post(&SEM_CACHE);
}

int saveBytesInCache(t_function_save_bytes_params params) {
	sem_wait(&SEM_CACHE);
	int cacheIndex = getPageInCache(params.pid, params.page);
	if (cacheIndex < 0) {
		sem_post(&SEM_CACHE);
		return -1;
	}

	memcpy(CACHE_TABLE[cacheIndex].content + params.offset, params.buffer, params.size);
	sem_post(&SEM_CACHE);
	
	return saveBytesInMemory(params);
}

int getPageInCache(int pid, int page) {
	int pidPagesCount = 0;
	int i;
	for (i = 0; i < CONF_DATA.entradasCache; i++) {
		if (CACHE_TABLE[i].pid == pid) {
			if (CACHE_TABLE[i].page == page) {
				setCacheIndexAsUsed(i);
				return 0;
			}
			pidPagesCount++;
			if (pidPagesCount >= CONF_DATA.cacheXProc) break;
		} else if (CACHE_TABLE[i].pid == -1) break;
	}
	
	if (getPageFromMemory(CACHE_TABLE[i].content, pid, page) < 0) return -1;

	CACHE_TABLE[i].pid = pid;
	CACHE_TABLE[i].page = page;
	setCacheIndexAsUsed(i);
	return 0;
}

void setCacheIndexAsUsed(int index) {
	t_cache_row row = CACHE_TABLE[index];
	int i;
	for (i = index; i > 0; i--) {
		CACHE_TABLE[i] = CACHE_TABLE[i - 1];
	}
	CACHE_TABLE[0] = row;
}

void eraseCacheIndex(int index) {
	if (isNotValidCacheIndex(index)) return;
	
	CACHE_TABLE[index].pid = -1;
	CACHE_TABLE[index].page = -1;
	
	moveCacheIndexToLastPosition(index);
}

void moveCacheIndexToLastPosition(int index) {
	t_cache_row row = CACHE_TABLE[index];
	int i;
	for (i = index; i < CONF_DATA.entradasCache - 1; i++) {
		CACHE_TABLE[i] = CACHE_TABLE[i + 1];
	}
	CACHE_TABLE[CONF_DATA.entradasCache - 1] = row;
}

int getBytesFromCache(void *buffer, t_function_get_bytes_params params) {
	sem_wait(&SEM_CACHE);
	int cacheIndex = getPageInCache(params.pid, params.page);
	if (cacheIndex >= 0) {
		memcpy(buffer, CACHE_TABLE[cacheIndex].content + params.offset, params.size);
		log_info(LOGGER, "Bytes leidos pid: %d, page: %d, offset: %d, size: %d", params.pid, params.page, params.offset, params.size);
	} else {
		log_error(LOGGER, "No se pudieron leer bytes para pid: %d, page: %d, offset: %d, size: %d", params.pid, params.page, params.offset, params.size);
	}
	sem_post(&SEM_CACHE);
	return cacheIndex;
}

void erasePagesOfPidInCache(int pid) {
	int i;
	sem_wait(&SEM_CACHE);
	for (i = 0; i < CONF_DATA.entradasCache; i++) {
		if (CACHE_TABLE[i].pid == pid) {
			eraseCacheIndex(i);
		}
	}
	sem_post(&SEM_CACHE);
}

void erasePageOfPidInCache(int pid, int page) {
	int i;
	sem_wait(&SEM_CACHE);
	for (i = 0; i < CONF_DATA.entradasCache; i++) {
		if (CACHE_TABLE[i].pid == pid && CACHE_TABLE[i].page == page) {
			eraseCacheIndex(i);
		}
	}
	sem_post(&SEM_CACHE);
}

void dumpCacheTable() {
	int i;
	for (i = 0; i < CONF_DATA.entradasCache; i++) {
		printf("row %d, pid %d, page %d, content: %s\n", i, CACHE_TABLE[i].pid, CACHE_TABLE[i].page, (char*) CACHE_TABLE[i].content);
	}
}

/*
 * CHACHE
 * end
 */





/*
 * VALIDATION
 */

bool isNotValidFrame(int frame) {
	return (frame < ADMIN_PAGES_COUNT) || (frame >= CONF_DATA.marcos);
}

bool isNotValidOffset(int offset) {
	return (offset < 0) || (offset > CONF_DATA.marcoSize);
}

bool isNotValidSize(int size) {
	return (size < 0) || (size > CONF_DATA.marcoSize);
}

bool isNotValidPageOffsetAndSize(int offset, int size) {
	return isNotValidOffset(offset) || isNotValidSize(size) || (offset + size > CONF_DATA.marcoSize);
}

bool isNotValidCacheIndex(int index) {
	return (index < 0) || (index > CONF_DATA.entradasCache);
}

bool isNotValidPid(int pid) {
	return pid < 0;
}

bool isNotValidPageCount(int pageCount) {
	return pageCount < 0;
}

bool isNotValidSaveBytesParams(t_function_save_bytes_params params) {
	return isNotValidPid(params.pid) || isNotValidPid(params.page) || isNotValidOffset(params.offset) ||
			isNotValidSize(params.size) || isNotValidPageOffsetAndSize(params.offset, params.size);
}

bool isNotValidGetBytesParams(t_function_get_bytes_params params) {
	return isNotValidPid(params.pid) || isNotValidPid(params.page) || isNotValidOffset(params.offset) || 
			isNotValidSize(params.size) || isNotValidPageOffsetAndSize(params.offset, params.size);
}



/*
 * VALIDATION
 * end
 */









/*
 * CONSOLE
 * start
 */
void initConsole() {
	char str[50];
		
	while(1) {
		fgets(str, sizeof(str), stdin);
		
		if (memcmp(str, "retardo", 7) == 0) {
			char **spl = string_split(str, " ");
			if (spl[1] != NULL) {
				CONF_DATA.retardoMemoria = atoi(spl[1]); 
				printf("Nuevo retardo %d\n", CONF_DATA.retardoMemoria);
			}
		} else if (memcmp(str, "dump", 4) == 0) {
			char **spl = string_split(str, " ");
			if (spl[1] != NULL) {
				if (memcmp(spl[1], "-cache", 6) == 0) {
					dumpCacheTable();
				} else if (memcmp(spl[1], "-s", 2) == 0) {
					dumpMemoryStructs();
				} else if (memcmp(spl[1], "-c", 2) == 0) {
					if (spl[2] == NULL) {
						dumpMemoryContent();						
					} else {
						int pid = atoi(spl[2]);
						if (pid >= 0) {
							dumpMemoryContentOf(pid);	
						} else {
							printf("PID incorrecto\n");
						}
					}
				}
			}
		} else if (memcmp(str, "flush", 5) == 0) {
			eraseCache();
			printf("Se limpio la memoria cache\n");
		} else if (memcmp(str, "size", 4) == 0) {
			if (str[4] == '\n') {
				int framesCount = getFramesCount();
				int emptyFramesCount = getEmptyFramesCount();
				printf("Frames ocupados: %d\nFrames libres: %d\n", framesCount - emptyFramesCount, emptyFramesCount);
			} else {
				char **spl = string_split(str, " "); 
				if (spl[1] != NULL) {
					int pid = atoi(spl[1]);
					if (pid >= 0) {
						int framesCount = getFramesCountTo(pid);
						printf("Frames ocupados por PID %d: %d\n", pid, framesCount);
					} else {
						printf("PID incorrecto\n");
					}
				}
			}
		} else if (memcmp(str, "hash", 4) == 0) {
			char **spl = string_split(str, " ");
			if (spl[1] != NULL) {
				HASH_NUMBER = atoi(spl[1]);
				printf("Nuevo numero de hash %d\n", HASH_NUMBER);
			}
		}
	}
}

int getFramesCount() {
	return CONF_DATA.marcos;
}

int getEmptyFramesCount() {
	int count = 0;
	int i;
	for (i = 0; i < getFramesCount(); i++) {
		if (isEmptyFrame(i)) count++;
	}
	return count;
}

int getFramesCountTo(int pid) {
	int count = 0;
	int i;
	for (i = 0; i < getFramesCount(); i++) {
		if (PAGES_TABLE[i].pid == pid) count++;
	}
	return count;
}

/*
 * CONSOLE
 * end
 */



void dumpMemoryStructs() {
	t_list *pids = list_create();

	printf("Tabla de paginas:\n");
	int i;
	for (i = 0; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid >= 0) {
			addIfNotPresent(pids, &(PAGES_TABLE[i].pid));
		}
		printf("frame %d, pid %d, page %d\n", i, PAGES_TABLE[i].pid, PAGES_TABLE[i].page);
	}
	
	printf("Procesos activos: ");
	for (i = 0; i < list_size(pids); i++) {
		int *pid = list_get(pids, i);
		printf("%d ", *pid);
	}
	printf("\n");
}

void addIfNotPresent(t_list *list, int *e) {
	int i;
	for (i = 0; i < list_size(list); i++) {
		int *e2 = list_get(list, i);
		if (*e == *e2) {
			return;
		}
	}
	list_add(list, e);
}

void dumpMemoryContent() {
	printf("Contenido de memoria\n");
	void *frame = malloc(CONF_DATA.marcoSize);
	int i;
	for (i = 0; i < CONF_DATA.marcos; i++) {
		getFromFrame(frame, i, 0, CONF_DATA.marcoSize);
		printf("Frame %d, contenido: [%s]\n", i, (char*)frame);
	}
	free(frame);
}

void dumpMemoryContentOf(int pid) {
	printf("Contenido de memoria para pid %d\n", pid);
	void *frame = malloc(CONF_DATA.marcoSize);
	int i;
	for (i = 0; i < CONF_DATA.marcos; i++) {
		if (PAGES_TABLE[i].pid == pid) {
			getFromFrame(frame, i, 0, CONF_DATA.marcoSize);
			printf("Frame %d, contenido: [%s]\n", i, (char*)frame);
		}
	}
	free(frame);
}

void sleepMillis(int millis) {
	int nanos = millis % 1000;
	int sec = millis / 1000;

	struct timespec tim, tim2;
	tim.tv_sec  = sec;
	tim.tv_nsec = nanos * 1000000L;

	nanosleep(&tim, &tim2);
}

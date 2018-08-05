#include "Utils.h"

t_log *LOGGER;

int conectarseAServidor(char* ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	getaddrinfo(ip, puerto, &hints, &serverInfo);

	int socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype,
			serverInfo->ai_protocol);

	int estadoConnect = connect(socketServidor, serverInfo->ai_addr,
			serverInfo->ai_addrlen);
	if (estadoConnect == -1) {
		return estadoConnect;
	}

	freeaddrinfo(serverInfo);

	return socketServidor;
}

void configLog(char *path, char* process) {
	LOGGER = log_create(path, process, false, LOG_LEVEL_TRACE);
	log_info(LOGGER, "Logger inicializado");
}

char * assignString(char * string) {

	char* assignString;
	size_t sizePuerto;
	sizePuerto = strlen(string);
	assignString = malloc(sizePuerto + 1);
	memcpy(assignString, string, sizePuerto);
	assignString[sizePuerto] = '\0';
	return assignString;
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

int conectarClienteDesdeServidor(int socketEscucha) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	int socketCliente = accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);

	return socketCliente;
}

int getClientType(int socket) {
	uint32_t code;
	int recvLenght = recv(socket, &code, sizeof(code), 0);
	if (recvLenght <= 0) { return -1; }
	
	return code;
}


#ifndef SRC_OBJ_MEMORIA_H_
#define SRC_OBJ_MEMORIA_H_

#include "../Kernel.h"

#define RESERVAR_MEMORIA 0
#define GUARDAR_MEMORIA 2
#define LIMPIAR_MEMORIA 4
#define OBTENER_MEMORIA 1
#define NUEVA_PAGINA_MEMORIA 3
#define DELETE_PAGINA 5

int conectarAMemoria(int puerto, char* ip) {
	memoria = conectar(puerto, ip);

	if (memoria < 0) {
		return 0;
	}

	send(memoria, &SUCCESS, sizeof(SUCCESS), 0);
	tamanioPagina = esperarConfirmacion(memoria);

	if (tamanioPagina < 1) {
		return 0;
	}

	return 1;
}

void liberarPagina(int pagina, int pid) {
	size_t bufferSize = sizeof(int32_t) * 3;
	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(int32_t);

	int32_t functionId = DELETE_PAGINA;
	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &pagina, sizeToCopy);

	send(memoria, buffer, bufferSize, 0);

	free(buffer);

	esperarConfirmacion(memoria);
}

int leerMemoria(int pid, int pagina, int offset, int size) {
	size_t bufferSize = sizeof(int32_t) * 5;

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(int32_t);

	int32_t functionId = OBTENER_MEMORIA;

	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &pagina, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &offset, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &size, sizeToCopy);

	send(memoria, buffer, bufferSize, 0);

	free(buffer);

	char* bufferHandshake = malloc(sizeof(char) * (size + 1));

	recv(memoria, &size, sizeof(size), MSG_WAITALL);

	int status = recv(memoria, bufferHandshake, size, MSG_WAITALL);

	if (status  < 0) logError("ERROR CON LA MEMORIA");

	bufferHandshake[size] = '\0';

	int pages = atoi(bufferHandshake);
	free(bufferHandshake);

	return pages;
}

int pedirPagina(int pid) {
	size_t bufferSize = sizeof(int32_t) * 3;

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(int32_t);

	int32_t functionId = NUEVA_PAGINA_MEMORIA;

	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	int cero = 0;

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &cero, sizeToCopy);

	send(memoria, buffer, bufferSize, 0);

	return esperarConfirmacion(memoria);
}

void escribirMemoria(int pid, int pagina, int offset, char* dato) {
	size_t bufferSize = sizeof(uint32_t) * 5
			+ sizeof(char) * string_length(dato);

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(uint32_t);

	uint32_t size = string_length(dato);

	uint32_t funcionId = GUARDAR_MEMORIA;

	memcpy(buffer, &funcionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &pagina, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &offset, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &size, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, dato, string_length(dato));

	send(memoria, buffer, bufferSize, 0);

	free(buffer);

	esperarConfirmacion(memoria);
}

int cantidadDePaginas(char* codigo) {
	int paginas = string_length(codigo) / tamanioPagina;
	if (string_length(codigo) % tamanioPagina)
		paginas++;
	return paginas;
}

int reservarEnMemoria(uint32_t id, uint32_t cantPaginas) {
	size_t bufferSize = sizeof(uint32_t) * 3;

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(uint32_t);

	uint32_t functionId = RESERVAR_MEMORIA;

	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &id, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &cantPaginas, sizeToCopy);

	send(memoria, buffer, bufferSize, 0);

	free(buffer);

	return esperarConfirmacion(memoria);
}

void insertarEnMemoria(char* codigo, uint32_t id) {
	int pagina = 0;
	int offSetSplit = 0;
	int tamanioTotal = string_length(codigo);

	while (offSetSplit < tamanioTotal) {
		char* subCodigo = string_substring(codigo, offSetSplit, tamanioPagina);
		escribirMemoria(id, pagina, 0, subCodigo);
		pagina++;
		offSetSplit = offSetSplit + tamanioPagina;
		free(subCodigo);
	}
}

void liberarRecursos(int id) {
	liberarFileSistem(id);
	size_t bufferSize = sizeof(uint32_t) + sizeof(int);
	size_t sizeToCopy = sizeof(uint32_t);

	void *buffer = malloc(bufferSize);

	uint32_t funcionId = LIMPIAR_MEMORIA;

	memcpy(buffer, &funcionId, sizeToCopy);
	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &id, sizeToCopy);

	send(memoria, buffer, bufferSize, 0);

	free(buffer);

	esperarConfirmacion(memoria);
}


int sendAssignPagesToProgramParams(int socket, int pid, int pagesCount) {
	size_t bufferSize = sizeof(int32_t) * 3;
	
	void *buffer = malloc(bufferSize);
	
	size_t sizeToCopy = sizeof(int32_t);
	int32_t functionId = NUEVA_PAGINA_MEMORIA;
	memcpy(buffer, &functionId, sizeToCopy);
	
	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &pagesCount, sizeToCopy);
	
	int sendStatus = send(socket, buffer, bufferSize, 0);
	
	free(buffer);
	
	return sendStatus < bufferSize ? -1 : 0;
}


int nuevaPagina(int pid) {
	int pagesCount = 1;
	int sendStatus = sendAssignPagesToProgramParams(memoria, pid, pagesCount);
	if (sendStatus < 0) return -1;
	int pagina = esperarConfirmacion(memoria);
	return pagina <= 0 ? 0 : pagina;
}


#endif

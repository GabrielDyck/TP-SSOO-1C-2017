#ifndef SRC_MEMORIA_H_
#define SRC_MEMORIA_H_

#include "Cpu.h"

int executeMemoryAction(int memoriaSocket, int actionId, int *buffer, int pid, int page, int offset, int size) {
	int statusMem = 0;
	switch(actionId) {
		case FUNC_GET_BYTES:
			log_info(LOGGER, "Leyendo de memoria: PID: %d || Pagina: %d || Offset: %d || size: %d", pid, page, offset, size);
			statusMem = getBytesFromMemory(buffer, memoriaSocket, pid, page, offset, size);
			break;
		case FUNC_SAVE_BYTES:
			log_info(LOGGER, "Escribiendo en memoria: PID: %d || Pagina: %d || Offset: %d || size: %d", pid, page, offset, size);
			statusMem = saveBytesOnMemory(memoriaSocket, pid, page, offset, size, buffer);
			break;
		default:
			log_error(LOGGER,"Funcion de memoria no reconocida");
			statusMem = -1;
			break;
	}

	if(statusMem < 0) {
		log_error(LOGGER, "La funcion solicitada a memoria ha fallado su ejecucion");
		log_info(LOGGER, "Detenemos el script..");
		char* mensaje = string_new();
		char* statusChar = toStringInt(status);
		string_append(&mensaje,statusChar);
		char* pidChar = toStringInt(pcb->id);
		string_append(&mensaje, pidChar);
		log_info(LOGGER,"Enviando PID: %s y status %s al kernel", statusChar, pidChar);
		enviarMensajeConProtocolo(kernel,mensaje,10);
		finalizado = -1;
	}
	return statusMem;
}

int saveBytesOnMemory(int socket, int pid, int page, int offset, int size, void *buffer) {
	int sendStatus = sendSaveBytesParams(socket, pid, page, offset, size, buffer);
	if (sendStatus < 0) {
		return -1;
	}

	int recvStatus = recvSaveBytesStatus(socket);
	if (recvStatus < 0) {
		return -1;
	}


	return 0;
}

int sendSaveBytesParams(int socket, int pid, int page, int offset, int size, void *bytes) {
	size_t bufferSize = sizeof(int32_t) * 5 + size;

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(int32_t);

	int32_t functionId = FUNC_SAVE_BYTES;

	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &page, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &offset, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &size, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, bytes, size);

	int sendStatus = send(socket, buffer, bufferSize, 0);

	free(buffer);

	if (sendStatus != bufferSize) {
		return -1;
	}

	return 0;
}

int getBytesFromMemory(void *buffer, int socket, int pid, int page, int offset, int size) {
	int sendStatus = sendGetBytesParams(socket, pid, page, offset, size);
	if (sendStatus < 0) {
		return -1;
	}

	int recvStatus = recvBytes(buffer, socket);
	if (recvStatus < 0) {
		return -1;
	}

	return 0;
}

int sendGetBytesParams(int socket, int pid, int page, int offset, int size) {
	size_t bufferSize = sizeof(int32_t) * 5;

	void *buffer = malloc(bufferSize);

	size_t sizeToCopy = sizeof(int32_t);

	int32_t functionId = FUNC_GET_BYTES;

	memcpy(buffer, &functionId, sizeToCopy);

	int offsetToCopy = sizeToCopy;
	memcpy(buffer + offsetToCopy, &pid, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &page, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &offset, sizeToCopy);

	offsetToCopy += sizeToCopy;
	memcpy(buffer + offsetToCopy, &size, sizeToCopy);

	int sendStatus = send(socket, buffer, bufferSize, 0);

	free(buffer);

	if (sendStatus < 0) {
		return -1;
	}

	return 0;
}

int recvBytes(void *buffer, int socket) {
	int32_t size = -1;

	int recvStatus = recv(socket, &size, sizeof(size), 0);
	if (recvStatus < 0 || size < 0) {
		return -1;
	}

	recvStatus = recv(socket, buffer, size, 0);
	if (recvStatus < 0) {
		return -1;
	}

	return recvStatus;
}

int recvSaveBytesStatus(int socket) {
	int32_t status = -1;

	int recvStatus = recv(socket, &status, sizeof(status), 0);
	if (recvStatus < 0 || status < 0) {
		return -1;
	}

	return recvStatus;
}


#endif

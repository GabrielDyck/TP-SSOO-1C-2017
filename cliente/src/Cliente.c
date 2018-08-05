#include "Cliente.h"

//La idea es que el cliente le mande Ping y el servidor responda Pong

int main() {
    archivoLog = log_create("Cliente.log", "Cliente", 1, log_level_from_string("INFO"));

    logInfo("Cliente estable...\n");
	
	if (!conectarseAlServidor()) return -1;

	procesarPeticion();

	logInfo("Cerrando Cliente.. \n");

	return 0;
}

int conectarseAlServidor() {
	logInfo("Conectando con el Servidor...");
	servidor = conectar(5000,"127.0.0.1");
	if (servidor < 0){
		logError("Error al conectarse con el servidor \n");
		return 0;
	}
	uint32_t s = 1;
	send(servidor, &s, sizeof(s), 0);
	logInfo("CONECTADO AL SERVIDOR !!\n");
	
	return 1;
}

void procesarPeticion() {
	char mensaje[10] = "Ping";

	logInfo("Enviando Mensaje ...\n");

	enviarMensajeConProtocolo(servidor, mensaje,0);

	logInfo("Esperando Respuesta ...\n");

	char* respuesta = esperarMensaje(servidor);

	logInfo(respuesta);
	
	free(respuesta);
}
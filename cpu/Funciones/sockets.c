#include "sockets.h"

int conectar(int puerto,char* ip){
	struct sockaddr_in direccServ;
	direccServ.sin_family = AF_INET;
	direccServ.sin_addr.s_addr = inet_addr(ip);
	direccServ.sin_port = htons(puerto);
	int conexion = socket(AF_INET, SOCK_STREAM, 0);
	while (connect(conexion, (void*) &direccServ, sizeof(direccServ)));
	return conexion;
}
void enviarMensaje(int conexion, char* mensaje){
	char* mensajeReal =string_new();
	uint32_t longitud = strlen(mensaje);
	string_append(&mensajeReal,header(longitud));
	string_append(&mensajeReal,mensaje);
	send(conexion,mensajeReal,strlen(mensajeReal),0);
	free(mensajeReal);
}

void enviarMensajeConProtocolo(int conexion, char* mensaje, int protocolo){
	char* mensajeReal = string_new();
	uint32_t longitud = strlen(mensaje);
	string_append(&mensajeReal,header(protocolo));
	string_append(&mensajeReal,header(longitud));
	string_append(&mensajeReal,mensaje);
	send(conexion,mensajeReal,strlen(mensajeReal),0);
	free(mensajeReal);
}

int autentificar(int conexion, char* autor){
	send(conexion,autor,strlen(autor),0);
	return (esperarConfirmacion(conexion));
}

int esperarConfirmacion(int conexion){
	int bufferHandshake;
	int bytesRecibidos = recv(conexion, &bufferHandshake, 4 , MSG_WAITALL);
	if (bytesRecibidos <= 0) {
		return 0;
	}
	bufferHandshake=ntohl(bufferHandshake);
	return bufferHandshake;
}


char* esperarMensaje(int conexion){
	char header[5];
	char* buffer;
	int bytes= recv(conexion, header,4,MSG_WAITALL);
	header[4]= '\0';
	uint32_t tamanioPaquete = atoi(header);
	if (bytes<=0){
		buffer = malloc(2*sizeof(char));
		buffer[0] = '\0';
	}else{
		buffer = malloc(tamanioPaquete+1);
		recv(conexion,buffer,tamanioPaquete,MSG_WAITALL);
		buffer[tamanioPaquete] = '\0';
	}
	return buffer;
}


char* header(int numero){										
	char* longitud=string_new();
	string_append(&longitud,string_reverse(string_itoa(numero)));
	string_append(&longitud,"0000");
	longitud=string_substring(longitud,0,4);
	longitud=string_reverse(longitud);
	return longitud;
}

int recibirProtocolo(int conexion){
	char protocolo[5];
	int bytes= recv(conexion, protocolo,4,MSG_WAITALL);
	if(bytes!=4){
		perror("Error protocolo");
		return -1;
	}
	protocolo[4] = '\0';
	return atoi(protocolo);
}


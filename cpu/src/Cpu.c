#include "parser.h"
#include "memoria.h"

int main() {
	LOGGER = log_create("Cpu.log", "CPU", 1, log_level_from_string("DEBUG"));
	if (!leerConfiguracion())
		return -1;

	conectarseAlKernel();
	if (kernel < 0)
		return -1;

	conectarseMemoria();
	if (memoria < 0)
		return -1;

	crearHiloSignal();

	log_info(LOGGER, "CPU estable...\n");

	procesarPeticionKernel();

	log_info(LOGGER, "Cerrando CPU.. \n");

	return 0;
}

void conectarseAlKernel() {
	log_info(LOGGER, "Conectando con el Kernel");
	kernel = conectar(atoi(PUERTO_KERNEL), IP_KERNEL);
	if (kernel < 0) {
		log_error(LOGGER, "Error al conectarse con el kernel \n");
		return;
	}
	uint32_t code;
	code = 2;
	send(kernel, &code, sizeof(code), 0);
	log_info(LOGGER, "esperando tamanio de pagina..");
	TAMANIO_PAGINA = recibirProtocolo(kernel);
	if (TAMANIO_PAGINA) {
		log_info(LOGGER, "Conexion con el kernel OK... \n");
		log_info(LOGGER, "TAMANIO_PAGINA: %d", TAMANIO_PAGINA);
	} else {
		kernel = -1;
	}
}

void conectarseMemoria() {
	log_info(LOGGER, "Conectando con la Memoria");
	memoria = conectar(atoi(PUERTO_MEMORIA), IP_MEMORIA);
	if (memoria < 0) {
		log_error(LOGGER, "Error al conectarse con la memoria \n");
		return;
	}

	int32_t code;
	code = 2;
	send(memoria, &code, sizeof(code), 0);
	log_info(LOGGER, "Conexion con la memoria OK... \n");
}

int leerConfiguracion() {
	log_info(LOGGER, "Buscando archivo de configuracion para la CPU");
	archivoConfiguracion = config_create(RUTA_CONFIG);
	if (archivoConfiguracion == NULL) {
		log_error(LOGGER,
				"No se encontro el archivo de configuracion para la CPU");
		return 0;
	} else {
		int cantidadKeys = config_keys_amount(archivoConfiguracion);
		if (cantidadKeys < 4) {
			log_error(LOGGER,
					"No se encontraron las 4 keys del archivo de configuracion...\n");
			return 0;
		} else {
			IP_KERNEL = string_new();
			IP_MEMORIA = string_new();
			string_append(&IP_KERNEL, getStringConf("IP_KERNEL"));
			log_info(LOGGER, "IP_KERNEL: %s", IP_KERNEL);
			PUERTO_KERNEL = string_itoa(
					config_get_int_value(archivoConfiguracion,
							"PUERTO_KERNEL"));
			log_info(LOGGER, "PUERTO_KERNEL: %s", PUERTO_KERNEL);
			string_append(&IP_MEMORIA, getStringConf("IP_MEMORIA"));
			log_info(LOGGER, "IP_MEMORIA: %s", IP_MEMORIA);
			PUERTO_MEMORIA = string_itoa(
					config_get_int_value(archivoConfiguracion,
							"PUERTO_MEMORIA"));
			log_info(LOGGER, "PUERTO_MEMORIA: %s", PUERTO_MEMORIA);
		}
	}
	return 1;
}

void crearHiloSignal() {
	log_info(LOGGER, "Creando hilo signal...");
	pthread_t th_seniales;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(&th_seniales, NULL, (void*) hiloSignal, NULL);
}

void hiloSignal() {
	signal(SIGUSR1, cerrarCPU);	//(SIGUSR1) ---------> Kill -10 (ProcessPid)
	signal(SIGINT, cerrarCPU);	//(SIGINT) ---------> Ctrl+C
}

void cerrarCPU(int senial) {
	log_info(LOGGER, "Me mataron con SIGUSR1\n");
	if (senial == SIGUSR1) {
		status = 0;
		if (!IS_EXECUTING) {
			close(kernel);
			close(memoria);
			exit(0);
		}
	} else if (senial == SIGINT) {
		if (IS_EXECUTING)
			enviarMensajeConProtocolo(kernel, toStringInt(pcb->id), PROTOCOLO_MORTAL);
		close(memoria);
		close(kernel);
		exit(0);
	}
}

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

////////////////////////////////////
/////// Kernel /////////////////7/
////////////////////////////////////

void procesarPeticionKernel() {
	//while ((!finalizado) && (status)){
	while (status) {
		log_info(LOGGER, "Esperando peticion del KERNEL..");
		char *mensajeKernel = esperarMensaje(kernel);
		if (!status) {
			sendInt(kernel, 0);
			free(mensajeKernel);
		} else {
			IS_EXECUTING = 1;
			sendInt(kernel, 1);
			log_info(LOGGER, "Peticion del kernel recibida..");

			char* subQuantum = toSubString(mensajeKernel, 0, 3);
			int quantum = atoi(subQuantum);
			free(subQuantum);
			char *subQuantumSleep = toSubString(mensajeKernel, 4, 7);
			int quantumSleep = atoi(subQuantumSleep);
			free(subQuantumSleep);
			log_info(LOGGER, "Quantum recibido: %d", quantum);
			log_info(LOGGER, "Quantum Sleep recibido: %d\n", quantumSleep);
			char* pcbRecibido = toSubString(mensajeKernel, 8,
					string_length(mensajeKernel) - 1);
			log_info(LOGGER, "PCB recibido %s\n", pcbRecibido);
			pcb = fromStringPCB(pcbRecibido);
			free(pcbRecibido);
			free(mensajeKernel);

			log_info(LOGGER, "Ejecutar Proceso: %d\n", pcb->id);
			procesarCodigo(quantum, quantumSleep);
			IS_EXECUTING = 0;
			liberarPCB(pcb);
		}
	}
}

void procesarCodigo(int quantum, int quantum_sleep) {
	finalizado = 0;
	if (quantum == 0) quantum--;
	while (quantum && !finalizado) {
		log_info(LOGGER, "Program Counter: %d\n", pcb->pc);
		char* linea = pedirLineaAMemoria();

		log_info(LOGGER, "Recibi: %s \n", linea);

		parsear(linea);
		quantum--;
		pcb->pc++;
		free(linea);
		usleep(quantum_sleep * 1000);
	}
	if (!finalizado) {
		char* mensaje = string_new();
		char* statusChar = toStringInt(status);
		string_append(&mensaje, statusChar);
		char* pcb_char = toStringPCB(pcb);
		string_append(&mensaje, pcb_char);
		log_info(LOGGER, "Enviando PCB al kernel: %s \n\n", pcb_char);
		enviarMensajeConProtocolo(kernel, mensaje, 5);
		free(pcb_char);
		free(mensaje);
		free(statusChar);
		log_info(LOGGER, "PCB enviado al kernel \n");
	}
}

char* pedirLineaAMemoria() {
	int pid = pcb->id;

	int start = pcb->indices->instrucciones_serializado[pcb->pc].start;
	int longitud = (pcb->indices->instrucciones_serializado[pcb->pc].offset)
			- 1;
	log_info(LOGGER, "Peticion de linea: %d , longitud: %d", start, longitud);

	int pag = start / TAMANIO_PAGINA;
	int off = start % TAMANIO_PAGINA;

	int size_page = longitud;

	char* bufferFinal = string_new();

	while (longitud > 0) {
		if (longitud > TAMANIO_PAGINA - off) {
			size_page = TAMANIO_PAGINA - off;
		} else {
			size_page = longitud;
		}

		char* buffer = malloc((size_page + 1) * sizeof(char));

		executeMemoryAction(memoria, FUNC_GET_BYTES, buffer, pid, pag, off,
				size_page);
		buffer[size_page] = '\0';

		log_info(LOGGER, "Envio Parcial de la memoria: %s", buffer);

		string_append(&bufferFinal, buffer);
		free(buffer);
		pag++;
		longitud -= size_page;
		off = 0;
	}

	return bufferFinal;
}

void parsear(char* instruccion) {
	analizadorLinea(strdup(instruccion), &funciones, &kernelParser);
	log_info(LOGGER, "Fin del parser\n");
}

////////////////////// Helpers /////////////////

Variable* crearVariable(t_nombre_variable variable) {
	Variable* var = malloc(sizeof(Variable));
	var->id = variable;
	var->offset = siguientePosicion();
	return var;
}

void sumarEnLasVariables(Variable* var) {
	Stack* stackActual = obtenerStack();
	log_info(LOGGER, "Agregando a la lista de variables: %c", var->id);
	if (isdigit(var->id)) {
		list_add(stackActual->args, var);
	} else {
		list_add(stackActual->vars, var);
	}
}

int recvInt(int socket) {
	int32_t number;
	if (recv(socket, &number, sizeof(number), 0) < 0) {
		log_error(LOGGER, "Fallo al recibir int");
		return -1;
	}
	return number;
}

int sendInt(int socket, int number) {
	int buf = htonl(number);
	return send(socket, &buf, sizeof(buf), 0);
}

//////////////////////////////////////////////////
/////////////////////////////////////////////////
/////////////////    Stack    //////////////////
///////////////////////////////////////////////
//////////////////////////////////////////////

Stack* obtenerStack() {
	int tamanioStack = list_size(pcb->stack);
	log_info(LOGGER, "Tamanio Stack: %d", tamanioStack);
	if (tamanioStack == 0) {
		return inicializarStack();
	}
	return (list_get(pcb->stack, tamanioStack - 1));
}

Stack* inicializarStack() {
	Stack* stack = malloc(sizeof(Stack));
	stack->vars = list_create();
	stack->retVar = 0;
	stack->retPos = 0;
	stack->args = list_create();
	list_add(pcb->stack, stack);
	return stack;
}

int siguientePosicion() {
	int i = list_size(pcb->stack);

	for (; i > 0; i--) {
		Stack* stackActual = list_get(pcb->stack, i - 1);
		if (list_size(stackActual->vars) > 0) {
			Variable* ultimaVariable = list_get(stackActual->vars,
					list_size(stackActual->vars) - 1);
			return ultimaVariable->offset + 4;
		}
	}

	return pcb->paginas_codigo * TAMANIO_PAGINA;

}

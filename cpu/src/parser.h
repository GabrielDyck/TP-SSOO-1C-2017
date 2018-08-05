#ifndef SRC_PARSER_H_
#define SRC_PARSER_H_

#include "Cpu.h"

t_puntero definirVariable(t_nombre_variable identificador_variable) {

	log_info(LOGGER, "DefinirVariable %c", identificador_variable);

	Variable* var = crearVariable(identificador_variable);

	log_info(LOGGER, "Variable %c creada", var->id);

	sumarEnLasVariables(var);
	return var->offset;
}

t_puntero obtenerPosicionVariable(t_nombre_variable nombre_variable) {

	int variableBuscada(Variable* var) {
		return (var->id == nombre_variable);
	}

	Variable* var;

	log_info(LOGGER, "ObtenerPosicion de %c", nombre_variable);

	Stack* stackActual = obtenerStack();

	if (isdigit(nombre_variable)) {
		var = (Variable*) list_find(stackActual->args, (void*) variableBuscada);
	} else {
		var = (Variable*) list_find(stackActual->vars, (void*) variableBuscada);
	}

	if (var != NULL) {
		log_debug(LOGGER,"Retorno : %d\n", var->offset);
		flag = true;
		return var->offset;
	}


	log_error(LOGGER, "Error: No se encontro la variable\n");
	return -1;

}

t_valor_variable dereferenciar(t_puntero pagina) {
	int valorLeido;
	int pag = pagina / TAMANIO_PAGINA;
	int off = pagina % TAMANIO_PAGINA;
	int tamanio = flag ? 4 : 1;

	log_info(LOGGER, "Dereferenciar pag: %d off: %d tamaño: %d", pag, off,
			tamanio);

	int status = executeMemoryAction(memoria, FUNC_GET_BYTES, &valorLeido,
			pcb->id, pag, off, tamanio);

	if (status < 0) {
		log_error(LOGGER, "Error: Fallo la conexion con la memoria\n");
		return -1;
	}
	log_info(LOGGER, "VALOR VARIABLE: %d \n", valorLeido);
	flag = false;
	return valorLeido;
}

void asignar(t_puntero pagina, t_valor_variable valor) {
	if (pagina != -1) {

		int pag = pagina / TAMANIO_PAGINA;
		int off = pagina % TAMANIO_PAGINA;
		int tamanio = 4;

		log_info(LOGGER, "Asignar %d a pag: %d off: %d size: %d\n", valor, pag,
				off, tamanio);

		enviarMensajeMemoriaAsignacion(pag, off, sizeof(valor), pcb->id, valor);
	}
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable) {
	log_info(LOGGER, "Obtener Valor Compartido de: %s", variable);
	enviarMensajeConProtocolo(kernel, variable, 2);
	char* valorChar = esperarMensaje(kernel);
	log_info(LOGGER, "Valor recibido: %s\n", valorChar);
	int valor = atoi(valorChar);
	free(valorChar);

	return valor;
}

t_valor_variable asignarValorCompartida(t_nombre_compartida variable,
		t_valor_variable valor) {

	log_info(LOGGER, "Asignar valor compartido %d a %s", valor, variable);
	char* mensaje = string_new();
	char * valorChar = toStringInt(valor);
	string_append(&mensaje, valorChar);
	string_append(&mensaje, variable);
	enviarMensajeConProtocolo(kernel, mensaje, 3);
	free(valorChar);
	free(mensaje);
	char * respuesta = esperarMensaje(kernel);
	log_info(LOGGER, "respuesta: %s", respuesta);
	if (atoi(respuesta) == 1) {
		log_info(LOGGER, "valor asignado correctamente");
	} else {
		log_error(LOGGER, "Falle en la asignación, el kernel me mando %s",
				respuesta);
		//deberiamos cerrar o algo asi
	}
	free(respuesta);
	return valor;
}

void finalizar() {
	log_info(LOGGER, "Proceso %d finalizado\n", pcb->id);
	Stack* stackActual = obtenerStack();
	pcb->pc = stackActual->retPos;
	int tamanioStack = list_size(pcb->stack);
	list_remove(pcb->stack, tamanioStack - 1);
	liberarStack(stackActual);
	if (tamanioStack == 1) {
		char* mensaje = string_new();
		pcb->stack = list_create();
		char* statusChar = toStringInt(status);
		string_append(&mensaje, statusChar);
		free(statusChar);
		char* pidChar = toStringInt(pcb->id);
		string_append(&mensaje, pidChar);
		free(pidChar);
		log_info(LOGGER, "ENVIANDO AL KERNEL %s", mensaje);
		enviarMensajeConProtocolo(kernel, mensaje, 1);
		free(mensaje);
		finalizado = 1;
		log_info(LOGGER, "PID enviado al kernel sin problemas \n");
	}
}

void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar) {
	log_info(LOGGER, "Llamada con retorno a: %s", etiqueta);

	Stack* stack = malloc(sizeof(Stack));
	stack->retVar = donde_retornar;
	stack->retPos = pcb->pc;
	stack->vars = list_create();
	stack->args = list_create();

	pcb->pc = metadata_buscar_etiqueta(etiqueta, pcb->indices->etiquetas,
			pcb->indices->etiquetas_size);
	log_info(LOGGER, "Salto a: %d\n", pcb->pc);
	pcb->pc--;

	list_add(pcb->stack, stack);
}

void llamarSinRetorno(t_nombre_etiqueta etiqueta) {
	log_info(LOGGER, "Llamada sin retorno a: %s", etiqueta);
	Stack* stack = malloc(sizeof(Stack));
	stack->retPos = pcb->pc;
	stack->vars = list_create();
	stack->args = list_create();

	pcb->pc = metadata_buscar_etiqueta(etiqueta, pcb->indices->etiquetas,
			pcb->indices->etiquetas_size);
	log_info(LOGGER, "Salto a: %d\n", pcb->pc);
	pcb->pc--;

	list_add(pcb->stack, stack);
}

void retornar(t_valor_variable retorno) {
	Stack* stackActual = obtenerStack();
	int puntero = stackActual->retVar;
	asignar(puntero, retorno);
}

void irAlLabel(t_nombre_etiqueta etiqueta) {
	log_info(LOGGER, "Ir a Label: %s", etiqueta);
	pcb->pc = metadata_buscar_etiqueta(etiqueta, pcb->indices->etiquetas,
			pcb->indices->etiquetas_size);
	log_info(LOGGER, "Salto a: %d\n", pcb->pc);
	pcb->pc--;
}

void wait(t_nombre_semaforo identificador_semaforo) {
	log_info(LOGGER, "Wait al semaforo: %s", identificador_semaforo);
	char* mensaje = string_new();
	string_append(&mensaje, identificador_semaforo);
	enviarMensajeConProtocolo(kernel, mensaje, 4);
	free(mensaje);

	char* respuestaChar = esperarMensaje(kernel);
	int respuesta = atoi(respuestaChar);
	free(respuestaChar);

	if (respuesta) {
		log_info(LOGGER, "Wait OK, sin problemas!\n");
	} else {
		log_info(LOGGER, "Semaforo bloqueante!\n");
		pcb->pc++;
		char* mensaje = string_new();
		char* char_pcb = toStringPCB(pcb);
		char* statusChar = toStringInt(status);
		string_append(&mensaje, statusChar);
		string_append(&mensaje, char_pcb);
		enviarMensaje(kernel, mensaje);
		log_info(LOGGER, "\n\nLe mande al kernel el PCB: %s \n\n", char_pcb);
		free(mensaje);
		free(statusChar);
		free(char_pcb);
		finalizado = -1;
	}
}

void signalAnsi(t_nombre_semaforo identificador_semaforo) {
	log_info(LOGGER, "Signal al semaforo: %s\n", identificador_semaforo);
	char* mensaje = string_new();
	string_append(&mensaje, identificador_semaforo);
	enviarMensajeConProtocolo(kernel, mensaje, 6);
	free(mensaje);
	int verificador = atoi(esperarMensaje(kernel));
	log_info(LOGGER, "Recibi del kernel %d\n", verificador);
	if (verificador != 1) {
		//IS_EXECUTING = 0;
		log_error(LOGGER, "Error: Error de conexion con el KERNEL \n");
		log_error(LOGGER,
				"Error: Algo fallo al enviar el mensaje para realizar un signal, recibi: %d \n",
				verificador);
	}
}

t_puntero reservar(t_valor_variable espacio) {
	log_info(LOGGER, "reservar %d cantidad de espacio en memoria", espacio);
	char* mensaje = string_new();
	char* espacioString = toStringInt(espacio);
	char* pidString = toStringInt(pcb->id);
	string_append(&mensaje, pidString);
	string_append(&mensaje, espacioString);
	enviarMensajeConProtocolo(kernel, mensaje, 7);
	free(mensaje);
	free(pidString);
	free(espacioString);
	char* respuesta = esperarMensaje(kernel);
	log_info(LOGGER, "el puntero a donde inicia la memoria reservada es: %s",
			respuesta);
	int puntero = atoi(respuesta);
	if (puntero == 0) {
		finalizado = -1;
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
	}
	free(respuesta);
	return puntero;
}

void liberar(t_puntero puntero) {
	log_info(LOGGER, "liberar la memoria del siguiente puntero: %d", puntero);
	char* mensaje = string_new();
	int pagina = puntero / TAMANIO_PAGINA;
	int offset = puntero % TAMANIO_PAGINA;
	char* pidString = toStringInt(pcb->id);
	char* paginaString = toStringInt(pagina);
	char* offsetString = toStringInt(offset);
	string_append(&mensaje, pidString);
	string_append(&mensaje, paginaString);
	string_append(&mensaje, offsetString);
	log_info(LOGGER, "Enviando al Kernel : %s", mensaje);
	enviarMensajeConProtocolo(kernel, mensaje, 8);
	char* respuesta = esperarMensaje(kernel);
	log_info(LOGGER, "El Kernel respondio: %s", respuesta);
	free(respuesta);
	free(mensaje);
	free(pidString);
	free(paginaString);
	free(offsetString);
}

t_descriptor_archivo abrir(t_direccion_archivo direccion, t_banderas flags) {
	log_info(LOGGER,
			"abrir archivo %s con flags lectura %d, escritura %d, creacion %d",
			direccion, flags.lectura, flags.escritura, flags.creacion);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* lecturaString = toStringInt(flags.lectura);
	char* escrituraString = toStringInt(flags.escritura);
	char* creacionString = toStringInt(flags.creacion);
	string_append(&mensaje, pidString);
	string_append(&mensaje, lecturaString);
	string_append(&mensaje, escrituraString);
	string_append(&mensaje, creacionString);
	string_append(&mensaje, direccion);
	enviarMensajeConProtocolo(kernel, mensaje, 11);
	free(mensaje);
	free(lecturaString);
	free(escrituraString);
	free(creacionString);
	free(pidString);
	char* respuestaChar = esperarMensaje(kernel);
	int respuesta = atoi(respuestaChar);
	free(respuestaChar);
	if (!respuesta) {
		log_error(LOGGER, "Error al abrir el archivo %s, cerrando script",
				direccion);
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
		finalizado = -1;
	}
	return respuesta;
}

void borrar(t_descriptor_archivo descriptor_archivo) {
	log_info(LOGGER, "borrar el archivo de descriptor: %d", descriptor_archivo);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* descriptorString = toStringInt(descriptor_archivo);
	string_append(&mensaje, pidString);
	string_append(&mensaje, descriptorString);
	enviarMensajeConProtocolo(kernel, mensaje, 12);
	free(mensaje);
	free(descriptorString);
	free(pidString);
	char* respuestaChar = esperarMensaje(kernel);
	int respuesta = atoi(respuestaChar);
	free(respuestaChar);
	if (!respuesta) {
		log_error(LOGGER, "Error al borar el archivo %d, cerrando script",
				descriptor_archivo);
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
		finalizado = -1;
	}
}

void cerrar(t_descriptor_archivo descriptor_archivo) {
	log_info(LOGGER, "cerrar el archivo de descriptor: %d", descriptor_archivo);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* descriptorString = toStringInt(descriptor_archivo);
	string_append(&mensaje, pidString);
	string_append(&mensaje, descriptorString);
	enviarMensajeConProtocolo(kernel, mensaje, 13);
	free(mensaje);
	free(pidString);
	free(descriptorString);
	char* respuestaChar = esperarMensaje(kernel);
	int respuesta = atoi(respuestaChar);
	free(respuestaChar);
	if (!respuesta) {
		log_error(LOGGER, "Error al cerrar el archivo %d, cerrando script",
				descriptor_archivo);
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
		finalizado = -1;
	}
}

void moverCursor(t_descriptor_archivo descriptor_archivo,
		t_valor_variable posicion) {
	log_info(LOGGER, "mover cursor del descriptor %d a la posicion %d",
			descriptor_archivo, posicion);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* descriptorString = toStringInt(descriptor_archivo);
	char* posicionString = toStringInt(posicion);
	string_append(&mensaje, pidString);
	string_append(&mensaje, descriptorString);
	string_append(&mensaje, posicionString);

	enviarMensajeConProtocolo(kernel, mensaje, 14);
	char* respuesta = esperarMensaje(kernel);

	log_info(LOGGER, "Kernel respondio %s", respuesta);
	free(respuesta);
	free(pidString);
	free(mensaje);
	free(descriptorString);
	free(posicionString);
}

void escribir(t_descriptor_archivo descriptor_archivo, void* informacion,
		t_valor_variable tamanio) {
	log_info(LOGGER, "escribir %d bytes descriptor %d : %s", tamanio,
			descriptor_archivo, informacion);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* descriptorString = toStringInt(descriptor_archivo);
	char* tamanioChar = toStringInt(tamanio);
	string_append(&mensaje, pidString);
	string_append(&mensaje, descriptorString);
	string_append(&mensaje, tamanioChar);
	enviarMensajeConProtocolo(kernel, mensaje, 15);
	esperarConfirmacion(kernel);
	send(kernel, informacion, tamanio, 0);

	free(mensaje);
	free(descriptorString);
	free(pidString);
	free(tamanioChar);

	char* respuesta = esperarMensaje(kernel);
	int confirmacion = atoi(respuesta);

	if (!confirmacion) {
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
		finalizado = -1;
	}
}

void leer(t_descriptor_archivo descriptor_archivo, t_puntero informacion,
		t_valor_variable tamanio) {
	log_info(LOGGER, "leer %d bytes descriptor %d", tamanio,
			descriptor_archivo);
	char* mensaje = string_new();
	char* pidString = toStringInt(pcb->id);
	char* descriptorString = toStringInt(descriptor_archivo);
	char* tamanioString = toStringInt(tamanio);
	string_append(&mensaje, pidString);
	string_append(&mensaje, descriptorString);
	string_append(&mensaje, tamanioString);
	free(tamanioString);
	free(descriptorString);
	free(pidString);
	enviarMensajeConProtocolo(kernel, mensaje, 16);
	free(mensaje);
	char* respuesta = esperarMensaje(kernel);
	log_info(LOGGER, "lei: %s", respuesta);
	char* confimacionChar = toSubString(respuesta, 0, 3);
	int confimarcion = atoi(confimacionChar);
	free(confimacionChar);
	if (confimarcion) {
		send(kernel, &kernel, sizeof(int), 0);
		void* buffer = malloc(tamanio);
		recv(kernel, buffer, tamanio, 0);
		int page = informacion / TAMANIO_PAGINA;
		int offset = informacion % TAMANIO_PAGINA;
		executeMemoryAction(memoria,FUNC_SAVE_BYTES, buffer, pcb->id, page, offset, tamanio);
		free(buffer);
	}
	else {
		char* statusChar = toStringInt(status);
		enviarMensaje(kernel, statusChar);
		free(statusChar);
		finalizado = -1;
	}
	free(respuesta);
}

/////// FUNCIONES AUXILIARES ///////////////

void enviarMensajeKernelConsulta(char* variable) {
	char* mensaje = string_new();
	string_append(&mensaje, "00020001");
	char* tamVariable = toStringInt(strlen(variable));
	string_append(&mensaje, tamVariable);
	free(tamVariable);
	string_append(&mensaje, variable);
	string_append(&mensaje, "\0");
	send(kernel, mensaje, strlen(mensaje), 0);
	free(mensaje);
}

void enviarMensajeMemoriaAsignacion(int pag, int off, int size, int proceso,
		int valor) {
	executeMemoryAction(memoria, FUNC_SAVE_BYTES, &valor, proceso, pag, off,
			size);
}

void enviarMensajeMemoriaConsulta(int pag, int off, int size, int proceso) {
	char* mensaje = string_new();
	char* procesoMje = toStringInt(proceso);
	char* pagMje = toStringInt(pag);
	char* offMje = toStringInt(off);
	char* sizeMje = toStringInt(size);
	string_append(&mensaje, "2");
	string_append(&mensaje, procesoMje);
	string_append(&mensaje, pagMje);
	string_append(&mensaje, offMje);
	string_append(&mensaje, sizeMje);
	string_append(&mensaje, "\0");
	log_debug(LOGGER, "Le envio a la memoria: %s", mensaje);
	send(memoria, mensaje, string_length(mensaje), 0);
	free(procesoMje);
	free(pagMje);
	free(offMje);
	free(sizeMje);
	free(mensaje);
}

#endif


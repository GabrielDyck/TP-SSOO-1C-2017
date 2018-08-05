#ifndef SRC_OBJ_CAPADEMEMORIA_H_
#define SRC_OBJ_CAPADEMEMORIA_H_

#include "../Kernel.h"

#define TAMANIO_DE_BLOQUE 5

void inicializarCapaDeMemoria() {
	paginas = list_create();
}

char* alocarMemoria(int cpu, int pid, int sizeReal) {
	int size = sizeReal - TAMANIO_DE_BLOQUE;
	int intentos = 1;
	logInfo("(MAIN) Buscando Pagina ...");
	while (1) {
		struct_pagina* pagina = buscarPagina(pid, sizeReal, intentos);

		if (pagina != NULL) {
			int  offsetReal = alocarEnPagina(pid, pagina->pagina, size, cpu);
			if (!offsetReal)
				intentos ++;
			else  {
				log_info(archivoLog, "(MAIN) Espacio conseguido pagina : %d, offset %d", pagina->pagina, offsetReal);
				pagina->free -= sizeReal;
				int offsetTotal = tamanioPagina*pagina->pagina + offsetReal;
				struct_proceso* proceso = getProceso(pid);
				proceso->cantidadDeAlocamientos ++ ;
				proceso->cantidadDeAlocamientosSize += size;
				return toStringLong(offsetTotal);
			}
		}
		else {
			logError("(MAIN) Se No se pudo conseguir pagina para el heap");
			return NULL;
		}
	}
}

struct_pagina* buscarPagina(int pid, int realSize, int intentos) {
	int i;
	for (i = 0; i < list_size(paginas); i++) {
		struct_pagina* pagina = list_get(paginas, i);
		if ((pagina->pid == pid) && (pagina->free >= realSize)) {
			intentos--;
			if (intentos <= 0)
				return pagina;
		}
	}

	int numeroPagina = nuevaPagina(pid);

	if (numeroPagina == 0) {
		return NULL;
	} else {
		crearBloqueEnMemoria(pid, numeroPagina, 0 , tamanioPagina - 5, 0);

		struct_pagina* pagina = malloc(sizeof(struct_pagina));
		pagina->free = tamanioPagina - TAMANIO_DE_BLOQUE;
		pagina->pagina = numeroPagina;
		pagina->pid = pid;
		list_add(paginas, pagina);

		return pagina;
	}
}

void crearBloqueEnMemoria(int pid, int pagina, int offset, int tamanio, int ocupado ) {
	char* mensaje = string_new();
	char* tamanioChar = toStringInt(tamanio);
	string_append(&mensaje, tamanioChar);
	if (ocupado)
		string_append(&mensaje, "1");
	else
		string_append(&mensaje, "0");
	escribirMemoria(pid, pagina,offset, mensaje);
	free(tamanioChar);
	free(mensaje);
}

int getTamanio(int pid, int pagina, int offset) {
	return leerMemoria(pid, pagina, offset, 4);
}

int getEstado(int pid, int pagina, int offset) {
	return leerMemoria(pid, pagina, offset + 4, 1);
}

void fragmentar(int pid, int pagina) {
	int offset = 0;
	while(offset < tamanioPagina) {
		int tamanio = getTamanio(pid, pagina, offset);
		int tamanioReal = tamanio + TAMANIO_DE_BLOQUE;
		int offsetSiguiente = offset + tamanioReal;

		if (offsetSiguiente >= tamanioPagina)
			return;

		int estado = getEstado(pid, pagina, offset);
		int estadoSiguiente = getEstado(pid, pagina, offsetSiguiente);

		if (!estado && !estadoSiguiente) {
			int tamanioSiguiente = getTamanio(pid, pagina, offsetSiguiente);
			crearBloqueEnMemoria(pid, pagina, offset, tamanioReal + tamanioSiguiente , 0);
		} else
			offset = offsetSiguiente;
	}
}

int alocarEnPagina(int pid, int pagina, int size, int cpu) {
	fragmentar(pid, pagina);
	int sizeReal = size + TAMANIO_DE_BLOQUE;
	int offset = 0;
	while(offset + sizeReal <= tamanioPagina) {
		int tamanio = getTamanio(pid, pagina, offset);
		int tamanioReal = tamanio + TAMANIO_DE_BLOQUE;
		int estado = getEstado(pid, pagina, offset);
		if (estado || (tamanioReal < sizeReal))
			offset += tamanioReal;
		else {
			int diff = tamanio - size;
			if (diff < TAMANIO_DE_BLOQUE) {
				size += diff;
				sizeReal += diff;
			} else {
				crearBloqueEnMemoria(pid, pagina, offset + sizeReal, diff, 0);
			}
			crearBloqueEnMemoria(pid, pagina, offset, size, 1);
			return offset + TAMANIO_DE_BLOQUE;
		}
	}
	return 0;
}

void liberarDelHeap(int pid, int numeroPagina, int offset) {
	int i;
	for (i = 0; i < list_size(paginas); i++) {
		struct_pagina* pagina = (struct_pagina*) list_get(paginas, i);
		if ((pagina->pagina == numeroPagina) && (pagina->pid == pid)) {
			int tamanio = getTamanio(pid, pagina->pagina, offset - TAMANIO_DE_BLOQUE);
			log_info(archivoLog, "(MAIN) Liberando %d bytes", tamanio);
			pagina->free += tamanio + TAMANIO_DE_BLOQUE;
			if (pagina->free == tamanioPagina) {
				list_remove(paginas, i);
				liberarPagina(pagina->pagina, pagina->pid);
				free(pagina);
			} else
				crearBloqueEnMemoria(pid, numeroPagina, offset - TAMANIO_DE_BLOQUE, tamanio, 0);
			struct_proceso* proceso = getProceso(pid);
			proceso->cantidadDeLiberaciones ++ ;
			proceso->cantidadDeLiberacionesSize += tamanio;
			return;
		}
	}
}


#endif

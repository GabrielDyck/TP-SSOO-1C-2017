#include "json.h"

char* toStringInstrucciones(t_intructions* instrucciones, t_size tamanio){
	char* char_instrucciones=string_new();
	string_append(&char_instrucciones, "\0");
	int i;
	for (i = 0 ; i< tamanio; i++){
		char* start=toStringInt(instrucciones->start);
		char* offset=toStringInt(instrucciones->offset);
		string_append(&char_instrucciones, start);
		string_append(&char_instrucciones, offset);
		free(start);
		free(offset);
		instrucciones++;
	}
	return char_instrucciones;
}

t_intructions* fromStringInstrucciones(char* char_instrucciones, t_size tamanio){
	t_intructions* instrucciones = malloc(tamanio*sizeof(t_intructions));
	int i;
	char* start;
	char* offset;
	for (i=0;i<tamanio;i++){
		start=string_substring(char_instrucciones,i*8,4);
		instrucciones[i].start = atoi(start);
		free(start);
		offset=string_substring(char_instrucciones,i*8+4,4);
		instrucciones[i].offset = atoi(offset);
		free(offset);
	}
	return instrucciones;
}

char* toStringMetadata(t_metadata_program meta){
	char* char_meta=string_new();
	char* inicio=toStringInt(meta.instruccion_inicio);
	char* instrSize=toStringInt(meta.instrucciones_size);
	char* etiqSize=toStringInt(meta.etiquetas_size);
	char* cantFunc=toStringInt(meta.cantidad_de_funciones);
	char* cantEtiq=toStringInt(meta.cantidad_de_etiquetas);
	string_append(&char_meta,inicio);
	string_append(&char_meta,instrSize);
	string_append(&char_meta,etiqSize);
	string_append(&char_meta,cantFunc);
	string_append(&char_meta,cantEtiq);
	free(inicio);
	free(instrSize);
	free(etiqSize);
	free(cantFunc);
	free(cantEtiq);
	int i;
	for(i=0;i<meta.etiquetas_size;i++){
		if(meta.etiquetas[i]=='\0'){
			meta.etiquetas[i]='@';
		}
	}
	if(meta.etiquetas_size){
		meta.etiquetas[meta.etiquetas_size]='\0';
		string_append(&char_meta,meta.etiquetas);
	}
	char* char_instrucciones=toStringInstrucciones(meta.instrucciones_serializado,meta.instrucciones_size);
	string_append(&char_meta, char_instrucciones);
	free(char_instrucciones);
	return char_meta;
}


t_metadata_program* fromStringMetadata(char* char_meta){
	t_metadata_program* meta = malloc(sizeof(t_metadata_program));

	char* inicio = toSubString(char_meta,0,3);
	meta->instruccion_inicio = atoi(inicio);
	free(inicio);
	char* instrSize=toSubString(char_meta,4,7);
	meta->instrucciones_size = atoi(instrSize);
	free(instrSize);
	char* etiquetasSize=toSubString(char_meta,8,11);
	meta->etiquetas_size = atoi(etiquetasSize);
	free(etiquetasSize);
	char* cantFunciones=toSubString(char_meta,12,15);
	meta->cantidad_de_funciones = atoi(cantFunciones);
	free(cantFunciones);
	char* cantEtiquetas=toSubString(char_meta,16,19);
	meta->cantidad_de_etiquetas = atoi(cantEtiquetas);
	free(cantEtiquetas);
	int puntero = 20;
	int i;
	meta->etiquetas = toSubString(char_meta,puntero,(puntero+meta->etiquetas_size-1));
	for(i=0;i<meta->etiquetas_size;i++){
		if(meta->etiquetas[i]=='@'){
			meta->etiquetas[i]='\0';
		}
	}
	puntero += meta->etiquetas_size;
	char * char_instrucciones = string_substring_from(char_meta,puntero);
	meta->instrucciones_serializado = fromStringInstrucciones(char_instrucciones,meta->instrucciones_size);
	free(char_instrucciones);
	return meta;
}

char* toStringPCB(PCB* pcb){
	char* char_pcb=string_new();
	char* char_id;
	char_id = toStringInt(pcb->id);
	char *char_metadata;
	char_metadata = toStringMetadata(*pcb->indices);
	char* char_paginas_codigo;
	char_paginas_codigo = toStringInt(pcb->paginas_codigo);
	char* char_pc;
	char_pc = toStringInt(pcb->pc);
	char* char_stack;
	char_stack = toStringListStack(pcb->stack);
	string_append(&char_pcb,char_id);
	char* aux = toStringInt(strlen(char_metadata));
	string_append(&char_pcb, aux);
	string_append(&char_pcb,char_metadata);
	string_append(&char_pcb, char_paginas_codigo);
	string_append(&char_pcb, char_pc);
	string_append(&char_pcb,char_stack);
	free (char_id);
	free(aux);
	free(char_metadata);
	free(char_paginas_codigo);
	free(char_pc);
	free(char_stack);
	return char_pcb;
}
PCB* fromStringPCB(char* char_pcb){
	PCB* pcb = malloc(sizeof(PCB));
	char* char_id = toSubString(char_pcb,0,3);
	pcb->id = atoi(char_id);
	char* char_tam_meta= toSubString(char_pcb,4,7);
	int tamanioMeta = atoi(char_tam_meta);
	char* char_indices = toSubString(char_pcb,8,(7+tamanioMeta));
	pcb->indices = fromStringMetadata(char_indices);
	int puntero = 8+tamanioMeta;
	char* char_pags = toSubString(char_pcb,puntero,puntero+3);
	pcb->paginas_codigo = atoi(char_pags);
	puntero = puntero +4;
	char* char_pc =toSubString(char_pcb,puntero, puntero +3);
	pcb->pc = atoi(char_pc);
	puntero = puntero + 4;
	char *subString = string_substring_from(char_pcb,puntero);
	pcb->stack = fromStringListStack(subString);
	free(char_id);
	free(char_tam_meta);
	free(char_indices);
	free(char_pags);
	free(char_pc);
	free(subString);
	return pcb;
}

char* toSubString(char* string, int inicio, int fin){
	if (fin<inicio){
		char* r = string_new();
		string_append(&r,"\0");
		return r;
	}
	return (string_substring(string,inicio,1+fin-inicio));
}

char* toStringInt(int numero){
	char* longitud=string_new();
	char* num_char = string_itoa(numero);
	char* rev = string_reverse(num_char);
	string_append(&longitud,rev);
	free(num_char);
	free(rev);
	string_append(&longitud,"0000");
	longitud=string_substring(longitud,0,4);
	longitud=string_reverse(longitud);
	return longitud;
}

t_list* fromStringListStack(char* char_stack){
	int i;
	int indice = 0;
	int subIndice;
	t_list* lista_stack = list_create();
	Stack* stack;
	char* subString_stack;
	for(i=0; i<strlen(char_stack);i++){
		subIndice = indice;
		if (char_stack[i]=='-'){
			indice = i;
			subString_stack = toSubString(char_stack,subIndice,indice-1);
			stack = fromStringStack(subString_stack);
			list_add(lista_stack,stack);
			free(subString_stack);
			indice++;
		}
	}
	return lista_stack;
}

char* toStringListStack(t_list* lista_stack){
	int i;
	char* char_lista_stack = string_new();
	string_append(&char_lista_stack,"\0");
	Stack* stack;
	char barra[2] = "-";
	char* char_stack;
	for (i=0; i<list_size(lista_stack);i++){
		stack = list_get(lista_stack,i);
		char_stack = toStringStack(*stack);
		string_append(&char_lista_stack,char_stack);
		string_append(&char_lista_stack,barra);
		free(char_stack);
	}
	return char_lista_stack;
}

char* toStringStack(Stack stack){
	char* char_stack=string_new();
	char* char_args = toStringListVariables(stack.args);
	char* char_retpos = toStringInt(stack.retPos);
	char* char_ret_var = toStringLong(stack.retVar);
	char* char_var_list = toStringListVariables(stack.vars);
	char* tamanioListaPagina = toStringInt(strlen(char_args));
	string_append_with_format(&char_stack,"%s%s%s%s%s",tamanioListaPagina,char_args,char_retpos,char_ret_var,char_var_list);
	free(char_args);
	free(tamanioListaPagina);
	free(char_retpos);
	free(char_ret_var);
	free(char_var_list);
	return char_stack;
}

Stack* fromStringStack(char* char_stack){
	Stack* stack = malloc(sizeof(Stack));
	char* char_arg_size = toSubString(char_stack,0,3);
	int arg_size = atoi(char_arg_size);
	free(char_arg_size);
	char* char_args = toSubString(char_stack,4,3+arg_size);
	stack->args = fromStringListVariables(char_args);
	free(char_args);
	int puntero = 4+ arg_size;
	char* char_retPos = toSubString(char_stack,puntero,puntero +3);
	stack->retPos = atoi(char_retPos);
	free(char_retPos);
	puntero += 4;
	char* char_retVar = toSubString(char_stack, puntero,puntero +7);
	stack->retVar = atoi(char_retVar);
	free(char_retVar);
	puntero += 8;
	char* char_vars = string_substring_from(char_stack,puntero);
	stack->vars = fromStringListVariables(char_vars);
	free(char_vars);
	return stack;
}

char* toStringListVariables(t_list* lista){
	int i;
	char* char_lista_var = string_new();
	string_append(&char_lista_var,"\0");
	Variable* variable;
	char* char_var;
	for (i=0; i<list_size(lista);i++){
		variable = list_get(lista,i);
		char_var = toStringVariable(*variable);
		string_append(&char_lista_var,char_var);
		free(char_var);
	}
	return char_lista_var;
}

t_list* fromStringListVariables(char* char_list){
	int i;
	int variables_size = strlen(char_list)/9;
	t_list* lista_var = list_create();
	Variable* variable;
	char* subString;
	for(i=0; i< variables_size;i++){
		subString = toSubString(char_list,i*9,(i*9)+8);
		variable = fromStringVariable(subString);
		free(subString);
		list_add(lista_var,variable);
	}
	return lista_var;
}

char* toStringLong(int numero){
	char* longitud=string_new();
	char* num_char = string_itoa(numero);
	char* rev = string_reverse(num_char);
	string_append(&longitud,rev);
	free(num_char);
	free(rev);
	string_append(&longitud,"00000000");
	longitud=string_substring(longitud,0,8);
	longitud=string_reverse(longitud);
	return longitud;
}

char* toStringVariable(Variable variable){
	char* char_variable = string_new();
	char id[2];
	id[0] = variable.id;
	id[1] = '\0';
	char* char_offset = toStringLong(variable.offset);
	string_append(&char_variable,id);
	string_append(&char_variable,char_offset);
	free(char_offset);
	return char_variable;
}

Variable* fromStringVariable(char* char_variable){
	Variable* variable = malloc(sizeof(Variable));
	variable->id = char_variable[0];
	char* char_offset = toSubString(char_variable,1,8);
	variable->offset = atoi(char_offset);
	free(char_offset);
	return variable;
}

void liberarPCBPuntero(PCB* pcb){
	int i;
	Stack* stack;
	for (i=0; i<list_size(pcb->stack);i++){
		stack = list_get(pcb->stack,i);
		liberarStack(stack);
		free(stack);
	}
	list_destroy(pcb->stack);
	free(pcb);
}
void liberarPCB(PCB* pcb){
	int i;
	Stack* stack;
	for (i=0; i<list_size(pcb->stack);i++){
		stack = list_get(pcb->stack,i);
		liberarStack(stack);
		free(stack);
	}
	list_destroy(pcb->stack);
	free(pcb->indices);
	free(pcb);
}

void liberarStack(Stack* stack){
	int i;
	list_destroy(stack->args);
	Variable* var;
	for(i=0; i<list_size(stack->vars);i++){
		var = list_get(stack->vars,i);
		free(var);
	}
	list_destroy(stack->vars);
}

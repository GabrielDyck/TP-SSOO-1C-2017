#include "obj/Consola.h"
#include "obj/Cpu.h"
#include "obj/Memoria.h"
#include "obj/PlanificadorLargoPlazo.h"
#include "obj/PlanificadorCortoPlazo.h"
#include "obj/Proceso.h"
#include "obj/ConsolaKernel.h"
#include "obj/CapaDeMemoria.h"
#include "obj/FileSystem.h"

int main(int argc, char *argv[]) {

	crearLog();

	leerConfiguracion(argv[1]);

	if (!conectarAMemoria(CONF_DATA.puertoMemoria, CONF_DATA.ipMemoria))
		return -1;

	if (!conectarAFileSystem(CONF_DATA.puertoMemoria, CONF_DATA.ipMemoria))
		return -1;

	constructor();

	if (!levantarServidor())
		return -1;

	esperarConexiones();


	return 0;
}

void constructor() {
	pthread_mutex_init(&mutex_multiprogramacion, NULL);
	exits = queue_create();
	crearVariablesGlobales();
	crearSemaforosGlobales();
	iniciarSemaforos();

	inicializarCapaDeMemoria();
	inicializarFileSystem();
	iniciarlizarCPU();
	iniciarlizarConsolas();
	inicializarProceso();
	inicializarPlanificadorLargoPlazo();
	inicializarPlanificadorCortoPlazo();
	inicializarConsolaKernel();

	logInfo("(MAIN) constructor finalizado");
}

void iniciarSemaforos() {
	pthread_mutex_init(&mutex_bloqueado, NULL);
	pthread_mutex_init(&mutex_exits, NULL);
}

void crearLog() {
	archivoLog = log_create("Kernel.log", "Kernel", 0,
	log_level_from_string("DEBUG"));
	logInfo("(MAIN) Inicio del Programa");
}

 char* getFullPath(char * path) {
	char cwd[200];
	if (getcwd(cwd, 200) == NULL) log_error(archivoLog, "(MAIN) Error en Full Path");

	char* fullPath = string_new();
	string_append(&fullPath, cwd);
	string_append(&fullPath, "/config/");
	string_append(&fullPath, path);
	string_append(&fullPath, ".properties");

	return fullPath;
}

void leerConfiguracion(char* path) {

	char* fullPath = getFullPath(path);

	log_info(archivoLog, "Leyendo el archivo de configuracion...: |%s| ", fullPath);

	t_config *configFile = config_create(fullPath);

	CONF_DATA.puertoProg = config_get_int_value(configFile, "PUERTO_PROG");

	CONF_DATA.ipMemoria = string_duplicate(config_get_string_value(configFile, "IP_MEMORIA"));
	CONF_DATA.puertoMemoria = config_get_int_value(configFile,
			"PUERTO_MEMORIA");

	CONF_DATA.puertoCpu = config_get_int_value(configFile, "PUERTO_CPU");

	CONF_DATA.ipFs = string_duplicate(config_get_string_value(configFile, "IP_FS"));

	CONF_DATA.puertoFs = config_get_int_value(configFile, "PUERTO_FS");

	CONF_DATA.quantum = config_get_int_value(configFile, "QUANTUM");

	CONF_DATA.quantumSleep = config_get_int_value(configFile, "QUANTUM_SLEEP");

	CONF_DATA.algoritmo = string_duplicate(config_get_string_value(configFile, "ALGORITMO"));

	CONF_DATA.gradoMultiprog = config_get_int_value(configFile,
			"GRADO_MULTIPROG");

	CONF_DATA.semIds = config_get_array_value(configFile, "SEM_IDS");

	CONF_DATA.semInit = config_get_array_value(configFile, "SEM_INIT");

	CONF_DATA.sharedVars = config_get_array_value(configFile, "SHARED_VARS");

	CONF_DATA.stackSize = config_get_int_value(configFile, "STACK_SIZE");

	logInfo("(MAIN) Ok");

	config_destroy(configFile);

	free(fullPath);
}

void crearVariablesGlobales() {
	variablesGlobales = list_create();
	int i = 0;
	while (CONF_DATA.sharedVars[i] != NULL) {
		struct_variable_global* variable = malloc(sizeof(struct_variable_global));
		variable->valor = 0;
		variable->nombre = CONF_DATA.sharedVars[i];
		list_add(variablesGlobales, variable);
		i++;
	}
}

void crearSemaforosGlobales() {
	semaforosGlobales = list_create();
	int i = 0;
	while (CONF_DATA.semIds[i] != NULL) {
		struct_semaforo_global* semaforo = malloc(
				sizeof(struct_semaforo_global));
		int valor = atoi(CONF_DATA.semInit[i]);
		semaforo->valor = valor;
		free(CONF_DATA.semInit[i]);
		semaforo->nombre = CONF_DATA.semIds[i];
		semaforo->cola = queue_create();
		list_add(semaforosGlobales, semaforo);
		i++;
	}
}

int levantarServidor() {
	logInfo("(MAIN) Creando el Servidor ...");

	socketServer = crearServidor(CONF_DATA.puertoProg);

	if (socketServer < 0) {
		logError("(MAIN) No se pudo levantar el servidor");
		return 0;
	}
	log_info(archivoLog, "(MAIN) Servidor creado (%d) ", socketServer);
	return 1;
}

void esperarConexiones() {

	int maxDescriptor;
	int clientSocket;

	while (1) {

		logInfo("(MAIN) Esperando Conexiones ... ");

		FD_ZERO(&descriptores);
		FD_SET(socketServer, &descriptores);

		maxDescriptor = socketServer;

		maximoDescriptorConsolas(&maxDescriptor);
		maximoDescriptorCpus(&maxDescriptor);

		if (select(maxDescriptor + 1, &descriptores, NULL, NULL, NULL) < 0) {
			logError("(MAIN) Error en el select");
			return;
		}

		revisarActividadConsolas();
		revisarActividadCpus();

		if (FD_ISSET(memoria, &descriptores)) {
			log_error(archivoLog, "(MAIN) La Memoria me esta hablando: %d",
					esperarConfirmacion(memoria));
		}

		if (FD_ISSET(fileSystem, &descriptores)) {
			log_error(archivoLog, "(MAIN) El fileSystem me esta hablando: %d",
					esperarConfirmacion(fileSystem));
		}

		if (FD_ISSET(socketServer, &descriptores)) {
			clientSocket = aceptar(socketServer);
			if (clientSocket < 0) {
				logError("(MAIN) Fallo la conexion con el nuevo cliente");
			} else {
				switch (getClientType(clientSocket)) {
				case CONSOLA:
					log_info(archivoLog, "Consola conectada : %d", clientSocket);
					agregarConsola(clientSocket);
					logInfo("(MAIN) Consola agregada");
					break;
				case CPU:
					log_info(archivoLog, "CPU conectada : %d", clientSocket);
					agregarCPU(clientSocket);
					logInfo("(MAIN) CPU agregado");

					break;
				}
			}
		}
	}
}

int getClientType(int socket) {
	uint32_t code;
	int recvLenght = recv(socket, &code, sizeof(code), 0);
	if (recvLenght <= 0) {
		return -1;
	}

	return code;
}

void agregarAExit(struct_proceso* proceso) {
	pthread_mutex_lock(&mutex_multiprogramacion);
	log_info(archivoLog, "(MAIN) Nivel multiprogramacion en  : %d", CONF_DATA.gradoMultiprog);
	log_info(archivoLog, "(MAIN) Cantidad en ejecucion  : %d", cantidadEnEjecucion);

	cantidadEnEjecucion-- ;
	if (cantidadEnEjecucion < CONF_DATA.gradoMultiprog) {
		sem_post(&sem_multiprogramacion);
	}
	pthread_mutex_unlock(&mutex_multiprogramacion);

	pthread_mutex_lock(&mutex_exits);
	queue_push(exits, proceso);
	pthread_mutex_unlock(&mutex_exits);
}

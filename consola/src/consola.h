#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/temporal.h>
#include <sys/timeb.h>
#include <commons/collections/list.h>
#include "./Funciones/sockets.h"

#define BACKLOG 5

#define CLIENT_TYPE_KERNEL 1
#define CLIENT_TYPE_CPU 2
#define MAX_LINE_SIZE 200
#define TOTAL_COMMANDS 5
#define MAX_ARGUMENTS 4
#define APP_NAME "soprano"
#define APP 0
#define COMMAND 1
char CURRENT_WORK_DIRECTORY[MAX_LINE_SIZE];

// Ids para funciones de input del user
#define INICIAR_PROGRAMA 0
#define FINALIZAR_PROGRAMA 1
#define DESCONECTAR_CONSOLA 2
#define LIMPIAR_MENSAJES 3
#define LISTAR 4


#define ID_CONSOLA 1

#define ERROR_CONNECT -3
#define ERROR_BIND -2
#define ERROR_LISTEN -1

#define TERMINATE "TERMINATE"

// Protocolos para recibir del kernel
#define PID_EXIT_SUCCESS 0
#define PID_EXIT_RECHAZADO 1
#define PID_EXIT_ARCHIVO_NO_EXISTENTE 2
#define PID_EXIT_LEER_ARCHIVO_SIN_PERMISOS 3
#define PID_EXIT_ERROR_DE_MEMORIA 5
#define PID_EXIT_ALOCAR_MAS_DE_UNA_PAGINA 8
#define PID_EXIT_NO_HAY_MAS_PAGINAS 9
#define PID_EXIT_ERROR_SIN_DEFINICION 20
#define PID_MESSAGE 6

// Nuevos protocolos by gian
#define FINALIZO_OKEY 0
#define RECHAZAR_CONSOLA 1
#define ARCHIVO_NO_EXISTENTE 2
#define LEER_ARCHIVO_SIN_PERMISOS 3
#define ESCRIBIR_ARCHIV_SIN_PERMISOS 4
#define ERROR_DE_MEMORIA 5
#define ALOCAR_MAS_DE_UNA_PAGINA 8
#define NO_HAY_MAS_PAGINAS 9
#define ERROR_SIN_DEFINICION 20
#define PID_EXPULSADO 21
#define PRINT 6

typedef struct {
	int pid;
	int socketLocal;
	pthread_t hilo;
	struct tm *horaInicio;
	struct timeb msInicio;
	uint impresiones;
} new_process;

void eliminarPid(new_process * process);
void kernelMessagesManager(int *s);
void commandsManager(int *s);
void config(int argc, char *argv[]);
void cpuManager(int *s);
int logRespuestaSocketF(int socket);
int get_line(char line[], int maxline);
void iniciarPrograma(char *argument);
void finalizarPrograma(char *argument);
int conectarKernelConHandshake();
int desiredSocket(int socket, int socketWanted);
int recvInt(int socket);
void listar();
void desconectarConsola();
int init();
void initializeLogger();
int readConfigFile(char *path);
unsigned countWords(char *str);
int commandParser(char **parameters, int *args);
void limpiarMensajes();
char *get_time_diff(new_process * process);
char *get_hora_inicio(new_process * process);
void guardarTiempo(new_process *process);

char *acceptedCommands[TOTAL_COMMANDS] = {
		"iniciar",
		"finalizar",
		"desconectar",
		"limpiar",
		"listar"
};



t_list *openSockets;


void processThread(new_process *);

typedef struct {
	int puerto;
	char *ip_kernel;
} t_conf;

t_log *LOGGER;
t_conf CONF_DATA;


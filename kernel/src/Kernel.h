#ifndef KERNEL_H_
#define KERNEL_H_

#include <stddef.h>
#include <commons/config.h>
#include <commons/log.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/select.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <parser/metadata_program.h>

#include "../Funciones/sockets.h"
#include "../Funciones/json.h"

//////////////////////////CONSOLA///////////////////////////
typedef struct {
	int socket;
	char * codigo;
	int pid;
} struct_consola;

pthread_mutex_t mutex_consolas;
t_list * consolas;

t_list* abortarProgramas;
pthread_mutex_t mutex_abortar_programas;


void iniciarlizarConsolas();
struct_consola* newConsola(char* codigo, int socket);
void freeConsola(int consolas);
void maximoDescriptorConsolas(int* maximo);
void revisarActividadConsolas();
int filtrarDesconeccionConsola(int pid);
void desconectarConsola(int consola);
void rechazarConsola(int socket);
void iniciarPrograma(int consola);
void finalizarPrograma(int pid);
void tamanioDeAlocamientoIncorrecto(int pid);
void finalizarPrograma(int pid);
void muerteCPU(int pid);
void abortarPid(int pid);

void noSePuedoCrearElArhivo(int pid);
void errorSinDefinicion(int pid);
void leerSinPermisos(int pid);
void escribirSinPermisos(int pid);
void imprimirEnConsola(int pid, char* dato);
/////////////////////////////////////////////////////////////
/////////////////////////CPU/////////////////////////////////

typedef struct {
	int socket;
	int disponible;
}struct_cpu;

typedef struct {
	int valor;
	char* nombre;
}struct_variable_global;

typedef struct {
	int valor;
	char* nombre;
	t_queue * cola;
}struct_semaforo_global;

t_list * cpus;
sem_t sem_cpu;
pthread_mutex_t mutex_cpus;
pthread_mutex_t mutex_mensaje_cpu;

void finDeQuantum(int cpu);
void finScript(int cpu);
void waitSemaforo(int cpu);
void signalSemaforo(int cpu);
void reservarMemoria(int cpu);
void liberarMemoriaReservada(int cpu);

struct_cpu* buscarCpu(int socket);
void removerCPU(struct_cpu* cpu);

void setValorVariableGlobal(t_list* variables, char* nombre, int valor);
int obtenerValorVariableGlobal(t_list* varcpuiables, char* nombre);
void enviarVariableGlobal(int cpu);
void recibirVariableGlobal(int cpu);

struct_semaforo_global* obtenerSemaforo(char* nombreSemaforo);
void bloquear(char* pcb, struct_semaforo_global* semaforo);
void wait(char* nombreSemaforo, int cpu);
void signalSemaforo(int cpu);
void signal(char* nombreSemaforo);
/////////////////////////////////////////////////////////////
///////////////////////////MEMORIA///////////////////////////
uint32_t SUCCESS = 1;

int memoria;
int tamanioPagina;

void escribirMemoria(int pid, int pagina, int offset, char* dato);
int leerMemoria(int pid, int pagina, int offset, int size);
int conectarAMemoria(int puerto, char* ip);
int cantidadDePaginas(char* codigo);
int reservarEnMemoria(uint32_t id, uint32_t cantPaginas);
void insertarEnMemoria(char* codigo, uint32_t id);
void liberarRecursos(int id);
int nuevaPagina(int pid);
void liberarPagina(int pagina, int pid);
/////////////////////////////////////////////////////////////
/////////////////////////////////PROCESO/////////////////////
typedef struct {
	int pid;
	int socket;
	int cantidadDeAlocamientos;
	int cantidadDeAlocamientosSize;
	int cantidadDeLiberaciones;
	int cantidadDeLiberacionesSize;
	int cantidadDeRafagas;
	int cantidadDeSysCalls;
	int cpu;
	int exitCode;
} struct_proceso;

t_list* procesos;

void inicializarProceso();
struct_proceso* getProceso(int pid);
int newProceso(int socket);
void aumentarsysCalls(int cpu);
void eliminarCPUDelProceso(int cpu);
/////////////////////////////////////////////////////////////
/////////////////////PLANIFICADOR LARGO PLAZO////////////////
typedef struct {
	int pid;
	char* codigo;
} struct_new;

sem_t sem_multiprogramacion;
pthread_mutex_t mutex_multiprogramacion;

sem_t sem_news;
t_queue * news;
pthread_mutex_t mutex_news;

pthread_t planificadorLargoPlazo_h;

int cantidadEnEjecucion = 0;

void inicializarPlanificadorLargoPlazo();
void planificadorLargoPlazo();
char * createPCB(struct_new* new);
/////////////////////////////////////////////////////////////
///////////////////PLANIFICADOR CORTO PLAZO//////////////////
pthread_t planificadorCortoPlazo_h;
sem_t sem_ready;
t_queue * ready;
t_list* exec;

pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_exec;

void inicializarPlanificadorCortoPlazo();
void planificadorCortoPlazo();
void agregarAReady(char* pcb);
void removerDeEjecutados(int pid);
/////////////////////////////////////////////////////////////
///////////////CONSOLA-KERNEL////////////////////////////////

pthread_t consolaKernel_h;

pthread_mutex_t mutex_planificacion;

void inicializarConsolaKernel();

void crearConsolaKernel();

void listarProcesos(int valor);

void listarNews();
void listarEnEjecucion();
char* escucharComando();
void listarEnReady();
void listarEnBloqueado();
void listarEnExits();

void modificarMultiprogramacion(int valor);
/////////////////////////////////////////////////////////////
//////////////////FILE SYSTEM///////////////////////////////
typedef struct {
	char * file;
	int open;
} struct_global_archivos;

typedef struct {
	int fileDescriptor;
	char * flag;
	struct_global_archivos* fileDescriptorGlobal;
	int cursor;
} struct_archivo_proceso;

t_dictionary * tabla_archivos_procesos;
t_list * tabla_global_archivos;

pthread_mutex_t mutex_tabla_global;
pthread_mutex_t mutex_tabla_archivo_proceso;


int conectarAFileSystem();
char* checkearPermisos(char * modo, char * flag);
int existeArchivo(char* path);
int abrirArchivo(char* path, t_banderas mode, int pid) ;
char * leerArchivo(int pid, int fileDescriptorProceso, int tamanio);
void crearArchivo(char * path);
int escribirArchivo(int pid, int fileDescriptorProceso, void* escritura, int tamanio);
void cerrarArchivo(int pid, int fileDescriptorProceso);
int eliminarArchivo(int pid, int fileDescriptorProceso);
void moverCursor(int pid, int fileDescriptorProceso, int posicion);
int getCursor(int pid,int fileDescriptorProceso);
void liberarFileSistem(int pid);
////////////////////////////////////////////////////////////
///////////////////////CAPA DE MEMORIA/////////////////////
#define ALOCAMIENTO_FAIL 0

typedef struct {
	int pid;
	int pagina;
	int free;
} struct_pagina;

t_list* paginas;

void inicializarCapaDeMemoria();

char* alocarMemoria(int cpu, int pid, int size);
struct_pagina* buscarPagina(int pid, int realSize, int intentos);
void crearBloqueEnMemoria(int pid, int pagina, int offset, int tamanio, int ocupado);
void alocamientoFail(int cpu, int pid);
void fragmentar(int pid, int pagina);
int alocarEnPagina(int pid, int pagina, int sizeReal, int cpu);

void liberarDelHeap(int pid, int pagina, int offset);
/////////////////////////////////////////////////////////////

///////////////////////////////KERNEL/////////////////////////

#define CONSOLA 1
#define CPU 2

typedef struct {

	int puertoProg;
	int puertoCpu;
	char *ipMemoria;
	int puertoMemoria;
	char * ipFs;
	int puertoFs;
	int quantum;
	int quantumSleep;
	char * algoritmo;
	int gradoMultiprog;
	char ** semIds;
	char ** sharedVars;
	char ** semInit;
	int stackSize;

} t_conf;

int fileSystem;
int socketServer;

t_list* variablesGlobales;
t_list* semaforosGlobales;

t_queue * exits;
pthread_mutex_t mutex_bloqueado;
pthread_mutex_t mutex_exits;

fd_set descriptores;

t_log *archivoLog;
t_conf CONF_DATA;

void agregarAExit(struct_proceso* proceso);
void crearLog();
void leerConfiguracion(char* direccion);
void crearVariablesGlobales();
int levantarServidor();
void esperarConexiones();
int getClientType(int socket);
void constructor();
void iniciarSemaforos();
void crearSemaforosGlobales();
///////////////////////////////////////////////////////////////////////
//////////////////////////GENERICO//////////////////////////////////////
void logInfo(char* message) { log_info(archivoLog, message); }
void logError(char* message) { log_error(archivoLog, message); }
void logWarning( char* message) { log_warning(archivoLog, message); }
///////////////////////////////////////////////////////////////////////

#endif /* KERNEL_H_ */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <signal.h>
#include "../Funciones/sockets.h"

#define BACKLOG 5

#define CLIENT_TYPE_KERNEL 1
#define CLIENT_TYPE_CPU 2

#define FUNC_INIT_PROGRAM 0
#define FUNC_GET_BYTES 1
#define FUNC_SAVE_BYTES 2
#define FUNC_ASSIGN_PAGES 3
#define FUNC_FINISH 4
#define FUNC_DELETE_PAGE 5

#define STATUS_OK 0
#define STATUS_ERROR -1

#define IS_CACHE_ACTIVE true
#define IS_HASHING_ACTIVE true

typedef struct {
	char *puerto;
	int marcos;
	int marcoSize;
	int entradasCache;
	int cacheXProc;
	char *reemplazoCache;
	int retardoMemoria;
} t_conf;

typedef struct {
	int pid;
	int page;
} t_pages_table_row;

typedef struct {
	int pid;
	int page;
	void *content;
} t_cache_row;

typedef struct {
	uint32_t pid;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} t_function_get_bytes_params;

typedef struct {
	uint32_t pid;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
	void *buffer;
} t_function_save_bytes_params;

t_log *LOGGER;
t_conf CONF_DATA;
void *MEMORY_BLOCK;
int MEMORY_SIZE;
int ADMIN_PAGES_COUNT;
void *CACHE;
t_cache_row *CACHE_TABLE;
t_pages_table_row *PAGES_TABLE;
sem_t SEM_PAGES_TABLE;
sem_t SEM_CACHE;
int SERVER_FD;
int IS_KERNEL_CONNECTED = 0;//0 no, 1 yes.

int HASH_NUMBER = 10;

int config();
int configLog();
int readConfigFile();
int configPagesTable(void *memory);
int initServer();
int getClientType(int socket);
void finalize();
void kernelManager(int *s);
void performKernelFunctions(int socketKernel);
int sendPageSize(int socket);
int recvInt(int socket);
void cpuManager(int *s);
int recvFunctionIdFromCpu(int socket);
int performInitProgramFunction(int socket);
int performAssignPagesToProgramFunction(int socket);
int performFinishProgramFunction(int socket);
int performDeletePageFunction(int fd);
int performGetBytesFunction(int socket);
int performSaveBytesFunction(int socket);
int recvGetBytesFunctionParams(t_function_get_bytes_params *params, int socket);
int sendBytes(int socket, int countBytes, void *bytes);
int recvSaveBytesFunctionParams(t_function_save_bytes_params *params, int socket);
int sendIntTo(int socket, int number);
int initProgram(int pid, int pagesCount);
int getBytesOfPage(void *buffer, t_function_get_bytes_params params);
int saveBytesInPage(t_function_save_bytes_params params);
int assignPagesToProgram(int pid, int pagesCount);
int finishProgram(int pid);
int deletePage(int pid, int pageNumber);
int getBytesFromMemory(void *buffer, t_function_get_bytes_params params);
int getPageFromMemory(void *buffer, int pid, int page);
int saveBytesInMemory(t_function_save_bytes_params params);
int getEmptyFrame(int pid, int page);
int hashOf(int pid, int page);
int getNextEmptyFrame(int frame);
bool isEmptyFrame(int frame);
bool isNotEmptyFrame(int frame);
void eraseFrame(int frame);
void eraseFramesOf(int pid);
int erasePageFromFrame(int pid, int page);
int getLastPageOf(int pid);
void dereferencePages(int pid, int fromPage, int toPage);
bool isPresentPid(int pid);
int getFrame(int pid, int page);
int writeOnFrame(int frame, int offset, int size, void *buffer);
int getFromFrame(void *buffer, int frame, int offset, int size);
int getOffsetOf(int frame, int offset);
void configCache();
void eraseCache();
int saveBytesInCache(t_function_save_bytes_params params);
void updateCache(int index, t_function_save_bytes_params params);
int getPageInCache(int pid, int page);
void setCacheIndexAsUsed(int index);
void eraseCacheIndex(int index);
void moveCacheIndexToLastPosition(int index);
int getBytesFromCache(void *buffer, t_function_get_bytes_params params);
void erasePagesOfPidInCache(int pid);
void erasePageOfPidInCache(int pid, int page);
void dumpCacheTable();
bool isNotValidFrame(int frame);
bool isNotValidOffset(int offset);
bool isNotValidSize(int size);
bool isNotValidPageOffsetAndSize(int offset, int size);
bool isNotValidPid(int pid);
bool isNotValidCacheIndex(int index);
bool isNotValidPageCount(int pageCount);
bool isNotValidSaveBytesParams(t_function_save_bytes_params params);
bool isNotValidGetBytesParams(t_function_get_bytes_params params);
void initConsole();
int getFramesCount();
int getEmptyFramesCount();
int getFramesCountTo(int pid);
void dumpMemoryStructs();
void addIfNotPresent(t_list *list, int *e);
void dumpMemoryContent();
void dumpMemoryContentOf(int pid);
void sleepMillis(int millis);


/*
 * Dado un puerto, levanta un socket escucha en el mismo.
 * Retorna el socket creado.
 * (El socket que retorna esta apto para incluirlo en los sockets
 * a monitorear por un select)
 */
int crearSocketEscucha(char *puerto, int backlog);

/*
 * Dado un puerto, levanta un socket escucha, espera que se
 * conecte un cliente y cierra el socket escucha creado.
 * Retorna el socket cliente conectado
 */
int levantarServidorYConectarCliente(char* puerto, int backlog);

/*
 * Dado una ip y un puerto, levanta un socket cliente
 * y se conecta a servidor en ip y puerto dado.
 * Retorna el socket cliente conectado.
 */
int conectarseAServidor(char* ip, char* puerto);

/*
 * Dado un socket escucha, conecta el cliente que este
 * intentando conectarlo.
 * Retorna el socket cliente conectado.
 * (Cuando se esta monitorenado el socketEscucha con un select
 * y el select detecte que algun cliente quiere conectarse al mismo,
 * esta funcion conecta al cliente.)
 */
int conectarClienteDesdeServidor(int socketEscucha);


#endif /* MEMORIA_H_ */

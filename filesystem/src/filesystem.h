/*
 * filesystem.h
 *
 *  Created on: 5 jun. 2017
 *      Author: facu
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "../Funciones/sockets.h"

//////////////////////////////////////////////////////INICIO ESTADOS DE RESPUESTA/////////////////////////////////////////////////////////////
#define SUCCESS 1
#define UNSUCCESS 0
//////////////////////////////////////////////////////FIN ESTADOS DE RESPUESTA/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////INICIO METODOS DE CONEXION CON KERNEL//////////////////////////////////////////////////////////////////
#define VALIDAR_ARCHIVO 0
#define CREAR_ARCHIVO 1
#define BORRAR_ARCHIVO 2
#define OBTENER_DATOS 3
#define GUARDAR_DATOS  4
//////////////////////////////////////////////////////FIN METODOS DE CONEXION CON KERNEL/////////////////////////////////////////////////////////////

#define BACKLOG 5

typedef struct {
	char *puerto;
	char *puntoMontaje;
} t_conf;

typedef struct {
	void* blocks;
	int blocksCount;
} t_blocks;

typedef struct {
	int fileSize;
	t_blocks blocks;
} t_file_metadata;

typedef struct{
char * path;
char* mode;
} t_kernel_command;

typedef struct {
	int tamanioBloques;
	int cantidadBloques;
	char* magicNumber;
} t_metadata;

typedef struct {
	char *path;
	int offset;
	int size;
	void *buffer;
} t_data;

t_conf CONFIG_DATA;
t_metadata METADATA;
char CURRENT_WORK_DIRECTORY[200];
char *BITMAP_PATH;
char *FILES_PATH;
char *BLOCKS_PATH;
t_log *LOGGER;


int config();
void configLog();
int readConfigFile();
int readMetadataFile();
int createBitMap();
void fillFilesPath();
void fillBlocksPath();
void initFS(int servidor);
int connectKernel();
int testBitmapPosition(int position);
void setBitmapPosition(int position);
void unsetBitmapPosition(int position);
int getEmptyBlockNumberFrom(int position);
int performFileValidation(int kernelFd);
int performCreateFile(int kernelFd);
int performDeleteFile(int kernelFd);
void performGetData(int kernelFd);
void performSaveData(int kernelFd);
int createFile(char *path);
int writeFileMetadata(char *path, t_file_metadata fileMetadata);
void fillDataBuffer(t_data **data, void *blocks);
int deleteFile(char *path);
int saveData(t_data *data);
int saveBlocks(t_data *data, t_blocks *blocks);
int addBlock(t_blocks *blocks);
int getData(t_data** data);
void readFileMetadata(t_file_metadata **fileMetadata, char *path);
void freeFileMetadata(t_file_metadata *fileMetadata);
void freeData(t_data *data);
int createBlockFile(int flag);
void deleteBlockFile(int block);
void getMetadataFilePath(char **metadataFilePath, char *path);
void getBlocksFilePath(char **blockFilePath, int b);
void concat(char **fullPath, char *mntPath, char *path);
bool fileExist(char *path);
bool validateBlockCount();
int recvInt(int fd);
int recvString(char **str, int fd);
int recvDataToGet(t_data **data, int fd);
void recvDataToSave(t_data **data, int fd);
void sendBuffer(int kernelFd, t_data *data);
int getServer();
int acceptClient(int serverfd);
#endif /* FILESYSTEM_H_ */

#include "filesystem.h"

int main() {
	if (config() < 0) return EXIT_FAILURE;

	int server = getServer();

	initFS(server);

	return EXIT_SUCCESS;
}

int config() {
	if (getcwd(CURRENT_WORK_DIRECTORY, 200) != NULL) {
		printf("CWD %s\n", CURRENT_WORK_DIRECTORY);
	}else{
		perror("getcwd() error");
		return -1;
	}

	configLog();
	if (readConfigFile() < 0) return -1;
	if (readMetadataFile() < 0) return -1;
	if (createBitMap() < 0) return -1;

	fillFilesPath();
	fillBlocksPath();

	return 0;
}

void configLog() {
	char *logFilePath = string_new();
	string_append(&logFilePath,CURRENT_WORK_DIRECTORY);
	string_append(&logFilePath, "/log/filesystem.log");
	printf("logFilePath %s\n", logFilePath);

	LOGGER = log_create(logFilePath, "FileSystem", true, LOG_LEVEL_TRACE);
	free(logFilePath);
}

int readConfigFile() {
	char *confFilePath = string_new();
	string_append(&confFilePath,CURRENT_WORK_DIRECTORY);
	string_append(&confFilePath, "/conf/filesystem.conf");

	log_info(LOGGER, "Properties file path %s", confFilePath);

	t_config *configFile = config_create(confFilePath);

	if (configFile == NULL) {
		log_error(LOGGER, "No se encontro el archivo de configuracion para el Filesystem");
		return -1;
	}

	CONFIG_DATA.puerto = string_duplicate(config_get_string_value(configFile, "PUERTO"));
	CONFIG_DATA.puntoMontaje = string_duplicate(config_get_string_value(configFile, "PUNTO_MONTAJE"));

	config_destroy(configFile);

	free(confFilePath);
	log_info(LOGGER, "Configuracion leida: PUERTO %s, PUNTO_MONTAJE %s", CONFIG_DATA.puerto, CONFIG_DATA.puntoMontaje);

	return 0;
}

int readMetadataFile() {
	char *metadataFilePath = string_new();

	string_append(&metadataFilePath, CONFIG_DATA.puntoMontaje);
	string_append(&metadataFilePath, "/Metadata/Metadata.bin");

	log_info(LOGGER, "Metadata file path %s", metadataFilePath);

	t_config *metadataFile = config_create(metadataFilePath);
	free(metadataFilePath);

	if (metadataFile == NULL) {
		log_error(LOGGER, "No se encontro el archivo de metadata");
		return -1;
	}

	METADATA.tamanioBloques = config_get_int_value(metadataFile, "TAMANIO_BLOQUES");

	METADATA.cantidadBloques = config_get_int_value(metadataFile, "CANTIDAD_BLOQUES");
	METADATA.magicNumber = string_duplicate(config_get_string_value(metadataFile, "MAGIC_NUMBER"));

	config_destroy(metadataFile);
	log_info(LOGGER, "Metadata leida: cantidadBloques %d, magicNumber %s, tamanioBloques %d",
			METADATA.cantidadBloques, METADATA.magicNumber, METADATA.tamanioBloques);
	return 0;
}

int createBitMap() {
	BITMAP_PATH = string_new();

	string_append(&BITMAP_PATH, CONFIG_DATA.puntoMontaje);
	string_append(&BITMAP_PATH, "Metadata/Bitmap.bin");

	log_info(LOGGER, "Bitmap file path %s", BITMAP_PATH);

	FILE *bitmapFile = fopen(BITMAP_PATH, "w");

	char *bitmap = malloc(METADATA.cantidadBloques);
	int n;
	for (n = 0; n < METADATA.cantidadBloques; n++) {
		bitmap[n] = '0';
	}
	fwrite(bitmap, sizeof(char), METADATA.cantidadBloques, bitmapFile);
	free(bitmap);

	fclose(bitmapFile);

	return 0;
}

void fillFilesPath() {
	FILES_PATH = string_new();

	string_append(&FILES_PATH, CONFIG_DATA.puntoMontaje);
	string_append(&FILES_PATH, "/Archivos");

	mkdir(FILES_PATH, S_IRUSR | S_IRGRP | S_IROTH |S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR );

	log_info(LOGGER, "files path %s", FILES_PATH);
}

void fillBlocksPath() {
	BLOCKS_PATH = string_new();

	string_append(&BLOCKS_PATH, CONFIG_DATA.puntoMontaje);
	string_append(&BLOCKS_PATH, "/Bloques");

	mkdir(BLOCKS_PATH, S_IRUSR | S_IRGRP | S_IROTH |S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR );

	log_info(LOGGER, "blocks path %s", BLOCKS_PATH);
}

void initFS(int servidor) {
	int kernelFd = connectKernel(servidor);
	if (kernelFd < 0) return;

	while (1) {
		int operationId = recibirProtocolo(kernelFd);

		switch (operationId) {
			case VALIDAR_ARCHIVO:
				performFileValidation(kernelFd);
				break;
			case CREAR_ARCHIVO:
				performCreateFile(kernelFd);
				break;
			case BORRAR_ARCHIVO:
				performDeleteFile(kernelFd);
				break;
			case OBTENER_DATOS:
				performGetData(kernelFd);
				break;
			case GUARDAR_DATOS:
				performSaveData(kernelFd);
				break;
			default:
				log_error(LOGGER, "Kernel desconectado %d", operationId);
				return;
		}
	}
}

int connectKernel(int servidor) {
	int kernelFd = acceptClient(servidor);
	if (kernelFd < 0) {
		log_error(LOGGER, "Error aceptando Kernel");
		return -1;
	}
	log_info(LOGGER, "Kernel conectado");
	return kernelFd;
}

int testBitmapPosition(int position) {
	FILE *bitmapFile = fopen(BITMAP_PATH, "r");

	fseek(bitmapFile, position, SEEK_SET);

	char value;
	fread(&value, sizeof(char), 1, bitmapFile);
	fclose(bitmapFile);

	return memcmp(&value, "0", 1) == 0 ? 0 : 1;
}

void setBitmapPosition(int position) {
	FILE *bitmapFile = fopen(BITMAP_PATH, "r+");

	fseek(bitmapFile, position, SEEK_SET);

	fwrite("1", sizeof(char), 1, bitmapFile);
	fclose(bitmapFile);
}

void unsetBitmapPosition(int position) {
	FILE *bitmapFile = fopen(BITMAP_PATH, "r+");

	fseek(bitmapFile, position, SEEK_SET);

	fwrite("0", sizeof(char), 1, bitmapFile);
	fclose(bitmapFile);
}

int getEmptyBlockNumberFrom(int position) {
	int i;
	for (i = position; i < METADATA.cantidadBloques; i++) {
		if (testBitmapPosition(i) == 0) {
			setBitmapPosition(i);
			return i;
		}
	}
	return -1;
}

int performFileValidation(int kernelFd) {
	log_info(LOGGER, "Validando archivo");

	char *path;
	if (recvString(&path, kernelFd) < 0) return -1;
	log_info(LOGGER, "Path recibido: %s", path);


	bool exist = fileExist(path);

	free(path);

	return sendIntTo(kernelFd, exist);
}

int performCreateFile(int kernelFd) {
	log_info(LOGGER, "Creando archivo");

	char *path;
	if (recvString(&path, kernelFd) < 0) return -1;
	log_info(LOGGER, "Path recibido: %s", path);

	int status = createFile(path);
	free(path);

	return sendIntTo(kernelFd, status);
}

int performDeleteFile(int kernelFd) {
	log_info(LOGGER, "Borrando archivo");

	char *path;
	if (recvString(&path, kernelFd) < 0) return -1;
	log_info(LOGGER, "Path recibido: %s", path);

	int status = deleteFile(path);
	free(path);

	return sendIntTo(kernelFd, status);
}

void performGetData(int kernelFd) {
	log_info(LOGGER, "Obteniendo datos de archivo");

	t_data *data = malloc(sizeof(t_data));
	if (recvDataToGet(&data, kernelFd) < 0) return;
	log_info(LOGGER, "Path recibido: %s", data->path);

	data->buffer = malloc(data->size);
	int status = getData(&data);
	if (status < 0) {
		log_error(LOGGER, "Fallo obteniendo datos de [%s]", data->path);
		int confirmacion = UNSUCCESS;
		send(kernelFd, &confirmacion, sizeof(int), 0);
	} else {

		sendBuffer(kernelFd, data);
	}

	freeData(data);
}

void performSaveData(int kernelFd) {
	log_info(LOGGER, "Guardando datos en archivo");

	t_data *data = malloc(sizeof(t_data));
	recvDataToSave(&data, kernelFd);
	log_info(LOGGER, "Path recibido: %s", data->path);

	int status = saveData(data);

	freeData(data);
	log_info(LOGGER, "Se escribio en el archivo: %d", status);


	enviarProtocolo(kernelFd, status);
}

int createFile(char *path) {
	if (fileExist(path)) {
		log_error(LOGGER, "El archivo [%s] ya existe", path);
		return UNSUCCESS;
	}
	int blockNumber = createBlockFile(0);
	if (blockNumber < 0) return -1;

	char **splitedPath = string_split(path, "/");

	char *filesPath;
	//size_t s = strlen(splitedPath[0]);

	char *p = string_duplicate(splitedPath[0]);

	int i = 0;
	while (splitedPath[i + 1] != NULL) {
		getMetadataFilePath(&filesPath, p);
		mkdir(filesPath, S_IRUSR | S_IRGRP | S_IROTH |S_IWUSR | S_IWGRP | S_IWOTH | S_IXUSR );
		free(filesPath);
		i++;
		string_append(&p, splitedPath[i]);
	}

	getMetadataFilePath(&filesPath, p);
	free(p);

	t_file_metadata meta;
	meta.fileSize = 0;
	meta.blocks.blocks = malloc(sizeof(blockNumber));
	memcpy(meta.blocks.blocks, &blockNumber, sizeof(blockNumber));
	meta.blocks.blocksCount = 1;

	writeFileMetadata(filesPath, meta);

	free(meta.blocks.blocks);
	free(filesPath);
	return SUCCESS;
}

int deleteFile(char *path) {
	if (!fileExist(path)) {
		log_error(LOGGER, "El archivo [%s] no existe", path);
		return UNSUCCESS;
	}

	char *filePath;
	getMetadataFilePath(&filePath, path);
	t_file_metadata *meta = malloc(sizeof(t_file_metadata));
	readFileMetadata(&meta, filePath);
	remove(filePath);
	free(filePath);

	int n;
	for (n = 0; n < meta->blocks.blocksCount; n++) {
		int block;
		memcpy(&block, meta->blocks.blocks + sizeof(int) * n, sizeof(int));
		deleteBlockFile(block);
	}

	freeFileMetadata(meta);
	return SUCCESS;
}

int saveData(t_data *data) {
	if (!fileExist(data->path)) {
		log_error(LOGGER, "El archivo [%s] no existe", data->path);
		return UNSUCCESS;
	}

	char *metadatafilePath;
	getMetadataFilePath(&metadatafilePath, data->path);
	t_file_metadata *meta = malloc(sizeof(t_file_metadata));
	readFileMetadata(&meta, metadatafilePath);

	meta->fileSize += data->size;
	int status = saveBlocks(data, &(meta->blocks));

	writeFileMetadata(metadatafilePath, *meta);
	free(metadatafilePath);
	freeFileMetadata(meta);

	return status;
}

int saveBlocks(t_data *data, t_blocks *blocks) {
	int offset = data->offset % METADATA.tamanioBloques;
	int n = data->offset / METADATA.tamanioBloques;
	int copiedSize = 0;
	char *blockFilePath;
	int block;
	while (copiedSize < data->size) {
		while (blocks->blocksCount <= n) {
			if (addBlock(blocks) < 0) return UNSUCCESS;
		}

		memcpy(&block, blocks->blocks + sizeof(int) * n, sizeof(int));
		getBlocksFilePath(&blockFilePath, block);

		FILE *blockFile = fopen(blockFilePath, "w");
		free(blockFilePath);

		int sizeToCopy;
		if (offset > 0) {
			fseek(blockFile, offset, SEEK_SET);
			sizeToCopy = METADATA.tamanioBloques - offset;
		} else {
			sizeToCopy = data->size - copiedSize > METADATA.tamanioBloques ? METADATA.tamanioBloques : data->size - copiedSize;
		}
		fwrite((data->buffer) + copiedSize, sizeToCopy, 1, blockFile);
		fclose(blockFile);

		n++;
		copiedSize += sizeToCopy;
		offset = 0;
	}
	return SUCCESS;
}

int addBlock(t_blocks *blocks) {
	int lastBlock;
	memcpy(&lastBlock, blocks->blocks + sizeof(int) * (blocks->blocksCount - 1), sizeof(int));
	int b = createBlockFile(lastBlock);
	if (b < 0) return -1;
	blocks->blocks = realloc(blocks->blocks, (blocks->blocksCount + 1) * sizeof(int));
	memcpy(blocks->blocks + sizeof(int) * blocks->blocksCount, &b, sizeof(int));
	blocks->blocksCount += 1;

	return 0;
}

int getData(t_data **data) {
	if (!fileExist((*data)->path)) {
		log_error(LOGGER, "El archivo [%s] no existe", (*data)->path);
		return UNSUCCESS;
	}

	char *metadatafilePath;
	getMetadataFilePath(&metadatafilePath, (*data)->path++);
	t_file_metadata *meta = malloc(sizeof(t_file_metadata));
	readFileMetadata(&meta, metadatafilePath);
	free(metadatafilePath);

	int lastBlock = ((*data)->offset + (*data)->size) % METADATA.tamanioBloques > 0 ? ((*data)->offset + (*data)->size) / METADATA.tamanioBloques + 1 : ((*data)->offset + (*data)->size) % METADATA.tamanioBloques;
	if (meta->blocks.blocksCount < lastBlock) return -1;

	fillDataBuffer(data, meta->blocks.blocks);
	freeFileMetadata(meta);
	return 0;
}

void fillDataBuffer(t_data **data, void *blocks) {
	int offset = (*data)->offset % METADATA.tamanioBloques;
	int n = (*data)->offset / METADATA.tamanioBloques;
	int copiedSize = 0;
	char *blockFilePath;
	int b;
	void *blockData;
	int sizeToCopy;
	while (copiedSize < (*data)->size) {
		memcpy(&b, blocks + sizeof(int) * n, sizeof(int));
		getBlocksFilePath(&blockFilePath, b);
		printf("blockPath: %s\n", blockFilePath);

		FILE *blockFile = fopen(blockFilePath, "r");
		free(blockFilePath);

		blockData = malloc(METADATA.tamanioBloques);
		fread(blockData, METADATA.tamanioBloques, 1, blockFile);
		fclose(blockFile);

		if (offset > 0) {
			sizeToCopy = METADATA.tamanioBloques - offset;
		} else {
			sizeToCopy = (*data)->size - copiedSize > METADATA.tamanioBloques ? METADATA.tamanioBloques : (*data)->size - copiedSize;
		}

		memcpy((*data)->buffer + copiedSize, blockData + offset, sizeToCopy);
		free(blockData);

		n++;
		copiedSize += sizeToCopy;
		offset = 0;
	}
}

int writeFileMetadata(char *path, t_file_metadata fileMetadata) {
	int intsCount = fileMetadata.blocks.blocksCount + 2;
	int *metadata = malloc(intsCount * sizeof(int));

	metadata[0] = fileMetadata.fileSize;
	metadata[1] = fileMetadata.blocks.blocksCount;

	int n;
	for (n = 2; n < intsCount; n++) {
		int pBlock;
		memcpy(&pBlock, fileMetadata.blocks.blocks + sizeof(int) * (n - 2), sizeof(int));
		metadata[n] = pBlock;
	}

	FILE *metadataFile = fopen(path, "w");
	fwrite(metadata, sizeof(int), intsCount, metadataFile);
	free(metadata);

	fclose(metadataFile);

	return 0;
}

void readFileMetadata(t_file_metadata **fileMetadata, char *path) {
	FILE *metadataFile = fopen(path, "r");

	fread(&((*fileMetadata)->fileSize), sizeof(int), 1, metadataFile);

	int position = sizeof(int);
	fseek(metadataFile, position, SEEK_SET);

	int blocksCount;
	fread(&blocksCount, sizeof(int), 1, metadataFile);

	(*fileMetadata)->blocks.blocksCount = blocksCount;
	(*fileMetadata)->blocks.blocks = malloc(sizeof(int) * blocksCount);
	int n;
	for (n = 0; n < blocksCount; n++) {
		position += sizeof(int);
		fseek(metadataFile, position, SEEK_SET);

		fread((*fileMetadata)->blocks.blocks + sizeof(int) * n, sizeof(int), 1, metadataFile);
	}

	fclose(metadataFile);
}

void freeFileMetadata(t_file_metadata *fileMetadata) {
	free(fileMetadata->blocks.blocks);
	free(fileMetadata);
}

void freeData(t_data *data) {
	free(data->buffer);
	//free(data->path);
	free(data);
}

int createBlockFile(int flag) {
	int blockNumber = getEmptyBlockNumberFrom(flag);
	if (blockNumber < 0) return -1;

	char *blocksPath;
	getBlocksFilePath(&blocksPath, blockNumber);
	FILE *blockFile = fopen(blocksPath, "w");
	free(blocksPath);

	fclose(blockFile);

	return blockNumber;
}

void deleteBlockFile(int block) {
	unsetBitmapPosition(block);
	char *blocksPath;
	getBlocksFilePath(&blocksPath, block);

	remove(blocksPath);
	free(blocksPath);
}

void getMetadataFilePath(char **metadataFilePath, char *path) {
	concat(metadataFilePath, FILES_PATH, path);
}

void getBlocksFilePath(char **blockFilePath, int b) {
	char strFlag[15];
	sprintf(strFlag, "%d.bin\0", b);
	concat(blockFilePath, BLOCKS_PATH, strFlag);
}

void concat(char **fullPath, char *mntPath, char *path) {
	*fullPath = string_new();

	string_append(fullPath, mntPath);
	string_append(fullPath, "/");
	string_append(fullPath, path);
}

bool fileExist(char *path) {
	char *filePath;
	getMetadataFilePath(&filePath, path);
	bool exist = access(filePath, F_OK) == 0;
	free(filePath);
	return exist;
}

int sendIntTo(int socket, int number) {
	int32_t n = number;
	int status = send(socket, &n, sizeof(n), 0);
	if (number < 0) {
		return -1;
	} else {
		return status <= 0 ? -1 : 0;
	}
}

int recvInt(int fd) {
	char protocolo[5];
	int bytes= recv(fd, protocolo,4,MSG_WAITALL);
	if(bytes!=4){
		perror("Error protocolo");
		return -1;
	}
	protocolo[4] = '\0';
	return atoi(protocolo);
}

int recvString(char **str, int fd) {
	int32_t strLenght = recvInt(fd);
	if (strLenght < 0) {
		log_error(LOGGER,"Fallo al recibir largo de string");
		return -1;
	}
	*str = malloc(strLenght + 1);
	if (recv(fd, *str, strLenght, 0) < strLenght) {
		log_error(LOGGER,"Fallo al recibir string");
		return -1;
	}
	(*str)[strLenght] = '\0';
	return 0;
}

int recvDataToGet(t_data **data, int fd) {
	if (recvString(&((*data)->path), fd) < 0) return -1;

	(*data)->offset = recvInt(fd);
	if ((*data)->offset < 0) return -1;

	(*data)->size = recvInt(fd);
	if ((*data)->size < 0) return -1;

	return 0;
}

void recvDataToSave(t_data **data, int fd) {
	char* mensaje = esperarMensaje(fd);
	log_info(LOGGER, "Recibi del Kernel: %s", mensaje);
	char* offsetChar = toSubString(mensaje, 0 ,3 );
	(*data)->offset = atoi(offsetChar);
	free(offsetChar);
	char* sizeChar = toSubString(mensaje, 4 ,7 );
	(*data)->size  = atoi(sizeChar);
	free(sizeChar);
	(*data)->path = toSubString(mensaje, 8, string_length(mensaje) -1);
	free(mensaje);

	send(fd,&fd, sizeof(int), 0);
	(*data)->buffer = malloc((*data)->size);
	recv(fd, (*data)->buffer, (*data)->size, 0);
}

void sendBuffer(int kernelFd, t_data *data) {
	int confirmacion = SUCCESS;
	send(kernelFd, &confirmacion, sizeof(int), 0);
	esperarConfirmacion(kernelFd);
	send(kernelFd, data->buffer, data->size, 0);
}

int getServer() {
	int servidor = crearServidor(atoi(CONFIG_DATA.puerto));
	if (servidor < 0) {
		log_error(LOGGER, "No se pudo levantar el filesystem en el puerto %s", CONFIG_DATA.puerto);
		return -1;
	}
	log_info(LOGGER, "Servidor creado en el puerto %s", CONFIG_DATA.puerto);
	return servidor;
}

int acceptClient(int serverfd) {
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);
	return accept(serverfd, (struct sockaddr *) &addr, &addrlen);
}

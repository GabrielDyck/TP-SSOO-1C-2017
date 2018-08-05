rm log/log
cp conf/pruebaFileSystem.properties conf/conf.properties
gcc src/Memoria.c Funciones/* src/Memoria.h -o memoria -lcommons -lpthread
./memoria

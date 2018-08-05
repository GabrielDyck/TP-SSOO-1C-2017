rm log/log
cp conf/pruebaBase.properties conf/conf.properties
gcc src/Memoria.c Funciones/* src/Memoria.h -o memoria -lcommons -lpthread
./memoria

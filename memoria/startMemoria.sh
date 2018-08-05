rm log/log
cp conf/default.properties conf/conf.properties
gcc src/Memoria.c src/Memoria.h -o memoria -lcommons -lpthread
./memoria

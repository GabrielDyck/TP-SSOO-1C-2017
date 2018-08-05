rm Kernel.log
rm kernel
gcc src/Kernel.c src/Kernel.h Funciones/* src/obj/* -o kernel -lcommons -lpthread	-lparser-ansisop
./kernel "pruebaFS"
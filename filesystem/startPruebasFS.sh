cp conf/pruebas.conf conf/filesystem.conf
mkdir $HOME/FS_SADICA
cp -a mnt/SADICA_FS/Metadata $HOME/FS_SADICA
gcc src/filesystem.c Funciones/* src/filesystem.h -o fs -lcommons
./fs

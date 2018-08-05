cp conf/default.conf conf/filesystem.conf
gcc src/filesystem.c src/filesystem.h -o fs -lcommons
./fs

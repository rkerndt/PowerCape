# Meant to be built on a BeagleBone (not cross-compiled)

default: ina219 power

powercape.o: powercape.c powercape.h
    gcc -c powercape.c

ina219:	ina219.c
	gcc -o ina219 ina219.c

power:	power.c powercape.o
	gcc -o power power.c powercape.o


test: main.o spi.o rs232.o flash25.o
	cc -o main main.o rs232.o spi.o flash25.o

main.o: main.c
	cc $(CFLAGS) -c main.c -o main.o
flash25.o: flash25.c flash25.h
	cc $(CFLAGS) -c flash25.c -o flash25.o
spi.o: spi.c spi.h
	cc $(CFLAGS) -c spi.c -o spi.o
rs232.o: rs232.c rs232.h
	cc $(CFLAGS) -c rs232.c -o rs232.o

clean:
	rm *.o main

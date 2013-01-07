all: digenv.o
	gcc -o Digenv digenv.o

digenv.o: digenv.c
	gcc -c digenv.c

clean:
	rm *.o Digenv

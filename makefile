main:main.o net.o
	gcc -o main main.o net.o -lpthread
main.o:main.c net.h
	gcc -c main.c
net.o:net.c net.h
	gcc -c net.c
clean:
	rm main main.o net.o

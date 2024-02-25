# Makefile for TCP project

all: tcp-Receiver tcp-Sender

tcp-Receiver: tcp-Receiver.c
	gcc -o tcp-Receiver tcp-Receiver.c

tcp-Sender: tcp-Sender.c
	gcc -o tcp-Sender tcp-Sender.c

clean:
	rm -f *.o tcp-Receiver tcp-Sender

runs:
	./tcp-Receiver

runc:
	./tcp-Sender

runs-strace:
	strace -f ./tcp-Receiver

runc-strace:
	strace -f ./tcp-Sender
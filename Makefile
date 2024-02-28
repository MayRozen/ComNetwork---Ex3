# Makefile for TCP project

all: TCP_Receiver TCP_Sender

TCP_Receiver: TCP_Receiver.c
	gcc -o TCP_Receiver TCP_Receiver.c

TCP_Sender: TCP_Sender.c
	gcc -o TCP_Sender TCP_Sender.c

clean:
	rm -f *.o TCP_Receiver TCP_Sender

runs:
	./TCP_Receiver -pPORT -algoALGO

runc:
	./TCP_Sender -ipIP -pPORT -algoALGO

runs-strace:
	strace -f ./TCP_Receiver

runc-strace:
	strace -f ./TCP_Sender

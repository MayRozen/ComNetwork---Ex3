# ----------------- Makefile for TCP ---------------------

# all: TCP_Receiver TCP_Sender

# TCP_Receiver: TCP_Receiver.c
# 	gcc -o TCP_Receiver TCP_Receiver.c

# TCP_Sender: TCP_Sender.c
# 	gcc -o TCP_Sender TCP_Sender.c

# clean:
# 	rm -f *.o TCP_Receiver TCP_Sender

# runs:
# 	./TCP_Receiver -pPORT -algoALGO

# runc:
# 	./TCP_Sender -ipIP -pPORT -algoALGO

# runs-strace:
# 	strace -f ./TCP_Receiver 

# runc-strace:
# 	strace -f ./TCP_Sender


# ----------------- Makefile for RUDP ---------------------
CC = gcc
AR = ar
AFLAFS = rcs
CFLAGS = -Wall -g

all: RUDP_Receiver RUDP_Sender 

RUDP_Receiver: RUDP_Receiver.o RUDP_API.a
	$(CC) $(CFLAGS) $^ -o $@

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_Receiver.c

RUDP_Sender: RUDP_Sender.o RUDP_API.a
	$(CC) $(CFLAGS) $^ -o $@

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_Sender.c

RUDP_API.a: RUDP_API.o
	$(AR) $(AFLAFS) RUDP_API.a RUDP_API.o

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_API.c

clean:
	rm -f *.o RUDP_Receiver RUDP_Sender

runs:
	./RUDP_Receiver -pPORT

runc:
	./RUDP_Sender -ipIP -pPORT

runs-strace:
	strace -f ./RUDP_Receiver

runc-strace:
	strace -f ./RUDP_Sender

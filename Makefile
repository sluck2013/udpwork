CC = gcc
UNP_PATH = ../unpv13e
LIBS = -lresolv -lnsl -lpthread $(UNP_PATH)/libunp.a

IFLAG = -I$(UNP_PATH)/lib
CFLAGS = -g -O2 -std=gnu99
FLAGS = $(IFLAG) $(CFLAGS)


all: client server

client: client.o  utility.o
	$(CC) $(CFLAGS) -o client client.o utility.o $(LIBS)
client.o: client.c client.h constants.h 
	$(CC) $(FLAGS) -c client.c $(LIBS)
server: server.o
	$(CC) $(CFLAGS) -o server server.o $(LIBS)
server.o: server.c server.h constants.h
	$(CC) $(FLAGS) -c server.c $(LIBS)
utility.o: utility.c utility.h
	$(CC) $(CFLAGS) -c utility.c

clean:
	echo "Removing object files..."
	rm -f *.o
	echo "Removing executable files..."
	rm -f client
	echo "Removing temp files..."
	rm -f *~ *.swp

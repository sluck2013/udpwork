CC = gcc
UNP_PATH = ../unpv13e
LIBS = -lresolv -lnsl -lpthread $(UNP_PATH)/libunp.a

IFLAG = -I$(UNP_PATH)/lib 
CFLAGS = -g -O2 -std=gnu99
FLAGS = $(IFLAG) $(CFLAGS)


all: client server

client: client.o  utility.o get_ifi_info_plus.o
	$(CC) $(CFLAGS) -o client client.o utility.o $(LIBS)
client.o: client.c client.h constants.h get_ifi_info_plus.o
	$(CC) $(FLAGS) -c client.c get_ifi_info_plus.o $(LIBS)
server: server.o utility.o
	$(CC) $(CFLAGS) -o server server.o utility.o $(LIBS)
server.o: server.c server.h constants.h lib/unprtt.h lib/unpifiplus.h get_ifi_info_plus.o
	$(CC) $(FLAGS) -c server.c get_ifi_info_plus.o $(LIBS)
utility.o: utility.c utility.h lib/unpifiplus.h
	$(CC) $(FLAGS) -c utility.c $(LIBS)
get_ifi_info_plus.o: lib/get_ifi_info_plus.c
	$(CC) $(FLAGS) -c lib/get_ifi_info_plus.c $(LIBS)
prifinfo_plus.o: lib/prifinfo_plus.c
	$(CC) $(FLAGS) -c lib/prifinfo_plus.c $(LIBS)

clean:
	echo "Removing object files..."
	rm -f *.o
	echo "Removing executable files..."
	rm -f client server
	echo "Removing temp files..."
	rm -f *~ *.swp

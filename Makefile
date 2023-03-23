CC = gcc
CFLAGS = -Wall
TARGET = bin
INTER = obj
OBJC = $(INTER)/compdetect_client.o $(INTER)/cJSON.o
OBJS = $(INTER)/compdetect_server.o $(INTER)/cJSON.o

all: client server

client: $(OBJC)
	$(CC) $(CFLAGS) $(OBJC) -o $(TARGET)/compdetect_client
server: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)/compdetect_server

# object files
$(INTER)/compdetect_client.o: compdetect_client.c cJSON.h
	$(CC) $(CFLAGS) -c compdetect_client.c -o $(INTER)/compdetect_client.o
$(INTER)/compdetect_server.o: compdetect_server.c cJSON.h
	$(CC) $(CFLAGS) -c compdetect_server.c -o $(INTER)/compdetect_server.o
$(INTER)/cJSON.o: cJSON.c
	$(CC) $(CFLAGS) -c cJSON.c -o $(INTER)/cJSON.o

clean:
	rm build/* ||:

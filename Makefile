CC = gcc
CFLAGS = -Wall -g -D DEBUG=0 -lpthread
target = bin
inter = obj

OBJC = $(inter)/compdetect_client.o $(inter)/cJSON.o $(inter)/sockets.o $(inter)/util.o
OBJS = $(inter)/compdetect_server.o $(inter)/cJSON.o $(inter)/sockets.o $(inter)/util.o
OBJA = $(inter)/compdetect.o $(inter)/cJSON.o $(inter)/headers.o $(inter)/sockets.o $(inter)/util.o

all: client server standalone

client: $(OBJC) | $(target)
	$(CC) $(CFLAGS) $(OBJC) -o $(target)/compdetect_client
server: $(OBJS) | $(target)
	$(CC) $(CFLAGS) $(OBJS) -o $(target)/compdetect_server
standalone: $(OBJA) | $(target)
	$(CC) $(CFLAGS) $(OBJA) -o $(target)/compdetect

# object files
$(inter)/compdetect_client.o: | $(inter)
	$(CC) $(CFLAGS) -c compdetect_client.c -o $(inter)/compdetect_client.o
$(inter)/compdetect_server.o: | $(inter)
	$(CC) $(CFLAGS) -c compdetect_server.c -o $(inter)/compdetect_server.o
$(inter)/compdetect.o: | $(inter)
	$(CC) $(CFLAGS) -c compdetect.c -o $(inter)/compdetect.o
$(inter)/cJSON.o: | $(inter)
	$(CC) $(CFLAGS) -c cJSON.c -o $(inter)/cJSON.o
$(inter)/sockets.o: | $(inter)
	$(CC) $(CFLAGS) -c sockets.c -o $(inter)/sockets.o
$(inter)/headers.o: | $(inter)
	$(CC) $(CFLAGS) -c headers.c -o $(inter)/headers.o
$(inter)/util.o: | $(inter)
	$(CC) $(CFLAGS) -c util.c -o $(inter)/util.o

$(target):
	mkdir $@
$(inter):
	mkdir $@

clean:
	rm obj/* bin/* ||:

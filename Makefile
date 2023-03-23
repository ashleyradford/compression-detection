CC = gcc
CFLAGS = -Wall
target = bin
inter = obj

OBJC = $(inter)/compdetect_client.o $(inter)/cJSON.o
OBJS = $(inter)/compdetect_server.o $(inter)/cJSON.o

all: client server

client: $(OBJC) | $(target)
	$(CC) $(CFLAGS) $(OBJC) -o $(target)/compdetect_client
server: $(OBJS) | $(target)
	$(CC) $(CFLAGS) $(OBJS) -o $(target)/compdetect_server

# object files
$(inter)/compdetect_client.o: compdetect_client.c cJSON.h | $(inter)
	$(CC) $(CFLAGS) -c compdetect_client.c -o $(inter)/compdetect_client.o
$(inter)/compdetect_server.o: compdetect_server.c cJSON.h | $(inter)
	$(CC) $(CFLAGS) -c compdetect_server.c -o $(inter)/compdetect_server.o
$(inter)/cJSON.o: cJSON.c | $(inter)
	$(CC) $(CFLAGS) -c cJSON.c -o $(inter)/cJSON.o

$(target):
	mkdir $@
$(inter):
	mkdir $@

clean:
	rm obj/* bin/* ||:

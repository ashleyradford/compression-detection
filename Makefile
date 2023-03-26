CC = gcc
CFLAGS = -Wall -g -D DEBUG=1
target = bin
inter = obj

OBJC = $(inter)/compdetect_client.o $(inter)/cJSON.o $(inter)/tcp.o $(inter)/udp.o
OBJS = $(inter)/compdetect_server.o $(inter)/cJSON.o $(inter)/tcp.o $(inter)/udp.o

all: client server

client: $(OBJC) | $(target)
	$(CC) $(CFLAGS) $(OBJC) -o $(target)/compdetect_client
server: $(OBJS) | $(target)
	$(CC) $(CFLAGS) $(OBJS) -o $(target)/compdetect_server

# object files
$(inter)/compdetect_client.o: compdetect_client.c cJSON.h tcp.h udp.h logger.h | $(inter)
	$(CC) $(CFLAGS) -c compdetect_client.c -o $(inter)/compdetect_client.o
$(inter)/compdetect_server.o: compdetect_server.c cJSON.h tcp.h udp.h logger.h | $(inter)
	$(CC) $(CFLAGS) -c compdetect_server.c -o $(inter)/compdetect_server.o
$(inter)/cJSON.o: cJSON.c | $(inter)
	$(CC) $(CFLAGS) -c cJSON.c -o $(inter)/cJSON.o
$(inter)/tcp.o: tcp.c logger.h | $(inter)
	$(CC) $(CFLAGS) -c tcp.c -o $(inter)/tcp.o
$(inter)/udp.o: udp.c logger.h | $(inter)
	$(CC) $(CFLAGS) -c udp.c -o $(inter)/udp.o

$(target):
	mkdir $@
$(inter):
	mkdir $@

clean:
	rm obj/* bin/* ||:

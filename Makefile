CC = gcc
CFLAGS = -Wall -g -D DEBUG=1
target = bin
inter = obj

OBJC = $(inter)/compdetect_client.o $(inter)/cJSON.o $(inter)/tcp.o $(inter)/udp.o
OBJS = $(inter)/compdetect_server.o $(inter)/cJSON.o $(inter)/tcp.o $(inter)/udp.o
OBJA = $(inter)/compdetect.o $(inter)/cJSON.o $(inter)/headers.o

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
$(inter)/cJSON.o: | $(inter)
	$(CC) $(CFLAGS) -c cJSON.c -o $(inter)/cJSON.o
$(inter)/tcp.o: | $(inter)
	$(CC) $(CFLAGS) -c tcp.c -o $(inter)/tcp.o
$(inter)/udp.o: | $(inter)
	$(CC) $(CFLAGS) -c udp.c -o $(inter)/udp.o
$(inter)/compdetect.o: | $(inter)
	$(CC) $(CFLAGS) -c compdetect.c -o $(inter)/compdetect.o
$(inter)/headers.o: | $(inter)
	$(CC) $(CFLAGS) -c headers.c -o $(inter)/headers.o

$(target):
	mkdir $@
$(inter):
	mkdir $@

clean:
	rm obj/* bin/* ||:

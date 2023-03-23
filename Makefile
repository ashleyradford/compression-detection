all: server client

server: compdetect_server.c
	gcc compdetect_server.c -o build/compdetect_server

client: compdetect_client.c
	gcc compdetect_client.c -o build/compdetect_client

clean:
	rm compdetect_server compdetect_client

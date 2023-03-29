# Compression Detection

Based on the techniques described in [End-to-End Detection of Compression of Traffic Flows by Intermediaries](https://www.cs.usfca.edu/vahab/resources/compression_detection.pdf) by Vahab Pournaghshband, Alexander Afanasyev, and Peter Reiher.

## Requirements
Requires cJSON source and header files. See Resources.<br>
A file filled with 2000 bytes of random data (included).<br>
A json configs file with the following keys:<br>
- **client_ip:** client IP address
- **server_ip:** server IP address
- **udp_source_port:** client UDP port number
- **udp_dest_port:** server UDP port number
- **tcp_head_dest:** server TCP port number (unreachable)
- **tcp_tail_dest:** server TCP port number (unreachable)
- **tcp_port:** server TCP port number (must match server command line arg)
- **udp_payload_size:** size of the UDP entropy payloads
- **inter_measurement_time:** time that the program will sleep in between sending packet trains
- **udp_train_size:** size of the UDP packet trains
- **udp_ttl:** UDP time to live value
- **udp_timeout:** timeout for receicing UDP packers in the server application
- **rst_timeout:** timeout for receiving RST packets in the standalone application
- **threshold:** compression detection threshold, times bigger than this value indicate compression

## Build
```
make server       # builds the server application
make client       # builds the client application
make standalone   # builds the standalone application
make              # builds server, client, and standalone applications
```

## Run
To run the cooperative compression detection, the server must first be up and running the *compdetect_server* program:
```
./bin/compdetect_server port
```
**Note:** the port number must match the *tcp_port* number defined in the configs file

To run the client side:
```
./bin/compdetect_client myconfigs.json
```

To run the standalone application (must have system admin permissions):
```
sudo ./bin/compdetect port
```

## Design Decisions

## Future Work

## Resources
[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/)<br>
[DaveGamble's cJSON](https://github.com/DaveGamble/cJSON)<br>
[P.D. Buchan's raw sockets](https://www.pdbuchan.com/rawsock/tcp4.c)<br>
[MaxXor's raw sockets](https://github.com/MaxXor/raw-sockets-example/blob/master/rawsockets.c)<br>
[Apple's ip.h documentation](https://opensource.apple.com/source/xnu/xnu-3247.10.11/bsd/netinet/ip.h.auto.html)<br>
[Apple's tcp.h documentation](https://opensource.apple.com/source/xnu/xnu-1504.9.17/bsd/netinet/tcp.h.auto.html)<br>
[Documentation on sockaddr_in struct](https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html)<br>

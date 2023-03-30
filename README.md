# Compression Detection
Based on the techniques described in [End-to-End Detection of Compression of Traffic Flows by Intermediaries](https://www.cs.usfca.edu/vahab/resources/compression_detection.pdf) by Vahab Pournaghshband, Alexander Afanasyev, and Peter Reiher.

## Requirements
Requires cJSON source and header files (included). See Resources.<br>
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
- **udp_timeout:** timeout for receiving UDP packets in the server application (must be shorter than the inter measurement time so as to not receive train 2 packets as train 1 packets, but not too short that the socket times out before the second train can be sent, around 3/5 the inter measurement time is good).
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
**Client TCP source port:** in the client and server application, the OS decides on the TCP port for the client's TCP connection request. All other ports are decided by what is defined in the configuration file.

**Compression detection:** when checking for compression using the delta times for low and high entropy trains, the server only checks if the *high entropy delta - low entropy delta > threshold*. The absolute value is not considered here because if the low entropy time is greater than the high entropy time then there must not be compression anyways.

**Receiving UDP packets:** when receiving UDP packets in the client and server application, the server does not check what percentage or range of UDP packets it received. The server is able to parse the UDP packet ids, however, after receiving them, the server simply moves on to the compression calculations. This may not be optimal in cases where only a small range of UDP packets are received. For example, if we only received packets 1000 - 2000 from the low entropy train and packets 1000 - 6000 from the high entropy train this will not be an accurate comparison of delta times.

**Receiving RST packets:** when receiving RST packets in the standalone application, we are assuming that the head and tail RST packets arrive in order and thus we are not checking the port numbers of the packets. It would be better design to check the port numbers in case of delayed responses.

**RST timeout:** since this is a raw socket, a normal socket timeout option for RST packets will not work here since we are receiving packets other than RST packets. As a result, once the program begins receiving packets in its receive thread, it continuously checks to see if its timer has passed the defined RST timeout range. Every time a new RST packet comes in (for a total of 4 RST packets), the timer is reset.

## Future Work
Instead of relying on the user to give a proper UDP timeout value in the configuration file, make this timeout a function of the inter measurement time.

Memory leaks have not been extensively checked and when the program fails, the memory is not freed on error.<br>
Need to ensure that all memory is freed.

Need to check that all `perror` calls are in response to the most recent errno.

## Resources
[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/)<br>
[DaveGamble's cJSON](https://github.com/DaveGamble/cJSON)<br>
[P.D. Buchan's raw sockets](https://www.pdbuchan.com/rawsock/tcp4.c)<br>
[MaxXor's raw sockets](https://github.com/MaxXor/raw-sockets-example/blob/master/rawsockets.c)<br>
[Apple's ip.h documentation](https://opensource.apple.com/source/xnu/xnu-3247.10.11/bsd/netinet/ip.h.auto.html)<br>
[Apple's tcp.h documentation](https://opensource.apple.com/source/xnu/xnu-1504.9.17/bsd/netinet/tcp.h.auto.html)<br>
[Documentation on sockaddr_in struct](https://www.gta.ufrj.br/ensino/eel878/sockets/sockaddr_inman.html)<br>

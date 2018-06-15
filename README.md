# network-test-packet-delay
This is used to test the delay on the road of network packets from one side to another side.
Two hostes is used, they are linked with two path, one is the fast path which is used to do sync of time.
Another path is the path to be tested, for example, LTE network.
First, client send a sync message to server through fast path, then client send udp messages with timestamp, 
server will print the delay of packets in path.

1. Configure 

struct PacketsDelay {
    char server_ip_test[20];
    char server_ip_sync[20];
    int debug;
    int port;
    int packetLen;
    int delayus;
}

server_ip_test: the server host ip in the path to be tested
server_ip_sync: the server host ip used to do sync of time

2. Run

server:
#packets_delay server

client:
#packets_delay client

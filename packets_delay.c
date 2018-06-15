#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>


#define BUFLEN 2048
#define TIMESYNC_INDEX 't'
#define TIME_MSG       3
struct PacketsDelay {
    char server_ip_test[20];
    char server_ip_sync[20];
    int debug;
    int port;
    int packetLen;
    int delayus;
}gPktDly = {"127.0.0.1", "127.0.0.1", 0, 8889, 66, 10000};

void client()
{
    int count;
    unsigned char ptmp[BUFLEN];

    struct sockaddr_in udp_si_other;
    struct sockaddr_in udp_si_time;
    int udp_socket, udp_slen = sizeof(udp_si_other);
    struct timeval val;
    int timems;

    if((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        fprintf(stderr, "Failed create udp socket\n");
        return;
    }

    memset(&udp_si_other, 0, udp_slen);
    udp_si_other.sin_family = AF_INET;
    udp_si_other.sin_port = htons(gPktDly.port);
    if(inet_aton(gPktDly.server_ip_test, &udp_si_other.sin_addr) == 0){
        fprintf(stderr, "Failed inet_aton\n");
        return;
    }

    memset(&udp_si_time, 0, udp_slen);
    udp_si_time.sin_family = AF_INET;
    udp_si_time.sin_port = htons(gPktDly.port);
    if(inet_aton(gPktDly.server_ip_sync, &udp_si_time.sin_addr) == 0){
        fprintf(stderr, "Failed aton\n");
        return;
    }

    memset(ptmp, 0, BUFLEN);

    ptmp[0] = TIMESYNC_INDEX;
    sendto(udp_socket, ptmp, 10, 0, (const struct sockaddr *)&udp_si_time, udp_slen);
    val.tv_sec = 0;
    val.tv_usec = 0;
    settimeofday(&val, NULL);

    /* Read message from kernel */
    ptmp[0] = TIME_MSG;
    while(1)
    {
        gettimeofday(&val, NULL);
        timems = (val.tv_sec & 0x07) * 1000 + val.tv_usec / 1000;
        ptmp[1] = (timems >> 16)&0xff;
        ptmp[2] = (timems >> 8)&0xff;
        ptmp[3] = (timems)&0xff;

        if((count = sendto(udp_socket, ptmp, gPktDly.packetLen, 0, (const struct sockaddr *)&udp_si_other, udp_slen)) == -1)
        {
            fprintf(stderr, "failed to send: %d %s\n", count, strerror(errno));
            return;
        }
        usleep(gPktDly.delayus);
    }

    close(udp_socket);
    return;
}

void server()
{
    struct sockaddr_in si_me, si_other;
    int s, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];

    struct timeval val;
    int timems, timemsold, delta;
    int count = 0;

    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
            fprintf(stderr, "create socket failed");
            return;
    }
    
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
    
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(gPktDly.port);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        fprintf(stderr, "error bind");
        return;
    }
    
    //keep listening for data
    while(1)
    {
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            fprintf(stderr, "error recvfrom()");
            return;
        }
            
        if(gPktDly.debug)
        {
            int i;

            fprintf(stdout, "Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
            for(i = 0; i < recv_len; i ++)
            {
                fprintf(stdout, "%02x ", (unsigned char)buf[i]);
                if((i + 1)%16 == 0)
                {
                    fprintf(stdout, "\n");
                }
            }
            fprintf(stdout, "\n");
        }
        if(buf[0] == TIME_MSG)
        {
            timemsold = ((buf[1] << 16)&0xff0000) | ((buf[2] << 8) & 0xff00) | (buf[3] & 0xff);

            gettimeofday(&val, NULL);
            delta = (val.tv_sec & 0x07) * 1000 + val.tv_usec/1000 - timemsold;
            fprintf(stdout, "%d ", delta);
            if((count + 1) % 40 == 0)
            {
                fprintf(stdout, "\n");
            }
            count ++;
        }
        else if(buf[0] == TIMESYNC_INDEX)
        {
            val.tv_sec = 0;
            val.tv_usec = 0;
            settimeofday(&val, NULL);
            fprintf(stdout, "Time Sync\n");
        }
    }

    close(s);
    return;

}

int main(int argc, const char * argv[])
{
    if(argc < 2)
        goto incorrect;

    if(strcmp(argv[1], "client") == 0)
    {
        client();
    }
    else if(strcmp(argv[1], "server") == 0)
    {
        server();
    }
    else
    {
        goto incorrect;
    }

    return 0;

incorrect:
    fprintf(stderr, "Usage: %s client|server\n", argv[0]);
    return -1;
}


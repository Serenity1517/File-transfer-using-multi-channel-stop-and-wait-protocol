#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#define SV_PORT_NO 8000


#define PACKET_SIZE 100
#define PACKET_DROP_RATE 10
#define RETRANSMISSION_TIME 5
#define BUFFER_LENGTH 1000


/*This is the basic structure of a packet being transmitted over the network*/
typedef struct Packet{
    //below field represents the data being transferred by the packet
    char packetData[PACKET_SIZE+1];
    //below field represents length (no. of bytes) of data present in the packet
    int dataLength;
    //below field tells us whether the packet is an ACK packet(true) or not(false)   
    bool isACK;
    //below field tells us whether the packet is the last packet or not.. used for terminating the listening by server
    bool isFinalPkt;
    //below field represents the sequence number of the packet
    int sequenceNumber;        
    //below field represents the channel ID (0 or 1) through which data has been transmitted
    int channelID;
}Packet;

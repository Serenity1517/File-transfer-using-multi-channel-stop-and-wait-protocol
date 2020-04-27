#include "packet.h"

int main() {
    int s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s<0){
        printf("\nsocket not created!!\n");
        exit(0);
    }
    printf("\nsocket formed!!\n");

    int trav, currLen;

    struct sockaddr_in svAddr;
    memset(&svAddr, 0, sizeof(svAddr));
    svAddr.sin_family=AF_INET;
    svAddr.sin_port=htons(SV_PORT_NO);
    svAddr.sin_addr.s_addr=htonl(INADDR_ANY);

    int b=bind(s,(struct sockaddr*)&svAddr, sizeof(svAddr));
    if(b<0){
        printf("\ncould not bind socket to server address!!!\n"); exit(0);
    }

    int l=listen(s,10);
    if(l<0) {
        printf("\ncould not listen on socket!!\n"); exit(0);
    }
    
    Packet tempStorage[100];
    currLen=-1;

    int clientLen = sizeof(clAddr);
    int* sockChannel;
    struct sockaddr_in clAddr;
    sockChannel = (int*)malloc(sizeof(int)*2);
    sockChannel[0] = accept(s, (struct sockaddr*)&clAddr, &clientLen);
    sockChannel[1] = accept(s, (struct sockaddr*)&clAddr, &clientLen);

    while(true) {
        bool terminated = false;
        Packet* incoming;
        bool* received;
        //int sockChannel = accept(s, (struct sockaddr*)&clAddr, &clientLen);
        struct timeval timeVal;
        timeVal.tv_sec=20;
        timeVal.tv_usec=0;
        
        fd_set sockets;
        FD_ZERO(&sockets);
        int maxsock=0;
        FD_SET(sockChannel[0], &sockets);
        if (sockChannel[0] > maxsock) maxsock=sockChannel[0];
        FD_SET(sockChannel[1], &sockets);
        if (sockChannel[1] > maxsock) maxsock=sockChannel[1];

        int waiting=select(maxsock + 1, &sockets, NULL, NULL, &timeVal);
        if(waiting==0)
            break;
        received=(bool*)malloc(sizeof(bool)*100);
        int n, i;
        for(i=0; i<2; i++){
            srand(time(0)); 
            if(FD_ISSET(sockChannel[i], &sockets)){
                incoming=(Packet*)malloc(sizeof(Packet));
                n=recv(sockChannel[i], incoming, sizeof(*incoming), 0);
                if(n==0){
                    printf("\nclient has terminated connection!!\n");
                    terminated=true;
                    break;
                }
                if(received[incoming->sequenceNumber/PACKET_SIZE] == true)
                    continue;
                printf("RCVD PKT: Seq. no %d of size %d Bytes from channel %d\n",incoming->sequenceNumber, PACKET_SIZE, i);
                int temp=rand()%PACKET_DROP_RATE;
                if(temp==0){
                    //printf("Dropped packet with seq no - %d \n\n", incoming->sequenceNumber);
                    continue;
                }
                received[incoming->sequenceNumber/PACKET_SIZE]=true;
                tempStorage[(incoming->sequenceNumber/PACKET_SIZE)]=*incoming;
                if(incoming->sequenceNumber/PACKET_SIZE>currLen){
                    currLen=incoming->sequenceNumber/PACKET_SIZE;
                }
                Packet* ACK = (Packet*)malloc(sizeof(Packet));
                ACK->isACK=true;
                ACK->sequenceNumber = incoming->sequenceNumber;
                printf("SENT ACK: for PKT with Seq No. %d from channel %d\n", incoming->sequenceNumber,i);
                printf("\n");
                int temp2 = send(sockChannel[i], ACK, sizeof(*ACK), 0);
                if(temp2 != sizeof(*ACK)){
                    printf("\nerror during message transmission to client\n");
                    exit(0);
                }
                if(incoming.isFinalPkt == true)
                    break;
            } 
        }
        if (clientLen < 0) {printf ("Error in client socket"); exit(0);}
        if(terminated == true){
            break;
        }                
    }
    //printf("\nOutside While\n");
    printf("writing to output file output.txt");
    FILE* fileptr;
	fileptr = fopen("Output.txt", "w");
    trav = 0;
    while(trav <= currLen){
        fprintf(fileptr,"%s", tempStorage[trav].packetData);
        trav++;
    }
    fclose(fileptr);
    close(s);
    printf("Ending Server\n");
}

                
            
            

#include "packet.h"

int lastSeq1, lastSeq2, lastAck1, lastAck2;
Packet* prevNotACKed, prevNotACKed2;
FILE* fileptr;
int currCount = 0;
int currBuffCount = -1;
char inputBuffer[BUFFER_LENGTH+1];

FILE* getFileStream(FILE* fileptr, char* buff, int buff_len){
    int lenOfInputRead;
    if(fileptr==NULL)
        exit(1);
    if(feof(fileptr)){
        fclose(fileptr);
        return NULL;
    }
    if((lenOfInputRead=(fread(buff, sizeof(char), buff_len, fileptr)))>0){
        buff[lenOfInputRead]='\0';
        return fileptr;
    }
    else {
        fclose(fileptr);
        return NULL;
    }
}

void inputIntoBuffer(){
    int i;
    for(i=0;i<BUFFER_LENGTH + 1;i++)
        inputBuffer[i] = '\0';
    fileptr = getFileStream(fileptr,inputBuffer,BUFFER_LENGTH);
    currBuffCount++;
}

Packet* createPacket(int dataLength, int sequenceNumber, int channelID){
    Packet* newPacket = (Packet*)malloc(sizeof(Packet));
    newPacket->dataLength = dataLength;
    newPacket->isACK = false;
    newPacket->channelID = channelID;
    int i = currCount;
    newPacket->sequenceNumber = sequenceNumber + BUFFER_LENGTH*(currBuffCount);
    for(i=currCount;i<(currCount + PACKET_SIZE);i++)
        newPacket->packetData[i-currCount] = inputBuffer[i];
    return newPacket;
}

bool compute()
{
    if(currCount == BUFFER_LENGTH){
        inputIntoBuffer();
        currCount = 0;
    }
    else if(inputBuffer[currCount]=='\0')
        return true;
    return false;
}

int main()
{
    int* s;
    s = (int*)malloc(sizeof(int)*2);
    s[0] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s[0] == -1)
        printf("\nfailed to create channel 1 socket!!\n");

    s[1] = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s[1] == -1)
        printf("\nfailed to create channel 2 socket!!\n");
    struct sockaddr_in svAddr;
    memset (&svAddr,0,sizeof(svAddr));

    svAddr.sin_family = AF_INET;
    svAddr.sin_port = htons(SV_PORT_NO);
    svAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    printf("\nAddress assigned to server\n");
    
    int temp1 = connect (s[0], (struct sockaddr*) &svAddr , sizeof (svAddr));
    if(temp1 < 0)
        printf("\nerror in connecting for channel 1!!\n");

    int temp2 = connect (s[1], (struct sockaddr*) &svAddr , sizeof (svAddr));
    if(temp2 < 0)
        printf("\nerror in connecting for channel 2!!\n");
    
    printf("Connection established successfully\n");
    int check = 0; 
    int state = 0;
    fileptr = fopen("input.txt","rb");
    inputIntoBuffer();
    while(true){   
        switch(state){
            case 0:{
                Packet* newPkt1 = createPacket(PACKET_SIZE, currCount, 0);
                lastSeq1 = currCount;
                currCount += PACKET_SIZE;
                if(compute() == true){
                	newPkt1->isFinalPkt = true;
                    check = 1;
                }
                int lenSent1 = send(s[0], newPkt1, sizeof(*newPkt1), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", newPkt1 -> sequenceNumber,PACKET_SIZE,0);
                prevNotACKed = newPkt1;
                if(check == 1){
                	break;
                }
                Packet* newPkt2 = createPacket(PACKET_SIZE, currCount, 1);
                lastSeq2 = currCount;
                currCount += PACKET_SIZE;
                if(compute() == true){
                	newPkt2->isFinalPkt = true;
                    check = 2;
                }
                int len_sent2 = send(s[1],newPkt2,sizeof(*newPkt2),0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", newPkt2 -> sequenceNumber,PACKET_SIZE,1);
                prevNotACKed2 = newPkt2;
                state = 1;
                break;
            }        
            case 1:{
            	fd_set currSetOfSocks;
				FD_ZERO(&currSetOfSocks);
				int tempMaxSocket =0;
				int i;
				for(i=0; i<2; i++){
					FD_SET(s[i], &currSetOfSocks);
					if (s[i] > tempMaxSocket) tempMaxSocket = s[i];
				}
                struct timeval timeVal;
                timeVal.tv_sec = RETRANSMISSION_TIME;
                timeVal.tv_usec = 0;
                int waiting = select(tempMaxSocket + 1, &currSetOfSocks, NULL, NULL, &timeVal);
                if(waiting == 0){
                    state = 4;
                    break;
                }
                Packet* inPacket;
                Packet* pkts[2];
                pkts[0] = NULL;
                pkts[1] = NULL;
                int n;
                for(i=0; i<2; i++){
                    if(FD_ISSET(s[i], &currSetOfSocks)){
                        inPacket = (Packet*)malloc(sizeof(Packet));
                        n = recv(s[i], inPacket, sizeof(*inPacket), 0);
                        if(i == 0){
                            pkts[0] = inPacket;
                            lastAck1 = inPacket -> sequenceNumber;
                            printf("RCVD ACK : For packet with Sequence No %d from channel : %d\n",lastAck1,i);
                        }
                        else{
                            pkts[1] = inPacket;
                            lastAck2 = inPacket -> sequenceNumber;
                            printf("RCVD ACK : For packet with Sequence No %d from channel : %d\n",lastAck2,i);
                        }
                    }
                }
                if(pkts[0] == NULL && pkts[1] == NULL){
                    state = 4;
                }
                else if(pkts[0] == NULL){
                    state = 3;
                }
                else if(pkts[1] == NULL){
                    state = 2;
                }
                else{
                    state = 0;
                }
                break;
            }
            case 2:{
                Packet* newPkt1 = createPacket(PACKET_SIZE, currCount, 0);
                lastSeq1 = currCount;
                currCount += PACKET_SIZE;
                state = 1;
                if(compute() == true){
                 	newPkt1->isFinalPkt = true;   
                    check = 2;
                }
                int lenSent1 = send(s[0], newPkt1, sizeof(*newPkt1), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", newPkt1 -> sequenceNumber,PACKET_SIZE,0);
                prevNotACKed = newPkt1;
                break;
            }
            case 3:{
                Packet* newPkt2 = createPacket(PACKET_SIZE, currCount, 0);
                lastSeq2 = currCount;
                currCount += PACKET_SIZE;
                state = 1;
                if(compute() == true){
                    newPkt2->isFinalPkt = true;
                    check = 2;
                }
                
                int len_sent2 = send(s[1], newPkt2, sizeof(*newPkt2), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", newPkt2 -> sequenceNumber,PACKET_SIZE,1);
                if(len_sent2 == -1){
                    printf("\n-1 bytes sent in channel 2 in case 3\n");
                }
                prevNotACKed2 = newPkt2;
                break;
            }
            case 4:{ 
                int lenSent1 = send(s[0], prevNotACKed, sizeof(*prevNotACKed), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", prevNotACKed -> sequenceNumber,PACKET_SIZE,0);
                int len_sent2 = send(s[1], prevNotACKed2, sizeof(*prevNotACKed2), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", prevNotACKed2 -> sequenceNumber,PACKET_SIZE,1);
                if(len_sent2 == -1){
                    printf("\n-1 bytes sent in channel 2 in case 4\n");
                }
                state = 1;
                break;
            }
        }

        if(check == 1){
            fd_set currSetOfSocks;
            FD_ZERO(&currSetOfSocks);
            int tempMaxSocket = 0;
            int i;
            for(i=0; i<1; i++){
                FD_SET(s[i], &currSetOfSocks);
                if (s[i] > tempMaxSocket) tempMaxSocket = s[i];
            }
            struct timeval timeVal;
            timeVal.tv_sec = RETRANSMISSION_TIME;
            timeVal.tv_usec = 0;
            int waiting = select(tempMaxSocket + 1, &currSetOfSocks, NULL, NULL, &timeVal);
            while(waiting == 0){
                int lenSent1 = send(s[0], prevNotACKed, sizeof(*prevNotACKed), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", prevNotACKed -> sequenceNumber,PACKET_SIZE,0);
                for(i=0; i<1; i++){
                    FD_SET(s[i], &currSetOfSocks);
                    if (s[i] > tempMaxSocket) tempMaxSocket = s[i];
                }
                struct timeval tv1;
                tv1.tv_sec = RETRANSMISSION_TIME;
                tv1.tv_usec = 0;
                waiting = select(tempMaxSocket + 1, &currSetOfSocks, NULL, NULL, &tv1);
            }
            printf("RCVD ACK : For packet with Sequence No %d from channel : 0\n",prevNotACKed->sequenceNumber);
        }
        else if(check == 2){
            fd_set currSetOfSocks;
            FD_ZERO(&currSetOfSocks);
            int tempMaxSocket = 0;
            int i;
            for(i=0; i<1; i++){
                FD_SET(s[i], &currSetOfSocks);
                if (s[i] > tempMaxSocket) tempMaxSocket = s[i];
            }
            struct timeval timeVal;
            timeVal.tv_sec = RETRANSMISSION_TIME;
            timeVal.tv_usec = 0;
            int waiting = select(tempMaxSocket + 1, &currSetOfSocks, NULL, NULL, &timeVal);
            while(waiting == 0){
                int lenSent1 = send(s[0], prevNotACKed, sizeof(*prevNotACKed), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", prevNotACKed -> sequenceNumber,PACKET_SIZE,0);
                for(i=0; i<1; i++){
                    FD_SET(s[i], &currSetOfSocks);
                    if (s[i] > tempMaxSocket) tempMaxSocket = s[i];
                } 
                struct timeval tv1;
                tv1.tv_sec = RETRANSMISSION_TIME;
                tv1.tv_usec = 0;
                waiting = select(tempMaxSocket + 1, &currSetOfSocks, NULL, NULL, &tv1);
            }
            printf("RCVD ACK : For packet with Sequence No %d from channel : 0\n",prevNotACKed->sequenceNumber);
            fd_set sockets2;
            FD_ZERO(&sockets2);
            int maxsock2 = 0;
            for(i=1; i<2; i++){
                FD_SET(s[i], &sockets2);
                if (s[i] > maxsock2) maxsock2 = s[i];
            }
            struct timeval timeVal2;
            timeVal2.tv_sec = RETRANSMISSION_TIME;
            timeVal2.tv_usec = 0;
            waiting = select(maxsock2 + 1, &sockets2, NULL, NULL, &timeVal2);
            while(waiting == 0){
                int len_sent2 = send(s[1], prevNotACKed2, sizeof(*prevNotACKed2), 0);
                printf("SENT PKT : Seq no %d of size %d bytes from channel %d \n", prevNotACKed2 -> sequenceNumber,PACKET_SIZE,1);
                if(len_sent2 == -1)
                {
                    printf("\n-1 Bytes sent in channel 1 in case 4\n");
                }
                for(i=1; i<2; i++){
                    FD_SET(s[i], &sockets2);
                    if (s[i] > maxsock2) maxsock2 = s[i];
                }
                struct timeval tv3;
                tv3.tv_sec = RETRANSMISSION_TIME;
                tv3.tv_usec = 0;
                waiting = select(maxsock2 + 1, &sockets2, NULL, NULL, &tv3);
            }
            printf("RCVD ACK : For packet with Sequence No %d from channel : 1\n",prevNotACKed2->sequenceNumber);
        }
        break;
    }
    return 0;
}

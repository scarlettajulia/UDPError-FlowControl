/*
* File : T1_rx.cpp
*/
#include "dcomm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include <pthread.h>

/* Delay to adjust speed of consuming buffer, in milliseconds */
#define DELAY 500

/* Define receive buffer size */
#define RXQSIZE 8
#define MIN_UPPER (RXQSIZE*0.5)

char buffer[2]; //for sending XON/XOFF signal
struct sockaddr_in serv_addr; //sockaddr for server
struct sockaddr_in cli_addr; //sockaddr for client
int cli_len= sizeof(cli_addr); // sizeof client address

Byte rxbuf[RXQSIZE];
QTYPE rcvq = { 0, 0, 0, RXQSIZE, rxbuf };
QTYPE *rxq = &rcvq;
Byte sent_xonxoff = XON;
bool send_xon = false,
	 send_xoff = false;

/* Socket */
int sockfd; // listen on sock_fd

/* Functions declaration */
static Byte *rcvchar(int sockfd, QTYPE *queue, int *j);
static Byte *q_get(QTYPE *);
static void *consume(void *param); // Thread function

using namespace std;

//main program
int main(int argc, char *argv[])
{
	printf("mulai program\n");

	Byte c;

	//membuat socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd <0)
		printf("socket error\n");
	else
		printf("socket OK\n");
		
	int port = atoi(argv[1]);

	//Insert code here to bind socket to the port number given in argv[1].
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; //assign UDP
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //assign server address
	serv_addr.sin_port = htons(port); //assign port number

	//binding
	if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))< 0)
		perror("binding gagal\n");
	else{
		//int portest = serv_addr.sin_port;
		printf("%s%s%s%d\n","Binding pada: ",inet_ntoa(serv_addr.sin_addr),":",port);
	}

	/* Initialize XON/XOFF flags */
	sent_xonxoff = XON;

    /* Membuat thread baru*/
    pthread_t cons_t;
    
    //Mengkonsumsi thread
    if(pthread_create(&cons_t, NULL, consume, &rcvq)){
        fprintf(stderr, "Gagal membuat thread\n");
        return 1;
    }
    
    // menerima thread
    printf("'Menerima thread\n");
    int j=1;
    while (true) {
        c = *(rcvchar(sockfd, &rcvq, &j));
        
        /* Quit on end of file */
        /*if (int(c) == Endfile) {
         exit(0);
         }*/
        j++;
    }
    
    //menggabungkan thread
    if(pthread_join(cons_t,NULL)){
        fprintf(stderr,"Error menggabungkan thread\n");
        return 2;
    }
    
    return 0;
}
static Byte *rcvchar(int sockfd, QTYPE *queue, int *j)
{
    
    int r; //Hasil pengiriman sinyal ke server
    if(queue->count == RXQSIZE && !send_xoff){ // jika buffer penuh =, kirim xoff
        printf("buffer full\n");
        send_xon = false; //set xon
        send_xoff = true; //set xoff
        sent_xonxoff = XOFF; //meng-assign signal
        
        buffer[0]=sent_xonxoff;
        r = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        
        //mengecek error on signal
        if(r>0)
            printf("xoff signal sent\n");
        else
            printf("xoff signal failed\n");
    }
    
    //menunggu karakter berikutnya
    char temp[1];
    r = recvfrom(sockfd, temp, RXQSIZE, 0, (struct sockaddr*) &cli_addr, (socklen_t*) &cli_len);
    
    //mengecek recvfrom error
    if(r < 0){
        printf("error recvfrom\n");
    }
    
    // mengecek end of file
    if(int(temp[0]) != Endfile){
        printf("Menerima byte ke-%d.\n",*j);
        *j++;
    }
    else
        *j=0; //reset j
    
    //mengupdate queue
    queue->data[queue->rear]=temp[0];
    queue->rear++;
    queue->rear = (queue->rear) % RXQSIZE;
    (queue->count)++;
    
    return queue->data;
}

/* q_get returns a pointer to the buffer where data is read or NULL if
 * buffer is empty.
 */
static Byte *q_get(QTYPE *queue)
{
    Byte *current;
    
    /* Nothing in the queue */
    if (!queue->count) {
        return (NULL);
    }
    current = &(queue->data[queue->front]);
    //printf("b\n");
    
    //update queue attr
    queue->front++;
    queue->front = (queue->front) % RXQSIZE;
    (queue->count)--;
    //mengirim XON signal
    if(queue->count < MIN_UPPER && !send_xon){
        send_xon = true; //set xon flag
        send_xoff = false; //set xoff flag
        sent_xonxoff= XON;
        
        int x;
        buffer[0]=sent_xonxoff;
        x = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, sizeof(cli_addr));
        
        //mengecek error sinyal
        if(x>0)
            printf("Mengirim XOn\n");
        else
            printf("Pengiriman signal xon gagal\n");
    }
    return current;
}

// Thread function (mengkonsumsi pesan)
static void *consume(void *param){
    printf("'consume' thread\n");
    QTYPE *rcvq_ptr = (QTYPE *)param;
    
    int i=1; //character index
    while (true) {
        Byte *res;
        res = q_get(rcvq_ptr);
        if(res){
            printf("ascii code: %d\n",int(*res));
            printf("Mengkonsumsi byte ke-%d : %c\n",i,*res);
            i++;
        }
        //mendelay konsum pesan
        usleep(100000); //delay
    }
    pthread_exit(0);
}

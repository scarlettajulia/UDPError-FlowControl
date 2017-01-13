/*
* File : transmitter.cpp
*/
#include "dcomm.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>

using namespace std;

struct sockaddr_in myaddress;
int sock_fd;
void *sendsignal(void *);

int main (int argc, char *argv[]){
	char last;
	last = XON;
	//cek format input
	if (argc != 4){
		printf("Input format yang benar\n");
		printf("Format: ./transmitter <IP Address> <Port Number> <File Name>\n");
	}
	else {

		char* fname = argv[3];
        //melakukan koneksi ke socket
		sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock_fd < 0){
			perror("gagal membuat socket");
			exit(1);
		} else {
			printf("Membuat socket untuk koneksi ke %s:%s ...\n", argv[1], argv[2]);
		}

        //memastikan address kosong
		memset((char*)&myaddress, 0, sizeof(myaddress));
		myaddress.sin_family = AF_INET;
		myaddress.sin_addr.s_addr = inet_addr(argv[1]);
		myaddress.sin_port = htons(atoi(argv[2]));

        //membuat thread untuk mengirim sinyal
		pthread_t thread;
		int rc;
		rc = pthread_create(&thread, NULL, &sendsignal, NULL);

		if (rc){
			printf ("Gagal membuat thread\n");
			exit(1);
		}
		printf("masuk");

        //mengambil file eksternal
		FILE * pFile;
		int c;
		int i = 1;
		pFile=fopen (fname,"r");
		if (pFile==NULL) {
			perror ("Gagal membuka file\n");
			exit(1);
		}
		else {
			do {
				c = fgetc (pFile);
				while (last == XOFF){
					printf("Menunggu XON...\n");
				}
				printf("Mengirim byte ke-%d: '%c'\n",i,c);
				if (sendto(sock_fd, (char *) &c, 1, 0, (struct sockaddr *) &myaddress, sizeof(myaddress))<0) {
					perror("Gagal mengirim");
				}
				usleep(100000);
				i++;
			} while (c != EOF);
			fclose (pFile);
		}
	}
	return 0;
}

//prosedur untuk menerima sinyal XON dan XOFF
void *sendsignal(void *){
	char buf;
	while (1){
		int servlen = sizeof(myaddress);
		int receivefr = recvfrom(sock_fd, (char *)&buf, 1, 0 ,(struct sockaddr *)&myaddress, (socklen_t *) &servlen);
		if (receivefr < 0) {
			perror("Gagal menerima");
			exit(1);
		}
		if (buf == XOFF){
			printf("XOFF diterima\n");
		}
		else {
			printf("XON diterima\n");
		}
	}
	pthread_exit(NULL);
}


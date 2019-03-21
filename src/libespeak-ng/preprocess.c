#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PORT 6135    // the port client will be connecting to
#define HOSTNAME "localhost"
#define ERROR -1

#define BUFFERSIZE 1024 // max size of buffer


/*- This function provides integration with external tools
 * Currently uses system call use GNU tools echo and tr to change "a" into "e"
 * Which writes output in file, which is read back into passed buffer
 *
 * This file can be compiled separately from espeak-ng with command:
 * gcc -DSTANDALONE -o preprocess.o preprocess.c
 *
 * and then executed as stand alone executable with
 * ./preprocess.o "passed arguments"
 */
void preprocessText(char **data) {
	printf(">preprocessText\n");
	printf("data:%s\n", *data);
	int sockfd;

	struct hostent *he;
	struct sockaddr_in sockedAddr; // connector's address information
	char hostname[50] = HOSTNAME;

	if ((he = gethostbyname(hostname)) == NULL) { // get the host info
		herror("gethostbyname");
		return;
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		return;
	}

	sockedAddr.sin_family = AF_INET; // host byte order
	sockedAddr.sin_port = htons(PORT); // short, network byte order
	sockedAddr.sin_addr = *((struct in_addr *) he->h_addr);
	bzero(&(sockedAddr.sin_zero), 8); // zero the rest of the struct

	if (connect(sockfd, (struct sockaddr *) &sockedAddr,
			sizeof(struct sockaddr)) == -1) {
		perror("connect");
		return;
	}

	char bufSend[BUFFERSIZE + 1] = { 0 }; // reserve last byte for null terminator (for debugging purposes)
	int sentPacketBytes;
	int sentBytes = 0;
	do {
		//memset(bufSend, 0, sizeof(bufSend));
		strncpy(bufSend, (*data) + sentBytes, BUFFERSIZE); // copy part of data into send buffer
		printf("bufSend:%s\n", bufSend);
		sentPacketBytes = send(sockfd, bufSend, BUFFERSIZE, 0);
		if (sentPacketBytes == ERROR) {
			perror("send() error");
			return;
		}
		printf("sentBytes:%d bufSend:%s\n", sentBytes, bufSend);
		sentBytes += sentPacketBytes;
	} while (bufSend[BUFFERSIZE - 1] > 0);
	printf("%d bytes sent\n", sentBytes);

	char bufRecieve[BUFFERSIZE + 1] = { 0 };
	int recvPacketBytes;
	int recvBytes = 0;
	do {
		recvPacketBytes = recv(sockfd, bufRecieve, BUFFERSIZE, 0);
		if (recvPacketBytes == ERROR) {
			perror("receive() error");
			return;
		}
		strncpy((*data) + recvBytes, &bufRecieve[0], recvPacketBytes); // copy data from received buffer to data buffer
		recvBytes += recvPacketBytes;
	} while (bufRecieve[BUFFERSIZE - 1] > 0);
	printf("%d bytes received\n", recvBytes);
	close(sockfd);
	printf("data:%s\n", *data);
	printf("<preprocessText\n");
}

/*
 * This function is used for testing as stand-alone executable
 */
#ifdef STANDALONE
int main(int argc, char **argv) {
	char data[1024] = { 0 };
	char *dataref = &data[0];
	if (argc > 1)
		strcpy(data, argv[1]);
	printf("data before:%s\n",data);
	processData(&dataref);
	printf("data  after:%s\n", data);
}
#endif


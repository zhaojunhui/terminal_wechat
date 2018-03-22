#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

#define CLIENTPORT 3333
#define MAXDATASIZE 4096

char recvBuffer[MAXDATASIZE];
char sendBuffer[MAXDATASIZE];

char currentUsername[MAXDATASIZE];
char currentPassword[MAXDATASIZE];

bool logIn(int sockfd) {
	int num;
	printf("\nplease log in(register):\n");

	//printf("%lu",strlen(sendBuffer));
	printf("username:");
	//memset(sendBuffer, 0, sizeof(sendBuffer));
	fgets(sendBuffer, MAXDATASIZE, stdin);
	sendBuffer[strlen(sendBuffer) - 1] = '\0';
	strcpy(currentUsername,sendBuffer);
	//printf("%lu", strlen(sendBuffer));
	if(send(sockfd, sendBuffer, strlen(sendBuffer) + 1, 0) == -1) {
		perror("fail to send username");
		exit(1);
	}
	num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
	if(recvBuffer[0] == '0') {
		printf("username already used!\n");
		return 1;
	}
	printf("password:");
	memset(sendBuffer, 0, sizeof(sendBuffer));
	fgets(sendBuffer, MAXDATASIZE, stdin);
	sendBuffer[strlen(sendBuffer) - 1] = '\0';
	if(send(sockfd, sendBuffer, strlen(sendBuffer) + 1, 0) == -1) {
		perror("fail to send password");
		exit(1);
	}
	num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
	if(recvBuffer[0] == '0') {
		printf("password incorrect!\n");
		return 1;
	}
	strcpy(currentPassword, sendBuffer);
	return 0;
}

void help() {
	printf("welcome to wechat software, you can do following things:\n1. search : to find all clients\n2. sendmsg name \"message\": send message to your friend\n3. recvmsg: receive other's message to you\n4. profile: show your account's information\n5. exit : withdraw this software\n6. exit_server : shut down the remote server, if you are administor\n7. add_admin : add more administor if you are administor\n8. recvfile : receive file from others\n9. sendfile : send file to others\n10. help : see this help\n");
}

void showProfile() {
	printf("current username : %s\n", currentUsername);
	printf("password : %s\n", currentPassword);
}

bool online(int sockfd) {
	printf(">> ");
	memset(sendBuffer, '\0', sizeof(sendBuffer));
	fgets(sendBuffer, MAXDATASIZE, stdin);
	sendBuffer[strlen(sendBuffer) - 1] = '\0';
	char tmp[MAXDATASIZE];
	//printf("%s", sendBuffer);
	if(send(sockfd, sendBuffer, strlen(sendBuffer) + 1, 0) == -1) {
		perror("fail to send");
		exit(1);
	}

	if(sendBuffer == "help") {
		help();
		return 1;
	} else if(strcmp(sendBuffer, "search") == 0) {
		//printf("\nhaha\n");
		int num;
		num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
		printf("%s", recvBuffer);
		//printf("\nOK\n");
		return 1;
	} else if(strcmp(strncpy(tmp, sendBuffer, 7), "sendmsg") == 0) {
		int num;
		num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
		if(recvBuffer[0] == '1')
			printf("no username match!\n");
		else 
			printf("successfully send to other\n");
		return 1;
	} else if(strcmp(sendBuffer, "recvmsg") == 0) {
		//printf("gagahg");
		int num;
		num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
		printf("%s", recvBuffer);
		return 1;
	} else if(strcmp(sendBuffer, "profile") == 0) {
		showProfile();
		return 1;
	} else if(sendBuffer == "exit") {
		return 0;
	} else if(sendBuffer == "exit_server") {
		int num;
		num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
		if(recvBuffer[0] == '1') {
			printf("you are not administor, so you don't have permission to close server!\n");
			return 1;
		}
		else {
			printf("successfully close server\n");
			return 0;
		}
	} else if(strncpy(tmp, sendBuffer, 9) == "add_admin") {
		int num;
		num = recv(sockfd, recvBuffer, MAXDATASIZE, 0);
		if(recvBuffer[0] == '0') {
			printf("fail to add administor, please check your level or this account may not exist\n");
		} else {
			printf("successfully add this administor\n");
		}
		return 1;
	}
}

int main() {

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	char* serverInetAddr = "127.0.0.1";
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(CLIENTPORT);
	inet_pton(AF_INET, serverInetAddr, &sock_addr.sin_addr);
	bzero(&(sock_addr.sin_zero), sizeof(sock_addr.sin_zero));
	if(connect(sockfd, (struct sockaddr*) &sock_addr, sizeof(struct sockaddr)) == -1) {
		perror("fail to connect");
		exit(1);
	}
	printf("successfully connected to server!\n");
	printf("welcome to the weChat!\n");

	while(logIn(sockfd));

	help();

	while(online(sockfd));
	close(sockfd);

	printf("you have exit successfully!\n");

	/*
	int numbytes = 0;
	if((numbytes = recv(sockfd, buffer, MAXDATASIZE, 0)) == -1) {
		perror("fail to recieve data");
		exit(1);
	}
	buffer[numbytes] = '\0';
	printf("%s\n", buffer);
	close(sockfd);
	*/
	//while(1);
	return 0;
}
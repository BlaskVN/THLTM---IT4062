#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 255

char buffer[BUFFER_SIZE];
int length;

bool containsSpace(char *buffer){
    for (int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == ' ') {
            return true;
        }
    }
    return false;
}

void getPassword() {
    printf("--------------\nEnter to exit\n");
    while (true) {
        printf("Password: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if (buffer[0] == '\n') {
            printf("Exiting...\n");
            exit(1);
        }
        
        buffer[strlen(buffer) - 1] = '\0';

        if (containsSpace(buffer)) {
            printf("Password cannot contain spaces. Please try again.\n");
        } else {
            printf("Password is valid: %s\n", buffer);
            break;
        }
    }
}

void getUser() {
    printf("--------------\nEnter to exit\n");
    while (true) {
        printf("Insert username: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        if (buffer[0] == '\n') {
            printf("Exit programming\n");
            exit(1);
        }
        buffer[strlen(buffer) - 1] = '\0';

        if (containsSpace(buffer)) {
            printf("Username cannot contain spaces. Please try again.\n");
        } else {
            printf("Username is valid: %s\n", buffer);
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    int sockfd, rcvBytes, sendBytes;

    struct sockaddr_in servaddr;
    char *serv_IP;
    short serv_PORT;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ServerIP> <ServerPort>\n", argv[0]);
        exit(1);
    }
    serv_IP = argv[1];
    serv_PORT = atoi(argv[2]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: ");
        return 0;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(serv_IP);
    servaddr.sin_port = htons(serv_PORT);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }

    int isLogin = 0;
    do {
        if (isLogin == 0) {
            getUser();
            if (buffer[0] == '\n'){
            	 exit(1);
			}
               
            length = strlen(buffer);
            sendBytes = send(sockfd, buffer, length, 0);
            rcvBytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
            printf("Reply from server: %s\n", buffer);
            if (buffer[0] == 'C')
                continue;
            if (buffer[0] == 'A')
                continue;
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
        }

        while (isLogin == 0) {
            getPassword();
            length = strlen(buffer);
            sendBytes = send(sockfd, buffer, length, 0);
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
            rcvBytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
            printf("Reply from server: %s\n", buffer);
            if (strcmp(buffer, "OK") == 0) {
                isLogin = 1;
            }
            if (buffer[0] == 'P')
                break;
            if (buffer[0] == 'C')
                break;
            if (buffer[0] == 'A')
                break;
        }

        if (isLogin == 1) {
            printf("Send new password to server: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            if (buffer[0] == '\n'){
            	printf("Exit programing\n");
            	 exit(1);
			}
               	buffer[strlen(buffer) - 1] = '\0';
            length = 255; 
            sendBytes = send(sockfd, buffer, length, 0);
            if (sendBytes < 0) {
                perror("Error 1");
                return 0;
            }
            for (;;) {
                rcvBytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
                if (rcvBytes < 0) {
                    perror("Error 2");
                    return 0;
                }
                buffer[rcvBytes] = '\0';
                if (buffer[0] >= '0' && buffer[0] <= '9')
				{
					printf("Reply from server(only number): %s\n", buffer);
				}
				else if (strcmp(buffer, "error") == 0)
				{
					printf("Reply from server: %s\n", buffer);
				}
				else if (buffer[0] != 'c' && buffer[0] != 'G')
				{
					printf("Reply from server(only character): %s\n", buffer);
				}

				if (strcmp(buffer, "continue send message to server") == 0)
				{
					printf("%s\n", buffer);
					printf("--------------------\n");
					break;
				}
				if (buffer[0] == 'G')
				{
					printf("Reply from server %s\n", buffer);
					printf("Signout\n");
					printf("--------------------\n");
					isLogin = 0;
					break;
				}
            }
        }
    } while (1);

    close(sockfd);
    return 0;
}


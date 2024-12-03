#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <sys/select.h>

#define FILENAME "nguoidung.txt"
#define BUFF_SIZE 255
#define BACKLOG 20

fd_set readfds, allset;
socklen_t clilen;
int nready, client[FD_SETSIZE];
char buff[BUFF_SIZE];
char recvBuff[BUFF_SIZE];
char done[BUFF_SIZE];
char username[50], password[50];
unsigned int len = sizeof(struct sockaddr_in);
short serv_PORT;


typedef struct User {
    char username[50];
    char password[50];
    int status;
    int attempt;
    struct User *next;
} User;

User *head = NULL;
int flag[BACKLOG];
User *currentUser[BACKLOG];
int isLoggedIn[BACKLOG];
char loggedInUsername[BACKLOG][50];
int error[BACKLOG];

char number[BUFF_SIZE];
char alphabet[BUFF_SIZE];


void loadUsersFromFile() {
    FILE *file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    char line[150];
    while (head != NULL) {
        User *temp = head;
        head = head->next;
        free(temp);
    }
    while (fgets(line, sizeof(line), file)) {
        User *newUser = (User *) malloc(sizeof(User));
        sscanf(line, "%s %s %d", newUser->username, newUser->password, &newUser->status);
        newUser->attempt = 0;
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}

/*ghi thong tin ra file*/
void saveUsersToFile() {
    FILE *file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }

    User *current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d  \n", current->username, current->password, current->status);
        current = current->next;
    }
    fclose(file);
}

void processRecvBuff(const char *receive, int i) {
    int countNumber = 0, countAlphabet = 0;
    for (size_t k = 0; k < strlen(receive); k++) {
        char currentChar = receive[k];

        if (isdigit(currentChar)) {
            if (countNumber < BUFF_SIZE - 1) {
                number[countNumber++] = currentChar;
            }
        } else if (isalpha(currentChar)) {
            if (countAlphabet < BUFF_SIZE - 1) {
                alphabet[countAlphabet++] = currentChar;
            }
        } else {
            error[i] = 1;
            break;
        }
    }
    if (error[i] == 0) {
        strcpy(currentUser[i]->password, receive);
        saveUsersToFile();
    }
    number[countNumber] = '\0';
    alphabet[countAlphabet] = '\0';
}

/*Chuc nang kiem tra mat khau*/
void checkWrongAttempts(User *user, char *enteredPassword, int connfd, int i) {
    int sendSize;
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFF_SIZE, 0);
        isLoggedIn[i] = 1;
        flag[i] = 2;
        currentUser[i] = user;
        return;
    }
    user->attempt++;
    if (user->attempt >= 3) {
        user->status = 0;
        flag[i] = 0;
        saveUsersToFile();
        sendSize = send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
        if (sendSize <= 0) {
            FD_CLR(connfd, &allset);
            close(connfd);
            client[i] = -1;
            return;
        }
    } else {
        sendSize = send(connfd, "NOT OK", BUFF_SIZE, 0);
        if (sendSize <= 0) {
            FD_CLR(connfd, &allset);
            close(connfd);
            client[i] = -1;
            return;
        }
    }
}

/*Chuc nang tim kiem tai khoan*/
User *searchUser(char *username) {
    User *current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int isValidPort(const char *portStr) {
    // Check if all characters are digits
    for (int i = 0; portStr[i] != '\0'; ++i) {
        if (!isdigit(portStr[i])) {
            return 0;  // Not a digit
        }
    }

    // Convert to integer and check the range
    int port = atoi(portStr);
    if (port < 0 || port > 65535) {
        return 0;  // Out of range
    }

    // Check for leading zeros
    if (portStr[0] == '0' && portStr[1] != '\0') {
        return 0;  // Contains leading zeros
    }

    return 1;  // Valid port
}

int maxfd, maxi;

void handleDataFromClient(int connfd, int i) {
    memset(buff, '\0', sizeof(buff));
    memset(done, '\0', sizeof(done));

    int rcvSize = recv(connfd, recvBuff, BUFF_SIZE, 0); // receive message from client
    if (rcvSize <= 0) {
        printf("Client %d disconnected\n", connfd); // Print a message for debugging
        FD_CLR(connfd, &readfds);  // Remove the file descriptor from the set

        close(connfd);
        client[i] = -1;
        flag[i] = 0;
        error[i] = 0;
        isLoggedIn[i] = 0;
        currentUser[i] = NULL;

        return;
    }

    int sendSize;
    recvBuff[rcvSize] = '\0';
    printf("%s\n", recvBuff);
    if (flag[i] == 0) {
        strcpy(username, recvBuff);
        currentUser[i] = searchUser(username);
        if (currentUser[i] != NULL) {
            if (currentUser[i]->status == 1) {
                sendSize = send(connfd, "USER FOUND", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    FD_CLR(connfd, &allset);
                    close(connfd);
                    client[i] = -1;
                    return;
                }
                flag[i] = 1;
            } else if (currentUser[i]->status == 2) {
                flag[i] = 0;
                sendSize = send(connfd, "Account is not activated\n", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    FD_CLR(connfd, &allset);
                    close(connfd);
                    client[i] = -1;
                    return;
                }
            } else if (currentUser[i]->status == 0) {
                flag[i] = 0;
                sendSize = send(connfd, "Account is blocked\n", BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    FD_CLR(connfd, &allset);
                    close(connfd);
                    client[i] = -1;
                    return;
                }
            }
        } else {
            sendSize = send(connfd, "Cannot find account\n", BUFF_SIZE, 0);
            if (sendSize <= 0) {
                FD_CLR(connfd, &allset);
                close(connfd);
                client[i] = -1;
                return;
            }
        }
        return;
    } else if (flag[i] == 1) {
        strcpy(password, recvBuff);
        checkWrongAttempts(currentUser[i], password, connfd, i);
    } else {
        if (rcvSize < 0) {
            perror("Error: ");

        }
        recvBuff[rcvSize] = '\0';
        if (strcmp(recvBuff, "bye") == 0) {
            char str1[100] = "Goodbye";
            char str2[100];
            strcpy(str2, currentUser[i]->username);
            currentUser[i]->attempt = 0;
            char str3[100];
            strcpy(str3, str1);
            strcat(str3, str2);
            sendSize = send(connfd, str3, BUFF_SIZE, 0);
            if (sendSize <= 0) {
                FD_CLR(connfd, &allset);
                close(connfd);
                client[i] = -1;
                return;
            }
            flag[i] = 0;
            return;
        }
        printf("Receive from client: %s\n", recvBuff);
        processRecvBuff(recvBuff, i);
        if (error[i] == 1) {
            strcpy(buff, "error");
            sendSize = send(connfd, buff, BUFF_SIZE, 0);
            if (sendSize <= 0) {
                FD_CLR(connfd, &allset);
                close(connfd);
                client[i] = -1;
                return;
            }
            strcpy(done, "continue send message to server");
            send(connfd, done, BUFF_SIZE, 0);
            error[i] = 0;
        } else {
            if (number[0] != '\0') {
                sendSize = send(connfd, number, BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    FD_CLR(connfd, &allset);
                    close(connfd);
                    client[i] = -1;
                    return;
                }
            }
            if (alphabet[0] != '\0') {
                sendSize = send(connfd, alphabet, BUFF_SIZE, 0);
                if (sendSize <= 0) {
                    FD_CLR(connfd, &allset);
                    close(connfd);
                    client[i] = -1;
                    return;
                }
            }
            strcpy(done, "continue send message to server");
            sendSize = send(connfd, done, BUFF_SIZE, 0);
            if (sendSize <= 0) {
                FD_CLR(connfd, &allset);
                close(connfd);
                client[i] = -1;
                return;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    for (int i = 0; i < BACKLOG; i++) {
        flag[i] = 0;
        currentUser[i] = NULL;
        isLoggedIn[i] = 0;
        error[i] = 0;
    }

    int listenfd, connfd, sockfd, i;

    struct sockaddr_in cliaddr, servaddr;

    loadUsersFromFile();

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    //check port isValid
    serv_PORT = atoi(argv[1]);
    if (!isValidPort(argv[1])) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        exit(1);
    }
    //init socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: ");
        return 0;
    }
    //Step 2: Bind address to socket
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }
    //Step 3: Listen request from client
    if (listen(listenfd, BACKLOG) < 0) {
        perror("Error: ");
        return 0;
    }

    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
//Step 4: Communicate with clients
    printf("Server started!\n");

    while (1) {
        maxfd = listenfd;
        maxi = -1;
        FD_ZERO(&allset);
        FD_SET(listenfd, &allset);
        for (i = 0; i < FD_SETSIZE; i++) {
            if (client[i] > 0) FD_SET(client[i], &allset);
            if (client[i] > maxfd) maxfd = client[i];
            if (i > maxi) maxi = i;
        }

        printf("\nSELECT...\n");
        nready = select(maxfd + 1, &allset, NULL, NULL, NULL);
        printf("nready = %d\n", nready);
        if (nready < 0) {
            perror("\nError: ");

            continue;
        }
        printf("FD_ISSET listenfd %d: %d\n", listenfd, FD_ISSET(listenfd, &allset));
        if (FD_ISSET(listenfd, &allset)) {    /* new client connection */
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
                perror("\nError: ");
            else {
                printf("You got a connection from %s\n", inet_ntoa(cliaddr.sin_addr)); /* prints client's IP */
                for (i = 0; i < FD_SETSIZE; i++) {
                    printf("client[%d] = %d\n", i, client[i]);
                    if (client[i] < 0) {
                        client[i] = connfd;    /* save descriptor */
                        printf("Added %d to client[%d]\n", connfd, i);
                        FD_SET(connfd, &allset);    /* add new descriptor to set */
                        break;
                    }
                }
                if (i == FD_SETSIZE) {
                    printf("\nToo many clients");
                    close(connfd);
                }

                if (connfd > maxfd)
                    maxfd = connfd;        /* for select */
                printf("maxfd = %d\n", maxfd);
                if (i > maxi)
                    maxi = i;        /* max index in client[] array */
                printf("maxi = %d\n", maxi);
                if (--nready <= 0)
                    continue;        /* no more readable descriptors */
            }
        }


        for (i = 0; i <= maxi; i++) {    /* check all clients for data */
            printf("client[%d] = %d\n", i, client[i]);
            if ((sockfd = client[i]) > 0) {
                printf("FD_ISSET %d: %d\n", sockfd, FD_ISSET(sockfd, &allset));
                if (FD_ISSET(sockfd, &allset)) {
                    handleDataFromClient(sockfd, i);
                    if (--nready <= 0)
                        break;        /* no more readable descriptors */
                }
            }
        }
    }
    close(listenfd);
    return 0;
}


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
#include<pthread.h>
#define FILENAME "account.txt"
#define BUFF_SIZE 255

int listenfd, connfd;
struct sockaddr_in servaddr, cliaddr;
unsigned int len = sizeof(struct sockaddr_in);
short serv_PORT;

typedef struct User {
    char username[50];
    char password[50];
    int status;
    int attempt;
    struct User* next;
} User;

User* head = NULL;



void loadUsersFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    while(head!=NULL){
        User* tmp =head;
        head = head->next;
        free(tmp);
    }
    char line[150];
    while (fgets(line, sizeof(line), file)) {
        User* newUser = (User*)malloc(sizeof(User));
        sscanf(line, "%s %s %d", newUser->username, newUser->password, &newUser->status);
        newUser->attempt = 0;
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}

/*ghi thong tin ra file*/
void saveUsersToFile() {
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    
    User* current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d  \n", current->username, current->password, current->status);
        current = current->next;
    }
    fclose(file);
}
void processRecvBuff(const char* receive, char* number, char* alphabet, int* error, User* currentUser) {
    if (strcmp(receive, "") == 0) exit(0);
    int countNumber = 0, countAlphabet = 0;
    for (size_t i = 0; i < strlen(receive); i++) {
        char currentChar = receive[i];

        if (isdigit(currentChar)) {
            if (countNumber < BUFF_SIZE - 1) {
                number[countNumber++] = currentChar;
            } else {
            }
        } else if (isalpha(currentChar)) {
            if (countAlphabet < BUFF_SIZE - 1) {
                alphabet[countAlphabet++] = currentChar;
            } else {
            }
        } else {
            *error = 1;
            break;
        }
    }
    if(*error==0){
        strcpy(currentUser->password,receive);
       saveUsersToFile();
    }
    number[countNumber] = '\0';
    alphabet[countAlphabet] = '\0';
}
/*Chuc nang kiem tra mat khau*/
void checkWrongAttempts(User* user, char* enteredPassword, int* isLoggedIn, int *flag, User* currentUser, int connfd) {
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFF_SIZE, 0);
        *isLoggedIn = 1;
        *flag = 2;
        currentUser = user;
        return;
    }
    user->attempt++;
    if (user->attempt >= 3) {
        user->status = 0;
        *flag = 0;
        saveUsersToFile();
        send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
    } else {
        send(connfd, "NOT OK", BUFF_SIZE, 0);
    }
}

/*Chuc nang tim kiem tai khoan*/
User* searchUser(char* username) {
    User* current = head;
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

void *client_handler(void *arg){
int flag = 0;

char buff[BUFF_SIZE];
char recvBuff[BUFF_SIZE];
char done[BUFF_SIZE];
 char username[50], password[50];
int isLoggedIn = 0;
char number[BUFF_SIZE];
char alphabet[BUFF_SIZE];
int error = 0;
User* currentUser = NULL;

int clientfd;
pthread_detach(pthread_self());
clientfd = *(int *) arg;



while(1){
				 memset(buff, '\0', sizeof(buff));
        			memset(done, '\0', sizeof(done));

        int rcvSize = recv(clientfd, recvBuff, BUFF_SIZE, 0); // receive message from client
        if (rcvSize < 0) {
            perror("Error: ");
            return 0;
        }

        recvBuff[rcvSize] = '\0';
			printf("%s\n",recvBuff);
        if (flag == 0) {
            strcpy(username, recvBuff);
            currentUser = searchUser(username);
            if (currentUser != NULL) {
                if (currentUser->status == 1) {
                    send(clientfd, "USER FOUND", BUFF_SIZE, 0);
                    flag = 1;
                }
                else if (currentUser->status == 2) {
                    flag = 0;
                    send(clientfd, "Account is not activated\n", BUFF_SIZE, 0);
                } else if (currentUser->status == 0) {
                    flag = 0;
                    send(clientfd, "Account is blocked\n", BUFF_SIZE, 0);
                }
            }
            else {
                send(clientfd, "Cannot find account\n", BUFF_SIZE, 0);
            }
            continue;
        } else if (flag == 1) {
            strcpy(password, recvBuff);
            checkWrongAttempts(currentUser, password, &isLoggedIn, &flag, currentUser, clientfd);
        } else {
            if (rcvSize < 0) {
                perror("Error: ");
                return 0;
            }
            recvBuff[rcvSize] = '\0';
            if (strcmp(recvBuff, "bye") == 0) {
                char str1[100] = "Goodbye";
                char str2[100];
                strcpy(str2, currentUser->username);
                currentUser->attempt = 0;
                char str3[100];
                strcpy(str3, str1);
                strcat(str3, str2);
                send(clientfd, str3, BUFF_SIZE, 0);
                flag = 0;
                continue;
            }
            printf("Receive from client: %s\n", recvBuff);
            processRecvBuff(recvBuff, number, alphabet, &error, currentUser);
            if (error == 1) {
                strcpy(buff, "error");
                send(clientfd, buff, BUFF_SIZE, 0);
               strcpy(done, "continue send message to server");
                send(clientfd, done, BUFF_SIZE, 0);
                error = 0;
            } else {
                if (number[0] != '\0') {
                    send(clientfd, number, BUFF_SIZE, 0);
                }
                if (alphabet[0] != '\0') {
                    send(clientfd, alphabet, BUFF_SIZE, 0);
                }
                strcpy(done, "continue send message to server");
                send(clientfd, done, BUFF_SIZE, 0);
            }
        }
			}



close(clientfd);
}



int main(int argc, char* argv[]) {
    loadUsersFromFile();
    len = sizeof(cliaddr);
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

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(serv_PORT);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error: ");
        return 0;
    }

    if (listen(listenfd, 5) < 0) {
        perror("Error: ");
        return 0;
    }

    printf("Server started!\n");
   
while (1) { 
    pthread_t tid;
            connfd =  accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            if(connfd<0){
                perror("Error: ");
                return 0;
            }
pthread_create(&tid, NULL, &client_handler,
(void *) &connfd);
}
close(listenfd);      
    return 0;
}


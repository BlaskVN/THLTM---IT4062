#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define FILE_DIR "nguoidung.txt"
#define BUFFER_SIZE 255

int listenfd, connfd;
int flag = 0;

char buff[BUFFER_SIZE];
char recvBuff[BUFFER_SIZE];
char done[BUFFER_SIZE];
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
User* currentUser = NULL;
int isLoggedIn = 0;
char loggedInUsername[50];
char CurrentHomepage[50];
char correctActivationCode[] = "20225743";
char number[BUFFER_SIZE];
char alphabet[BUFFER_SIZE];
int error = 0;



void loadUser() {
    FILE* file = fopen(FILE_DIR, "r");
    if (file == NULL) {
        perror("Can't open file");
        exit(1);
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
    FILE* file = fopen(FILE_DIR, "w");
    if (file == NULL) {
        perror("Can't open file");
        exit(1);
    }
    User* current = head;
    while (current != NULL) {
        fprintf(file, "%s %s %d  \n", current->username, current->password, current->status);
        current = current->next;
    }
    fclose(file);
}
void processRecvBuff(const char* receive) {
    int countNumber = 0, countAlphabet = 0;
    for (size_t i = 0; i < strlen(receive); i++) {
        char currentChar = receive[i];

        if (isdigit(currentChar)) {
            if (countNumber < BUFFER_SIZE - 1) {
                number[countNumber++] = currentChar;
            } else {
            }
        } else if (isalpha(currentChar)) {
            if (countAlphabet < BUFFER_SIZE - 1) {
                alphabet[countAlphabet++] = currentChar;
            } else {
            }
        } else {
            error = 1;
            break;
        }
    }
    if(error==0){
        strcpy(currentUser->password,receive);
        saveUsersToFile();
    }
    number[countNumber] = '\0';
    alphabet[countAlphabet] = '\0';
}
/*Chuc nang kiem tra mat khau*/
void checkWrongAttempts(User* user, char* enteredPassword) {
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFFER_SIZE, 0);
        isLoggedIn = 1;
        flag = 2;
        currentUser = user;
        return;
    }
    user->attempt++;
    if (user->attempt >= 3) {
        user->status = 0;
        flag = 0;
        saveUsersToFile();
        send(connfd, "Password is incorrect. Account is blocked\n", BUFFER_SIZE, 0);
    } else {
        send(connfd, "NOT OK", BUFFER_SIZE, 0);
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

int main(int argc, char* argv[]) {
    loadUser();
    len = sizeof(cliaddr);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    serv_PORT = atoi(argv[1]);

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

    if (listen(listenfd, 2) < 0) {
        perror("Error: ");
        return 0;
    }

    printf("Server started!\n");
    char username[50], password[50];

    for (;;) {
        if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0) {
            perror("Error: ");
            return 0;
        }
			while(1){
				 memset(buff, '\0', sizeof(buff));
        			memset(done, '\0', sizeof(done));

        int rcvSize = recv(connfd, recvBuff, BUFFER_SIZE, 0); // receive message from client
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
                    send(connfd, "USER FOUND", BUFFER_SIZE, 0);
                    flag = 1;
                }
                else if (currentUser->status == 2) {
                    flag = 0;
                    send(connfd, "Account is not activated\n", BUFFER_SIZE, 0);
                } else if (currentUser->status == 0) {
                    flag = 0;
                    send(connfd, "Account is blocked\n", BUFFER_SIZE, 0);
                }
            }
            else {
                send(connfd, "Cannot find account\n", BUFFER_SIZE, 0);
            }
            continue;
        } else if (flag == 1) {
            strcpy(password, recvBuff);
            checkWrongAttempts(currentUser, password);
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
                send(connfd, str3, BUFFER_SIZE, 0);
                flag = 0;
                continue;
            }
            printf("Receive from client: %s\n", recvBuff);
            processRecvBuff(recvBuff);
            if (error == 1) {
                strcpy(buff, "error");
                send(connfd, buff, BUFFER_SIZE, 0);
               strcpy(done, "continue send message to server");
                send(connfd, done, BUFFER_SIZE, 0);
                error = 0;
            } else {
                if (number[0] != '\0') {
                    send(connfd, number, BUFFER_SIZE, 0);
                }
                if (alphabet[0] != '\0') {
                    send(connfd, alphabet, BUFFER_SIZE, 0);
                }
                strcpy(done, "continue send message to server");
                send(connfd, done, BUFFER_SIZE, 0);
            }
        }
			}
        close(connfd); 
    }
    saveUsersToFile();
    close(listenfd);
    return 0;
}


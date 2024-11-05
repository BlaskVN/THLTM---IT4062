#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>

#define FILENAME "account.txt"
#define BUFF_SIZE 255

int listenfd, connfd;
int flag = 0;

char buff[BUFF_SIZE];
char recvBuff[BUFF_SIZE];
char done[BUFF_SIZE];
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
char number[BUFF_SIZE];
char alphabet[BUFF_SIZE];
int error = 0;


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

User* create_user(const char* username, const char* password, int status, int attempt) {
    User* new_user = (User*)malloc(sizeof(User));
    if (new_user == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strncpy(new_user->username, username, sizeof(new_user->username) - 1);
    strncpy(new_user->password, password, sizeof(new_user->password) - 1);
    new_user->status = status;
    new_user->attempt = attempt;
    new_user->next = NULL;

    return new_user;
}

void add_user(User* new_user) {
    if (head == NULL) {
        head = new_user;
    } else {
        User* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_user;
    }
}

void delete_user(User* user_to_delete) {
    if (head == NULL) {
        fprintf(stderr, "Danh sách liên kết trống\n");
        return;
    }

    if (head == user_to_delete) {
        head = head->next;
        free(user_to_delete);
        return;
    }

    User* current = head;
    while (current->next != NULL && current->next != user_to_delete) {
        current = current->next;
    }

    if (current->next == NULL) {
        fprintf(stderr, "Người dùng không tồn tại trong danh sách\n");
        return;
    }

    current->next = user_to_delete->next;
    free(user_to_delete);
}

void display_users() {
    User* current = head;
    while (current != NULL) {
        printf("Username: %s, Password: %s, Status: %d, Attempt: %d\n",
               current->username, current->password, current->status, current->attempt);
        current = current->next;
    }
}

void free_users() {
    User* current = head;
    User* next_user;

    while (current != NULL) {
        next_user = current->next;
        free(current);
        current = next_user;
    }

    head = NULL;
}



void sig_chld(int signo){
	pid_t pid;
	int stat;
	pid = waitpid(-1, &stat, WNOHANG);
	printf("child %d terminated\n", pid );
}

void loadUsersFromFile() {
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        perror("Khong the mo file");
        exit(1);
    }
    while (head != NULL) {
        User* temp = head;
        head = head->next;
        free(temp);
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

void checkWrongAttempts(User* user, char* enteredPassword) {
    if (strcmp(enteredPassword, user->password) == 0) {
        send(connfd, "OK", BUFF_SIZE, 0);
        isLoggedIn = 1;
        flag = 2;
        currentUser = user;
        return;
    }
    //user->attempt++;
    if (user->attempt >= 3) {
        user->status = 0;
        flag = 0;
         User* new_user = create_user(user->username,user->password,0,0);
        delete_user(user);
        add_user(new_user);
       // display_users();
        saveUsersToFile();
       
        send(connfd, "Password is incorrect. Account is blocked\n", BUFF_SIZE, 0);
        
    } else {
        send(connfd, "NOT OK", BUFF_SIZE, 0);
    }
}

void processRecvBuff(const char* receive) {
     
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
            error = 1;
            break;
        }
    }
    printf("%s\n",currentUser->password);
    if(error==0){
        //strcpy(currentUser->password,receive);
      /*  User* new_user = create_user(currentUser->username,receive,0,0);
        delete_user(currentUser);
        add_user(new_user);
        display_users();
        saveUsersToFile();*/
       
    }
    printf("%s\n",currentUser->password);
    number[countNumber] = '\0';
    alphabet[countAlphabet] = '\0';
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
int main(int argc, char* argv[]) {
    loadUsersFromFile();
    len = sizeof(cliaddr);
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <TCP SERVER PORT>\n", argv[0]);
        exit(1);
    }
    //check port 
    serv_PORT = atoi(argv[1]);
     if (!isValidPort(argv[1])) {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        exit(1);
    }
    pid_t pid;
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

    if (listen(listenfd, 3) < 0) {
        perror("Error: ");
        return 0;
    }
    signal(SIGCHLD, sig_chld);
    
    printf("Server started!\n");
    char username[50], password[50];

   while(1) {
        if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len)) < 0) {
            perror("Error: ");
            return 0;
        }
        pid = fork();
        
        if(pid==0){
            close(listenfd); //cho nay truoc do co while 1
         int dem=0;
           while(1){
           
                memset(buff, '\0', sizeof(buff));
        			memset(done, '\0', sizeof(done));

        int rcvSize = recv(connfd, recvBuff, BUFF_SIZE, 0);
         // receive message from client
         loadUsersFromFile();
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
                    send(connfd, "USER FOUND", BUFF_SIZE, 0);
                    flag = 1;
                }
                else if (currentUser->status == 2) {
                    flag = 0;
                    send(connfd, "Account is not activated\n", BUFF_SIZE, 0);
                } else if (currentUser->status == 0) {
                    flag = 0;
                    send(connfd, "Account is blocked\n", BUFF_SIZE, 0);
                }
            }
            else {
                send(connfd, "Cannot find account\n", BUFF_SIZE, 0);
            }
            continue;
        } else if (flag == 1) {
            strcpy(password, recvBuff);
            currentUser->attempt =dem;
            checkWrongAttempts(currentUser, password);
            dem++;

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
                send(connfd, str3, BUFF_SIZE, 0);
                flag = 0;
                continue;
            }
            printf("Receive from client: %s\n", recvBuff);
            processRecvBuff(recvBuff);
            if (error == 1) {
                strcpy(buff, "error");
                send(connfd, buff, BUFF_SIZE, 0);
               strcpy(done, "continue send message to server");
                send(connfd, done, BUFF_SIZE, 0);
                error = 0;
            } else {
                if (number[0] != '\0') {
                    send(connfd, number, BUFF_SIZE, 0);
                }
                if (alphabet[0] != '\0') {
                    send(connfd, alphabet, BUFF_SIZE, 0);
                }
                strcpy(done, "continue send message to server");
                send(connfd, done, BUFF_SIZE, 0);
            }
        }

           }
				
			
             close(connfd); 
             exit(0);
        }


			else{
                close(connfd); 
            }
        
    }
   //saveUsersToFile();
    close(listenfd);
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define ACCOUNT_FILE "nguoidung.txt"

typedef struct User {
    char username[50];
    char password[50];
    int status; // 1: active, 0: blocked
    int attempts;
    char ip_or_domain[100];
    struct User* next;
} User;

User* head = NULL;

void readUserFiles() {
    FILE *file = fopen(ACCOUNT_FILE, "r");
    if (file == NULL) {
        printf("File nguoidung.txt doesn't exist!\n");
        exit(1);
    }

    char line[200];
    while (fgets(line, sizeof(line), file)) {
        User* newUser = (User*)malloc(sizeof(User));
        if (newUser == NULL) {
            printf("Memory allocation failed!\n");
            exit(1);
        }

        sscanf(line, "%s %s %d %s", newUser->username, newUser->password, &newUser->status, newUser->ip_or_domain);
        newUser->attempts = 0; // reset trang thai dang nhap
        newUser->next = head;
        head = newUser;
    }
    fclose(file);
}

void registerUser(){
    FILE *file = fopen(ACCOUNT_FILE, "w");
    if(file == NULL){
        printf("File doesn't exist!!!\n");
        exit(1);
    }
    User *current = head;
    while(current != NULL){
        fprintf(file, "%s %s %d %s\n",
        current->username,
        current->password,
        current->status,
        current->ip_or_domain);
        current = current->next;
    }
    fclose(file);
}

void encryptPassword(char* password, char* letters, char* digits) {
    int l_idx = 0, d_idx = 0;
    for (int i = 0; password[i] != '\0'; i++) {
        if (isalpha(password[i])) {
            letters[l_idx++] = password[i];
        } else if (isdigit(password[i])) {
            digits[d_idx++] = password[i];
        }
    }
    letters[l_idx] = '\0';
    digits[d_idx] = '\0';
}

int passwordCheck(char* password) {
    for (int i = 0; password[i] != '\0'; i++) {
        if (!isalnum(password[i])) { // kiem tra xem co phai la chu so hoac chu cai khong
            return 0;
        }
    }
    return 1;
}

User* loginCheck(char* username, char* password) {
    User* current = head;
    while (current != NULL) {
        if (strcmp(current->username, username) == 0) {
            if (strcmp(current->password, password) == 0) {
                current->attempts = 0;  
                return current;
            } else {
                current->attempts++;
                if (current->attempts >= 3) {
                    current->status = 0; // sai qua 3 lan thi block tai khoan
                    registerUser();
                }
                return NULL;
            }
        }
        current = current->next;
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./server PortNumber\n");
        return 1;
    }

    int port = atoi(argv[1]);

    // Khởi tạo server UDP
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 0;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return 0;
    }

    readUserFiles();
    printf("Server is running on port %d...\n", port);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);

        char username[50], password[50];
        sscanf(buffer, "%s %s", username, password);
        printf("Login attempt: %s %s\n", username, password);

        User* user = loginCheck(username, password);
        if (user == NULL) {
            sendto(sockfd, "not OK", strlen("not OK"), 0, (struct sockaddr *)&client_addr, addr_len);
        } else {
            if (user->status == 1) {
                sendto(sockfd, "OK", strlen("OK"), 0, (struct sockaddr *)&client_addr, addr_len);

                memset(buffer, 0, BUFFER_SIZE);
                recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);

                if (strcmp(buffer, "bye") == 0) {
                    printf("User %s signed out.\n", username);
                    sendto(sockfd, "Signed out", strlen("Signed out"), 0, (struct sockaddr *)&client_addr, addr_len);
                } 

                else if (strcmp(buffer, "homepage") == 0) {
                    printf("User %s requested homepage.\n", username);
                    sendto(sockfd, user->ip_or_domain, strlen(user->ip_or_domain), 0, (struct sockaddr *)&client_addr, addr_len);
                } 

                else {
                    printf("User %s requested password change.\n", username);
    
                    if (!passwordCheck(buffer)) {
                        sendto(sockfd, "Invalid password", strlen("Invalid password"), 0, (struct sockaddr *)&client_addr, addr_len);
                    } else {
                        char letters[BUFFER_SIZE] = {0}, digits[BUFFER_SIZE] = {0};
                        encryptPassword(buffer, letters, digits);

                        snprintf(buffer, BUFFER_SIZE, "New password: Letters: %s, Digits: %s", letters, digits);
                        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr, addr_len);

                        char new_password[BUFFER_SIZE] = {0};
                        snprintf(new_password, BUFFER_SIZE, "%s%s", letters, digits);

                        strcpy(user->password, new_password);
                        registerUser(); 
                        printf("Password for user %s has been updated to %s.\n", username, new_password);
                    }
                }
            } else {
                sendto(sockfd, "Account not ready", strlen("Account not ready"), 0, (struct sockaddr *)&client_addr, addr_len);
            }
        }
    }
    close(sockfd);
    return 0;
}

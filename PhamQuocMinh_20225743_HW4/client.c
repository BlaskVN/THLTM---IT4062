#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Command must be: ./client <IP-Address> <Port-Number>\n");
        return 1;
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    if (!(atoi(argv[2]) >= 0 && atoi(argv[2]) <= 65535))
    {
        printf("Port number must be in range 0-65535\n");
        return 1;
    }
    
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return 0;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);


    while (1) {
        char username[50], password[50];
        printf("Username (empty to quit): ");
        fgets(username, sizeof(username), stdin);
        username[strcspn(username, "\n")] = 0;

        if (strlen(username) == 0) {
            printf("Exiting...\n");
            break;
        }

        printf("Password: ");
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = 0; 

        snprintf(buffer, sizeof(buffer), "%s %s", username, password);
        sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, addr_len);

        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        printf("Server response: %s\n", buffer);

        if (strcmp(buffer, "OK") == 0) {
            while (1) {
                printf("\n\n======================================\n");
                printf("\t\tCLIENT\n");
                printf("======================================\n\n");
                printf("1. Change password\n");
                printf("2. Go to homepage\n");
                printf("3. Signout\n\n");
                printf("======================================\n\n");
                printf("Enter your choice: ");
                fgets(buffer, sizeof(buffer), stdin);
                buffer[strcspn(buffer, "\n")] = 0; 

                if (strcmp(buffer, "1") == 0) {
                    char new_password[BUFFER_SIZE];
                    printf("New password: ");
                    fgets(new_password, sizeof(new_password), stdin);
                    new_password[strcspn(new_password, "\n")] = 0;

                    sendto(sockfd, new_password, strlen(new_password), 0, (const struct sockaddr *)&server_addr, addr_len);

                    memset(buffer, 0, BUFFER_SIZE);
                    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
                    printf("Server response: %s\n", buffer);

                } else if (strcmp(buffer, "2") == 0) {
                    strcpy(buffer, "homepage");
                    sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, addr_len);

                    memset(buffer, 0, BUFFER_SIZE);
                    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
                    printf("Server response: %s\n", buffer);

                } else if (strcmp(buffer, "3") == 0 || strcmp(buffer, "bye") == 0) {
                    strcpy(buffer, "bye");
                    sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&server_addr, addr_len);

                    memset(buffer, 0, BUFFER_SIZE);
                    recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
                    printf("Server response: %s\n", buffer);
                    break;
                } else {
                    printf("Invalid choice, please try again.\n");
                }
            }
        } else if (strcmp(buffer, "account not ready") == 0) {
            printf("Account is blocked.\n");
        } else if (strcmp(buffer, "not OK") == 0) {
            printf("Incorrect password.\n");
        }
    }
    close(sockfd);
    return 0;
}

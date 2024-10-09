#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>

#define ACCOUNT_FILE "nguoidung.txt"
#define HISTORY_FILE "history.txt"

typedef struct Account {
    char username[50];
    char password[50];
    int status;  
    char homepage[100];  
    struct Account *next;
    struct Account *prev;
} Account;

Account *headAcc = NULL;
Account *currentAcc = NULL;
bool isAuthenticated = false;

void loadUserAccounts() {
    FILE* file = fopen(ACCOUNT_FILE, "r");
    if (file == NULL) {
        printf("Cannot find user file %s\n", ACCOUNT_FILE);
        return;
    }
    char line[200];
    while (fgets(line, sizeof(line), file)) {
        Account *newAcc = (Account *)malloc(sizeof(Account));
        sscanf(line, "%s %s %d %s", newAcc->username, newAcc->password, &newAcc->status, newAcc->homepage);
        newAcc->next = headAcc;
        newAcc->prev = NULL;
        if (headAcc != NULL) {
            headAcc->prev = newAcc;
        }
        headAcc = newAcc;
    }
    fclose(file);
}

void saveUserAccounts() {
    FILE* file = fopen(ACCOUNT_FILE, "w");
    if (file == NULL) {
        printf("Cannot open file %s to save\n", ACCOUNT_FILE);
        return;
    }
    Account *curr = headAcc;
    while (curr != NULL) {
        fprintf(file, "%s %s %d %s\n", curr->username, curr->password, curr->status, curr->homepage);
        curr = curr->next;
    }
    fclose(file);
}

void registerNewUser() {
    Account *newAcc = (Account *)malloc(sizeof(Account));
    printf("Username: ");
    scanf("%s", newAcc->username);
    printf("Password: ");
    scanf("%s", newAcc->password);
    printf("Homepage: ");
    scanf("%s", newAcc->homepage);
    newAcc->status = 2;  
    newAcc->next = headAcc;
    newAcc->prev = NULL;
    if (headAcc != NULL) {
        headAcc->prev = newAcc;
    }
    headAcc = newAcc;
    saveUserAccounts();
    printf("Account registered successfully.\n");
}

void userLogin() {
    if (isAuthenticated) {
        printf("Already logged in.\n");
        return;
    }
    char username[50], password[50];
    printf("Username: ");
    scanf("%s", username);
    printf("Password: ");
    scanf("%s", password);
    
    Account *curr = headAcc;
    while (curr != NULL) {
        if (strcmp(curr->username, username) == 0) {
            if (strcmp(curr->password, password) == 0) {
                if (curr->status == 1) {
                    currentAcc = curr;
                    isAuthenticated = true;
                    printf("Login successful. Welcome %s!\n", curr->username);
                    return;
                } else if (curr->status == 0) {
                    printf("Your account is blocked.\n");
                    return;
                } else {
                    printf("Your account is idle. Please contact admin.\n");
                    return;
                }
            } else {
                printf("Incorrect password.\n");
                return;
            }
        }
        curr = curr->next;
    }
    printf("Username not found.\n");
}

void updatePassword() {
    if (!isAuthenticated) {
        printf("You need to log in first.\n");
        userLogin();
        return;
    }
    char oldPass[50], newPass[50];
    printf("Old password: ");
    scanf("%s", oldPass);
    if (strcmp(currentAcc->password, oldPass) == 0) {
        printf("New password: ");
        scanf("%s", newPass);
        strcpy(currentAcc->password, newPass);
        saveUserAccounts();
        printf("Password changed successfully.\n");
    } else {
        printf("Old password is incorrect.\n");
    }
}

void modifyAccountDetails() {
    if (!isAuthenticated) {
        printf("You need to log in first.\n");
        userLogin();
        return;
    }
    printf("New homepage: ");
    scanf("%s", currentAcc->homepage);
    saveUserAccounts();
    printf("Account information updated.\n");
}

void viewLoginHistory() {
    if (!isAuthenticated) {
        printf("You need to log in first.\n");
        userLogin();
        return;
    }
    FILE *histFile = fopen(HISTORY_FILE, "r");
    if (histFile == NULL) {
        printf("Cannot find history file %s\n", HISTORY_FILE);
        return;
    }
    char buffer[200];
    bool foundHistory = false;
    while (fgets(buffer, sizeof(buffer), histFile) != NULL) {
        char storedUser[50], date[20], time[20];
        sscanf(buffer, "%s | %s | %s", storedUser, date, time);
        if (strcmp(storedUser, currentAcc->username) == 0) {
            printf("%s | %s | %s\n", storedUser, date, time);
            foundHistory = true;
        }
    }
    if (!foundHistory) {
        printf("No login history for user %s.\n", currentAcc->username);
    }
    fclose(histFile);
}

void logOut() {
    if (!isAuthenticated) {
        printf("You need to log in first.\n");
        userLogin();
        return;
    }
    isAuthenticated = false;
    currentAcc = NULL;
    printf("Logged out successfully.\n");
}

void displayHomepage(int option) {
    if (!isAuthenticated) {
        printf("You need to log in first.\n");
        userLogin();
        return;
    }

    if (option == 7) { // Display domain name
        printf("Homepage (domain name): %s\n", currentAcc->homepage);
    } else if (option == 8) { // Resolve IP address
        struct addrinfo hints, *res;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
        hints.ai_socktype = SOCK_STREAM;

        int status = getaddrinfo(currentAcc->homepage, NULL, &hints, &res);
        if (status != 0) {
            printf("Unable to resolve %s\n", currentAcc->homepage);
            return;
        }

        void *addr;
        char ipstr[INET6_ADDRSTRLEN];
        if (res->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
            addr = &(ipv4->sin_addr);
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
            addr = &(ipv6->sin6_addr);
        }

        inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
        printf("Resolved IP address: %s\n", ipstr);
        freeaddrinfo(res);
    }
}

int main() {
    loadUserAccounts();
    int choice;
    do {
        printf("USER MANAGEMENT PROGRAM\n");
        printf("-----------------------------------\n");
        printf("1. Register\n");
        printf("2. Sign in\n");
        printf("3. Change password\n");
        printf("4. Update account info\n");
        printf("5. Reset password\n");
        printf("6. View login history\n");
        printf("7. Homepage with domain name\n");
        printf("8. Homepage with IP address\n");
        printf("9. Log out\n");
        printf("Your choice (1-9, other to quit): ");
        scanf("%d", &choice);
        switch (choice) {
            case 1:
                registerNewUser();
                break;
            case 2:
                userLogin();
                break;
            case 3:
                updatePassword();
                break;
            case 4:
                modifyAccountDetails();
                break;
            case 5:
                printf("Reset password\n");
                break;
            case 6:
                viewLoginHistory();
                break;
            case 7:
            case 8:
                displayHomepage(choice);
                break;
            case 9:
                logOut();
                break;
            default:
                printf("Goodbye!\n");
        }
    } while (choice >= 1 && choice <= 9);
}

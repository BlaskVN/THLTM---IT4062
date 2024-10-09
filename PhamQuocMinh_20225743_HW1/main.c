#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define ACCOUNT_FILE "./account.txt"
#define LOGIN_HISTORY_FILE "history.txt"
#define OTP "123456"

typedef struct Account {
    char username[50];
    char password[50];
    char email[40];
    char phone[10];
    int status;
    struct Account *prev;
    struct Account *next;
} Account;

Account *head = NULL;
Account *tail = NULL;
Account *loggedInAccount = NULL;

void loadAccount(){
    FILE* file = fopen(ACCOUNT_FILE, "r");
    if(!file){
        printf("Unable to open the file %s\n", ACCOUNT_FILE);
        return;
    }
    char user[150];
    while(fgets(user, sizeof(user), file)){
        Account *newAccount = (Account *)malloc(sizeof(Account));
        sscanf(user, "%s %s %s %s %d", newAccount->username, newAccount->password, newAccount->email, newAccount->phone, &newAccount->status);
        newAccount->next = NULL;
        newAccount->prev = tail;
        if (tail != NULL) {
            tail->next = newAccount;
        } else {
            head = newAccount;
        }
        tail = newAccount;
    }
    fclose(file);
}

void saveAccount(){
    FILE* file = fopen(ACCOUNT_FILE, "w");  
    if(!file){
        printf("Unable to open the file %s for writing\n", ACCOUNT_FILE);
        return;
    }
    Account *current = head;
    while(current){
        fprintf(file, "%s %s %s %s %d\n", current->username, current->password, current->email, current->phone, current->status);
        current = current->next;
    }
    fclose(file);
}

void addLoginHistory(Account *acc) {
    FILE *file = fopen(LOGIN_HISTORY_FILE, "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date[20], timeStr[20];
    strftime(date, sizeof(date), "%Y-%m-%d", t);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", t);
    fprintf(file, "%s | %s | %s\n", acc->username, date, timeStr);
    fclose(file);
}

bool isValidPhoneNumber(char phone[]) {
    if (strlen(phone) != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (!isdigit(phone[i])) return false;
    }
    return true;
}

// void clearInputBuffer(){
//     int c;
//     while ((c = getchar()) != '\n' && c != EOF);
// }

bool validStringNOSPACE(char str[]){
    for (int i = 0; i < strlen(str); i++){
        if (str[i] == ' ') return false;
    }
    return true;
}

void createAccount(){
    Account *newAccount = (Account *)malloc(sizeof(Account));
    bool usernameExists = false;
    do {
        // clearInputBuffer();
        printf("Username: ");
        // fgets(newAccount->username, sizeof(newAccount->username), stdin);
        // newAccount->username[strcspn(newAccount->username, "\n")] = '\0';
        sscanf("%s", newAccount -> username);
        
        Account *current = head;

        while (current) {
            if (strcmp(current->username, newAccount->username) == 0) {
                usernameExists = true;
                break;
            }
            current = current->next;
        }
        
        if (usernameExists) {
            printf("Username already exists.");
            continue;
        }
        
        if (!validStringNOSPACE(newAccount->username)) {
            printf("Username must not contain spaces!");
        }
    } while (!validStringNOSPACE(newAccount->username) || usernameExists);
    
    do {
        // clearInputBuffer();
        printf("Password: ");
        // fgets(newAccount->password, sizeof(newAccount->password), stdin);
        // newAccount->password[strcspn(newAccount->password, "\n")] = '\0';
        sscanf("%s", newAccount -> password);
        if (!validStringNOSPACE(newAccount->password)) {
            printf("Password must not contain spaces!");
        }
    } while (!validStringNOSPACE(newAccount->password));

    do {
        // clearInputBuffer();
        printf("Email: ");
        // fgets(newAccount->email, sizeof(newAccount->email), stdin);
        // newAccount->email[strcspn(newAccount->email, "\n")] = '\0';
        sscanf("%s", newAccount -> email);
        if (!validStringNOSPACE(newAccount->email)) {
            printf("Email must not contain spaces!\n");
        }
    } while (!validStringNOSPACE(newAccount->email));
    
    do {
        printf("Phone number: ");
        sscanf("%s", newAccount->phone);
        if (!isValidPhoneNumber(newAccount->phone)) {
            printf("Phone number must contain exactly 10 digits.\n");
        }
    } while (!isValidPhoneNumber(newAccount->phone));
    
    newAccount->status = 1;
    newAccount->next = NULL;
    newAccount->prev = tail;
    
    if (tail) {
        tail->next = newAccount;
    } else {
        head = newAccount;
    }
    tail = newAccount;
    
    saveAccount();
}

bool userLoggedIn = false;
void login() {
    if (userLoggedIn) {
        printf("An account is already signed in.\n");
        return;
    }
    char username[50];
    printf("Username: ");
    scanf("%s", username);
    Account *current = head;
    
    while (current) {
        if (strcmp(current->username, username) == 0) {
            if (current->status == 1) {
                for (int attempt = 0; attempt < 3; attempt++) {
                    char password[50];
                    printf("Password: ");
                    scanf("%s", password);
                    if (strcmp(current->password, password) == 0) {
                        loggedInAccount = current;
                        printf("Welcome!\n");
                        addLoginHistory(current);
                        userLoggedIn = true;
                        return;
                    }
                    printf("Password is incorrect. You have %d attempts left.\n", 2 - attempt);
                }
                current->status = 0;
                saveAccount();
                printf("Your account is blocked.\n");
                return;
            } else {
                printf("Your account is blocked.\n");
                return;
            }
        }
        current = current->next;
    }
    printf("Username not found.\n");
}

void changePassword(){
    if (!userLoggedIn) {
        printf("Must sign in to use this choice.\n");
        login();
        return;
    }
    
    char oldPassword[50], newPassword[50];
    printf("Old password: ");
    scanf("%s", oldPassword);
    
    if (strcmp(loggedInAccount->password, oldPassword) == 0) {
        printf("New password: ");
        scanf("%s", newPassword);
        strcpy(loggedInAccount->password, newPassword);
        saveAccount();
        printf("Password updated successfully.\n");
    } else {
        printf("Incorrect old password.\n");
    }
}

void updateAccountInfo(){
    if (!userLoggedIn) {
        printf("Must sign in to use this choice.\n");
        login();
        return;
    }
    
    printf("New email: ");
    scanf("%s", loggedInAccount->email);
    
    do {
        printf("New phone number: ");
        scanf("%s", loggedInAccount->phone);
        if (!isValidPhoneNumber(loggedInAccount->phone)) {
            printf("Phone number must contain exactly 10 digits.\n");
        }
    } while (!isValidPhoneNumber(loggedInAccount->phone));
    
    saveAccount();
    printf("Account information updated successfully.\n");
}

char otpCode[] = OTP;
void resetPassword() {
    char username[50], otp[8];
    printf("Enter Username: ");
    scanf("%s", username);
    
    Account *current = head;
    while (current) {
        if (strcmp(current->username, username) == 0) {
            printf("Enter OTP: ");
            scanf("%s", otp);
            if (strcmp(otp, otpCode) == 0) {
                printf("Enter new password: ");
                scanf("%s", current->password);
                saveAccount();
                printf("Password reset successfully.\n");
                return;
            } else {
                printf("Invalid OTP.\n");
                return;
            }
        }
        current = current->next;
    }
    printf("Username not found.\n");
}

void viewLoginHistory(){
    if (!userLoggedIn) {
        printf("Must sign in to use this choice.\n");
        login();
        return;
    }
    
    FILE *file = fopen(LOGIN_HISTORY_FILE, "r");
    if (!file) {
        printf("Unable to open login history file.\n");
        return;
    }
    
    char user[200];
    bool historyExists = false;
    
    while (fgets(user, sizeof(user), file)) {
        char logUser[50], date[20], time[20];
        sscanf(user, "%s | %s | %s", logUser, date, time);
        if (strcmp(logUser, loggedInAccount->username) == 0) {
            printf("Date: %s, Time: %s\n", date, time);
            historyExists = true;
        }
    }
    
    if (!historyExists) {
        printf("No login history for %s.\n", loggedInAccount->username);
    }
    
    fclose(file);
}

void logout(){
    if (!userLoggedIn) {
        printf("Already sign out.\n");
        return;
    }
    
    userLoggedIn = false;
    printf("You have logged out.\n");
}

int main(){
    loadAccount();
    int choice;
    
    do{
        printf("USER MANAGEMENT PROGRAM\n");
        printf("-----------------------------------\n");
        printf("1. Register\n");
        printf("2. Sign in\n");
        printf("3. Change password\n");
        printf("4. Update account info\n");
        printf("5. Reset password\n");
        printf("6. View login history\n");
        printf("7. Sign out\n");
        printf("Your choice (1-7, other to quit):");
        scanf("%d", &choice);
        
        switch (choice) {
            case 1: createAccount(); break;
            case 2: login(); break;
            case 3: changePassword(); break;
            case 4: updateAccountInfo(); break;
            case 5: resetPassword(); break;
            case 6: viewLoginHistory(); break;
            case 7: logout(); break;
            default: printf("Goodbye\n"); break;
        }
    } while (!(choice < 1 || choice > 7));
}

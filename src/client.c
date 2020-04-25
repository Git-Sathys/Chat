#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "string.h"

// Port d'écoute de la socket
#define PORT 8888
// Adresse d'écoute (toutes les adresses)
#define IP "148.100.86.184"

#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

// Global variables
volatile sig_atomic_t flag = 0;
int sockfd = 0;
int verrif = 0;
char nickname[LENGTH_NAME] = {};

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}


// Fonction permettant de recevoir un message
void recv_msg_handler() { 
    char receiveMessage[LENGTH_SEND] = {};
    while (1) {
        int receive = recv(sockfd, receiveMessage, LENGTH_SEND, 0);
        if (receive > 0) {  
            printf("\r%s\n", receiveMessage);
            str_overwrite_stdout(nickname);      // Fonction permettant d'afficher le nom avant d'écrire le message
        } else if (receive == 0) {
            printf("\nArret du serveur");
            break; 
        } else { 
            printf("\nUne erreur est survenu");
            break;
        }
    }
    catch_ctrl_c_and_exit(2);
}


// Fonction permettant d'envoyer un message
void send_msg_handler() {
    char message[LENGTH_MSG] = {};
    while (1) {
        str_overwrite_stdout(nickname);
        while (fgets(message, LENGTH_MSG, stdin) != NULL) {
            str_trim_lf(message, LENGTH_MSG);
            if (strlen(message) == 0) {
                str_overwrite_stdout(nickname);
            } else {
                break;
            }
        }
        if (strcmp(message, "!exit") == 0) {
            break;
        }
         send(sockfd, message, LENGTH_MSG, 0);
    }
    catch_ctrl_c_and_exit(2);
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    

    // Création de la socket
    sockfd = socket(AF_INET , SOCK_STREAM , 0);  //IPv4 protocol, TCP, protocol
    if (sockfd == -1) {
        printf("Erreur création socket.");
        exit(EXIT_FAILURE);
    }

    // Initialisation de la structure sockaddr_in
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    // memset(&server_info, 0, s_addrlen);
    // memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(IP);
    server_info.sin_port = htons(PORT);


    // Verrification du serveur on
    verrif = connect(sockfd, (struct sockaddr *)&server_info, s_addrlen);
    if (verrif == -1) {
        printf("Erreur connexion Server !\n");
        exit(EXIT_FAILURE);
    }

    // Naming
    printf("Veuillez entrer votre nom: ");
    if (fgets(nickname, LENGTH_NAME, stdin) != NULL) {
        str_trim_lf(nickname, LENGTH_NAME); // remplace le \n de la saisie par un \0
    }
    if (strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("\nLe nom doit comporter plus de deux et moins de 30 caractères.\n");
        exit(EXIT_FAILURE);
    }
    
    // Names
    getsockname(sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
    getpeername(sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Connexion au Server: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    send(sockfd, nickname, LENGTH_NAME, 0);

    pthread_t send_msg_thread;
    if (pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {
        printf ("Erreur création de pthread !\n");
        exit(EXIT_FAILURE);
    }

    pthread_t recv_msg_thread;
    if (pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {
        printf ("Erreur création de pthread !\n");;
        exit(EXIT_FAILURE);
    }

    while (1) {
        if(flag) {
            printf("\nMerci à bientôt\n");
            break;
        }
    }

    close(sockfd);
    return 0;
}
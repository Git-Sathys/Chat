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
#include "proto.h"
#include "server.h"

// Global variables
int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *now;

void catch_ctrl_c_and_exit(int sig) {
    ClientList *tmp;
    while (root != NULL) {
        printf("\nsocketfd fermé: %d\n", root->data);
        close(root->data); // close all socket include server_sockfd
        tmp = root;
        root = root->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void send_to_all_clients(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = root->link;
    while (tmp != NULL) {
        if (np->data != tmp->data) { // all clients except itself.
            printf("Envoyé à sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void send_to_me(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = root->link;
    while (tmp != NULL) {
        if (np->data == tmp->data) { // itself.
            printf("Envoyé à sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void user(ClientList *np) { //A upgrate 1 send_to_me
    ClientList *tmp = root->link;
    char send_buffer[LENGTH_SEND] = {};
    sprintf(send_buffer, "Voici la liste des Users:\n-%s", np->name);
    send_to_me(np,send_buffer);
    while (tmp != NULL) {
        if (np->data != tmp->data) {
        sprintf(send_buffer,"-%s", tmp->name);
        send_to_me(np,send_buffer);
        }
        tmp = tmp->link;
    }
}



void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)p_client;

    // NOM
    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s n'a pas entré de nom.\n", np->ip);
        leave_flag = 1;
    } else {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("%s(%s)(%d) a rejoint le salon.\n", np->name, np->ip, np->data); //Affichage serveur
        sprintf(send_buffer, "%s (%d) a rejoint le salon.", np->name, np->data);   //Affichage client
        send_to_all_clients(np, send_buffer);                                   //Affichage client
    }

    // TCHAT
    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(np->data, recv_buffer, LENGTH_MSG, 0);

        if (receive > 0) {
            if (strlen(recv_buffer) == 0) {
                continue;
            }
            sprintf(send_buffer, "%s (%d)：%s", np->name, np->data, recv_buffer);  //Afichage du msg Nom User(data): msg
        } else if (receive == 0 || strcmp(recv_buffer, "!exit") == 0) {
            printf("%s(%s)(%d) a quitté le salon.\n", np->name, np->ip, np->data);
            sprintf(send_buffer, "%s(%s) a quitté le salon.", np->name, np->ip);
            leave_flag = 1;
        }
        else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        if(strcmp(recv_buffer, "!user")!=0){ //msg!="!user"
            send_to_all_clients(np, send_buffer);
        }else{
        printf("%s(%s)(%d) a utilisé la commande !user.\n", np->name, np->ip, np->data); //Affichage serveur    
        user(np);
        }

        
    }

    // Remove Node
    close(np->data);
    if (np == now) { // remove an edge node
        now = np->prev;
        now->link = NULL;
    } else { // remove a middle node
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    // Création socket
    server_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sockfd == -1) {
        printf("Erreur création socket.");
        exit(EXIT_FAILURE);
    }

    // Socket information
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_info.sin_port = htons(8888);

    // Bind and Listen
    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    // Affichage Server IP
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    // Initial linked list for users
    root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = root;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

        // Affichage User IP
        //getpeername(client_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
        //printf("User %s:%d vient de rejoindre.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        // Append linked list for users
        ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        pthread_t id;
        if (pthread_create(&id, NULL, (void *)client_handler, (void *)c) != 0) {
            perror("Création thread erreur!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
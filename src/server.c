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
#include <time.h>


// Global variables
int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *now;


//Fonction return date tableau
int *date(){
    // int h, min, s;
    time_t now;
    static int date[3];
    // Renvoie l'heure actuelle
    time(&now);
    // Convertir au format heure locale
    struct tm *local = localtime(&now);
    date[0] = local->tm_hour;
    date[1] = local->tm_min;
    date[2] = local->tm_sec;
    return date;
}

//Fonction pour ecrire dans le info.LOG
int info_log(char tmp_buffer[])
{
  FILE * pFile;
  pFile = fopen ("info.LOG","a");
  char msg[LENGTH_SEND];
  int *heure;
  heure=date();
  if (pFile!=NULL)
  {
    sprintf(msg,"[%02d:%02d:%02d] %s\n",heure[0],heure[1], heure[2],tmp_buffer);
    fputs (msg,pFile);
    fclose (pFile);
  }
  return 0;
}


//Fonction pour envoyer un msg aux autres personnes
void send_to_all_clients(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = root->link;
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data != tmp->data) { // all clients except itself.
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n",heure[0],heure[1], heure[2], tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;//Voir à quoi ça sert
    }
}



//Fonction pour arreter le serveur
void catch_ctrl_c_and_exit(int sig) {
    ClientList *tmp= root->link;
    char send_buffer[LENGTH_SEND]="stop";
    while (tmp != NULL) {
        send(tmp->data, send_buffer, LENGTH_SEND, 0);
        tmp = tmp->link;
    }
    while (root != NULL) {
        printf("\nsocketfd fermé: %d\n", root->data);
        close(root->data); // close all socket include server_sockfd
        tmp = root;
        root = root->link;
        free(tmp);
    }
    printf("Bye\n");
    info_log("Server Stop");
    exit(EXIT_SUCCESS);
}




//Fonction pour envoyer un msg à une seul personne
void send_to_me(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = root->link;
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data == tmp->data) { // itself.
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n", heure[0],heure[1], heure[2],tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}


//[Commande] pour afficher les users du serveur
void user(ClientList *np) { //A upgrate 1 send_to_me
    ClientList *tmp = root->link;
    char send_buffer[LENGTH_SEND] = {};
    sprintf(send_buffer, "Voici la liste des Users:\n-%s (%d)",np->name, np->data);
    send_to_me(np,send_buffer);
    while (tmp != NULL) {
        if (np->data != tmp->data) {
        sprintf(send_buffer,"-%s (%d)", tmp->name, tmp->data);
        send_to_me(np,send_buffer);
        }
        tmp = tmp->link;
    }
}

//[Commande] pour afficher l'horaire du serveur
void horaire(ClientList *np) {
    ClientList *tmp = root->link;
    char tmp_buffer[LENGTH_SEND];
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data == tmp->data) { // itself.
            sprintf(tmp_buffer,"Il est:  %02d:%02d:%02d", heure[0],heure[1], heure[2]);
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n", heure[0],heure[1], heure[2],tmp->data, tmp_buffer);
            send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}



/*
void mp(np) {
    ClientList *tmp = root->link;
    int data;
    while (tmp != NULL) {
        if (np == tmp->data) { // itself.
            printf("A qui voulez parlez ?");
            send(tmp->data, msg, LENGTH_SEND, 0);
            scanf("%d",data);



            sprintf(msg,"MP: ");
            
        }
        tmp = tmp->link;
    }
}
*/


void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)p_client;
    int *heure;
    heure=date();
     

    // NOM
    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s n'a pas entré de nom.\n", np->ip);
        leave_flag = 1;
    } else {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("[%02d:%02d:%02d] %s(%s)(%d) a rejoint le salon.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data); //Affichage serveur
        sprintf(send_buffer, "[%02d:%02d:%02d] %s (%d) a rejoint le salon.",heure[0],heure[1], heure[2], np->name, np->data);   //Affichage client
        send_to_all_clients(np, send_buffer);                                   //Affichage client
        sprintf(send_buffer,"%s(%s)(%d) a rejoint le salon",np->name, np->ip, np->data);
        info_log(send_buffer);
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
            sprintf(send_buffer, "[%02d:%02d:%02d] %s (%d)：%s",heure[0],heure[1], heure[2], np->name, np->data, recv_buffer);  //Send_buffer du msg Nom User(data): msg
            
        } else if (receive == 0 || strcmp(recv_buffer, "!exit") == 0) {
            printf("[%02d:%02d:%02d] %s(%s) a quitté le salon.\n",heure[0],heure[1], heure[2], np->name, np->ip);
            leave_flag = 1;
            sprintf(send_buffer,"%s(%s)(%d) a quitté le salon",np->name, np->ip, np->data);
            info_log(send_buffer);
            sprintf(send_buffer, "[%02d:%02d:%02d] %s (%d) a quitté le salon.",heure[0],heure[1], heure[2], np->name, np->data);
            
        }
        else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }

        if(strcmp(recv_buffer, "!user")==0){ //commandes TODO petit bug !exit
            printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !user.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data); //Affichage serveur    
            user(np);
        }
                else if (strcmp(recv_buffer, "!time")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !time.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data); //Affichage serveur    
                horaire(np);
              }
                else if (strcmp(recv_buffer, "!help")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !help.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data);
                sprintf(send_buffer, "Voici les commandes:\n-!user -> Liste les utilisateurs connectés\n-!time -> Pour afficher l'heure\n-!exit -> Pour fermer l'application");
                send_to_me(np, send_buffer);
              }
                else if (strncmp(recv_buffer, "!mp",3)==0)
              {
                 printf("mp envpoyé à : %s\n",recv_buffer+4);
                 //TODO FINIR LE MP
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
                 //
              }
                else
                {
                    send_to_all_clients(np, send_buffer);
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
    info_log("Start Server");
    

    //client(void *p_client));


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
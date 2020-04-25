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
#include "server.h"
#include <time.h>
#include <errno.h>

// Port d'écoute de la socket
#define PORT 8888
// Adresse d'écoute (toutes les adresses)
#define IP "127.0.0.1"
// Taille de la file d'attente
#define BACKLOG 5

#define LENGTH_NAME 31
#define LENGTH_MSG 101
#define LENGTH_SEND 201

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
int info_log(char message[])
{
  FILE * pFile;
  pFile = fopen ("info.LOG","a");
  char msg[LENGTH_SEND];
  int *heure;
  heure=date();
  if (pFile!=NULL)
  {
    sprintf(msg,"[%02d:%02d:%02d] %s\n",heure[0],heure[1], heure[2],message);
    fputs (msg,pFile);
    fclose (pFile);
  }
  return 0;
}

//Fonction pour envoyer un msg aux autres personnes
void send_to_all_clients(ClientList *np, char message[]) {
    ClientList *tmp = root->link;
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data != tmp->data) { // all clients except itself.
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n",heure[0],heure[1], heure[2], tmp->data, message);
            send(tmp->data, message, LENGTH_SEND, 0);
        }
        tmp = tmp->link;//Voir à quoi ça sert
    }
}

//Fonction pour arreter le serveur
void catch_ctrl_c_and_exit(int sig) {
    ClientList *tmp= root->link;
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
void send_to_me(ClientList *np, char message[]) {
    ClientList *tmp = root->link;
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data == tmp->data) { // itself.
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n", heure[0],heure[1], heure[2],tmp->data, message);
            send(tmp->data, message, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

//[Commande] pour afficher les users du serveur
void user(ClientList *np) { //A upgrate 1 send_to_me
    ClientList *tmp = root->link;
    char message[LENGTH_SEND] = {};
    sprintf(message, "Voici la liste des Users:\n-%s (%d)",np->name, np->data);
    send_to_me(np,message);
    while (tmp != NULL) {
        if (np->data != tmp->data) {
        sprintf(message,"-%s (%d)", tmp->name, tmp->data);
        send_to_me(np,message);
        }
        tmp = tmp->link;
    }
}

//[Commande] pour afficher l'horaire du serveur
void horaire(ClientList *np) {
    ClientList *tmp = root->link;
    char message[LENGTH_SEND];
    int *heure;
    heure=date();
    while (tmp != NULL) {
        if (np->data == tmp->data) { // itself.
            sprintf(message,"Il est:  %02d:%02d:%02d", heure[0],heure[1], heure[2]);
            printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n", heure[0],heure[1], heure[2],tmp->data, message);
            send(tmp->data, message, LENGTH_SEND, 0);
        }
        tmp = tmp->link;
    }
}

void mp(char saisie[], char *name) {
    int data = saisie[0] - 48;
    int i;
    char message[LENGTH_MSG];
    char envoie[LENGTH_MSG];
    for (i = 0; i < strlen(saisie) + 2; i++){
        envoie[i] = saisie[i+2];
    }
    sprintf(message, "Mp de %s : %s", name, envoie);
    send(data, message, LENGTH_MSG, 0);
}

void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char message_saisi[LENGTH_MSG] = {};
    char message_send[LENGTH_SEND] = {};
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
        sprintf(message_send, "[%02d:%02d:%02d] %s (%d) a rejoint le salon.",heure[0],heure[1], heure[2], np->name, np->data);   //Affichage client
        send_to_all_clients(np, message_send);                                   //Affichage client
        sprintf(message_send,"%s(%s)(%d) a rejoint le salon",np->name, np->ip, np->data);
        info_log(message_send);
    }

    // TCHAT
     while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(np->data, message_saisi, LENGTH_MSG, 0);
 
        if (receive > 0) {
            if (strlen(message_saisi) == 0) {
                continue;
            }
            sprintf(message_send, "[%02d:%02d:%02d] %s (%d)：%s",heure[0],heure[1], heure[2], np->name, np->data, message_saisi);  //message du msg Nom User(data): msg
           
        } else if (receive == 0 || strcmp(message_saisi, "!exit") == 0) {
            printf("[%02d:%02d:%02d] %s(%s) a quitté le salon.\n",heure[0],heure[1], heure[2], np->name, np->ip);
            leave_flag = 1;
            sprintf(message_send,"%s(%s)(%d) a quitté le salon",np->name, np->ip, np->data);
            info_log(message_send);
            sprintf(message_send, "[%02d:%02d:%02d] %s (%d) a quitté le salon.",heure[0],heure[1], heure[2], np->name, np->data);
           
        }
        else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }

        if(strcmp(message_saisi, "!user")==0){ //commandes TODO petit bug !exit
            printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !user.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data); //Affichage serveur    
            user(np);
            sprintf(message_saisi," ");
        }
                else if (strcmp(message_saisi, "!time")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !time.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data); //Affichage serveur    
                horaire(np);
                sprintf(message_saisi," ");
              }
                else if (strcmp(message_saisi, "!help")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%d) a utilisé la commande !help.\n",heure[0],heure[1], heure[2], np->name, np->ip, np->data);
                sprintf(message_send, "Voici les commandes:\n-!user -> Liste les utilisateurs connectés\n-!time -> Pour afficher l'heure\n-!mp [numero user] [msg]\n-!exit -> Pour fermer l'application");
                send_to_me(np, message_send);
                sprintf(message_saisi," ");
              }
                else if (strncmp(message_saisi, "!mp",3)==0)
              {
                printf("mp envpoyé à : %s\n",message_saisi+4);
                mp(message_saisi+4, np->name);
                sprintf(message_saisi," ");
              }
                else
                {
                    send_to_all_clients(np, message_send);
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

    // Initialisation de la structure sockaddr_in
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    // memset(&server_info, 0, s_addrlen);
    // memset(&client_info, 0, c_addrlen);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(IP);
    server_info.sin_port = htons(PORT);

    // Création de la socket en TCP
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Echéc de la création: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
    printf("Création de la socket\n");     

    // Attachement de la socket sur le port et l'adresse IP
	if (bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen)) {
		printf("Echéc d'attachement: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	printf("Attachement de la socket sur le port %i\n", PORT);

	// Passage en écoute de la socket
	if (listen(server_sockfd, BACKLOG) != 0) {
		printf("Echéc de la mise en écoute: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}


    // Affichage Server IP
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Start Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));
    info_log("Start Server");

    // Initial linked list for users
    root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = root;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

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
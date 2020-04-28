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
// Adresse du serveur
#define IP "127.0.0.1"
// Taille de la file d'attente
#define BACKLOG 5
//Taille
#define LENGTH_NAME 31
#define LENGTH_MSG 201
#define LENGTH_SEND 201
//Couleur
#define RED      "\033[0;31m"
#define YELLOW   "\033[0;33m"
#define BLUE     "\033[0;34m"
#define NORMAL   "\033[00m"


// Global variables
int server_sockfd = 0, client_sockfd = 0;
ClientList *user, *now;
char error_send[LENGTH_SEND] = {};

//Fonction return date tableau
int *date(){
    // int h, min, s;
    time_t now;
    static int date[6];
    // Renvoie l'heure actuelle
    time(&now);
    // Convertir au format heure locale
    struct tm *local = localtime(&now);
    date[0] = local->tm_hour;
    date[1] = local->tm_min;
    date[2] = local->tm_sec;
    date[3]= local->tm_mday;          
    date[4] = local->tm_mon + 1;     
    date[5] = local->tm_year + 1900;

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
    sprintf(msg,"[%02d/%02d/%i] [%02d:%02d:%02d] %s\n",heure[3],heure[4], heure[5], heure[0],heure[1], heure[2],message);
    fputs (msg,pFile);
    fclose (pFile);
  }
  return 0;
}

//Fonction pour envoyer un msg aux autres personnes
void send_to_all_clients(ClientList *client, char message[]) {
    ClientList *all_client = user->link;
    int *heure;
    heure=date();
    while (all_client != NULL) {
        if (client->data != all_client->data) { // tous les clients sauf lui
            printf("[%02d:%02d:%02d] Envoyé à User %i: \"%s\" \n",heure[0],heure[1], heure[2], all_client->data, message);
            send(all_client->data, message, LENGTH_SEND, 0);
        }
        all_client = all_client->link;//Voir à quoi ça sert
    }
}


//Fonction pour envoyer un msg à une seul personne
void send_to_one(ClientList *client, char message[]) {
    int *heure;
    heure=date();
    printf("[%02d:%02d:%02d] Envoyé à User %i: \"%s\" \n", heure[0],heure[1], heure[2],client->data, message);
    send(client->data, message, LENGTH_SEND, 0);
}


//Fonction pour arreter le serveur
void catch_ctrl_c_and_exit(int sig) {
    ClientList *all_client= user->link;
    while (user != NULL) {
        close(user->data); // close all socket include server_sockfd
        all_client = user;
        user = user->link;
        free(all_client);
    }
    printf("Fermeture des socket \n");
    printf("Arret du serveur\n");
    info_log("Server Stop");
    exit(EXIT_SUCCESS);
}



//[Commande] pour afficher les users du serveur
void C_user(ClientList *client) {
    ClientList *all_client = user->link;
    char message[LENGTH_SEND] = {};
    sprintf(message, "Voici la liste des Users:\n-%s (%i)",client->name, client->data);
    send_to_one(client,message);
    while (all_client != NULL) {
        if (client->data != all_client->data) {
        sprintf(message,"-%s (%i)", all_client->name, all_client->data);
        send_to_one(client,message);
        }
        all_client = all_client->link;
    }
}

//[Commande] pour afficher l'horaire du serveur
void C_horaire(ClientList *client) {
    char message[LENGTH_SEND];
    int *heure;
    heure=date();
    sprintf(message,"Il est:  %02d:%02d:%02d", heure[0],heure[1], heure[2]);
    printf("[%02d:%02d:%02d] Envoyé à User %d: \"%s\" \n", heure[0],heure[1], heure[2],client->data, message);
    send_to_one(client,message);
}

//[Commande] pour envoyer un message privé
void C_mp(char saisie[], ClientList *client) {
    ClientList * all_client = user->link;
    int  valeur1 = saisie[0] - 48;
    int data;
    char data1[2];
    char data2[2];
    int i;
    char message[LENGTH_MSG];
    char envoie[LENGTH_MSG];
    int msg1 = 0;
    if (saisie[1]>47 && saisie[1]<58)
    {
        int valeur2 = saisie[1] - 48;
        sprintf(data1, "%i", valeur1);
        sprintf(data2, "%i", valeur2);
        strcat(data1, data2);
        data=atoi(data1);
    }else{
        data=saisie[0]-48;
    }
    while(all_client != NULL){
        if(data == all_client->data){
            for (i = 0; i < strlen(saisie) + 2; i++){
            envoie[i] = saisie[i+2];
            }
            sprintf(message, YELLOW"Mp de %s :"NORMAL" %s", client->name, envoie);
            send(data, message, LENGTH_MSG, 0);
            msg1 = 1;
        }
    all_client = all_client->link;
    }
    if (msg1 == 0) {
        sprintf(message, RED"Le client : %i n'existe pas"NORMAL , data);
        send(client->data, message, LENGTH_MSG, 0);  
    }
}


void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char message_saisi[LENGTH_MSG] = {};
    char message_send[LENGTH_SEND] = {};
    ClientList *client = (ClientList *)p_client;
    int *heure;
    heure=date();
     

    // NOM
    if (recv(client->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf(RED"%s n'a pas entré de nom.\n"NORMAL, client->ip);
        leave_flag = 1;
    } else {
        strncpy(client->name, nickname, LENGTH_NAME);
        printf("[%02d:%02d:%02d] %s(%s)(%i) a rejoint le salon.\n",heure[0],heure[1], heure[2], client->name, client->ip, client->data); //Affichage serveur
        sprintf(message_send, "[%02d:%02d:%02d] "BLUE"%s (%i) a rejoint le salon."NORMAL,heure[0],heure[1], heure[2], client->name, client->data);   //Affichage client
        send_to_all_clients(client, message_send);                                   //Affichage client
        sprintf(message_send,"%s(%s)(%i) a rejoint le salon",client->name, client->ip, client->data);
        info_log(message_send);
    }

    // TCHAT
     while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(client->data, message_saisi, LENGTH_MSG, 0);
 
        if (receive > 0) {
            if (strlen(message_saisi) == 0) {
                continue;
            }
            sprintf(message_send, "[%02d:%02d:%02d] %s (%i)：%s",heure[0],heure[1], heure[2], client->name, client->data, message_saisi);  //message du msg Nom User(data): msg
           
        } else if (receive == 0 || strcmp(message_saisi, "!exit") == 0) {
            printf("[%02d:%02d:%02d] %s(%s) a quitté le salon.\n",heure[0],heure[1], heure[2], client->name, client->ip);
            leave_flag = 1;
            sprintf(message_send,"%s(%s)(%i) a quitté le salon",client->name, client->ip, client->data);
            info_log(message_send);
            sprintf(message_send, "[%02d:%02d:%02d]"BLUE" %s (%i) a quitté le salon."NORMAL,heure[0],heure[1], heure[2], client->name, client->data);
           
        }
        else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }

        if(strcmp(message_saisi, "!user")==0){ //commandes TODO petit bug !exit
            printf("[%02d:%02d:%02d] %s(%s)(%i) a utilisé la commande !user.\n",heure[0],heure[1], heure[2], client->name, client->ip, client->data); //Affichage serveur    
            C_user(client);
            sprintf(message_saisi," ");
        }
                else if (strcmp(message_saisi, "!time")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%i) a utilisé la commande !time.\n",heure[0],heure[1], heure[2], client->name, client->ip, client->data); //Affichage serveur    
                C_horaire(client);
                sprintf(message_saisi," ");
              }
                else if (strcmp(message_saisi, "!help")==0)
              {
                printf("[%02d:%02d:%02d] %s(%s)(%i) a utilisé la commande !help.\n",heure[0],heure[1], heure[2], client->name, client->ip, client->data);
                sprintf(message_send, "Voici les commandes:\n-!user -> Liste les utilisateurs connectés\n-!time -> Pour afficher l'heure\n-!mp [numero user] [msg]\n-!exit -> Pour fermer l'application");
                send_to_one(client, message_send);
                sprintf(message_saisi," ");
              }
                else if (strncmp(message_saisi, "!mp",3)==0)
              {
                printf("mp envpoyé à : %s\n",message_saisi+4);
                C_mp(message_saisi+4, client);
                sprintf(message_saisi," ");
              }
                else
                {
                    send_to_all_clients(client, message_send);
                }
    }

    // Remove Node
    close(client->data);
    if (client == now) { // remove an edge node
        now = client->prev;
        now->link = NULL;
    } else { // remove a middle node
        client->prev->link = client->link;
        client->link->prev = client->prev;
    }
    free(client);
}

int main()
{
    
    signal(SIGINT, catch_ctrl_c_and_exit);
    signal(SIGTERM, catch_ctrl_c_and_exit);
    
    // Initialisation de la structure sockaddr_in
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    server_info.sin_family = AF_INET;
    server_info.sin_addr.s_addr = inet_addr(IP);
    server_info.sin_port = htons(PORT);

    // Création de la socket en TCP
	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Echéc de la création: %s\n", strerror(errno));
        sprintf(error_send,"Echéc de la création: %s", strerror(errno));
        info_log(error_send);
		exit(EXIT_FAILURE);
	}
    printf("Création de la socket\n");

    // Attachement de la socket sur le port et l'adresse IP
	if (bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen)) {
		printf("Echéc d'attachement: %s\n", strerror(errno));
        sprintf(error_send,"Echéc d'attachement: %s", strerror(errno));
        info_log(error_send);
		exit(EXIT_FAILURE);
	}
	printf("Attachement de la socket sur le port %i\n", PORT);

	// Passage en écoute de la socket
	if (listen(server_sockfd, BACKLOG) != 0) {
		printf("Echéc de la mise en écoute: %s\n", strerror(errno));
        sprintf(error_send,"Echéc de la mise en écoute: %s", strerror(errno));
        info_log(error_send);
		exit(EXIT_FAILURE);
	}

    // Affichage Server IP
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Start Server on: %s:%i\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port) );
    info_log("Start Server");

    // Initial linked list for users
    user = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = user;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

        // Append linked list for users
        ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        pthread_t id;
        if (pthread_create(&id, NULL, (void *)client_handler, (void *)c) != 0) {
            printf("Echéc de la mise en écoute: %s\n",strerror(errno));
            sprintf(error_send,"Echéc de la mise en écoute %s",strerror(errno));
            info_log(error_send);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
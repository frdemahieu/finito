/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient le code principal du client.
*/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "./structs.h"
#include "./util.h"
#include "./semaphore.h"
#include "./client.h"

#define SERVEURNAME "127.0.0.1"

int server_socket = -1;
int numJoueur;

char* pointeurMemoire = NULL;
Client joueurs[4]; 



MessageClient *msgClient;
MessageServeur *msgServeur;


int main (int argc, char *argv[]) {

	InscriptionClient *msgInscription = malloc(sizeof(InscriptionClient));
	char *server_name = SERVEURNAME;
	struct sockaddr_in sockAddr;
	struct hostent *hostinfo = NULL;
	typedef struct in_addr IN_ADDR;
	struct sigaction sigact;

	msgClient = malloc(sizeof(MessageClient));
	msgServeur = malloc(sizeof(MessageServeur));
	
	srand(time(NULL));

	//hostAddr = inet_addr(SERVEURNAME);
	hostinfo = gethostbyname(server_name);
	if (hostinfo == NULL) /* l'hôte n'existe pas */
	{
    		printf ("Serveur %s inconnu", server_name);
    		exit(errno);
	}
		
	int port = 15555;
	if(argc == 2) {
		port = atoi(argv[1]);
	}
	
	sockAddr.sin_addr = *(IN_ADDR *) hostinfo->h_addr;		
	sockAddr.sin_port = htons(port);
	sockAddr.sin_family = AF_INET;

	/* creation du socket */
	if ( (server_socket = socket(AF_INET,SOCK_STREAM,0)) < 0) {
		printf("Echec création socket\n");
		exit(0);
	}

	sigact.sa_handler = &deconnexion;
	if (sigaction(SIGINT, &sigact, NULL) < 0) {
		perror ("sigaction fermeture ctrl c");
		exit(errno);
	}
	if (sigaction(SIGHUP, &sigact, NULL) < 0) {
		perror ("sigaction fermeture terminal");
		exit(errno);
	}	

	/* Connexion */
	//on demande d'abord le pseudo
	printf("Veuillez choisir un pseudo : \n");
	fgets(msgInscription->pseudo,30,stdin);
	enleverRetourLigne(msgInscription->pseudo);
	msgInscription->type = CONNEXION;

	/* requete de connexion */
	if(connect(server_socket, (struct sockaddr *)&sockAddr, sizeof(sockAddr)) < 0 ) {
		printf("Serveur inacessible\n");
		exit(0);
	}
	
	write(server_socket, msgInscription, sizeof(InscriptionClient));

	//On regarde si le serveur nous accepte.
	read(server_socket, msgServeur,sizeof(MessageServeur));
	
	if (msgServeur->type == CONN_OK) {
		int rejouer = 1;
		printf("Vous êtes connecté\n");
		
		// Memoire partagée
		pointeurMemoire = getZoneMemoire("memoire.txt");	
		
		// Initialisation Sémaphore
		initSemaphore();
		
		printf("Attente de joueur ... \n");
		
		while(rejouer == 1) {
			// Attente du demarrage de la partie
			read(server_socket, msgServeur, sizeof(MessageServeur));
			if (msgServeur->type == DEBUT) {
				numJoueur = msgServeur->numJoueur;
				printf("La partie va commencer, vous êtes le joueur n°%d\n", numJoueur);
				commencerLecture();
				memcpy(joueurs, pointeurMemoire, sizeof(joueurs));
				arreterLecture();
				printf("Les joueurs sont %s %s %s %s\n", joueurs[0].pseudo, joueurs[1].pseudo, joueurs[2].pseudo, joueurs[3].pseudo);
				
				// Lancement du jeu
				rejouer = jeu(server_socket,numJoueur,pointeurMemoire,msgServeur->nombreJoueur);
			} else if (msgServeur->type == FIN) {
				printf("Personne ne veut rejouer une partie\n");
				fermerSocket();
			}
		}	
	} else if (msgServeur->type == CONN_KO) {
		printf("Le serveur vous a refusé car une partie est en cours \n");
		fermerSocket();
	} else {
		printf("ERROR : message inattendu : %d\n", msgServeur->type);
		fermerSocket();
		return EXIT_FAILURE;
	}
	fermerSocket();
	return EXIT_SUCCESS;
}


void deconnexion(int sig){
	printf("Vous avez quitté la partie\n");
	/* fermeture de la connection */
	msgClient->type = SERVEUR_QUIT;
	write(server_socket, msgClient, sizeof(MessageClient));
	fermerSocket();
}

void fermerSocket(){
	free(msgClient);
	free(msgServeur);
	close(server_socket);
	if(pointeurMemoire != NULL){
		detacherZoneMemoire(pointeurMemoire);
	}
	exit(0);
}


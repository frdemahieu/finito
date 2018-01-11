/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient le code principal du serveur.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>


#include "./util.h"
#include "./structs.h"
#include "./serveur.h"
#include "./semaphore.h"

#define MAX 20;

#define TEMPS_INSCR 30;

int sock = -1;
Client joueurs[4];
char* pointeurMemoire;
int nbJoueur = 0;
int max;
struct sockaddr_in adresseClient;
int adresseLongueur;

MessageClient *msgClient;
MessageServeur *msgServeur;

int main (int argc,char *argv[]) {
	msgClient = msgClient = malloc(sizeof(MessageClient));
	msgServeur = malloc(sizeof(MessageServeur));
	srand(time(NULL));
	
	int port = 15555;
		
	struct sockaddr_in adresse, adresseClient;
	struct sigaction sigact;
	sigact.sa_handler = &fermetureServeur;
	sigact.sa_flags = 0;
	sigemptyset(&sigact.sa_mask);
	
	if (sigaction(SIGINT, &sigact, NULL) != 0) {
		perror ("Erreur sigaction fermeture ctrl c");
		exit(errno);																																								
	}
	if (sigaction(SIGHUP, &sigact, NULL) != 0) {
		perror ("Erreur sigaction fermeture terminal");
		exit(errno);
	}
	
	if(argc == 2) {
		port = atoi(argv[1]);
	}
	adresse.sin_port = htons(port);
	adresse.sin_family = AF_INET;
	adresse.sin_addr.s_addr = htonl(INADDR_ANY);

	/* creation de socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	  perror("Erreur lors de la création d'un socket\n");
	  exit(errno);
	}

	/* bind serveur - socket */
	if(bind(sock, (struct sockaddr *)&adresse, sizeof(adresse)) == -1) {
		perror("Le port est déjà utilisé\n");
		exit(errno);
	}

	/* ecoute sur la socket */
	if(listen(sock,5) == -1) {
		perror("Erreur lors du listen");
		exit(errno);
	}

	//Le file descriptor maximum ! 
	max = sock;
			
	// Memoire partagée
	pointeurMemoire = getZoneMemoire("memoire.txt");
	printf("Mémoire partagée créée\n");
	
	// Initialisation du sémaphore
	initSemaphore();
	
	// Processus d'inscription
	lancerInscription();
	
	
	fermerSocket();
	return EXIT_SUCCESS;
}

void lancerInscription(){
	InscriptionClient *message = malloc(sizeof(InscriptionClient));
	fd_set rfds;
	struct sigaction sa;
	struct itimerval timer;
	sa.sa_handler = &jeu;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);

	//Initialisation timer
	if(sigaction(SIGALRM, &sa, NULL) == -1){
		perror("Erreur sigaction alarme");
		exit(errno);
	}
	timer.it_value.tv_sec = TEMPS_INSCR;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = TEMPS_INSCR;
	timer.it_interval.tv_usec = 0;
	
	/* accept la connexion */
	adresseLongueur = sizeof(adresseClient);

	while(1){
		int i;
		/* on remet à zéro les file descriptor dans le select */
		FD_ZERO(&rfds);
		FD_SET(sock,&rfds);
		for(i = 0; i < nbJoueur; i++){
				FD_SET(joueurs[i].socket, &rfds);
		}

		printf("En attente d'utilisateur\n");

		//On lance un select
		if(select(max+1, &rfds, NULL, NULL, NULL) == -1) {
			perror("Erreur lors du select");
			exit(errno);	
		}
		//Si c'est une connexion
		if(FD_ISSET(sock,&rfds)) {
			int utilisateur;
			utilisateur = accept(sock, (struct sockaddr *)&adresseClient, &adresseLongueur);
			read(utilisateur, message, sizeof(InscriptionClient));
			if (message->type == CONNEXION) {
				if (nbJoueur < 4) {
					joueurs[nbJoueur].socket = utilisateur;
					strcpy(joueurs[nbJoueur].pseudo, message->pseudo);
					printf("%s s'est connecté\n", message->pseudo);
					msgServeur->type = CONN_OK;
					write(joueurs[nbJoueur].socket, msgServeur, sizeof(MessageServeur));
					max = (utilisateur > max) ? utilisateur:max; 
					nbJoueur++;
				
					if (nbJoueur == 4){
						setitimer(ITIMER_REAL, NULL, NULL);	
						jeu();
					}
					else if (nbJoueur  >= 1) {
						setitimer(ITIMER_REAL, &timer, NULL);				
					}
					else if (nbJoueur == 0){
						setitimer(ITIMER_REAL, NULL, NULL);
					}
				}
			}
		}
		//si c'est un autre message
		else {
			int i;
			for(i = 0; i < nbJoueur; i++) {
				//on regarde qui a lancé le message
				if(FD_ISSET(joueurs[i].socket, &rfds)) {
					read(joueurs[i].socket, msgClient, sizeof(MessageClient));
					if(msgClient->type == SERVEUR_QUIT) {
						close(joueurs[i].socket);
						printf("%s a quitté la partie.\n", joueurs[i].pseudo);
						// remplace le trou par le dernier joueur
						memcpy(&joueurs[i], &joueurs[nbJoueur-1], sizeof(joueurs[nbJoueur-1]));
						nbJoueur--;
					
					}
				}
			}
		}
	}
}

void jeu() {	
	if(nbJoueur < 2) {
		printf("le nombre minimum de joueur n'est pas atteint\n");
		fermerSocket();
	} else {
		fd_set rfds;
		int i;
		int tableau[36] = {0};
		int de;
		int gagnant = 0;
		int joueurGagnant = -1;
		int nbReponses = 0;
		
		printf("Le jeux va commencer\n");
		//Initialisation du jeu
		msgServeur->type = DEBUT;
		for(i = 0; i < nbJoueur; i++) {
			msgServeur->numJoueur = i;
			msgServeur->nombreJoueur = nbJoueur;
			sprintf(joueurs[i].couleur,"%d", i + 31);
			memcpy(joueurs[i].plateau,tableau,sizeof(joueurs[i].plateau));
			write(joueurs[i].socket, msgServeur, sizeof(MessageServeur));
		}
		downSemaphore(WRITE);
		memcpy(pointeurMemoire,joueurs,sizeof(joueurs));
		upSemaphore(WRITE);
		
		// Boucle du jeu
		while(gagnant == 0 || nbReponses < nbJoueur) {
			int nbMessagesRecus = 0;
			if (gagnant == 0) {
				de = envoyerDe();
			} else {
				msgServeur->type = SCORES;
				msgServeur->numJoueur = joueurGagnant;
				majScores();
				envoyerMessageATous();
			}
			while(nbMessagesRecus < nbJoueur) {

				FD_ZERO(&rfds);
				FD_SET(sock,&rfds);
				for(i = 0; i < nbJoueur; i++){
						FD_SET(joueurs[i].socket, &rfds);
				}
				//On lance un select pour tous les sockets
				if(select(joueurs[nbJoueur-1].socket+1, &rfds, NULL, NULL, NULL) == -1) {
					perror("Erreur lors du select");
					exit(errno);	
				}
				if(FD_ISSET(sock,&rfds)) {
					int utilisateur = accept(sock, (struct sockaddr *)&adresseClient, &adresseLongueur);
					msgServeur->type = CONN_KO;
					write(utilisateur, msgServeur, sizeof(MessageServeur));
				} else {
					for(i = 0; i < nbJoueur; ++i) {
						//on regarde qui a lancé le message
						if(FD_ISSET(joueurs[i].socket, &rfds)) {	
							read(joueurs[i].socket, msgClient, sizeof(MessageClient));
							if(msgClient->type == SERVEUR_QUIT) {
								Client joueurQuittant;
								close(joueurs[i].socket);
								printf("%s a quitté la partie.\n", joueurs[i].pseudo);
								// remplace le trou par le dernier joueur
								
								memcpy(&joueurQuittant, &joueurs[i], sizeof(joueurs[nbJoueur-1]));
								memcpy(&joueurs[i], &joueurs[nbJoueur-1], sizeof(joueurs[nbJoueur-1]));
								memcpy(&joueurs[nbJoueur-1], &joueurQuittant, sizeof(joueurs[nbJoueur-1]));
								
								downSemaphore(WRITE);
								memcpy(pointeurMemoire,joueurs,sizeof(joueurs));
								upSemaphore(WRITE);
										
								nbJoueur--;	
								
								if(nbJoueur == 1){
									majScores();
									msgServeur->type = FIN;
									sleep(1);
									fermerSocket();
								} else {								
									msgServeur->type = NOMBREJOUEUR;
									msgServeur->nombreJoueur = nbJoueur;
									msgServeur->numJoueur = i;
									envoyerMessageATous();
									
									msgServeur->type = CHANGENUM;
									msgServeur->numJoueur = i;
									write(joueurs[i].socket, msgServeur, sizeof(MessageServeur));
								}
							} else if(msgClient->type == PLACEMENT) {
								int position = msgClient->nouvellePosition;

								joueurs[i].plateau[position] = msgClient->jeton;
								printf("------------------------------------\n");
								printf("PLACEMENT\n");
								printf("joueur n°%d\n", i);
								printf("jeton %d\n",joueurs[i].plateau[position]);
								printf("Position %d\n",msgClient->nouvellePosition);
								printf("------------------------------------\n");
								downSemaphore(WRITE);
								memcpy(pointeurMemoire,joueurs,sizeof(joueurs));
								upSemaphore(WRITE);
								nbMessagesRecus++;
								if(msgClient->finito == FINITO && gagnant == 0){
									gagnant = 1;
									joueurGagnant = i;
								}
							} else if(msgClient->type == DEPLACEMENT) {
								int nouvellePosition = msgClient->nouvellePosition;
								int anciennePosition = msgClient->anciennePosition;
								
								joueurs[i].plateau[nouvellePosition] = msgClient->jeton;
								joueurs[i].plateau[anciennePosition] = 0;
								printf("------------------------------------\n");
								printf("DEPLACEMENT\n");
								printf("joueur n°%d\n", i);
								printf("jeton %d\n", msgClient->jeton);
								printf("Position %d -> %d\n", msgClient->anciennePosition, msgClient->nouvellePosition);
								printf("------------------------------------\n");
								downSemaphore(WRITE);
								memcpy(pointeurMemoire,joueurs,sizeof(joueurs));
								upSemaphore(WRITE);
								nbMessagesRecus++;
								if(msgClient->finito == FINITO && gagnant == 0){
									gagnant = 1;
									joueurGagnant = i;
								}
							} else if(msgClient->type == NOUVELLE_PARTIE) {
								nbReponses++;
								nbMessagesRecus++;
							} else{
								msgServeur->type = CONN_KO;
							}
						}
					}
				}
			}
		}
	}
}

int envoyerDe() {
	int de;
	
	de = rand() % MAX;
	de++;
	msgServeur->type = DE;
	msgServeur->de = de;
	envoyerMessageATous();
	return de;
}

void envoyerMessageATous() {
	int i;
	for(i = 0; i < nbJoueur; i++) {
		write(joueurs[i].socket, msgServeur, sizeof(MessageServeur));
	}
}

void majScores() {
	int cpt, mem;
	int i, joueur;
	int jeton = 0;
	printf("-----------------------------\n");
	for(joueur = 0; joueur < 4; ++joueur) {
		cpt = 0;
		mem = 0;
		for (i = 0; i < 36; ++i) {
			if (joueurs[joueur].plateau[i] > jeton) {
				jeton = joueurs[joueur].plateau[i];
				cpt++;
			} else if (joueurs[joueur].plateau[i] != 0) {
				if(cpt > mem) {
					mem = cpt;
				}
				cpt = 1;
				jeton = joueurs[joueur].plateau[i];
			}
		}
		if(cpt > mem) {
			mem = cpt;
		}
		if(mem == 12) {
			mem = 24;
		}
		printf("Score joueur n°%d : %d\n", joueur, mem);
		joueurs[joueur].score += mem;
	}
	printf("-----------------------------\n");
	
	downSemaphore(WRITE);
	memcpy(pointeurMemoire, joueurs, sizeof(joueurs));
	upSemaphore(WRITE);
}

void fermetureServeur(int sig) {
	fermerSocket();
}

void fermerSocket() {
	envoyerMessageATous();
	close(sock);
	free(msgClient);
	free(msgServeur);
	detacherZoneMemoire(pointeurMemoire);
	supprimerZonesMemoire();
	supprimerSemaphore();
	exit(0);
}

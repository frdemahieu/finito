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

#define couleur(param) printf("\033[%sm",param)

Client joueurs[4];
char* pointeurMemoire;
static int tableauBase[36] = {1,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,
				11,11,12,12,13,13,14,14,15,15,16,16,17,17,18,18,19,20};
int nombreJetonPasJoue = 12;
int numJoueur;
int jetonsDecouverts[3];
int nombreJoueur;
int nombreJoueursTotal = 0;
void afficherTableau();

int jeu(int server_socket,int num,char* pointeurMemoireParam,int nombreJoue){
	int i;

	void afficherPlateau(int num);
	int premierDroite(int de);
	int premierGauche(int de);
	int verifierFinito();
	void afficherScores();

	numJoueur = num;
	nombreJoueursTotal = nombreJoue;
	pointeurMemoire = pointeurMemoireParam;
	nombreJoueur = nombreJoue;
	
	MessageClient *msgClient = malloc(sizeof(MessageClient));
	MessageServeur *msgServeur = malloc(sizeof(MessageServeur));
	
	for (i = 0; i < 3; ++i) {
		jetonsDecouverts[i] = decouvrirJeton();
	}
	
	while(1) {
		read(server_socket, msgServeur ,sizeof(MessageServeur));
		if(msgServeur->type == DE) {
			int de = msgServeur->de;
			int numJeton = 0;
			char chaine[5];
			int jetTemp;
			
			msgClient = malloc(sizeof(MessageClient));
			system("clear");
			afficherPlateau(numJoueur);

			// Tant qu'il y a des jetons non joués 
			if (nombreJetonPasJoue > 0) {
				numJeton = demanderJeton(de);
					
				msgClient->jeton = jetonsDecouverts[numJeton];
				jetTemp = decouvrirJeton();
				if(jetTemp != -1){
					jetonsDecouverts[numJeton] = jetTemp;
				}
				else{
					if(numJeton != nombreJetonPasJoue){
						jetonsDecouverts[numJeton] = jetonsDecouverts[nombreJetonPasJoue];
					}
				}
				// Remplissage struct message
				msgClient->type = PLACEMENT;
				msgClient->nouvellePosition = placementJeton(de);
				if(nombreJetonPasJoue < 1) {
					msgClient->finito = verifierFinito();
				}
			}
			// Phase de déplacement des jetons
			else {
				int posJeton = choisirJeton(de);
				msgClient->type = DEPLACEMENT;
				msgClient->jeton = joueurs[numJoueur].plateau[posJeton];
				msgClient->anciennePosition = posJeton;
				msgClient->nouvellePosition = placementJeton(de);
				msgClient->finito = verifierFinito();
			}		
			write(server_socket,msgClient,sizeof(Client));
			printf("En attente des autres joueurs ...\n");
		} else if (msgServeur->type == SCORES){
			int num = msgServeur->numJoueur;			
			char chaine[5];
			afficherPlateau(numJoueur);
			printf("Le joueur %s a gagné!\n",joueurs[num].pseudo);
			afficherScores();
			while(chaine[0] != 'N' && chaine[0] != 'n' && chaine[0] != 'O' && chaine[0] != 'o') {
				printf("Voulez-vous rejouer ? oui (o) / non (n) ou voir les tableaux (t)\n");
				fgets(chaine, 5, stdin);
				if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
				}
			}
			if ((chaine[0] == 'o' || chaine[0] == 'O')) { 
				msgClient->type = NOUVELLE_PARTIE;
				write(server_socket,msgClient,sizeof(Client));
				printf("En attente des réponses des autres joueurs...\n");
				return 1;	// recommencer nouvelle partie dans le main
			} else if(chaine[0] == 'n' || chaine[0] == 'N'){
				msgClient->type = SERVEUR_QUIT;
				write(server_socket,msgClient,sizeof(Client));
				printf("Vous avez quitté la partie\n");
				return 0;
			}
		} else if(msgServeur->type == CHANGENUM) {
			numJoueur = msgServeur->numJoueur;
		} else if( msgServeur->type == NOMBREJOUEUR) {
			nombreJoueur = msgServeur->nombreJoueur;
		}  else if(msgServeur->type == ANNUL) {
			printf("Le serveur s'est déconnecté ...\n");
			fermerSocket();
		} else if(msgServeur->type == FIN) {
			printf("Vous avez gagné la partie!\n",joueurs[num].pseudo);
			afficherScores();
			fermerSocket();
		} else {
			printf("ERROR : message inattendu : %d\n", msgServeur->type);
			fermerSocket();
		}
	}
}

int decouvrirJeton () {
	static int jetons[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
	int jet, hasard;
	static nombreJeton = 12;
	
	if(nombreJeton  > 0){
		hasard = rand() % nombreJeton;
		jet = jetons[hasard];
		nombreJeton--;
		jetons[hasard] = jetons[nombreJeton];	
	}
	else{
		jet = -1;
	}
	return jet;
}

int premierGauche(int de) {
	int premierGauche = -1;
	int i;
	for(i = 36; i > -1; --i) {
		if(tableauBase[i] == de) {
			break;
		 }
	}
	if (i > 0 && tableauBase[i-1] == de && joueurs[numJoueur].plateau[i-1] == 0) {
		i--;
	}				
	for(; i > -1; --i) {
		if(joueurs[numJoueur].plateau[i] == 0) {
			premierGauche = i;
			break;
		}
	}
	return premierGauche;
}

int premierDroite(int de) {
	int premierDroite = 36;
	int i;
	for(i = 0; i < 36; ++i) {
		if(tableauBase[i] == de){
			break;
		}
	}
	if (i < 35 && tableauBase[i+1] == de && joueurs[numJoueur].plateau[i+1] == 0) {
		i++;
	}
	for(; i < 36; ++i) {
		if(joueurs[numJoueur].plateau[i] == 0) {
			premierDroite = i;
			break;
		}
	}
	return premierDroite;
}

int placementJeton(int de) {
	int pos = -1;
	int premGauche = premierGauche(de);
	int premDroit = premierDroite(de);
	// Si une seule case possible
	if (premDroit == premGauche) {
		pos = premDroit;
	} else if (premGauche == -1) {
		pos = premDroit;
	} else if (premDroit == 36) {
		pos = premGauche;
	}
	
	// Si plusieurs cases possibles
	while (pos == -1) {
		char chaine[5];
		printf("Case de droite (D) ou case de gauche (G) ? \nou voir les tableaux (t)\n");
		fgets(chaine,5,stdin);
		if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
		}
		if(chaine[0] == 'D' || chaine[0] == 'd') {
			pos = premDroit;
		} else if (chaine[0] == 'G' || chaine[0] == 'g') {
			pos = premGauche;				
		}
	}
	return pos;
}

int checkFinito() {
	int ok = 1;
	int jeton = 0;
	int i;
	for (i = 0; i < 36; ++i) {
		if (joueurs[numJoueur].plateau[i] != 0 && joueurs[numJoueur].plateau[i] > jeton) {
			jeton = joueurs[numJoueur].plateau[i];
		} else {
			ok = -1;
			break;
		}
	}
	return ok;
}

int verifierFinito() {
	char chaine[5];
	do {
		printf("Finito ? oui (o) / non (n)\nou voir les tableaux (t)\n");
		fgets(chaine, 5, stdin);
		if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
		}
	} while (chaine[0] != 'o' && chaine[0] != 'O' && chaine[0] != 'n' && chaine[0] != 'N');
	
	if ((chaine[0] == 'o' || chaine[0] == 'O') && checkFinito() == 1 ) {
		return FINITO;
	} else if(chaine[0] == 'n' || chaine[0] == 'N'){
		return PAS_FINITO;
	}
	else{
		printf("Finito non valide, vous n'avez pas encore gagné la partie\n");
		return PAS_FINITO;
	}
}

void afficherPlateau(int num) {
	int i, j;
	commencerLecture();
	memcpy(joueurs, pointeurMemoire, sizeof(joueurs));
	arreterLecture();
	printf("Plateau de %s :\n", joueurs[num].pseudo);
	printf(" _______________________________________________\n|");
	for (i = 0; i < 36; ++i) {
			if(joueurs[num].plateau[i] == 0){
				printf("%d\t", tableauBase[i]);
			}
			else{
				couleur(joueurs[num].couleur);
				printf("%d\t",joueurs[num].plateau[i]);
				couleur("0");
			}
			if ( i % 6  == 5) {
				printf("|\n|");
			}
		}
	printf("_______________________________________________|\n");
}

void afficherScores() {
	int i;
	commencerLecture();
	memcpy(joueurs, pointeurMemoire, sizeof(joueurs));
	arreterLecture();
	printf("________________________________\n");
	for(i = 0; i < nombreJoueursTotal; ++i) {
		printf("Score de %s : %d\n", joueurs[i].pseudo, joueurs[i].score);
	}
	printf("________________________________\n");
}

int demanderJeton(de) {
	int numJeton = 0;
	char chaine[5];
	if(nombreJetonPasJoue >= 3) {
		while(numJeton > 3 || numJeton < 1) {
			printf("Résultat du dé : %d\n", de);
			printf("Quel jeton voulez-vous placer ? (%d | %d | %d )\nou voir les tableaux (t)\n",jetonsDecouverts[0],jetonsDecouverts[1],jetonsDecouverts[2]);
			printf("Choisir 1, 2, 3\n");
			fgets(chaine,5,stdin);
			if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
			}
			numJeton = strtol(chaine, NULL, 10);
		}
	}
	else if(nombreJetonPasJoue == 2) {
		while(numJeton > 2 || numJeton < 1) {
			printf("Résultat du dé : %d\n", de);
			printf("Quel jeton voulez-vous placer ? (%d | %d)\n ou voir les tableaux (t)\n",jetonsDecouverts[0],jetonsDecouverts[1]);
			printf("Choisir 1, 2\n");
			fgets(chaine,5,stdin);
			if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
			}
			numJeton = strtol(chaine, NULL, 10);
		}
	} else {
		printf("Résultat du dé : %d\nou voir les tableaux (t)\n", de);
		printf("Il ne reste que 1 jeton. (%d)\n",jetonsDecouverts[0]);
		if(chaine[0] == 't' || chaine[0] == 'T'){
				afficherTableau();
		}
		numJeton = 1;
	}	
	nombreJetonPasJoue--;
	return numJeton-1;
}
void afficherTableau(){
	int i;
	printf("____________________________________________________________\n");
	for( i = 0;i < nombreJoueur;i++){
		afficherPlateau(i);
	}
	printf("____________________________________________________________\n");
}
int choisirJeton(de) {
	int jeton = 0;
	int i = 0;
	char chaine[5];
	
	while(jeton > 12 || jeton < 1) {
		printf("Résultat du dé : %d\n", de);
		printf("Quel jeton voulez-vous déplacer ?\n");
		fgets(chaine, 5, stdin);
		jeton = strtol(chaine, NULL, 10);
	}
	for(i = 0; i < 36; ++i) {
		if(joueurs[numJoueur].plateau[i] == jeton) {
			break;
		}
	}
	return i;
}

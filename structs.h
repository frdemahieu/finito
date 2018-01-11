/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient les différentes structures utilisées.
*/
#ifndef __STRUCTS__
#define __STRUCTS__

#include <stdlib.h>
#include <stdio.h>
#define  WRITE 1
#define  READ  0

//MsgClient

enum{CONNEXION,PLACEMENT,DEPLACEMENT,FINITO,PAS_FINITO,SERVEUR_QUIT,NOUVELLE_PARTIE};
//MsgServeur
enum{CONN_OK,CONN_KO,DEBUT,DE,FIN,SCORES,ANNUL,CHANGENUM,GAGNER,NOMBREJOUEUR};

typedef struct MessageClient {
	int type;
	int jeton;
	int nouvellePosition;
	int anciennePosition;
	int finito;
} MessageClient;

typedef struct InscriptionClient {
	int type;
	char pseudo[30];
}InscriptionClient;
typedef struct MessageServeur {
	int type;
	int numJoueur;
	int nombreJoueur;
	int de;
} MessageServeur;

typedef struct Client{
	int socket;
	char pseudo[30];
	int plateau[36];
	int score;
	char couleur[4];
} Client;
#endif

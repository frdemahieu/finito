/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient le code des diverses méthodes utilitaires.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>

#include "./util.h"
#include "./structs.h"

int shm_id;		// Id de la zone
int shm_id_int;		// Id de la zone

void enleverRetourLigne(char *a){
	*(a+strlen(a)-1) = '\0';
}

char* getZoneMemoire(char * fichier) {
	char* shm_zone;	// Zone
	key_t shm_key;	// Clé de la zone
	
	// Ouverture zone mémoire
	shm_key = ftok(fichier, 0);
	if ((shm_id = shmget(shm_key, sizeof(Client*) * 4, IPC_CREAT | 0666))  == -1) {
			perror("erreur shmget");
			exit(errno);
	}
	// Attachement zone mémoire
	if (*(shm_zone = (char *) shmat(shm_id, NULL, 0)) == -1) {
			perror("erreur shmat");
			exit(errno);
	}

	return shm_zone;
}

void* getZoneMemoireInt(char * fichier,int *i) {
	char* shm_zone;	// Zone
	key_t shm_key;	// Clé de la zone
	
	// Ouverture zone mémoire
	shm_key = ftok(fichier, 0);
	
	if ((shm_id_int = shmget(shm_key, sizeof(int), IPC_CREAT | 0666))  == -1) {
			perror("erreur shmget");
			exit(errno);
	}
	// Attachement zone mémoire
	if ((i = (int *) shmat(shm_id_int, NULL, 0)) == NULL) {
			perror("erreur shmat");
			exit(errno);
	}

	return shm_zone;
}

void detacherZoneMemoireInt(int* i) {
	if (shmdt(i) == -1) {
		perror("erreur shmdt\n");
		exit(errno);
	}
}

void detacherZoneMemoire(char* shm_zone) {
	if (shmdt(shm_zone) == -1) {
		perror("erreur shmdt\n");
		exit(errno);
	}
}

void supprimerZonesMemoire() {
	shmctl(shm_id, IPC_RMID, NULL);
	shmctl(shm_id_int, IPC_RMID, NULL);
}

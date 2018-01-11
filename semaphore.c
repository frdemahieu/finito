/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient le code des semaphores.
*/


#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/ipc.h>

#include "./semaphore.h"
#include "./structs.h"

int semid;
int nbrPersonne;
struct sembuf op;

void initSemaphore()
{
	key_t semKey;
	if( (semKey = ftok("semaphore.txt", 0)) < 0 ) {	
		perror("erreur ftok");
		exit(errno);
	}
			
	if( (semid = semget(semKey, 2, IPC_CREAT | 0666)) < 0 ) {	
		perror("erreur semget");
		exit(errno);
	}
	if( semctl(semid,0,SETVAL,1) < 0 ) {	
		perror("erreur semctl");
		exit(errno);
	}
	
	getZoneMemoireInt("nbrLecteur.txt",&nbrPersonne); 
	
	
}

void downSemaphore(int i) {
	op.sem_num = i; 
    op.sem_op = 1; 
    op.sem_flg = 0; 
    semop(semid, &op, 1);	
}

void upSemaphore(int i ) {
	op.sem_num = i;
	op.sem_op = -1; 
    semop(semid, &op, 1); 
}

void supprimerSemaphore() {
	semctl(semid, 0, IPC_RMID, 0); 
}

void detacherSemaphore() {
	detacherZoneMemoireInt(&nbrPersonne);
}

void commencerLecture() {
	
 downSemaphore(READ);
 nbrPersonne++;
 if (nbrPersonne == 1) {
	downSemaphore(WRITE);
 }
	upSemaphore(READ);
}

void arreterLecture() {
	
 downSemaphore(READ);
 nbrPersonne--;
 if (nbrPersonne == 0) {
  upSemaphore(WRITE);
 }
 upSemaphore(READ);
}

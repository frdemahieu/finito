/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient les prototypes des méthodes du fichier semaphore.c.
*/

#ifndef __SEMAPHORE__
#define __SEMAPHORE__

void initSemaphore();
void downSemaphore(int i );
void upSemaphore(int i );
void supprimerSemaphore();
void detacherSemaphore();
void commencerLecture();
void arreterLecture();

#endif

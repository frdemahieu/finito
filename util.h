/*
 nom : 
  de Mahieu François
  de Wasseige Antoine
 login :
  fdemahi
  adewass
 
 Le fichier contient les prototypes des diverses méthodes utilitaires.
*/

#ifndef __UTIL_H__
#define __UTIL_H__

void enleverRetourLigne(char*);
char* getZoneMemoire(char*);
void detacherZoneMemoire(char*);
void supprimerZonesMemoire();
void* getZoneMemoireInt(char* ,int*);
void detacherZoneMemoireInt(int*);

#endif

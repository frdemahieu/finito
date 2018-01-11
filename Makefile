#  nom : 
#  de Mahieu François
#  de Wasseige Antoine
# login :
#  fdemahi
#  adewass

all: serveur client clean

serveur: serveur.o util.o semaphore.o
	gcc -o serveur serveur.o util.o semaphore.o

serveur.o: serveur.c
	gcc -o serveur.o -c serveur.c
	
client: client.o util.o semaphore.o  jeu_client.o
	gcc -o client client.o util.o semaphore.o jeu_client.o

client.o: client.c
	gcc -o client.o -c client.c
	
jeu_client.o:
	gcc -o jeu_client.o -c jeu_client.c
	
util.o: util.c
	gcc -o util.o -c util.c
	
semaphore.o: semaphore.c
	gcc -o semaphore.o -c semaphore.c

clean:
	rm -rf *.o

mrproper: clean
	rm -rf serveur
	rm -rf client

/*

Questions:
-Quand le PacMan meurt,on fait reaparaitre ou les fantomes?

A Faire:
-Tester les retours de toute les fonctions (pthread_create et autres)
-Figer les fantomes
-Bloquer les signaux lors de la modification de l'interfae graphique
-HandlerThreadPacMan inutile
-Refaire le changement de direction du fantome

A ne pas oublier:
-Na pas lock le mutex lors d'un delai
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>	
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNE 21
#define NB_COLONNE 17

// Macros utilisees dans le tableau tab
#define VIDE         0
#define MUR          1
#define PACMAN       -2
#define PACGOM       -3
#define SUPERPACGOM  -4
#define BONUS        -5

// Autres macros
#define LENTREE 15
#define CENTREE 8

// Mes defines
#define DEBUG

//Struct
typedef struct 
{
 int L ;
 int C ;
 int couleur ;
 int cache ;
} S_FANTOME ;


//Var global
int L;
int C;
int DIR;
int nbPacGom;
int delai = 300;
int niveauJeu = 1;
int score = 0;
int mode = 1;
bool MAJScore = false;
int nbFantomesRouge = 1;
int nbFantomesVert = 2;
int nbFantomesMauve = 2;
int nbFantomesOrange = 2;
int nbVies = 3;
pthread_key_t key;
pthread_mutex_t mutexTab;
pthread_mutex_t mutexLCDIR;
pthread_mutex_t mutexNbPacGom;
pthread_mutex_t mutexDelai;
pthread_mutex_t mutexNiveauJeu;
pthread_mutex_t mutexScore;
pthread_mutex_t mutexNbFantomes;

//Pas besoin de mutex car on ne va pas modifer les valeurs
pthread_t tidPacMan;
pthread_t tidEvent;
pthread_t tidPacGom;
pthread_t tidScore;
pthread_t tidBonus;
pthread_t tidCompteurFantomes;
pthread_t tidThreadVie;

pthread_cond_t condNbPacGom;
pthread_cond_t condScore;
pthread_cond_t condNbFantomes;

//Fonctions
void* threadPacMan(void*);
void* threadEvent(void*);
void* threadPacGom(void*);
void* threadScore(void*);
void* threadBonus(void*);
void* threadCompteurFantomes(void*);
void* threadFantomes(void*);
void* threadVies(void*);
void HandlerInt(int s);
void HandlerHup(int s);
void HandlerUsr1(int s);
void HandlerUsr2(int s);
void HandlerThreadPacMan(void *arg);
void MonDessinePacMan(int l,int c,int dir);
void Debug(const char * format, ...);
void AfficheTab(void);

int tab[NB_LIGNE][NB_COLONNE]
={  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
	{1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
	{1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,0,1,1,0,1,0,1,1,1,0,1,0,1,1,0,1},
	{1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
	{1,1,1,1,0,1,1,0,1,0,1,1,0,1,1,1,1},
	{1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
	{1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
	{0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0},
	{1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
	{1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,1,1},
	{1,1,1,1,0,1,0,1,1,1,0,1,0,1,1,1,1},
	{1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1},
	{1,0,1,1,0,1,1,0,1,0,1,1,0,1,1,0,1},
	{1,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,1},
	{1,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,1},
	{1,0,0,0,0,1,0,0,1,0,0,1,0,0,0,0,1},
	{1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,0,1},
	{1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
	{1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

void DessineGrilleBase();
void Attente(int milli);


int main(int argc,char* argv[])
{
	srand((unsigned)time(NULL));

	// Ouverture de la fenetre graphique
	printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
	if (OuvertureFenetreGraphique() < 0)
	{
		printf("Erreur de OuvrirGrilleSDL\n");
		fflush(stdout);
		exit(1);
	}

	DessineGrilleBase();

	//Debut de notre prog
	printf("[Main][Debut]Pid: %d\n",getpid());

	//Masquer les signaux
	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGUSR1);
	sigaddset(&mask,SIGUSR2);
	sigaddset(&mask,SIGHUP);
	sigaddset(&mask,SIGINT);
	sigprocmask(SIG_SETMASK,&mask,NULL);

	if(pthread_mutex_init(&mutexTab,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexTab\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexLCDIR,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexLCDIR\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexNbPacGom,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexNbPacGom\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexDelai,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexDelai\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexNiveauJeu,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexNiveauJeu\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexScore,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexScore\n");
		exit(1);
	}
	if(pthread_mutex_init(&mutexNbFantomes,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexNbFantomes\n");
		exit(1);
	}
	if(pthread_cond_init(&condNbPacGom,NULL))
	{
		perror("[Main][Erreur]pthread_cond_init sur condNbPacGom\n");
		exit(1);
	}
	if(pthread_cond_init(&condScore,NULL))
	{
		perror("[Main][Erreur]pthread_cond_init sur condScore\n");
		exit(1);
	}
	if(pthread_cond_init(&condNbFantomes,NULL))
	{
		perror("[Main][Erreur]pthread_cond_init sur condNbFantomes\n");
		exit(1);
	}


	pthread_key_create(&key,NULL);
	

	//Ouverture des threads
	//Thread PacGom
	pthread_create(&tidPacGom,NULL,threadPacGom,NULL);
	//Thread Vies
	pthread_create(&tidThreadVie,NULL,threadVies,NULL);
	//Thread Event
	pthread_create(&tidEvent,NULL,threadEvent,NULL);
	//Thread Score
	pthread_create(&tidScore,NULL,threadScore,NULL);
	//Thread Bonus
	pthread_create(&tidBonus,NULL,threadBonus,NULL);
	//Thread Compteur Fantomes
	pthread_create(&tidCompteurFantomes,NULL,threadCompteurFantomes,NULL);

	//Au finial ne faire un join que sur l'event
	pthread_join(tidEvent,NULL);

	//Fin de notre prog
	if(pthread_mutex_destroy(&mutexTab))
	{
		perror("[Main][Erreur]pthread_mutex_destroy on mutexTab\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexLCDIR))
	{
		perror("[Main][Erreur]pthread_mutex_destroy on mutexLCDIR\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNbPacGom))
	{
		perror("[Main][Erreur]pthread_mutex_destroy sur mutexNbPacGom\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexDelai))
	{
		perror("[Main][Erreur]pthread_mutex_destroy sur mutexDelai\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNiveauJeu))
	{
		perror("[Main][Erreur]pthread_mutex_destroy sur mutexNiveauJeu\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexScore))
	{
		perror("[Main][Erreur]pthread_mutex_destroy sur mutexScore\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNbFantomes))
	{
		perror("[Main][Erreur]pthread_mutex_destroy sur mutexNbPacGom\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condNbPacGom))
	{
		perror("[Main][Erreur]pthread_cond_destroy sur condNbPacGom\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condScore))
	{
		perror("[Main][Erreur]pthread_cond_destroy sur condScore\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condNbFantomes))
	{
		perror("[Main][Erreur]pthread_cond_destroy sur condNbFantomes\n");
		exit(1);
	}
	printf("[Main][Fin]\n");

	// Fermeture de la fenetre
	printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
	FermetureFenetreGraphique();
	printf("OK\n"); fflush(stdout);

	exit(0);
}

void Attente(int milli)
{
	struct timespec del;
	del.tv_sec = milli/1000;
	del.tv_nsec = (milli%1000)*1000000;
	nanosleep(&del,NULL);
}

void DessineGrilleBase()
{
	for (int l=0 ; l<NB_LIGNE ; l++)
		for (int c=0 ; c<NB_COLONNE ; c++)
		{
		if (tab[l][c] == VIDE) EffaceCarre(l,c);
		if (tab[l][c] == MUR) DessineMur(l,c);
		}
}

void* threadPacMan(void*)
{
	Debug("[threadPacMan][Debut]Tid: %d\n",pthread_self());

	int etat;
	//Armement des signaux sur tout le processus
	//sigint
	struct sigaction A1;
	A1.sa_handler = HandlerInt;
	A1.sa_flags = 0;
	sigemptyset(&A1.sa_mask);
	sigaction(SIGINT,&A1,NULL);
	//sighup
	struct sigaction A2;
	A2.sa_handler = HandlerHup;
	A2.sa_flags = 0;
	sigemptyset(&A2.sa_mask);
	sigaction(SIGHUP,&A2,NULL);
	//sigusr1
	struct sigaction A3;
	A3.sa_handler = HandlerUsr1;
	A3.sa_flags = 0;
	sigemptyset(&A3.sa_mask);
	sigaction(SIGUSR1,&A3,NULL);
	//sigusr2
	struct sigaction A4;
	A4.sa_handler = HandlerUsr2;
	A4.sa_flags = 0;
	sigemptyset(&A4.sa_mask);
	sigaction(SIGUSR2,&A4,NULL);
	

	//La fonction lorsque la thread meurt
	//pthread_cleanup_push(HandlerThreadPacMan,NULL);

	//Cancel du thread quand il meurt
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&etat);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&etat);

	//Mask
	sigset_t mask3;

	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	L = 15;
	C = 8;
	DIR = GAUCHE;
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}

	if(pthread_mutex_lock(&mutexTab))
	{

		perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexTab\n");
		exit(1);
	}

	sigfillset(&mask3);
	sigprocmask(SIG_SETMASK,&mask3,NULL);

	MonDessinePacMan(15,8,GAUCHE);

	sigemptyset(&mask3);
	sigaction(SIGINT,&A1,NULL);
	sigaction(SIGHUP,&A2,NULL);
	sigaction(SIGUSR1,&A3,NULL);
	sigaction(SIGUSR2,&A4,NULL);
	sigprocmask(SIG_SETMASK,&mask3,NULL);

	if(pthread_mutex_unlock(&mutexTab))
	{
		perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexTab\n");
		exit(1);
	}

	while(1)
	{
		printf("[threadPacMan]Boucle\n");
		int modifier = 0;
		int mangerPacGom = 0;
		int mangerSuperPacGom = 0;
		int mangerBonus = 0;
		int L2 = L;
		int C2 = C;
		int mort = 0;
		//Zone critique ne pas recevoir de signal durant l'attente
		sigfillset(&mask3);
		sigprocmask(SIG_SETMASK,&mask3,NULL);

		Attente(delai);

		sigemptyset(&mask3);
		sigaction(SIGINT,&A1,NULL);
		sigaction(SIGHUP,&A2,NULL);
		sigaction(SIGUSR1,&A3,NULL);
		sigaction(SIGUSR2,&A4,NULL);
		sigprocmask(SIG_SETMASK,&mask3,NULL);

		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}

		switch(DIR)
		{
			case HAUT:
				if(tab[L - 1][C] != MUR)
				{
					if(tab[L - 1][C] == PACGOM)
						mangerPacGom++;
					else if(tab[L - 1][C] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L - 1][C] == BONUS)
						mangerBonus++;
					else if(tab[L - 1][C] != VIDE)
					{
						printf("[threadPacMan]Mort: %d\n",tab[L - 1][C]);
						mort++;
					}
					modifier++;
					L2--;
				}
				else if(L == 0)
				{
					if(tab[NB_LIGNE - 1][C] == PACGOM)
						mangerPacGom++;
					else if(tab[NB_LIGNE - 1][C] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[NB_LIGNE - 1][C] == BONUS)
						mangerBonus++;
					modifier++;
					L2 = NB_LIGNE - 1;
				}	
				break;
			case BAS:
				if(tab[L + 1][C] != MUR)
				{
					if(tab[L + 1][C] == PACGOM)
						mangerPacGom++;
					else if(tab[L + 1][C] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L + 1][C] == BONUS)
						mangerBonus++;
					else if(tab[L + 1][C] != VIDE)
					{
						printf("[threadPacMan]Mort: %d\n",tab[L + 1][C]);
						mort++;
					}
					modifier++;
					L2++;
				}
				else if(L == NB_LIGNE -1)
				{
					if(tab[0][C] == PACGOM)
						mangerPacGom++;
					else if(tab[0][C] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[0][C] == BONUS)
						mangerBonus++;
					modifier++;
					L2 = 0;
				}
				break;
			case GAUCHE:
				if(tab[L][C - 1] != MUR)
				{
					if(tab[L][C - 1] == PACGOM)
						mangerPacGom++;
					else if(tab[L][C - 1] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L][C - 1] == BONUS)
						mangerBonus++;
					else if(tab[L][C - 1] != VIDE)
					{
						printf("[threadPacMan]Mort: %d\n",tab[L][C - 1]);
						mort++;
					}
					modifier++;
					C2--;
				}
				else if(C == 0)
				{
					if(tab[L][NB_COLONNE - 1] == PACGOM)
						mangerPacGom++;
					else if(tab[L][NB_COLONNE - 1] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L][NB_COLONNE - 1] == BONUS)
						mangerBonus++;
					modifier++;
					C2 = NB_COLONNE - 1;
				}
				break;
			case DROITE:
				if(tab[L][C + 1] != MUR)
				{
					if(tab[L][C + 1] == PACGOM)
						mangerPacGom++;
					else if(tab[L][C + 1] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L][C + 1] == BONUS)
						mangerBonus++;
					else if(tab[L][C + 1] != VIDE)
					{
						printf("[threadPacMan]Mort: %d\n",tab[L][C + 1]);
						mort++;
					}
					modifier++;
					C2++;
				}
				else if(C == NB_COLONNE - 1)
				{
					if(tab[L][0] == PACGOM)
						mangerPacGom++;
					else if(tab[L][0] == SUPERPACGOM)
						mangerSuperPacGom++;
					else if(tab[L][0] == BONUS)
						mangerBonus++;
					modifier++;
					C2 = 0;
				}
				break;
		}
		
		if(modifier && !mort)
		{
			sigfillset(&mask3);
			sigprocmask(SIG_SETMASK,&mask3,NULL);

			tab[L][C] = VIDE;
			//Section Critique Interface graphique
			EffaceCarre(L,C);
			MonDessinePacMan(L2,C2,DIR);

			sigemptyset(&mask3);
			sigaction(SIGINT,&A1,NULL);
			sigaction(SIGHUP,&A2,NULL);
			sigaction(SIGUSR1,&A3,NULL);
			sigaction(SIGUSR2,&A4,NULL);
			sigprocmask(SIG_SETMASK,&mask3,NULL);
		}
		if(mangerPacGom && !mort)
		{
			//Incrementer le score de 1
			if(pthread_mutex_lock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexScore");
				exit(1);
			}
			score++;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexScore");
				exit(1);
			}
			pthread_cond_signal(&condScore);
			//Decrementer le nbPacGom de 1
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexNbPacGom");
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexNbPacGom");
				exit(1);
			}
			pthread_cond_signal(&condNbPacGom);
		}
		if(mangerSuperPacGom && !mort)
		{
			//incrementer le score de 5
			if(pthread_mutex_lock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexScore");
				exit(1);
			}
			score += 5;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexScore");
				exit(1);
			}
			pthread_cond_signal(&condScore);
			//Decrementer le nbPacGom de 1
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexNbPacGom");
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexNbPacGom");
				exit(1);
			}
			pthread_cond_signal(&condNbPacGom);
		}
		if(mangerBonus && !mort)
		{
			//incrementer le score de 30
			if(pthread_mutex_lock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_lock on mutexScore");
				exit(1);
			}
			score += 30;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexScore");
				exit(1);
			}
			pthread_cond_signal(&condScore);
		}
		if(mort)
		{
			EffaceCarre(L,C);
			pthread_cancel(tidPacMan);
		}

		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
		pthread_testcancel();
	}
}

void* threadEvent(void*)
{
	printf("[threadEvent][Debut]Tid: %d\n",pthread_self());

	//On bloque les signaux dans le threadEvent pour pas les envoyer a sois meme
	sigset_t mask2;
	sigemptyset(&mask2);
	sigaddset(&mask2,SIGUSR1);
	sigaddset(&mask2,SIGUSR2);
	sigaddset(&mask2,SIGHUP);
	sigaddset(&mask2,SIGINT);
	sigprocmask(SIG_SETMASK,&mask2,NULL);
	
	EVENT_GRILLE_SDL event;

	while(1)
	{
		event = ReadEvent();

		switch(event.type)
		{
			case CROIX:
				pthread_exit(NULL);
				break;
			case CLAVIER:
				switch(event.touche)
				{
					case KEY_UP:
						kill(getpid(),SIGUSR1);
						Debug("[threadEvent]SIGUSR1\n");
						break;
					case KEY_DOWN:
						kill(getpid(),SIGUSR2);
						Debug("[threadEvent]SIGUSR2\n");
						break;
					case KEY_LEFT:
						kill(getpid(),SIGINT);
						Debug("[threadEvent]SIGINT\n");
						break;
					case KEY_RIGHT:
						kill(getpid(),SIGHUP);
						Debug("[threadEvent]SIGHUP\n");
						break;
				}
				break;
		}
	}
}

void* threadPacGom(void*)
{
	printf("[threadPacGom][Debut]Tid: %d\n",pthread_self());

	while(1)
	{
		int valeur1 = 0;
		int valeur2 = 0;
		int valeur3 = 0;

		//Init PacGom
		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}
		if(pthread_mutex_lock(&mutexNbPacGom))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNbPacGom\n");
			exit(1);
		}
		for(int x = 0; x < NB_LIGNE; x++)
		{
			for(int y = 0; y < NB_COLONNE; y++)
			{
				if(tab[x][y] == VIDE)
				{
					if(!(x == 15 && y == 8) && !(x == 8 && y == 8) && !(x == 9 && y == 8) && !(x == 2 && y == 1) && !(x == 2 && y == 15) && !(x == 15 && y == 1) && !(x == 15 && y == 15))
					{
						DessinePacGom(x,y);
						tab[x][y] = PACGOM;
						nbPacGom++;
					}
				}
			}
		}
		DessineSuperPacGom(2,1);
		tab[2][1] = SUPERPACGOM;
		DessineSuperPacGom(2,15);
		tab[2][15] = SUPERPACGOM;
		DessineSuperPacGom(15,1);
		tab[15][1] = SUPERPACGOM;
		DessineSuperPacGom(15,15);
		tab[15][15] = SUPERPACGOM;
		nbPacGom += 4;

		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}	

		if(pthread_mutex_unlock(&mutexNbPacGom))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNbPacGom\n");
			exit(1);
		}

		//Afficher le niveau de jeu
		if(pthread_mutex_lock(&mutexNiveauJeu))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNiveauJeu\n");
			exit(1);
		}
		DessineChiffre(14,22,niveauJeu);
		if(pthread_mutex_unlock(&mutexNiveauJeu))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNiveauJeu\n");
			exit(1);
		}

		//Attendre que le joueur gagne
		while(nbPacGom > 0)
		{
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNbPacGom");
				exit(1);
			}
			pthread_cond_wait(&condNbPacGom,&mutexNbPacGom);

			valeur1 = nbPacGom / 100;
			valeur2 = (nbPacGom - ((nbPacGom / 100) * 100)) / 10 ;
			valeur3 = (nbPacGom - ((nbPacGom / 100) * 100) - (((nbPacGom - ((nbPacGom / 100) * 100)) / 10) * 10)) / 1;

			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNbPacGom");
				exit(1);
			}

			DessineChiffre(12,22,valeur1);
			DessineChiffre(12,23,valeur2);
			DessineChiffre(12,24,valeur3);
		}

		Debug("[threadPacGom]Partie Gagnee");
		//Le jouer a gagner le niveau
		//Incrementer le niveai de jeu
		if(pthread_mutex_lock(&mutexNiveauJeu))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNiveauJeu\n");
			exit(1);
		}
		niveauJeu++;
		if(pthread_mutex_unlock(&mutexNiveauJeu))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNiveauJeu\n");
			exit(1);
		}
		//Augmenter la vitesse du jeu
		if(pthread_mutex_lock(&mutexDelai))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexDelai\n");
			exit(1);
		}
		delai /= 2;
		if(pthread_mutex_unlock(&mutexDelai))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexDelai\n");
			exit(1);
		}
		//On place on pacGom a la case de spawn d'origine du pacGom
		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}
		if(pthread_mutex_lock(&mutexNbPacGom))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNbPacGom\n");
			exit(1);
		}
		tab[15][8] = PACGOM;
		DessinePacGom(15,8);
		nbPacGom++;
		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
		if(pthread_mutex_unlock(&mutexNbPacGom))
		{
			perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNbPacGom\n");
			exit(1);
		}
	}
}

void* threadScore(void*)
{
	printf("[threadScore][Debut]Tid: %d\n",pthread_self());

	while(1)
	{
		int valeur1 = 0;
		int valeur2 = 0;
		int valeur3 = 0;
		int valeur4 = 0;
		int reste = 0;

		if(pthread_mutex_lock(&mutexScore))
		{
			perror("[threadScore][Erreur]pthread_mutex_lock on mutexScore");
			exit(1);
		}

		while(MAJScore == false)
		{
			pthread_cond_wait(&condScore,&mutexScore);
		}

		reste = score;
		valeur1 = score / 1000;
		reste -= valeur1 * 1000;
		valeur2 = reste / 100;
		reste -= valeur2 * 100;
		valeur3 = reste / 10;
		reste -= valeur3 * 10;
		valeur4 = reste;

		DessineChiffre(16,22,valeur1);
		DessineChiffre(16,23,valeur2);
		DessineChiffre(16,24,valeur3);
		DessineChiffre(16,25,valeur4);

		MAJScore = false;

		if(pthread_mutex_unlock(&mutexScore))
		{
			perror("[threadScore][Erreur]pthread_mutex_unlock on mutexScore");
			exit(1);
		}
	}
}

void* threadBonus(void*)
{
	printf("[threadScore][Debut]Tid: %d\n",pthread_self());
	srand(time(NULL));
	int delaiMin = 10;
	int delaiMax = 20;
	int xMin = 0;
	int xMax = NB_LIGNE - 1;
	int yMin = 0;
	int yMax = NB_COLONNE - 1;
	int delaiBonus = 10000;
	while(1)
	{
		int randX = 0;
		int randY = 0;
		int randNumber = delaiMin + (rand() % (delaiMax - delaiMin + 1));
		randNumber *= 1000;

		Attente(randNumber);

		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadBonus][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}
		do
		{
			randX = xMin + (rand() % (xMax - xMin + 1));
			randY = yMin + (rand() % (yMax - yMin + 1));
		}while(tab[randX][randY] != VIDE);
		DessineBonus(randX,randY);
		tab[randX][randY] = BONUS;

		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadBonus][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}

		Attente(delaiBonus);

		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadBonus][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}

		if(tab[randX][randY] == BONUS)
		{
			tab[randX][randY] = VIDE;
			EffaceCarre(randX,randY);
		}

		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadBonus][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
	}
}

void* threadCompteurFantomes(void*)
{
	printf("[threadCompteurFantomes][Debut]Tid: %d\n",pthread_self());

	while(1)
	{
		if(pthread_mutex_lock(&mutexNbFantomes))
		{
			perror("[threadCompteurFantomes][Erreur]pthread_mutex_lock on mutexNbFantomes");
			exit(1);
		}

		while(nbFantomesRouge == 2 && nbFantomesVert == 2 && nbFantomesOrange == 2 && nbFantomesMauve == 2)
		{
			pthread_cond_wait(&condNbFantomes,&mutexNbFantomes);
		}

		while(nbFantomesRouge < 2)
		{
			S_FANTOME* paramFantome = new S_FANTOME;
			paramFantome->L = 9;
			paramFantome->C = 8;
			paramFantome->couleur = ROUGE;
			paramFantome->cache = 0;
			pthread_create(NULL,NULL,threadFantomes,paramFantome);
			printf("[threadCompteurFantomes]Fantome  Rouge ++\n");
			nbFantomesRouge++;
		}
		while(nbFantomesVert < 2)
		{
			S_FANTOME* paramFantome = new S_FANTOME;
			paramFantome->L = 9;
			paramFantome->C = 8;
			paramFantome->couleur = VERT;
			paramFantome->cache = 0;
			pthread_create(NULL,NULL,threadFantomes,paramFantome);
			nbFantomesVert++;
		}
		while(nbFantomesOrange < 2)
		{
			S_FANTOME* paramFantome = new S_FANTOME;
			paramFantome->L = 9;
			paramFantome->C = 8;
			paramFantome->couleur = ORANGE;
			paramFantome->cache = 0;
			pthread_create(NULL,NULL,threadFantomes,paramFantome);
			nbFantomesOrange++;
		}
		while(nbFantomesMauve < 2)
		{
			S_FANTOME* paramFantome = new S_FANTOME;
			paramFantome->L = 9;
			paramFantome->C = 8;
			paramFantome->couleur = MAUVE;
			paramFantome->cache = 0;
			pthread_create(NULL,NULL,threadFantomes,paramFantome);
			nbFantomesMauve++;
		}

		if(pthread_mutex_unlock(&mutexNbFantomes))
		{
			perror("[threadCompteurFantomes][Erreur]pthread_mutex_unlock on mutexNbFantomes");
			exit(1);
		}

	}
}

void* threadFantomes(void* p)
{
	printf("[threadFantomes][Debut]Tid: %d\n",pthread_self());

	sigset_t maskFantomes;
	sigfillset(&maskFantomes);
	sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

	srand(time(NULL));
	S_FANTOME* paramFantome = (S_FANTOME*)p;
	char couleur[10] = {0};
	int dirFantome = HAUT;
	int min = 0;
	int max = 0;
	int spawnFantome = 0;
	int newDir = 0;
	int changeDir = 0;
	int newDirOk = 0;
	int newTabVal = 0;

	if(paramFantome->couleur == ROUGE)
		strcat(couleur,"Rouge");
	else if(paramFantome->couleur == VERT)
		strcat(couleur,"Vert");
	else if(paramFantome->couleur == ORANGE)
		strcat(couleur,"Orange");
	else if(paramFantome->couleur == MAUVE)
		strcat(couleur,"Mauve");
	printf("[threadFantomes]L: %d C: %d Couleur: %s Cache: %d\n",paramFantome->L,paramFantome->C,couleur,paramFantome->cache);

	pthread_setspecific(key,NULL);

	do
	{
		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}
		if(tab[9][8] == VIDE && tab[8][8] == VIDE)
		{
			paramFantome->cache = VIDE;
			DessineFantome(paramFantome->L,paramFantome->C,paramFantome->couleur,dirFantome);
			tab[paramFantome->L][paramFantome->C] = pthread_self();
			spawnFantome = 1;
			printf("[threadFantomes]Fantome place a L: %d C: %d\n",paramFantome->L,paramFantome->C);
		}
		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
	}while(!spawnFantome);

	while(1)
	{
		printf("[threadFantomes: %d] Boucle\n",pthread_self());
		newDir = 0;
		newDirOk = 0;
		int vectVide = 1;
		int vect[4] = {0};

		vect[0] = 0;
		vect[1] = 0;
		vect[2] = 0;
		vect[3] = 0;
		max = 0;
		//system("clear");
		//AfficheTab();
		//printf("[threadFantomes]Boucle\n");
		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}

		//Restitution du Cache
		switch(paramFantome->cache)
		{
			case VIDE:
				EffaceCarre(paramFantome->L,paramFantome->C);
				newTabVal = VIDE;
				break;
			case PACGOM:
				DessinePacGom(paramFantome->L,paramFantome->C);
				newTabVal = PACGOM;
				break;
			case SUPERPACGOM:
				DessineSuperPacGom(paramFantome->L,paramFantome->C);
				newTabVal = SUPERPACGOM;
				break;
			case BONUS:
				DessineBonus(paramFantome->L,paramFantome->C);
				newTabVal = BONUS;
				break;
			default:
				printf("[threadFantomes][Erreur]Switch cache: %d\n",paramFantome->cache);
				break;
		}
		tab[paramFantome->L][paramFantome->C] = newTabVal;
		printf("[threadFantomes: %d] Cache Restituer\n",pthread_self());
		
		//Mouvement
		switch(dirFantome)
		{
			case HAUT:
				if(mode == 1 && tab[paramFantome->L - 1][paramFantome->C] == PACMAN)
				{	
					//On tue PacMan
					pthread_cancel(tidPacMan);
					EffaceCarre(paramFantome->L - 1,paramFantome->C);
					tab[paramFantome->L - 1][paramFantome->C] = VIDE;
				}
				if(tab[paramFantome->L - 1][paramFantome->C] == VIDE || tab[paramFantome->L - 1][paramFantome->C] == PACGOM || tab[paramFantome->L - 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L - 1][paramFantome->C] == BONUS)
				{
					//printf("[threadFantomes]HAUT\n");
					paramFantome->cache = tab[paramFantome->L - 1][paramFantome->C];
					paramFantome->L--;
				}
				else
					changeDir++;
				break;
			case BAS:
				if(mode == 1 && tab[paramFantome->L + 1][paramFantome->C] == PACMAN)
				{	
					//On tue PacMan
					pthread_cancel(tidPacMan);
					EffaceCarre(paramFantome->L + 1,paramFantome->C);
					tab[paramFantome->L + 1][paramFantome->C] = VIDE;
				}
				if(tab[paramFantome->L + 1][paramFantome->C] == VIDE || tab[paramFantome->L + 1][paramFantome->C] == PACGOM || tab[paramFantome->L + 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L + 1][paramFantome->C] == BONUS)
				{
					//printf("[threadFantomes]BAS\n");
					paramFantome->cache = tab[paramFantome->L + 1][paramFantome->C];
					paramFantome->L++;
				}
				else
					changeDir++;
				break;
			case GAUCHE:
				if(mode == 1 && tab[paramFantome->L][paramFantome->C - 1] == PACMAN)
				{	
					//On tue PacMan
					pthread_cancel(tidPacMan);
					EffaceCarre(paramFantome->L,paramFantome->C - 1);
					tab[paramFantome->L][paramFantome->C - 1] = VIDE;
				}
				if(tab[paramFantome->L][paramFantome->C - 1] == VIDE || tab[paramFantome->L][paramFantome->C - 1] == PACGOM || tab[paramFantome->L][paramFantome->C - 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C - 1] == BONUS)
				{
					//printf("[threadFantomes]GAUCHE\n");
					paramFantome->cache = tab[paramFantome->L][paramFantome->C - 1];
					paramFantome->C--;
				}
				else 
					changeDir++;
				break;
			case DROITE:
				if(mode == 1 && tab[paramFantome->L][paramFantome->C + 1] == PACMAN)
				{	
					//On tue PacMan
					pthread_cancel(tidPacMan);
					EffaceCarre(paramFantome->L,paramFantome->C + 1);
					tab[paramFantome->L][paramFantome->C + 1] = VIDE;
				}
				if(tab[paramFantome->L][paramFantome->C + 1] == VIDE || tab[paramFantome->L][paramFantome->C + 1] == PACGOM || tab[paramFantome->L][paramFantome->C + 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C + 1] == BONUS)
				{
					//printf("[threadFantomes]DROITE\n");
					paramFantome->cache = tab[paramFantome->L][paramFantome->C + 1];
					paramFantome->C++;
				}
				else
					changeDir++;
				break;
			default:
				printf("[threadFantomes][Erreur]Switch(dirFantome)\n");
				break;
		}
		printf("[threadFantomes: %d] Apres switch(dirFantome)\n",pthread_self());
		if(changeDir)
		{
			//Recupere le nombre de cases vides
			
			if(tab[paramFantome->L - 1][paramFantome->C] == VIDE || tab[paramFantome->L - 1][paramFantome->C] == PACGOM || tab[paramFantome->L - 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L - 1][paramFantome->C] == BONUS)
			{
				//La case du haut est dispo
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = HAUT;
			}
			if(tab[paramFantome->L + 1][paramFantome->C] == VIDE || tab[paramFantome->L + 1][paramFantome->C] == PACGOM || tab[paramFantome->L + 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L + 1][paramFantome->C] == BONUS)
			{
				//La case du haut est dispo
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = BAS;
			}
			if(tab[paramFantome->L][paramFantome->C - 1] == VIDE || tab[paramFantome->L][paramFantome->C - 1] == PACGOM || tab[paramFantome->L][paramFantome->C - 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C - 1] == BONUS)
			{
				//La case du haut est dispo
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = GAUCHE;
			}
			if(tab[paramFantome->L][paramFantome->C + 1] == VIDE || tab[paramFantome->L][paramFantome->C + 1] == PACGOM || tab[paramFantome->L][paramFantome->C + 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C + 1] == BONUS)
			{
				//La case du haut est dispo
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = DROITE;
			}

			for(int i = 0; i < 4; i++)
			{
				if(vect[i] != 0)
				{
					max++;
					vectVide = 0;
				}
			}
			max--;

			do
			{
				if(!vectVide)
				{
					newDir = min + rand() % (max - min + 1);
					newDir += 500000;
					if(newDir != dirFantome)
					{
						switch(newDir)
						{
							case HAUT:
								if(mode == 1 && tab[paramFantome->L - 1][paramFantome->C] == PACMAN)
								{	
									//On tue PacMan
									pthread_cancel(tidPacMan);
									EffaceCarre(paramFantome->L - 1,paramFantome->C);
									tab[paramFantome->L - 1][paramFantome->C] = VIDE;
								}
								if(tab[paramFantome->L - 1][paramFantome->C] == VIDE || tab[paramFantome->L - 1][paramFantome->C] == PACGOM || tab[paramFantome->L - 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L - 1][paramFantome->C] == BONUS)
								{
									//printf("[threadFantomes]newDir HAUT\n");
									paramFantome->cache = tab[paramFantome->L - 1][paramFantome->C];
									paramFantome->L--;
									newDirOk++;
								}
								break;
							case BAS:
								if(mode == 1 && tab[paramFantome->L + 1][paramFantome->C] == PACMAN)
								{	
									//On tue PacMan
									pthread_cancel(tidPacMan);
									EffaceCarre(paramFantome->L + 1,paramFantome->C);
									tab[paramFantome->L + 1][paramFantome->C] = VIDE;
								}
								if(tab[paramFantome->L + 1][paramFantome->C] == VIDE || tab[paramFantome->L + 1][paramFantome->C] == PACGOM || tab[paramFantome->L + 1][paramFantome->C] == SUPERPACGOM || tab[paramFantome->L + 1][paramFantome->C] == BONUS)
								{
									//printf("[threadFantomes]newDir BAS\n");
									paramFantome->cache = tab[paramFantome->L + 1][paramFantome->C];
									paramFantome->L++;
									newDirOk++;
								}
								break;
							case GAUCHE:
								if(mode == 1 && tab[paramFantome->L][paramFantome->C - 1] == PACMAN)
								{	
									//On tue PacMan
									pthread_cancel(tidPacMan);
									EffaceCarre(paramFantome->L,paramFantome->C - 1);
									tab[paramFantome->L][paramFantome->C - 1] = VIDE;
								}
								if(tab[paramFantome->L][paramFantome->C - 1] == VIDE || tab[paramFantome->L][paramFantome->C - 1] == PACGOM || tab[paramFantome->L][paramFantome->C - 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C - 1] == BONUS)
								{
									//printf("[threadFantomes]newDir GAUCHE\n");
									paramFantome->cache = tab[paramFantome->L][paramFantome->C - 1];
									paramFantome->C--;
									newDirOk++;
								}
								break;
							case DROITE:
								if(mode == 1 && tab[paramFantome->L][paramFantome->C + 1] == PACMAN)
								{	
									//On tue PacMan
									pthread_cancel(tidPacMan);
									EffaceCarre(paramFantome->L,paramFantome->C + 1);
									tab[paramFantome->L][paramFantome->C + 1] = VIDE;
								}
								if(tab[paramFantome->L][paramFantome->C + 1] == VIDE || tab[paramFantome->L][paramFantome->C + 1] == PACGOM || tab[paramFantome->L][paramFantome->C + 1] == SUPERPACGOM || tab[paramFantome->L][paramFantome->C + 1] == BONUS)
								{
									//printf("[threadFantomes]newDir DROITE\n");
									paramFantome->cache = tab[paramFantome->L][paramFantome->C + 1];
									paramFantome->C++;
									newDirOk++;
								}
								break;
							default:
								printf("[threadFantomes][Erreur]Switch(newDir): %d\n",newDir);
								break;
						}
					}
				}
				else
				{
					newDir = dirFantome;
					newDirOk++;
				}
			}while(!newDirOk);
			dirFantome = newDir;
			changeDir--;
		}
		printf("[threadFantomes: %d] Apres switch(changeDir)\n",pthread_self());

		DessineFantome(paramFantome->L,paramFantome->C,paramFantome->couleur,dirFantome);
		tab[paramFantome->L][paramFantome->C] = pthread_self();
	
		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
		Attente(delai * (5 / 3));
	}
}

void* threadVies(void*)
{
	printf("[threadVies][Debut]Tid: %d\n",pthread_self());

	DessineChiffre(18,22,nbVies);

	while(nbVies > 0)
	{
		//Thread PacMan
		pthread_create(&tidPacMan,NULL,threadPacMan,NULL);
		printf("[threadVies]pthread_create\n");
		pthread_join(tidPacMan,NULL);
		nbVies--;
		printf("[threadVies]pthread_join\n");
		DessineChiffre(18,22,nbVies);
	}

	DessineGameOver(9,4);
	//Figer les fantomes
	
	//Le pacMan meurt fin de la game
	pthread_exit(NULL);
}

void HandlerInt(int s)
{
	Debug("[HandlerInt]SIGINT\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerInt][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C - 1] != MUR)
	{
		DIR = GAUCHE;
		Debug("[HandlerInt]DIR = GAUCHE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerInt][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerHup(int s)
{	
	Debug("[HandlerHup]SIGHUP\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerHup][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C + 1] != MUR)
	{
		DIR = DROITE;
		Debug("[HandlerHup]DIR = DROITE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerHup][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr1(int s)
{
	Debug("[HandlerUsr1]SIGUSR1\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerUsr1][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L - 1][C] != MUR)
	{
		DIR = HAUT;
		Debug("[HandlerUsr1]DIR = HAUT\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerUsr1][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr2(int s)
{
	Debug("[HandlerUsr2]SIGUSR2\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerUsr2][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L + 1][C] != MUR)
	{
		DIR = BAS;
		Debug("[HandlerUsr2]DIR = BAS\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerUsr2][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerThreadPacMan(void *arg)
{
	system("clear");
	printf("[HandlerThreadPacMan] PacMan est mort!");
	exit(1);
}

void MonDessinePacMan(int l,int c,int dir)
{
	//Debug("[MonDessinePacMan]l = %d,c = %d,dir = %d",l,c,dir);
	
	//Section Critique Interface graphique
	DessinePacMan(l,c,dir);
	//Section critique tab
	tab[l][c] = PACMAN;

	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[MonDessinePacMan][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	L = l;
	C = c;
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[MonDessinePacMan][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void Debug(const char * format, ...)
{
	#ifdef DEBUG
    int tailleFormat = strlen(format);

    va_list args;
    va_start (args, format);
    vfprintf (stdout, format, args);
    if(format[tailleFormat - 1] != '\n')
    {
        printf("\n");
    }
    va_end(args);
    #endif
}

void AfficheTab(void)
{
	if(pthread_mutex_lock(&mutexTab))
	{
		perror("[AfficheTab][Erreur]pthread_mutex_lock on mutexTab\n");
		exit(1);
	}
	for(int i = 0; i < NB_LIGNE; i++)
	{
		for(int j = 0; j < NB_COLONNE; j++)
		{
			printf("%3d ",tab[i][j]);
		}
		printf("\n");
	}
	if(pthread_mutex_unlock(&mutexTab))
	{
		perror("[AfficheTab][Erreur]pthread_mutex_unlock on mutexTab\n");
		exit(1);
	}
}






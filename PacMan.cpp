/*
Ca sert a quoi condNbPacGom?



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

//Var global
int L;
int C;
int DIR;
int nbPacGom;
int delai = 300;
int niveauJeu = 1;
pthread_mutex_t mutexTab;
pthread_mutex_t mutexLCDIR;
pthread_mutex_t mutexNbPacGom;

pthread_t tidPacMan;
pthread_t tidEvent;
pthread_t tidPacGom;
//Utile pour connaitre le pid? Ou pas besoin car le pid ne change pas mais si le thread demande
//le pid avant que l'autre thread ne soit lancer on peu avoir un probleme car tid = 0
pthread_mutex_t mutexTidPacMan;
pthread_mutex_t mutexTidEvent;
pthread_mutex_t mutexTidPacGom;


//Fonctions
void* threadPacMan(void*);
void* threadEvent(void*);
void* threadPacGom(void*);
void HandlerInt(int s);
void HandlerHup(int s);
void HandlerUsr1(int s);
void HandlerUsr2(int s);
void MonDessinePacMan(int l,int c,int dir);

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

///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
	EVENT_GRILLE_SDL event;
	char ok;

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

	//mutexTab
	if(pthread_mutex_init(&mutexTab,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexTab\n");
		exit(1);
	}
	//mutexLCDIR
	if(pthread_mutex_init(&mutexTab,NULL))
	{
		perror("[Main][Erreur]pthread_mutex_init sur mutexLCDIR\n");
		exit(1);
	}

	//Ouverture des threads
	//Thread PacGom
	pthread_create(&tidPacGom,NULL,threadPacGom,NULL);
	//Thread PacMan
	pthread_create(&tidPacMan,NULL,threadPacMan,NULL);
	//Thread Event
	pthread_create(&tidEvent,NULL,threadEvent,NULL);

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
	printf("[Main][Fin]\n");

	// Fermeture de la fenetre
	printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
	FermetureFenetreGraphique();
	printf("OK\n"); fflush(stdout);

	exit(0);
}

//*********************************************************************************************
void Attente(int milli)
{
	struct timespec del;
	del.tv_sec = milli/1000;
	del.tv_nsec = (milli%1000)*1000000;
	nanosleep(&del,NULL);
}

//*********************************************************************************************
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
	printf("[threadPacMan][Debut]Tid: %d\n",pthread_self());

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

	MonDessinePacMan(15,8,GAUCHE);

	while(1)
	{
		int modifier = 0;
		int L2 = L;
		int C2 = C;
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
		switch(DIR)
		{
			case HAUT:
				if(tab[L - 1][C] != MUR)
				{
					modifier++;
					L2--;
				}
				break;
			case BAS:
				if(tab[L + 1][C] != MUR)
				{
					modifier++;
					L2++;
				}
				break;
			case GAUCHE:
				if(tab[L][C - 1] != MUR)
				{
					modifier++;
					C2--;
				}
				break;
			case DROITE:
				if(tab[L][C + 1] != MUR)
				{
					modifier++;
					C2++;
				}
				break;
		}
		if(modifier)
		{
			sigfillset(&mask3);
			sigprocmask(SIG_SETMASK,&mask3,NULL);

			EffaceCarre(L,C);
			MonDessinePacMan(L2,C2,DIR);

			sigemptyset(&mask3);
			sigaction(SIGINT,&A1,NULL);
			sigaction(SIGHUP,&A2,NULL);
			sigaction(SIGUSR1,&A3,NULL);
			sigaction(SIGUSR2,&A4,NULL);
			sigprocmask(SIG_SETMASK,&mask3,NULL);
		}
	}

	if(pthread_mutex_unlock(&mutexTab))
	{
		perror("[threadPacMan][Erreur]pthread_mutex_unlock on mutexTab\n");
		exit(1);
	}

	printf("[threadPacMan][Fin]\n");
	pthread_exit(NULL);
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
						printf("[threadEvent]SIGUSR1\n");
						break;
					case KEY_DOWN:
						kill(getpid(),SIGUSR2);
						printf("[threadEvent]SIGUSR2\n");
						break;
					case KEY_LEFT:
						kill(getpid(),SIGINT);
						printf("[threadEvent]SIGINT\n");
						break;
					case KEY_RIGHT:
						kill(getpid(),SIGHUP);
						printf("[threadEvent]SIGHUP\n");
						break;
				}
				break;
		}
	}

	printf("[threadEvent][Fin]\n");
}

void* threadPacGom(void*)
{
	printf("[threadPacGom][Debut]Tid: %d\n",pthread_self());

	if(pthread_mutex_lock(&mutexTab))
	{

		perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexTab\n");
		exit(1);
	}

	for(int x = 0; x < NB_LIGNE; x++)
	{
		for(int y = 0; y < NB_COLONNE; y++)
		{
			if(tab[x][y] == VIDE)
			{
				if(!(x == 15 && y == 8) && !(x == 8 && y == 8) && !(x == 9 && y == 8))
				{
					DessinePacGom(x,y);
					nbPacGom++;
				}
			}
		}
	}
	DessineSuperPacGom(2,1);
	DessineSuperPacGom(2,15);
	DessineSuperPacGom(15,1);
	DessineSuperPacGom(15,15);
	nbPacGom += 4;

	if(pthread_mutex_unlock(&mutexTab))
	{
		perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexTab\n");
		exit(1);
	}

	if(pthread_mutex_lock(&mutexNbPacGom))
	{
		perror("[threadPacGom][Erreur]pthread_mutex_lock on mutexNbPacGom\n");
	}

	while(nbPacGom > 0)
	{
		//Tant qu'on a pas tout manger on a pas gagner

	}

	if(pthread_mutex_unlock(&mutexNbPacGom))
	{
		perror("[threadPacGom][Erreur]pthread_mutex_unlock on mutexNbPacGom\n");
	}

	printf("[threadPacGom][Fin]\n");
}

void MonDessinePacMan(int l,int c,int dir)
{
	DessinePacMan(l,c,dir);
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

void HandlerInt(int s)
{
	printf("[HandlerInt]SIGINT\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerInt][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C - 1] != MUR)
	{
		DIR = GAUCHE;
		printf("[HandlerInt]DIR = GAUCHE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerInt][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerHup(int s)
{	
	printf("[HandlerHup]SIGHUP\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerHup][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C + 1] != MUR)
	{
		DIR = DROITE;
		printf("[HandlerHup]DIR = DROITE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerHup][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr1(int s)
{
	printf("[HandlerUsr1]SIGUSR1\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerUsr1][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L - 1][C] != MUR)
	{
		DIR = HAUT;
		printf("[HandlerUsr1]DIR = HAUT\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerUsr1][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr2(int s)
{
	printf("[HandlerUsr2]SIGUSR2\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		perror("[HandlerUsr2][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L + 1][C] != MUR)
	{
		DIR = BAS;
		printf("[HandlerUsr2]DIR = BAS\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		perror("[HandlerUsr2][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}











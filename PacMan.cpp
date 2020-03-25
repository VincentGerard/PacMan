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
// #define DEBUG

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
int nbFantomesRouge = 0;
int nbFantomesVert = 0;
int nbFantomesMauve = 0;
int nbFantomesOrange = 0;
int nbVies = 3;
pthread_key_t key;
pthread_mutex_t mutexTab;
pthread_mutex_t mutexLCDIR;
pthread_mutex_t mutexNbPacGom;
pthread_mutex_t mutexDelai;
pthread_mutex_t mutexNiveauJeu;
pthread_mutex_t mutexScore;
pthread_mutex_t mutexNbFantomes;
pthread_mutex_t mutexNbVies;
pthread_mutex_t mutexMode;


//Pas besoin de mutex car on ne va pas modifer les valeurs
pthread_t tidPacMan;
pthread_t tidEvent;
pthread_t tidPacGom;
pthread_t tidScore;
pthread_t tidBonus;
pthread_t tidCompteurFantomes;
pthread_t tidThreadVie;
pthread_t tidTimeOut;

pthread_cond_t condNbPacGom;
pthread_cond_t condScore;
pthread_cond_t condNbFantomes;
pthread_cond_t condMode;

//Fonctions
void* threadPacMan(void*);
void* threadEvent(void*);
void* threadPacGom(void*);
void* threadScore(void*);
void* threadBonus(void*);
void* threadCompteurFantomes(void*);
void* threadFantomes(void*);
void* threadVies(void*);
void* threadTimeOut(void*);
void HandlerInt(int s);
void HandlerHup(int s);
void HandlerUsr1(int s);
void HandlerUsr2(int s);
void HandlerSigchld(int s);
void HandlerAlarm(int s);
void* TerminaisonFantomes(void* arg);
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
	printf("[Main: %d]Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
	if (OuvertureFenetreGraphique() < 0)
	{
		printf("Erreur de OuvrirGrilleSDL\n");
		fflush(stdout);
		exit(1);
	}

	DessineGrilleBase();

	//Debut de notre prog
	printf("[Main: %d][Debut]",getpid());

	//Masquer les signaux
	sigset_t mask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK,&mask,NULL);

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
	//sigalrm pas de handler
	struct sigaction A5;
	A5.sa_handler = HandlerAlarm;
	A5.sa_flags = 0;
	sigemptyset(&A5.sa_mask);
	sigaction(SIGALRM,&A5,NULL);
	//sigchild
	struct sigaction A6;
	A6.sa_handler = HandlerSigchld;
	A6.sa_flags = 0;
	sigemptyset(&A6.sa_mask);
	sigaction(SIGCHLD,&A6,NULL);

	//Init des mutex et variable de conditions
	if(pthread_mutex_init(&mutexTab,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexTab\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexLCDIR,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexLCDIR\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexNbPacGom,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexNbPacGom\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexDelai,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexDelai\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexNiveauJeu,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexNiveauJeu\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexScore,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexScore\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexNbFantomes,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexNbFantomes\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexNbVies,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexNbVies\n",getpid());
		exit(1);
	}
	if(pthread_mutex_init(&mutexMode,NULL))
	{
		printf("[Main: %d][Erreur]pthread_mutex_init sur mutexMode\n",getpid());
		exit(1);
	}
	if(pthread_cond_init(&condNbPacGom,NULL))
	{
		printf("[Main: %d][Erreur]pthread_cond_init sur condNbPacGom\n",getpid());
		exit(1);
	}
	if(pthread_cond_init(&condScore,NULL))
	{
		printf("[Main: %d][Erreur]pthread_cond_init sur condScore\n",getpid());
		exit(1);
	}
	if(pthread_cond_init(&condNbFantomes,NULL))
	{
		printf("[Main: %d][Erreur]pthread_cond_init sur condNbFantomes\n",getpid());
		exit(1);
	}
	if(pthread_cond_init(&condMode,NULL))
	{
		printf("[Main: %d][Erreur]pthread_cond_init sur condMode\n",getpid());
		exit(1);
	}


	if(pthread_key_create(&key,NULL))
	{
		printf("[Main: %d][Erreur]pthread_key_create\n",pthread_self());
		exit(1);
	}

	//Ouverture des threads
	if(pthread_create(&tidPacGom,NULL,threadPacGom,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadPacGom\n",pthread_self());
		exit(1);
	}
	if(pthread_create(&tidThreadVie,NULL,threadVies,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadVies\n",pthread_self());
		exit(1);
	}
	if(pthread_create(&tidEvent,NULL,threadEvent,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadEvent\n",pthread_self());
		exit(1);
	}
	if(pthread_create(&tidScore,NULL,threadScore,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadScore\n",pthread_self());
		exit(1);
	}
	if(pthread_create(&tidBonus,NULL,threadBonus,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadBonus\n",pthread_self());
		exit(1);
	}
	if(pthread_create(&tidCompteurFantomes,NULL,threadCompteurFantomes,NULL))
	{
		printf("[Main: %d][Erreur]pthread_create on threadFantomes\n",pthread_self());
		exit(1);
	}

	if(pthread_join(tidEvent,NULL))
	{
		printf("[Main: %d][Erreur]pthread_join on threadEvent\n",pthread_self());
		exit(1);
	}

	//Fin de notre prog
	//Destroy des mutex et variable de conditions
	if(pthread_mutex_destroy(&mutexTab))
	{
		printf("[Main][Erreur]pthread_mutex_destroy on mutexTab\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexLCDIR))
	{
		printf("[Main][Erreur]pthread_mutex_destroy on mutexLCDIR\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNbPacGom))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexNbPacGom\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexDelai))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexDelai\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNiveauJeu))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexNiveauJeu\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexScore))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexScore\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNbFantomes))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexNbPacGom\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexNbVies))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexNbVies\n");
		exit(1);
	}
	if(pthread_mutex_destroy(&mutexMode))
	{
		printf("[Main][Erreur]pthread_mutex_destroy sur mutexMode\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condNbPacGom))
	{
		printf("[Main][Erreur]pthread_cond_destroy sur condNbPacGom\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condScore))
	{
		printf("[Main][Erreur]pthread_cond_destroy sur condScore\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condNbFantomes))
	{
		printf("[Main][Erreur]pthread_cond_destroy sur condNbFantomes\n");
		exit(1);
	}
	if(pthread_cond_destroy(&condMode))
	{
		printf("[Main][Erreur]pthread_cond_destroy sur condMode\n");
		exit(1);
	}
	printf("[Main: %d][Fin]\n",pthread_self());

	// Fermeture de la fenetre
	printf("[Main %d] Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
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
	printf("[threadPacMan: %d][Debut]\n",pthread_self());

	int res = 0;
	int etat;
	bool spawnPacman = false;
	sigset_t maskPacMan;
	
	//Etablier le mode de fermeture du thread
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&etat);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,&etat);

	//Configurer le pacman
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexLCDIR\n",pthread_self());
		exit(1);
	}
	L = 15;
	C = 8;
	DIR = GAUCHE;
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexLCDIR\n",pthread_self());
		exit(1);
	}

	//Zone critique ne pas recevoir de signal durant l'attente et la zone graphique
	sigfillset(&maskPacMan);
	sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

	while(!spawnPacman)
	{
		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(tab[L][C] == VIDE)
		{
			MonDessinePacMan(L,C,DIR);
			spawnPacman++;
		}
		if(pthread_mutex_unlock(&mutexTab))
		{

			printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
		Attente(delai);
	}
	printf("[threadPacMan: %d][Info]PacMan a spawn L = %d C = %d\n",pthread_self(),L,C);

	sigfillset(&maskPacMan);
	sigdelset(&maskPacMan,SIGUSR1);
	sigdelset(&maskPacMan,SIGUSR2);
	sigdelset(&maskPacMan,SIGHUP);
	sigdelset(&maskPacMan,SIGINT);
	sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

	while(1)
	{
		int modifier = 0;
		int mangerPacGom = 0;
		int mangerSuperPacGom = 0;
		int mangerBonus = 0;
		int L2 = L;
		int C2 = C;
		int mort = 0;
		//Zone critique ne pas recevoir de signal durant l'attente
		sigfillset(&maskPacMan);
		sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

		Attente(delai);

		sigfillset(&maskPacMan);
		sigdelset(&maskPacMan,SIGUSR1);
		sigdelset(&maskPacMan,SIGUSR2);
		sigdelset(&maskPacMan,SIGHUP);
		sigdelset(&maskPacMan,SIGINT);
		sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_lock(&mutexMode))
		{
			printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexMode\n",pthread_self());
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
					else if(tab[L - 1][C] != VIDE && mode == 1)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome: %d\n",pthread_self(),tab[L - 1][C]);
						mort++;
					}
					else if(tab[L - 1][C] != VIDE && mode == 2)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome commestible: %d\n",pthread_self(),tab[L - 1][C]);
						pthread_kill(tab[L - 1][C],SIGCHLD);
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
					else if(tab[L + 1][C] != VIDE && mode == 1)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome: %d\n",pthread_self(),tab[L + 1][C]);
						mort++;
					}
					else if(tab[L + 1][C] != VIDE && mode == 2)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome commestible: %d\n",pthread_self(),tab[L + 1][C]);
						pthread_kill(tab[L + 1][C],SIGCHLD);
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
					else if(tab[L][C - 1] != VIDE && mode == 1)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome: %d\n",pthread_self(),tab[L][C - 1]);
						mort++;
					}
					else if(tab[L][C - 1] != VIDE && mode == 2)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome commestible: %d\n",pthread_self(),tab[L][C - 1]);
						pthread_kill(tab[L][C - 1],SIGCHLD);
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
					else if(tab[L][C + 1] != VIDE && mode == 1)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome: %d\n",pthread_self(),tab[L][C + 1]);
						mort++;
					}
					else if(tab[L][C + 1] != VIDE && mode == 2)
					{
						printf("[threadPacMan: %d][Info]Pacman a manger un fantome commestible: %d\n",pthread_self(),tab[L][C + 1]);
						pthread_kill(tab[L][C + 1],SIGCHLD);
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

		if(pthread_mutex_unlock(&mutexMode))
		{
			printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexMode\n",pthread_self());
			exit(1);
		}

		if(mort)
		{
			if(pthread_mutex_unlock(&mutexTab))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
				exit(1);
			}

			pthread_testcancel();
			
			if(pthread_mutex_lock(&mutexTab))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
				exit(1);
			}

			sigfillset(&maskPacMan);
			sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

			EffaceCarre(L,C);

			sigfillset(&maskPacMan);
			sigdelset(&maskPacMan,SIGUSR1);
			sigdelset(&maskPacMan,SIGUSR2);
			sigdelset(&maskPacMan,SIGHUP);
			sigdelset(&maskPacMan,SIGINT);
			sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

			tab[L][C] = VIDE;
			if(pthread_mutex_unlock(&mutexTab))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
				exit(1);
			}
			pthread_cancel(tidPacMan);
		}
		
		if(modifier && !mort)
		{
			sigfillset(&maskPacMan);
			sigprocmask(SIG_SETMASK,&maskPacMan,NULL);

			tab[L][C] = VIDE;
			EffaceCarre(L,C);
			MonDessinePacMan(L2,C2,DIR);

			sigfillset(&maskPacMan);
			sigdelset(&maskPacMan,SIGUSR1);
			sigdelset(&maskPacMan,SIGUSR2);
			sigdelset(&maskPacMan,SIGHUP);
			sigdelset(&maskPacMan,SIGINT);
			sigprocmask(SIG_SETMASK,&maskPacMan,NULL);
		}
		if(mangerPacGom && !mort)
		{
			if(pthread_mutex_lock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
				exit(1);
			}
			score++;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
				exit(1);
			}
			pthread_cond_signal(&condScore);
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			pthread_cond_signal(&condNbPacGom);
		}
		if(mangerSuperPacGom && !mort)
		{
			if(pthread_mutex_lock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
				exit(1);
			}
			score += 5;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
				exit(1);
			}
			pthread_cond_signal(&condScore);
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			pthread_cond_signal(&condNbPacGom);
			//Fantomes Comestibles
			if((res = alarm(0)) == 0)
			{
				if(pthread_mutex_lock(&mutexMode))
				{
					printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexMode\n",pthread_self());
					exit(1);
				}
				mode = 2;
				if(pthread_mutex_unlock(&mutexMode))
				{
					printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexMode\n",pthread_self());
					exit(1);
				}
				pthread_cond_signal(&condMode);
				
				if(pthread_create(&tidTimeOut,NULL,threadTimeOut,NULL))
				{
					printf("[threadPacMan: %d][Erreur]pthread_create on threadTimeOut\n",pthread_self());
					exit(1);
				}
			}
			else
			{
				//Si une alarm est deja en cours on l'arrete et recupere le nombre de secondes restantes
				pthread_kill(tidTimeOut,SIGQUIT);
				pthread_create(&tidTimeOut,NULL,threadTimeOut,&res);
			}
		}
		if(mangerBonus && !mort)
		{
			if(pthread_mutex_lock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
				exit(1);
			}
			score += 30;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
				exit(1);
			}
			pthread_cond_signal(&condScore);
		}

		if(pthread_mutex_unlock(&mutexTab))
		{
			printf("[threadPacMan: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
		pthread_testcancel();
	}
}

void* threadEvent(void*)
{
	printf("[threadEvent: %d][Debut]\n",pthread_self());

	//On bloque les signaux dans le threadEvent pour pas les envoyer a sois meme
	sigset_t maskEvent;
	sigfillset(&maskEvent);
	sigprocmask(SIG_SETMASK,&maskEvent,NULL);
	
	EVENT_GRILLE_SDL event;

	while(1)
	{
		event = ReadEvent();

		switch(event.type)
		{
			case CROIX:
				pthread_exit(NULL);
				Debug("[threadEvent: %d][Info]CROIX");
				break;
			case CLAVIER:
				switch(event.touche)
				{
					case KEY_UP:
						kill(getpid(),SIGUSR1);
						Debug("[threadEvent: %d][Info]SIGUSR1\n",pthread_self());
						break;
					case KEY_DOWN:
						kill(getpid(),SIGUSR2);
						Debug("[threadEvent: %d][Info]SIGUSR2\n",pthread_self());
						break;
					case KEY_LEFT:
						kill(getpid(),SIGINT);
						Debug("[threadEvent: %d][Info]SIGINT\n",pthread_self());
						break;
					case KEY_RIGHT:
						kill(getpid(),SIGHUP);
						Debug("[threadEvent: %d][Info]SIGHUP\n",pthread_self());
						break;
				}
				break;
		}
	}
}

void* threadPacGom(void*)
{
	printf("[threadPacGom: %d][Debut]\n",pthread_self());

	sigset_t maskPacgom;
	sigfillset(&maskPacgom);
	sigprocmask(SIG_SETMASK,&maskPacgom,NULL);

	while(1)
	{
		int valeur1 = 0;
		int valeur2 = 0;
		int valeur3 = 0;

		//Init PacGom
		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_lock(&mutexNbPacGom))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexNbPacGom\n",pthread_self());
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
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}	

		if(pthread_mutex_unlock(&mutexNbPacGom))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom\n",pthread_self());
			exit(1);
		}

		//Afficher le niveau de jeu
		if(pthread_mutex_lock(&mutexNiveauJeu))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexNiveauJeu\n",pthread_self());
			exit(1);
		}
		DessineChiffre(14,22,niveauJeu);
		if(pthread_mutex_unlock(&mutexNiveauJeu))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexNiveauJeu\n",pthread_self());
			exit(1);
		}

		//Attendre que le joueur gagne
		while(nbPacGom > 0)
		{
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			pthread_cond_wait(&condNbPacGom,&mutexNbPacGom);

			valeur1 = nbPacGom / 100;
			valeur2 = (nbPacGom - ((nbPacGom / 100) * 100)) / 10 ;
			valeur3 = (nbPacGom - ((nbPacGom / 100) * 100) - (((nbPacGom - ((nbPacGom / 100) * 100)) / 10) * 10)) / 1;

			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom",pthread_self());
				exit(1);
			}

			DessineChiffre(12,22,valeur1);
			DessineChiffre(12,23,valeur2);
			DessineChiffre(12,24,valeur3);
		}

		Debug("[threadPacGom: %d]Partie Gagnee",pthread_self());

		//Incrementer le niveai de jeu
		if(pthread_mutex_lock(&mutexNiveauJeu))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexNiveauJeu\n",pthread_self());
			exit(1);
		}
		niveauJeu++;
		if(pthread_mutex_unlock(&mutexNiveauJeu))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexNiveauJeu\n",pthread_self());
			exit(1);
		}
		//Augmenter la vitesse du jeu
		if(pthread_mutex_lock(&mutexDelai))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexDelai\n",pthread_self());
			exit(1);
		}
		delai /= 2;
		if(pthread_mutex_unlock(&mutexDelai))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexDelai\n",pthread_self());
			exit(1);
		}
		//On place un pacgom a la case d'origine du pacman
		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_lock(&mutexNbPacGom))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_lock on mutexNbPacGom\n",pthread_self());
			exit(1);
		}
		tab[15][8] = PACGOM;
		DessinePacGom(15,8);
		nbPacGom++;
		if(pthread_mutex_unlock(&mutexTab))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_unlock(&mutexNbPacGom))
		{
			printf("[threadPacGom: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom\n",pthread_self());
			exit(1);
		}
	}
}

void* threadScore(void*)
{
	printf("[threadScore: %d][Debut]\n",pthread_self());

	sigset_t maskScore;
	sigfillset(&maskScore);
	sigprocmask(SIG_SETMASK,&maskScore,NULL);

	while(1)
	{
		int valeur1 = 0;
		int valeur2 = 0;
		int valeur3 = 0;
		int valeur4 = 0;
		int reste = 0;

		if(pthread_mutex_lock(&mutexScore))
		{
			printf("[threadScore: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
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
			printf("[threadScore: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
			exit(1);
		}
	}
}

void* threadBonus(void*)
{
	printf("[threadBonus: %d][Debut]\n",pthread_self());

	sigset_t maskBonus;
	sigfillset(&maskBonus);
	sigprocmask(SIG_SETMASK,&maskBonus,NULL);

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
			printf("[threadBonus: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
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
			printf("[threadBonus: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}

		Attente(delaiBonus);

		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadBonus: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}

		if(tab[randX][randY] == BONUS)
		{
			tab[randX][randY] = VIDE;
			EffaceCarre(randX,randY);
		}

		if(pthread_mutex_unlock(&mutexTab))
		{
			printf("[threadBonus: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
	}
}

void* threadCompteurFantomes(void*)
{
	printf("[threadCompteurFantomes: %d][Debut]\n",pthread_self());

	sigset_t maskComptFantomes;
	sigfillset(&maskComptFantomes);
	sigprocmask(SIG_SETMASK,&maskComptFantomes,NULL);

	while(1)
	{
		if(pthread_mutex_lock(&mutexMode))
		{
			printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_lock on mutexMode",pthread_self());
			exit(1);
		}
		while(mode == 2)
		{
			pthread_cond_wait(&condMode,&mutexMode);
		}
		if(pthread_mutex_unlock(&mutexMode))
		{
			printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_unlock on mutexMode",pthread_self());
			exit(1);
		}

		if(pthread_mutex_lock(&mutexNbFantomes))
		{
			printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_lock on mutexNbFantomes",pthread_self());
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
			printf("[threadCompteurFantomes: %d][Erreur]pthread_mutex_unlock on mutexNbFantomes",pthread_self());
			exit(1);
		}
	}
}

void* threadFantomes(void* p)
{
	printf("[threadFantomes: %d][Debut]\n",pthread_self());

	sigset_t maskFantomes;
	sigfillset(&maskFantomes);
	sigdelset(&maskFantomes,SIGCHLD);
	sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

	srand(time(NULL));
	S_FANTOME* pFantome = (S_FANTOME*)p;
	char couleur[10] = {0};
	int dirFantome = HAUT;
	int min = 0;
	int max = 0;
	int spawnFantome = 0;
	int newDir = 0;
	int changeDir = 0;
	int tuerPacman = 0;
	int continuer = 0;
	int newTabVal = 0;
	int vect[4] = {0};
	int newDirOk = 0;
	int bloquer = 0;

	int nextC = 0;
	int nextL = 0;

	if(pFantome->couleur == ROUGE)
		strcat(couleur,"Rouge");
	else if(pFantome->couleur == VERT)
		strcat(couleur,"Vert");
	else if(pFantome->couleur == ORANGE)
		strcat(couleur,"Orange");
	else if(pFantome->couleur == MAUVE)
		strcat(couleur,"Mauve");

	pthread_setspecific(key,pFantome);
	pthread_cleanup_push(TerminaisonFantomes,NULL);

	do
	{
		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(tab[9][8] == VIDE && tab[8][8] == VIDE)
		{
			pFantome->cache = VIDE;
			if(pthread_mutex_lock(&mutexMode))
			{
				printf("[threadFantomes: %d][Erreur]pthread_mutex_lock on mutexMode\n",pthread_self());
				exit(1);
			}
			if(mode == 1)
				DessineFantome(pFantome->L,pFantome->C,pFantome->couleur,dirFantome);
			else
				DessineFantomeComestible(pFantome->L,pFantome->C);
			if(pthread_mutex_unlock(&mutexMode))
			{
				printf("[threadFantomes: %d][Erreur]pthread_mutex_unlock on mutexMode\n",pthread_self());
				exit(1);
			}
			tab[pFantome->L][pFantome->C] = pthread_self();
			spawnFantome = 1;
		}
		if(pthread_mutex_unlock(&mutexTab))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
	}while(!spawnFantome);

	sigfillset(&maskFantomes);
	sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

	if(pthread_mutex_lock(&mutexDelai))
	{
		printf("[threadFantomes: %d][Erreur]pthread_mutex_lock on mutexDelai\n",pthread_self());
		exit(1);
	}
	Attente(delai * (5 / 3));
	if(pthread_mutex_unlock(&mutexDelai))
	{
		printf("[threadFantomes: %d][Erreur]pthread_mutex_unlock on mutexDelai\n",pthread_self());
		exit(1);
	}

	sigfillset(&maskFantomes);
	sigdelset(&maskFantomes,SIGCHLD);
	sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

	while(nbVies > 0)
	{
		newDir = 0;
		tuerPacman = 0;
		continuer = 0;
		nextL = 0;
		nextC = 0;
		vect[0] = 0;
		vect[1] = 0;
		vect[2] = 0;
		vect[3] = 0;
		max = 0;
		changeDir = 0;
		newDirOk = 0;
		bloquer = 0;

		if(pthread_mutex_lock(&mutexTab))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_lock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_lock(&mutexMode))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_lock on mutexMode\n",pthread_self());
			exit(1);
		}

		sigfillset(&maskFantomes);
		sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

		//Restitution du Cache
		switch(pFantome->cache)
		{
			case VIDE:
				EffaceCarre(pFantome->L,pFantome->C);
				newTabVal = VIDE;
				break;
			case PACGOM:
				DessinePacGom(pFantome->L,pFantome->C);
				newTabVal = PACGOM;
				break;
			case SUPERPACGOM:
				DessineSuperPacGom(pFantome->L,pFantome->C);
				newTabVal = SUPERPACGOM;
				break;
			case BONUS:
				DessineBonus(pFantome->L,pFantome->C);
				newTabVal = BONUS;
				break;
			default:
				printf("[threadFantomes: %d][Erreur]Switch(cache): %d\n",pthread_self(),pFantome->cache);
				exit(1);
				break;
		}

		sigfillset(&maskFantomes);
		sigdelset(&maskFantomes,SIGCHLD);
		sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

		tab[pFantome->L][pFantome->C] = newTabVal;
		
		//Mouvement
		switch(dirFantome)
		{
			case HAUT:
				nextL = pFantome->L - 1;
				nextC = pFantome->C;
				break;
			case BAS:
				nextL = pFantome->L + 1;
				nextC = pFantome->C;
				break;
			case GAUCHE:
				nextL = pFantome->L;
				nextC = pFantome->C - 1;
				break;
			case DROITE:
				nextL = pFantome->L;
				nextC = pFantome->C + 1;
				break;
			default:
				printf("[threadFantomes: %d][Erreur]Switch(dirFantome): %d\n",pthread_self(),dirFantome);
				exit(1);
				break;
		}

		if(mode == 1 && tab[nextL][nextC] == PACMAN)
			tuerPacman++;
		else if(mode == 2 && tab[nextL][nextC] == PACMAN)
			changeDir++;
		else if(tab[nextL][nextC] == VIDE || tab[nextL][nextC] == PACGOM || tab[nextL][nextC] == SUPERPACGOM || tab[nextL][nextC] == BONUS)
			continuer++;
		else
			changeDir++;

		if(tuerPacman)
		{
			tab[nextL][nextC] = VIDE;
			pFantome->L = nextL;
			pFantome->C = nextC;
			if(pthread_mutex_unlock(&mutexTab))
			{
				printf("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
				exit(1);
			}
			if(pthread_mutex_unlock(&mutexMode))
			{
				printf("[threadFantomes][Erreur]pthread_mutex_unlock on mutexMode\n");
				exit(1);
			}

			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

			EffaceCarre(nextL,nextC);

			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);
			pthread_cancel(tidPacMan);
		}

		if(continuer)
		{
			pFantome->L = nextL;
			pFantome->C = nextC;
		}

		if(changeDir)
		{
			//Recupere le nombre de cases vides
			if(tab[pFantome->L - 1][pFantome->C] == VIDE || tab[pFantome->L - 1][pFantome->C] == PACGOM || tab[pFantome->L - 1][pFantome->C] == SUPERPACGOM || tab[pFantome->L - 1][pFantome->C] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
					{
						vect[i] = HAUT;
						i = 4;
					}
			}
			if(tab[pFantome->L + 1][pFantome->C] == VIDE || tab[pFantome->L + 1][pFantome->C] == PACGOM || tab[pFantome->L + 1][pFantome->C] == SUPERPACGOM || tab[pFantome->L + 1][pFantome->C] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
					{
						vect[i] = BAS;
						i = 4;
					}
			}
			if(tab[pFantome->L][pFantome->C - 1] == VIDE || tab[pFantome->L][pFantome->C - 1] == PACGOM || tab[pFantome->L][pFantome->C - 1] == SUPERPACGOM || tab[pFantome->L][pFantome->C - 1] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
					{
						vect[i] = GAUCHE;
						i = 4;
					}
			}
			if(tab[pFantome->L][pFantome->C + 1] == VIDE || tab[pFantome->L][pFantome->C + 1] == PACGOM || tab[pFantome->L][pFantome->C + 1] == SUPERPACGOM || tab[pFantome->L][pFantome->C + 1] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
					{
						vect[i] = DROITE;
						i = 4;
					}
			}

			for(int i = 0; i < 4; i++)
			{
				if(vect[i] != 0)
				{
					max++;
				}
			}
			if(vect[0] != 0)
			{
				max--;
			}


			if(max == 0 && vect[0] == 0)
			{
				bloquer++;
			}

			while(!newDirOk && !bloquer)
			{
				newDir = min + rand() % (max - min + 1);
				newDir = vect[newDir];
				for(int i = 0; i < 4; i++)
					if(vect[i] == newDir)
						newDirOk++;
			}

			if(!bloquer)
			{
				if(newDir == HAUT)
				pFantome->L--;
				else if(newDir == BAS)
					pFantome->L++;
				else if(newDir == GAUCHE)
					pFantome->C--;
				else if(newDir == DROITE)
					pFantome->C++;
				
				dirFantome = newDir;
			}
		}

		//Recuperer le cache
		pFantome->cache = tab[pFantome->L][pFantome->C];

		//Afficher le fantome
		if(mode == 1)
		{
			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);
			
			DessineFantome(pFantome->L,pFantome->C,pFantome->couleur,dirFantome);
			
			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);
		}
		else if(mode == 2)
		{
			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

			DessineFantomeComestible(pFantome->L,pFantome->C);

			sigfillset(&maskFantomes);
			sigdelset(&maskFantomes,SIGCHLD);
			sigprocmask(SIG_SETMASK,&maskFantomes,NULL);
		}
		
		tab[pFantome->L][pFantome->C] = pthread_self();
	
		if(pthread_mutex_unlock(&mutexTab))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_unlock on mutexTab\n",pthread_self());
			exit(1);
		}
		if(pthread_mutex_unlock(&mutexMode))
		{
			printf("[threadFantomes: %d][Erreur]pthread_mutex_unlock on mutexMode\n",pthread_self());
			exit(1);
		}

		sigfillset(&maskFantomes);
		sigprocmask(SIG_SETMASK,&maskFantomes,NULL);

		Attente(delai * (5 / 3));

		sigfillset(&maskFantomes);
		sigdelset(&maskFantomes,SIGCHLD);
		sigprocmask(SIG_SETMASK,&maskFantomes,NULL);
	}
	pthread_cleanup_pop(0);
	pthread_kill(tidCompteurFantomes,SIGQUIT);
	pthread_exit(NULL);
}

void* threadVies(void*)
{
	printf("[threadVies: %d][Debut]\n",pthread_self());

	sigset_t maskVies;
	sigfillset(&maskVies);
	sigprocmask(SIG_SETMASK,&maskVies,NULL);

	DessineChiffre(18,22,nbVies);

	while(nbVies > 0)
	{
		//Thread PacMan
		pthread_create(&tidPacMan,NULL,threadPacMan,NULL);
		//printf("[threadVies]pthread_create\n");
		pthread_join(tidPacMan,NULL);
		if(pthread_mutex_lock(&mutexNbVies))
		{
			printf("[threadVies: %d][Erreur]pthread_mutex_lock on mutexNbVies\n",pthread_self());
			exit(1);
		}
		nbVies--;
		DessineChiffre(18,22,nbVies);
		if(pthread_mutex_unlock(&mutexNbVies))
		{
			printf("[threadVies: %d][Erreur]pthread_mutex_unlock on mutexNbVies\n",pthread_self());
			exit(1);
		}
	}

	//Ce delais permet d'etre sur que les fantomes sont bien figer
	//car sinon ils peuvent panger le game over

	Attente(delai);

	DessineGameOver(9,4);

	pthread_exit(NULL);
}

void* threadTimeOut(void* param)
{
	printf("[threadTimeOut: %d][Debut]\n",pthread_self());
	
	sigset_t maskTimeout;
	sigfillset(&maskTimeout);
	sigdelset(&maskTimeout,SIGALRM);
	sigprocmask(SIG_SETMASK,&maskTimeout,NULL);

	int min = 8;
	int max = 15;
	int time = 0;

	time = min + rand() % (max - min + 1);

	if(param)
	{
		time += *(int*)param;
	}

	alarm(time);
	pause();
	pthread_exit(NULL);
}

void HandlerInt(int s)
{
	Debug("[HandlerInt]SIGINT\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[HandlerInt][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C - 1] != MUR)
	{
		DIR = GAUCHE;
		Debug("[HandlerInt]DIR = GAUCHE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[HandlerInt][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerHup(int s)
{	
	Debug("[HandlerHup]SIGHUP\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[HandlerHup][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L][C + 1] != MUR)
	{
		DIR = DROITE;
		Debug("[HandlerHup]DIR = DROITE\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[HandlerHup][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr1(int s)
{
	Debug("[HandlerUsr1]SIGUSR1\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[HandlerUsr1][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L - 1][C] != MUR)
	{
		DIR = HAUT;
		Debug("[HandlerUsr1]DIR = HAUT\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[HandlerUsr1][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerUsr2(int s)
{
	Debug("[HandlerUsr2]SIGUSR2\n");
	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[HandlerUsr2][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	if(tab[L + 1][C] != MUR)
	{
		DIR = BAS;
		Debug("[HandlerUsr2]DIR = BAS\n");
	}
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[HandlerUsr2][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
		exit(1);
	}
}

void HandlerSigchld(int s)
{
	// printf("[HandlerSigchld]\n");
	pthread_exit(NULL);
}

void HandlerAlarm(int s)
{
	if(pthread_mutex_lock(&mutexMode))
	{
		printf("[threadTimeOut][Erreur]pthread_mutex_lock on mutexMode\n");
		exit(1);
	}
	mode = 1;
	if(pthread_mutex_unlock(&mutexMode))
	{
		printf("[threadTimeOut][Erreur]pthread_mutex_unlock on mutexMode\n");
		exit(1);
	}
	pthread_cond_signal(&condMode);

	pthread_exit(0);
}

void* TerminaisonFantomes(void* arg)
{
	//Incrementer le score de 50
	if(pthread_mutex_lock(&mutexScore))
	{
		printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
		exit(1);
	}
	score += 50;
	MAJScore = true;
	if(pthread_mutex_unlock(&mutexScore))
	{
		printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
		exit(1);
	}
	//Incrementer le socre avec le contenu du cach
	S_FANTOME* pFantome = NULL;
	pFantome = (S_FANTOME*)pthread_getspecific(key);
	if(pFantome == NULL)
	{
		printf("[TerminaisonFantomes: %d]pFantome == NULL\n",pthread_self());
		exit(1);
	}

	switch(pFantome->cache)
	{
		case VIDE:
			break;
		case PACGOM:
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			break;
		case SUPERPACGOM:
			if(pthread_mutex_lock(&mutexNbPacGom))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			nbPacGom--;
			if(pthread_mutex_unlock(&mutexNbPacGom))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexNbPacGom",pthread_self());
				exit(1);
			}
			break;
		case BONUS:
			if(pthread_mutex_lock(&mutexScore))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexScore",pthread_self());
				exit(1);
			}
			score += 30;
			MAJScore = true;
			if(pthread_mutex_unlock(&mutexScore))
			{
				printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexScore",pthread_self());
				exit(1);
			}
			break;
		default:
			printf("[TerminaisonFantomes: %d]switch(pFantome->cache) default\n",pthread_self());
			break;
	}
	//Reveiller le threadScore
	pthread_cond_signal(&condScore);
	//Reveille le threadNbPacGom
	pthread_cond_signal(&condNbPacGom);

	//Decrementer le bon fantome
	if(pthread_mutex_lock(&mutexNbFantomes))
	{
		printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_lock on mutexNbFantomes",pthread_self());
		exit(1);
	}
	// printf("[TerminaisonFantomes]couleur = %d\n",pFantome->couleur);
	if(pFantome->couleur == ROUGE)
		nbFantomesRouge--;
	else if(pFantome->couleur == VERT)
		nbFantomesVert--;
	else if(pFantome->couleur == ORANGE)
		nbFantomesOrange--;
	else if(pFantome->couleur == MAUVE)
		nbFantomesMauve--;
	if(pthread_mutex_unlock(&mutexNbFantomes))
	{
		printf("[TerminaisonFantomes: %d][Erreur]pthread_mutex_unlock on mutexNbFantomes",pthread_self());
		exit(1);
	}
	pthread_cond_signal(&condNbFantomes);
	pthread_exit(NULL);
}

void MonDessinePacMan(int l,int c,int dir)
{
	
	DessinePacMan(l,c,dir);
	tab[l][c] = PACMAN;

	if(pthread_mutex_lock(&mutexLCDIR))
	{
		printf("[MonDessinePacMan][Erreur]pthread_mutex_lock on mutexLCDIR\n");
		exit(1);
	}
	L = l;
	C = c;
	if(pthread_mutex_unlock(&mutexLCDIR))
	{
		printf("[MonDessinePacMan][Erreur]pthread_mutex_unlock on mutexLCDIR\n");
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
		printf("[AfficheTab][Erreur]pthread_mutex_lock on mutexTab\n");
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
		printf("[AfficheTab][Erreur]pthread_mutex_unlock on mutexTab\n");
		exit(1);
	}
}



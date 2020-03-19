void* threadFantomes(void* p)
{
	printf("[threadFantomes][Debut]Tid: %d\n",pthread_self());

	sigset_t maskFantomes;
	sigfillset(&maskFantomes);
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
	int fuirePacman = 0;
	int tuerPacman = 0;
	int continuer = 0;
	int newTabVal = 0;
	int vect[4] = {0};

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
	printf("[threadFantomes]L: %d C: %d Couleur: %s Cache: %d\n",pFantome->L,pFantome->C,couleur,pFantome->cache);

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
			pFantome->cache = VIDE;
			DessineFantome(pFantome->L,pFantome->C,pFantome->couleur,dirFantome);
			tab[pFantome->L][pFantome->C] = pthread_self();
			spawnFantome = 1;
			printf("[threadFantomes]Fantome place a L: %d C: %d\n",pFantome->L,pFantome->C);
		}
		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
	}while(!spawnFantome);

	while(nbVies > 0)
	{
		newDir = 0;
		tuerPacman = 0;
		fuirePacman = 0;
		continuer = 0;
		nextL = 0;
		nextC = 0;
		vect[0] = 0;
		vect[1] = 0;
		vect[2] = 0;
		vect[3] = 0;
		max = 0;

		if(pthread_mutex_lock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_lock on mutexTab\n");
			exit(1);
		}
		printf("[threadFantomes: %d]Bouble L = %2d C = %2d\n",pthread_self(),pFantome->L,pFantome->C);

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
				printf("[threadFantomes][Erreur]Switch cache: %d\n",pFantome->cache);
				break;
		}
		tab[pFantome->L][pFantome->C] = newTabVal;
		
		//Mouvement
		switch(dirFantome)
		{
			case HAUT:
				nextL = pFantome->L - 1;
				nextC = pFantome->C;

				if(mode == 1 && tab[nextL][nextC] == PACMAN)
					tuerPacman++;
				else if(mode == 2 && tab[nextL][nextC] == PACMAN)
					changeDir++;
				else if(tab[nextL][nextC] == VIDE || tab[nextL][nextC] == PACGOM || tab[nextL][nextC] == SUPERPACGOM || tab[nextL][nextC] == BONUS)
					continuer++;
				else
					changeDir++;
				break;
			case BAS:
				nextL = pFantome->L + 1;
				nextC = pFantome->C;

				if(mode == 1 && tab[nextL][nextC] == PACMAN)
					tuerPacman++;
				else if(mode == 2 && tab[nextL][nextC] == PACMAN)
					changeDir++;
				else if(tab[nextL][nextC] == VIDE || tab[nextL][nextC] == PACGOM || tab[nextL][nextC] == SUPERPACGOM || tab[nextL][nextC] == BONUS)
					continuer++;
				else
					changeDir++;
				break;
			case GAUCHE:
				nextL = pFantome->L;
				nextC = pFantome->C - 1;

				if(mode == 1 && tab[nextL][nextC] == PACMAN)
					tuerPacman++;
				else if(mode == 2 && tab[nextL][nextC] == PACMAN)
					changeDir++;
				else if(tab[nextL][nextC] == VIDE || tab[nextL][nextC] == PACGOM || tab[nextL][nextC] == SUPERPACGOM || tab[nextL][nextC] == BONUS)
					continuer++;
				else
					changeDir++;
				break;
			case DROITE:
				nextL = pFantome->L + 1;
				nextC = pFantome->C;

				if(mode == 1 && tab[nextL][nextC] == PACMAN)
					tuerPacman++;
				else if(mode == 2 && tab[nextL][nextC] == PACMAN)
					changeDir++;
				else if(tab[nextL][nextC] == VIDE || tab[nextL][nextC] == PACGOM || tab[nextL][nextC] == SUPERPACGOM || tab[nextL][nextC] == BONUS)
					continuer++;
				else
					changeDir++;
				break;
			default:
				printf("[threadFantomes][Erreur]Switch(dirFantome)\n");
				break;
		}

		if(tuerPacman)
		{
			printf("[threadFantomes]tuerPacman\n");
			pthread_cancel(tidPacMan);
			EffaceCarre(nextL,nextC);
			tab[nextL][nextC] = VIDE;
			//Le cache est forcement vide car le fantome a manger le pacgom/superPacgom/Bonus
			pFantome->L = nextL;
			pFantome->C = nextC;
		}

		if(continuer)
		{
			printf("[threadFantomes]continuer\n");
			pFantome->L = nextL;
			pFantome->C = nextC;
		}

		if(changeDir)
		{
			printf("[threadFantomes]changeDir\n");
			//Recupere le nombre de cases vides
			if(tab[pFantome->L - 1][pFantome->C] == VIDE || tab[pFantome->L - 1][pFantome->C] == PACGOM || tab[pFantome->L - 1][pFantome->C] == SUPERPACGOM || tab[pFantome->L - 1][pFantome->C] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = HAUT;
			}
			if(tab[pFantome->L + 1][pFantome->C] == VIDE || tab[pFantome->L + 1][pFantome->C] == PACGOM || tab[pFantome->L + 1][pFantome->C] == SUPERPACGOM || tab[pFantome->L + 1][pFantome->C] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = BAS;
			}
			if(tab[pFantome->L][pFantome->C - 1] == VIDE || tab[pFantome->L][pFantome->C - 1] == PACGOM || tab[pFantome->L][pFantome->C - 1] == SUPERPACGOM || tab[pFantome->L][pFantome->C - 1] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = GAUCHE;
			}
			if(tab[pFantome->L][pFantome->C + 1] == VIDE || tab[pFantome->L][pFantome->C + 1] == PACGOM || tab[pFantome->L][pFantome->C + 1] == SUPERPACGOM || tab[pFantome->L][pFantome->C + 1] == BONUS)
			{
				for(int i = 0; i < 4; i++)
					if(vect[i] == 0)
						vect[i] = DROITE;
			}

			for(int i = 0; i < 4; i++)
			{
				if(vect[i] != 0)
				{
					max++;
				}
			}
			max--;

			newDir = min + rand() % (max - min + 1);
			newDir += 500000;

			// printf("[threadFantomes: %d]dirFantome = %d newDir = %d\n",pthread_self(),dirFantome,newDir);
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

		//Choper le cache
		pFantome->cache = tab[pFantome->L][pFantome->C];


		//Afficher le fantome
		if(mode == 1)
		{
			//Dessin normal
			DessineFantome(pFantome->L,pFantome->C,pFantome->couleur,dirFantome);
		}
		else if(mode == 2)
		{
			//Dessin bleu
			DessineFantomeComestible(pFantome->L,pFantome->C);
		}

		printf("[threadFantomes: %d]L: %2d C: %2d nL: %2d nC: %d\n",pthread_self(),pFantome->L,pFantome->C,nextL,nextC);
		printf("[threadFantomes: %d]Bouble2 L = %2d C = %2d\n",pthread_self(),pFantome->L,pFantome->C);
		
		tab[pFantome->L][pFantome->C] = pthread_self();
	
		if(pthread_mutex_unlock(&mutexTab))
		{
			perror("[threadFantomes][Erreur]pthread_mutex_unlock on mutexTab\n");
			exit(1);
		}
		Attente(delai * (5 / 3));
	}
}
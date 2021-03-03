/* Disciplina: 	Sisteme de Operare
 * Student: 	Stoian Iulia Tia
 * Grupa: 	30221
 * Semigrupa:	2
 *
 * -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * 	Reactie: 		BaCl + H2SO4 -> HCl + BaSO4 
 *
 * Conditia de terminare: 	Daca s-au format N molecule de HCl.
 * 
 * Se formeaza thread-uri individuale de tipul fiecarui atom implicat in reactie, iar mai apoi thread-uri individuale pentru moleculele de BaCl2,
 * respectiv H2SO4.
 * -------------------------------------------------------------------------------------------------------------------------------------------------- *
 */

#include <stdio.h>	// input, output
#include <stdlib.h>	// diverse functii & macrouri
#include <pthread.h>	// thread-uri
#include <semaphore.h>	// semafoare
#include <unistd.h>	// sleep
#include <string.h>	// string manipulation

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se declara variabilele globale
 */
int		isOrdered;		// "flag" care ia valoarea 0 pt generarea aleatoare a atomilor, iar valoarea 1 pt generarea odonata a atomilor
int		N;			// nr de molecule deHCl are trebuie formate
int 		necessaryMolecules;	// nr minim de molecule (BaCl, respectiv H2SO4) necesare pt formarea celor N molecule de HCl			
	
int 		waitingBarium,		// nr de atomi de tipul specificat, care asteapta sa formeze o molecula
		waitingChlorine,			
		waitingSodium,
		waitingHydrogen,
		waitingOxygen;	

int		waitingBaCl2,		// nr de molecule de tipul specificat, care asteapta sa reactioneze
		waitingH2SO4;

int		bariumCnt,		// contoare de atomi creati de tipul specificat
		chlorineCnt,
		sodiumCnt,
		hydrogenCnt,
		oxygenCnt,
		BaCl2Cnt,		// contoare pentru moleculele de tipul specificat
		H2SO4Cnt,
		reactionCnt;		// contor de reactii care au avut loc

int 		atom;			// index pentru array-urile de thread-uri de atomi
					// in acelasi timp, este si reprezentant al momentului de timp

long int 	*bariumID, 		// array-uri alocate dinamic pt a retine thread id al fiecarui thread de atom, respectiv molecula, de
		*chlorineID,		// tipul specificat in identificator
		*hydrogenID,
		*sodiumID,		// thread id-ul celui de al nume_atomCnt creat este nume_atomID[nume_atomCnt]
		*oxygenID,
		*BaCl2ID,		// thread id-ul celei de a nume_moleculaCnt creata este nume_moleculaID[nume_moleculaCnt]
		*H2SO4ID;

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se declara mutex-urile, semafoarele, barierele si thread-urile lobale
 */
pthread_mutex_t 	mutexBaCl2, 		// pentru a nu se accesa sectiunea critica de catre thread-urile "barium" si "chlorine" in acelasi timp
			mutexH2SO4, 		// analog pt "sodium", "hydrogen" si "oxygen"
		 	mutexReaction;		// analog pt moleculele de BaCl2 si H2SO4 care reactioneaza

sem_t			bariumQueue,		// valorile negative indica numarul de atomi de	bariu care asteapta sa formeze o molecula
			chlorineQueue,		// analog pt clor
			sodiumQueue,		// analog pt sodiu
			oxygenQueue,		// analog pt oxigen
			hydrogenQueue,		// analog pt hidrogen
			BaCl2Queue,		// analog pt nr de molecule de BaCl2 care asteapta sa intre in reactie
			H2SO4Queue;		// analog pt nr de molecule de H2SO4 care asteapta sa intre in reactie
					
pthread_barrier_t	barrierBaCl2,		// rendezvous pentru 1 atom de bariu si 2 atomi de clor => init cu valoarea 3
			barrierH2SO4,		// rendezvous pentru 1 atom de sodiu, 2 atomi de hidrogen si 4 de oxigen => init cu valoarea 7
			barrierReaction;	// rendezvous pentru 1 molecula de BaCl2 si 1 molecula de H2SO4 -> init cu valoarea  2

pthread_t		moleculeBaCl2Thr,	// array de thread-uri pentru moleculele care se vor forma
			moleculeH2SO4Thr;

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se definesc si implementeaza functiile de afisare a evenimentelor
 */

void displayCreationInfo(char *atomName, int atomID) {						// mesaj afisat la crearea unui atom
	printf("\nCREARE	");
	printf("	Atom type: %s", atomName);
	printf("	Atom ID: %d", atomID);
	printf("	Moment in time: %d", atom);
}

void displayFormationInfo(char *moleculeName,  int moleculeID, char* atomName, int atomID) {	// mesaj afisat la intrarea unui atom in formarea unei
	printf("\nFORMARE	");								// molecule
	printf("	Molecule type: %s", moleculeName);
	printf("	Molecule ID: %d", moleculeID);
	printf("	Atom type: %s", atomName);
	printf("	Atom ID: %d", atomID);
	printf("	Moment in time: %d", atom);
}

void displayTerminatedAtomInfo(char *atomName, int atomID) {					// mesaj afisat la distrugerea unui atom 
	printf("\nTERMINARE_ATOM");								// (s-a folosit la formarea unei molecule)
	printf("	Atom type: %s", atomName);
	printf("	Atom ID: %d", atomID);
	printf("	Moment in time: %d", atom);
}

void displayReactionInfo(char* moleculeName, int moleculeID) {					// mesaj afisat la intrarea in reactie a unei molecule
	printf("\nREACTIE	");
	printf("	Reaction type: BaCl + H2SO4 -> 2HCl + BaSO4");
	printf("	Reaction ID: %d", reactionCnt);
	printf("	Molecule type: %s", moleculeName);
	printf("	Molecule ID: %d", moleculeID);
	printf("	Moment in time: %d", atom);
}

void displayTerminatedMoleculeInfo(char *moleculeName, int moleculeID) {			// mesaj afisat la distrugerea unei molecule
	printf("\nTERMINARE_MOLECULA");								// (a intrat in reactie)
	printf("	Molecule type: %s", moleculeName);
	printf("	Molecule ID: %d", moleculeID);
	printf("	Moment in time: %d", atom);
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se definesc si implementeaza functiile de gasire a thread id-ului unui atom sau al unei molecule
 * index - al catelea atom/(molecula) creat/(creata) a fost
 * la pozitia i se afla thread id-ul celui de al i-lea atom/(molecula) creat/(creata)
 */

int findBariumID(long int tid) {
	for (int i = 1; i <= bariumCnt; ++i) {
		if (bariumID[i] == tid) { return i; }
	}
	return -1;
}

int findChlorineID(long int tid) {
	for (int i = 1; i <= chlorineCnt; ++i) {
		if (chlorineID[i] == tid) { return i; }
	}
	return -1;
}

int findOxygenID(long int tid) {
	for (int i = 1; i <= oxygenCnt; ++i) {
		if (oxygenID[i] == tid) { return i; }
	}
	return -1;
}
int findHydrogenID(long int tid) {
	for (int i = 1; i <= hydrogenCnt; ++i) {
		if (hydrogenID[i] == tid) { return i; }
	}
	return -1;
}
int findSodiumID(long int tid) {
	for (int i = 1; i <= sodiumCnt; ++i) {
		if (sodiumID[i] == tid) { return i; }
	}
	return -1;
}

int findBaCl2ID(long int tid) {
	for (int i = 1; i <= BaCl2Cnt; ++i) {
		if (BaCl2ID[i] == tid) { return i; }
	}
	return -1;
}

int findH2SO4ID(long int tid) {
	for (int i = 1; i <= H2SO4Cnt; ++i) {
		if (H2SO4ID[i] == tid) { return i; }
	}
	return -1;
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se implementeaza functia care calculeaza cate molecule este necesar sa se formeze pentru a intra in reactie si a crea N molecule de HCl; aceasta
 * valoare va fi stocata in variabila globala necessaryMolecules
 *
 * Explicatie:	Reactia este BaCl2 + H2SO4 -> 2HCl + BaSO4, de unde rezulta ca la 1 molecula de BaCl2 si
 * 		1 molecula de H2SO4, se formeaza 2 molecule de HCl.
 *
 * In consecinta, conditia de oprire va fi satisfacuta in momentul in care avem necessaryMolecules molecule de
 * BaCl2 si necessaryMolecules molecule de H2SO4.
 *
 * Observatie:	Daca N este par, 	atunci necessaryMolecules va fi (N / 2)
 * 		Daca N este impar,	atunci necessaryMolecules va fi (N + 1) / 2
 */
int computeNecessaryMolecules() { return (N % 2) ? ((N + 1) / 2) : (N / 2); }

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se implementeaza functia care verifica daca se satisface conditia de terminare a programului
 */
int done() {
	if (reactionCnt >= necessaryMolecules) { 						// daca au avut loc necessaryMolecules reactii,
		printf("\n************* All %d HCl molecules were formed! *************\n", N);	// atunci inseamna ca avem cele N molecule de HCl
		printf("			(Moment in time: %d)\n", atom);
		return 1;
	}
	return 0;
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * se implementeaza functia care semnaleaza faptul ca a avut loc o reactie
 */
int reaction() { printf("\n----------New reaction took place----------Total: %d", reactionCnt);	}

/*	De aici in jos sunt implementate in ordine urmatoarele functii:
 *
 *		void* sulfuricAcid(void* arg)
 *		void bondSulfuricAcid()
 *		void* sodium(void *arg)
 *		void* hydrogen(void *arg)
 *		void* oxygen(void *arg)
 *		
 *		void* bariumChloride(void* arg)
 *		void bondBariumChloride()
 *		void* barium(void *arg)
 *		void* chlorine(void *arg)
 */

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului moleculei de acid sulfuric
 */
void* sulfuricAcid(void* arg) {
	++H2SO4Cnt;						// creste nr de molecule de sodiu H2SO4
	printf("\n---------- H2SO4 molecule formed ---------- Total: %d", H2SO4Cnt);

	H2SO4ID[H2SO4Cnt] = pthread_self();			// se stocheaza id-ul thread-ului curent in array-ul de thread-uri
	
	pthread_mutex_lock(&mutexReaction); 			// **************** inceputul zonei critice

	++waitingH2SO4;						// a aparut o noua molecula de H2SO4 in asteptare
	if (waitingBaCl2 >= 1 && waitingH2SO4 >= 1) {		// se verifica daca se poate intra in reactie
		++reactionCnt; 
		--waitingBaCl2;					// se actualizeaza contorul de molecule de BaCl2 in asteptare
		sem_post(&BaCl2Queue);				// se "elibereaza" 1 molecula de BaCl2 din coada

		--waitingH2SO4;					// se actualizeaza contorul de molecule de H2SO4 in asteptare
		sem_post(&H2SO4Queue);				// se "elibereaza" 1 molecula de H2SO4 din coada
	}
	else {
		pthread_mutex_unlock(&mutexReaction);		// **************** finalul zonei critice
	}

	sem_wait(&H2SO4Queue);					// molecula de H2SO4 in asteptare sa intre in reactie
	
	displayReactionInfo("H2SO4",
	findH2SO4ID(pthread_self()));				// afiseaza informatiile REACTIE

	pthread_barrier_wait(&barrierReaction);			// se ajunge la bariera

	displayTerminatedMoleculeInfo("H2SO4",
	findH2SO4ID(pthread_self()));				// afiseaza informatiile TERMINARE_MOLECULA
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * imlementarea functiei care face legatura intre atomi
 * se creeaza thread-uri pentru fiecare molecula formata si se verifica daca s-au format cu succes
 */
void bondSulfuricAcid() {
	if (0 != pthread_create(&moleculeH2SO4Thr, NULL, sulfuricAcid, NULL)) {
		printf("Could not create the barium thread!");
		exit(1);
	}
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului de sodiu
 */
void* sodium(void *arg) {
	++sodiumCnt;						// creste nr de atomi de sodiu creati

	displayCreationInfo("sodium", sodiumCnt);		// afiseaza informatiile CREARE

	sodiumID[sodiumCnt] = pthread_self();			// se stocheaza id-ul thread-ului curent in array-ul de thread-uri
	
	pthread_mutex_lock(&mutexH2SO4); 			// **************** inceputul zonei critice
	
	++waitingSodium;					// a aparut un nou atom de sodiu in asteptare
	if (waitingHydrogen >= 2 && waitingOxygen >= 4) {	// se verifica daca se poate forma o molecula de H2SO4
		sem_post(&hydrogenQueue);	
		sem_post(&hydrogenQueue);			// se "elibereaza" 2 atomi de hidrogen din coada
		waitingHydrogen -= 2;				// se actualizeaza contorul atomilor de hidrogen in asteptare
		
		sem_post(&sodiumQueue);				// se "elibereaza" un atom de sodiu din coada
		--waitingSodium;				// se actualizeaza contorul atomilor de sodiu in asteptare

		sem_post(&oxygenQueue);				// se "elibereaza" 4 atomi de oxigen din coada
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		waitingOxygen -= 4;				// se actualizeaza contorul atomilor de oxigen in asteptare
	}
	else {
		pthread_mutex_unlock(&mutexH2SO4);		// **************** finalul 1 al zonei critice
	}

	sem_wait(&sodiumQueue);					// atom de sodiu in asteptare sa formeze o molecula

	displayFormationInfo("H2SO4", H2SO4Cnt,
	"sodium", findSodiumID(pthread_self()));		// afiseaza informatiile FORMARE
		
	bondSulfuricAcid();					// se creeaza legatura cu ceilalti 6 atomi

	pthread_barrier_wait(&barrierH2SO4); 			// se ajunge la bariera

	displayTerminatedAtomInfo("sodium",
	findSodiumID(pthread_self()));				// afiseaza informatiile TERMINARE_ATOM	

	pthread_mutex_unlock(&mutexH2SO4); 			// **************** finalul 2 al zonei critice
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului de hidrogen
 */
void* hydrogen(void *arg) {
	++hydrogenCnt;						// creste nr de atomi de hidrogen creati

	displayCreationInfo("hydrogen", hydrogenCnt);		// afiseaza informatiile CREARE

	hydrogenID[hydrogenCnt] = pthread_self();		// se stocheaza id-ul thread-ului curent in array-ul de thread-uri

	pthread_mutex_lock(&mutexH2SO4); 			// **************** inceputul zonei critice

	++waitingHydrogen;					// a aparut un nou atom de hidrogen in asteptare
	if (waitingHydrogen >= 2 && waitingSodium >= 1 && waitingOxygen >= 4) {		// se verifica daca se poate forma o molecula de H2SO4
		sem_post(&hydrogenQueue);
		sem_post(&hydrogenQueue);			// se "elibereaza" 2 atomi de hidrogen din coada
		waitingHydrogen -= 2;				// se actualizeaza contorul atomilor de hidrogen in asteptare
		
		sem_post(&sodiumQueue);				// se "elibereaza" un atom de sodiu din coada
		--waitingSodium;				// se actualizeaza contorul atomilor de sodiu in asteptare

		sem_post(&oxygenQueue);				// se "elibereaza" 4 atomi de oxigen din coada
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		waitingOxygen -= 4;				// se actualizeaza contorul atomilor de oxigen in asteptare
	}
	else {
		pthread_mutex_unlock(&mutexH2SO4);		// **************** finalul zonei critice
	}

	sem_wait(&hydrogenQueue);				// atom de hidrogen in asteptare sa formeze o molecula
	
	displayFormationInfo("H2SO4", H2SO4Cnt,
	"hydrogen", findHydrogenID(pthread_self()));		// afiseaza informatiile FORMARE

	pthread_barrier_wait(&barrierH2SO4);			// se ajunge la bariera
	
	displayTerminatedAtomInfo("hydrogen",
	findHydrogenID(pthread_self()));			// afiseaza informatiile TERMINARE_ATOM	
}


/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului de oxigen
 */
void* oxygen(void *arg) {
	++oxygenCnt;						// creste nr de atomi de oxigen creati

	displayCreationInfo("oxygen", oxygenCnt);		// afiseaza informatiile CREARE

	oxygenID[oxygenCnt] = pthread_self();			// se stocheaza id-ul thread-ului curent in array-ul de thread-uri

	pthread_mutex_lock(&mutexH2SO4); 			// **************** inceputul zonei critice
	
	++waitingOxygen;					// a aparut un nou atom de oxigen in asteptare
	if (waitingHydrogen >= 2 && waitingSodium >= 1 && waitingOxygen >= 4) {		// se verifica daca se poate forma o molecula de H2SO4
		sem_post(&hydrogenQueue);
		sem_post(&hydrogenQueue);			// se "elibereaza" 2 atomi de hidrogen din coada
		waitingHydrogen -= 2;				// se actualizeaza contorul atomilor de hidrogen in asteptare
		
		sem_post(&sodiumQueue);				// se "elibereaza" un atom de sodiu din coada
		--waitingSodium;				// se actualizeaza contorul atomilor de sodiu in asteptare

		sem_post(&oxygenQueue);				// se "elibereaza" 4 atomi de oxigen din coada
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		sem_post(&oxygenQueue);
		waitingOxygen -= 4;				// se actualizeaza contorul atomilor de oxigen in asteptare
	}
	else {
		pthread_mutex_unlock(&mutexH2SO4);		// **************** finalul zonei critice
	}

	sem_wait(&oxygenQueue);					// atom de oxigen in asteptare sa formeze o molecula

	displayFormationInfo("H2SO4", H2SO4Cnt,
	"oxygen", findOxygenID(pthread_self()));		// afiseaza informatiile FORMARE

	pthread_barrier_wait(&barrierH2SO4);			// se ajunge la bariera

	displayTerminatedAtomInfo("oxygen",
	findOxygenID(pthread_self()));				// afiseaza informatiile TERMINARE_ATOM	
}



/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului moleculei de clorura de bariu
 */
void* bariumChloride(void* arg) {
	++BaCl2Cnt;						// creste nr de molecule de BaCl2 create
	printf("\n---------- BaCl2 molecule formed ---------- Total: %d", BaCl2Cnt);

	BaCl2ID[BaCl2Cnt] = pthread_self();			// se stocheaza id-ul thread-ului curent in array-ul de thread-uri
	
	pthread_mutex_lock(&mutexReaction);			// **************** inceputul zonei critice

	++waitingBaCl2;						// a aparut o noua molecula de BaCl2 in asteptare
	if (waitingBaCl2 >= 1 && waitingH2SO4 >= 1) {		// se verifica daca se poate intra in reactie
		++reactionCnt; 
		--waitingBaCl2;					// se actualizeaza contorul de molecule de BaCl2 in asteptare
		sem_post(&BaCl2Queue);				// se "elibereaza" 1 molecula de BaCl2 din coada

		--waitingH2SO4;					// se actualizeaza contorul de molecule de H2SO4 in asteptare
		sem_post(&H2SO4Queue);				// se "elibereaza" 1 molecula de H2SO4 din coada
	}
	else {
		pthread_mutex_unlock(&mutexReaction);		// **************** finalul 1 al zonei critice
	}

	sem_wait(&BaCl2Queue);					// molecula de BaCl2 in asteptare sa intre in reactie
	
	displayReactionInfo("BaCl2",
	findBaCl2ID(pthread_self()));				// afiseaza informatiile REACTIE	

	reaction();						// semnaleaza faptul ca are loc o reactie
	
	pthread_barrier_wait(&barrierReaction);			// se ajunge la bariera

	displayTerminatedMoleculeInfo("BaCl2",
	findBaCl2ID(pthread_self()));				// afiseaza informatiile TERMINARE_MOLECULA

	pthread_mutex_unlock(&mutexReaction); 			// **************** finalul 2 al zonei critice
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * imlementarea functiei care face legatura intre atomi
 * se creeaza thread-uri pentru fiecare molecula formata si se verifica daca s-au format cu succes
 */
void bondBariumChloride() {
	if (0 != pthread_create(&moleculeBaCl2Thr, NULL, bariumChloride, NULL)) {
		printf("Could not create the barium thread!");
		exit(1);
	}
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului de bariu
 */
void* barium(void *arg) {
	++bariumCnt;						// creste nr de atomi de bariu creati
	displayCreationInfo("barium", bariumCnt);		// afiseaza informatiile CREARE

	bariumID[bariumCnt] = pthread_self();
	
	pthread_mutex_lock(&mutexBaCl2); 			// **************** inceputul zonei critice
	
	++waitingBarium;					// a aparut un nou atom de bariu in asteptare
	if (waitingChlorine >= 2) {				// se verifica daca se poate forma o molecula de BaCl2
		sem_post(&chlorineQueue);	
		sem_post(&chlorineQueue);			// se "elibereaza" 2 atomi de clor din coada
		waitingChlorine -= 2;				// se actualizeaza contorul atomilor de clor in asteptare
		
		sem_post(&bariumQueue);				// se "elibereaza" un atom de bariu din coada
		--waitingBarium;				// se actualizeaza contorul atomilor de bariu in asteptare
	}
	else {
		pthread_mutex_unlock(&mutexBaCl2);		// **************** finalul 1 al zonei critice
	}

	sem_wait(&bariumQueue);					// atom de bariu in asteptare sa formeze o molecula
	
	displayFormationInfo("BaCl2", BaCl2Cnt,
	"barium", findBariumID(pthread_self()));		// afiseaza informatiile FORMARE

	bondBariumChloride();					// se creeaza legatura cu ceilalti 2 atomi

	pthread_barrier_wait(&barrierBaCl2); 			// se ajunge la bariera
	
	displayTerminatedAtomInfo("barium",
	findBariumID(pthread_self()));				// afiseaza informatiile TERMINARE_ATOM	

	pthread_mutex_unlock(&mutexBaCl2); 			// **************** finalul 2 al zonei critice
}	

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * instructiunile thread-ului de clor
 */
void* chlorine(void *arg) {
	++chlorineCnt;						// creste nr de atomi de clor creati
	displayCreationInfo("chlorine", chlorineCnt);		// afiseaza informatiile de CREARE
	
	chlorineID[chlorineCnt] = pthread_self();		// se stocheaza id-ul thread-ului curent in array-ul de thread-uri

	pthread_mutex_lock(&mutexBaCl2); 			// **************** inceputul zonei critice
	
	++waitingChlorine;					// a aparut un nou atom de clor in asteptare
	if (waitingChlorine >= 2 && waitingBarium >= 1) {	// se verifica daca se poate forma o molecula de BaCl2
		sem_post(&chlorineQueue);
		sem_post(&chlorineQueue);			// se "elibereaza" 2 atomi de clor din coada
		waitingChlorine -= 2;				// se actualizeaza contorul atomilor de clor in asteptare
		
		sem_post(&bariumQueue);				// se "elibereaza" un atom de bariu din coada
		--waitingBarium;				// se actualizeaza contorul atomilor de bariu in asteptare
	}
	else {
		pthread_mutex_unlock(&mutexBaCl2);		// **************** finalul zonei critice
	}

	sem_wait(&chlorineQueue);				// atom de clor in asteptare sa formeze o molecula

	displayFormationInfo("BaCl2", BaCl2Cnt,
	"chlorine", findChlorineID(pthread_self()));		// afiseaza informatiile FORMARE

	pthread_barrier_wait(&barrierBaCl2);			// se ajunge la bariera
	
	displayTerminatedAtomInfo("chlorine",
	findChlorineID(pthread_self()));			// afiseaza informatiile TERMINARE_ATOM	
}

/* -------------------------------------------------------------------------------------------------------------------------------------------------- *
 * functia principala main
 */
int main(int argc, char **argv) {
	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * se verifica daca numarul de argumente este corect
	 */
	switch (argc) {
		case 2:		/* ---------------------------------------------------------------------------------------- *
				 * se verifica daca argumentele sunt corecte
				 */
				if (0!= strcmp(argv[0], "./t2")) {
					printf("The arguments are wrong!\nUse the following pattern: ./t2 N\n");
					exit(1);
				}

				N = atoi(argv[1]);		// numarul de molecule de HCl care trebuie formate
				if (N <= 0) {			// verific daca utilizatorul a introdus un nr relevant
					printf("The value of N has to be 1 or higher.\n");
					exit(1);
				}

				isOrdered = 0; // se poate comenta aceasta linie, deoare este variabila globala

				break;

		case 3:		/* ---------------------------------------------------------------------------------------- *
				 * se verifica daca argumentele sunt corecte
				 */
				if (0!= strcmp(argv[0], "./t2") || 0 != strcmp(argv[1], "-o")) {
					printf("The arguments are wrong!\nUse the following pattern: ./t2 -o N\n");
					exit(1);
				}
				
				N = atoi(argv[2]);		// numarul de molecule de HCl care trebuie formate	
				if (N <= 0) {			// verific daca utilizatorul a introdus un nr relevant
					printf("The value of N has to be 1 or higher./n");
					exit(1);
				}

				isOrdered = 1;

				break;

		default:	printf("The number of arguments are wrong!\nUse one of the following patterns:\n");
				printf("./t2 -o N	(for forming the molecules orderly)\n");
				printf("./t2 N 		(for forming the molecules randomly)\n");
				exit(1);

				break;
	}

	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * se initializeaza semafoarele necesare
	 */
	pthread_mutex_init(&mutexBaCl2		, NULL);	// se initializeaza toate mutex-urile
	pthread_mutex_init(&mutexH2SO4		, NULL);
	pthread_mutex_init(&mutexReaction	, NULL);

	sem_init(&bariumQueue	, 0, 0);			// se initializeaza toate semafoarele
	sem_init(&chlorineQueue	, 0, 0);
	sem_init(&hydrogenQueue	, 0, 0);
	sem_init(&sodiumQueue	, 0, 0);
	sem_init(&oxygenQueue	, 0, 0);
	sem_init(&BaCl2Queue	, 0, 0);
	sem_init(&H2SO4Queue	, 0, 0);

	pthread_barrier_init(&barrierBaCl2	, 0, 3);	// pt a forma o molecula de BaCl2 avem nevoie de 3 atomi
	pthread_barrier_init(&barrierH2SO4	, 0, 7);	// pt a forma o molecula de H2SO4 avem nevoie de 7 atomi
	pthread_barrier_init(&barrierReaction	, 0, 2);	// pt a forma o reactie avem nevoie de 1 molecula de BaCl2 si 1 de H2SO4
	
	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * se aloca dinamic memoria necesara pentru array-uri
	 */
	necessaryMolecules = computeNecessaryMolecules();					// se calculeaza nr necesar de molecule care trebuie formate
	
	pthread_t	bariumThr, chlorineThr, sodiumThr, oxygenThr, hydrogenThr;		// se creeaza thread-urile pentru atomi

	bariumID		= (long int*)malloc(sizeof(long int) * necessaryMolecules);
	chlorineID		= (long int*)malloc(sizeof(long int) * necessaryMolecules * 2);
	sodiumID		= (long int*)malloc(sizeof(long int) * necessaryMolecules);
	oxygenID		= (long int*)malloc(sizeof(long int) * necessaryMolecules * 4);
	hydrogenID		= (long int*)malloc(sizeof(long int) * necessaryMolecules * 2);
	BaCl2ID			= (long int*)malloc(sizeof(long int) * necessaryMolecules);
	H2SO4ID 		= (long int*)malloc(sizeof(long int) * necessaryMolecules);
	

	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * generatoarele de atomi
	 * se creeaza thread-urile de atomi, verificandu-se in acelasi timp daca s-au creat cu succes
	 * 
	 * isOrdered = 0 generare neordonata
	 * isOrdered = 1 generare ordonata
	 */
	switch (isOrdered) {
		case 0:		/* ---------------------------------------------------------------------------------------- *
				 * generarea neordonata
				 * cat timp nu este satisfacuta conditia de terminare a programului...
				 */
				while (!done()) {
					// "generator" de atomi de bariu
					if (bariumCnt < necessaryMolecules) {	
						if (0 != pthread_create(&bariumThr, NULL, barium, NULL)) {
							printf("Could not create the barium thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de clor
					if (chlorineCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&chlorineThr, NULL, chlorine, NULL)) {
							printf("Could not create the chlorine thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de sodiu
					if (sodiumCnt < necessaryMolecules) {
						if (0 != pthread_create(&sodiumThr, NULL, sodium, NULL)) {
							printf("Could not create the sodium thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de hidrogen
					if (hydrogenCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&hydrogenThr, NULL, hydrogen, NULL)) {
							printf("Could not create the hydrogen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de oxigen
					if (oxygenCnt < 4 * necessaryMolecules) {
						if (0 != pthread_create(&oxygenThr, NULL, oxygen, NULL) ){
							printf("Could not create the oxygen thread!");
							exit(1);
						}
						sleep(1);
					}

					++atom;

					if (done()) { exit(0); }		// se reverifica daca se satisface conditia de terminare
				}

				break;

	case 1:			/* ---------------------------------------------------------------------------------------- *
				 * generarea ordonata
				 * cat timp nu este satisfacuta conditia de terminare a programului...
				 */
				while (!done()) {
					// "generator" de atomi de bariu
					if (bariumCnt < necessaryMolecules) {	
						if (0 != pthread_create(&bariumThr, NULL, barium, NULL)) {
							printf("Could not create the barium thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de clor
					if (chlorineCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&chlorineThr, NULL, chlorine, NULL)) {
							printf("Could not create the chlorine thread!");
							exit(1);
						}
						sleep(1);
					}
						
					// "generator" de atomi de clor
					if (chlorineCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&chlorineThr, NULL, chlorine, NULL)) {
							printf("Could not create the chlorine thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de sodiu
					if (sodiumCnt < necessaryMolecules) {
						if (0 != pthread_create(&sodiumThr, NULL, sodium, NULL)) {
							printf("Could not create the sodium thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de hidrogen
					if (hydrogenCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&hydrogenThr, NULL, hydrogen, NULL)) {
							printf("Could not create the hydrogen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de hidrogen
					if (hydrogenCnt < 2 * necessaryMolecules) {
						if (0 != pthread_create(&hydrogenThr, NULL, hydrogen, NULL)) {
							printf("Could not create the hydrogen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de oxigen
					if (oxygenCnt < 4 * necessaryMolecules) {
						if (0 != pthread_create(&oxygenThr, NULL, oxygen, NULL) ){
							printf("Could not create the oxygen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de oxigen
					if (oxygenCnt < 4 * necessaryMolecules) {
						if (0 != pthread_create(&oxygenThr, NULL, oxygen, NULL) ){
							printf("Could not create the oxygen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de oxigen
					if (oxygenCnt < 4 * necessaryMolecules) {
						if (0 != pthread_create(&oxygenThr, NULL, oxygen, NULL) ){
							printf("Could not create the oxygen thread!");
							exit(1);
						}
						sleep(1);
					}

					// "generator" de atomi de oxigen
					if (oxygenCnt < 4 * necessaryMolecules) {
						if (0 != pthread_create(&oxygenThr, NULL, oxygen, NULL) ){
							printf("Could not create the oxygen thread!");
							exit(1);
						}
						sleep(1);
					}

					++atom;

					if (done()) { exit(0); }		// se reverifica daca se satisface conditia de terminare
				}

				break;
	default:		break;
	}

	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * join tuturor thread-urilor
	 */
	pthread_join(  bariumThr, NULL);
	pthread_join(chlorineThr, NULL);
	pthread_join(  sodiumThr, NULL);
	pthread_join(  oxygenThr, NULL);
	pthread_join(hydrogenThr, NULL);

	pthread_join(moleculeBaCl2Thr, NULL);
	pthread_join(moleculeH2SO4Thr, NULL);

	/* ------------------------------------------------------------------------------------------------------------------------------------------ *
	 * se distrug mutex-urile, semafoarele si barierele
	 */
	pthread_mutex_destroy(&mutexBaCl2);
	pthread_mutex_destroy(&mutexH2SO4);
	pthread_mutex_destroy(&mutexReaction);

	sem_destroy(&bariumQueue);
	sem_destroy(&chlorineQueue);
	sem_destroy(&sodiumQueue);
	sem_destroy(&oxygenQueue);
	sem_destroy(&hydrogenQueue);
	sem_destroy(&BaCl2Queue);
	sem_destroy(&H2SO4Queue);

	pthread_barrier_destroy(&barrierBaCl2);
	pthread_barrier_destroy(&barrierH2SO4);
	pthread_barrier_destroy(&barrierReaction);

	exit(0);
}



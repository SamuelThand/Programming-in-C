#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>

#define HYDROGEN_THREADS 10
#define OXYGEN_THREADS 5
#define ATOMS_TO_FORM_MOLECULE 3

typedef enum {
  OXYGEN,
  HYDROGEN
} Element;

typedef struct {
    int atom_number;
    Element element;
} Atom;

struct ReactionChamber {
    int atoms_inside;
    sem_t react_semaphore;
    sem_t consumed_semaphore;
    pthread_mutex_t reaction_lock;
} reactionChamber = {
        .atoms_inside = 0,
        .reaction_lock = PTHREAD_MUTEX_INITIALIZER
};

struct Lab {
    int oxygen_count;
    int hydrogen_count;
    sem_t oxygen_semaphore;
    sem_t hydrogen_semaphore;
    pthread_mutex_t molecule_creation_procedure_lock;
} lab = {
        .oxygen_count = 0,
        .hydrogen_count = 0,
        .molecule_creation_procedure_lock = PTHREAD_MUTEX_INITIALIZER
};


void createOxygenThreads(pthread_t *oxygen_threads);
void createHydrogenThreads(pthread_t *hydrogen_threads);
void *oxygenReady(void *arg);
void *hydrogenReady(void *arg);
void enterReactionChamber(Atom *atom);
void formWaterMolecule(Atom *atom);
char *getElementString(Atom *atom);

/**
 * Runs a simulation where H20 molecules are formed. Each atom is a separate thread, and they need to
 * ensure they only call the enterReactionChamber() function together with the correct amount of the other atom type.
 *
 * @return Status code
 */
int main()
{
    srand(time(NULL));
    sem_init(&lab.oxygen_semaphore, 0 , 0);
    sem_init(&lab.hydrogen_semaphore, 0 , 0);
    sem_init(&reactionChamber.react_semaphore, 0, 0);
    sem_init(&reactionChamber.consumed_semaphore, 0, 1);

    pthread_t *oxygen_threads = malloc(OXYGEN_THREADS * sizeof (pthread_t));
    pthread_t *hydrogen_threads = malloc(HYDROGEN_THREADS * sizeof (pthread_t));
    createOxygenThreads(oxygen_threads);
    createHydrogenThreads(hydrogen_threads);

    for (int i = 0; i < OXYGEN_THREADS ; ++i) {
        void *result;
        pthread_join(oxygen_threads[i], &result);
        if (result != NULL) {
            free(result);
        }
    }
    free(oxygen_threads);

    for (int i = 0; i < HYDROGEN_THREADS; ++i) {
        void *result;
        pthread_join(hydrogen_threads[i], &result);
        if (result != NULL) {
            free(result);
        }
    }
    free(hydrogen_threads);

    sem_destroy(&lab.oxygen_semaphore);
    sem_destroy(&lab.hydrogen_semaphore);
    pthread_mutex_destroy(&lab.molecule_creation_procedure_lock);

    sem_destroy(&reactionChamber.react_semaphore);
    sem_destroy(&reactionChamber.consumed_semaphore);
    pthread_mutex_destroy(&reactionChamber.reaction_lock);

    return 0;
}

/**
* Creates Oxygen threads with an Atom struct with the atom number and its element and adds their
* thread identifiers to the oxygen_threads array.
*
* @param oxygen_threads Thread identifier array for oxygen threads
*/
void createOxygenThreads(pthread_t *oxygen_threads)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    for (int i = 0; i < OXYGEN_THREADS; ++i) {
        Atom* atom = (Atom*) malloc(sizeof(Atom));
        atom -> atom_number = i + 1;
        atom -> element = OXYGEN;

        int thread_created = pthread_create(
                &oxygen_threads[i], &attr, oxygenReady, (void *) atom);
        if (thread_created != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
}

/**
* Creates Hydrogen threads with an Atom struct with the atom number and its element and adds their
* thread identifiers to the hydrogen_threads array.
*
* @param hydrogen_threads Thread identifier array for hydrogen threads
*/
void createHydrogenThreads(pthread_t *hydrogen_threads)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    for (int i = 0; i < HYDROGEN_THREADS; ++i) {
        Atom* atom = (Atom*) malloc(sizeof(Atom));
        atom -> atom_number = i + 1;
        atom -> element = HYDROGEN;

        int thread_created = pthread_create(
                &hydrogen_threads[i], &attr, hydrogenReady, (void*) atom);
        if (thread_created != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
}

/**
* Waits for two hydrogen atoms, then enters the reaction chamber together with them.
*
* @param arg Atom struct
*/
void *oxygenReady(void *arg)
{
    Atom *atom = (Atom*) arg;

    pthread_mutex_lock(&lab.molecule_creation_procedure_lock);
    lab.oxygen_count++;
    if (lab.hydrogen_count >= 2) {
        sem_post(&lab.hydrogen_semaphore);
        sem_post(&lab.hydrogen_semaphore);
        lab.hydrogen_count--;
        lab.hydrogen_count--;
        sem_post(&lab.oxygen_semaphore);
        lab.oxygen_count--;
    } else {
        pthread_mutex_unlock(&lab.molecule_creation_procedure_lock);
    }

    sem_wait(&lab.oxygen_semaphore);
    enterReactionChamber(atom);
    formWaterMolecule(atom);
    pthread_mutex_unlock(&lab.molecule_creation_procedure_lock);

    return atom;
}

/**
* Waits for one oxygen atom and another hydrogen atom, then goes into the reaction chamber with them.
*
* @param arg Atom struct
*/
void *hydrogenReady(void *arg)
{
    pthread_mutex_lock(&lab.molecule_creation_procedure_lock);
    Atom *atom = (Atom*) arg;

    lab.hydrogen_count++;
    if (lab.hydrogen_count >= 2 && lab.oxygen_count >= 1) {
        sem_post(&lab.hydrogen_semaphore);
        sem_post(&lab.hydrogen_semaphore);
        lab.hydrogen_count--;
        lab.hydrogen_count--;
        sem_post(&lab.oxygen_semaphore);
        lab.oxygen_count--;
    } else {
        pthread_mutex_unlock(&lab.molecule_creation_procedure_lock);
    }

    sem_wait(&lab.hydrogen_semaphore);
    enterReactionChamber(atom);
    formWaterMolecule(atom);

    return atom;
}

/**
* An atom enters the reaction chamber, waits for two other atoms, and reacts for a random amount of time before
* continuing.
*
* @param arg Atom struct
*/
void enterReactionChamber(Atom *atom)
{
    pthread_mutex_lock(&reactionChamber.reaction_lock);
    printf("| %s ATOM %d | enters the reaction chamber..\n", getElementString(atom), atom->atom_number);
    reactionChamber.atoms_inside++;
    if (reactionChamber.atoms_inside == ATOMS_TO_FORM_MOLECULE) {
        sem_wait(&reactionChamber.consumed_semaphore);
        sem_post(&reactionChamber.react_semaphore);
    }
    sleep((rand() % 4) / 3);
    pthread_mutex_unlock(&reactionChamber.reaction_lock);

    sem_wait(&reactionChamber.react_semaphore);
    sem_post(&reactionChamber.react_semaphore);
}

/**
* An atom waits for two other atoms, is consumed, and a water molecule is formed for each oxygen atom.
*
* @param arg Atom struct
*/
void formWaterMolecule(Atom *atom)
{
    pthread_mutex_lock(&reactionChamber.reaction_lock);
    reactionChamber.atoms_inside--;
    if (reactionChamber.atoms_inside == 0) {
        sem_wait(&reactionChamber.react_semaphore);
        sem_post(&reactionChamber.consumed_semaphore);
    }
    pthread_mutex_unlock(&reactionChamber.reaction_lock);

    sem_wait(&reactionChamber.consumed_semaphore);
    sem_post(&reactionChamber.consumed_semaphore);

    if (strcmp(getElementString(atom), "OXYGEN") == 0) {
        printf("\033[0;34m| H20 MOLECULE CREATED | \033[0m\n");
    }
}

/**
 * Get the string representation of the atom element.
 *
 * @param atom Atom struct
 * @return Atom Element as string literal
 */
char *getElementString(Atom *atom)
{
    switch (atom->element) {
        case OXYGEN:
            return "OXYGEN";
        case HYDROGEN:
            return "HYDROGEN";
        default:
            return "UNKNOWN";
    }
}

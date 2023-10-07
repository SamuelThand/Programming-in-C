#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>

#define HACKERS 6
#define PEASANTS 6
#define BOAT_CAPACITY 4
#define NAMES_AMOUNT 50
#define NAMES_MAX_LEN 20
#define NAMES {\
    "Erik", "Sara", "Gustav", "Hanna", "Oscar", \
    "Emilia", "Johan", "Elin", "Filip", "Julia", \
    "Anders", "Moa", "Sebastian", "Frida", "Axel", \
    "Linnea", "Karl", "Linnéa", "Jesper", "Ida", \
    "Simon", "Wilma", "David", "Ellen", "Nils", \
    "Ebba", "Björn", "Evelina", "Mikael", "Jenny", \
    "Maria", "Magnus", "Anna", "Daniel", "Cecilia", \
    "Ludvig", "Nora", "Peter", "Emma", "Victor", \
    "Felicia", "Robin", "Maja", "Jonathan", "Alma", \
    "Johannes", "Elsa", "Samuel", "Tove", "Janne"\
}

typedef enum {
    HACKER,
    PEASANT
} PersonType;

typedef struct {
    int id;
    PersonType type;
    char name[NAMES_MAX_LEN];
} Person;

struct Boat {
    int persons_boarding;
    pthread_mutex_t board_disembark_lock;
    sem_t board_semaphore;
    sem_t disembark_semaphore;
} boat = {
        .persons_boarding = 0,
        .board_disembark_lock = PTHREAD_MUTEX_INITIALIZER
};

struct Dock {
    int boat_counter;
    int hackers_waiting_to_board;
    int peasants_waiting_to_board;
    sem_t hacker_semaphore;
    sem_t peasant_semaphore;
    pthread_mutex_t travel_procedure_lock;
} dock = {
        .boat_counter = 1,
        .hackers_waiting_to_board = 0,
        .peasants_waiting_to_board = 0,
        .travel_procedure_lock = PTHREAD_MUTEX_INITIALIZER
};


void createHackerThreads(pthread_t *hacker_threads);
void createPeasantThreads(pthread_t *peasant_threads);
void *hackerFunction(void *arg);
void *peasantFunction(void *arg);
void board(Person *person);
void disembark(Person *person);
void rowBoat(Person *person);
char *getTypeString(Person *person);

/**
 * Runs a simulation where hackers and peasants board and row a boat. Each boat must be filled to BOAT_CAPACITY, and
 * there can only be a full boat of hackers/peasants or half of each. Only one boatload should board at the time, and
 * exactly one person should row the boat. The boatload should have disembarked before a new boatload can board.
 *
 * @return Status code
 */
int main()
{
    srand(time(NULL));
    sem_init(&dock.hacker_semaphore, 0, 0);
    sem_init(&dock.peasant_semaphore, 0, 0);
    sem_init(&boat.board_semaphore, 0, 0);
    sem_init(&boat.disembark_semaphore, 0, 1);

    pthread_t *hacker_threads = malloc(HACKERS * sizeof (pthread_t));
    pthread_t *peasant_threads = malloc(PEASANTS * sizeof (pthread_t));
    createHackerThreads(hacker_threads);
    createPeasantThreads(peasant_threads);

    for (int i = 0; i < HACKERS; ++i) {
        void* result;
        pthread_join(hacker_threads[i], &result);
        if (result != NULL) {
            free(result);
        }
    }
    free(hacker_threads);

    for (int i = 0; i < PEASANTS ; ++i) {
        void* result;
        pthread_join(peasant_threads[i], &result);
        if (result != NULL) {
            free(result);
        }
    }
    free(peasant_threads);

    pthread_mutex_destroy(&dock.travel_procedure_lock);
    sem_destroy(&dock.hacker_semaphore);
    sem_destroy(&dock.peasant_semaphore);

    pthread_mutex_destroy(&boat.board_disembark_lock);
    sem_destroy(&boat.board_semaphore);
    sem_destroy(&boat.disembark_semaphore);

    return 0;
}

/**
 * Create hacker threads with a number and a random name, and add them to the hacker_threads array.
 *
 * @param hacker_threads Pointer to thread identifier array
 */
void createHackerThreads(pthread_t *hacker_threads)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    char names[NAMES_AMOUNT][NAMES_MAX_LEN] = NAMES;
    for (int i = 0; i < HACKERS; ++i) {
        Person* hacker = (Person*) malloc(sizeof(Person));
        hacker -> id = i + 1;
        hacker -> type = HACKER;
        strcpy(hacker->name, names[(rand() % NAMES_AMOUNT)]);

        int thread_created = pthread_create(
                &hacker_threads[i], &attr, hackerFunction, (void*) hacker);
        if (thread_created != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
}

/**
 * Create hacker threads with a number and a random name, and add them to the peasant_threads array.
 *
 * @param peasant_threads Pointer to thread identifier array
 */
void createPeasantThreads(pthread_t *peasant_threads)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    char names[NAMES_AMOUNT][NAMES_MAX_LEN] = NAMES;
    for (int i = 0; i < HACKERS; ++i) {
        Person* peasant = (Person*) malloc(sizeof(Person));
        peasant -> id = i + 1;
        peasant -> type = PEASANT;
        strcpy(peasant->name, names[(rand() % NAMES_AMOUNT)]);

        int thread_created = pthread_create(
                &peasant_threads[i], &attr, peasantFunction, (void*) peasant);
        if (thread_created != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
}

/**
 * A hacker tries to board the dock together with other people if there are enough hackers and/or peasants
 * waiting to board, and waits if not. Then rows the dock if he is designated captain, and disembarks together
 * with the other passengers.
 *
 * @param arg Person struct
 * @return Person pointer
 */
void *hackerFunction(void *arg)
{
    Person *person = (Person *) arg;
    bool is_captain = false;

    pthread_mutex_lock(&dock.travel_procedure_lock);
    dock.hackers_waiting_to_board++;
    if (dock.hackers_waiting_to_board == BOAT_CAPACITY) {
        for (int i = 0; i < BOAT_CAPACITY; ++i) {
            sem_post(&dock.hacker_semaphore);
        }
        dock.hackers_waiting_to_board = 0;
        is_captain = true;
    } else if (dock.hackers_waiting_to_board == (BOAT_CAPACITY / 2) && dock.peasants_waiting_to_board >= (BOAT_CAPACITY / 2)) {
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            sem_post(&dock.hacker_semaphore);
        }
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            sem_post(&dock.peasant_semaphore);
        }
        dock.hackers_waiting_to_board = 0;
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            dock.peasants_waiting_to_board--;
        }
        is_captain = true;
    } else {
        pthread_mutex_unlock(&dock.travel_procedure_lock);
    }

    sem_wait(&dock.hacker_semaphore);
    board(person);

    if (is_captain) {
        rowBoat(person);
        dock.boat_counter++;
    }

    disembark(person);

    if (is_captain) {
        pthread_mutex_unlock(&dock.travel_procedure_lock);
    }

    return person;
}

/**
 * A peasant tries to board the boat together with other people if there are enough hackers and/or peasants
 * waiting to board, and waits if not. Then rows the boat if he is designated captain, and disembarks together
 * with the other passengers.
 *
 * @param arg Person struct
 * @return Person pointer
 */
void *peasantFunction(void *arg)
{
    Person *person = (Person *) arg;
    bool is_captain = false;

    pthread_mutex_lock(&dock.travel_procedure_lock);
    dock.peasants_waiting_to_board++;
    if (dock.peasants_waiting_to_board == BOAT_CAPACITY) {
        for (int i = 0; i < BOAT_CAPACITY; ++i) {
            sem_post(&dock.peasant_semaphore);
        }
        dock.peasants_waiting_to_board = 0;
        is_captain = true;
    } else if (dock.peasants_waiting_to_board == (BOAT_CAPACITY / 2) && dock.hackers_waiting_to_board >= (BOAT_CAPACITY / 2)) {
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            sem_post(&dock.peasant_semaphore);
        }
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            sem_post(&dock.hacker_semaphore);
        }
        dock.peasants_waiting_to_board = 0;
        for (int i = 0; i < (BOAT_CAPACITY / 2); ++i) {
            dock.hackers_waiting_to_board--;
        }
        is_captain = true;
    } else {
        pthread_mutex_unlock(&dock.travel_procedure_lock);
    }

    sem_wait(&dock.peasant_semaphore);
    board(person);

    if (is_captain) {
        rowBoat(person);
        dock.boat_counter++;
    }

    disembark(person);

    if (is_captain) {
        pthread_mutex_unlock(&dock.travel_procedure_lock);
    }

    return person;
}

/**
 * A person boards the boat and waits until the boat is full before execution continues.
 *
 * @param person Person struct
 */
void board(Person *person)
{
    pthread_mutex_lock(&boat.board_disembark_lock);
    printf("[%s %d: %s] is boarding boat %d..\n", getTypeString(person), person->id, person->name, dock.boat_counter);
    boat.persons_boarding++;
    if (boat.persons_boarding == BOAT_CAPACITY) {
        sem_wait(&boat.disembark_semaphore);
        sem_post(&boat.board_semaphore);
    }
    sleep((rand() % 4) / 3);
    pthread_mutex_unlock(&boat.board_disembark_lock);

    sem_wait(&boat.board_semaphore);
    sem_post(&boat.board_semaphore);
}

/**
 * A person disembarks from the the boat and waits until every other passenger has disembarked before execution continues.
 *
 * @param person Person struct
 */
void disembark(Person *person)
{
    pthread_mutex_lock(&boat.board_disembark_lock);
    boat.persons_boarding--;
    if (boat.persons_boarding == 0) {
        sem_wait(&boat.board_semaphore);
        sem_post(&boat.disembark_semaphore);
    }
    pthread_mutex_unlock(&boat.board_disembark_lock);

    sem_wait(&boat.disembark_semaphore);
    sem_post(&boat.disembark_semaphore);
}

/**
 * A person rows the boat.
 *
 * @param person Person struct
 */
void rowBoat(Person *person)
{
    printf("\033[0;34m[%s %d: %s] is rowing boat %d!\033[0m\n", getTypeString(person), person->id, person->name, dock.boat_counter);
}

/**
 * Get the string representation of the person type.
 *
 * @param person Person struct
 * @return Person type as string literal
 */
char *getTypeString(Person *person)
{
    switch (person->type) {
        case HACKER:
            return "HACKER";
        case PEASANT:
            return "PEASANT";
        default:
            return "UNKNOWN";
    }
}
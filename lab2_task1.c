#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>

#define STUDENTS 8
#define CHAIRS 5
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

typedef struct {
    int student_number;
    char name[NAMES_MAX_LEN];
} Student;

struct Teacher {
    bool is_asleep;
    sem_t sleeping_semaphore;
    sem_t help_complete_sempahore;
    pthread_mutex_t available_lock;
} teacher = {
        .is_asleep = false,
        .available_lock = PTHREAD_MUTEX_INITIALIZER
};

struct Office {
    Student *student_being_helped;
    sem_t office_door_semaphore;
} office = {
        .student_being_helped = NULL
};

struct WaitingRoom {
    int waiting_students;
    sem_t chairs_available_semaphore;
    pthread_mutex_t mutex;
} waitingRoom = {
        .waiting_students = 0,
        .mutex = PTHREAD_MUTEX_INITIALIZER
};

pthread_t createTeacherThread();
void createStudentThreads(pthread_t *student_threads);
void *teacherFunction();
void *studentFunction(void *arg);

/**
 * Runs a simulation where 8 student threads are working on a task, and competing for
 * access to the teacher, by going through the waiting room, then into his office.
 *
 * @return Status code
 */
int main()
{
    srand(time(NULL));
    sem_init(&teacher.sleeping_semaphore, 0, 0);
    sem_init(&teacher.help_complete_sempahore, 0, 0);
    sem_init(&office.office_door_semaphore, 0, 1);
    sem_init(&waitingRoom.chairs_available_semaphore, 0, CHAIRS);

    pthread_t teacher_thread = createTeacherThread();
    pthread_t *student_threads = malloc(STUDENTS * sizeof (pthread_t));
    createStudentThreads(student_threads);

    for (int i = 0; i < STUDENTS; ++i) {
        void *result;
        pthread_join(student_threads[i], &result);
        if (result != NULL) {
            free(result);
        }
    }
    free(student_threads);

    pthread_join(teacher_thread, NULL);

    sem_destroy(&teacher.sleeping_semaphore);
    sem_destroy(&office.office_door_semaphore);
    sem_destroy(&waitingRoom.chairs_available_semaphore);
    pthread_mutex_destroy(&teacher.available_lock);

    return 0;
}

/**
 * Create student threads with a number and a random name, and add them to the student_threads array.
 *
 * @param student_threads Pointer to thread identifier array
 */
void createStudentThreads(pthread_t *student_threads)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    char names[NAMES_AMOUNT][NAMES_MAX_LEN] = NAMES;
    for (int i = 0; i < STUDENTS; ++i) {
        Student* student = (Student*) malloc(sizeof (Student));
        student -> student_number = i + 1;
        strcpy(student->name, names[(rand() % NAMES_AMOUNT)]);

        int thread_created = pthread_create(
                &student_threads[i], &attr, studentFunction, (void*) student);
        if (thread_created != 0) {
            perror("Thread creation failed");
            exit(1);
        }
    }
}

/**
 * Create a teacher thread and return its thread identifier.
 *
 * @return Thread identifier
 */
pthread_t createTeacherThread()
{
    pthread_t teacher_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if(pthread_attr_setschedpolicy(&attr, SCHED_FIFO) != 0) {
        printf("Unable to set FIFO policy..");
    }

    int thread_created = pthread_create(&teacher_id, &attr, teacherFunction, NULL);
    if (thread_created != 0) {
        perror("Thread creation failed");
        exit(1);
    }

    return teacher_id;
}

/**
 * The student works on a simulated task for a random time, then tries to enter the waiting room, enter the
 * teachers office, and get help from the teacher. Wakes the teacher if he is sleeping.
 *
 * @param arg Student struct
 * @return Student struct
 */
void *studentFunction(void *arg)
{
    Student *student = (Student*) arg;

    int completed_work_percent = 0;
    while (completed_work_percent < 100) {

        printf("\033[0;32m[Student %d - %s]\033[0m Programming \033[0;36m[%d%% Progress]\033[0m\n",
               student->student_number, student->name, completed_work_percent);

        int work = rand() % 10;
        completed_work_percent += work * 10;
        sleep(work);

        if (sem_trywait(&waitingRoom.chairs_available_semaphore) == 0) {

            pthread_mutex_lock(&waitingRoom.mutex);
            waitingRoom.waiting_students++;
            pthread_mutex_unlock(&waitingRoom.mutex);
            printf("\033[0;32m[Student %d - %s]\033[0m Entered waiting room \033[0;35m[%d/%d chairs taken]\033[0m\n",
                   student->student_number, student->name, waitingRoom.waiting_students, CHAIRS);

            sem_wait(&office.office_door_semaphore);
            pthread_mutex_lock(&waitingRoom.mutex);
            waitingRoom.waiting_students--;
            pthread_mutex_unlock(&waitingRoom.mutex);
            printf("\033[0;32m[Student %d - %s]\033[0m Enters teachers office\n",
                   student->student_number, student->name);
            sem_post(&waitingRoom.chairs_available_semaphore);


            pthread_mutex_lock(&teacher.available_lock);
            if (teacher.is_asleep) {
                printf("\033[0;32m[Student %d - %s]\033[0m Waking teacher\n",
                       student->student_number, student->name);
                sem_post(&teacher.sleeping_semaphore);
            }

            office.student_being_helped = student;
            sem_wait(&teacher.help_complete_sempahore);
            printf("\033[0;32m[Student %d - %s]\033[0m Leaving teachers office\n",
                   student->student_number, student->name);
            pthread_mutex_unlock(&teacher.available_lock);

        } else {
            printf("\033[0;34m[Student %d - %s] Tried to enter full waiting room, continues programming for a while\033[0m\n",
                   student->student_number, student->name);
        }
    }

    printf("\033[1;33m[Student %d - %s] PROGRAMMING 100%\%\033[0m\n",
           student->student_number, student->name);

    return student;
}

/**
 * The teacher sleeps if the waiting room is empty, if he is awake he waits for a student to enter his office,
 * then helps the student for a random period of time and sends him away.
 */
void *teacherFunction()
{
    while (true) {
        if (waitingRoom.waiting_students == 0) {
            teacher.is_asleep = true;
            printf("\033[0;31m|TEACHER| \033[0mNo waiting students, going to sleep..\n");
            sem_wait(&teacher.sleeping_semaphore);
        }
        teacher.is_asleep = false;

        int student_in_office;
        sem_getvalue(&office.office_door_semaphore, &student_in_office);
        printf("\033[0;31m|TEACHER| \033[0mChecking if student is in the office\n");

        while (student_in_office != 0) {
            printf("\033[0;31m|TEACHER| \033[0mWaiting for student to come into the office..\n");
            sleep(1);
            sem_getvalue(&office.office_door_semaphore, &student_in_office);
        }

        printf("\033[0;31m|TEACHER|\033[0m helping \033[0;32m[Student %d - %s]..\033[0m\n",
               office.student_being_helped->student_number, office.student_being_helped->name);
        sleep(rand() % 5);
        printf("\033[0;31m|TEACHER|\033[0m Helping done!\n");
        sem_post(&teacher.help_complete_sempahore);
        printf("\033[0;31m|TEACHER|\033[0m Opened office door\n");
        sem_post(&office.office_door_semaphore);
    }
}

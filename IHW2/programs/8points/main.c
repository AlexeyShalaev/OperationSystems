#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define NUMBER_OF_PROGRAMMERS 3
#define NUMBER_OF_PROGRAMS 5

#define DEBUG_MODE 1

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

typedef enum
{
    NOT_EXISTING,
    WRITTEN,
    CORRECT,
    INCORRECT
} ProgramStatus;

typedef struct
{
    int id;
    ProgramStatus status;
} Program;

typedef struct
{
    Program program;
    int programmers_to_check[NUMBER_OF_PROGRAMMERS];
} Programmer;

typedef struct
{
    Programmer programmers[NUMBER_OF_PROGRAMMERS];
    int target_programs_number;
    int programs_id;
    int created_programs_number;
    int stop_flag;
} Coworking;

int m_sems[NUMBER_OF_PROGRAMMERS];
int shm_id;
Coworking *coworking;

void sigint_handler(int signum)
{
    log("[COWORKING] %d / %d programs created\n", coworking->created_programs_number, coworking->target_programs_number);

    log("Release resources\n");

    coworking->stop_flag = 1;
    sleep(5);
    shmdt(coworking);
    shmctl(shm_id, IPC_RMID, NULL);

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        semctl(m_sems[i], 0, IPC_RMID);
    }

    exit(0);
}

Coworking *get_coworking()
{
    Coworking *coworking;

    shm_id = shmget(0x2FF, getpagesize(), 0666 | IPC_CREAT);
    if (shm_id < 0)
    {
        perror("shmget()");
        exit(1);
    }

    coworking = (Coworking *)shmat(shm_id, NULL, 0);
    if (coworking == (Coworking *)-1)
    {
        perror("shmat()");
        exit(1);
    }

    return coworking;
}

int main()
{
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    coworking = get_coworking();
    coworking->stop_flag = 0;
    coworking->created_programs_number = 0;
    coworking->target_programs_number = NUMBER_OF_PROGRAMS;
    coworking->programs_id = coworking->target_programs_number;

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {

        log("Initializing programmer %d...\n", i);

        coworking->programmers[i].program.status = NOT_EXISTING;

        if ((m_sems[i] = semget(i, 1, IPC_CREAT | 0666)) == -1)
        {
            perror("Error sem get");
            return 1;
        }
    }

    while (coworking->created_programs_number != coworking->target_programs_number && coworking->stop_flag == 0)
    {
        sleep(1);
        log("Waiting for programs to be created...\n");
    }

    sigint_handler(0);

    return 0;
}

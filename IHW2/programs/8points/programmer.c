#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <time.h>

#define NUMBER_OF_PROGRAMMERS 3
#define NUMBER_OF_PROGRAMS 5

#define SLEEP usleep((rand() % 3000000) + 1000000) // случайная задержка от 1 до 3 секунд

#define DEBUG_MODE 1

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

int get_count_of_programmers_to_check(int *arr)
{
    int count = 0;
    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        if (arr[i] == 1)
        {
            ++count;
        }
    }
    return count;
}

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

struct sembuf sem_wait = {0, -1, SEM_UNDO};
struct sembuf sem_signal = {0, 1, SEM_UNDO};

int get_programmer_to_check(Programmer *programmers, int exclude_programmer)
{
    int mn = NUMBER_OF_PROGRAMMERS;
    int index = 0;
    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        if (i == exclude_programmer)
        {
            continue; // Пропускаем программиста, который отправил на проверку (самопроверка)
        }
        int tmp = get_count_of_programmers_to_check(programmers[i].programmers_to_check);
        if (tmp < mn)
        {
            mn = tmp;
            index = i;
        }
    }
    return index;
}

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

void *programmer_work(void *thread_data)
{
    int index = *(int *)thread_data;

    Coworking *coworking = get_coworking();
    Programmer *programmer = &coworking->programmers[index];

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        programmer->programmers_to_check[i] = 0;
    }

    // пишем первую работу
    programmer->program.id = coworking->programs_id;
    --coworking->programs_id;
    SLEEP;
    programmer->program.status = WRITTEN;
    int inspector = get_programmer_to_check(coworking->programmers, index);
    coworking->programmers[inspector].programmers_to_check[index] = 1;
    log("[%d -> %d] sent new program (%d)\n", index, inspector, programmer->program.id);
    semop(index, &sem_signal, 1);

    while (1)
    {
        if (coworking->stop_flag == 1)
        {
            break;
        }

        semop(index, &sem_wait, 1); // Ожидаем своей очереди

        if (coworking->created_programs_number == coworking->target_programs_number)
        {
            break; // все программы созданы
        }

        // Посмотрим что есть на проверку

        for (int surveyed = 0; surveyed < NUMBER_OF_PROGRAMMERS; ++surveyed)
        {
            if (programmer->programmers_to_check[surveyed] == 1)
            {
                if (coworking->programmers[surveyed].program.status == WRITTEN)
                {
                    // нам отправили на проверку, проверяем
                    SLEEP;
                    int verdict = (rand() % 2 == 0);
                    if (verdict == 1)
                    {
                        ++coworking->created_programs_number;
                    }
                    coworking->programmers[surveyed].program.status = verdict ? CORRECT : INCORRECT;
                    log("[%d -> %d] sent verdict (%s) of program (%d)\n", index, surveyed, verdict ? "CORRECT" : "INCORRECT", coworking->programmers[surveyed].program.id);
                    programmer->programmers_to_check[surveyed] = 0;
                    semop(surveyed, &sem_signal, 1); // Отправляем сигнал программисту, чью работу проверили
                }
            }
        }

        if (programmer->program.status == CORRECT)
        {
            if (coworking->created_programs_number == coworking->target_programs_number)
            {
                for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
                {
                    if (i != index)
                    {
                        semop(i, &sem_signal, 1); // отправляем сигнал всем, чтобы они завершились
                    }
                }
                break; // все программы созданы
            }

            if (coworking->programs_id == 0)
            {
                continue; // нет программ для создания
            }

            programmer->program.id = coworking->programs_id;
            --coworking->programs_id;
            SLEEP;
            programmer->program.status = WRITTEN;
            inspector = get_programmer_to_check(coworking->programmers, index);
            log("[%d -> %d] sent new program (%d)\n", index, inspector, programmer->program.id);
            coworking->programmers[inspector].programmers_to_check[index] = 1;
            semop(inspector, &sem_signal, 1); // отправляем на проверку
        }
        else if (programmer->program.status == INCORRECT)
        {
            SLEEP;
            programmer->program.status = WRITTEN;
            coworking->programmers[inspector].programmers_to_check[index] = 1;
            log("[%d -> %d] sent fixed program (%d)\n", index, inspector, programmer->program.id);
            semop(inspector, &sem_signal, 1); // отправляем на проверку
        }
    }

    log("[Programmer %d] finished\n", index);

    pthread_exit(NULL);
}

int main()
{
    sleep(5);
    srand(time(NULL));

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    pthread_t threads[NUMBER_OF_PROGRAMMERS];
    int thread_args[NUMBER_OF_PROGRAMMERS];

    coworking = get_coworking();

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        if ((m_sems[i] = semget(i, 1, IPC_CREAT | 0666)) == -1)
        {
            perror("Error sem get");
            return 1;
        }
    }

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        thread_args[i] = i;
        if (pthread_create(&threads[i], NULL, programmer_work,
                           (void *)&thread_args[i]))
        {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        if (pthread_join(threads[i], NULL))
        {
            perror("pthread_join");
            return 1;
        }
    }

    sigint_handler(0);

    return 0;
}

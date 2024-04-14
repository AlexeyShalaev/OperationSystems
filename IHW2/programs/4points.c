#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUMBER_OF_PROGRAMMERS 3
#define SHARED_MEMORY "/shared_memory"

#define SLEEP usleep((rand() % 3000000) + 1000000) // случайная задержка от 1 до 3 секунд

#define DEBUG_MODE 1

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

/*
 Пул программистов - циклический массив.
 На проверку посылаем следующему в цепочке.
 Т.е. нашу проверяет (index + 1) % NUMBER_OF_PROGRAMMERS
 А мы проверяем (index - 1 + NUMBER_OF_PROGRAMMERS) % NUMBER_OF_PROGRAMMERS.
*/

char *get_programmer_semaphore_name(int i)
{
    char *name = (char *)malloc(100);
    sprintf(name, "/programmer_%d_sem", i);
    return name;
}

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
    sem_t *sem;
    Program program;
    int programmers_to_check[NUMBER_OF_PROGRAMMERS];
} Programmer;

typedef struct
{
    Programmer programmers[NUMBER_OF_PROGRAMMERS];
    char *sem_names[NUMBER_OF_PROGRAMMERS];
    int target_programs_number;
    int programs_id;
    int created_programs_number;
} Coworking;

Coworking *coworking;

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
    log("Release resources\n");
    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        // освобождение ресурсов семафора
        sem_close(coworking->programmers[i].sem);
        sem_unlink(coworking->sem_names[i]);
    }

    // освобождение разделяемой памяти
    munmap(coworking, sizeof(Coworking));
    shm_unlink(SHARED_MEMORY);
    exit(0);
}

Coworking *get_coworking()
{
    Coworking *coworking;

    int shm_fd = shm_open(SHARED_MEMORY, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(Coworking)) == -1)
    {
        perror("ftruncate");
        return 1;
    }

    coworking = mmap(NULL, sizeof(Coworking), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (coworking == MAP_FAILED)
    {
        perror("mmap");
        return 1;
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
    sem_post(coworking->programmers[inspector].sem); // Отправляем сигнал проверяющему программисту

    while (1)
    {
        sem_wait(programmer->sem); // Ожидаем своей очереди

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
                    sem_post(coworking->programmers[surveyed].sem); // Отправляем сигнал программисту, чью работу проверили
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
                        sem_post(coworking->programmers[i].sem); // отправляем сигнал всем, чтобы они завершились
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
            sem_post(coworking->programmers[inspector].sem); // отправляем на проверку
        }
        else if (programmer->program.status == INCORRECT)
        {
            SLEEP;
            programmer->program.status = WRITTEN;
            coworking->programmers[inspector].programmers_to_check[index] = 1;
            log("[%d -> %d] sent fixed program (%d)\n", index, inspector, programmer->program.id);
            sem_post(coworking->programmers[inspector].sem); // отправляем на проверку
        }
    }

    log("[Programmer %d] finished\n", index);

    pthread_exit(NULL);
}

int main()
{
    srand(time(NULL));

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    pthread_t threads[NUMBER_OF_PROGRAMMERS];
    int thread_args[NUMBER_OF_PROGRAMMERS];

    coworking = get_coworking();
    coworking->created_programs_number = 0;
    coworking->target_programs_number = 5; // TODO: parse from args
    coworking->programs_id = coworking->target_programs_number;

    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        coworking->sem_names[i] = get_programmer_semaphore_name(i);

        log("Initializing programmer %d...\n", i);

        coworking->programmers[i].program.status = NOT_EXISTING;
        coworking->programmers[i].sem = sem_open(coworking->sem_names[i], O_CREAT, 0666, 0);
        if (coworking->programmers[i].sem == SEM_FAILED)
        {
            perror("Error sem_open");
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

    log("DONE!\n");

    sigint_handler(0);

    return 0;
}

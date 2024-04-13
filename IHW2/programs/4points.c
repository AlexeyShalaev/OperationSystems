#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>

#define NUMBER_OF_PROGRAMMERS 3

typedef enum
{
    SLEEPING,
    WRITING,
    CHECKING,
    CORRECT,
    INCORRECT
} ProgrammerState;

char *get_programmer_semaphore_name(int i)
{
    char *name = (char *)malloc(100);
    sprintf(name, "/programmer_%d_sem", i);
    return name;
}

char *get_programmer_shared_memory_name(int i)
{
    char *name = (char *)malloc(100);
    sprintf(name, "/programmer_%d_shm", i);
    return name;
}

typedef struct
{
    ProgrammerState state;
    int checked_by;
    int is_correct;
} Program;

typedef struct
{
    sem_t *sem;
    Program program;
} Programmer;

Programmer programmers[NUMBER_OF_PROGRAMMERS];
Program *shared_queues[NUMBER_OF_PROGRAMMERS];
char *sem_names[NUMBER_OF_PROGRAMMERS];
char *shm_names[NUMBER_OF_PROGRAMMERS];
int shm_fds[NUMBER_OF_PROGRAMMERS];

void sigint_handler(int signum)
{
    for (int i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        // освобождение ресурсов семафора
        sem_close(programmers[i].sem);
        sem_unlink(sem_names[i]);
        // освобождение разделяемой памяти
        munmap(shared_queues[i], sizeof(Program));
        close(shm_fds[i]);
        shm_unlink(shm_names[i]);
    }
    exit(-1);
}

void *programmer(void *thread_data)
{
    int index = *(int *)thread_data;
    sem_t *my_sem = programmers[index].sem;
    Program *my_program = &programmers[index].program;

    printf("Programmer %d initialized.\n", index);

    while (1)
    {
        sem_wait(my_sem); // Ожидаем своей очереди

        printf("Programmer %d is working...\n", index);

        switch (my_program->state)
        {
        case SLEEPING:
            // Здесь происходит запись программы
            printf("Programmer %d is writing the program.\n", index);
            usleep(rand() % 1000000); // случайная задержка до 1 секунды
            my_program->state = CHECKING;
            my_program->checked_by = (index + 1) % NUMBER_OF_PROGRAMMERS; // Проверяется следующий по кругу программист
            printf("Programmer %d wrote the program.\n", index);
            sem_post(programmers[my_program->checked_by].sem); // Отправляем сигнал проверяющему программисту
            break;
        case WRITING:
            // Нельзя сюда попасть, так как писать программу может только один программист
            break;
        case CHECKING:
            // Программа проверяется другим программистом
            if (my_program->checked_by == index)
            {
                // Программа проверена, результаты возвращаются
                printf("Programmer %d is checking the program.\n", index);
                usleep(rand() % 1000000); // случайная задержка до 1 секунды
                my_program->state = (rand() % 2 == 0) ? CORRECT : INCORRECT;
                printf("Programmer %d checked the program. Result: %s\n", index, (my_program->state == CORRECT) ? "Correct" : "Incorrect");
                sem_post(programmers[my_program->checked_by].sem); // Отправляем сигнал проверяющему программисту
            }
            break;
        case CORRECT:
            // Писать новую программу
            printf("Programmer %d is writing a new program.\n", index);
            usleep(rand() % 1000000); // случайная задержка до 1 секунды
            my_program->state = SLEEPING;
            sem_post(my_sem); // Отправляем сигнал самому себе для начала работы над новой программой
            break;
        case INCORRECT:
            // Исправить программу и отправить её на проверку тому же программисту
            printf("Programmer %d is correcting the program.\n", index);
            usleep(rand() % 1000000); // случайная задержка до 1 секунды
            my_program->state = CHECKING;
            printf("Programmer %d sends the program for re-checking.\n", index);
            sem_post(programmers[my_program->checked_by].sem); // Отправляем сигнал проверяющему программисту
            break;
        }
    }

    pthread_exit(NULL);
}

int main()
{
    srand(time(NULL));

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    pthread_t threads[NUMBER_OF_PROGRAMMERS];
    int thread_args[NUMBER_OF_PROGRAMMERS];

    // Инициализация программистов
    for (size_t i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        sem_names[i] = get_programmer_semaphore_name(i);
        shm_names[i] = get_programmer_shared_memory_name(i);

        programmers[i].program.state = SLEEPING;
        programmers[i].program.checked_by = -1; // Никто не проверяет программу
        programmers[i].program.is_correct = 0;

        printf("Initializing programmer %d...\n", i);

        programmers[i].sem = sem_open(sem_names[i], O_CREAT, 0666, 0); // Изначально все спят
        if (programmers[i].sem == SEM_FAILED)
        {
            perror("Error sem_open");
            return 1;
        }

        shm_fds[i] = shm_open(shm_names[i], O_CREAT | O_RDWR, 0666);
        if (shm_fds[i] == -1)
        {
            perror("shm_open");
            return 1;
        }
        if (ftruncate(shm_fds[i], sizeof(Program)) == -1)
        {
            perror("ftruncate");
            return 1;
        }
        shared_queues[i] = mmap(NULL, sizeof(Program), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fds[i], 0);
        if (shared_queues[i] == MAP_FAILED)
        {
            perror("mmap");
            return 1;
        }
    }

    for (size_t i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
    {
        // Устанавливаем начальное состояние WRITING для всех программистов
        programmers[i].program.state = WRITING;
        sem_post(programmers[i].sem); // Отправляем сигнал каждому программисту, чтобы он начал писать программу

        thread_args[i] = i;
        if (pthread_create(&threads[i], NULL, programmer, (void *)&thread_args[i]))
        {
            perror("pthread_create");
            return 1;
        }
    }

    // Основной поток ждет завершения всех потоков программистов
    for (size_t i = 0; i < NUMBER_OF_PROGRAMMERS; ++i)
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
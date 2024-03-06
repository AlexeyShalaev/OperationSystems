#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5000
#define FILE_PERMISSIONS 0666
#define DEBUG_MODE 0
#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define FIFO3 "fifo3"

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

int main(int argc, char *argv[])
{
    // Second process
    int fd1, fd2, fd3;
    int attempt = 0;

    do
    {
        sleep(1);
        fd1 = open(FIFO1, O_RDONLY);
        fd2 = open(FIFO2, O_WRONLY);
        fd3 = open(FIFO3, O_WRONLY);
        ++attempt;
        if (attempt > 5)
        {
            perror("[HANDLER]: Opening FIFO error");
            exit(EXIT_FAILURE);
        }
    } while (fd1 == -1 || fd2 == -1 || fd3 == -1);

    log("Second process\n");

    char buffer[BUFFER_SIZE * 2];
    ssize_t bytes_read;

    // Чтение данных из первого канала
    bytes_read = read(fd1, buffer, BUFFER_SIZE * 2);
    if (bytes_read == -1)
    {
        perror("[HANDLER]: Read error");
        exit(EXIT_FAILURE);
    }

    log("[Second process] Read %d bytes from first pipe\n", bytes_read);

    // Обработка данных
    char unique_in_first[BUFFER_SIZE];
    char unique_in_second[BUFFER_SIZE];
    int unique_in_first_index = 0;
    int unique_in_second_index = 0;
    char *first_string = buffer;
    char *second_string = buffer + strlen(buffer) + 1;

    for (int i = 0; i < strlen(first_string); ++i)
    {
        if (strchr(second_string, first_string[i]) == NULL)
        {
            if (strchr(unique_in_first, first_string[i]) == NULL)
            {
                unique_in_first[unique_in_first_index++] = first_string[i];
            }
        }
    }

    for (int i = 0; i < strlen(second_string); ++i)
    {
        if (strchr(first_string, second_string[i]) == NULL)
        {
            if (strchr(unique_in_second, second_string[i]) == NULL)
            {
                unique_in_second[unique_in_second_index++] = second_string[i];
            }
        }
    }

    // Записываем объединенные данные в канал
    if (write(fd2, unique_in_first, strlen(unique_in_first) + 1) == -1)
    {
        perror("Write error");
        exit(EXIT_FAILURE);
    }

    log("[Second process] Write data to first pipe: '%s'\n", unique_in_first);

    if (write(fd3, unique_in_second, strlen(unique_in_second) + 1) == -1)
    {
        perror("Write error");
        exit(EXIT_FAILURE);
    }

    log("[Second process] Write data to second pipe: '%s'\n", unique_in_second);

    close(fd1);
    close(fd2);
    close(fd3);

    log("Second process finished\n");

    exit(EXIT_SUCCESS);

    // Удаление именованных каналов
    if (unlink(FIFO1) == -1 || unlink(FIFO2) == -1 || unlink(FIFO3) == -1)
    {
        perror("Unlink error");
        exit(EXIT_FAILURE);
    }

    return 0;
}

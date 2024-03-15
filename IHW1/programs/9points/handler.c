#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 128
#define FILE_PERMISSIONS 0666
#define DEBUG_MODE 0
#define FIFO1 "fifo1"
#define FIFO2 "fifo2"
#define FIFO3 "fifo3"
#define ASCII_SIZE 128

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
            perror("[Second process]: Opening FIFO error");
            exit(EXIT_FAILURE);
        }
    } while (fd1 == -1 || fd2 == -1 || fd3 == -1);

    log("Second process\n");

    char buffer[BUFFER_SIZE * 2 + 2];
    ssize_t bytes_read;

    int ascii_map_first[ASCII_SIZE] = {0};
    int ascii_map_second[ASCII_SIZE] = {0};

    // Чтение данных из первого канала
    do {
        bytes_read = read(fd1, buffer, BUFFER_SIZE * 2 + 2);
        
        if (bytes_read == -1)
        {
            perror("[Second process]: Read error");
            exit(EXIT_FAILURE);
        }

        log("[Second process] Read %d bytes from first pipe\n", bytes_read);    
        //log("[Second process] Read 1st data from first pipe %s\n", buffer);
        //log("[Second process] Read 2nd data from first pipe %s\n", buffer + strlen(buffer) + 1);
        log("[Second process] 1st data length %d\n", strlen(buffer));
        log("[Second process] 2nd data length %d\n", strlen(buffer + strlen(buffer) + 1));

        for (int i = 0; i < strlen(buffer); ++i)
        {
            if(ascii_map_first[buffer[i]] == 0){
                log("[Second process] ASCII code 1: %d\n", buffer[i]);
            }

            ascii_map_first[buffer[i]] = 1;
        }

        for (int i = 0; i < strlen(buffer + strlen(buffer) + 1); ++i)
        {
            if(ascii_map_second[buffer[strlen(buffer) + 1 + i]] == 0){
                log("[Second process] ASCII code 2: %d\n", buffer[strlen(buffer) + 1 + i]);
            }

            ascii_map_second[buffer[strlen(buffer) + 1 + i]] = 1;
        }
    } while (bytes_read == BUFFER_SIZE * 2 + 2);

    // Обработка данных
    char unique_in_first[ASCII_SIZE];
    char unique_in_second[ASCII_SIZE];
    int unique_in_first_index = 0;
    int unique_in_second_index = 0;

    for (int i = 0; i < ASCII_SIZE; ++i)
    {
        log("[Second process] solver ascii {%c}: %d-%d\n", (char)i, ascii_map_first[i], ascii_map_second[i]);
        if (ascii_map_first[i] && !ascii_map_second[i])
        {
            unique_in_first[unique_in_first_index++] = (char)i;
            //log("[Second process] solver ascii 1: %c\n", (char)i);
        }
        else if (ascii_map_second[i] && !ascii_map_first[i])
        {
            unique_in_second[unique_in_second_index++] = (char)i;
            //log("[Second process] solver ascii 2: %c\n", (char)i);
        }
    }

    unique_in_first[unique_in_first_index] = '\0';
    unique_in_second[unique_in_second_index] = '\0';
    
    log("[Second process]: Unique in first: %s\n", unique_in_first);
    log("[Second process]: Unique in second: %s\n", unique_in_second);
    log("[Second process]: Unique in first length: %d\n", strlen(unique_in_first));
    log("[Second process]: Unique in first length: %d\n", strlen(unique_in_second));

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

    // Удаление именованных каналов
    if (unlink(FIFO1) == -1 || unlink(FIFO2) == -1 || unlink(FIFO3) == -1)
    {
        perror("Unlink error");
        exit(EXIT_FAILURE);
    }

    return 0;
}

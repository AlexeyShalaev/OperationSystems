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

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <input_file1> <input_file2> <output_file1> <output_file2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Создаём именованные каналы
    // mknod(FIFO1, S_IFIFO | 0666, 0);
    // mknod(FIFO2, S_IFIFO | 0666, 0);
    // mknod(FIFO3, S_IFIFO | 0666, 0);

    // Создание именованных каналов
    if (access(FIFO1, F_OK) == -1)
    {
        if (mkfifo(FIFO1, FILE_PERMISSIONS) == -1)
        {
            perror("[MANAGER]: FIFO1 creation error");
            exit(EXIT_FAILURE);
        }
    }

    if (access(FIFO2, F_OK) == -1)
    {
        if (mkfifo(FIFO2, FILE_PERMISSIONS) == -1)
        {
            perror("[MANAGER]: FIFO2 creation error");
            exit(EXIT_FAILURE);
        }
    }

    if (access(FIFO3, F_OK) == -1)
    {
        if (mkfifo(FIFO3, FILE_PERMISSIONS) == -1)
        {
            perror("[MANAGER]: FIFO3 creation error");
            exit(EXIT_FAILURE);
        }
    }

    int fd1 = open(FIFO1, O_WRONLY);
    if (fd1 == -1)
    {
        perror("[MANAGER]: Opening FIFO1 error");
        exit(EXIT_FAILURE);
    }

    log("First process\n");

    // Открытие файлов для чтения и записи
    int input_file1 = open(argv[1], O_RDONLY);
    int input_file2 = open(argv[2], O_RDONLY);

    if (input_file1 == -1 || input_file2 == -1)
    {
        perror("File error");
        exit(EXIT_FAILURE);
    }

    // Чтение данных из файлов и запись в fd1
    char input_buffer[BUFFER_SIZE * 2 + 1];
    ssize_t bytes_read_1, bytes_read_2, bytes_read; 

    do{
        bytes_read_1 = read(input_file1, input_buffer, BUFFER_SIZE);
        if (bytes_read_1 == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }
        input_buffer[bytes_read_1] = '\0';

        bytes_read_2 = read(input_file2, input_buffer + strlen(input_buffer) + 1, BUFFER_SIZE);
        if (bytes_read_2 == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }
        input_buffer[strlen(input_buffer) + bytes_read_2 + 1] = '\0';

        log("[First process] Read data from files (%d): %s\n", strlen(input_buffer), input_buffer);
        log("[First process] Read data from files (%d): %s\n", strlen(input_buffer + strlen(input_buffer) + 1), input_buffer + strlen(input_buffer) + 1);

        if (write(fd1, input_buffer, bytes_read_1 + bytes_read_2 + 2) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[First process] Wrote %d bytes at pipe\n", bytes_read_1 + bytes_read_2 + 2);
    } while(bytes_read_1 == BUFFER_SIZE || bytes_read_2 == BUFFER_SIZE);

    // Закрытие файлов и каналов
    close(input_file1);
    close(input_file2);
    close(fd1);

    // ------------------------------ First process after second ------------------------------

    int fd2 = open(FIFO2, O_RDONLY);
    int fd3 = open(FIFO3, O_RDONLY);
    if (fd2 == -1 || fd3 == -1)
    {
        perror("Opening FIFO error");
        exit(EXIT_FAILURE);
    }

    // Открытие файлов для записи
    int output_file1 = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
    int output_file2 = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);

    if (output_file1 == -1 || output_file2 == -1)
    {
        perror("File error");
        exit(EXIT_FAILURE);
    }

    log("[First process] Opened files for writing\n");

    // Чтение данных из второго канала
    char buffer[BUFFER_SIZE];

    log("[First process] Reading from pipe\n");

    bytes_read = read(fd2, buffer, BUFFER_SIZE);

    log("[First process] Read %ld bytes from pipe\n", bytes_read);

    if (bytes_read == -1)
    {
        perror("Read error");
        exit(EXIT_FAILURE);
    }

    log("[First process] Read data from previous pipe: '%s'\n", buffer);

    // Запись данных в файлы
    if (write(output_file1, buffer, strlen(buffer)) == -1)
    {
        perror("Write error");
        exit(EXIT_FAILURE);
    }

    log("[First process] Write data to file1: '%s'\n", buffer);

    bytes_read = read(fd3, buffer, BUFFER_SIZE);
    if (bytes_read == -1)
    {
        perror("Read error");
        exit(EXIT_FAILURE);
    }

    log("[First process] Read data from previous pipe: '%s'\n", buffer);

    if (write(output_file2, buffer, strlen(buffer)) == -1)
    {
        perror("Write error");
        exit(EXIT_FAILURE);
    }

    log("[First process] Write data to file2: %s\n", buffer);

    // Закрытие файлов и каналов
    close(output_file1);
    close(output_file2);
    close(fd2);
    close(fd3);

    log("First process finished\n");

    // Удаление именованных каналов
    if (unlink(FIFO1) == -1 || unlink(FIFO2) == -1 || unlink(FIFO3) == -1)
    {
        perror("Unlink error");
        exit(EXIT_FAILURE);
    }

    return 0;
}

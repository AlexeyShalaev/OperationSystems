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
    if (mkfifo(FIFO1, FILE_PERMISSIONS) == -1 || mkfifo(FIFO2, FILE_PERMISSIONS) == -1 || mkfifo(FIFO3, FILE_PERMISSIONS) == -1)
    {
        perror("FIFO creation error");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    // First process
    if (pid == -1)
    {
        perror("Fork error");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        int fd1 = open(FIFO1, O_WRONLY);
        if (fd1 == -1)
        {
            perror("Opening FIFO1 error");
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

        // Чтение данных из файлов и запись в fifo1
        char input_buffer[BUFFER_SIZE * 2];
        ssize_t bytes_read;

        bytes_read = read(input_file1, input_buffer, BUFFER_SIZE);
        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }
        input_buffer[bytes_read] = '\0';

        bytes_read = read(input_file2, input_buffer + strlen(input_buffer) + 1, BUFFER_SIZE);
        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }
        input_buffer[strlen(input_buffer) + bytes_read + 1] = '\0';

        log("[First process] Read data from files (%d): %s\n", strlen(input_buffer), input_buffer);
        log("[First process] Read data from files (%d): %s\n", strlen(input_buffer + strlen(input_buffer) + 1), input_buffer + strlen(input_buffer) + 1);

        if (write(fd1, input_buffer, strlen(input_buffer) + bytes_read + 2) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[First process] Wrote %d bytes at fifo\n", strlen(input_buffer) + bytes_read + 2);

        // Закрытие файлов и каналов
        close(input_file1);
        close(input_file2);
        close(fd1);

        log("First process finished\n");

        exit(EXIT_SUCCESS);
    }

    // Second process
    pid = fork();
    if (pid == -1)
    {
        perror("Fork error");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        int fd1 = open(FIFO1, O_RDONLY);
        int fd2 = open(FIFO2, O_WRONLY);
        int fd3 = open(FIFO3, O_WRONLY);
        if (fd1 == -1 || fd2 == -1 || fd3 == -1)
        {
            perror("Opening FIFO error");
            exit(EXIT_FAILURE);
        }

        log("Second process\n");

        char buffer[BUFFER_SIZE * 2];
        ssize_t bytes_read;

        // Чтение данных из первого канала
        bytes_read = read(fd1, buffer, BUFFER_SIZE * 2);
        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        log("[Second process] Read %d bytes from first fifo\n", bytes_read);

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

        log("[Second process] Write data to first fifo: '%s'\n", unique_in_first);

        if (write(fd3, unique_in_second, strlen(unique_in_second) + 1) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Second process] Write data to second fifo: '%s'\n", unique_in_second);

        close(fd1);
        close(fd2);
        close(fd3);

        log("Second process finished\n");

        exit(EXIT_SUCCESS);
    }

    // Third process
    pid = fork();
    if (pid == -1)
    {
        perror("Fork error");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        int fd2 = open(FIFO2, O_RDONLY);
        int fd3 = open(FIFO3, O_RDONLY);
        if (fd2 == -1 || fd3 == -1)
        {
            perror("Opening FIFO error");
            exit(EXIT_FAILURE);
        }

        log("Third process\n");

        // Открытие файлов для записи
        int output_file1 = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
        int output_file2 = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);

        if (output_file1 == -1 || output_file2 == -1)
        {
            perror("File error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Opened files for writing\n");

        // Чтение данных из второго канала
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read;

        log("[Third process] Reading from fifo\n");

        bytes_read = read(fd2, buffer, BUFFER_SIZE);

        log("[Third process] Read %ld bytes from fifo\n", bytes_read);

        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Read data from previous fifo: '%s'\n", buffer);

        // Запись данных в файлы
        if (write(output_file1, buffer, strlen(buffer)) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Write data to file1: '%s'\n", buffer);

        bytes_read = read(fd3, buffer, BUFFER_SIZE);
        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Read data from previous fifo: '%s'\n", buffer);

        if (write(output_file2, buffer, strlen(buffer)) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Write data to file2: %s\n", buffer);

        // Закрытие файлов и каналов
        close(output_file1);
        close(output_file2);
        close(fd2);
        close(fd3);

        log("Third process finished\n");

        exit(EXIT_SUCCESS);
    }

    // Ожидание завершения всех дочерних процессов
    while (wait(NULL) != -1)
        ;

    // Удаление именованных каналов
    if (unlink(FIFO1) == -1 || unlink(FIFO2) == -1 || unlink(FIFO3) == -1)
    {
        perror("Unlink error");
        exit(EXIT_FAILURE);
    }

    return 0;
}

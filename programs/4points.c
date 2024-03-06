#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 5000
#define FILE_PERMISSIONS 0666
#define DEBUG_MODE 1

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

    // Создание неименованных каналов
    int fd1[2];
    int fd2_1[2], fd2_2[2];

    if (pipe(fd1) == -1 || pipe(fd2_1) == -1 || pipe(fd2_2) == -1)
    {
        perror("Pipe error");
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
        close(fd1[0]);
        close(fd2_1[0]);
        close(fd2_1[1]);
        close(fd2_2[0]);
        close(fd2_2[1]);

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

        if (write(fd1[1], input_buffer, strlen(input_buffer) + bytes_read + 2) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[First process] Wrote %d bytes at pipe\n", strlen(input_buffer) + bytes_read + 2);

        // Закрытие файлов и каналов
        close(input_file1);
        close(input_file2);
        close(fd1[1]);

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
        close(fd1[1]);
        close(fd2_1[0]);
        close(fd2_2[0]);

        log("Second process\n");

        char buffer[BUFFER_SIZE * 2];
        ssize_t bytes_read;

        // Чтение данных из первого канала
        bytes_read = read(fd1[0], buffer, BUFFER_SIZE * 2);
        if (bytes_read == -1)
        {
            perror("Read error");
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
        if (write(fd2_1[1], unique_in_first, strlen(unique_in_first) + 1) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Second process] Write data to first pipe: '%s'\n", unique_in_first);

        if (write(fd2_2[1], unique_in_second, strlen(unique_in_second) + 1) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Second process] Write data to second pipe: '%s'\n", unique_in_second);

        close(fd1[0]);
        close(fd2_1[1]);
        close(fd2_2[1]);

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
        close(fd1[0]);
        close(fd1[1]);
        close(fd2_1[1]);
        close(fd2_2[1]);

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

        log("[Third process] Reading from pipe\n");

        bytes_read = read(fd2_1[0], buffer, BUFFER_SIZE);

        log("[Third process] Read %ld bytes from pipe\n", bytes_read);

        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Read data from previous pipe: '%s'\n", buffer);

        // Запись данных в файлы
        if (write(output_file1, buffer, strlen(buffer)) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Write data to file1: '%s'\n", buffer);

        bytes_read = read(fd2_2[0], buffer, BUFFER_SIZE);
        if (bytes_read == -1)
        {
            perror("Read error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Read data from previous pipe: '%s'\n", buffer);

        if (write(output_file2, buffer, strlen(buffer)) == -1)
        {
            perror("Write error");
            exit(EXIT_FAILURE);
        }

        log("[Third process] Write data to file2: %s\n", buffer);

        // Закрытие файлов и каналов
        close(output_file1);
        close(output_file2);
        close(fd2_1[0]);
        close(fd2_2[0]);

        log("Third process finished\n");

        exit(EXIT_SUCCESS);
    }

    // Родительский процесс
    close(fd1[0]);   // Закрыть чтение из fd1
    close(fd1[1]);   // Закрыть запись в fd1
    close(fd2_1[0]); // Закрыть чтение из fd2_1
    close(fd2_1[1]); // Закрыть запись в fd2_1
    close(fd2_2[0]); // Закрыть чтение из fd2_2
    close(fd2_2[1]); // Закрыть запись в fd2_2

    // Ожидание завершения всех дочерних процессов
    while (wait(NULL) != -1)
        ;

    return 0;
}

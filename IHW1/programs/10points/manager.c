#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 128
#define ASCII_SIZE 128
#define FILE_PERMISSIONS 0666
#define DEBUG_MODE 0

// Определение макроса log
#if DEBUG_MODE
#define log(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define log(fmt, ...)
#endif

int main(int argc, char *argv[])
{   
    log("[MANAGER] started\n");

    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s <input_file1> <input_file2> <output_file1> <output_file2>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int msqid;
    //char pathname[] = "./programs/10points/handler.c";
    char pathname[] = ".";
    key_t key;
    int len, maxlen;

    // структура запроса клиента
    struct mymsgbuf1
    {
        long mtype; // тип сообщения (1)
        struct
        {
            pid_t client_pid;                 // идентификатор клиента
            char input_buffer_1[BUFFER_SIZE]; // 1 файл
            size_t input_buffer_1_size;       // Размер данных в буфере 1
            char input_buffer_2[BUFFER_SIZE]; // 2 файл
            size_t input_buffer_2_size;       // Размер данных в буфере 2
        } request;
    } request_buf;

    // структура ответа сервера
    struct mymsgbuf2
    {
        long mtype; // тип сообщения (pid)
        struct
        {
            char result_1[ASCII_SIZE]; // 1 ответ
            char result_2[ASCII_SIZE]; // 2 ответ
        } response;
    } response_buf;

    log("[MANAGER] getting key\n");

    if ((key = ftok(pathname, 0)) < 0)
    {
        printf("[MANAGER] can't generate key\n");
        exit(-1);
    }

    log("[MANAGER] getting msqid\n");

    if ((msqid = msgget(key, FILE_PERMISSIONS | IPC_CREAT)) < 0)
    {
        printf("[MANAGER] can't get msqid\n");
        exit(-1);
    }

    request_buf.mtype = 1;
    request_buf.request.client_pid = getpid();

    // Открытие файлов для чтения и записи
    int input_file1 = open(argv[1], O_RDONLY);
    int input_file2 = open(argv[2], O_RDONLY);

    if (input_file1 == -1 || input_file2 == -1)
    {
        perror("[MANAGER] File error");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read_1, bytes_read_2;

    log("[MANAGER] reading files\n");

    do
    {
        bytes_read_1 = read(input_file1, request_buf.request.input_buffer_1, BUFFER_SIZE);
        if (bytes_read_1 == -1)
        {
            perror("[MANAGER] Read error");
            exit(EXIT_FAILURE);
        }
        request_buf.request.input_buffer_1[bytes_read_1] = '\0';
        request_buf.request.input_buffer_1_size = bytes_read_1;

        bytes_read_2 = read(input_file2, request_buf.request.input_buffer_2, BUFFER_SIZE);
        if (bytes_read_2 == -1)
        {
            perror("[MANAGER] Read error");
            exit(EXIT_FAILURE);
        }
        request_buf.request.input_buffer_2[bytes_read_2] = '\0';
        request_buf.request.input_buffer_2_size = bytes_read_2;

        len = sizeof(request_buf.request);

        if (msgsnd(msqid, (struct msgbuf *)&request_buf, len, 0) < 0)
        {
            printf("can't send message to queue\n");
            msgctl(msqid, IPC_RMID, (struct msqid_ds *)NULL);
            exit(-1);
        }

        log("[MANAGER] bytes_read_1: %d\n", bytes_read_1);
        log("[MANAGER] bytes_read_2: %d\n", bytes_read_2);

    } while (bytes_read_1 == BUFFER_SIZE || bytes_read_2 == BUFFER_SIZE);

    // Закрытие файлов и каналов
    close(input_file1);
    close(input_file2);

    // ------------------------------ First process after second ------------------------------

    log("[MANAGER] receiving messages\n");

    maxlen = sizeof(response_buf.response);
    if (len = msgrcv(msqid, (struct msgbuf *)&response_buf, maxlen, getpid(), 0) < 0)
    {
        printf("[MANAGER] can't receive message from queue\n");
        exit(-1);
    }

    log("[MANAGER] result_1: %s\n", response_buf.response.result_1);
    log("[MANAGER] result_2: %s\n", response_buf.response.result_2);

    log("[MANAGER] writing files\n");

    // Открытие файлов для записи
    int output_file1 = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);
    int output_file2 = open(argv[4], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERMISSIONS);

    if (output_file1 == -1 || output_file2 == -1)
    {
        perror("[MANAGER] File error");
        exit(EXIT_FAILURE);
    }

    // Запись данных в файлы
    if (write(output_file1, response_buf.response.result_1, strlen(response_buf.response.result_1)) == -1)
    {
        perror("[MANAGER] Write error");
        exit(EXIT_FAILURE);
    }

    if (write(output_file2, response_buf.response.result_2, strlen(response_buf.response.result_2)) == -1)
    {
        perror("[MANAGER] Write error");
        exit(EXIT_FAILURE);
    }

    // Закрытие файлов и каналов
    close(output_file1);
    close(output_file2);

    log("[MANAGER] finished\n");

    exit(EXIT_SUCCESS);
}

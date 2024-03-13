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

int main()
{
    log("[HANDLER] started\n");

    int msqid;
    //char pathname[] = "./programs/10points/manager.c";
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

    log("[HANDLER] getting key\n");

    if ((key = ftok(pathname, 0)) < 0)
    {
        printf("[HANDLER] can't generate key\n");
        exit(-1);
    }

    log("[HANDLER] getting msqid\n");

    if ((msqid = msgget(key, FILE_PERMISSIONS | IPC_CREAT)) < 0)
    {
        printf("[HANDLER] can't get msqid\n");
        exit(-1);
    }

    int ascii_map_first[ASCII_SIZE] = {0};
    int ascii_map_second[ASCII_SIZE] = {0};

    log("[HANDLER] receiving messages\n");

    do
    {
        maxlen = sizeof(request_buf.request);
        if (len = msgrcv(msqid, (struct msgbuf *)&request_buf, maxlen, 1, 0) < 0)
        {
            printf("[HANDLER] can't receive message from queue\n");
            exit(-1);
        }

        log("[HANDLER] received message\n");
        //log("[HANDLER] input_buffer_1 (%d=%d): %s\n", strlen(request_buf.request.input_buffer_1), request_buf.request.input_buffer_1_size, request_buf.request.input_buffer_1);
        //log("[HANDLER] input_buffer_2 (%d=%d): %s\n", strlen(request_buf.request.input_buffer_2), request_buf.request.input_buffer_2_size, request_buf.request.input_buffer_2);

        for (int i = 0; i < request_buf.request.input_buffer_1_size; ++i)
        {
            ascii_map_first[request_buf.request.input_buffer_1[i]] = 1;
        }

        for (int i = 0; i < request_buf.request.input_buffer_2_size; ++i)
        {
            ascii_map_second[request_buf.request.input_buffer_2[i]] = 1;
        }

    } while (request_buf.request.input_buffer_1_size == BUFFER_SIZE || request_buf.request.input_buffer_2_size == BUFFER_SIZE);

    log("[HANDLER] processing data\n");

    // Обработка данных
    int unique_in_first_index = 0;
    int unique_in_second_index = 0;

    for (int i = 0; i < ASCII_SIZE; ++i)
    {
        if (ascii_map_first[i] && !ascii_map_second[i])
        {
            response_buf.response.result_1[unique_in_first_index++] = (char)i;
        }
        else if (ascii_map_second[i] && !ascii_map_first[i])
        {
            response_buf.response.result_2[unique_in_second_index++] = (char)i;
        }
    }

    response_buf.response.result_1[unique_in_first_index] = '\0';
    response_buf.response.result_2[unique_in_second_index] = '\0';

    log("[HANDLER] result_1: %s\n", response_buf.response.result_1);
    log("[HANDLER] result_2: %s\n", response_buf.response.result_2);

    response_buf.mtype = request_buf.request.client_pid;
    len = sizeof(response_buf.response);

    log("[HANDLER] sending response\n");

    if (msgsnd(msqid, (struct msgbuf *)&response_buf, len, 0) < 0)
    {
        printf("[HANDLER] can't send message to queue\n");
        msgctl(msqid, IPC_RMID, (struct msqid_ds *)NULL);
        exit(-1);
    }

    // msgctl(msqid, IPC_RMID, (struct msqid_ds *) NULL);

    log("[HANDLER] finished\n");

    return 0;
}
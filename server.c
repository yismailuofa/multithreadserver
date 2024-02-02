#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"
#include "timer.h"

char **data;
pthread_rwlock_t *locks;

int time_index = 0;
double times[COM_NUM_REQUEST];
pthread_mutex_t time_lock;

void *ServerAccess(void *args)
{
    int clientFileDescriptor = (intptr_t)args;
    char str[COM_BUFF_SIZE];

    read(clientFileDescriptor, str, COM_BUFF_SIZE - 1);
    str[COM_BUFF_SIZE - 1] = '\0';

    ClientRequest req;
    ParseMsg(str, &req);
    memset(str, 0, COM_BUFF_SIZE - 1);

    double start, end;
    GET_TIME(start);
    if (req.is_read)
    {
        pthread_rwlock_rdlock(&locks[req.pos]);
        getContent(str, req.pos, data);
        pthread_rwlock_unlock(&locks[req.pos]);
    }
    else
    {
        pthread_rwlock_wrlock(&locks[req.pos]);
        setContent(req.msg, req.pos, data);
        pthread_rwlock_unlock(&locks[req.pos]);

        pthread_rwlock_rdlock(&locks[req.pos]);
        getContent(str, req.pos, data);
        pthread_rwlock_unlock(&locks[req.pos]);
    }
    GET_TIME(end);

    double elapsed = end - start;

    pthread_mutex_lock(&time_lock);
    times[time_index++] = elapsed;
    pthread_mutex_unlock(&time_lock);

    write(clientFileDescriptor, str, COM_BUFF_SIZE);
    close(clientFileDescriptor);
    return NULL;
}

int main(int argc, char *argv[])
{

    // Command Line Arguments
    if (argc < 4)
    {
        fprintf(stderr, "usage: %s <n: size of the array> <server ip> <server port>\n", argv[0]);
        exit(0);
    }
    int n = atoi(argv[1]);
    const char *ip = argv[2];
    in_port_t port = atoi(argv[3]);

    // Initialize data and locks
    data = (char **)malloc(n * sizeof(char *));
    locks = (pthread_rwlock_t *)malloc(n * sizeof(pthread_rwlock_t));
    pthread_mutex_init(&time_lock, NULL);

    for (int i = 0; i < n; i++)
    {
        pthread_rwlock_init(&locks[i], NULL);
        data[i] = (char *)malloc(COM_BUFF_SIZE * sizeof(char));
        sprintf(data[i], "String %d: the initial value", i);
    }

    // Create the socket
    struct sockaddr_in sock_var;
    int serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    int clientFileDescriptor;
    int i;
    pthread_t t[COM_NUM_REQUEST];

    sock_var.sin_addr.s_addr = inet_addr(ip);
    sock_var.sin_port = port;
    sock_var.sin_family = AF_INET;

    // Allow the socket to be reused
    setsockopt(serverFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (bind(serverFileDescriptor, (struct sockaddr *)&sock_var, sizeof(sock_var)) >= 0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor, COM_NUM_REQUEST);
        while (1) // loop infinity
        {
            for (i = 0; i < COM_NUM_REQUEST; i++) // can support COM_NUM_REQUESTs clients at a time
            {
                clientFileDescriptor = accept(serverFileDescriptor, NULL, NULL);
                pthread_create(&t[i], NULL, ServerAccess, (void *)(long)clientFileDescriptor);
            }

            for (i = 0; i < COM_NUM_REQUEST; i++)
            {
                pthread_join(t[i], NULL);
            }

            saveTimes(times, COM_NUM_REQUEST);

            time_index = 0;
        }
        close(serverFileDescriptor);
    }
    else
    {
        printf("socket creation failed\n");
    }

    // Close the socket
    close(serverFileDescriptor);

    // Free data and locks
    for (int i = 0; i < n; i++)
    {
        pthread_rwlock_destroy(&locks[i]);
        free(data[i]);
    }

    free(data);
    free(locks);

    return 0;
}
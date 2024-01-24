#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "common.h"

char **data;
void *ServerAccess(void *args)
{
    int clientFileDescriptor = (int)args;
    char str[COM_BUFF_SIZE];

    read(clientFileDescriptor, str, COM_BUFF_SIZE);
    printf("reading from client:%s\n", str);
    ClientRequest req;
    char *content;
    ParseMsg(str, &req);
    if (req.is_read)
    {
        setContent(req.msg, req.pos, data);
        content = "Success";
    }
    else
    {
        getContent(content, req.pos, data);
    }
    write(clientFileDescriptor, content, COM_BUFF_SIZE);
    close(clientFileDescriptor);
    return NULL;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in sock_var;
    int serverFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    int clientFileDescriptor;
    int i;
    pthread_t t[COM_NUM_REQUEST];

    sock_var.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_var.sin_port = 3000;
    sock_var.sin_family = AF_INET;
    if (bind(serverFileDescriptor, (struct sockaddr *)&sock_var, sizeof(sock_var)) >= 0)
    {
        printf("socket has been created\n");
        listen(serverFileDescriptor, 2000);
        while (1) // loop infinity
        {
            for (i = 0; i < COM_NUM_REQUEST; i++) // can support COM_NUM_REQUESTs clients at a time
            {
                clientFileDescriptor = accept(serverFileDescriptor, NULL, NULL);
                printf("Connected to client %d\n", clientFileDescriptor);
                pthread_create(&t[i], NULL, ServerAccess, (void *)(long)clientFileDescriptor);
            }
        }
        close(serverFileDescriptor);
    }
    else
    {
        printf("socket creation failed\n");
    }
    return 0;
}
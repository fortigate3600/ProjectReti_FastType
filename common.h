#ifndef COMMON_H
#define COMMON_H

#define handle_error(string) {perror(string); exit(EXIT_FAILURE); }

#define MAX_CONN_QUEUE  10 
#define SERVER_ADDRESS  "127.0.0.2"
#define SERVER_PORT     2015
#define ADMIN           0 //mettere a uno per debuggare



typedef struct msg{
    char payload[1000];
    int flagReady;
    double time;
    char frase[1000];
}msg;

#endif

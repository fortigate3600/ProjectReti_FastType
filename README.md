# ProjectReti
FAST TYPE

Scambio di messaggi tramite socket
Il server accetta coppie di client e li mette in una partita: game(client1,client2)

lo scambio di send e recv avviene attraverso una struct msg: 
typedef struct msg{
    char payload[1000];
    int flagReady;
    double time;
    char frase[1000];
}msg;

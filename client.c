#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "common.h"

int main(int argc, char* argv[]) {
    int ret, game=1;

    int socket_desc;
    struct sockaddr_in server_addr = {0};

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc < 0)
        handle_error("errore creazione socket");
    if(ADMIN) fprintf(stderr, "Socket creata...\n");

    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT);
    
    ret = connect(socket_desc, (struct sockaddr*) &server_addr, sizeof(struct sockaddr_in));
    if (ret < 0)
        handle_error("errore nella connection");
    if(ADMIN) fprintf(stderr, "Connection connessione stabilita\n");

    msg* ricezione=malloc(sizeof(msg));
    msg* invio=malloc(sizeof(msg));

    //connection_ handler
    ret = recv(socket_desc, ricezione, sizeof(msg), 0);
    if (ret<0) handle_error("errore recv 1");
    fprintf(stderr,"%s\n", ricezione->payload);

    fprintf(stderr,"aspettiamo si connetti l'altro giocatore\n");

    // GAME
    do{
        
        ret = recv(socket_desc, ricezione, sizeof(msg), 0);
        if (ret < 0)
            handle_error("errore recv 2");
        fprintf(stderr,"%s", ricezione->payload);//entrambi i giocatori connessi


        char input[300];
        fprintf(stderr,"SEI PRONTO, VUOI GIOCARE? scrivi y\n");
        do
            fgets(input, sizeof(input), stdin);    
        while(strcmp(input,"y\n") && strcmp(input,"n\n"));
        fprintf(stderr,"aspettiamo l'altro giocatore sia pronto\n");


        invio->flagReady=0;
        if(strcmp(input,"y\n")==0)
            invio->flagReady=1;

        ret=send(socket_desc, invio, sizeof(msg), 0);//mando che sono pronto
        if (ret < 0)
            handle_error("errore send 1");


    
        ret = recv(socket_desc, ricezione, sizeof(msg), 0);//frase da scrivere e entrambi i giocatori pronti
        if (ret < 0)
            handle_error("errore recv 3");


        if(ricezione->flagReady)
            fprintf(stderr,"avversario pronto\n");
        else {
            fprintf(stderr,"l'altro giocatore se ne è andato\n");
            break;
        }



        fprintf(stderr,"%s%s", ricezione->payload,ricezione->frase);
        fprintf(stderr,"3\n");sleep(1);
        fprintf(stderr,"2\n");sleep(1);
        fprintf(stderr,"1\n");sleep(1);
        fprintf(stderr,"VIA!!!\n");
        


        struct timeval inizio, fine;
        gettimeofday(&inizio, NULL);

        int firstTry=1;
        do{
            if(!firstTry) fprintf(stderr,"frase sbagliata ritenta, veloce!\n");
            fgets(input, sizeof(input), stdin);
            firstTry=0;
        }
        while(strcmp(input,ricezione->frase));
        fprintf(stderr,"frase corretta\n");

        gettimeofday(&fine, NULL);
        double tempo_trascorso = (fine.tv_sec - inizio.tv_sec); // Secondi
        tempo_trascorso += (fine.tv_usec - inizio.tv_usec) / 1000000.0; //micro in secondi
        tempo_trascorso*=1000;//in ms

        fprintf(stderr,"ci hai messo: %.3f s\n",tempo_trascorso/1000);
        invio->time=tempo_trascorso;

        fprintf(stderr,"aspettiamo che il tuo avversario finisca\n");

        ret=send(socket_desc, invio, sizeof(msg), 0);
        if (ret < 0)
            handle_error("errore send 2");

        //chi ha vinto?
        ret = recv(socket_desc, ricezione, sizeof(msg), 0);
        if (ret < 0)
            handle_error("errore recv 4");
        
        if(ricezione->time!=0)
            fprintf(stderr,"l'altro giocatore ci ha messo %.3f s\n", ricezione->time/1000);
        else{
            fprintf(stderr,"l'altro giocatore non ha finito la frase\n");
            break;
            }
        fprintf(stderr,"%s", ricezione->payload);


        fprintf(stderr,"vuoi il rematch? scrivi y\n");
        fgets(input, sizeof(input), stdin); 

        if(strcmp(input,"y\n")==0){
            invio->flagReady=1;
        } else {
            invio->flagReady=0;
        }
        send(socket_desc, invio, sizeof(msg), 0);
        if (ret < 0)
            handle_error("errore send 3");
            
        if(invio->flagReady==0){//se non voglio giocare non mi interessa cosa vuole fare l'altro
            game=0;
        } else {//voglio giocare aspetto risposta avversario
            fprintf(stderr,"aspettando risposta dall'avversario\n");
            ret = recv(socket_desc, ricezione, sizeof(msg), 0);
            if (ret < 0)
                handle_error("errore recv 5");

            if(ricezione->flagReady==0){
                game=0;
                fprintf(stderr,"l'avversario se ne è andato\n");
            }
        }
        

    }while(game);


    ret = close(socket_desc);
    free(ricezione);
    free(invio);

    if (ret < 0) handle_error("errore chiusura socket");

    if(ADMIN) fprintf(stderr, "chiusura socket\n");
    fprintf(stderr, "GAME OVER\n");

    exit(EXIT_SUCCESS);

}

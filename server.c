#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <semaphore.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "common.h"

char * getFrase(){

    srand(time(NULL));//random variable time based
    int numeroCasuale = rand() % 17 + 1;

    
    FILE* file = fopen("frasi.txt", "r");
    if(file==NULL) handle_error("errore aperture file");
    char line[1000];
    int currline=0;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (++currline == numeroCasuale) {
            fclose(file);
            return strdup(line);
        }
    }
    fclose(file);
    return strdup(line);//default case, error avoidance
}

void game(int client1, int client2, int cont) {//cont is the current connection
    int ret;
    pid_t pid;
    msg* msginvio=malloc(sizeof(msg));
    msg* msginvio2=malloc(sizeof(msg));
    msg* ricezione1=malloc(sizeof(msg));
    msg* ricezione2=malloc(sizeof(msg));
    
    int shfd = shm_open("/shmname", O_RDWR|O_CREAT, 0666);
    if (shfd<0)
        handle_error("shm");
    
    ret=ftruncate(shfd,sizeof(double));
    if(ret<0)
        handle_error("ftrucate");
    
    double *shmem = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, shfd, 0);
    if (shmem == MAP_FAILED)
        handle_error("mmap");
        
    strcpy(msginvio->payload,"entrambi i giocatori connessi\n");

    

    ret = send(client1, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 1, client 1");
    ret = send(client2, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 1, clinet 2");

    //waiting for players to be ready
    *shmem=1;
    pid=fork();
    if(pid<0)
        handle_error("errore nella fork");
    if(pid==0){
        ret=recv(client1,ricezione1,sizeof(msg),0);
        if(ret<0) handle_error("FIGLIO: errore recv 1, clinet 1");
        if((ricezione1->flagReady)==1) fprintf(stderr, "CONN %d, game: FIGLIO: client 1 pronto\n",cont);
        else
            *shmem=0;
        exit(EXIT_SUCCESS);
    }
    int flagready=1;
    if(pid>0){
        ret=recv(client2,ricezione2,sizeof(msg),0);
        if(ret<0) handle_error("errore riceziorecv 1, clinet 2");
        if((ricezione2->flagReady)==1) fprintf(stderr, "CONN %d, game: PADRE: client 2 pronto\n",cont);
        else
            flagready=0;
    }
    wait(0);
    
    if(flagready && *shmem)
        msginvio->flagReady=1;
    else handle_error("fine game");


    //both players ready
    strcpy(msginvio->payload,"scrivi questa frase piu veloce del tuo avversario:\n");
    strcpy(msginvio->frase,getFrase());
    
    ret = send(client1, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 2, clinet 1");
    ret = send(client2, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 2, client 2");

    //start round
    struct timeval inizio, fine;
    gettimeofday(&inizio, NULL);
    double tempo_trascorso;

    pid=fork();
    if(pid<0){
        handle_error("fork fallita");
    }

    if(pid==0){
        
        do{
            ret=recv(client1,ricezione1,sizeof(msg),0);
            if(ret<0) handle_error("errore recv 2, client 1");
            if(strcmp (ricezione1->frase, msginvio->frase) == 0){

                gettimeofday(&fine, NULL);
                tempo_trascorso = (fine.tv_sec - inizio.tv_sec); // Secondi
                tempo_trascorso += (fine.tv_usec - inizio.tv_usec) / 1000000.0; //micro in secondi
                
                strcpy(msginvio->payload,"frase corretta");
                msginvio->time=tempo_trascorso;
                ret = send(client1, msginvio, sizeof(msg), 0);
                if(ret<0) handle_error("errore send 2, clinet 1");
                
                break;
            
            } else {

                strcpy(msginvio->payload,"frase sbagliata");
                ret = send(client1, msginvio, sizeof(msg), 0);
                if(ret<0) handle_error("errore send 2, clinet 1");
            }
        }while(1);

        *shmem=tempo_trascorso;
        fprintf(stderr,"CONN %d, game: FIGLIO: Il client1 ci ha messo: %.3f s\n",cont,(*shmem));
        exit(EXIT_SUCCESS);
    }
    else{

        do{
            ret=recv(client2,ricezione2,sizeof(msg),0);
            if(ret<0) handle_error("errore recv 2, client 1");
            if(strcmp (ricezione2->frase, msginvio->frase) == 0){

                gettimeofday(&fine, NULL);
                tempo_trascorso = (fine.tv_sec - inizio.tv_sec); // Secondi
                tempo_trascorso += (fine.tv_usec - inizio.tv_usec) / 1000000.0; //micro in secondi
                
                strcpy(msginvio2->payload,"frase corretta");
                msginvio2->time=tempo_trascorso;
                ret = send(client2, msginvio2, sizeof(msg), 0);
                if(ret<0) handle_error("errore send 2, clinet 1");
                
                break;
            
            } else {

                strcpy(msginvio2->payload,"frase sbagliata");
                ret = send(client2, msginvio2, sizeof(msg), 0);
                if(ret<0) handle_error("errore send 2, clinet 1");
            }
        }while(1);
        fprintf(stderr,"CONN %d, game: PADRE: Il client2 ci ha messo: %.3f s\n",cont, tempo_trascorso);
        wait(NULL);

    }
    double t1=(*shmem);
    
    msginvio2->time=t1;
    msginvio->time=tempo_trascorso;

    //winner
    if(t1<tempo_trascorso){
        strcpy(msginvio->payload,"hai vinto!\n");
        strcpy(msginvio2->payload,"hai perso\n");
    
    } else {
        strcpy(msginvio->payload,"hai perso\n");
        strcpy(msginvio2->payload,"hai vinto!\n");
    }
    ret = send(client1, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 3, client 1");
    ret = send(client2, msginvio2, sizeof(msg), 0);
    if(ret<0) handle_error("errore send 3, client 2");


    if(t1==0 || tempo_trascorso==0){
        return;
    }


    // rematch?
    ricezione1->flagReady = 0;
    ricezione2->flagReady = 0;

    pid=fork();
    if(pid<0)
        handle_error("errore fork");
    if(pid==0){
        ret=recv(client1,ricezione1,sizeof(msg),0);
        if(ret<0) handle_error("errore recv rematch, client 1");
        if(ricezione1->flagReady == 1)
            fprintf(stderr,"CONN %d, game: FIFLIO: Il client1 vuole il rematch\n",cont);
        else
            fprintf(stderr,"CONN %d, game: FIFLIO: Il client1 NON vuole il rematch\n",cont);
        *shmem=(double) ricezione1->flagReady;
        exit(EXIT_SUCCESS);
    }
    if(pid>0){
        ret=recv(client2,ricezione2,sizeof(msg),0);
        if(ret<0) handle_error("errore recv rematch, client 2");
        if(ricezione2->flagReady==1)
            fprintf(stderr,"CONN %d, game: FIFLIO: Il client2 vuole il rematch\n",cont);
        else
            fprintf(stderr,"CONN %d, game: FIFLIO: Il client2 NON vuole il rematch\n",cont);
    }
    wait(0);
    int fr=(int)*shmem;

    if(fr && ricezione2->flagReady){
        msginvio->flagReady=1;
        send(client1,msginvio,sizeof(msg),0);
        if(ret<0) handle_error("errore send 4, client 1");
        send(client2,msginvio,sizeof(msg),0);
        if(ret<0) handle_error("errore send 4, client 1");
        
        fprintf(stderr,"CONN %d, game: faccio partire il rematch\n",cont);

        game(client1,client2,cont);
    } else {
        msginvio->flagReady=0;
        send(client1,msginvio,sizeof(msg),0);
        if(ret<0) handle_error("errore send 4, client 1");
        send(client2,msginvio,sizeof(msg),0);
        if(ret<0) handle_error("errore send 4, client 1");
    }



    free(msginvio);
    free(msginvio2);
    free(ricezione1);
    free(ricezione2);

    close(shfd);
    shm_unlink("/shmname");
    
}

void* connection_handler(int socket_desc) {
    
    msg* msginvio=malloc(sizeof(msg));
    sprintf(msginvio->payload, "FAST TYPE: hello");

    int ret = send(socket_desc, msginvio, sizeof(msg), 0);
    if(ret<0) handle_error("errore send conn_hand");

    free(msginvio);
    return NULL;
}

int main(int argc, char* argv[]) {
    int ret;
    int client_desc1, client_desc2;

    struct sockaddr_in server_addr = {0}, client_addr = {0};
    
    int socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc < 0)
        handle_error("errore creazione socket");
    if(ADMIN) fprintf(stderr, "Socket creata\n");

    int reuseaddr_opt = 1;
    ret = setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_opt, sizeof(reuseaddr_opt));
    if (ret < 0)
        handle_error("errore sockpot");

    
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(SERVER_PORT); 

    int sockaddr_len = sizeof(struct sockaddr_in);
    ret = bind(socket_desc, (struct sockaddr*) &server_addr, sockaddr_len);
    if (ret < 0)
        handle_error("Cannot bind address to socket");
    if(ADMIN) fprintf(stderr, "bind fatta\n");

    ret = listen(socket_desc, MAX_CONN_QUEUE);
    if (ret < 0)
        handle_error("Cannot listen on socket");
    if(ADMIN) fprintf(stderr, "listen\n");

    int cont=0;
    while (1) {
		cont++;
		fprintf(stderr, "CONN %d, mi metto in ascolto di una coppia...\n",cont);
    
        client_desc1 = accept(socket_desc, (struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc1 < 0)
            handle_error("errore accept primo client");        
        fprintf(stderr, "CONN %d, connessione 1 accetta\n",cont);
    
        // connection initializer 1
        connection_handler(client_desc1);

        fprintf(stderr,"CONN %d, aspetto il secondo client...\n",cont);
        client_desc2 = accept(socket_desc, (struct sockaddr*) &client_addr, (socklen_t*) &sockaddr_len);
        if (client_desc2 < 0)
            handle_error("errore accept secondo client");
        fprintf(stderr, "CONN %d, connessione 2 accetta\n",cont);
        
        // connection initializer 2
        connection_handler(client_desc2);

        pid_t pid=fork();
        if(pid<0)
            handle_error("fork non andata a buon fine");
        if(pid==0){
            fprintf(stderr, "CONN %d, FIGLIO: iniziamo il gioco\n",cont);
            game(client_desc1,client_desc2,cont);

            fprintf(stderr, "CONN %d, gioco finito!\n",cont);
            close(client_desc1);
            close(client_desc2);
            _exit(EXIT_SUCCESS);
        }
        if(pid>0){
            close(client_desc1);
            close(client_desc2);
        }
    
    }

}

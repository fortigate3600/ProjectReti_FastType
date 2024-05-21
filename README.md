# FAST TYPE

The server accepts pairs of clients (max 5 couple) and initializes the connection with a connection_handler(client). When both clients are ready and it puts them into a game: game(client1, client2).

The data exchange between client and server occurs through the following msg structure:

typedef struct msg{
    char payload[1000];
    int flagReady;
    double time;
    char frase[1000];
}msg;

game: (form the point of view of the server)
The client creates a new process using fork. Both the parent and child processes wait with a recv to check if both clients are ready.
The child's response is sent to the parent via shared memory;

if both clients are ready, the child sends the phrase. Two separate processes handle the two clients simultaneously.

The client receives the phrase and verifies it. If it is correct, the client sends an "ok" message and saves the reception time. If it is incorrect, the client is asked to rewrite it.

Once both clients have finished sending the correct phrase, the times are compared to determine the winner. The result is sent to both clients.

The server asks both clients if they want to play again. If both agree, the game function is called again.

tp compiling: make

to run the server: ./server

to run the clients do n times: ./clinet

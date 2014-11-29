/*

Lista de coisas:

* Dados vida
* Dados magic points
* Trevos
* Grana
* Chaves
* Comportamento


*/

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <stdlib.h>

#include "actor.h"
#include "scene.h"
#include "sockmanager.h"

// "SocketAction"

enum SocketAction {
    getLife      = 0,
    getMP        = 1,
    getBehaviour = 2
};

int socketMainLoop(int sockfd) {
    int action;
    char buffer[256];


    while(1){
        bzero(buffer, 256);
        printf("[Socket Reporter] Aguardando...");
        read(sockfd, buffer, 255);
        sscanf(buffer, "%d", &action);
        printf("[Socket Reporter] Recebido '%d'\n", action);
        
        switch (action) {
            case getLife:
                bzero(buffer, 256);
                
                sprintf(buffer, "%d", sceneHero->life);
                printf("[Socket Reporter] Enviando: '%s'\n", buffer);
                write(sockfd, buffer, 255);
                break;
            case getMP:
                bzero(buffer, 256);
                sprintf(buffer, "%d", sceneHero->life);
                write(sockfd, buffer, 255);
                break;
            case getBehaviour:
                bzero(buffer, 256);
                sprintf(buffer, "%d", sceneHero->life);
                write(sockfd, buffer, 255);
                break;
            default:
                perror("[Socket Reportar] Invalid action!");
                break;
        }
    }

    return 0;
}



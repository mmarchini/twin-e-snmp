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
#include "gamestate.h"
#include "sockmanager.h"
#include "lbaengine.h"

#define RESPONSE_SIZE 255

// "SocketAction"

enum SocketAction {
    // Game Statistics
    getPaused      = 0,
    getPlayerName  = 1,
    // Player Statistics
    getLife        = 3,
    getMagicPoints = 4,
    getBehaviour   = 5
};

void intToResponse(int value, char *msg){
    bzero(msg, RESPONSE_SIZE);
    sprintf(msg, "%d", value);
}

void strToResponse(char *str, char *msg){
    bzero(msg, RESPONSE_SIZE);
    sprintf(msg, "%s", str);
}

void sendResponse(int sockfd, char *response) {
    printf("[Socket Reporter] Enviando: '%s'\n", response);
    write(sockfd, response, RESPONSE_SIZE);
}

int socketMainLoop(int sockfd) {
    int action;
//    int responseAux;
    char buffer[RESPONSE_SIZE+1];
    bzero(buffer, RESPONSE_SIZE+1);


    while(1){
        bzero(buffer, 256);
        printf("[Socket Reporter] Aguardando...");
        read(sockfd, buffer, RESPONSE_SIZE);
        sscanf(buffer, "%d", &action);
        printf("[Socket Reporter] Recebido '%d'\n", action);
        
        switch (action) {
            case getPaused:
                intToResponse(isTimeFreezedFn(), buffer);
                sendResponse(sockfd, buffer);
                break;
            case getPlayerName:
                strToResponse(savePlayerName, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getLife:
                intToResponse(sceneHero->life, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getMagicPoints:
                intToResponse(inventoryMagicPoints, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getBehaviour:
                intToResponse(heroBehaviour, buffer);
                sendResponse(sockfd, buffer);
                break;
            default:
                bzero(buffer, RESPONSE_SIZE);
                sendResponse(sockfd, buffer);
                break;
        }
    }

    return 0;
}



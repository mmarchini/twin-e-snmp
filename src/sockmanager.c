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
  /////////////////
 //-GET ACTIONS-//
/////////////////
    
    // Game Statistics
    getPaused       =  0,
    getPlayerName   =  1,
    // Player Statistics
    getMaxLife      =  3,
    getCurLife      =  4,
    getMaxMP        =  5,
    getCurMP        =  6,
    getKeys         =  7,
    getCash         =  8,
    // -- Leaf Table
    getNumLeafs     =  9,
    getNumLeafBoxes = 10,
    // Player Behaviour
    getCurBehaviour = 11,
    getHasProtopack = 12,

  /////////////////
 //-SET ACTIONS-//
/////////////////

    // Game Statistics
    setPaused         = 100,
    setPlayerName     = 101,
    // Player Behaviour
    setCurBehaviour   = 111,
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
    int intValue;
    char *value;
    char buffer[RESPONSE_SIZE+1];
    bzero(buffer, RESPONSE_SIZE+1);


    while(1){
        bzero(buffer, 256);
        printf("[Socket Reporter] Aguardando...");
        read(sockfd, buffer, RESPONSE_SIZE);
        sscanf(buffer, "%d", &action);
        value = strchr(buffer, ' ');

        printf("[Socket Reporter] Recebido '%d'\n", action);
        
        switch (action) {
             /////////////
            // GETTERS //
           /////////////

             //---------------------//
            // LBA Game Statistics //
            case getPaused: // Indica se o jogo está pausado  TODO 
                intToResponse(isTimeFreezedFn(), buffer);
                sendResponse(sockfd, buffer);
                break;
            case getPlayerName: // Responde com o nome do jogador
                strToResponse(savePlayerName, buffer);
                sendResponse(sockfd, buffer);
                break;

             //---------------------//
            // LBA Hero Statistics //
            case getMaxLife: // Vida máxima do jogador 
                intToResponse(HERO_MAX_LIFE, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getCurLife: // Vida atual do jogador
                intToResponse(sceneHero->life, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getMaxMP: // Pontos de Magia máxido do jogador
                intToResponse(maxMagicPoints(), buffer);
                sendResponse(sockfd, buffer);
                break;
            case getCurMP: // Pontos de Magia atual do jogador
                intToResponse(inventoryMagicPoints, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getKeys: // Chaves que o jogador possuí
                intToResponse(inventoryNumKeys, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getCash: // Dinheiro do jogador
                intToResponse(inventoryNumKashes, buffer);
                sendResponse(sockfd, buffer);
                break;
             
            // -- Leaf Table -- //
            case getNumLeafs: // Trevos que o jogador possuí
                intToResponse(inventoryNumLeafs, buffer);
                sendResponse(sockfd, buffer);
                break;
            case getNumLeafBoxes: // Caixas de trevo que o jogador possuí
                intToResponse(inventoryNumLeafsBox, buffer);
                sendResponse(sockfd, buffer);
                break;

             //--------------------//
            // LBA Hero Behaviour //
            case getCurBehaviour: // Retorna com o comportamento atual do jogador
                intToResponse(heroBehaviour, buffer);
                sendResponse(sockfd, buffer);
                break;

            case getHasProtopack: // 1 se o jogador tem o protopack, 0 caso contrário
                intToResponse(gameFlags[GAMEFLAG_PROTOPACK], buffer);
                sendResponse(sockfd, buffer);
                break;

             /////////////
            // SETTERS //
           /////////////
            case setPaused: // TODO
                intToResponse(isTimeFreezedFn(), buffer);
                sendResponse(sockfd, buffer);
                break;
            case setPlayerName:
                if(value) value++;

                strncpy(savePlayerName, buffer, PLAYER_NAME_LEN);

                strToResponse(savePlayerName, buffer);
                sendResponse(sockfd, buffer);
                break;
            case setCurBehaviour:
                if(value) value++;
                
                sscanf(value, "%d", &intValue);

                printf("\n\nVALOR RECEBIDO: %d\n\n", intValue);

                if(0<=intValue && intValue<=(3+gameFlags[GAMEFLAG_PROTOPACK]))
                    setBehaviour(intValue);

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



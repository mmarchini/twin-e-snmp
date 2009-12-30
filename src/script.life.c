/** @file script.life.c
	@brief
	This file contains movies routines

	Prequengine: a Little Big Adventure engine

	Copyright (C) 2008 Prequengine team \n
	Copyright (C) 2002-2007 The TwinEngine team \n

	This program is free software; you can redistribute it and/or \n
	modify it under the terms of the GNU General Public License \n
	as published by the Free Software Foundation; either version 2 \n
	of the License, or (at your option) any later version. \n

	This program is distributed in the hope that it will be useful, \n
	but WITHOUT ANY WARRANTY; without even the implied warranty of \n
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the \n
	GNU General Public License for more details. \n

	You should have received a copy of the GNU General Public License \n
	along with this program; if not, write to the Free Software \n
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

	$URL$
	$Id$
*/

#include <stdio.h>
#include <string.h>

#include "script.life.h"
#include "scene.h"
#include "actor.h"
#include "movements.h"
#include "animations.h"
#include "scene.h"
#include "renderer.h"
#include "sound.h"
#include "redraw.h"
#include "lbaengine.h"
#include "gamestate.h"
#include "grid.h"
#include "music.h"
#include "movies.h"

uint8 *scriptPtr; // local script pointer
uint8 *opcodePtr; // local opcode script pointer

/** Returns:
	   -1 - TODO
		0 - Completed
		1 - Break script */
typedef int32 ScriptLifeFunc(int32 actorIdx, ActorStruct *actor);

typedef struct ScriptLifeFunction {
	const uint8 *name;
	ScriptLifeFunc *function;
} ScriptLifeFunction;

#define MAPFUNC(name, func) {name, func}

/** Script condition operators */
enum LifeScriptOperators {
	/*==*/ kEqualTo = 0,
	/*> */ kGreaterThan = 1,
	/*< */ kLessThan = 2,
	/*>=*/ kGreaterThanOrEqualTo = 3,
	/*<=*/ kLessThanOrEqualTo = 4,
	/*!=*/ kNotEqualTo = 5
};

/** Script condition command opcodes */
enum LifeScriptConditions {
	/*0x00*/ kcCOL = 0,
	/*0x01*/ kcCOL_OBJ = 1,
	/*0x02*/ kcDISTANCE = 2,
	/*0x03*/ kcZONE = 3,
	/*0x04*/ kcZONE_OBJ = 4,
	/*0x05*/ kcBODY = 5,
	/*0x06*/ kcBODY_OBJ = 6,
	/*0x07*/ kcANIM = 7,
	/*0x08*/ kcANIM_OBJ = 8,
	/*0x09*/ kcL_TRACK = 9,
	/*0x0A*/ kcL_TRACK_OBJ = 10,
	/*0x0B*/ kcFLAG_CUBE = 11,
	/*0x0C*/ kcCONE_VIEW = 12,
	/*0x0D*/ kcHIT_BY = 13,
	/*0x0E*/ kcACTION = 14,
	/*0x0F*/ kcFLAG_GAME = 15,
	/*0x10*/ kcLIFE_POINT = 16,
	/*0x11*/ kcLIFE_POINT_OBJ = 17,
	/*0x12*/ kcNUM_LITTLE_KEYS = 18,
	/*0x13*/ kcNUM_GOLD_PIECES = 19,
	/*0x14*/ kcBEHAVIOUR = 20,
	/*0x15*/ kcCHAPTER = 21,
	/*0x16*/ kcDISTANCE_3D = 22,
	/*0x17 - 23 unused */
	/*0x18 - 24 unused */
	/*0x19*/ kcUSE_INVENTORY = 25,
	/*0x1A*/ kcCHOICE= 26,
	/*0x1B*/ kcFUEL = 27,
	/*0x1C*/ kcCARRIED_BY = 28,
	/*0x1D*/ kcCDROM = 29
};



/** Returns:
	   -1 - TODO
		1 - Condition value size (1 byte)
		2 - Condition value size (2 byes) */
int32 process_life_conditions(ActorStruct *actor) {
	int32 conditionOpcode, conditionValueSize;

	conditionValueSize = 1;
	conditionOpcode = *(scriptPtr++);

	switch(conditionOpcode) {
	case kcCOL: 
		if (actor->life <= 0) {
			currentScriptValue = -1;
		} else {
			currentScriptValue = actor->collision;
		}
		break;
	case kcCOL_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		if (sceneActors[actorIdx].life <= 0) {
			currentScriptValue = -1;
		} else {
			currentScriptValue = sceneActors[actorIdx].collision;
		}
	}
		break;
	case kcDISTANCE: {
		ActorStruct *otherActor;
		int32 actorIdx = *(scriptPtr++);
		conditionValueSize = 2;
		otherActor = &sceneActors[actorIdx];
		if (!otherActor->dynamicFlags.bIsDead) {
			if (Abs(otherActor->Y - actor->Y) >= 1500) {
				currentScriptValue = 0x7D00;	
			} else {
				currentScriptValue = get_distance_2D(actor->X, actor->Z, otherActor->X, otherActor->Z);
				if (currentScriptValue > 0x7D00) {
					currentScriptValue = 0x7D00;
				}
			}
		} else {
			currentScriptValue = 0x7D00;
		}
	}
		break;
	case kcZONE:
		currentScriptValue = actor->zone;
		break;
	case kcZONE_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		currentScriptValue = sceneActors[actorIdx].zone;
	}
		break;
	case kcBODY:
		currentScriptValue = actor->body;
		break;
	case kcBODY_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		currentScriptValue = sceneActors[actorIdx].body;
	}
		break;
	case kcANIM:
		currentScriptValue = actor->anim;
		break;
	case kcANIM_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		currentScriptValue = sceneActors[actorIdx].anim;
	}
		break;
	case kcL_TRACK:
		currentScriptValue = actor->labelIdx;
		break;
	case kcL_TRACK_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		currentScriptValue = sceneActors[actorIdx].labelIdx;
	}
		break;
	case kcFLAG_CUBE: {
		int32 flagIdx = *(scriptPtr++);
		currentScriptValue = sceneFlags[flagIdx];
	}
		break;
	case kcCONE_VIEW: {
		int32 newAngle;
		int32 targetActorIdx;
		ActorStruct *targetActor;

		targetActorIdx = *(scriptPtr++);
		targetActor = &sceneActors[targetActorIdx];

		conditionValueSize = 2;

		if (!targetActor->dynamicFlags.bIsDead) {
			if (targetActor->Z - actor->Z < 1500) {
				newAngle = get_angle(actor->X, actor->Z, targetActor->X, targetActor->Z);
				if (moveAngle > 0x7D00) { // TODO: recheck this
					moveAngle = 0x7D00;
				} else {
					moveAngle = 0x7D00;
				}

				if (!targetActorIdx) {
					int32 heroAngle;

					heroAngle = actor->angle + 0x480 - newAngle + 0x400;
					heroAngle &= 0x3FF;

					if (heroAngle >= 0x100) {
						currentScriptValue = 0x7D00;
					} else {
						currentScriptValue = moveAngle;
					}
				}
			}
		} else {
			currentScriptValue = 0x7D00;
		}
	}
		break;
	case kcHIT_BY:
		currentScriptValue = actor->hitBy;
		break;
	case kcACTION:
		currentScriptValue = heroAction;
		break;
	case kcFLAG_GAME: {
		int32 flagIdx = *(scriptPtr++);
		if (!gameFlags[GAMEFLAG_INVENTORY_DISABLED] ||
			(gameFlags[GAMEFLAG_INVENTORY_DISABLED] && flagIdx >= 28)) {
			currentScriptValue = gameFlags[flagIdx];
		} else {
			if (flagIdx == GAMEFLAG_INVENTORY_DISABLED) {
				currentScriptValue = gameFlags[flagIdx];
			} else {
				currentScriptValue = 0;
			}
		}
	}
		break;
	case kcLIFE_POINT:
		currentScriptValue = actor->life;
		break;
	case kcLIFE_POINT_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		currentScriptValue = sceneActors[actorIdx].life;
	}
		break;
	case kcNUM_LITTLE_KEYS:
		currentScriptValue = inventoryNumKeys;
		break;
	case kcNUM_GOLD_PIECES:
		conditionValueSize = 2;
		currentScriptValue = inventoryNumCoins;
		break;
	case kcBEHAVIOUR:
		currentScriptValue = heroBehaviour;
		break;
	case kcCHAPTER:
		currentScriptValue = gameChapter;
		break;
	case kcDISTANCE_3D: {
		int32 targetActorIdx;
		ActorStruct *targetActor;

		targetActorIdx = *(scriptPtr++);
		targetActor = &sceneActors[targetActorIdx];

		conditionValueSize = 2;

		if (!targetActor->dynamicFlags.bIsDead) {
			currentScriptValue = get_distance_3D(actor->X, actor->Y, actor->Z, targetActor->X, targetActor->Y, targetActor->Z);		
			if (currentScriptValue > 0x7D00) { // TODO: recheck this
				currentScriptValue = 0x7D00;
			}
		} else {
			currentScriptValue = 0x7D00;
		}
	}
		break;
	case 23: // unused
	case 24:
		break;
	case kcUSE_INVENTORY: { // TODO
		int32 invItem = *(scriptPtr++);
		//inventoryFlags[invItem]
		currentScriptValue = -1; // currentScriptValue = inGameMenuAnswer;
	}
		break;
	case kcCHOICE: // TODO
		conditionValueSize = 2;
		currentScriptValue = -1; // currentScriptValue = inGameMenuAnswer;
		break;
	case kcFUEL:
		currentScriptValue = inventoryNumGas;
		break;
	case kcCARRIED_BY:
		currentScriptValue = actor->standOn;
		break;
	case kcCDROM:
		currentScriptValue = 1;
		break;
	default:
		printf("ERROR: Actor condition opcode %d\n", conditionOpcode);
		break;
	}

	return conditionValueSize;
}

/** Returns:
	   -1 - TODO
		0 - Condition false
		1 - Condition true */
int32 process_life_operators(int32 valueSize) {
	int32 operatorCode, conditionValue;

	operatorCode = *(scriptPtr++);

	if (valueSize == 1) {
		conditionValue = *(scriptPtr++);
	} else if(valueSize == 2) {
		conditionValue = *((int16 *)scriptPtr);
		scriptPtr += 2;
	} else {
		// TODO: add message
		return 0;
	}

	switch(operatorCode) {
	case kEqualTo:
		if (currentScriptValue == conditionValue) {
			return 1;
		}
		break;
	case kGreaterThan:
		if (currentScriptValue > conditionValue) {
			return 1;
		}
		break;
	case kLessThan:
		if (currentScriptValue < conditionValue) {
			return 1;
		}
		break;
	case kGreaterThanOrEqualTo:
		if (currentScriptValue >= conditionValue) {
			return 1;
		}
		break;
	case kLessThanOrEqualTo:
		if (currentScriptValue <= conditionValue) {
			return 1;
		}
		break;
	case kNotEqualTo:
		if (currentScriptValue != conditionValue) {
			return 1;
		}
		break;
	default:
		printf("ERROR: Actor operator opcode %d\n", operatorCode);
		break;
	}

	return 0;
}

/** Life script command definitions */

/* For unused opcodes */
int32 lEMPTY(int32 actorIdx, ActorStruct *actor) {
	return 0;
}

/*0x00*/
int32 lEND(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = -1;
	return 1; // break script
}

/*0x01*/
int32 lNOP(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x02*/
int32 lSNIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	if (!process_life_operators(valueSize)) {
		*opcodePtr = 13; // SWIF
	}
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	return 0;
}

/*0x03*/
int32 lOFFSET(int32 actorIdx, ActorStruct *actor) {
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // offset
	return 0;
}

/*0x04*/
int32 lNEVERIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	process_life_operators(valueSize);	
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	return 0;
}

/*0x06*/
int32 lNO_IF(int32 actorIdx, ActorStruct *actor) {
	return 0;
}

/*0x0A*/
int32 lLABEL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x0B*/
int32 lRETURN(int32 actorIdx, ActorStruct *actor) {
	return 1; // break script
}

/*0x0C*/
int32 lIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	if (!process_life_operators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
	}
	
	return 0;
}

/*0x0D*/
int32 lSWIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	if (!process_life_operators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
		*opcodePtr = 2; // SNIF
	}
	
	return 0;
}

/*0x0E*/
int32 lONEIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	if (!process_life_operators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
		*opcodePtr = 4; // NEVERIF
	}
	
	return 0;
}

/*0x0F*/
int32 lELSE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // offset
	return 0;
}

/*0x11*/
int32 lBODY(int32 actorIdx, ActorStruct *actor) {
	int32 bodyIdx = *(scriptPtr);
	init_body(bodyIdx, actorIdx);
	scriptPtr++;
	return 0;
}

/*0x12*/
int32 lBODY_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 otherBodyIdx = *(scriptPtr++);
	init_body(otherBodyIdx, otherActorIdx);
	return 0;
}

/*0x13*/
int32 lANIM(int32 actorIdx, ActorStruct *actor) {
	int32 animIdx = *(scriptPtr++);
	init_anim(animIdx, 0, 0, actorIdx);
	return 0;
}

/*0x14*/
int32 lANIM_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 otherAnimIdx = *(scriptPtr++);
	init_anim(otherAnimIdx, 0, 0, otherActorIdx);
	return 0;
}

/*0x15*/
int32 lSET_LIFE(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x16*/
int32 lSET_LIFE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	sceneActors[otherActorIdx].positionInLifeScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x17*/
int32 lSET_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->positionInMoveScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x18*/
int32 lSET_TRACK_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	sceneActors[otherActorIdx].positionInMoveScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x19*/
int32 lMESSAGE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2;
	return -1;
}

/*0x1A*/
int32 lFALLABLE(int32 actorIdx, ActorStruct *actor) {
	int32 flag = *(scriptPtr++);
	actor->staticFlags.bCanFall = flag & 1;
	return 0;
}

/*0x1B*/
int32 lSET_DIRMODE(int32 actorIdx, ActorStruct *actor) {
	int32 controlMode = *(scriptPtr++);

	actor->controlMode = controlMode;
	if (controlMode == kFOLLOW) {
		actor->followedActor = *(scriptPtr++);
	}

	return 0;
}

/*0x1C*/
int32 lSET_DIRMODE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 controlMode = *(scriptPtr++);

	sceneActors[otherActorIdx].controlMode = controlMode;
	if (controlMode == kFOLLOW) {
		sceneActors[otherActorIdx].followedActor = *(scriptPtr++);
	}

	return 0;
}

/*0x1D*/
int32 lCAM_FOLLOW(int32 actorIdx, ActorStruct *actor) {
	int32 followedActorIdx;
	followedActorIdx = *(scriptPtr++);

	if (currentlyFollowedActor != followedActorIdx) {
		newCameraX = sceneActors[followedActorIdx].X >> 9;
		newCameraY = sceneActors[followedActorIdx].Y >> 8;
		newCameraZ = sceneActors[followedActorIdx].Z >> 9;

		currentlyFollowedActor = followedActorIdx;
		reqBgRedraw = 1;
	}

	return 0;
}

/*0x1E*/
int32 lSET_BEHAVIOUR(int32 actorIdx, ActorStruct *actor) {
	int32 behavior = *(scriptPtr++);

	init_anim(ANIM_STANDING, 0, 255, 0);
	set_behaviour(behavior);

	return 0;
}

/*0x1F*/
int32 lSET_FLAG_CUBE(int32 actorIdx, ActorStruct *actor) {
	int32 flagIdx = *(scriptPtr++);
	int32 flagValue = *(scriptPtr++);

	sceneFlags[flagIdx] = flagValue;
	
	return 0;
}

/*0x20*/
int32 lCOMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x21*/
int32 lSET_COMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = *((int16 *)scriptPtr);
	scriptPtr += 2;
	return 0;
}

/*0x22*/
int32 lSET_COMPORTEMENT_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);

	sceneActors[otherActorIdx].positionInLifeScript = *((int16 *)scriptPtr);
	scriptPtr += 2;

	return 0;
}

/*0x23*/
int32 lEND_COMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	return 1; // break
}

/*0x24*/
int32 lSET_FLAG_GAME(int32 actorIdx, ActorStruct *actor) {
	int32 flagIdx = *(scriptPtr++);
	int32 flagValue = *(scriptPtr++);

	gameFlags[flagIdx] = flagValue;
	
	return 0;
}

/*0x25*/
int32 lKILL_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++); // TODO

	process_actor_carrier(otherActorIdx);
	actor->dynamicFlags.bIsDead = 1;
	actor->entity = -1;
	actor->zone = -1;
	actor->life = 0;

	return 0;
}

/*0x26*/
int32 lSUICIDE(int32 actorIdx, ActorStruct *actor) {
	process_actor_carrier(actorIdx);
	actor->dynamicFlags.bIsDead = 1;
	actor->entity = -1;
	actor->zone = -1;
	actor->life = 0;
	return 0;
}

/*0x27*/
int32 lUSE_ONE_LITTLE_KEY(int32 actorIdx, ActorStruct *actor) {
	inventoryNumKeys--;

	if (inventoryNumKeys < 0) {
		inventoryNumKeys = 0;
	}

	// TODO: add overlay sprite for keys
	
	return 0;
}

/*0x28*/
int32 lGIVE_GOLD_PIECES(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x29*/
int32 lEND_LIFE(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = -1;
	return 1; // break;
}

/*0x2A*/
int32 lSTOP_L_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->pausedTrackPtr = actor->currentLabelPtr;
	actor->positionInMoveScript = -1;
	return 0;
}

/*0x2B*/
int32 lRESTORE_L_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->positionInMoveScript = actor->pausedTrackPtr;
	return 0;
}

/*0x2C*/
int32 lMESSAGE_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 3; // TODO
	return -1;
}

/*0x2D*/
int32 lINC_CHAPTER(int32 actorIdx, ActorStruct *actor) {
	gameChapter++;
	return 0;
}

/*0x2E*/
int32 lFOUND_OBJECT(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x2F*/
int32 lSET_DOOR_LEFT(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;
	
	actor->angle = 0x300;
	actor->X = actor->lastX - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x30*/
int32 lSET_DOOR_RIGHT(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0x100;
	actor->X = actor->lastX - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x31*/
int32 lSET_DOOR_UP(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0x200;
	actor->Z = actor->lastX - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x32*/
int32 lSET_DOOR_DOWN(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0;
	actor->Z = actor->lastX - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x33*/
int32 lGIVE_BONUS(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x34*/
int32 lCHANGE_CUBE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x35*/
int32 lOBJ_COL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x36*/
int32 lBRICK_COL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x37*/
int32 lOR_IF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = process_life_conditions(actor);
	if (process_life_operators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
	}
	
	return 0;
}

/*0x38*/
int32 lINVISIBLE(int32 actorIdx, ActorStruct *actor) {
	actor->staticFlags.bIsHidden = *(scriptPtr);
	scriptPtr++;
	return 0;
}

/*0x39*/
int32 lZOOM(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x3A*/
int32 lPOS_POINT(int32 actorIdx, ActorStruct *actor) {
	int32 trackIdx = *(scriptPtr++);

	destX = sceneTracks[trackIdx].X;
	destY = sceneTracks[trackIdx].Y;
	destZ = sceneTracks[trackIdx].Z;

	actor->X = destX;
	actor->Y = destY;
	actor->Z = destZ;

	return 0;
}

/*0x3B*/
int32 lSET_MAGIC_LEVEL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x3C*/
int32 lSUB_MAGIC_POINT(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x3D*/
int32 lSET_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x3E*/
int32 lSUB_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x3F*/
int32 lHIT_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x40*/
int32 lPLAY_FLA(int32 actorIdx, ActorStruct *actor) {
	int8 *movie = (int8 *)scriptPtr;
	int32 nameSize = strlen(movie);
	play_movie(movie);
	scriptPtr += nameSize + 1;
	return 0;
}

/*0x41*/
int32 lPLAY_MIDI(int32 actorIdx, ActorStruct *actor) {
	int32 midiIdx = *(scriptPtr++);
	play_midi_music(midiIdx, 0); // TODO: improve this
	return 0;
}

/*0x42*/
int32 lINC_CLOVER_BOX(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x43*/
int32 lSET_USED_INVENTORY(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x44*/
int32 lADD_CHOICE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x45*/
int32 lASK_CHOICE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x46*/
int32 lBIG_MESSAGE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x47*/
int32 lINIT_PINGOUIN(int32 actorIdx, ActorStruct *actor) {
	int32 pingouinActor = *(scriptPtr++);
	sceneActors[pingouinActor].dynamicFlags.bIsDead = 1;
	mecaPinguinIdx = pingouinActor;
	sceneActors[pingouinActor].entity = -1;
	sceneActors[pingouinActor].zone = -1;
	return 0;
}

/*0x48*/
int32 lSET_HOLO_POS(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x49*/
int32 lCLR_HOLO_POS(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x4A*/
int32 lADD_FUEL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x4B*/
int32 lSUB_FUEL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x4C*/
int32 lSET_GRM(int32 actorIdx, ActorStruct *actor) {
	cellingGridIdx = *(scriptPtr++);
	init_celling_grid(cellingGridIdx);
	return 0;
}

/*0x4D*/
int32 lSAY_MESSAGE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*04E*/
int32 lSAY_MESSAGE_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 3; // TODO
	return -1;
}

/*0x4F*/
int32 lFULL_POINT(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x50*/
int32 lBETA(int32 actorIdx, ActorStruct *actor) {
	int32 newAngle = *((int16 *)scriptPtr);
	scriptPtr += 2;
	actor->angle = newAngle;
	clear_real_angle(actor);
	return 0;
}

/*0x51*/
int32 lGRM_OFF(int32 actorIdx, ActorStruct *actor) {
	if (cellingGridIdx != -1) {
		useCellingGrid = -1;
		cellingGridIdx = -1;
		create_grid_map();
		redraw_engine_actions(1);
	}
	
	return 0;
}

/*0x52*/
int32 lFADE_PAL_RED(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x53*/
int32 lFADE_ALARM_RED(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x54*/
int32 lFADE_ALARM_PAL(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x55*/
int32 lFADE_RED_PAL(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x56*/
int32 lFADE_RED_ALARM(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x57*/
int32 lFADE_PAL_ALARM(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x58*/
int32 lEXPLODE_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x59*/
int32 lBUBBLE_ON(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x5A*/
int32 lBUBBLE_OFF(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x5B*/
int32 lASK_CHOICE_OBJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 3; // TODO
	return -1;
}

/*0x5C*/
int32 lSET_DARK_PAL(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x5D*/
int32 lSET_NORMAL_PAL(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x5E*/
int32 lMESSAGE_SENDELL(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x5F*/
int32 lANIM_SET(int32 actorIdx, ActorStruct *actor) {
	int32 animIdx = *(scriptPtr++);

	actor->anim = -1;
	actor->previousAnimIdx = -1;
	init_anim(animIdx, 0, 0, actorIdx);

	return 0;
}

/*0x60*/
int32 lHOLOMAP_TRAJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x61*/
int32 lGAME_OVER(int32 actorIdx, ActorStruct *actor) {
	sceneHero->dynamicFlags.bAnimEnded = 1;
	sceneHero->life = 0;
	inventoryNumLeafs = 0;
	return 1; // break
}

/*0x62*/
int32 lTHE_END(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x63*/
int32 lMIDI_OFF(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x64*/
int32 lPLAY_CD_TRACK(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x65*/
int32 lPROJ_ISO(int32 actorIdx, ActorStruct *actor) {
	set_ortho_projection(311, 240, 512);
	set_base_translation(0, 0, 0);
	set_base_rotation(0, 0, 0);
	set_light_vector(alphaLight, betaLight, 0);
	return 0;
}

/*0x66*/
int32 lPROJ_3D(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x67*/
int32 lTEXT(int32 actorIdx, ActorStruct *actor) {
	scriptPtr += 2; // TODO
	return -1;
}

/*0x68*/
int32 lCLEAR_TEXT(int32 actorIdx, ActorStruct *actor) {
	return -1;
}

/*0x69*/
int32 lBRUTAL_EXIT(int32 actorIdx, ActorStruct *actor) {
	return 1; // break
}

static const ScriptLifeFunction function_map[] = {
	/*0x00*/ MAPFUNC("END", lEND),
	/*0x01*/ MAPFUNC("NOP", lNOP),
	/*0x02*/ MAPFUNC("SNIF", lSNIF),
	/*0x03*/ MAPFUNC("OFFSET", lOFFSET),
	/*0x04*/ MAPFUNC("NEVERIF", lNEVERIF),
	/*0x05*/ MAPFUNC("", lEMPTY), // unused
	/*0x06*/ MAPFUNC("NO_IF", lNO_IF),
	/*0x07*/ MAPFUNC("", lEMPTY), // unused
	/*0x08*/ MAPFUNC("", lEMPTY), // unused
	/*0x09*/ MAPFUNC("", lEMPTY), // unused
	/*0x0A*/ MAPFUNC("LABEL", lLABEL),
	/*0x0B*/ MAPFUNC("RETURN", lRETURN),
	/*0x0C*/ MAPFUNC("IF", lIF),
	/*0x0D*/ MAPFUNC("SWIF", lSWIF),
	/*0x0E*/ MAPFUNC("ONEIF", lONEIF),
	/*0x0F*/ MAPFUNC("ELSE", lELSE),
	/*0x10*/ MAPFUNC("ENDIF", lEMPTY), // unused
	/*0x11*/ MAPFUNC("BODY", lBODY),
	/*0x12*/ MAPFUNC("BODY_OBJ", lBODY_OBJ),
	/*0x13*/ MAPFUNC("ANIM", lANIM),
	/*0x14*/ MAPFUNC("ANIM_OBJ", lANIM_OBJ),
	/*0x15*/ MAPFUNC("SET_LIFE", lSET_LIFE),
	/*0x16*/ MAPFUNC("SET_LIFE_OBJ", lSET_LIFE_OBJ),
	/*0x17*/ MAPFUNC("SET_TRACK", lSET_TRACK),
	/*0x18*/ MAPFUNC("SET_TRACK_OBJ", lSET_TRACK_OBJ),
	/*0x19*/ MAPFUNC("MESSAGE", lMESSAGE),
	/*0x1A*/ MAPFUNC("FALLABLE", lFALLABLE),
	/*0x1B*/ MAPFUNC("SET_DIRMODE", lSET_DIRMODE),
	/*0x1C*/ MAPFUNC("SET_DIRMODE_OBJ", lSET_DIRMODE_OBJ),
	/*0x1D*/ MAPFUNC("CAM_FOLLOW", lCAM_FOLLOW),
	/*0x1E*/ MAPFUNC("SET_BEHAVIOUR", lSET_BEHAVIOUR),
	/*0x1F*/ MAPFUNC("SET_FLAG_CUBE", lSET_FLAG_CUBE),
	/*0x20*/ MAPFUNC("COMPORTEMENT", lCOMPORTEMENT),
	/*0x21*/ MAPFUNC("SET_COMPORTEMENT", lSET_COMPORTEMENT),
	/*0x22*/ MAPFUNC("SET_COMPORTEMENT_OBJ", lSET_COMPORTEMENT_OBJ),
	/*0x23*/ MAPFUNC("END_COMPORTEMENT", lEND_COMPORTEMENT),
	/*0x24*/ MAPFUNC("SET_FLAG_GAME", lSET_FLAG_GAME),
	/*0x25*/ MAPFUNC("KILL_OBJ", lKILL_OBJ),
	/*0x26*/ MAPFUNC("SUICIDE", lSUICIDE),
	/*0x27*/ MAPFUNC("USE_ONE_LITTLE_KEY", lUSE_ONE_LITTLE_KEY),
	/*0x28*/ MAPFUNC("GIVE_GOLD_PIECES", lGIVE_GOLD_PIECES),
	/*0x29*/ MAPFUNC("END_LIFE", lEND_LIFE),
	/*0x2A*/ MAPFUNC("STOP_L_TRACK", lSTOP_L_TRACK),
	/*0x2B*/ MAPFUNC("RESTORE_L_TRACK", lRESTORE_L_TRACK),
	/*0x2C*/ MAPFUNC("MESSAGE_OBJ", lMESSAGE_OBJ),
	/*0x2D*/ MAPFUNC("INC_CHAPTER", lINC_CHAPTER),
	/*0x2E*/ MAPFUNC("FOUND_OBJECT", lFOUND_OBJECT),
	/*0x2F*/ MAPFUNC("SET_DOOR_LEFT", lSET_DOOR_LEFT),
	/*0x30*/ MAPFUNC("SET_DOOR_RIGHT", lSET_DOOR_RIGHT),
	/*0x31*/ MAPFUNC("SET_DOOR_UP", lSET_DOOR_UP),
	/*0x32*/ MAPFUNC("SET_DOOR_DOWN", lSET_DOOR_DOWN),
	/*0x33*/ MAPFUNC("GIVE_BONUS", lGIVE_BONUS),
	/*0x34*/ MAPFUNC("CHANGE_CUBE", lCHANGE_CUBE),
	/*0x35*/ MAPFUNC("OBJ_COL", lOBJ_COL),
	/*0x36*/ MAPFUNC("BRICK_COL", lBRICK_COL),
	/*0x37*/ MAPFUNC("OR_IF", lOR_IF),
	/*0x38*/ MAPFUNC("INVISIBLE", lINVISIBLE),
	/*0x39*/ MAPFUNC("ZOOM", lZOOM),
	/*0x3A*/ MAPFUNC("POS_POINT", lPOS_POINT),
	/*0x3B*/ MAPFUNC("SET_MAGIC_LEVEL", lSET_MAGIC_LEVEL),
	/*0x3C*/ MAPFUNC("SUB_MAGIC_POINT", lSUB_MAGIC_POINT),
	/*0x3D*/ MAPFUNC("SET_LIFE_POINT_OBJ", lSET_LIFE_POINT_OBJ),
	/*0x3E*/ MAPFUNC("SUB_LIFE_POINT_OBJ", lSUB_LIFE_POINT_OBJ),
	/*0x3F*/ MAPFUNC("HIT_OBJ", lHIT_OBJ),
	/*0x40*/ MAPFUNC("PLAY_FLA", lPLAY_FLA),
	/*0x41*/ MAPFUNC("PLAY_MIDI", lPLAY_MIDI),
	/*0x42*/ MAPFUNC("INC_CLOVER_BOX", lINC_CLOVER_BOX),
	/*0x43*/ MAPFUNC("SET_USED_INVENTORY", lSET_USED_INVENTORY),
	/*0x44*/ MAPFUNC("ADD_CHOICE", lADD_CHOICE),
	/*0x45*/ MAPFUNC("ASK_CHOICE", lASK_CHOICE),
	/*0x46*/ MAPFUNC("BIG_MESSAGE", lBIG_MESSAGE),
	/*0x47*/ MAPFUNC("INIT_PINGOUIN", lINIT_PINGOUIN),
	/*0x48*/ MAPFUNC("SET_HOLO_POS", lSET_HOLO_POS),
	/*0x49*/ MAPFUNC("CLR_HOLO_POS", lCLR_HOLO_POS),
	/*0x4A*/ MAPFUNC("ADD_FUEL", lADD_FUEL),
	/*0x4B*/ MAPFUNC("SUB_FUEL", lSUB_FUEL),
	/*0x4C*/ MAPFUNC("SET_GRM", lSET_GRM),
	/*0x4D*/ MAPFUNC("SAY_MESSAGE", lSAY_MESSAGE),
	/*0x4E*/ MAPFUNC("SAY_MESSAGE_OBJ", lSAY_MESSAGE_OBJ),
	/*0x4F*/ MAPFUNC("FULL_POINT", lFULL_POINT),
	/*0x50*/ MAPFUNC("BETA", lBETA),
	/*0x51*/ MAPFUNC("GRM_OFF", lGRM_OFF),
	/*0x52*/ MAPFUNC("FADE_PAL_RED", lFADE_PAL_RED),
	/*0x53*/ MAPFUNC("FADE_ALARM_RED", lFADE_ALARM_RED),
	/*0x54*/ MAPFUNC("FADE_ALARM_PAL", lFADE_ALARM_PAL),
	/*0x55*/ MAPFUNC("FADE_RED_PAL", lFADE_RED_PAL),
	/*0x56*/ MAPFUNC("FADE_RED_ALARM", lFADE_RED_ALARM),
	/*0x57*/ MAPFUNC("FADE_PAL_ALARM", lFADE_PAL_ALARM),
	/*0x58*/ MAPFUNC("EXPLODE_OBJ", lEXPLODE_OBJ),
	/*0x59*/ MAPFUNC("BUBBLE_ON", lBUBBLE_ON),
	/*0x5A*/ MAPFUNC("BUBBLE_OFF", lBUBBLE_OFF),
	/*0x5B*/ MAPFUNC("ASK_CHOICE_OBJ", lASK_CHOICE_OBJ),
	/*0x5C*/ MAPFUNC("SET_DARK_PAL", lSET_DARK_PAL),
	/*0x5D*/ MAPFUNC("SET_NORMAL_PAL", lSET_NORMAL_PAL),
	/*0x5E*/ MAPFUNC("MESSAGE_SENDELL", lMESSAGE_SENDELL),
	/*0x5F*/ MAPFUNC("ANIM_SET", lANIM_SET),
	/*0x60*/ MAPFUNC("HOLOMAP_TRAJ", lHOLOMAP_TRAJ),
	/*0x61*/ MAPFUNC("GAME_OVER", lGAME_OVER),
	/*0x62*/ MAPFUNC("THE_END", lTHE_END),
	/*0x63*/ MAPFUNC("MIDI_OFF", lMIDI_OFF),
	/*0x64*/ MAPFUNC("PLAY_CD_TRACK", lPLAY_CD_TRACK),
	/*0x65*/ MAPFUNC("PROJ_ISO", lPROJ_ISO),
	/*0x66*/ MAPFUNC("PROJ_3D", lPROJ_3D),
	/*0x67*/ MAPFUNC("TEXT", lTEXT),
	/*0x68*/ MAPFUNC("CLEAR_TEXT", lCLEAR_TEXT),
	/*0x69*/ MAPFUNC("BRUTAL_EXIT", lBRUTAL_EXIT)
};

/** Process actor move script
	@param actorIdx Current processed actor index */
void process_life_script(int32 actorIdx) {
	int32 end, scriptOpcode;
	ActorStruct *actor;

	actor = &sceneActors[actorIdx];
	scriptPtr = actor->lifeScript + actor->positionInLifeScript;

	end = -2;

	do {
		opcodePtr	   = scriptPtr;
		scriptOpcode   = *(scriptPtr++);

		if (scriptOpcode <= 105) {
			end = function_map[scriptOpcode].function(actorIdx, actor);
		} else {
			printf("ERROR: Actor %d with wrong offset/opcode - Offset: %d\n", actorIdx, actor->positionInLifeScript);
		}

		if (end < 0) { // show error message
			printf("Actor %d Life script [%s] not implemented\n", actorIdx, function_map[scriptOpcode].name);
		}

	} while(end != 1);
}
#ifndef ENEMY_H
#define ENEMY_H


#include "raylib.h"
#include "raymath.h"
#include "constants.h"

typedef enum EnemyJumpState {
    ENEMY_STATE_GROUNDED,
    ENEMY_STATE_ONE_JUMP,
    ENEMY_STATE_DOUBLE_JUMP
} EnemyJumpState;

typedef enum EnemyState {
    ENEMY_DEAD,
    ENEMY_ALIVE
} EnemyState;


typedef struct Enemy {
    Vector2 pos;
    Vector2 vel;
    Vector2 accel;
    EnemyJumpState jumpState;
    EnemyState obostha;
} Enemy;

static inline Enemy InitEnemy(void) {
    Enemy p;
    p.pos = (Vector2){ ENEMY_START_X, ENEMY_START_Y };
    p.vel = (Vector2){ 0.0f, 0.0f };
    p.accel = (Vector2){ 0.0f, 0.0f };
    p.jumpState = STATE_GROUNDED;
    p.obostha = Alive;
    return p;
}

#endif
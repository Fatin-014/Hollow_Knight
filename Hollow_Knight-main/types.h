#ifndef TYPES_H
#define TYPES_H

#include "raylib.h"
#include <stdbool.h>

typedef enum GameState
{
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_VICTORY,
    STATE_INSTRUCTIONS,
    STATE_CREDITS
} GameState;

typedef enum EnemyAnimState
{
    ENEMY_ANIM_IDLE,
    ENEMY_ANIM_RUN,
    ENEMY_ANIM_HIT,
    ENEMY_ANIM_ATTACK,
    ENEMY_ANIM_DEATH
} EnemyAnimState;

typedef enum PlayerAnimState
{
    PLAYER_ANIM_IDLE,
    PLAYER_ANIM_RUN,
    PLAYER_ANIM_JUMP,
    PLAYER_ANIM_FALL,
    PLAYER_ANIM_DASH,
    PLAYER_ANIM_ATTACK1,
    PLAYER_ANIM_ATTACK2,
    PLAYER_ANIM_HURT,
    PLAYER_ANIM_DEATH
} PlayerAnimState;

typedef struct Enemy
{
    Rectangle rec;
    float speed;
    bool active;        // still part of the level (updated + drawn)
    bool facingRight;
    float minX;
    float maxX;
    EnemyAnimState animState;
    int currentFrame;
    float frameTimer;
} Enemy;

//holds each goblin animation strip + how many frames it contains, loaded once and shared by every goblin
typedef struct EnemyAnimSet
{
    Texture2D idle;   int idleFrames;
    Texture2D run;    int runFrames;
    Texture2D hit;    int hitFrames;
    Texture2D attack; int attackFrames;
    Texture2D death;  int deathFrames;
} EnemyAnimSet;

//holds every knight animation strip + its frame count
typedef struct PlayerAnimSet
{
    Texture2D idle;    int idleFrames;
    Texture2D run;     int runFrames;
    Texture2D jump;    int jumpFrames;
    Texture2D fall;    int fallFrames;
    Texture2D dash;    int dashFrames;
    Texture2D attack1; int attack1Frames;
    Texture2D attack2; int attack2Frames;
    Texture2D hurt;    int hurtFrames;
    Texture2D death;   int deathFrames;
} PlayerAnimSet;

typedef struct Player
{
    Rectangle rec;
} Player;

//shared across files because it takes/returns these types
void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW);

#endif //TYPES_H

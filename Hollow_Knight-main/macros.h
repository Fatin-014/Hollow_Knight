#ifndef MACROS_H
#define MACROS_H

#define ABSOLUTE_MAX_ENEMIES 12
#define screenWidth 1280
#define screenHeight 600
#define GROUND_LEVEL 560.0f
#define GRAVITY 900.0f
#define JUMP_FORCE -450.0f
#define PLAYER_SPEED 250.0f
#define INVINCIBILITY_DURATION 1.0f
#define SPRITE_SCALE 3.0f
#define SPRITE_OFFSET_X -30.0f
#define SPRITE_OFFSET_Y 180.0f
#define ENEMY_OFFSET_Y -12.0f
#define ENEMY_FRAME_TIME (1.0f/10.0f) //how fast goblin animation strips advance
#define PLAYER_FRAME_TIME (1.0f/12.0f) //how fast idle/run/jump/fall/dash/hurt/death advance
#define DASH_SPEED 700.0f
#define DASH_DURATION 0.2f   //how long the dash's forward slide lasts
#define DASH_COOLDOWN 0.5f   //time before you can dash again

// attack fixing
#define PLAYER_COLLISION_WIDTH 40.0f
#define PLAYER_COLLISION_HEIGHT 80.0f
#define PLAYER_OFFSET_X 173.0f
#define PLAYER_OFFSET_Y 10.0f
#define ENEMY_COLLISION_WIDTH 45.0f
#define ENEMY_COLLISION_HEIGHT 75.0f
#define ENEMY_OFFSET_X 15.0f
#define ENEMY_COLLISION_OFFSET_Y 10.0f
#define PLAYER_ATTACK_FORWARD_OFFSET -15.0f
#define ATTACK1_HIT_START_FRAME 3
#define ATTACK1_HIT_END_FRAME 5
#define ATTACK2_HIT_START_FRAME 4
#define ATTACK2_HIT_END_FRAME 7
#define ATTACK1_DURATION 0.4f
#define ATTACK2_DURATION 0.6f

#endif //MACROS_H

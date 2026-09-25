#include "raylib.h"
#include "raymath.h"
#include <math.h>

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
#define PLAYER_FRAME_TIME (1.0f/12.0f) //how fast the knight's animation strips advance
#define DASH_SPEED 700.0f
#define DASH_DURATION 0.2f   //how long the dash's forward slide lasts
#define DASH_COOLDOWN 0.5f   //time before you can dash again

typedef enum GameState
{
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_VICTORY
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

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW);

int main()
{
    InitWindow(screenWidth, screenHeight, "HOLLOW KNIGHT");
    SetTargetFPS(60);
    GameState currentState=STATE_MENU;
    int selectedOption=0;
    float verticalVelocity=0.0f;
    bool isGrounded=true;
    Player player={0};
    player.rec=(Rectangle){100.0f, GROUND_LEVEL-64.0f, 64.0f, 64.0f};

    int playerHealth=5;
    const int maxPlayerHealth=5;
    bool isInvincible=false;
    float invincibilityTimer=0.0f;
    const float invincibilityDuration=INVINCIBILITY_DURATION;
    bool facingRight=true;

    //player animation state
    PlayerAnimState playerAnimState=PLAYER_ANIM_IDLE;
    int playerFrame=0;
    float playerFrameTimer=0.0f;

    //attack
    bool isAttacking=false;
    float attackTimer=0.0f;
    float currentAttackDuration=0.0f;
    int currentAttackType=1; //1 = left mouse (Attack 1.png), 2 = right mouse (Attack 2.png)

    //dash
    bool isDashing=false;
    float dashTimer=0.0f;
    float dashCooldownTimer=0.0f;

    //hurt reaction
    bool isHurt=false;
    float hurtTimer=0.0f;

    //death
    bool deathAnimStarted=false;

    int jumpcount=0;

    //level up
    int currentLevel=1;
    int activeEnemyCount=0;
    Enemy enemies[ABSOLUTE_MAX_ENEMIES]={0};

    //--- goblin enemy animations ---
    //fix: copy these from the asset pack into assets/enemies/goblin/ in the project folder
    EnemyAnimSet goblinAnim={0};
    goblinAnim.idle=LoadTexture("assets/enemies/goblin/goblin_idle_anim_strip_4.png");     goblinAnim.idleFrames=4;
    goblinAnim.run=LoadTexture("assets/enemies/goblin/goblin_run_anim_strip_6.png");       goblinAnim.runFrames=6;
    goblinAnim.hit=LoadTexture("assets/enemies/goblin/goblin_hit_anim_strip_3.png");       goblinAnim.hitFrames=3;
    goblinAnim.attack=LoadTexture("assets/enemies/goblin/goblin_attack_anim_strip_4.png"); goblinAnim.attackFrames=4;
    goblinAnim.death=LoadTexture("assets/enemies/goblin/goblin_death_anim_strip_6.png");   goblinAnim.deathFrames=6;

    //--- player animations ---
    //fix: copy these from D:\Hollow_Knight-try-this\gamerunfolder\assets\player\ into assets/player/ in the project folder
    PlayerAnimSet playerAnim={0};
    playerAnim.idle=LoadTexture("assets/player/Idle.png");         playerAnim.idleFrames=7;
    playerAnim.run=LoadTexture("assets/player/Run.png");           playerAnim.runFrames=8;
    playerAnim.jump=LoadTexture("assets/player/Jump.png");         playerAnim.jumpFrames=4;
    playerAnim.fall=LoadTexture("assets/player/Fall.png");         playerAnim.fallFrames=4;
    playerAnim.dash=LoadTexture("assets/player/Dash.png");         playerAnim.dashFrames=12;
    playerAnim.hurt=LoadTexture("assets/player/Hurt.png");         playerAnim.hurtFrames=3;
    playerAnim.attack1=LoadTexture("assets/player/Attack 1.png");  playerAnim.attack1Frames=10;
    playerAnim.attack2=LoadTexture("assets/player/Attack 2.png");  playerAnim.attack2Frames=15;
    playerAnim.death=LoadTexture("assets/player/Death.png");
    //frame count wasn't certain for Death, so estimate it from the strip's aspect ratio (assumes square frames,
    //which is common for this kind of asset pack) - check the debug overlay below and hardcode the real number if it looks off
    playerAnim.deathFrames=(playerAnim.death.height>0)?(int)roundf((float)playerAnim.death.width/(float)playerAnim.death.height):16;
    if(playerAnim.deathFrames<=0) playerAnim.deathFrames=16;

    const float hurtAnimDuration=playerAnim.hurtFrames*PLAYER_FRAME_TIME;

    // Level Backgrounds
    Texture2D bgTextureLvl1=LoadTexture("assets/bg.png");
    Texture2D bgTextureLvl2=LoadTexture("assets/bg2.png");
    Texture2D bgTextureLvl3=LoadTexture("assets/bg3.png");
    Texture2D currentBgTexture=bgTextureLvl1;

    //level 1 shuru
    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);

    while(!WindowShouldClose())
    {
        float deltaTime=GetFrameTime();
        //main screen
        if(currentState==STATE_MENU)
        {
            if(IsKeyPressed(KEY_DOWN)||IsKeyPressed(KEY_S)) selectedOption=1;
            if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_W)) selectedOption=0;
            if(IsKeyPressed(KEY_ENTER))
            {
                if(selectedOption==0) currentState=STATE_GAMEPLAY;
                else if(selectedOption==1) break;
            }
        }
        else if(currentState==STATE_GAMEPLAY)
        {
            if(playerHealth<=0)
            {
                if(!deathAnimStarted)
                {
                    deathAnimStarted=true;
                    playerAnimState=PLAYER_ANIM_DEATH;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }
                //keep the death animation playing (holds on its last frame) while we wait for a restart
                playerFrameTimer+=deltaTime;
                if(playerFrameTimer>=PLAYER_FRAME_TIME)
                {
                    playerFrameTimer=0.0f;
                    if(playerFrame<playerAnim.deathFrames-1) playerFrame++;
                }

                if(IsKeyPressed(KEY_R))
                {
                    playerHealth=maxPlayerHealth;
                    currentLevel=1;
                    currentBgTexture=bgTextureLvl1;
                    player.rec.x=100.0f;
                    player.rec.y=GROUND_LEVEL-64.0f;
                    verticalVelocity=0.0f;
                    isAttacking=false;
                    isDashing=false;
                    dashTimer=0.0f;
                    dashCooldownTimer=0.0f;
                    isHurt=false;
                    hurtTimer=0.0f;
                    deathAnimStarted=false;
                    playerAnimState=PLAYER_ANIM_IDLE;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                }
            }
            else
            {
                if(isInvincible)
                {
                    invincibilityTimer-=deltaTime;
                    if(invincibilityTimer<=0.0f) isInvincible=false;
                }
                if(isHurt)
                {
                    hurtTimer-=deltaTime;
                    if(hurtTimer<=0.0f) isHurt=false;
                }
                if(dashCooldownTimer>0.0f) dashCooldownTimer-=deltaTime;

                bool isMoving=false;

                //dash trigger (Shift)
                if((IsKeyPressed(KEY_LEFT_SHIFT)||IsKeyPressed(KEY_RIGHT_SHIFT))&&!isAttacking&&!isDashing&&dashCooldownTimer<=0.0f)
                {
                    isDashing=true;
                    dashTimer=DASH_DURATION;
                }

                if(isDashing)
                {
                    //flat dash: no normal move input, slides in the facing direction
                    player.rec.x+=(facingRight?1.0f:-1.0f)*DASH_SPEED*deltaTime;
                    dashTimer-=deltaTime;
                    if(dashTimer<=0.0f)
                    {
                        isDashing=false;
                        dashCooldownTimer=DASH_COOLDOWN;
                    }
                }
                else if(!isAttacking)
                {
                    if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))
                    {
                        player.rec.x+=PLAYER_SPEED*deltaTime;
                        facingRight=true;
                        isMoving=true;
                    }
                    if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))
                    {
                        player.rec.x-=PLAYER_SPEED*deltaTime;
                        facingRight=false;
                        isMoving=true;
                    }
                }

                if(player.rec.x<0) player.rec.x=0;
                if(player.rec.x+player.rec.width>screenWidth) player.rec.x=screenWidth-player.rec.width;

                if(!isDashing) //fix: dash ignores gravity for a flat slide, Hollow Knight-style
                {
                    verticalVelocity+=GRAVITY*deltaTime;
                    player.rec.y+=verticalVelocity*deltaTime;
                }
                if(player.rec.y>=GROUND_LEVEL-player.rec.height)
                {
                    player.rec.y=GROUND_LEVEL-player.rec.height;
                    verticalVelocity=0.0f;
                    isGrounded=true;
                }
                else isGrounded=false;
                if(isGrounded)
                {
                    jumpcount=0; //grounded hoile 0
                }
                if((IsKeyPressed(KEY_SPACE)||IsKeyPressed(KEY_W))&&jumpcount<=1&&!isAttacking&&!isDashing)
                {
                    verticalVelocity=JUMP_FORCE;
                    isGrounded=false;
                    jumpcount+=1;
                }

                //attack triggers: left mouse = Attack 1, right mouse = Attack 2
                if((IsKeyPressed(KEY_J)||IsMouseButtonPressed(MOUSE_BUTTON_LEFT))&&!isAttacking&&!isDashing)
                {
                    isAttacking=true;
                    currentAttackType=1;
                    currentAttackDuration=playerAnim.attack1Frames*PLAYER_FRAME_TIME;
                    attackTimer=currentAttackDuration;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }
                else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)&&!isAttacking&&!isDashing)
                {
                    isAttacking=true;
                    currentAttackType=2;
                    currentAttackDuration=playerAnim.attack2Frames*PLAYER_FRAME_TIME;
                    attackTimer=currentAttackDuration;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }

                //attack and damage
                if(isAttacking)
                {
                    attackTimer-=deltaTime;

                    float attackRange=150.0f;
                    float playerCenterX=player.rec.x+(player.rec.width/2.0f);
                    Rectangle attackBox=facingRight?(Rectangle){ playerCenterX, player.rec.y-20.0f, attackRange, player.rec.height+40.0f }
                        :(Rectangle){ playerCenterX-attackRange, player.rec.y-20.0f, attackRange, player.rec.height+40.0f };

                    for(int i=0;i<activeEnemyCount;i++)
                    {
                        bool alreadyDying=(enemies[i].animState==ENEMY_ANIM_HIT||enemies[i].animState==ENEMY_ANIM_DEATH);
                        if(enemies[i].active&&!alreadyDying&&CheckCollisionRecs(attackBox, enemies[i].rec))
                        {
                            enemies[i].animState=ENEMY_ANIM_HIT;
                            enemies[i].currentFrame=0;
                            enemies[i].frameTimer=0.0f;
                        }
                    }
                    if(attackTimer<=0.0f)isAttacking=false;
                }

                //enemy auto
                for(int i=0; i<activeEnemyCount; i++)
                {
                    Enemy *e=&enemies[i];
                    if(!e->active) continue;

                    if(e->animState==ENEMY_ANIM_DEATH)
                    {
                        e->frameTimer+=deltaTime;
                        if(e->frameTimer>=ENEMY_FRAME_TIME)
                        {
                            e->frameTimer=0.0f;
                            e->currentFrame++;
                            if(e->currentFrame>=goblinAnim.deathFrames) e->active=false;
                        }
                        continue;
                    }

                    if(e->animState==ENEMY_ANIM_HIT)
                    {
                        e->frameTimer+=deltaTime;
                        if(e->frameTimer>=ENEMY_FRAME_TIME)
                        {
                            e->frameTimer=0.0f;
                            e->currentFrame++;
                            if(e->currentFrame>=goblinAnim.hitFrames)
                            {
                                e->animState=ENEMY_ANIM_DEATH;
                                e->currentFrame=0;
                                e->frameTimer=0.0f;
                            }
                        }
                        continue;
                    }

                    if(e->animState==ENEMY_ANIM_ATTACK)
                    {
                        e->frameTimer+=deltaTime;
                        if(e->frameTimer>=ENEMY_FRAME_TIME)
                        {
                            e->frameTimer=0.0f;
                            e->currentFrame++;
                            if(e->currentFrame>=goblinAnim.attackFrames)
                            {
                                e->animState=ENEMY_ANIM_RUN;
                                e->currentFrame=0;
                                e->frameTimer=0.0f;
                            }
                        }
                    }
                    else
                    {
                        e->animState=ENEMY_ANIM_RUN;
                        if(e->facingRight)
                        {
                            e->rec.x+=e->speed*deltaTime;
                            if(e->rec.x>=e->maxX) e->facingRight=false;
                        }
                        else
                        {
                            e->rec.x-=e->speed*deltaTime;
                            if(e->rec.x<=e->minX) e->facingRight=true;
                        }
                        e->frameTimer+=deltaTime;
                        if(e->frameTimer>=ENEMY_FRAME_TIME)
                        {
                            e->frameTimer=0.0f;
                            e->currentFrame++;
                            if(e->currentFrame>=goblinAnim.runFrames) e->currentFrame=0;
                        }
                    }

                    if(!isInvincible&&!isDashing&&e->animState!=ENEMY_ANIM_ATTACK)
                    {
                        float pCenterX=player.rec.x+(player.rec.width/2.0f);
                        float pCenterY=player.rec.y+(player.rec.height/2.0f);
                        float eCenterX=e->rec.x+(e->rec.width/2.0f);
                        float eCenterY=e->rec.y+(e->rec.height/2.0f);
                        float deltaX=fabsf(pCenterX-eCenterX);
                        float deltaY=fabsf(pCenterY-eCenterY);
                        if(deltaX<50.0f&&deltaY<45.0f)
                        {
                            playerHealth--;
                            isInvincible=true;
                            invincibilityTimer=invincibilityDuration;
                            isHurt=true;
                            hurtTimer=hurtAnimDuration;
                            playerFrame=0;
                            playerFrameTimer=0.0f;
                            if(pCenterX<eCenterX) player.rec.x-=40.0f;
                            else player.rec.x+=40.0f;
                            verticalVelocity=-200.0f;
                            e->animState=ENEMY_ANIM_ATTACK;
                            e->currentFrame=0;
                            e->frameTimer=0.0f;
                        }
                    }
                }

                //pick which player animation should be playing right now
                PlayerAnimState newAnimState;
                if(isAttacking) newAnimState=(currentAttackType==1)?PLAYER_ANIM_ATTACK1:PLAYER_ANIM_ATTACK2;
                else if(isDashing) newAnimState=PLAYER_ANIM_DASH;
                else if(isHurt) newAnimState=PLAYER_ANIM_HURT;
                else if(!isGrounded) newAnimState=(verticalVelocity<0.0f)?PLAYER_ANIM_JUMP:PLAYER_ANIM_FALL;
                else if(isMoving) newAnimState=PLAYER_ANIM_RUN;
                else newAnimState=PLAYER_ANIM_IDLE;

                if(newAnimState!=playerAnimState)
                {
                    playerAnimState=newAnimState;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }

                int curPlayerFrameCount=1;
                switch(playerAnimState)
                {
                    case PLAYER_ANIM_IDLE:    curPlayerFrameCount=playerAnim.idleFrames;    break;
                    case PLAYER_ANIM_RUN:     curPlayerFrameCount=playerAnim.runFrames;     break;
                    case PLAYER_ANIM_JUMP:    curPlayerFrameCount=playerAnim.jumpFrames;    break;
                    case PLAYER_ANIM_FALL:    curPlayerFrameCount=playerAnim.fallFrames;    break;
                    case PLAYER_ANIM_DASH:    curPlayerFrameCount=playerAnim.dashFrames;    break;
                    case PLAYER_ANIM_ATTACK1: curPlayerFrameCount=playerAnim.attack1Frames; break;
                    case PLAYER_ANIM_ATTACK2: curPlayerFrameCount=playerAnim.attack2Frames; break;
                    case PLAYER_ANIM_HURT:    curPlayerFrameCount=playerAnim.hurtFrames;    break;
                    case PLAYER_ANIM_DEATH:   curPlayerFrameCount=playerAnim.deathFrames;   break;
                }
                if(curPlayerFrameCount<1) curPlayerFrameCount=1;

                playerFrameTimer+=deltaTime;
                if(playerFrameTimer>=PLAYER_FRAME_TIME)
                {
                    playerFrameTimer=0.0f;
                    playerFrame++;
                    bool loopingAnim=(playerAnimState==PLAYER_ANIM_IDLE||playerAnimState==PLAYER_ANIM_RUN||
                                       playerAnimState==PLAYER_ANIM_JUMP||playerAnimState==PLAYER_ANIM_FALL);
                    if(playerFrame>=curPlayerFrameCount)
                    {
                        playerFrame=loopingAnim?0:(curPlayerFrameCount-1); //hold last frame for one-shot anims
                    }
                }

                //level par korar part
                bool allEnemiesDefeated=true;
                for(int i=0;i<activeEnemyCount;i++)
                {
                    if(enemies[i].active)
                    {
                        allEnemiesDefeated=false;
                        break;
                    }
                }
                if(allEnemiesDefeated)
                {
                    if(currentLevel>=3)
                    {
                        currentState=STATE_VICTORY;
                    }
                    else
                    {
                        currentLevel++;
                        player.rec.x=50.0f;
                        if(currentLevel==2&&bgTextureLvl2.id!=0) currentBgTexture=bgTextureLvl2;
                        else if(currentLevel==3&&bgTextureLvl3.id!=0) currentBgTexture=bgTextureLvl3;
                        SpawnLevelEnemies(enemies, currentLevel,&activeEnemyCount,GROUND_LEVEL,screenWidth);
                    }
                }
            }
        }
        else if(currentState==STATE_VICTORY)
        {
            if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_R))
            {
                currentLevel=1;
                playerHealth=maxPlayerHealth;
                currentBgTexture=bgTextureLvl1;
                player.rec.x=100.0f;
                player.rec.y=GROUND_LEVEL-64.0f;
                verticalVelocity=0.0f;
                isAttacking=false;
                isDashing=false;
                dashTimer=0.0f;
                dashCooldownTimer=0.0f;
                isHurt=false;
                hurtTimer=0.0f;
                deathAnimStarted=false;
                playerAnimState=PLAYER_ANIM_IDLE;
                playerFrame=0;
                playerFrameTimer=0.0f;
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                currentState=STATE_GAMEPLAY;
            }
        }

        BeginDrawing();
        ClearBackground((Color){20,20,30,255});
        if(currentState==STATE_MENU)
        {
            int title_l=MeasureText("HOLLOW KNIGHT",55);
            DrawText("HOLLOW KNIGHT",(screenWidth-title_l)/2,160,55,GOLD);
            Color opt1Color=(selectedOption==0)?YELLOW:GRAY;
            const char*opt1Text=(selectedOption==0)?"> Start New Game <":"Start New Game";
            DrawText(opt1Text,(screenWidth-MeasureText(opt1Text,28))/2,270,28,opt1Color);
            Color opt2Color=(selectedOption==1)?YELLOW:GRAY;
            const char*opt2Text=(selectedOption==1)?"> Exit <":"Exit";
            DrawText(opt2Text,(screenWidth-MeasureText(opt2Text,28))/2,320,28,opt2Color);
        }
        else if(currentState==STATE_GAMEPLAY)
        {
            //background
            if(currentBgTexture.id!=0)
            {
                Rectangle bgSource={0.0f, 0.0f, (float)currentBgTexture.width, (float)currentBgTexture.height};
                Rectangle bgDest={0.0f, 0.0f, (float)screenWidth, (float)screenHeight};
                DrawTexturePro(currentBgTexture, bgSource, bgDest, (Vector2){0, 0}, 0.0f, WHITE);
            }
            Color playerTint=(isInvincible&&((int)(invincibilityTimer*10)%2==0))?RED:WHITE;

            //main character, drawn from whichever strip matches playerAnimState
            Texture2D pTex; int pFrameCount;
            switch(playerAnimState)
            {
                case PLAYER_ANIM_IDLE:    pTex=playerAnim.idle;    pFrameCount=playerAnim.idleFrames;    break;
                case PLAYER_ANIM_RUN:     pTex=playerAnim.run;     pFrameCount=playerAnim.runFrames;     break;
                case PLAYER_ANIM_JUMP:    pTex=playerAnim.jump;    pFrameCount=playerAnim.jumpFrames;    break;
                case PLAYER_ANIM_FALL:    pTex=playerAnim.fall;    pFrameCount=playerAnim.fallFrames;    break;
                case PLAYER_ANIM_DASH:    pTex=playerAnim.dash;    pFrameCount=playerAnim.dashFrames;    break;
                case PLAYER_ANIM_ATTACK1: pTex=playerAnim.attack1; pFrameCount=playerAnim.attack1Frames; break;
                case PLAYER_ANIM_ATTACK2: pTex=playerAnim.attack2; pFrameCount=playerAnim.attack2Frames; break;
                case PLAYER_ANIM_HURT:    pTex=playerAnim.hurt;    pFrameCount=playerAnim.hurtFrames;    break;
                case PLAYER_ANIM_DEATH:   pTex=playerAnim.death;   pFrameCount=playerAnim.deathFrames;   break;
                default:                  pTex=playerAnim.idle;    pFrameCount=playerAnim.idleFrames;    break;
            }
            if(pTex.id!=0&&pFrameCount>0)
            {
                float frameW=(float)pTex.width/(float)pFrameCount;
                float frameH=(float)pTex.height;
                int frame=playerFrame%pFrameCount;
                float srcW=facingRight?frameW:-frameW;
                Rectangle sourceRec={ frame*frameW, 0.0f, srcW, frameH };
                float renderWidth=frameW*SPRITE_SCALE;
                float renderHeight=frameH*SPRITE_SCALE;
                Rectangle destRec={ player.rec.x+SPRITE_OFFSET_X, player.rec.y+player.rec.height-renderHeight+SPRITE_OFFSET_Y, renderWidth, renderHeight };
                DrawTexturePro(pTex, sourceRec, destRec, (Vector2){0,0}, 0.0f, playerTint);
            }

            //enemy akaaki
            for(int i=0;i<activeEnemyCount;i++)
            {
                if(!enemies[i].active) continue;

                Texture2D tex; int frameCount;
                switch(enemies[i].animState)
                {
                    case ENEMY_ANIM_IDLE:   tex=goblinAnim.idle;   frameCount=goblinAnim.idleFrames;   break;
                    case ENEMY_ANIM_RUN:    tex=goblinAnim.run;    frameCount=goblinAnim.runFrames;    break;
                    case ENEMY_ANIM_HIT:    tex=goblinAnim.hit;    frameCount=goblinAnim.hitFrames;    break;
                    case ENEMY_ANIM_ATTACK: tex=goblinAnim.attack; frameCount=goblinAnim.attackFrames; break;
                    case ENEMY_ANIM_DEATH:  tex=goblinAnim.death;  frameCount=goblinAnim.deathFrames;  break;
                    default:                tex=goblinAnim.idle;   frameCount=goblinAnim.idleFrames;   break;
                }

                if(tex.id!=0&&frameCount>0)
                {
                    float frameW=(float)tex.width/(float)frameCount;
                    float frameH=(float)tex.height;
                    int frame=enemies[i].currentFrame%frameCount;
                    float srcW=enemies[i].facingRight?frameW:-frameW;
                    Rectangle src={ frame*frameW, 0.0f, srcW, frameH };
                    Rectangle dest={ enemies[i].rec.x, enemies[i].rec.y+ENEMY_OFFSET_Y, enemies[i].rec.width, enemies[i].rec.height };
                    DrawTexturePro(tex,src,dest,(Vector2){0, 0},0.0f,WHITE);
                }
            }

            DrawText(TextFormat("LEVEL %d/3",currentLevel),10,10,22,YELLOW);
            for(int i=0;i<maxPlayerHealth;i++)
            {
                Color heartColor=(i<playerHealth)?RED:DARKGRAY;
                DrawRectangle(160+(i*25),10,20,20,heartColor);
                DrawRectangleLines(160+(i*25),10,20,20,WHITE);
            }
            if(dashCooldownTimer>0.0f)
            {
                DrawText(TextFormat("Dash: %.1fs", dashCooldownTimer), 10, 40, 18, SKYBLUE);
            }
            else
            {
                DrawText("Dash: ready", 10, 40, 18, SKYBLUE);
            }
            if(playerHealth<=0)
            {
                DrawText("GAME OVER!!Press R to Restart",screenWidth/2-200,screenHeight/2,28,RED);
            }

            //--- TEMP DEBUG: remove once goblins are confirmed visible ---
            //id==0 means LoadTexture failed for that file (wrong path / file not copied / working directory issue)
            DrawText(TextFormat("goblin idle : id=%d  %dx%d", goblinAnim.idle.id,   goblinAnim.idle.width,   goblinAnim.idle.height),   10, 460, 16, goblinAnim.idle.id  ?LIME:RED);
            DrawText(TextFormat("goblin run  : id=%d  %dx%d", goblinAnim.run.id,    goblinAnim.run.width,    goblinAnim.run.height),    10, 478, 16, goblinAnim.run.id   ?LIME:RED);
            DrawText(TextFormat("goblin hit  : id=%d  %dx%d", goblinAnim.hit.id,    goblinAnim.hit.width,    goblinAnim.hit.height),    10, 496, 16, goblinAnim.hit.id   ?LIME:RED);
            DrawText(TextFormat("goblin atk  : id=%d  %dx%d", goblinAnim.attack.id, goblinAnim.attack.width, goblinAnim.attack.height), 10, 514, 16, goblinAnim.attack.id?LIME:RED);
            DrawText(TextFormat("goblin death: id=%d  %dx%d", goblinAnim.death.id,  goblinAnim.death.width,  goblinAnim.death.height),  10, 532, 16, goblinAnim.death.id ?LIME:RED);
            DrawText(TextFormat("active enemies: %d", activeEnemyCount), 10, 550, 16, LIME);
            DrawText(TextFormat("player idle=%d run=%d jump=%d fall=%d dash=%d atk1=%d atk2=%d hurt=%d death=%d",
                playerAnim.idle.id, playerAnim.run.id, playerAnim.jump.id, playerAnim.fall.id, playerAnim.dash.id,
                playerAnim.attack1.id, playerAnim.attack2.id, playerAnim.hurt.id, playerAnim.death.id), 250, 550, 14, LIME);
            //--- END TEMP DEBUG ---
        }
        else if(currentState==STATE_VICTORY)
        {
            const char*winText="VICTORY! YOU CLEARED ALL 3 LEVELS!";
            int winWidth=MeasureText(winText,32);
            DrawText(winText,(screenWidth-winWidth)/2,220,32,GOLD);
            const char*subText="Press ENTER or R to Play Again";
            int subWidth=MeasureText(subText,20);
            DrawText(subText,(screenWidth-subWidth)/2,300,20,RAYWHITE);
        }
        EndDrawing();
    }

    UnloadTexture(bgTextureLvl1);
    UnloadTexture(bgTextureLvl2);
    UnloadTexture(bgTextureLvl3);

    UnloadTexture(playerAnim.idle);
    UnloadTexture(playerAnim.run);
    UnloadTexture(playerAnim.jump);
    UnloadTexture(playerAnim.fall);
    UnloadTexture(playerAnim.dash);
    UnloadTexture(playerAnim.hurt);
    UnloadTexture(playerAnim.attack1);
    UnloadTexture(playerAnim.attack2);
    UnloadTexture(playerAnim.death);

    UnloadTexture(goblinAnim.idle);
    UnloadTexture(goblinAnim.run);
    UnloadTexture(goblinAnim.hit);
    UnloadTexture(goblinAnim.attack);
    UnloadTexture(goblinAnim.death);

    CloseWindow();
    return 0;
}

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW)
{
    *activeCount=6+(level-1)*2;
    if(*activeCount>ABSOLUTE_MAX_ENEMIES)*activeCount=ABSOLUTE_MAX_ENEMIES;
    float speedBoost=(level-1)*30.0f;
    float zoneMinX[3]={ 300.0f, 600.0f, 900.0f }; //enemy er norar jayga
    float zoneMaxX[3]={ 500.0f, 800.0f, 1150.0f };
    for(int i=0; i<*activeCount; i++)
    {
        int groupIndex=i%3;
        float minX=zoneMinX[groupIndex];
        float maxX=zoneMaxX[groupIndex];
        float spawnOffset=(i/3)*60.0f;
        float startX=minX+20.0f+spawnOffset;
        if(startX>maxX-64.0f) startX=maxX-64.0f;
        float enemySpeed=(100.0f+speedBoost)+((i%2)*20.0f);
        bool startFacingRight=(i%2==0);
        enemies[i]=(Enemy){(Rectangle){ startX, groundLevel-64.0f, 64.0f, 64.0f },
            enemySpeed,
            true,
            startFacingRight,
            minX,
            maxX,
            ENEMY_ANIM_RUN,
            0,
            0.0f
        };
    }
    (void)screenW; //currently unused, kept for future spawn logic that scales with screen width
}

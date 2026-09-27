#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include<stddef.h>
#include<stdio.h>

#define MAX_NAME_LEN 16
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
#define DASH_COOLDOWN 3.0f   //time before you can dash again
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



typedef enum GameState
{
    STATE_MENU,
    STATE_GAMEPLAY,
    STATE_VICTORY,
    STATE_INSTRUCTIONS,
    STATE_CREDITS,
    STATE_DIFFICULTY,
    STATE_NAME_ENTRY,
    STATE_HIGHSCORES
} GameState;
char playerName[MAX_NAME_LEN+1]="";
int nameLetterCount=0;
bool scoreSaved=false;
typedef enum Difficulty
{
    DIFF_EASY,
    DIFF_MEDIUM,
    DIFF_HARD
}Difficulty;

float difficultyScoreMultiplier[3]={1.0f, 1.5f, 2.0f};

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

typedef struct HighScoreEntry
{
    char name[MAX_NAME_LEN+1];
    int score;
} HighScoreEntry;

void SaveScore(const char* name, int score)
{
    FILE *f=fopen("highscores.txt","a");
    if(f!=NULL)
    {
        fprintf(f,"%s,%d\n",name,score);
        fclose(f);
    }
}

int LoadTopScores(HighScoreEntry *outEntries, int maxEntries)
{
    FILE *f=fopen("highscores.txt","r");
    if(f==NULL) return 0;
    HighScoreEntry temp[256];
    int count=0;
    while(count<256&&fscanf(f,"%16[^,],%d\n",temp[count].name,&temp[count].score)==2)
    {
        count++;
    }
    fclose(f);
    for(int i=0;i<count-1;i++)         
    {
        for(int j=0;j<count-1-i;j++)
        {
            if(temp[j].score<temp[j+1].score)
            {
                HighScoreEntry tmp=temp[j];
                temp[j]=temp[j+1];
                temp[j+1]=tmp;
            }
        }
    }
    int n=(count<maxEntries)?count:maxEntries;
    for(int i=0;i<n;i++) outEntries[i]=temp[i];
    return n;
}

int main()
{
    InitWindow(screenWidth, screenHeight, "HOLLOW KNIGHT");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    InitAudioDevice();
    GameState currentState=STATE_MENU;
    int selectedOption=0;
    float verticalVelocity=0.0f;
    bool isGrounded=true;
    Player player={0};
    player.rec=(Rectangle){100.0f, GROUND_LEVEL-64.0f, 64.0f, 64.0f};

    int playerHealth=5;
    int score=0;
    Difficulty selectedDifficulty=DIFF_MEDIUM;
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
    EnemyAnimSet goblinAnim={0};
    goblinAnim.idle=LoadTexture("assets/enemies/goblin/goblin_idle_anim_strip_4.png");     goblinAnim.idleFrames=4;
    goblinAnim.run=LoadTexture("assets/enemies/goblin/goblin_run_anim_strip_6.png");       goblinAnim.runFrames=6;
    goblinAnim.hit=LoadTexture("assets/enemies/goblin/goblin_hit_anim_strip_3.png");       goblinAnim.hitFrames=3;
    goblinAnim.attack=LoadTexture("assets/enemies/goblin/goblin_attack_anim_strip_4.png"); goblinAnim.attackFrames=4;
    goblinAnim.death=LoadTexture("assets/enemies/goblin/goblin_death_anim_strip_6.png");   goblinAnim.deathFrames=6;

    //--- player animations ---
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
    Texture2D menubg=LoadTexture("assets/menubg.png");
    Font myfont=LoadFontEx("assets/themefont.TTF",100,NULL,0);
    Sound clicksound=LoadSound("assets/audio/clicksound.mp3");
    Music gamemusic=LoadMusicStream("assets/audio/Hollow Knight OST - Sealed Vessel.mp3");
    Texture2D currentBgTexture=bgTextureLvl1;
    bool soundOn=true;
    
    char* start="START GAME";
    char* exit="EXIT GAME";
    char* instruction="Instructions";
    char* credits="Credits";
    char* easy="EASY";
    char* medium="MEDIUM";
    char* hard="HARD";
    char* highscores="High Scores";

    Vector2 sizeHighscores=MeasureTextEx(myfont,highscores,40,2);
    Vector2 sizeStart=MeasureTextEx(myfont,start,40,2);
    Vector2 sizeExit=MeasureTextEx(myfont,exit,40,2);
    Vector2 sizeinstructions=MeasureTextEx(myfont,instruction,40,2);
    Vector2 sizecredits=MeasureTextEx(myfont,credits,40,2);
    Vector2 sizeEasy=MeasureTextEx(myfont,easy,40,2);
    Vector2 sizeMedium=MeasureTextEx(myfont,medium,40,2);
    Vector2 sizeHard=MeasureTextEx(myfont,hard,40,2);
    
    Rectangle highscorebtn={screenWidth/2-sizeHighscores.x/2,500,sizeHighscores.x,sizeHighscores.y};
    Rectangle easybtn={screenWidth/2-sizeEasy.x/2,250,sizeEasy.x,sizeEasy.y};
    Rectangle mediumbtn={screenWidth/2-sizeMedium.x/2,320,sizeMedium.x,sizeMedium.y};
    Rectangle hardbtn={screenWidth/2-sizeHard.x/2,390,sizeHard.x,sizeHard.y};
    Rectangle startbtn={screenWidth/2-sizeStart.x/2,250,sizeStart.x,sizeStart.y};
    Rectangle exitbtn={screenWidth/2-sizeExit.x/2,300,sizeExit.x,sizeExit.y};
    Rectangle instrbtn={screenWidth/2-sizeinstructions.x/2,350,sizeinstructions.x,sizeinstructions.y};
    Rectangle creditbtn={screenWidth/2-sizecredits.x/2,400,sizecredits.x,sizecredits.y};

    //level 1 shuru
    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);

    while(!WindowShouldClose())
    {
        float deltaTime=GetFrameTime();
        //main screen
        Vector2 mousepos=GetMousePosition();
        char* soundLabel=soundOn?"Sound: ON":"Sound: OFF";
        Vector2 sizeSound=MeasureTextEx(myfont,soundLabel,40,2);
        Rectangle soundbtn={screenWidth/2-sizeSound.x/2,450,sizeSound.x,sizeSound.y};
        if(currentState==STATE_MENU)
        {
            if(CheckCollisionPointRec(mousepos,startbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                currentState=STATE_NAME_ENTRY;
            }
            if(CheckCollisionPointRec(mousepos,exitbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                break;
            }
            if(CheckCollisionPointRec(mousepos,instrbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                currentState=STATE_INSTRUCTIONS;
            }
            if(CheckCollisionPointRec(mousepos,creditbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                currentState=STATE_CREDITS;
            }
            if(CheckCollisionPointRec(mousepos,soundbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                soundOn=!soundOn;
                SetMasterVolume(soundOn?1:0);
                if(soundOn) PlaySound(clicksound);
            }
            if(CheckCollisionPointRec(mousepos,highscorebtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                currentState=STATE_HIGHSCORES;
            }
        }
        else if(currentState==STATE_HIGHSCORES)
        {
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_NAME_ENTRY)
        {
            int key=GetCharPressed();
            while(key>0)
            {
                if((key>=32)&&(key<=125)&&nameLetterCount<MAX_NAME_LEN)
                {
                    playerName[nameLetterCount]=(char)key;
                    playerName[nameLetterCount+1]='\0';
                    nameLetterCount++;
                }
                key=GetCharPressed();
            }
            if(IsKeyPressed(KEY_BACKSPACE)&&nameLetterCount>0)
            {
                nameLetterCount--;
                playerName[nameLetterCount]='\0';
            }
            if(IsKeyPressed(KEY_ENTER)&&nameLetterCount>0)
            {
                PlaySound(clicksound);
                currentState=STATE_DIFFICULTY;
            }
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_INSTRUCTIONS)
        {
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_CREDITS)
        {
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_DIFFICULTY)
        {
            if(CheckCollisionPointRec(mousepos,easybtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                selectedDifficulty=DIFF_EASY;
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
            }
            if(CheckCollisionPointRec(mousepos,mediumbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                selectedDifficulty=DIFF_MEDIUM;
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
            }
            if(CheckCollisionPointRec(mousepos,hardbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                selectedDifficulty=DIFF_HARD;
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
            }
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_GAMEPLAY)
        {
            UpdateMusicStream(gamemusic);
            if(playerHealth<=0)
            {
                if(!deathAnimStarted)
                {
                    deathAnimStarted=true;
                    playerAnimState=PLAYER_ANIM_DEATH;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                    if(!scoreSaved)
                    { 
                        SaveScore(playerName,score); 
                        scoreSaved=true; 
                    }
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
                    scoreSaved=false;
                    score=0;               
                    playerName[0]='\0';     
                    nameLetterCount=0;               
                    playerAnimState=PLAYER_ANIM_IDLE;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth);
                    currentState=STATE_MENU;
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
                    //currentAttackDuration=playerAnim.attack1Frames*PLAYER_FRAME_TIME;
                    currentAttackDuration=ATTACK1_DURATION;
                    attackTimer=currentAttackDuration;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }
                else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)&&!isAttacking&&!isDashing)
                {
                    isAttacking=true;
                    currentAttackType=2;
                    //currentAttackDuration=playerAnim.attack2Frames*PLAYER_FRAME_TIME;
                    currentAttackDuration=ATTACK2_DURATION;
                    attackTimer=currentAttackDuration;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }

                //attack and damage
                if(isAttacking)
                {
                    attackTimer-=deltaTime;

                    int hitStartFrame=(currentAttackType==1)?ATTACK1_HIT_START_FRAME:ATTACK2_HIT_START_FRAME;
                    int hitEndFrame=(currentAttackType==1)?ATTACK1_HIT_END_FRAME:ATTACK2_HIT_END_FRAME;
                    bool inHitWindow=(playerFrame>=hitStartFrame&&playerFrame<=hitEndFrame);

                    if(inHitWindow)
                    {
                        float attackRange=70.0f;
                        Rectangle playerCollisionRec = {
                            player.rec.x + PLAYER_OFFSET_X,
                            player.rec.y + PLAYER_OFFSET_Y,
                            PLAYER_COLLISION_WIDTH,
                            PLAYER_COLLISION_HEIGHT
                        };
                        Rectangle attackBox=facingRight?
                            (Rectangle){ playerCollisionRec.x+playerCollisionRec.width+PLAYER_ATTACK_FORWARD_OFFSET, playerCollisionRec.y, attackRange, playerCollisionRec.height }
                            :(Rectangle){ playerCollisionRec.x-attackRange-PLAYER_ATTACK_FORWARD_OFFSET, playerCollisionRec.y, attackRange, playerCollisionRec.height };
                        for(int i=0;i<activeEnemyCount;i++)
                        {
                            bool alreadyDying=(enemies[i].animState==ENEMY_ANIM_HIT||enemies[i].animState==ENEMY_ANIM_DEATH);
                            Rectangle enemyCollisionRec = {
                                enemies[i].rec.x + ENEMY_OFFSET_X,
                                enemies[i].rec.y + ENEMY_COLLISION_OFFSET_Y,
                                ENEMY_COLLISION_WIDTH,
                                ENEMY_COLLISION_HEIGHT
                            };
                            if(enemies[i].active&&!alreadyDying&&CheckCollisionRecs(attackBox, enemyCollisionRec))
                            {
                                enemies[i].animState=ENEMY_ANIM_HIT;
                                enemies[i].currentFrame=0;
                                enemies[i].frameTimer=0.0f;
                                score+=(int)(10*difficultyScoreMultiplier[selectedDifficulty]);
                            }
                        }
                    }
                    if(attackTimer<=0.0f)
                    {
                        isAttacking=false;
                    }
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
                            Rectangle playerCollisionRec = {
                                player.rec.x + PLAYER_OFFSET_X,
                                player.rec.y + PLAYER_OFFSET_Y,
                                PLAYER_COLLISION_WIDTH,
                                PLAYER_COLLISION_HEIGHT
                            };
                            Rectangle enemyCollisionRec = {
                                e->rec.x + ENEMY_OFFSET_X,
                                e->rec.y + ENEMY_COLLISION_OFFSET_Y,
                                ENEMY_COLLISION_WIDTH,
                                ENEMY_COLLISION_HEIGHT
                            };
                            if(CheckCollisionRecs(playerCollisionRec, enemyCollisionRec))
                            {
                                playerHealth--;
                                isInvincible=true;
                                invincibilityTimer=invincibilityDuration;
                                isHurt=true;
                                hurtTimer=hurtAnimDuration;
                                playerFrame=0;
                                playerFrameTimer=0.0f;
                                if(player.rec.x<e->rec.x) player.rec.x-=40.0f;
                                else player.rec.x+=40.0f;
                                verticalVelocity=-200.0f;
                                e->animState=ENEMY_ANIM_ATTACK;
                                e->currentFrame=0;
                                e->frameTimer=0.0f; 
                            }
                        }
                }

                //picking which player animation should be playing right now
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
                        score+=(int)(100*difficultyScoreMultiplier[selectedDifficulty]);
                        if(!scoreSaved)
                        {
                            SaveScore(playerName,score);
                            scoreSaved=true;
                        }
                        currentState=STATE_VICTORY;
                    }
                    else
                    {
                        score+=(int)(50*difficultyScoreMultiplier[selectedDifficulty]);
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
                scoreSaved=false;               
                playerName[0]='\0';     
                nameLetterCount=0;  
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
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Main Menu",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Main Menu",80,2).x/2,screenHeight/2-150},80,2,(Color){48,120,148,255});
            Color colstart=CheckCollisionPointRec(mousepos,startbtn)?GREEN:RED;
            DrawTextEx(myfont,start,(Vector2){startbtn.x,startbtn.y},40,2,colstart);
            Color colexit=CheckCollisionPointRec(mousepos,exitbtn)?GREEN:RED;
            DrawTextEx(myfont,exit,(Vector2){exitbtn.x,exitbtn.y},40,2,colexit); 
            Color colinstr=CheckCollisionPointRec(mousepos,instrbtn)?GREEN:RED;
            DrawTextEx(myfont,instruction,(Vector2){instrbtn.x,instrbtn.y},40,2,colinstr);
            Color colcred=CheckCollisionPointRec(mousepos,creditbtn)?GREEN:RED;
            DrawTextEx(myfont,credits,(Vector2){creditbtn.x,creditbtn.y},40,2,colcred);
            Color colsound=CheckCollisionPointRec(mousepos,soundbtn)?GREEN:RED;
            DrawTextEx(myfont,soundLabel,(Vector2){soundbtn.x,soundbtn.y},40,2,colsound);
            Color colhighscore=CheckCollisionPointRec(mousepos,highscorebtn)?GREEN:RED;
            DrawTextEx(myfont,highscores,(Vector2){highscorebtn.x,highscorebtn.y},40,2,colhighscore);
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
                    Rectangle playerCollisionRecDraw = {
                        player.rec.x + PLAYER_OFFSET_X,
                        player.rec.y + PLAYER_OFFSET_Y,
                        PLAYER_COLLISION_WIDTH,
                        PLAYER_COLLISION_HEIGHT
                    };
                    for(int i=0;i<activeEnemyCount;i++)
                    {
                        if(!enemies[i].active) continue;
                        Rectangle enemyCollisionRecDraw = {
                            enemies[i].rec.x + ENEMY_OFFSET_X,
                            enemies[i].rec.y + ENEMY_COLLISION_OFFSET_Y,
                            ENEMY_COLLISION_WIDTH,
                            ENEMY_COLLISION_HEIGHT
                        };
                    }

            DrawTextEx(myfont,TextFormat("LEVEL %d/3",currentLevel),(Vector2){10,10},22,2,YELLOW);
            DrawTextEx(myfont,TextFormat("SCORE: %d",score),(Vector2){10,90},22,2,YELLOW);
            for(int i=0;i<maxPlayerHealth;i++)
            {
                Color heartColor=(i<playerHealth)?RED:DARKGRAY;
                DrawRectangle(160+(i*25),10,20,20,heartColor);
                DrawRectangleLines(160+(i*25),10,20,20,WHITE);
            }
            const char* diffLabel=(selectedDifficulty==DIFF_EASY)?"EASY":(selectedDifficulty==DIFF_HARD)?"HARD":"MEDIUM";
            DrawTextEx(myfont,TextFormat("Difficulty: %s",diffLabel),(Vector2){10,65},18,2,ORANGE);
            if(dashCooldownTimer>0.0f)
            {
                DrawTextEx(myfont,TextFormat("Dash: %.1fs", dashCooldownTimer),(Vector2){ 10, 40}, 18,2, SKYBLUE);
            }
            else
            {
                DrawTextEx(myfont,"Dash: ready",(Vector2){10, 40}, 18,2, SKYBLUE);
            }
            if(playerHealth<=0)
            {
                DrawTextEx(myfont,"GAME OVER!!! Press R to go back",(Vector2){screenWidth/2-200,screenHeight/2},28,2,RED);
            }
        }
        else if(currentState==STATE_INSTRUCTIONS)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Instructions",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Instructions",60,2).x/2,100},60,2,(Color){48,120,148,255});
            DrawTextEx(myfont,"Move: A/D or Arrow Keys",(Vector2){200, 220},22,2,RAYWHITE);
            DrawTextEx(myfont,"Jump: SPACE or W (double jump available)",(Vector2){200,255},22,2, RAYWHITE);
            DrawTextEx(myfont,"Dash: LEFT SHIFT or RIGHT SHIFT", (Vector2){200, 290}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Attack 1: J or LEFT MOUSE BUTTON", (Vector2){200, 325}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Attack 2: RIGHT MOUSE BUTTON",(Vector2) {200, 360}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Defeat all enemies to clear each level!", (Vector2){200, 410}, 22,2, YELLOW);
            DrawTextEx(myfont,"Press ESC to return to menu", (Vector2){screenWidth/2-220, screenHeight-60}, 20,2, GRAY);
        }
        else if(currentState==STATE_CREDITS)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Credits",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Credits",60,2).x/2,100},60,2,(Color){48,120,148,255});
            DrawTextEx(myfont,"Game design & programming:DANIEL & FATIN", (Vector2){250, 230}, 30,2, (Color){125,18,44,255});
            DrawTextEx(myfont,"Music: Hollow Knight OST - Sealed Vessel", (Vector2){250, 265}, 30,2, (Color){125,18,44,255});
            DrawTextEx(myfont,"Sprites: Craftpix & Itch.io and other open sources",(Vector2) {250, 300}, 30,2, (Color){125,18,44,255});
            DrawTextEx(myfont,"Made with raylib", (Vector2){250, 335}, 30,2, (Color){125,18,44,255});
            DrawTextEx(myfont,"Press ESC to return to menu",(Vector2) {screenWidth/2-220, screenHeight-60}, 20,2, GRAY);
        }
        else if(currentState==STATE_DIFFICULTY)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Choose Difficulty",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Choose Difficulty",60,2).x/2,120},60,2,(Color){48,120,148,255});
            Color colEasy=CheckCollisionPointRec(mousepos,easybtn)?GREEN:RED;
            DrawTextEx(myfont,easy,(Vector2){easybtn.x,easybtn.y},40,2,colEasy);
            Color colMedium=CheckCollisionPointRec(mousepos,mediumbtn)?GREEN:RED;
            DrawTextEx(myfont,medium,(Vector2){mediumbtn.x,mediumbtn.y},40,2,colMedium);
            Color colHard=CheckCollisionPointRec(mousepos,hardbtn)?GREEN:RED;
            DrawTextEx(myfont,hard,(Vector2){hardbtn.x,hardbtn.y},40,2,colHard);
            DrawTextEx(myfont,"Press ESC to go back",(Vector2){screenWidth/2-130,screenHeight-60},20,2,GRAY);
        }
        else if(currentState=STATE_HIGHSCORES)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"High Scores",(Vector2){screenWidth/2-MeasureTextEx(myfont,"High Scores",60,2).x/2,100},60,2,(Color){48,120,148,255});

            HighScoreEntry topScores[5];
            int topCount=LoadTopScores(topScores,5);
            if(topCount==0)
            {
                DrawTextEx(myfont,"No scores yet - play a game!",(Vector2){screenWidth/2-220,250},24,2,GRAY);
            }
            else
            {
                for(int i=0;i<topCount;i++)
                {
                    DrawTextEx(myfont,TextFormat("%d. %s - %d",i+1,topScores[i].name,topScores[i].score),
                        (Vector2){screenWidth/2-180,230+i*45},28,2,RAYWHITE);
                }
            }
            DrawTextEx(myfont,"Press ESC to return to menu",(Vector2){screenWidth/2-220,screenHeight-60},20,2,GRAY);
        }
        else if(currentState==STATE_NAME_ENTRY)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Enter Your Name",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Enter Your Name",60,2).x/2,120},60,2,(Color){48,120,148,255});
            Rectangle nameBox={screenWidth/2-200,260,400,50};
            DrawRectangleRec(nameBox,(Color){30,30,40,255});
            DrawRectangleLinesEx(nameBox,2,SKYBLUE);
            DrawTextEx(myfont,playerName,(Vector2){nameBox.x+10,nameBox.y+8},30,2,WHITE);
            if(((int)(GetTime()*2)%2)==0)   // blinking cursor
            {
                float cursorX=nameBox.x+10+MeasureTextEx(myfont,playerName,30,2).x+4;
                DrawTextEx(myfont,"|",(Vector2){cursorX,nameBox.y+8},30,2,WHITE);
            }
            DrawTextEx(myfont,"Press ENTER to continue",(Vector2){screenWidth/2-180,340},20,2,GRAY);
            DrawTextEx(myfont,"Press ESC to go back",(Vector2){screenWidth/2-180,screenHeight-60},20,2,GRAY);
        }
        else if(currentState==STATE_VICTORY)
        {
            const char*winText="VICTORY! YOU CLEARED ALL 3 LEVELS!";
            int winWidth=MeasureText(winText,32);
            DrawText(winText,(screenWidth-winWidth)/2,220,32,GOLD);
            const char*subText="Press ENTER or R to Play Again";
            int subWidth=MeasureText(subText,20);
            DrawText(subText,(screenWidth-subWidth)/2,300,20,RAYWHITE);
            const char*scoreText=TextFormat("Final Score: %d",score);
            int scoreWidth=MeasureText(scoreText,24);
            DrawTextEx(myfont,scoreText,(Vector2){(screenWidth-scoreWidth)/2,340},24,2,YELLOW);
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

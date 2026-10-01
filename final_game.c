#include "raylib.h"
#include "raymath.h"
#include <stddef.h>
#include <stdio.h>
#define LEVEL_INTRO_DURATION 2.5f
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
#define PLAYER_FRAME_TIME (1.0f/12.0f) //how fast idle/run/jump/fall/dash/hurt/death advance
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

// rooms / levels
#define ROOM_EXIT_ZONE_WIDTH 30.0f   //strip along the room's right edge the player's body must touch, once every enemy is dead, to move on
#define MAX_LEVEL 6      //rooms 1-5 have goblins, room 6 (BOSS_LEVEL) is the demon boss
#define BOSS_LEVEL 6

// boss (demon) - sprites are separate numbered PNGs per frame, not one strip
#define MAX_ANIM_FRAMES 24        //big enough to hold the 22-frame death animation
#define BOSS_MAX_HEALTH 8
#define BOSS_FRAME_TIME (1.0f/10.0f)
#define BOSS_ATTACK_RANGE_BASE 75.0f       //detection margin: how far beyond EACH side of the boss's body the player can be and still get noticed (+ difficulty bonus)
#define BOSS_WINDUP_SPEED_MULT 2.0f        //frames before BOSS_HIT_START_FRAME (the windup) play this many times faster than the rest of the swing
#define BOSS_SPAWN_COOLDOWN 1.0f           //short grace period after the boss room loads before its first swing
#define BOSS_ATTACK_COOLDOWN_BASE 1.0f     //time after a swing before the boss can attack again, before difficulty speed-up
#define BOSS_HIT_START_FRAME 6             //frame range of demon_cleave that can actually hit the player
#define BOSS_HIT_END_FRAME 9
#define BOSS_COLLISION_WIDTH 70.0f     //hurtbox: where your attacks have to land (mirrored when the boss faces left)
#define BOSS_COLLISION_HEIGHT 130.0f
#define BOSS_OFFSET_X 40.0f
#define BOSS_COLLISION_OFFSET_Y 10.0f
//the boss's symmetric "body" (boss.rec minus these insets) is what blocks the player and what the
//detection range is measured from - it never needs mirroring
#define BOSS_BODY_INSET_X 0.0f
#define BOSS_BODY_INSET_Y 0.0f
#define BOSS_ATTACK_FORWARD_OFFSET 0.0f
#define BOSS_ATTACK_RANGE_BOX_BASE 90.0f   //actual reach of the boss's swing, before difficulty bonus
#define BOSS_SPRITE_SCALE 2.5f
#define BOSS_SPRITE_DEFAULT_FACES_RIGHT false
#define BOSS_SPRITE_OFFSET_X 0.0f
#define BOSS_SPRITE_OFFSET_Y 0.0f
#define BOSS_REC_WIDTH 80.0f
#define BOSS_REC_HEIGHT 140.0f

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
int difficultyMaxEnemies[3]={6, 8, 10};
float enemySpeedMultiplier[3]={1.0f, 1.5f, 2.0f};
float bossRangeBonus[3]={0.0f, 20.0f, 40.0f};
float bossAttackSpeedMultiplier[3]={1.0f, 1.2f, 1.5f};
int difficultyEnemyHealth[3]={1,2,3};

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

typedef enum BossAnimState
{
    BOSS_ANIM_IDLE,
    BOSS_ANIM_CLEAVE, //its attack
    BOSS_ANIM_HIT,
    BOSS_ANIM_DEATH
} BossAnimState;

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
    int health;
    int maxHealth;
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

//the demon's sprites come as separate numbered files per frame (demon_idle_1.png, _2.png, ...)
//instead of one strip, so each animation is just an array of individually-loaded textures
typedef struct AnimFrames
{
    Texture2D frames[MAX_ANIM_FRAMES];
    int count;
} AnimFrames;

typedef struct BossAnimSet
{
    AnimFrames idle;
    AnimFrames cleave;
    AnimFrames hit;
    AnimFrames death;
} BossAnimSet;

typedef struct Boss
{
    Rectangle rec;
    bool active;
    int health;
    bool facingRight;
    BossAnimState animState;
    int currentFrame;
    float frameTimer;
    float attackCooldownTimer;
} Boss;

typedef struct Player
{
    Rectangle rec;
} Player;

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW, Difficulty difficulty);
void SpawnBoss(Boss *boss, float groundLevel, int screenW);
void LoadAnimFrames(AnimFrames *anim, const char *folder, const char *prefix, int count);
Rectangle GetBossBody(const Boss *boss);

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

  //levelintro
    bool showLevelIntro=false;
    float levelIntroTimer=0;
    int introLevelNumber=1;
void StartLevelIntro(int level)
{
    showLevelIntro=true;
    levelIntroTimer=LEVEL_INTRO_DURATION;
    introLevelNumber=level;
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
    player.rec=(Rectangle){75.0f, GROUND_LEVEL-64.0f, 64.0f, 64.0f};

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
    float attackFrameTime=PLAYER_FRAME_TIME; //fix: how fast THIS attack's frames advance, so the whole strip fits inside currentAttackDuration
    int currentAttackType=1; //1 = Q / left mouse (Attack 1.png), 2 = E / right mouse (Attack 2.png)

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
    bool showHitboxes=false; //toggled with T - draws every collision/attack rectangle in the boss room and normal rooms alike

    //level up
    int currentLevel=1;
    int activeEnemyCount=0;
    Enemy enemies[ABSOLUTE_MAX_ENEMIES]={0};

    //boss
    Boss boss={0};
    boss.health=BOSS_MAX_HEALTH;
    boss.animState=BOSS_ANIM_IDLE;

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

    //--- boss (demon) animations ---
    //fix: copy the assets/demon folder (with its 01_demon_idle, 03_demon_cleave, etc subfolders) into your
    //project's assets/demon folder, keeping the exact same subfolder and file names. Walk sprites aren't used.
    BossAnimSet bossAnim={0};
    LoadAnimFrames(&bossAnim.idle,   "01_demon_idle",     "demon_idle",     6);
    LoadAnimFrames(&bossAnim.cleave, "03_demon_cleave",   "demon_cleave",   15);
    LoadAnimFrames(&bossAnim.hit,    "04_demon_take_hit", "demon_take_hit", 5);
    LoadAnimFrames(&bossAnim.death,  "05_demon_death",    "demon_death",    22);

    // Level Backgrounds - rooms 1-5 (goblins) + a boss-room background
    Texture2D bgTextureLvl1=LoadTexture("assets/bg.png");
    Texture2D bgTextureLvl2=LoadTexture("assets/bg2.png");
    Texture2D bgTextureLvl3=LoadTexture("assets/bg3.png");
    Texture2D bgTextureLvl4=LoadTexture("assets/bg4.png");
    Texture2D bgTextureLvl5=LoadTexture("assets/bg5.png");
    Texture2D bgTextureBoss=LoadTexture("assets/bg_boss.png");
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
    
    Rectangle highscorebtn={screenWidth/2-sizeHighscores.x/2,300,sizeHighscores.x,sizeHighscores.y};
    Rectangle easybtn={screenWidth/2-sizeEasy.x/2,250,sizeEasy.x,sizeEasy.y};
    Rectangle mediumbtn={screenWidth/2-sizeMedium.x/2,320,sizeMedium.x,sizeMedium.y};
    Rectangle hardbtn={screenWidth/2-sizeHard.x/2,390,sizeHard.x,sizeHard.y};
    Rectangle startbtn={screenWidth/2-sizeStart.x/2,250,sizeStart.x,sizeStart.y};
    Rectangle exitbtn={screenWidth/2-sizeExit.x/2,500,sizeExit.x,sizeExit.y};
    Rectangle instrbtn={screenWidth/2-sizeinstructions.x/2,350,sizeinstructions.x,sizeinstructions.y};
    Rectangle creditbtn={screenWidth/2-sizecredits.x/2,400,sizecredits.x,sizecredits.y};

    //level 1 shuru
    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);

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
                //fix: room 1 was always spawned at startup with the default (medium) difficulty and never
                //re-spawned when a difficulty was actually picked, so easy/hard never applied to room 1
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
                StartLevelIntro(currentLevel);
            }
            if(CheckCollisionPointRec(mousepos,mediumbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                selectedDifficulty=DIFF_MEDIUM;
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
                StartLevelIntro(currentLevel);
            }
            if(CheckCollisionPointRec(mousepos,hardbtn)&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                PlaySound(clicksound);
                selectedDifficulty=DIFF_HARD;
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);
                PlayMusicStream(gamemusic);
                currentState=STATE_GAMEPLAY;
                StartLevelIntro(currentLevel);
            }
            if(IsKeyPressed(KEY_ESCAPE))
            {
                currentState=STATE_MENU;
            }
        }
        else if(currentState==STATE_GAMEPLAY)
        {
            UpdateMusicStream(gamemusic);
            if(IsKeyPressed(KEY_T)) showHitboxes=!showHitboxes;
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
                    boss.active=false;
                    boss.health=BOSS_MAX_HEALTH;
                    boss.animState=BOSS_ANIM_IDLE;
                    boss.currentFrame=0;
                    boss.frameTimer=0.0f;
                    boss.attackCooldownTimer=0.0f;
                    SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);
                    currentState=STATE_MENU;
                }
            }
            else
            {
                if(showLevelIntro)
                {
                    levelIntroTimer-=deltaTime;
                    if(levelIntroTimer<=0)
                    {
                        showLevelIntro=false;
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
                //keep the knight's actual body (not the 64x64 player.rec, which sits ~173px to its left) inside the room on the right,
                //so the "end of the room" is where the body really touches the edge
                if(player.rec.x+PLAYER_OFFSET_X+PLAYER_COLLISION_WIDTH>screenWidth) player.rec.x=screenWidth-PLAYER_OFFSET_X-PLAYER_COLLISION_WIDTH;
                if(currentLevel==BOSS_LEVEL&&boss.active)
                {
                    //fix: compare the player's actual body box (player.rec is ~173px left of where the knight is
                    //drawn) against the boss's body box, otherwise the player got stopped inside the boss on the
                    //left and ~170px short of it on the right
                    Rectangle playerBodyBlock={ player.rec.x+PLAYER_OFFSET_X, player.rec.y+PLAYER_OFFSET_Y, PLAYER_COLLISION_WIDTH, PLAYER_COLLISION_HEIGHT };
                    Rectangle bossBodyBlock=GetBossBody(&boss);
                    if(CheckCollisionRecs(playerBodyBlock, bossBodyBlock))
                    {
                        float playerBodyMid=playerBodyBlock.x+playerBodyBlock.width/2.0f;
                        float bossBodyMid=bossBodyBlock.x+bossBodyBlock.width/2.0f;
                        if(playerBodyMid<bossBodyMid)
                        {
                            player.rec.x=bossBodyBlock.x-PLAYER_OFFSET_X-PLAYER_COLLISION_WIDTH;          // push back to the left edge
                        }
                        else
                        {
                            player.rec.x=bossBodyBlock.x+bossBodyBlock.width-PLAYER_OFFSET_X;             // push back to the right edge
                        }
                    }
                }
                if(!isDashing) //dash ignores gravity for a flat slide, Hollow Knight-style
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

                //attack triggers: Q or left mouse = Attack 1, E or right mouse = Attack 2
                if((IsKeyPressed(KEY_Q)||IsMouseButtonPressed(MOUSE_BUTTON_LEFT))&&!isAttacking&&!isDashing)
                {
                    isAttacking=true;
                    currentAttackType=1;
                    currentAttackDuration=ATTACK1_DURATION;
                    //fix: play all attack1Frames within ATTACK1_DURATION instead of the fixed global PLAYER_FRAME_TIME,
                    //which was cutting the strip off early (same root cause as the attack2 bug you flagged)
                    attackFrameTime=currentAttackDuration/(float)playerAnim.attack1Frames;
                    attackTimer=currentAttackDuration;
                    playerFrame=0;
                    playerFrameTimer=0.0f;
                }
                else if((IsKeyPressed(KEY_E)||IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))&&!isAttacking&&!isDashing)
                {
                    isAttacking=true;
                    currentAttackType=2;
                    currentAttackDuration=ATTACK2_DURATION;
                    //fix: same idea - play all attack2Frames within ATTACK2_DURATION instead of getting cut off
                    attackFrameTime=currentAttackDuration/(float)playerAnim.attack2Frames;
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
                                enemies[i].health--;
                                enemies[i].animState=ENEMY_ANIM_HIT;
                                enemies[i].currentFrame=0;
                                enemies[i].frameTimer=0.0f;
                                if(enemies[i].health<=0)
                                {
                                    score+=(int)(10*difficultyScoreMultiplier[selectedDifficulty]);
                                }    
                            }
                        }
                        //same attack box also lands on the boss when we're in the boss room
                        if(currentLevel==BOSS_LEVEL&&boss.active&&boss.animState!=BOSS_ANIM_HIT&&boss.animState!=BOSS_ANIM_DEATH)
                        {
                            //mirror the offset when the boss faces left, same as its sprite does
                            Rectangle bossCollisionRec = boss.facingRight?
                                (Rectangle){ boss.rec.x+BOSS_OFFSET_X, boss.rec.y+BOSS_COLLISION_OFFSET_Y, BOSS_COLLISION_WIDTH, BOSS_COLLISION_HEIGHT }
                                :(Rectangle){ boss.rec.x+boss.rec.width-BOSS_OFFSET_X-BOSS_COLLISION_WIDTH, boss.rec.y+BOSS_COLLISION_OFFSET_Y, BOSS_COLLISION_WIDTH, BOSS_COLLISION_HEIGHT };
                            if(CheckCollisionRecs(attackBox, bossCollisionRec))
                            {
                                boss.health--;
                                boss.currentFrame=0;
                                boss.frameTimer=0.0f;
                                boss.animState=(boss.health<=0)?BOSS_ANIM_DEATH:BOSS_ANIM_HIT;
                                if(boss.health>0)
                                {
                                    //if the boss was mid-swing and got interrupted, it never reached the natural
                                    //end-of-cleave cooldown assignment - give it a fresh cooldown here so hitting
                                    //it back-to-back doesn't let every future attack fire instantly
                                    float curBossCooldown=BOSS_ATTACK_COOLDOWN_BASE/bossAttackSpeedMultiplier[selectedDifficulty];
                                    boss.attackCooldownTimer=curBossCooldown;
                                }
                                score+=(int)(15*difficultyScoreMultiplier[selectedDifficulty]);
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
                                if(e->health<=0)
                                {
                                    e->animState=ENEMY_ANIM_DEATH;
                                }
                                else
                                {
                                    e->animState=ENEMY_ANIM_RUN;
                                }
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

                //--- boss update (only relevant in the boss room) ---
                if(currentLevel==BOSS_LEVEL&&boss.active)
                {
                    float curBossAttackRange=BOSS_ATTACK_RANGE_BASE+bossRangeBonus[selectedDifficulty];
                    float curBossAttackRangeBox=BOSS_ATTACK_RANGE_BOX_BASE+bossRangeBonus[selectedDifficulty];
                    float curBossFrameTime=BOSS_FRAME_TIME/bossAttackSpeedMultiplier[selectedDifficulty]; //faster swing on higher difficulty
                    float curBossCooldown=BOSS_ATTACK_COOLDOWN_BASE/bossAttackSpeedMultiplier[selectedDifficulty]; //attacks more often too

                    //fix: always track the player's relative position to decide which way the boss faces
                    //(and therefore which way it attacks), in every state except while a swing is actively
                    //playing - freezing only during CLEAVE stops it spinning around mid-attack, but idle,
                    //hit-reaction and death should still turn to keep facing you
                    //fix: use the player's BODY center (player.rec.x is ~173px left of where the knight actually is),
                    //otherwise the boss turned to face the wrong way whenever you were standing close to it
                    Rectangle bossBodyNow=GetBossBody(&boss);
                    Rectangle playerBodyNow={ player.rec.x+PLAYER_OFFSET_X, player.rec.y+PLAYER_OFFSET_Y, PLAYER_COLLISION_WIDTH, PLAYER_COLLISION_HEIGHT };
                    float bossBodyCenterX=bossBodyNow.x+bossBodyNow.width/2.0f;
                    float playerBodyCenterX=playerBodyNow.x+playerBodyNow.width/2.0f;
                    if(boss.animState!=BOSS_ANIM_CLEAVE)
                    {
                        boss.facingRight=(playerBodyCenterX>=bossBodyCenterX);
                    }

                    //detection zone: the boss's body plus curBossAttackRange pixels on BOTH sides
                    bool playerDetected=(playerBodyNow.x<bossBodyNow.x+bossBodyNow.width+curBossAttackRange)
                                      &&(playerBodyNow.x+playerBodyNow.width>bossBodyNow.x-curBossAttackRange);

                    if(boss.animState==BOSS_ANIM_DEATH)
                    {
                        boss.frameTimer+=deltaTime;
                        if(boss.frameTimer>=BOSS_FRAME_TIME)
                        {
                            boss.frameTimer=0.0f;
                            boss.currentFrame++;
                            if(boss.currentFrame>=bossAnim.death.count) boss.active=false; //boss fully gone -> level-clear check below sees this
                        }
                    }
                    else if(boss.animState==BOSS_ANIM_HIT)
                    {
                        boss.frameTimer+=deltaTime;
                        if(boss.frameTimer>=BOSS_FRAME_TIME)
                        {
                            boss.frameTimer=0.0f;
                            boss.currentFrame++;
                            if(boss.currentFrame>=bossAnim.hit.count)
                            {
                                boss.animState=BOSS_ANIM_IDLE;
                                boss.currentFrame=0;
                                boss.frameTimer=0.0f;
                            }
                        }
                    }
                    else if(boss.animState==BOSS_ANIM_CLEAVE)
                    {
                        //faster windup: the frames before the hit window play BOSS_WINDUP_SPEED_MULT times faster
                        float cleaveFrameTime=(boss.currentFrame<BOSS_HIT_START_FRAME)?(curBossFrameTime/BOSS_WINDUP_SPEED_MULT):curBossFrameTime;
                        boss.frameTimer+=deltaTime;
                        if(boss.frameTimer>=cleaveFrameTime)
                        {
                            boss.frameTimer=0.0f;
                            boss.currentFrame++;
                            if(boss.currentFrame>=bossAnim.cleave.count)
                            {
                                boss.animState=BOSS_ANIM_IDLE;
                                boss.currentFrame=0;
                                boss.frameTimer=0.0f;
                                boss.attackCooldownTimer=curBossCooldown;
                            }
                        }
                        //the boss's own hit window against the player - dodge by moving/dashing out of the box
                        bool bossInHitWindow=(boss.currentFrame>=BOSS_HIT_START_FRAME&&boss.currentFrame<=BOSS_HIT_END_FRAME);
                        if(bossInHitWindow&&!isInvincible&&!isDashing)
                        {
                            Rectangle playerCollisionRec = {
                                player.rec.x + PLAYER_OFFSET_X,
                                player.rec.y + PLAYER_OFFSET_Y,
                                PLAYER_COLLISION_WIDTH,
                                PLAYER_COLLISION_HEIGHT
                            };
                            Rectangle bossAttackBox=boss.facingRight?
                                (Rectangle){ boss.rec.x+boss.rec.width+BOSS_ATTACK_FORWARD_OFFSET, boss.rec.y, curBossAttackRangeBox, boss.rec.height }
                                :(Rectangle){ boss.rec.x-curBossAttackRangeBox-BOSS_ATTACK_FORWARD_OFFSET, boss.rec.y, curBossAttackRangeBox, boss.rec.height };
                            if(CheckCollisionRecs(bossAttackBox, playerCollisionRec))
                            {
                                playerHealth--;
                                isInvincible=true;
                                invincibilityTimer=invincibilityDuration;
                                isHurt=true;
                                hurtTimer=hurtAnimDuration;
                                playerFrame=0;
                                playerFrameTimer=0.0f;
                                if(playerBodyCenterX<bossBodyCenterX) player.rec.x-=50.0f;
                                else player.rec.x+=50.0f;
                                verticalVelocity=-200.0f;
                            }
                        }
                    }
                    else
                    {
                        if(boss.attackCooldownTimer>0.0f) boss.attackCooldownTimer-=deltaTime;
                        if(boss.attackCooldownTimer<=0.0f&&playerDetected)
                        {
                            boss.animState=BOSS_ANIM_CLEAVE;
                            boss.currentFrame=0;
                            boss.frameTimer=0.0f;
                        }
                        else
                        {
                            boss.frameTimer+=deltaTime;
                            if(boss.frameTimer>=BOSS_FRAME_TIME)
                            {
                                boss.frameTimer=0.0f;
                                boss.currentFrame++;
                                if(bossAnim.idle.count>0&&boss.currentFrame>=bossAnim.idle.count) boss.currentFrame=0;
                            }
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

                bool isAttackAnim=(playerAnimState==PLAYER_ANIM_ATTACK1||playerAnimState==PLAYER_ANIM_ATTACK2);
                float curFrameTime=isAttackAnim?attackFrameTime:PLAYER_FRAME_TIME;

                playerFrameTimer+=deltaTime;
                if(playerFrameTimer>=curFrameTime)
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
                if(currentLevel==BOSS_LEVEL)
                {
                    //boss room clears the instant the boss's death animation finishes (see boss update above)
                    if(!boss.active)
                    {
                        score+=(int)(200*difficultyScoreMultiplier[selectedDifficulty]);
                        if(!scoreSaved)
                        {
                            SaveScore(playerName,score);
                            scoreSaved=true;
                        }
                        currentState=STATE_VICTORY;
                    }
                }
                else
                {
                    bool allEnemiesDefeated=true;
                    for(int i=0;i<activeEnemyCount;i++)
                    {
                        if(enemies[i].active)
                        {
                            allEnemiesDefeated=false;
                            break;
                        }
                    }
                    //the player's body rectangle (its real width AND height) has to overlap the exit strip at the room's right end
                    Rectangle playerBodyExit={ player.rec.x+PLAYER_OFFSET_X, player.rec.y+PLAYER_OFFSET_Y, PLAYER_COLLISION_WIDTH, PLAYER_COLLISION_HEIGHT };
                    Rectangle roomExitZone={ screenWidth-ROOM_EXIT_ZONE_WIDTH, 0.0f, ROOM_EXIT_ZONE_WIDTH, (float)screenHeight };
                    bool playerAtRoomEnd=CheckCollisionRecs(playerBodyExit, roomExitZone);
                    if(allEnemiesDefeated&&playerAtRoomEnd)
                    {
                        score+=(int)(50*difficultyScoreMultiplier[selectedDifficulty]);
                        currentLevel++;
                        StartLevelIntro(currentLevel);
                        player.rec.x=50.0f;
                        if(currentLevel==2&&bgTextureLvl2.id!=0) currentBgTexture=bgTextureLvl2;
                        else if(currentLevel==3&&bgTextureLvl3.id!=0) currentBgTexture=bgTextureLvl3;
                        else if(currentLevel==4&&bgTextureLvl4.id!=0) currentBgTexture=bgTextureLvl4;
                        else if(currentLevel==5&&bgTextureLvl5.id!=0) currentBgTexture=bgTextureLvl5;
                        else if(currentLevel==BOSS_LEVEL&&bgTextureBoss.id!=0) currentBgTexture=bgTextureBoss;

                        if(currentLevel==BOSS_LEVEL)
                        {
                            activeEnemyCount=0; //no goblins in the boss room
                            SpawnBoss(&boss, GROUND_LEVEL, screenWidth);
                        }
                        else
                        {
                            SpawnLevelEnemies(enemies, currentLevel,&activeEnemyCount,GROUND_LEVEL,screenWidth,selectedDifficulty);
                        }
                    }
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
                boss.active=false;
                boss.health=BOSS_MAX_HEALTH;
                boss.animState=BOSS_ANIM_IDLE;
                boss.currentFrame=0;
                boss.frameTimer=0.0f;
                boss.attackCooldownTimer=0.0f;
                SpawnLevelEnemies(enemies, currentLevel, &activeEnemyCount, GROUND_LEVEL, screenWidth, selectedDifficulty);
                currentState=STATE_MENU;
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
                    if(enemies[i].active&&enemies[i].animState!=ENEMY_ANIM_DEATH)
                    {
                        float barWidth=40.0f;
                        float barHeight=5.0f;
                        float barX=enemies[i].rec.x+(enemies[i].rec.width-barWidth)/2.0f;
                        float barY=enemies[i].rec.y+ENEMY_OFFSET_Y-10.0f;
                        float healthRatio=(float)enemies[i].health/(float)enemies[i].maxHealth;
                        if(healthRatio<0.0f) healthRatio=0.0f;

                        DrawRectangle(barX,barY,barWidth,barHeight,(Color){40,40,40,255});   // background
                        DrawRectangle(barX,barY,barWidth*healthRatio,barHeight,RED);          // fill
                        DrawRectangleLines(barX,barY,barWidth,barHeight,BLACK);               // border
                    }
                }
            }

            //--- boss draw (frame-by-frame textures, not a strip - pick the array + index, not a slice) ---
            if(currentLevel==BOSS_LEVEL&&boss.active)
            {
                AnimFrames *curBossAnim;
                switch(boss.animState)
                {
                    case BOSS_ANIM_IDLE:   curBossAnim=&bossAnim.idle;   break;
                    case BOSS_ANIM_CLEAVE: curBossAnim=&bossAnim.cleave; break;
                    case BOSS_ANIM_HIT:    curBossAnim=&bossAnim.hit;    break;
                    case BOSS_ANIM_DEATH:  curBossAnim=&bossAnim.death;  break;
                    default:               curBossAnim=&bossAnim.idle;   break;
                }
                if(curBossAnim->count>0)
                {
                    int bf=boss.currentFrame%curBossAnim->count;
                    Texture2D btex=curBossAnim->frames[bf];
                    if(btex.id!=0)
                    {
                        Color bossTint=(boss.animState==BOSS_ANIM_HIT)?RED:WHITE;
                        float srcW=(BOSS_SPRITE_DEFAULT_FACES_RIGHT==boss.facingRight)?(float)btex.width:-(float)btex.width;
                        Rectangle bsrc={0.0f,0.0f,srcW,(float)btex.height};
                        float rw=btex.width*BOSS_SPRITE_SCALE;
                        float rh=btex.height*BOSS_SPRITE_SCALE;
                        //anchor by center-x + bottom against boss.rec instead of the left edge - this pack's
                        //frames aren't all the same canvas size (cleave is wider to fit the weapon swing), so
                        //anchoring by the left edge made the demon visually drift away from its own hitbox
                        float bossCenterX=boss.rec.x+boss.rec.width/2.0f;
                        Rectangle bdest={ bossCenterX-rw/2.0f+BOSS_SPRITE_OFFSET_X, boss.rec.y+boss.rec.height-rh+BOSS_SPRITE_OFFSET_Y, rw, rh };
                        DrawTexturePro(btex,bsrc,bdest,(Vector2){0,0},0.0f,bossTint);
                    }
                }
                //boss health bar
                float barW=300.0f, barH=20.0f;
                float barX=screenWidth/2.0f-barW/2.0f, barY=50.0f;
                DrawRectangle((int)barX,(int)barY,(int)barW,(int)barH,DARKGRAY);
                float healthRatio=(float)boss.health/(float)BOSS_MAX_HEALTH;
                if(healthRatio<0.0f) healthRatio=0.0f;
                DrawRectangle((int)barX,(int)barY,(int)(barW*healthRatio),(int)barH,RED);
                DrawRectangleLines((int)barX,(int)barY,(int)barW,(int)barH,WHITE);
                DrawTextEx(myfont,"DEMON",(Vector2){barX+100,barY-26},20,2,GOLD);
            }

            //once every goblin is dead, show where to go: the right edge of the room
            if(currentLevel!=BOSS_LEVEL&&!showLevelIntro&&playerHealth>0)
            {
                bool roomClearedDraw=true;
                for(int i=0;i<activeEnemyCount;i++)
                {
                    if(enemies[i].active){ roomClearedDraw=false; break; }
                }
                if(roomClearedDraw)
                {
                    float pulse=0.5f+0.5f*sinf((float)GetTime()*5.0f);
                    DrawRectangle(screenWidth-(int)ROOM_EXIT_ZONE_WIDTH,0,(int)ROOM_EXIT_ZONE_WIDTH,screenHeight,Fade(GREEN,0.12f+0.20f*pulse));
                    const char* clearedText="ROOM CLEARED - head to the right edge >>";
                    DrawTextEx(myfont,clearedText,(Vector2){screenWidth/2-MeasureTextEx(myfont,clearedText,26,2).x/2,120},26,2,GREEN);
                }
            }

            //--- hitbox visualization: T toggles this on/off ---
            if(showHitboxes)
            {
                Rectangle playerCollisionRecDraw = {
                    player.rec.x + PLAYER_OFFSET_X,
                    player.rec.y + PLAYER_OFFSET_Y,
                    PLAYER_COLLISION_WIDTH,
                    PLAYER_COLLISION_HEIGHT
                };
                DrawRectangleLines((int)playerCollisionRecDraw.x,(int)playerCollisionRecDraw.y,(int)playerCollisionRecDraw.width,(int)playerCollisionRecDraw.height,GREEN);

                if(isAttacking)
                {
                    float attackRangeDraw=70.0f;
                    Rectangle attackBoxDraw=facingRight?
                        (Rectangle){ playerCollisionRecDraw.x+playerCollisionRecDraw.width+PLAYER_ATTACK_FORWARD_OFFSET, playerCollisionRecDraw.y, attackRangeDraw, playerCollisionRecDraw.height }
                        :(Rectangle){ playerCollisionRecDraw.x-attackRangeDraw-PLAYER_ATTACK_FORWARD_OFFSET, playerCollisionRecDraw.y, attackRangeDraw, playerCollisionRecDraw.height };
                    DrawRectangleLines((int)attackBoxDraw.x,(int)attackBoxDraw.y,(int)attackBoxDraw.width,(int)attackBoxDraw.height,YELLOW);
                }

                for(int i=0;i<activeEnemyCount;i++)
                {
                    if(!enemies[i].active) continue;
                    Rectangle enemyCollisionRecDraw = {
                        enemies[i].rec.x + ENEMY_OFFSET_X,
                        enemies[i].rec.y + ENEMY_COLLISION_OFFSET_Y,
                        ENEMY_COLLISION_WIDTH,
                        ENEMY_COLLISION_HEIGHT
                    };
                    DrawRectangleLines((int)enemyCollisionRecDraw.x,(int)enemyCollisionRecDraw.y,(int)enemyCollisionRecDraw.width,(int)enemyCollisionRecDraw.height,ORANGE);
                }

                if(currentLevel!=BOSS_LEVEL)
                {
                    DrawRectangleLines(screenWidth-(int)ROOM_EXIT_ZONE_WIDTH,0,(int)ROOM_EXIT_ZONE_WIDTH,screenHeight,PURPLE); //room exit strip
                }

                if(currentLevel==BOSS_LEVEL&&boss.active)
                {
                    DrawRectangleLines((int)boss.rec.x,(int)boss.rec.y,(int)boss.rec.width,(int)boss.rec.height,SKYBLUE); //boss.rec itself (the positioning anchor)

                    Rectangle bossCollisionRecDraw = boss.facingRight?
                        (Rectangle){ boss.rec.x+BOSS_OFFSET_X, boss.rec.y+BOSS_COLLISION_OFFSET_Y, BOSS_COLLISION_WIDTH, BOSS_COLLISION_HEIGHT }
                        :(Rectangle){ boss.rec.x+boss.rec.width-BOSS_OFFSET_X-BOSS_COLLISION_WIDTH, boss.rec.y+BOSS_COLLISION_OFFSET_Y, BOSS_COLLISION_WIDTH, BOSS_COLLISION_HEIGHT };
                    DrawRectangleLines((int)bossCollisionRecDraw.x,(int)bossCollisionRecDraw.y,(int)bossCollisionRecDraw.width,(int)bossCollisionRecDraw.height,LIME);

                    //detection zone: boss body + range on both sides (gold)
                    Rectangle bossBodyDraw=GetBossBody(&boss);
                    float detectDraw=BOSS_ATTACK_RANGE_BASE+bossRangeBonus[selectedDifficulty];
                    DrawRectangleLines((int)(bossBodyDraw.x-detectDraw),(int)bossBodyDraw.y,(int)(bossBodyDraw.width+2.0f*detectDraw),(int)bossBodyDraw.height,GOLD);

                    if(boss.animState==BOSS_ANIM_CLEAVE)
                    {
                        float curBossAttackRangeBoxDraw=BOSS_ATTACK_RANGE_BOX_BASE+bossRangeBonus[selectedDifficulty];
                        Rectangle bossAttackBoxDraw=boss.facingRight?
                            (Rectangle){ boss.rec.x+boss.rec.width+BOSS_ATTACK_FORWARD_OFFSET, boss.rec.y, curBossAttackRangeBoxDraw, boss.rec.height }
                            :(Rectangle){ boss.rec.x-curBossAttackRangeBoxDraw-BOSS_ATTACK_FORWARD_OFFSET, boss.rec.y, curBossAttackRangeBoxDraw, boss.rec.height };
                        DrawRectangleLines((int)bossAttackBoxDraw.x,(int)bossAttackBoxDraw.y,(int)bossAttackBoxDraw.width,(int)bossAttackBoxDraw.height,MAGENTA);
                    }
                }
            }
            //DrawText(showHitboxes?"Hitboxes: ON (T)":"Hitboxes: OFF (T)", 10, 590, 14, showHitboxes?LIME:GRAY);

            DrawTextEx(myfont,TextFormat("LEVEL %d/%d",currentLevel,MAX_LEVEL),(Vector2){10,10},22,2,YELLOW);
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

            //--- TEMP DEBUG: remove once the boss is confirmed visible - id==0 means that file failed to load ---
            //DrawText(TextFormat("boss idle[0]=%d cleave[0]=%d hit[0]=%d death[0]=%d",
                //bossAnim.idle.frames[0].id, bossAnim.cleave.frames[0].id,
                //bossAnim.hit.frames[0].id, bossAnim.death.frames[0].id), 10, 570, 14, LIME);
            //--- END TEMP DEBUG ---
            //==================================================
// LEVEL INTRO ANIMATION
//==================================================
if(showLevelIntro)
{
    float progress=1.0f-(levelIntroTimer/LEVEL_INTRO_DURATION);

    // Clamp progress between 0 and 1
    if(progress<0.0f) progress=0.0f;
    if(progress>1.0f) progress=1.0f;

    // Fade in during first half, fade out during second half
    float alpha;

    if(progress<0.5f)
    {
        alpha=progress*2.0f;
    }
    else
    {
        alpha=(1.0f-progress)*2.0f;
    }

    if(alpha<0.0f) alpha=0.0f;
    if(alpha>1.0f) alpha=1.0f;

    // Dark overlay
    DrawRectangle(
        0,
        0,
        screenWidth,
        screenHeight,
        (Color){0,0,0,(unsigned char)(180*alpha)}
    );

    // Small pulse effect
    float pulse=sinf(progress*PI*4.0f)*5.0f;

    // LEVEL X
    char levelText[32];
    sprintf(levelText,"LEVEL %d",introLevelNumber);

    float fontSize=80.0f+pulse;

    Vector2 levelSize=MeasureTextEx(
        myfont,
        levelText,
        fontSize,
        2
    );

    DrawTextEx(
        myfont,
        levelText,
        (Vector2){
            screenWidth/2.0f-levelSize.x/2.0f,
            screenHeight/2.0f-70.0f
        },
        fontSize,
        2,
        (Color){
            255,
            255,
            255,
            (unsigned char)(255*alpha)
        }
    );

    // STARTING...
    const char *startingText="STARTING...";

    Vector2 startingSize=MeasureTextEx(
        myfont,
        startingText,
        30,
        2
    );

    DrawTextEx(
        myfont,
        startingText,
        (Vector2){
            screenWidth/2.0f-startingSize.x/2.0f,
            screenHeight/2.0f+30.0f
        },
        30,
        2,
        (Color){
            220,
            220,
            220,
            (unsigned char)(255*alpha)
        }
    );

    // Decorative horizontal lines
    float lineWidth=300.0f*alpha;

    DrawRectangle(
        (int)(screenWidth/2.0f-lineWidth/2.0f),
        screenHeight/2+80,
        (int)lineWidth,
        3,
        (Color){
            255,
            255,
            255,
            (unsigned char)(200*alpha)
        }
    );
}
        }
        else if(currentState==STATE_INSTRUCTIONS)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Instructions",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Instructions",60,2).x/2,100},60,2,(Color){48,120,148,255});
            DrawTextEx(myfont,"Move: A/D or Arrow Keys",(Vector2){200, 220},22,2,RAYWHITE);
            DrawTextEx(myfont,"Jump: SPACE or W (double jump available)",(Vector2){200,255},22,2, RAYWHITE);
            DrawTextEx(myfont,"Dash: LEFT SHIFT or RIGHT SHIFT", (Vector2){200, 290}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Attack 1: Q or LEFT MOUSE BUTTON", (Vector2){200, 325}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Attack 2: E or RIGHT MOUSE BUTTON",(Vector2) {200, 360}, 22,2, RAYWHITE);
            DrawTextEx(myfont,"Defeat all enemies, then reach the right edge of the room to advance!", (Vector2){200, 410}, 22,2, YELLOW);
            DrawTextEx(myfont,"Room 6: a demon boss awaits - dodge its cleave and strike back!", (Vector2){200, 440}, 20,2, YELLOW);
            DrawTextEx(myfont,"Press ESC to return to menu", (Vector2){screenWidth/2-130, screenHeight-60}, 20,2, GRAY);
        }
        else if(currentState==STATE_CREDITS)
        {
            DrawTexture(menubg,0,0,WHITE);
            DrawTextEx(myfont,"Credits",(Vector2){screenWidth/2-MeasureTextEx(myfont,"Credits",60,2).x/2,100},60,2,(Color){48,120,148,255});
            DrawTextEx(myfont,"Game design & programming:DANIEL & FATIN", (Vector2){250, 230}, 30,2, (Color){120,163,124,255});
            DrawTextEx(myfont,"Music: Hollow Knight OST - Sealed Vessel", (Vector2){250, 265}, 30,2, (Color){120,163,124,255});
            DrawTextEx(myfont,"Sprites: Craftpix & Itch.io and other open sources",(Vector2) {250, 300}, 30,2, (Color){120,163,124,255});
            DrawTextEx(myfont,"Made with raylib", (Vector2){250, 335}, 30,2, (Color){120,163,124,255});
            DrawTextEx(myfont,"Press ESC to return to menu",(Vector2) {screenWidth/2-130, screenHeight-60}, 20,2, GRAY);
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
        else if(currentState==STATE_HIGHSCORES)
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
            DrawTextEx(myfont,"Press ESC to return to menu",(Vector2){screenWidth/2-170,screenHeight-60},20,2,GRAY);
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
            DrawTextEx(myfont,"Press ENTER to continue",(Vector2){screenWidth/2-130,340},20,2,GRAY);
            DrawTextEx(myfont,"Press ESC to go back",(Vector2){screenWidth/2-130,screenHeight-60},20,2,GRAY);
        }
        else if(currentState==STATE_VICTORY)
        {
            const char*winText="VICTORY! THE DEMON HAS FALLEN!";
            int winWidth=MeasureText(winText,32);
            DrawTextEx(myfont,winText,(Vector2){(screenWidth-winWidth)/2,220},32,2,GOLD);
            const char*subText="Press ENTER or R to Play Again";
            int subWidth=MeasureText(subText,20);
            DrawTextEx(myfont,subText,(Vector2){(screenWidth-subWidth)/2,300},20,2,RAYWHITE);
            const char*scoreText=TextFormat("Final Score: %d",score);
            int scoreWidth=MeasureText(scoreText,24);
            DrawTextEx(myfont,scoreText,(Vector2){(screenWidth-scoreWidth)/2,340},24,2,YELLOW);
        }
        EndDrawing();
    }

    UnloadTexture(bgTextureLvl1);
    UnloadTexture(bgTextureLvl2);
    UnloadTexture(bgTextureLvl3);
    UnloadTexture(bgTextureLvl4);
    UnloadTexture(bgTextureLvl5);
    UnloadTexture(bgTextureBoss);

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

    for(int i=0;i<bossAnim.idle.count;i++)   UnloadTexture(bossAnim.idle.frames[i]);
    for(int i=0;i<bossAnim.cleave.count;i++) UnloadTexture(bossAnim.cleave.frames[i]);
    for(int i=0;i<bossAnim.hit.count;i++)    UnloadTexture(bossAnim.hit.frames[i]);
    for(int i=0;i<bossAnim.death.count;i++)  UnloadTexture(bossAnim.death.frames[i]);

    CloseWindow();
    return 0;
}

void SpawnLevelEnemies(Enemy enemies[], int level, int *activeCount, float groundLevel, int screenW, Difficulty difficulty)
{
    *activeCount=6+(level-1)*2;
    int maxForDifficulty=difficultyMaxEnemies[difficulty];
    if(*activeCount>maxForDifficulty)*activeCount=maxForDifficulty;
    if(*activeCount>ABSOLUTE_MAX_ENEMIES)*activeCount=ABSOLUTE_MAX_ENEMIES; //hard safety cap on the array itself
    float speedBoost=(level-1)*30.0f;
    int scaledHealth=difficultyEnemyHealth[difficulty]+(level-1);
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
        float enemySpeed=((100.0f+speedBoost)+((i%2)*20.0f))*enemySpeedMultiplier[difficulty];
        bool startFacingRight=(i%2==0);
        enemies[i]=(Enemy){(Rectangle){ startX, groundLevel-64.0f, 64.0f, 64.0f },
            enemySpeed,
            true,
            startFacingRight,
            minX,
            maxX,
            ENEMY_ANIM_RUN,
            0,
            0.0f,
            scaledHealth,
            scaledHealth
        };
    }
    (void)screenW; //currently unused, kept for future spawn logic that scales with screen width
}

void SpawnBoss(Boss *boss, float groundLevel, int screenW)
{
    //centered horizontally in the room, as requested
    boss->rec=(Rectangle){ (float)screenW/2.0f-BOSS_REC_WIDTH/2.0f, groundLevel-BOSS_REC_HEIGHT, BOSS_REC_WIDTH, BOSS_REC_HEIGHT };
    boss->active=true;
    boss->health=BOSS_MAX_HEALTH;
    boss->facingRight=false; //the player enters from the left, so the boss starts facing left toward them
    boss->animState=BOSS_ANIM_IDLE;
    boss->currentFrame=0;
    boss->frameTimer=0.0f;
    boss->attackCooldownTimer=BOSS_SPAWN_COOLDOWN;
}

//the boss's real body box: used for being hit, for blocking the player, and for measuring the detection range
Rectangle GetBossBody(const Boss *boss)
{
    return (Rectangle){ boss->rec.x+BOSS_BODY_INSET_X, boss->rec.y+BOSS_BODY_INSET_Y,
                        boss->rec.width-2.0f*BOSS_BODY_INSET_X, boss->rec.height-BOSS_BODY_INSET_Y };
}

void LoadAnimFrames(AnimFrames *anim, const char *folder, const char *prefix, int count)
{
    if(count>MAX_ANIM_FRAMES) count=MAX_ANIM_FRAMES; //safety clamp
    anim->count=count;
    for(int i=1;i<=count;i++)
    {
        char path[256];
        snprintf(path,sizeof(path),"assets/demon/%s/%s_%d.png",folder,prefix,i);
        anim->frames[i-1]=LoadTexture(path);
    }
}
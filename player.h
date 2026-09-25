#ifndef PLAYER_H
#define PLAYER_H

#include "raylib.h"
#include "raymath.h"
#include "constants.h"

typedef enum JumpState
{
    STATE_GROUNDED,
    STATE_ONE_JUMP,
    STATE_DOUBLE_JUMP,
    STATE_PLATFORMED
} JumpState;

typedef enum PlayerState
{
    Dead,
    Alive
} PlayerState;

typedef struct Player
{
    Vector2 pos;
    Vector2 vel;
    Vector2 accel;
    JumpState jumpState;
    PlayerState obostha;
} Player;

static inline Player InitPlayer(void)
{
    Player p;
    p.pos = (Vector2){PLAYER_START_X, PLAYER_START_Y};
    p.vel = (Vector2){0.0f, 0.0f};
    p.accel = (Vector2){0.0f, 0.0f};
    p.jumpState = STATE_GROUNDED;
    p.obostha = Alive;
    return p;
}

// STATE_GROUNDED and STATE_PLATFORMED both mean "feet on something solid" —
// jumps reset either way. Use this instead of comparing to STATE_GROUNDED directly.
static inline bool IsGrounded(JumpState s)
{
    return (s == STATE_GROUNDED || s == STATE_PLATFORMED);
}

static inline void UpdatePlayer(Player *p, float dt, Rectangle platformRect)
{
    const Vector2 gravAccel = {0.0f, GRAVITY_ACCEL_Y};

    // 1. Reset frame-based acceleration
    p->accel = (Vector2){0.0f, 0.0f};

    // 2. Horizontal input -> acceleration
    if (IsKeyDown(KEY_A))
        p->accel.x -= MOVE_ACCEL;
    if (IsKeyDown(KEY_D))
        p->accel.x += MOVE_ACCEL;

    // 3. Jump input -> vertical velocity
    if (IsKeyPressed(KEY_SPACE))
    {
        if (IsGrounded(p->jumpState))
        {
            p->vel.y = JUMP_IMPULSE;
            p->jumpState = STATE_ONE_JUMP;
        }
        else if (p->jumpState == STATE_ONE_JUMP)
        {
            p->vel.y = JUMP_IMPULSE;
            p->jumpState = STATE_DOUBLE_JUMP;
        }
    }

    // 4. Gravity
    p->accel = Vector2Add(p->accel, gravAccel);

    // 5. Integrate velocity
    p->vel = Vector2Add(p->vel, Vector2Scale(p->accel, dt));

    // 6. Friction
    if (!IsKeyDown(KEY_A) && !IsKeyDown(KEY_D))
    {
        p->vel.x = Lerp(p->vel.x, 0.0f, FRICTION_COEFF * dt);
    }

    // 7. Clamp horizontal speed
    if (p->vel.x > MAX_MOVE_SPEED)
        p->vel.x = MAX_MOVE_SPEED;
    if (p->vel.x < -MAX_MOVE_SPEED)
        p->vel.x = -MAX_MOVE_SPEED;

    // Integrate position
    p->pos = Vector2Add(p->pos, Vector2Scale(p->vel, dt));

    // 8. Platform collision (landing on top only)
    Rectangle playerRect = {p->pos.x, p->pos.y, PLAYER_WIDTH, PLAYER_HEIGHT};
    bool onPlatform = false;
    if (CheckCollisionRecs(playerRect, platformRect))
    {
        float playerBottom = p->pos.y + PLAYER_HEIGHT;
        float platformTop = platformRect.y;
        if (p->vel.y >= 0 && (playerBottom - platformTop) < 20.0f)
        {
            p->pos.y = platformTop - PLAYER_HEIGHT;
            p->vel.y = 0.0f;
            p->jumpState = STATE_PLATFORMED;
            onPlatform = true;
        }
    }

    // Left the platform (walked off edge, or got knocked off) without jumping
    if (!onPlatform && p->jumpState == STATE_PLATFORMED)
    {
        p->jumpState = STATE_ONE_JUMP; // now airborne, one jump still available
    }

    // 9. Ground collision (skip if already resolved by platform this frame)
    if (!onPlatform && p->pos.y >= GROUND_Y - PLAYER_HEIGHT)
    {
        p->pos.y = GROUND_Y - PLAYER_HEIGHT;
        p->vel.y = 0.0f;
        p->jumpState = STATE_GROUNDED;
    }

    // Screen edge clamping
    if ((p->pos.x + PLAYER_WIDTH) >= SC_WIDTH)
        p->pos.x = SC_WIDTH - PLAYER_WIDTH;
    if (p->pos.x <= 0)
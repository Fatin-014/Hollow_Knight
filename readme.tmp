#HOLLOW KNIGHT—README File

~Description:
A single-file 2D action platformer written in C with [raylib](https://www.raylib.com/). Fight through five rooms of goblins, then face a demon boss in the final room. Choose a difficulty, enter your name, and compete on the local high-score table.



## Essential Project Files:

Hollow_Knight/
  final_game.c          <- the game source (this is the only source file)
  assets/                <- all art, fonts and audio (see full layout below)
  highscores.txt          <- created automatically on first run

***Alert:The game must always be run from the folder containing `assets/`, because every asset is loaded with a relative path (e.g. `"assets/bg.png"`). Running the compiled executable from anywhere else will cause textures, fonts and sounds to fail to load (the game still starts, but things will be invisible/silent.



## Required asset layout:

assets/
  bg.png  bg2.png  bg3.png  bg4.png  bg5.png  bg_boss.png
  menubg.png
  themefont.TTF
  audio/
    clicksound.mp3                                 
    Hollow Knight OST - Sealed Vessel.mp3
  enemies/goblin/
    goblin_idle_anim_strip_4.png
    goblin_run_anim_strip_6.png
    goblin_hit_anim_strip_3.png
    goblin_attack_anim_strip_4.png
    goblin_death_anim_strip_6.png
  player/
    Idle.png  Run.png  Jump.png  Fall.png  Dash.png  Hurt.png
    Attack 1.png  Attack 2.png  Death.png
  demon/
    01_demon_idle/       demon_idle_1.png      ... demon_idle_6.png
    03_demon_cleave/     demon_cleave_1.png    ... demon_cleave_15.png
    04_demon_take_hit/   demon_take_hit_1.png  ... demon_take_hit_5.png
    05_demon_death/      demon_death_1.png     ... demon_death_22.png



##Setup & build instructions

~Windows

1. Install raylib using the [official Windows installer](https://github.com/raysan5/raylib/releases) (it bundles w64devkit, a ready-to-use GCC toolchain — no separate compiler install needed).
2. Open the **w64devkit** terminal that comes with it.
3. `cd` into the folder containing `final_game.c` and `assets/`.
4. Build.
   gcc final_game.c -o final_game.exe -lraylib -lopengl32 -lgdi32 -lwinmm
5. Run it from the **same terminal** (so you stay in the right folder): final_game.exe.

~Linux

1. Install raylib and build tools (Debian/Ubuntu example):
   sudo apt install build-essential libraylib-dev
   (If your distro doesn't package raylib, follow the [raylib Linux build guide](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux) to build it from source first.)
2. `cd` into the project folder.
3. Build.
   gcc final_game.c -o final_game -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
4. Run:./final_game.

~macOS

1. Install raylib, e.g. via Homebrew:
   brew install raylib
2. `cd` into the project folder.
3. Build.
   gcc final_game.c -o final_game -lraylib -framework IOKit -framework Cocoa -framework OpenGL
4. Run:./final_game
> The file compiles cleanly with `-Wall -Wextra` against raylib 5.5, producing one harmless warning (`unused variable 'selectedOption'`) — this does not affect gameplay.



## Running the game:

- Always launch the built executable from inside the project folder (the one holding `assets/`) — double-clicking it from a different location, or moving just the `.exe`/binary elsewhere, will break asset loading.
- The game creates/updates `highscores.txt` **in the folder you launched it from** the first time a score is saved. You can safely delete this file to reset the leaderboard.
- No internet connection, installation wizard, admin rights, or additional configuration files are needed beyond what's above — once it's built, it's fully self-contained.



## How to play

~Controls

-> Action-keys: 
1. Move: `A` / `D` or Left / Right arrows
2.Jump: (double jump available):`Space` or `W`
3.Dash: Left or Right `Shift`:(0.2s dash, 3s cooldown)
4.Attack 1: (quick sword swing):`Q` or left mouse button 
5.Attack 2: (slow sword swing):`E` or right mouse button 
6.Back from a sub-screen:  `Esc` 
7.After dying: return to main menu: `R`
8.After winning: to play again:`Enter` or `R`

~Flow of a run

1. Main menu: Start Game, Instructions, Credits, High Scores, Sound ON/OFF, Exit.
2. Enter your name: (up to 16 characters, `Enter` to confirm).
3. Choose a difficulty — Easy, Medium, or Hard.
4. Rooms 1–5:  defeat every goblin, then walk to the right edge of the room (a green strip pulses once it's clear). Each room opens with a "LEVEL X" banner.
5. Room 6: the demon boss. Defeat it to win.
6. Your score is saved to `highscores.txt` on death or victory. The top 5 scores appear under **High Scores** on the main menu.

You start with 5 hearts. After taking a hit you get 1 second of invincibility (the knight flashes red) plus a knockback.


#Hope You Enjoy The Game :)) 

#ifndef EXAMPLE_H
#define EXAMPLE_H

#include "linkedlist.h"
#include <pthread.h>

#define MOVE_LEFT 'a'
#define MOVE_RIGHT 'd'
#define MOVE_UP 'w'
#define MOVE_DOWN 's'
#define SHOOT ' '
#define SPACE ' '
#define QUIT 'q'
#define PAUSE 'p'
#define CAT_HEIGHT 2
#define GAME_ROWS 24
#define GAME_COLS 80
#define PLAYER_LIMIT 18
#define CAT_LIMIT 14
#define CAT_SPEED 15 // Lower number == faster
#define CAT_MIN_LENGTH 5
#define SCORE_POS 26
#define LIVES_POS 42
#define LEFT_DIR 0
#define RIGHT_DIR 1
#define DEFAULT_SNAKE_LENGTH 30
#define DEFAULT_SNAKE_HEAD_POS_Y 79
#define DEFAULT_SNAKE_HEAD_POS_X 2
#define DEFAULT_SNAKE_DIR 0

void *cat_thr(void *arg);
void run();
void runGame();
void drawInt(int x, int row, int col);
void setScore();
void updateScore();
void setLives();
void pauseGame();
void updateLives();
void *cat_shot_thr(void *arg);
void *shoot_thr(void *arg);
void drawString(int row, int col, char* str);
int string_equals(char* str, char* str2);

typedef struct Shots {
    char* type;
    int shot_pos_x;
    int shot_pos_y;
    int orig_x;
    bool kill;
} Shot;

typedef struct Caterpillars {
    int head_pos_x;
    int head_pos_y;
    int length;
    int dir;
    int ticksSinceShot;
    bool alive;
} Caterpillar;

const double MIN_PLAYER_SHOT_DELAY = 0.3;
int playerpos_x = PLAYER_LIMIT;
int playerpos_y = 35;
int lastCatPos_x, lastCatPos_y;
int prev_playerpos_y, prev_playerpos_x, anim_pos;
int score = 0;
int lives = 2;
int lastShotPosX, lastShotPosY, lastLength, lastDir;
int cat_count = 1;
bool newPlayerMovement = true, gameRunning = true, gameCollision = false, gamePaused = false, playerDead = false, gameOver = false;
pthread_mutex_t playerpos_lock, console_lock, shot_lock, cat_lock;
pthread_t animate_player_pid, caterpillar_pid, redraw_pid, upkeep_pid, playerinput_pid;
struct timeval t1, t2;
double elapsedTime;
LinkedList *shots;
LinkedList *caterpillars;

#endif

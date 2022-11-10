#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/time.h>
#include <curses.h>
#include <stdbool.h>
#include <stdlib.h>
#include "console.h"
#include "linkedlist.h"
#include "game.h"

/***********************************
 *             Images              *
 ***********************************/

char *GAME_BOARD[] = {
        "                   Score:          Lives:",
        "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-centipede!=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "",
        "\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"\"",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
};
char* PLAYER_BODY[GAME_COLS][CAT_HEIGHT] = {{"[^]",""}};
char* PLAYER_BODY_ANIM[GAME_COLS][CAT_HEIGHT] = {{"[@]",""}};
char* SHOT_BODY[GAME_COLS][CAT_HEIGHT] = {{"@",""} };
char* CSHOT_BODY[GAME_COLS][CAT_HEIGHT] = {{".",""} };

/***********************************
 *         Game functions          *
 ***********************************/

// Detect collisions between either a given Caterpillar object and a player-type
// Shot object, or between the Player and a Caterpillar-type Shot object.
// If there is a player-shot collision, kill caterpillar and draw a new caterpillar as long as
// conditions are met
// If there is a caterpillar-shot collision, reduces lives by 1
void collisionDetection(Caterpillar *c, Shot *shot)
{
    // Detect collisions between player shots and caterpillar
    if (string_equals(shot->type, "player")) {
        int orig_length = c->length;
        int orig_dir = c->dir;
        int new_snake_length;
        bool col = false;

        // If direction is right
        if (c->dir == RIGHT_DIR)
        {
            // If caterpillar is moving RIGHT without wrapping
            if ((c->head_pos_y - c->length) > 0) {
                if (shot->shot_pos_x == c->head_pos_x &&
                    (shot->shot_pos_y <= c->head_pos_y) &&
                    (shot->shot_pos_y >= c->head_pos_y - c->length)) {
                    col = true;
                    c->length = c->head_pos_y - shot->shot_pos_y;
                    new_snake_length = orig_length - c->length;
                }

            }
                // If caterpillar is moving RIGHT with wrapping
            else if ((c->head_pos_y - c->length) <= 0) {
                if (shot->shot_pos_x == c->head_pos_x &&
                    (shot->shot_pos_y >= 0) &&
                    (shot->shot_pos_y <= c->head_pos_y)) {
                    col = true;
                    c->length = c->head_pos_y - shot->shot_pos_y;
                    new_snake_length = orig_length - c->length;
                }
                else if (shot->shot_pos_x == c->head_pos_x - CAT_HEIGHT &&
                         (shot->shot_pos_y >= c->head_pos_y) &&
                         (shot->shot_pos_y <= c->length - c->head_pos_y)) {
                    col = true;
                    int max = c->length - c->head_pos_y;
                    new_snake_length = max - shot->shot_pos_y;
                    c->length = c->length - new_snake_length;
                }
            }
        }

        // If direction is left
        else if (c->dir == LEFT_DIR)
        {
            // If caterpillar is moving LEFT without wrapping
            if ((c->head_pos_y + c->length) < GAME_COLS) {
                if (shot->shot_pos_x == c->head_pos_x &&
                    (shot->shot_pos_y >= c->head_pos_y) &&
                    (shot->shot_pos_y <= (c->head_pos_y + c->length))) {
                    col = true;
                    c->length = shot->shot_pos_y - c->head_pos_y;
                    new_snake_length = orig_length - c->length;
                }
            }

            // If caterpillar is moving LEFT with wrapping
            else if ((c->head_pos_y + c->length) >= GAME_COLS) {
                if (shot->shot_pos_x == c->head_pos_x &&
                    (shot->shot_pos_y >= c->head_pos_y) &&
                    (shot->shot_pos_y <= GAME_COLS)) {
                    col = true;
                    c->length = shot->shot_pos_y - c->head_pos_y;
                    new_snake_length = orig_length - c->length;
                }
                else if (shot->shot_pos_x == c->head_pos_x - CAT_HEIGHT &&
                         (shot->shot_pos_y <= c->head_pos_y) &&
                         (shot->shot_pos_y >= (GAME_COLS - c->length + (GAME_COLS - c->head_pos_y)))) {
                    col = true;
                    int min = (GAME_COLS - c->length + (GAME_COLS - c->head_pos_y));
                    new_snake_length = shot->shot_pos_y - min;
                    c->length = c->length - new_snake_length;
                }
            }
        }

        // If there was a collision & the new caterpillar (ie: the caterpillar that is generated
        // from the tail) is greater than the minimum length, we generate a new caterpillar
        if (col) {
            shot->kill = true;
            updateScore();

            // If tail end was big enough, create new caterpillar
            if (new_snake_length > CAT_MIN_LENGTH) {
                lastShotPosX = shot->shot_pos_x;
                lastShotPosY = shot->shot_pos_y;
                lastLength = new_snake_length;
                lastDir = orig_dir;
                gameCollision = true;
                cat_count++;
                pthread_t pid;
                pthread_create(&pid, NULL, &cat_thr, NULL);
            }
        }
    }

    // Detect collisions between caterpillar shots and player
    else if (string_equals(shot->type, "caterpillar")) {
        if (shot->shot_pos_x == playerpos_x && (shot->shot_pos_y == playerpos_y
                || shot->shot_pos_y == playerpos_y + 1 || shot->shot_pos_y == playerpos_y + 2)) {
            lives--;
            if (lives == -1) {
                gameOver = true;
            } else {
                drawInt(lives, 0, LIVES_POS);
            }
            playerDead = true;
            shot->kill = true;
        }
    }
}

// Draw Caterpillar object
void drawCaterpillar(Caterpillar *caterpillar)
{
    int orig_x = caterpillar->head_pos_x;
    bool wrapped = false; // Used for wrap-around logic

    // Animate caterpillar
    // This changes the head every four frames
    // 3/4 frames look like option #1, 1/4 frames look like option #2
    char *head[CAT_HEIGHT], *body[CAT_HEIGHT];
    if (caterpillar->head_pos_y % 4 != 0) {
        head[0] = "@";
        head[1] = "=";
    } else {
        head[0] = "-";
        head[1] = "=";
    }

    // Draw head
    consoleDrawImage(caterpillar->head_pos_x, caterpillar->head_pos_y,
                     head, CAT_HEIGHT);

    // Draw body
    int i = 1;
    while (i < caterpillar->length + 1) {

        // Animate caterpillar body
        // Creates sliding movement effect by sliding characters forward w/ every frame
        if ((caterpillar->head_pos_y - i) % 4 == 0) {
            body[0] = "#";
            body[1] = "#";
        } else {
            body[0] = "|";
            body[1] = "|";
        }

        // Draw caterpillar body with wrap-around effect
        // Determines distance from wall using head pos, length & distance
        // If there is not enough space to fit the whole length of the caterpillar, it must wrap
        if (caterpillar->dir == RIGHT_DIR) {
            if (!wrapped) {
                if (caterpillar->head_pos_y - i < 0) {
                    orig_x -= CAT_HEIGHT;
                    wrapped = true;
                }
                consoleDrawImage(orig_x,caterpillar->head_pos_y - i,body,CAT_HEIGHT);
            } else {
                consoleDrawImage(orig_x,caterpillar->length - i,body,CAT_HEIGHT);
            }
        } else if (caterpillar->dir == LEFT_DIR) {
            if (!wrapped) {
                if (caterpillar->head_pos_x > 2 && caterpillar->head_pos_y + i > GAME_COLS) {
                    orig_x -= CAT_HEIGHT;
                    wrapped = true;
                }
                consoleDrawImage(orig_x,caterpillar->head_pos_y + i,body,CAT_HEIGHT);
            } else {
                consoleDrawImage(orig_x,GAME_COLS - (caterpillar->length - i),body,CAT_HEIGHT);
            }
        }
        i++;
    }
}

void killAllBullets() {
    if (shots != NULL) {
        Node* curr = shots->head;
        while (curr != NULL) {
            Shot *shot = (Shot *) curr->data;
            shot->kill = true;
            curr = curr->next;
        }
    }
}

// Spawns the initial caterpillar after a 1s pause
void *spawner(void *arg)
{
    sleepTicks(100);
    pthread_t pid;
    pthread_create(&pid, NULL, &cat_thr, NULL);
    pthread_join(pid, NULL);
    pthread_exit(NULL);
}

// Shoots new bullet
// Limits player shoot-speed by ensuring that > 0.3s have passed
void shoot()
{
    gettimeofday(&t2, NULL);
    elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;
    if (elapsedTime > MIN_PLAYER_SHOT_DELAY) {
        pthread_t pid;
        pthread_create(&pid, NULL, &shoot_thr, NULL);
    }
}

// Shoots new bullet from caterpillar
void cat_shoot()
{
    pthread_t pid;
    pthread_create(&pid, NULL, &cat_shot_thr, NULL);
}

/***********************************
 *            Threads              *
 ***********************************/

// Refreshes the game every 1 tick
void *redraw_thr(void *arg)
{
    int i = 0;
    while (gameRunning) {
        sleepTicks(1); // Sleep for 1 tick
        i++;
        pthread_mutex_lock(&console_lock);
        consoleClearImage(18, 0, GAME_ROWS, GAME_COLS);

        // Draw all living caterpillars
        pthread_mutex_lock(&cat_lock);
        if (caterpillars != NULL) {
            consoleClearImage(2, 0, CAT_LIMIT, GAME_COLS);
            Node* curr = caterpillars->head;
            while (curr != NULL) {
                drawCaterpillar((Caterpillar *) curr->data);
                curr = curr->next;
            }
        }
        pthread_mutex_unlock(&cat_lock);

        // Draw bullets
        // Detect collisions with player shots/caterpillars
        pthread_mutex_lock(&shot_lock);
        if (shots != NULL) {
            Node* curr = shots->head;
            while (curr != NULL) {
                Shot *shot = (Shot *) curr->data;
                if (string_equals(shot->type, "player")) {
                    if (shot->orig_x - shot->shot_pos_x > 0) {
                        consoleClearImage(shot->shot_pos_x + 1, shot->shot_pos_y, 1, 1);
                    }
                }
                else {
                    consoleClearImage(shot->shot_pos_x - 1, shot->shot_pos_y, 1, 1);
                }
                if (!shot->kill) {
                    if (string_equals(shot->type, "player"))
                        consoleDrawImage(shot->shot_pos_x, shot->shot_pos_y,SHOT_BODY[0], 1);
                    else if (shot->shot_pos_x < GAME_ROWS - 1)
                        consoleDrawImage(shot->shot_pos_x, shot->shot_pos_y,CSHOT_BODY[0], 1);
                    pthread_mutex_lock(&cat_lock);
                    if (caterpillars != NULL) {
                        Node* curr_cat = caterpillars->head;
                        while (curr_cat != NULL) {
                            Caterpillar* caterpillar = (Caterpillar *)curr_cat->data;
                            collisionDetection(caterpillar, shot);
                            curr_cat = curr_cat->next;
                        }
                    }
                    pthread_mutex_unlock(&cat_lock);
                }
                curr = curr->next;
            }
        }
        pthread_mutex_unlock(&shot_lock);

        // Draw player
        pthread_mutex_lock(&playerpos_lock);
        consoleClearImage(prev_playerpos_x,prev_playerpos_y,CAT_HEIGHT,3);
        if (i % 10 != 0)
            consoleDrawImage(playerpos_x,playerpos_y,PLAYER_BODY[0],CAT_HEIGHT);
        else
            consoleDrawImage(playerpos_x,playerpos_y,PLAYER_BODY_ANIM[0],CAT_HEIGHT);
        pthread_mutex_unlock(&playerpos_lock);

        // Refresh display
        consoleRefresh();
        pthread_mutex_unlock(&console_lock);

        // If the player died during the last 1 tick, pause the game temporarily
        // Wait 1s, kill all bullets, then unpause the game
        if (playerDead) {
            pauseGame(); // Pauses
            sleepTicks(100);
            killAllBullets(); // Kills bullets
            pauseGame(); // Unpauses
            playerDead = false;
        }

        // End game if lives < 0
        if (gameOver) {
            pthread_mutex_lock(&console_lock);
            drawString(10, 34, "Game over! Press any key to end game :)");
            consoleRefresh();
            pthread_mutex_unlock(&console_lock);
            pauseGame(); // Pauses
            sleepTicks(500);
            gameRunning = false;
        }
    }
    pthread_exit(NULL);
}

void *shoot_thr(void *arg)
{
    // Log time that the shot was made to ensure player
    // can only shoot 1 shoot per 0.3s
    gettimeofday(&t1, NULL);

    // Initialize new Shot object
    Shot *s1 = malloc(sizeof(Shot));

    // Initialize the Shot's position relative to the player (one block above)
    pthread_mutex_lock(&playerpos_lock);
    s1->shot_pos_x = playerpos_x - 1;
    s1->shot_pos_y = playerpos_y + 1;
    pthread_mutex_unlock(&playerpos_lock);
    s1->orig_x = s1->shot_pos_x;
    s1->kill = false;
    s1->type = "player";

    // Add Shot to shots LinkedList
    pthread_mutex_lock(&shot_lock);
    AddNode(shots, s1, sizeof(s1) + 1);
    pthread_mutex_unlock(&shot_lock);

    // As long as the Shot is active, we update the shot movement
    while (s1->kill == false) {
        sleepTicks(4);
        pthread_mutex_lock(&shot_lock);
        s1->shot_pos_x = s1->shot_pos_x - 1;

        // If the Shot reaches the end of the window without a collision,
        // we kill it
        if (s1->shot_pos_x == 1) {
            s1->kill = true;
        }
        pthread_mutex_unlock(&shot_lock);
    }

    // Once killed, we remove it from shots LinkedList
    pthread_mutex_lock(&shot_lock);
    DeleteNode(shots, FindNodeByRef(shots, s1));
    pthread_mutex_unlock(&shot_lock);
    pthread_exit(NULL);
}

void *cat_shot_thr(void *arg)
{
    // Initialize new Shot object
    Shot *s1 = malloc(sizeof(Shot));

    // Initialize the Shot's position relative to the caterpillar (one block below)
    s1->shot_pos_x = lastCatPos_x + 1;
    s1->shot_pos_y = lastCatPos_y + 1;
    s1->orig_x = s1->shot_pos_x;
    s1->kill = false;
    s1->type = "caterpillar";

    // Add Shot to shots LinkedList
    pthread_mutex_lock(&shot_lock);
    AddNode(shots, s1, sizeof(s1) + 1);
    pthread_mutex_unlock(&shot_lock);

    // As long as the Shot is active, we update the shot movement
    while (s1->kill == false) {
        sleepTicks(4);
        pthread_mutex_lock(&shot_lock);
        s1->shot_pos_x = s1->shot_pos_x + 1;

        // If the Shot reaches the end of the window without a collision,
        // we kill it
        if (s1->shot_pos_x == GAME_ROWS) {
            s1->kill = true;
        }
        pthread_mutex_unlock(&shot_lock);
    }

    // Once killed, we remove it from shots LinkedList
    pthread_mutex_lock(&shot_lock);
    DeleteNode(shots, FindNodeByRef(shots, s1));
    pthread_mutex_unlock(&shot_lock);
    pthread_exit(NULL);
}

// A separate thread is created for every single caterpillar that is currently alive,
// including any that are generated from a bullet.
void *cat_thr(void *arg)
{
    // Wait 1s before initializing caterpillar
    sleepTicks(100);

    // Initialize new caterpillar
    Caterpillar *caterpillar = malloc(sizeof(Caterpillar));

    // There are two ways for new snakes to be generated
    // If the caterpillar is being generated as a result of a collision (tail -> new caterpillar),
    // then we create the new caterpillar in a relative position/direction, and with the
    // correct tail length. Otherwise, we generate it at the top-right
    if (gameCollision) {
        caterpillar->head_pos_x = lastShotPosX;
        caterpillar->head_pos_y = lastShotPosY;
        caterpillar->length = lastLength;
        caterpillar->dir = lastDir;
        gameCollision = false;
    } else {
        caterpillar->head_pos_x = DEFAULT_SNAKE_HEAD_POS_X;
        caterpillar->head_pos_y = DEFAULT_SNAKE_HEAD_POS_Y;
        caterpillar->length = DEFAULT_SNAKE_LENGTH;
        caterpillar->dir = DEFAULT_SNAKE_DIR;
    }
    caterpillar->ticksSinceShot = 0;
    caterpillar->alive = true;

    // Add caterpillar to caterpillars LinkedList
    pthread_mutex_lock(&cat_lock);
    AddNode(caterpillars, caterpillar, sizeof(caterpillar) + 1);
    pthread_mutex_unlock(&cat_lock);

    // As long as the caterpillar stays alive, we continue to
    // update the caterpillar's position
    while (caterpillar->alive) {
        sleepTicks(CAT_SPEED);
        pthread_mutex_lock(&cat_lock);

        // Caterpillar shoots at player once every 10 movements
        caterpillar->ticksSinceShot += CAT_SPEED;
        if (caterpillar->ticksSinceShot > CAT_SPEED * 10) {
            lastCatPos_x = caterpillar->head_pos_x;
            lastCatPos_y = caterpillar->head_pos_y;
            cat_shoot();
            caterpillar->ticksSinceShot = 0;
        }
        if ((caterpillar->head_pos_y == 0 && caterpillar->dir == LEFT_DIR) ||
                (caterpillar->head_pos_y == GAME_COLS - 1 && caterpillar->dir == RIGHT_DIR)) {
            if (caterpillar->head_pos_x < CAT_LIMIT)
                caterpillar->head_pos_x += CAT_HEIGHT;
            else
                caterpillar->head_pos_x -= CAT_HEIGHT;
            if (caterpillar->dir == LEFT_DIR) {
                caterpillar->dir = RIGHT_DIR;
                caterpillar->head_pos_y--;
            } else caterpillar->dir = LEFT_DIR;
        }
        if (caterpillar->dir == LEFT_DIR)
            caterpillar->head_pos_y--;
        if (caterpillar->dir == RIGHT_DIR)
            caterpillar->head_pos_y++;

        // Kill caterpillar when length is reduced to under CAT_MIN_LENGTH
        if (caterpillar->length <= CAT_MIN_LENGTH)
            caterpillar->alive = false;
        pthread_mutex_unlock(&cat_lock);
    }

    // Once killed, we remove it from caterpillars LinkedList & reduce the active cat_count
    DeleteNode(caterpillars, FindNodeByRef(caterpillars, caterpillar));
    cat_count--;
    pthread_exit(NULL);
}

// This thread is responsible for animating the player
void *animate_player_thr(void *arg)
{
    while (gameRunning) {
        sleepTicks(100);
        pthread_mutex_lock(&console_lock);
        pthread_mutex_lock(&playerpos_lock);
        consoleClearImage(playerpos_x,playerpos_y,CAT_HEIGHT,strlen(PLAYER_BODY[0][0]) + 1);
        if (anim_pos == 0) {
            consoleDrawImage(playerpos_x,playerpos_y,PLAYER_BODY[0],CAT_HEIGHT);
            anim_pos = 1;
        } else {
            consoleDrawImage(playerpos_x,playerpos_y,PLAYER_BODY_ANIM[0],CAT_HEIGHT);
            anim_pos = 0;
        }
        pthread_mutex_unlock(&playerpos_lock);
        pthread_mutex_unlock(&console_lock);
    }
    pthread_exit(NULL);
}

// Upkeep
void *upkeep_thr(void *arg)
{
    // Initialize score
    setScore();
    setLives();
    while (gameRunning) {
        sleepTicks(5);

        // Check if all caterpillars have been killed
        // If so, end the game with a victory.
        if (score > 0 && cat_count == 0) {
            gameRunning = false;
        }
    }
    pthread_exit(NULL);
}

// Get player input
void *playerinput_thr(void *arg)
{
    fd_set set;
    while (gameRunning) {
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set);
        struct timespec timeout = getTimeout(1);
        int ret = pselect(FD_SETSIZE, &set, NULL, NULL, &timeout, NULL);
        if (gameRunning && ret >= 1) {
            char c = getchar();
            if (c == SPACE) {
                shoot();
            } else if (c == PAUSE) {
                pauseGame();
            } else if (c == QUIT) {
                gameRunning = false;
            } else {
                pthread_mutex_lock(&playerpos_lock);
                newPlayerMovement = true;
                prev_playerpos_x = playerpos_x;
                prev_playerpos_y = playerpos_y;
                if (c == MOVE_LEFT && playerpos_y >= 1) {
                    playerpos_y -= 1;
                } else if (c == MOVE_RIGHT && playerpos_y < GAME_COLS - 3) {
                    playerpos_y += 1;
                } else if (c == MOVE_DOWN && playerpos_x < GAME_ROWS - 1) {
                    playerpos_x += 1;
                } else if (c == MOVE_UP && playerpos_x >= PLAYER_LIMIT) {
                    playerpos_x -= 1;
                }
                pthread_mutex_unlock(&playerpos_lock);
            }
        }
    }
    pthread_exit(NULL);
}

/***********************************
 *           Run Game              *
 ***********************************/

void run()
{
    // Initialize all mutexes
    pthread_mutex_init(&playerpos_lock, NULL);
    pthread_mutex_init(&shot_lock, NULL);
    pthread_mutex_init(&console_lock, NULL);
    pthread_mutex_init(&cat_lock, NULL);

    // Initialize console
    if (consoleInit(GAME_ROWS, GAME_COLS, GAME_BOARD)) {
        runGame();
        finalKeypress();
    }
    consoleFinish();

    // Kill mutexes
    pthread_mutex_destroy(&playerpos_lock);
    pthread_mutex_destroy(&cat_lock);
    pthread_mutex_destroy(&shot_lock);
    pthread_mutex_destroy(&console_lock);

    // Free space allocated for lists
    FreeList(shots);
    FreeList(caterpillars);
    free(shots);
    free(caterpillars);
}

void runGame()
{
    // Redraw the board every 1 tick
    pthread_create(&redraw_pid, NULL, &redraw_thr, NULL);

    // Upkeep thread - responsible for updating score/lives
    // The upkeep thread is also responsible for determining when the game is over
    // Ends the game when the player has won/lost
    pthread_create(&upkeep_pid, NULL, &upkeep_thr, NULL);

    // Animate player
    pthread_create(&animate_player_pid, NULL, &animate_player_thr, NULL);

    // Spawn caterpillars
    pthread_create(&caterpillar_pid, NULL, &spawner, NULL);

    // Get player input
    pthread_create(&playerinput_pid, NULL, &playerinput_thr, NULL);
    pthread_join(playerinput_pid, NULL);
}

void pauseGame() {
    if (!gamePaused) {
        pthread_mutex_lock(&console_lock);
        pthread_mutex_lock(&playerpos_lock);
        pthread_mutex_lock(&shot_lock);
        pthread_mutex_lock(&cat_lock);
        gamePaused = true;
    }
    else {
        pthread_mutex_unlock(&console_lock);
        pthread_mutex_unlock(&playerpos_lock);
        pthread_mutex_unlock(&shot_lock);
        pthread_mutex_unlock(&cat_lock);
        gamePaused = false;
    }
}

/***********************************
 *       Helpers / Utilities       *
 ***********************************/

// Place integer x at (row, col)
// Used for updating the score/lives section
void drawInt(int x, int row, int col)
{
    int length = snprintf(NULL, 0, "%d", x);
    char* str = malloc(length + 1);
    snprintf(str, length + 1, "%d", x);
    mvaddnstr(row, col, str, 2);
    free(str);
}

// Place integer x at (row, col)
// Used for updating the score/lives section
void drawString(int row, int col, char* str)
{
    mvaddnstr(row, col, str, 50);
}

// Write current score to board
void setScore()
{
    drawInt(score, 0, SCORE_POS);
}

// Increase score by 1, then write score to board
void updateScore()
{
    score++;
    setScore();
}

// Write current lives to board
void setLives()
{
    drawInt(lives, 0, LIVES_POS);
}

// Decrease lives by 1, then write lives to board
void updateLives()
{
    lives--;
    setLives();
}

// Increase readability of the default strcmp function
// Returns 1 if and only if both strings are identical. Else, return 0
int string_equals(char* str, char* str2) {
    return (strcmp(str, str2) == 0) ? 1 : 0;
}

/***********************************
 *              Main               *
 ***********************************/

int main(int argc, char**argv)
{
    shots = malloc(sizeof(LinkedList) * 256);
    memset(shots, 0x00, sizeof(LinkedList) * 256);
    caterpillars = malloc(sizeof(LinkedList) * 256);
    memset(caterpillars, 0x00, sizeof(LinkedList) * 256);
    run();
	printf("done!\n");
}

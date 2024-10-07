/*
 * Application.h
 *
 *  Created on: Dec 29, 2019
 *      Author: Matthew Zhong
 *  Supervisor: Leyla Nazhand-Ali
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <HAL/HAL.h>

#define MIN_DIM 2
#define MAX_DIM 5
#define DEFAULT_DIM 3

#define COORDINATES_LEN 4
#define X1 0
#define Y1 1
#define DIRECTION 2

#define MAX_PLAYERS 2
#define MAX_BOXES 16

#define TOP_LINE 0       // Represents the top side of a box
#define BOX_COMPLETED 1  // Represents a completed box


typedef enum { TitleScreen, InstructionsScreen, SettingsScreen, GameScreen, ResultsScreen } _appGameFSMstate;
typedef enum { Cursor_0, Cursor_1, Cursor_2, NUM_CURSOR_CHOICES } _appCursorFSMstate;
typedef enum { FirstQuestion, ReceiveInput } _appPlayFSMstate;

struct _Player {
    int boxesWon;
    uint32_t color;
};
typedef struct _Player Player;

struct _Box {
    int boxesToWin;
    int boxesCompleted[MAX_BOXES][2];  // Tracks completion of boxes (sides completed, if box is complete)
    int linesDrawn[MAX_BOXES * 4];     // Tracks lines drawn on the game board
};
typedef struct _Box Box;

struct _Settings {
    int width;
    int height;
    int maxTurns;
    int numPlayers;
};
typedef struct _Settings Settings;

struct _GameState {
    int width;
    int height;
    int numBoxes;
    uint8_t lines[5][5][4]; // Store line status (top, right, bottom, left)
    uint8_t boxes[5][5];    // Store completed box status
    int currentTurn;        // Current player turn (0 = Player 1, 1 = Player 2)
};
typedef struct _GameState GameState;

struct _Application {
    // Main application structure to manage the game
    UART_Baudrate baudChoice;
    bool firstCall;
    _appGameFSMstate state;
    _appCursorFSMstate cursorState;
    _appPlayFSMstate playState;
    Settings settings;
    Player players[MAX_PLAYERS];
    Box boxes;
    int numTurn;
    int numPlayer;
    char rxChar;
    GameState gameState;
};
typedef struct _Application Application;

// Called once to construct the application
Application Application_construct();

// Main loop function
void Application_loop(Application* app, HAL* hal);

// Screen handlers for different FSM states
void Application_handleTitleScreen(Application* app_p, HAL* hal_p);
void Application_handleInstructionsScreen(Application* app_p, HAL* hal_p);
void Application_handleSettingsScreen(Application* app_p, HAL* hal_p);
void Application_handleGameScreen(Application* app_p, HAL* hal_p);
void Application_handleResultsScreen(Application* app_p, HAL* hal_p);

// Display functions for various screens
void Application_showTitleScreen(GFX* gfx_p);
void Application_showInstructionsScreen(GFX* gfx_p);
void Application_showSettingsScreen(Application* app_p, GFX* gfx_p);
void Application_showGameScreen(Application* app_p, GFX* gfx_p);
void Application_showResultsScreen(Application* app_p, GFX* gfx_p);

// Update cursor and settings during the game
void Application_updateCursor(Application* app_p, GFX* gfx_p);
void Application_updateSettings(Application* app_p, GFX* gfx_p);
void Application_updateGameScreen(Application* app_p, HAL* hal_p);

// UART and input handling
void Application_sendFirstQuestion(Application* app_p, UART* uart_p);
void Application_sendInvalidCoordinates(Application* app_p, UART* uart_p);
void Application_receiveCoordinates(Application* app_p, HAL* hal_p);
void Application_interpretCoordinates(Application* app_p, HAL* hal_p, int x, int y, char direction);

// Game logic helpers
void Application_checkBoxWon(Application* app_p);
void GameState_init(Application* app_p);
void GameState_drawLine(Application* app_p, uint8_t x, uint8_t y, char direction);
bool GameState_checkBoxCompletion(Application* app_p, uint8_t x, uint8_t y);
void GameState_displayBoard(Application* app_p, UART* uart_p);

// UART communications update
void Application_updateCommunications(Application* app, HAL* hal);

// Circular increment functions
uint32_t CircularIncrement(uint32_t value, uint32_t maximum);
uint32_t RangedCircularIncrement(uint32_t value, uint32_t minimum, uint32_t maximum);

#endif /* APPLICATION_H_ */

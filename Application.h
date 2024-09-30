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

#define COORDINATES_FORMAT_L 3
#define COORDINATES_FORMAT_N 4
#define COORDINATES_LEN 4

#define X1 0
#define Y1 1
#define X2 2
#define Y2 3

#define SIDES 4

#define MAX_PLAYERS 2
#define MAX_TURNS 40
#define MAX_BOXES 16

#define NAME_LEN 3

typedef enum { TitleScreen, InstructionsScreen, SettingsScreen,
               GameScreen, ResultsScreen } _appGameFSMstate;

typedef enum { Cursor_0, Cursor_1, Cursor_2, NUM_CURSOR_CHOICES } _appCursorFSMstate;

typedef enum { FirstQuestion, ReceiveInput, RoundOver } _appPlayFSMstate;

enum _appPlayerColors { Red = GRAPHICS_COLOR_RED, Blue = GRAPHICS_COLOR_BLUE };
typedef enum _appPlayerColors appPlayerColors;

enum _appInvalidCoordinates { SameCoordinate, ExistingLine };
typedef enum _appInvalidCoordinates appInvalidCoordinates;

struct _Player {
    int boxesWon;
    appPlayerColors color;
};
typedef struct _Player Player;

struct _Box {
    char coordinates[COORDINATES_LEN + 1];
    int boxesToWin;
    int boxes[MAX_BOXES][2];  // Record of Made Boxes (First Side, If Box is Complete)
    int linesDrawn[MAX_TURNS];  // Record of Existing Lines
};
typedef struct _Box Box;

struct _Settings {
    int width;
    int height;
    int maxTurns;
    int numPlayers;
};
typedef struct _Settings Settings;

struct _Application {
    // Put your application members and FSM state variables here!
    // =========================================================================
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
};
typedef struct _Application Application;

// Called only a single time - inside of main(), where the application is
// constructed
Application Application_construct();

// Called once per super-loop of the main application.
void Application_loop(Application* app, HAL* hal);

//
void Application_handleTitleScreen(Application* app_p, HAL* hal_p);
void Application_handleInstructionsScreen(Application* app_p, HAL* hal_p);
void Application_handleSettingsScreen(Application* app_p, HAL* hal_p);
void Application_handleGameScreen(Application* app_p, HAL* hal_p);

//
void Application_showTitleScreen(GFX* gfx_p);
void Application_showInstructionsScreen(GFX* gfx_p);
void Application_showSettingsScreen(Application* app_p, GFX* gfx_p);
void Application_showGameScreen(Application* app_p, GFX* gfx_p);
void Application_showResultsScreen(Application* app_p, GFX* gfx_p);

void Application_updateCursor(Application* app_p, GFX* gfx_p);
void Application_updateSettings(Application* app_p, GFX* gfx_p);
void Application_updateGameScreen(Application* app_p, HAL* hal_p);

void Application_sendFirstQuestion(Application* app_p, UART* uart_p);
void Application_sendInvalidCoordinates(Application* app_p, UART* uart_p);
void Application_receiveCoordinates(Application* app_p, HAL* hal_p);
void Application_interpretCoordinates(Application* app_p, HAL* hal_p);
bool Application_checkCoordinate(Application* app_p, GFX* gfx_p);

// Called whenever the UART module needs to be updated
void Application_updateCommunications(Application* app, HAL* hal);

// Generic circular increment function
uint32_t CircularIncrement(uint32_t value, uint32_t maximum);

// Ranged circular increment fuction
uint32_t RangedCircularIncrement(uint32_t value, uint32_t minimum, uint32_t maximum);

#endif /* APPLICATION_H_ */

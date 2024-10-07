/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* HAL and Application includes */
#include <Application.h>
#include <HAL/HAL.h>
#include <HAL/Timer.h>

// Non-blocking check. Whenever Launchpad S1 is pressed, LED1 turns on.
static void InitNonBlockingLED() {
    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
}

// Non-blocking check. Whenever Launchpad S1 is pressed, LED1 turns on.
static void PollNonBlockingLED() {
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
    if (GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN1) == 0) {
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
    }
}

/**
 * The main entry point of your project.
 */
int main(void) {
    // Stop Watchdog Timer - THIS SHOULD ALWAYS BE THE FIRST LINE OF YOUR MAIN
    WDT_A_holdTimer();

    // Initialize the system clock and background hardware timer
    InitSystemTiming();

    // Initialize the main Application object and HAL object
    HAL hal = HAL_construct();
    Application app = Application_construct();

    // Do not remove this line. This is your non-blocking check.
    InitNonBlockingLED();

    Application_showTitleScreen(&hal.gfx);

    // Main super-loop
    while (true) {
        PollNonBlockingLED();
        HAL_refresh(&hal);
        Application_loop(&app, &hal);
    }
}

/**
 * A helper function which increments a value with a maximum.
 * If incrementing the number causes the value to hit its maximum,
 * the number wraps around to 0.
 */
uint32_t CircularIncrement(uint32_t value, uint32_t maximum) {
    return (value + 1) % maximum;
}

uint32_t RangedCircularIncrement(uint32_t value, uint32_t minimum, uint32_t maximum) {
    return (value - (minimum - 1)) % (maximum - (minimum - 1)) + minimum;
}

/**
 * The main constructor for your application.
 * This function should initialize each of the FSMs which implement the application logic.
 */
Application Application_construct() {
    Application app;

    app.baudChoice       = BAUD_9600;
    app.firstCall        = true;
    app.state            = TitleScreen;
    app.cursorState      = Cursor_0;
    app.playState        = FirstQuestion;
    app.settings.width   = DEFAULT_DIM;
    app.settings.height  = DEFAULT_DIM;
    app.players[0].color = GRAPHICS_COLOR_RED;
    app.players[1].color = GRAPHICS_COLOR_BLUE;
    app.numTurn          = 0;
    app.numPlayer        = 0;

    GameState_init(&app);  // Initialize game state

    return app;
}

/* Game State Initialization */
void GameState_init(Application* app_p) {
    app_p->gameState.width = app_p->settings.width;
    app_p->gameState.height = app_p->settings.height;
    app_p->gameState.numBoxes = 0;
    app_p->gameState.currentTurn = 0;
    memset(app_p->gameState.lines, 0, sizeof(app_p->gameState.lines));
    memset(app_p->gameState.boxes, 0, sizeof(app_p->gameState.boxes));
}

/**
 * The main super-loop function of the application.
 */
void Application_loop(Application* app_p, HAL* hal_p) {
    if (Button_isTapped(&hal_p->boosterpackS2) || app_p->firstCall) {
        Application_updateCommunications(app_p, hal_p);
    }

    switch (app_p->state) {
        case TitleScreen:
            Application_handleTitleScreen(app_p, hal_p);
            break;

        case InstructionsScreen:
            Application_handleInstructionsScreen(app_p, hal_p);
            break;

        case SettingsScreen:
            Application_handleSettingsScreen(app_p, hal_p);
            break;

        case GameScreen:
            if (UART_hasChar(&hal_p->uart)) app_p->rxChar = UART_getChar(&hal_p->uart);
            Application_handleGameScreen(app_p, hal_p);
            break;

        case ResultsScreen:
            break;

        default: break;
    }
}

/**
 * Handles the GameScreen FSM state.
 */
void Application_handleGameScreen(Application* app_p, HAL* hal_p) {
    char inputBuffer[10];
    GameState_displayBoard(app_p, &hal_p->uart);  // Display game state

    UART_sendString(&hal_p->uart, "Enter coordinates and direction (e.g., 00U for Up): ");
    UART_receiveString(&hal_p->uart, inputBuffer, sizeof(inputBuffer));

    Application_receiveCoordinates(app_p, hal_p);  // Receive coordinates from player

    if (app_p->gameState.numBoxes == (app_p->gameState.width - 1) * (app_p->gameState.height - 1)) {
        UART_sendString(&hal_p->uart, "Game Over!\n");
        Application_showResultsScreen(app_p, &hal_p->gfx);
    }
}

/**
 * Interprets the coordinates entered by the player and processes the game move.
 */
void Application_interpretCoordinates(Application* app_p, HAL* hal_p, int x, int y, char direction) {
    bool isValid = true;

    if (direction == 'U' && x == 0) isValid = false;
    if (direction == 'D' && x == app_p->settings.height - 1) isValid = false;
    if (direction == 'L' && y == 0) isValid = false;
    if (direction == 'R' && y == app_p->settings.width - 1) isValid = false;

    if (isValid) {
        // Draw the line and check for completed boxes
        GameState_drawLine(app_p, x, y, direction);

        // Check for completed boxes after the line is drawn
        Application_checkBoxWon(app_p);

        app_p->playState = FirstQuestion;
    } else {
        Application_sendInvalidCoordinates(app_p, &hal_p->uart);
    }
}

/**
 * Draws a line on the game board and checks if any box is completed.
 */
void GameState_drawLine(Application* app_p, uint8_t x, uint8_t y, char direction) {
    int lineIndex = -1;
    switch (direction) {
        case 'U': lineIndex = 0; break;  // Top
        case 'R': lineIndex = 1; break;  // Right
        case 'D': lineIndex = 2; break;  // Bottom
        case 'L': lineIndex = 3; break;  // Left
        default: return;  // Invalid direction
    }

    if (lineIndex != -1 && app_p->gameState.lines[x][y][lineIndex] == 0) {
        app_p->gameState.lines[x][y][lineIndex] = 1;

        if (GameState_checkBoxCompletion(app_p, x, y)) {
            app_p->gameState.boxes[x][y] = app_p->gameState.currentTurn + 1;
            app_p->gameState.numBoxes++;
        }

        // Check for completed boxes after drawing the line
        Application_checkBoxWon(app_p);

        // Switch turns if no box completed
        if (!app_p->gameState.boxes[x][y]) {
            app_p->gameState.currentTurn = (app_p->gameState.currentTurn + 1) % 2;
        }
    }
}

/**
 * Checks if the box is completed.
 */
bool GameState_checkBoxCompletion(Application* app_p, uint8_t x, uint8_t y) {
    return app_p->gameState.lines[x][y][0] && app_p->gameState.lines[x][y][1] &&
           app_p->gameState.lines[x][y][2] && app_p->gameState.lines[x][y][3];
}

/**
 * Checks if any box was won after the line was drawn.
 */
void Application_checkBoxWon(Application* app_p) {
    int winCount = 0;
    int width = app_p->settings.width - 1;
    int i;
    for (i = 0; i < app_p->boxes.boxesToWin; i++) {
        int topLine = app_p->boxes.boxesCompleted[i][TOP_LINE];

        if (!app_p->boxes.boxesCompleted[i][BOX_COMPLETED] &&
            app_p->boxes.linesDrawn[topLine] &&
            app_p->boxes.linesDrawn[topLine + width] &&
            app_p->boxes.linesDrawn[topLine + width + 1] &&
            app_p->boxes.linesDrawn[topLine + width + 1 + width]) {

            app_p->boxes.boxesCompleted[i][BOX_COMPLETED] = 1;
            winCount++;
        }
    }

    if (winCount > 0) {
        app_p->players[app_p->numPlayer].boxesWon += winCount;
    } else {
        app_p->numPlayer = RangedCircularIncrement(app_p->numPlayer, 0, MAX_PLAYERS - 1);
    }
}

/**
 * Display the current game board.
 */
void GameState_displayBoard(Application* app_p, UART* uart_p)
{
    uint8_t i, j;
    for (i = 0; i < app_p->gameState.width; i++) {
        for (j = 0; j < app_p->gameState.height; j++) {
            if (app_p->gameState.lines[i][j][0]) UART_sendString(uart_p, "+---");
            else UART_sendString(uart_p, "+   ");

            if (j == app_p->gameState.width - 1) UART_sendString(uart_p, "+\n");

            if (app_p->gameState.lines[i][j][3]) UART_sendString(uart_p, "|");
            else UART_sendString(uart_p, " ");

            if (app_p->gameState.boxes[i][j] != 0) {
                char buffer[10];
                snprintf(buffer, sizeof(buffer), " P%d ", app_p->gameState.boxes[i][j]);
                UART_sendString(uart_p, buffer);
            } else {
                UART_sendString(uart_p, "   ");
            }

            if (j == app_p->gameState.width - 1 && app_p->gameState.lines[i][j][1]) UART_sendString(uart_p, "|\n");
            else if (j == app_p->gameState.width - 1) UART_sendString(uart_p, "\n");
        }
    }
    uint8_t ji;
    for (ji = 0; ji < app_p->gameState.width; ji++) {
        UART_sendString(uart_p, "+---");
    }
    UART_sendString(uart_p, "+\n");
}

/**
 * Updates the communications and LEDs based on the baud rate.
 */
void Application_updateCommunications(Application* app_p, HAL* hal_p) {
    if (app_p->firstCall) {
        app_p->firstCall = false;
    } else {
        uint32_t newBaudNumber = CircularIncrement((uint32_t)app_p->baudChoice, NUM_BAUD_CHOICES);
        app_p->baudChoice = (UART_Baudrate)newBaudNumber;
    }

    UART_SetBaud_Enable(&hal_p->uart, app_p->baudChoice);

    LED_turnOff(&hal_p->launchpadLED2Red);
    LED_turnOff(&hal_p->launchpadLED2Green);
    LED_turnOff(&hal_p->launchpadLED2Blue);

    switch (app_p->baudChoice) {
        case BAUD_9600:
            LED_turnOn(&hal_p->launchpadLED2Red);
            break;
        case BAUD_19200:
            LED_turnOn(&hal_p->launchpadLED2Green);
            break;
        case BAUD_38400:
            LED_turnOn(&hal_p->launchpadLED2Blue);
            break;
        case BAUD_57600:
            LED_turnOn(&hal_p->launchpadLED2Red);
            LED_turnOn(&hal_p->launchpadLED2Green);
            LED_turnOn(&hal_p->launchpadLED2Blue);
            break;
        default:
            break;
    }
}

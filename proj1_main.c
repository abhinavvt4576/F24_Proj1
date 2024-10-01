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
 * The main entry point of your project. The main function should immediately
 * stop the Watchdog timer, call the Application constructor, and then
 * repeatedly call the main super-loop function. The Application constructor
 * should be responsible for initializing all hardware components as well as all
 * other finite state machines you choose to use in this project.
 *
 * THIS FUNCTION IS ALREADY COMPLETE. Unless you want to temporarily experiment
 * with some behavior of a code snippet you may have, we DO NOT RECOMMEND
 * modifying this function in any way.
 */
int main(void) {
    // Stop Watchdog Timer - THIS SHOULD ALWAYS BE THE FIRST LINE OF YOUR MAIN
    WDT_A_holdTimer();

    // Initialize the system clock and background hardware timer, used to enable
    // software timers to time their measurements properly.
    InitSystemTiming();

    // Initialize the main Application object and HAL object
    HAL hal = HAL_construct();
    Application app = Application_construct();

    // Do not remove this line. This is your non-blocking check.
    InitNonBlockingLED();

    Application_showTitleScreen(&hal.gfx);

    // Main super-loop! In a polling architecture, this function should call
    // your main FSM function over and over.
    while (true) {
        // Do not remove this line. This is your non-blocking check.
        PollNonBlockingLED();
        HAL_refresh(&hal);
        Application_loop(&app, &hal);
    }
}

/**
 * A helper function which increments a value with a maximum. If incrementing
 * the number causes the value to hit its maximum, the number wraps around
 * to 0.
 */
uint32_t CircularIncrement(uint32_t value, uint32_t maximum) {
    return (value + 1) % maximum;
}

uint32_t RangedCircularIncrement(uint32_t value, uint32_t minimum, uint32_t maximum) {
    return (value - (minimum - 1)) % (maximum - (minimum - 1)) + minimum;
}

/**
 * The main constructor for your application. This function should initialize
 * each of the FSMs which implement the application logic of your project.
 *
 * @return a completely initialized Application object
 */
Application Application_construct() {
    Application app;

    // Initialize local application state variables here!
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

    app.boxes.coordinates[COORDINATES_FORMAT_L] = '\0';
    app.boxes.coordinates[COORDINATES_FORMAT_N] = '\0';

    return app;
}

/**
 * The main super-loop function of the application. We place this inside of a
 * single infinite loop in main. In this way, we can model a polling system of
 * FSMs. Every cycle of this loop function, we poll each of the FSMs one time,
 * followed by refreshing all inputs to the system through a convenient
 * [HAL_refresh()] call.
 *
 * @param app_p:  A pointer to the main Application object.
 * @param hal_p:  A pointer to the main HAL object
 */
void Application_loop(Application* app_p, HAL* hal_p) {
    // Restart/Update communications if either this is the first time the
    // application is run or if BoosterPack S2 is pressed (which means a new
    // baudrate is being set up)
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
* Callback function for when the game is in the TITLE_SCREEN state.
*
* @param app_p: A pointer to the main Application object.
*/
void Application_handleTitleScreen(Application* app_p, HAL* hal_p) {
    
    // Display the Settings Screen if LB1 Tapped
    if (Button_isTapped(&hal_p->launchpadS1)) {
        app_p->state = SettingsScreen;
        Application_showSettingsScreen(app_p, &hal_p->gfx);
    }

    // Display the Instructions Screen if LB2 Tapped
    else if (Button_isTapped(&hal_p->launchpadS2)) {
        app_p->state = InstructionsScreen;
        Application_showInstructionsScreen(&hal_p->gfx);
    }

}

void Application_handleInstructionsScreen(Application* app_p, HAL* hal_p) {

    // Display the Title Screen if LB2 Tapped
    if (Button_isTapped(&hal_p->launchpadS2)) {
        app_p->state = TitleScreen;
        Application_showTitleScreen(&hal_p->gfx);
    }

}

void Application_handleSettingsScreen(Application* app_p, HAL* hal_p) {

    // Update Cursor Position if JSB Tapped
    if (Button_isTapped(&hal_p->boosterpackJS)) {
        app_p->cursorState = (_appCursorFSMstate)CircularIncrement((uint32_t)app_p->cursorState, NUM_CURSOR_CHOICES);
        Application_updateCursor(app_p, &hal_p->gfx);
    }

    // Update Width/Height of Game Board OR Display Game Screen if BB1 is Tapped
    if (Button_isTapped(&hal_p->boosterpackS1)) {
        Application_updateSettings(app_p, &hal_p->gfx);
    }

}

void Application_handleGameScreen(Application* app_p, HAL* hal_p) {

    // Play Game Until All Turns Done
    if      (app_p->numTurn < app_p->settings.maxTurns) Application_updateGameScreen(app_p, hal_p);

    // Send End Game Instructions to View Results
    else if (app_p->numTurn++ == app_p->settings.maxTurns) {
        char instr[] = "Press BB1 to end the game";
        UART_sendString(&hal_p->uart, instr);
    }

    // Display Results Screen
    else if (Button_isTapped(&hal_p->boosterpackS1)) {
        app_p->state = ResultsScreen;
        Application_showResultsScreen(app_p, &hal_p->gfx);
    }

}

void Application_showTitleScreen(GFX* gfx_p) {

    GFX_clear(gfx_p);

    GFX_print(gfx_p, "Dots and Boxes", 2, 1);

    GFX_print(gfx_p, "NAME", 5, 1);

    GFX_print(gfx_p, "LB1: Play Game",    7, 1);
    GFX_print(gfx_p, "LB2: Instructions", 8, 1);

}

void Application_showInstructionsScreen(GFX* gfx_p) {

    GFX_clear(gfx_p);

    GFX_print(gfx_p, "Instructions Screen", 2, 1);

    char* instr[] = { "Select the dimensions",
                      "for the board. Each",
                      "player connects the",
                      "adjacent dots with",
                      "lines by entering",
                      "coordinates to make",
                      "a box. A player goes",
                      "again if they make a",
                      "box. Results are shown",
                      "when last line is",
                      "drawn."};

    int length = (int)(sizeof(instr) / sizeof(instr[0]));
    GFX_printTextRows(gfx_p, instr, length, 3.5, 0);

    GFX_print(gfx_p, "LB2: Go Back", 15, 1);

}

/**
* A helper function which clears the screen.
*
* @param app_p: A pointer to the main Application object.
* @param gfx_p: A pointer to the main GFX object.
*/
void Application_showSettingsScreen(Application* app_p, GFX* gfx_p) {

    GFX_clear(gfx_p);

    GFX_print(gfx_p, "Settings", 2, 6.5);

    char* instr[] = { "Tap JSB to switch",
                      "between Width, Height",
                      "and PLAY",
                      "Tap BB1 to change #",
                      "or confirm settings",
                      "to play game"};

    int length = (int)(sizeof(instr) / sizeof(instr[0]));
    GFX_printTextRows(gfx_p, instr, length, 4, 0);

    char num[] = "0";

    num[0] += app_p->settings.width;
    GFX_print(gfx_p, "Width:", 11, 1);
    GFX_print(gfx_p, num, 11, 9);

    num[0] = app_p->settings.height + '0';
    GFX_print(gfx_p, "Height:", 12, 1);
    GFX_print(gfx_p, num, 12, 9);

    GFX_print(gfx_p, "PLAY", 14, 8.5);

    GFX_print(gfx_p, "*", 11, 13.5);

}

void Application_showGameScreen(Application* app_p, GFX* gfx_p) {

    GFX_clear(gfx_p);

    int spaceWidth  = 128 / app_p->settings.width - 1;
    int spaceHeight = 128 / app_p->settings.height - 1;

    float i, j; for (i = 0.5; i <= app_p->settings.width; i++) {
        for (j = 0.5; j <= app_p->settings.height; j++) {
            GFX_drawSolidCircle(gfx_p, 2.5 + spaceWidth * i, 2.5 + spaceHeight * j, 1);
        }
    }

    GFX_print(gfx_p, "Game Screen", 15, 5.5);

}

void Application_showResultsScreen(Application* app_p, GFX* gfx_p) {

    GFX_clear(gfx_p);

    char results[13], winner[] = "Winner: Player  #";

    GFX_print(gfx_p, "Results Screen", 1, 3.5);

    sprintf(results, "Player %i: %i", 1, app_p->players[0].boxesWon);
    GFX_print(gfx_p, results, 3, 1);

    sprintf(results, "Player %i: %i", 2, app_p->players[1].boxesWon);
    GFX_print(gfx_p, results, 4, 1);

    if (app_p->players[0].boxesWon == app_p->players[1].boxesWon) GFX_print(gfx_p, "TIE", 6, 1);
    else {
        char player = (app_p->players[0].boxesWon > app_p->players[1].boxesWon) ? '1' : '2';
        winner[16] = player;
        GFX_print(gfx_p, winner, 6, 1);
    }

}

void Application_updateCursor(Application* app_p, GFX* gfx_p) {

    char* asterick = "*";

    switch (app_p->cursorState) {

    case Cursor_0:
        GFX_eraseText(gfx_p, asterick, 12, 13.5);
        GFX_eraseText(gfx_p, asterick, 14, 13.5);
        GFX_print(gfx_p, asterick, 11, 13.5);
        break;

    case Cursor_1:
        GFX_eraseText(gfx_p, asterick, 11, 13.5);
        GFX_eraseText(gfx_p, asterick, 14, 13.5);
        GFX_print(gfx_p, asterick, 12, 13.5);
        break;

    case Cursor_2:
        GFX_eraseText(gfx_p, asterick, 11, 13.5);
        GFX_eraseText(gfx_p, asterick, 12, 13.5);
        GFX_print(gfx_p, asterick, 14, 13.5);
        break;

    default: break;

    }

}

void Application_updateSettings(Application* app_p, GFX* gfx_p) {

    char setting[] = "0";

    switch (app_p->cursorState) {

    // Increment Width
    case Cursor_0:
        app_p->settings.width = RangedCircularIncrement(app_p->settings.width, MIN_DIM, MAX_DIM);
        setting[0] += app_p->settings.width;
        GFX_print(gfx_p, setting, 11, 9);
        break;

    // Increment Height
    case Cursor_1:
        app_p->settings.height = RangedCircularIncrement(app_p->settings.height, MIN_DIM, MAX_DIM);
        setting[0] += app_p->settings.height;
        GFX_print(gfx_p, setting, 12, 9);
        break;

    // Display Game Screen
    case Cursor_2:
        app_p->state = GameScreen;
        app_p->settings.maxTurns = app_p->settings.width  * (app_p->settings.height - 1) +
                                   app_p->settings.height * (app_p->settings.width - 1);
        app_p->boxes.boxesToWin  = (app_p->settings.width - 1) * (app_p->settings.height - 1);

        int i, j = 1;
        
        for (i = 0; i < app_p->settings.maxTurns; i++) app_p->boxes.linesDrawn[i] = 0;

        int topLine = 0;
        for (i = 0; i < app_p->settings.height - 1; i++) {
            for (j = 0; j < app_p->settings.width - 1; j++) {
                app_p->boxes.boxesCompleted[i * (app_p->settings.width - 1) + j][TOP_LINE] = topLine++;
                app_p->boxes.boxesCompleted[i * (app_p->settings.width - 1) + j][BOX_COMPLETED] = 0;
            }
            topLine += app_p->settings.width;
        }
        Application_showGameScreen(app_p, gfx_p);

        break;

    default: break;

    }

}

void Application_updateGameScreen(Application* app_p, HAL* hal_p) {

    switch (app_p->playState) {

    case FirstQuestion:
        Application_sendFirstQuestion(app_p, &hal_p->uart);
        app_p->playState = ReceiveInput;

    case ReceiveInput:
        if (app_p->rxChar != 1) Application_receiveCoordinates(app_p, hal_p);
        break;

    default: break;

    }

}

void Application_sendFirstQuestion(Application* app_p, UART* uart_p) {

    char player[] = "Player #";
    char instr[] = ", please enter a number from 0-# for X and 0-# for Y with any of the following formats:\n\
                    \rXYU (U - Line UP)\n\
                    \rXYD (D - Line DOWN)\n\
                    \rXYL (L - Line LEFT)\n\
                    \rXYR (R - Line RIGHT)\n\
                    \rX1XY1X2Y2\n\
                    \rPress enter to submit your move: ";

    player[7] = app_p->numPlayer + 1 + '0';
    UART_sendString(uart_p, player);

    instr[31] = app_p->settings.width  - 1 + '0';
    instr[45] = app_p->settings.height - 1 + '0';
    UART_sendString(uart_p, instr);

}

void Application_sendInvalidCoordinates(Application* app_p, UART* uart_p) {

    char instr[] = "Please enter a number from 0-# for X and 0-# for Y with any of the following formats:\n\
                    \rXYU (U - Line UP)\n\
                    \rXYD (D - Line DOWN)\n\
                    \rXYL (L - Line LEFT)\n\
                    \rXYR (R - Line RIGHT)\n\
                    \rX1XY1X2Y2\n\
                    \rPress enter to submit your move: ";
    instr[29] = app_p->settings.width - 1 + '0';
    instr[43] = app_p->settings.height - 1 + '0';
    UART_sendString(uart_p, instr);

}

void Application_receiveCoordinates(Application* app_p, HAL* hal_p) {

    static int i = 0;

    // Accept the Valid Number of Characters
    if (i < COORDINATES_LEN) {

        int maxDim = (app_p->settings.width > app_p->settings.height) ? app_p->settings.width : app_p->settings.height;

        if ((app_p->rxChar >= '0' && app_p->rxChar <= maxDim + '0') ||
            (i > 1 && (toupper(app_p->rxChar) == 'U' || toupper(app_p->rxChar) == 'D' || toupper(app_p->rxChar) == 'L' || toupper(app_p->rxChar) == 'R'))) {
            if ((app_p->rxChar >= 'a' && app_p->rxChar <= 'z') || (app_p->rxChar >= 'A' && app_p->rxChar <= 'Z')) {
                app_p->boxes.coordinates[i] = toupper(app_p->rxChar);
                UART_sendChar(&hal_p->uart, app_p->boxes.coordinates[i]);
                if (i == COORDINATES_LEN - 1 - 1) i = COORDINATES_LEN;
            }
            else {
                app_p->boxes.coordinates[i] = app_p->rxChar;
                UART_sendChar(&hal_p->uart, app_p->boxes.coordinates[i++]);
            }
        }

    }



    if (i >= COORDINATES_LEN) {

        UART_sendChar(&hal_p->uart, '\n');
        UART_sendChar(&hal_p->uart, '\n');
        UART_sendChar(&hal_p->uart, '\r');

        Application_interpretCoordinates(app_p, hal_p);

        i = 0;
        app_p->boxes.coordinates[COORDINATES_FORMAT_L] = '\0';

    }


    app_p->rxChar = 1;

}

void Application_interpretCoordinates(Application* app_p, HAL* hal_p) {

    bool valid = true;

    if (strlen(app_p->boxes.coordinates) == COORDINATES_FORMAT_N) {

        if      (abs((app_p->boxes.coordinates[X1] - '0') - (app_p->boxes.coordinates[X2] - '0')) > 1 ||
                 abs((app_p->boxes.coordinates[Y1] - '0') - (app_p->boxes.coordinates[Y2] - '0') > 1)) valid = false;
        else if (abs((app_p->boxes.coordinates[X1] - '0') - (app_p->boxes.coordinates[X2] - '0')) >= 1 &&
                 abs((app_p->boxes.coordinates[Y1] - '0') - (app_p->boxes.coordinates[Y2] - '0') >= 1)) valid = false;
        else if ((app_p->boxes.coordinates[X1] - '0') - (app_p->boxes.coordinates[X2] - '0') > 0) app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] = 'U';
        else if ((app_p->boxes.coordinates[X1] - '0') - (app_p->boxes.coordinates[X2] - '0') < 0) app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] = 'D';
        else if ((app_p->boxes.coordinates[Y1] - '0') - (app_p->boxes.coordinates[Y2] - '0') > 0) app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] = 'L';
        else if ((app_p->boxes.coordinates[Y1] - '0') - (app_p->boxes.coordinates[Y2] - '0') < 0) app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] = 'R';
        else valid = false;
    }
    


    if (valid) {
        if      (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'U' && app_p->boxes.coordinates[0] == '0')                              valid = false;
        else if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'D' && app_p->boxes.coordinates[0] == app_p->settings.height - 1 + '0') valid = false;
        else if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'L' && app_p->boxes.coordinates[1] == '0')                              valid = false;
        else if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'R' && app_p->boxes.coordinates[1] == app_p->settings.width - 1 + '0')  valid = false;
    }

    if (valid && Application_checkCoordinate(app_p, &hal_p->gfx)){

        GFX_setForeground(&hal_p->gfx, app_p->players[app_p->numPlayer].color);

        int spaceWidth = 128 / app_p->settings.width - 1;
        int spaceHeight = 128 / app_p->settings.height - 1;
        float x = app_p->boxes.coordinates[X1] - '0' + 0.5;
        float y = app_p->boxes.coordinates[Y1] - '0' + 0.5;

        if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'U')
            GFX_drawLine(&hal_p->gfx, 2.5 + spaceWidth * y, 2.5 + spaceWidth * y, 2.5 + spaceHeight * x, 2.5 + spaceHeight * (x - 1));
        else if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'D')
            GFX_drawLine(&hal_p->gfx, 2.5 + spaceWidth * y, 2.5 + spaceWidth * y, 2.5 + spaceHeight * x, 2.5 + spaceHeight * (x + 1));
        else if (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1] == 'L')
            GFX_drawLine(&hal_p->gfx, 2.5 + spaceWidth * y, 2.5 + spaceWidth * (y - 1), 2.5 + spaceHeight * x, 2.5 + spaceHeight * x);
        else GFX_drawLine(&hal_p->gfx, 2.5 + spaceWidth * y, 2.5 + spaceWidth * (y + 1), 2.5 + spaceHeight * x, 2.5 + spaceHeight * x);

        GFX_setForeground(&hal_p->gfx, FG_COLOR);

        Application_checkBoxWon(app_p);

        app_p->numTurn++;
        app_p->playState = FirstQuestion;

    }

    else Application_sendInvalidCoordinates(app_p, &hal_p->uart);

}

bool Application_checkCoordinate(Application* app_p, GFX* gfx_p) {

    int side;

    switch (app_p->boxes.coordinates[COORDINATES_FORMAT_L - 1]) {
    case 'U':
        side = (app_p->settings.height * 2 - 1) * ((int)(app_p->boxes.coordinates[X1] - '0') - 1) + (int)(app_p->boxes.coordinates[Y1] - '0') + (app_p->settings.width - 1);    // (Height * 2 - 1) * X + Y + (Width - 1)
        break;
    case 'D':
        side = (app_p->settings.height * 2 - 1) * (int)(app_p->boxes.coordinates[X1] - '0') + (int)(app_p->boxes.coordinates[Y1] - '0') + (app_p->settings.width - 1); // (Height * 2 - 1) X +
        break;
    case 'L':
        side = (app_p->settings.height * 2 - 1) * (int)(app_p->boxes.coordinates[X1] - '0') + ((int)(app_p->boxes.coordinates[Y1] - '0') - 1);   // (Height * 2 - 1) * X + (Y-1)
        break;
    case 'R':
        side = (app_p->settings.height * 2 - 1) * (int)(app_p->boxes.coordinates[X1] - '0') + (int)(app_p->boxes.coordinates[Y1] - '0');    // (Height * 2 - 1) * X + Y
        break;
    default: break;
    }

    bool valid = (app_p->boxes.linesDrawn[side] == 1) ? false : true;

    app_p->boxes.linesDrawn[side] = 1;

    return valid;

}

void Application_checkBoxWon(Application* app_p) {

    int winCount = 0;
    int spacing = app_p->settings.width - 1;

    int i; for (i = 0; i < app_p->boxes.boxesToWin; i++) {
        //

        int topLine = app_p->boxes.boxesCompleted[i][TOP_LINE];

        if (app_p->boxes.boxesCompleted[i][BOX_COMPLETED]) ;                // Don't Bother to Check a Completed Box
        else if (app_p->boxes.linesDrawn[topLine] != 1) ;
        else if (app_p->boxes.linesDrawn[topLine + spacing] != 1) ;
        else if (app_p->boxes.linesDrawn[topLine + spacing + 1] != 1) ;
        else if (app_p->boxes.linesDrawn[topLine + spacing + 1 + spacing] != 1);
        else {
            winCount++;
            app_p->boxes.boxesCompleted[i][BOX_COMPLETED] = 1;
        }
    }

    if (winCount == 0) app_p->numPlayer = RangedCircularIncrement(app_p->numPlayer, 0, 1);   // Change Turns if No Player Wins
    else app_p->players[app_p->numPlayer].boxesWon += winCount;

}

/**
 * Updates which LEDs are lit and what baud rate the UART module communicates
 * with, based on what the application's baud choice is at the time this
 * function is called.
 *
 * @param app_p:  A pointer to the main Application object.
 * @param hal_p:  A pointer to the main HAL object
 */
void Application_updateCommunications(Application* app_p, HAL* hal_p) {
    // When this application first loops, the proper LEDs aren't lit. The
    // firstCall flag is used to ensure that the
    if (app_p->firstCall) {
        app_p->firstCall = false;
    }

    // When BoosterPack S2 is tapped, circularly increment which baud rate is
    // used.
    else {
        uint32_t newBaudNumber =
                CircularIncrement((uint32_t)app_p->baudChoice, NUM_BAUD_CHOICES);
        app_p->baudChoice = (UART_Baudrate)newBaudNumber;
    }

    // Start/update the baud rate according to the one set above.
    UART_SetBaud_Enable(&hal_p->uart, app_p->baudChoice);

    // Based on the new application choice, turn on the correct LED.
    // To make your life easier, we recommend turning off all LEDs before
    // selectively turning back on only the LEDs that need to be relit.
    // -------------------------------------------------------------------------
    LED_turnOff(&hal_p->launchpadLED2Red);
    LED_turnOff(&hal_p->launchpadLED2Green);
    LED_turnOff(&hal_p->launchpadLED2Blue);

    // TODO: Turn on all appropriate LEDs according to the tasks below.
    switch (app_p->baudChoice) {
    // When the baud rate is 9600, turn on Launchpad LED Red
    case BAUD_9600:
        LED_turnOn(&hal_p->launchpadLED2Red);
        break;

        // TODO: When the baud rate is 19200, turn on Launchpad LED Green
    case BAUD_19200:
        LED_turnOn(&hal_p->launchpadLED2Green);
        break;

        // TODO: When the baud rate is 38400, turn on Launchpad LED Blue
    case BAUD_38400:
        LED_turnOn(&hal_p->launchpadLED2Blue);
        break;

        // TODO: When the baud rate is 57600, turn on all Launchpad LEDs
        // (illuminates white)
    case BAUD_57600:
        LED_turnOn(&hal_p->launchpadLED2Red);
        LED_turnOn(&hal_p->launchpadLED2Green);
        LED_turnOn(&hal_p->launchpadLED2Blue);
        break;

        // In the default case, this program will do nothing.
    default:
        break;
    }
}

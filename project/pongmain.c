#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include <stdio.h>

// Ball variables
short ballPos[2] = {screenWidth / 2, screenHeight / 2}; // Ball position
short ballVelocity[2] = {1, 1};                        // Ball velocity (dx, dy)

// Paddle variables
#define PADDLE_WIDTH 2
#define PADDLE_HEIGHT 20
#define PADDLE_OFFSET 5 // Distance from edge of the screen
short playerPaddlePos = screenHeight / 2; // Player paddle position

// Score variable
int playerScore = 0;

// Game control variables
int switches = 0;        // Current switch states
short redrawScreen = 1;  // Flag to trigger redraw
short paused = 0;        // Game paused state

// Switch definitions
#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8
#define SWITCHES (SW1 | SW2 | SW3 | SW4)

// Function prototypes
void update_ball();
void update_player_paddle();
void draw_paddle(short x, short y, u_int color);
void draw_ball(short x, short y, u_int color);
void update_shape();
void reset_game();
void pause_game();
void switch_interrupt_handler();
void __interrupt_vec(PORT2_VECTOR) Port_2();
void wdt_c_handler();
void switch_init();
static char switch_update_interrupt_sense();

// Initialize switches
void switch_init() {
  P2REN |= SWITCHES;  // Enable resistors for switches
  P2IE |= SWITCHES;   // Enable interrupts for switches
  P2OUT |= SWITCHES;  // Pull-ups for switches
  P2DIR &= ~SWITCHES; // Set switches as inputs
  switch_update_interrupt_sense();
}

// Update switch interrupt sense
static char switch_update_interrupt_sense() {
  char p2val = P2IN;
  P2IES |= (p2val & SWITCHES);  // Sense falling edge
  P2IES &= (p2val | ~SWITCHES); // Sense rising edge
  return p2val;
}

// Ball movement logic
void update_ball() {
  // Update ball position
  ballPos[0] += ballVelocity[0];
  ballPos[1] += ballVelocity[1];

  // Check for top and bottom screen collisions
  if (ballPos[1] <= 0 || ballPos[1] >= screenHeight) {
    ballVelocity[1] = -ballVelocity[1]; // Reverse Y direction
  }

  // Check for player paddle collision
  if (ballPos[0] <= PADDLE_OFFSET + PADDLE_WIDTH &&
      ballPos[1] >= playerPaddlePos - (PADDLE_HEIGHT / 2) &&
      ballPos[1] <= playerPaddlePos + (PADDLE_HEIGHT / 2)) {
    ballVelocity[0] = -ballVelocity[0]; // Reverse X direction
    playerScore++;                      // Increase score
    buzzer_set_period(1000);            // Play sound
    __delay_cycles(500000);             // Small delay for the sound
    buzzer_off();                       // Turn off the buzzer
  }

  // Check if the ball goes past the player paddle
  if (ballPos[0] <= 0) {
    buzzer_set_period(500); // Play "miss" sound
    __delay_cycles(500000); // Small delay for the sound
    buzzer_off();
    reset_game(); // Reset game if the ball passes the paddle
  }

  // Prevent the ball from wrapping around the screen width
  if (ballPos[0] >= screenWidth) {
    ballVelocity[0] = -ballVelocity[0]; // Reverse X direction
  }
}

// Update player paddle position based on switches
void update_player_paddle() {
  if (switches & SW1) { // Move up
    if (playerPaddlePos > PADDLE_HEIGHT / 2) {
      playerPaddlePos -= 2;
    }
  }
  if (switches & SW2) { // Move down
    if (playerPaddlePos < screenHeight - (PADDLE_HEIGHT / 2)) {
      playerPaddlePos += 2;
    }
  }
}

// Draw paddle
void draw_paddle(short x, short y, u_int color) {
  fillRectangle(x, y - (PADDLE_HEIGHT / 2), PADDLE_WIDTH, PADDLE_HEIGHT, color);
}

// Draw ball
void draw_ball(short x, short y, u_int color) {
  fillRectangle(x - 1, y - 1, 3, 3, color); // Ball is a 3x3 square
}

// Reset the game
void reset_game() {
  ballPos[0] = screenWidth / 2;
  ballPos[1] = screenHeight / 2;
  ballVelocity[0] = 1;
  ballVelocity[1] = 1;
  playerPaddlePos = screenHeight / 2;
  playerScore = 0;

  buzzer_set_period(2000); // Play "reset" sound
  __delay_cycles(500000); // Small delay for the sound
  buzzer_off();
  
  redrawScreen = 1;
}

// Pause or resume the game
void pause_game() {
  paused = !paused; // Toggle the paused state

  if (paused) {
    buzzer_set_period(1500); // Play "pause" sound
    __delay_cycles(250000); // Small delay for the sound
    buzzer_off();
    // Display "Paused" message at the center of the screen
    drawString11x16(screenWidth / 2 - 33, screenHeight / 2 - 8, "Paused", COLOR_WHITE, COLOR_BLACK);
  } else {
    buzzer_set_period(1000); // Play "resume" sound
    __delay_cycles(250000); // Small delay for the sound
    buzzer_off();
    // Erase "Paused" message by overwriting it with the background color
    drawString11x16(screenWidth / 2 - 33, screenHeight / 2 - 8, "Paused", COLOR_BLACK, COLOR_BLACK);
  }
}

short prevBallPos[2] = {screenWidth / 2, screenHeight / 2};
short prevPaddlePos = screenHeight / 2;

void update_shape() {
  // Erase the previous ball position
  draw_ball(prevBallPos[0], prevBallPos[1], COLOR_BLACK);

  // Erase the previous paddle position
  draw_paddle(PADDLE_OFFSET, prevPaddlePos, COLOR_BLACK);

  // Draw the current ball position
  draw_ball(ballPos[0], ballPos[1], COLOR_WHITE);

  // Draw the current paddle position
  draw_paddle(PADDLE_OFFSET, playerPaddlePos, COLOR_GREEN);

  // Display the score
  char scoreStr[20];
  sprintf(scoreStr, "Score: %d", playerScore);
  drawString5x7(5, 5, scoreStr, COLOR_YELLOW, COLOR_BLACK);

  // Update previous positions for the next frame
  prevBallPos[0] = ballPos[0];
  prevBallPos[1] = ballPos[1];
  prevPaddlePos = playerPaddlePos;
}

// Switch interrupt handler
void switch_interrupt_handler() {
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES; // Update switches state

  // Handle game control switches
  if (switches & SW3) { // Reset game
    reset_game();
  }
  if (switches & SW4) { // Pause game
    pause_game();
  }
}

// Watchdog Timer interrupt handler
void wdt_c_handler() {
  static int updateCounter = 0;

  if (paused) {
    return; // Do nothing if the game is paused
  }

  updateCounter++;
  if (updateCounter >= 5) { // Adjust speed (lower = faster)
    update_player_paddle(); // Update player paddle
    update_ball();          // Update ball position
    redrawScreen = 1;       // Redraw screen
    updateCounter = 0;
  }
}

void main() {
  configureClocks();
  lcd_init();
  switch_init();
  buzzer_init();
  enableWDTInterrupts();
  or_sr(0x8); // Enable interrupts

  clearScreen(COLOR_BLACK); // Initial screen clear

  while (1) {
    if (redrawScreen) {
      redrawScreen = 0;
      update_shape(); // Update only the affected regions
    }
    or_sr(0x10); // Enter low-power mode
  }
}

// Port 2 interrupt handler
void __interrupt_vec(PORT2_VECTOR) Port_2() {
  if (P2IFG & SWITCHES) {      // Check if a switch caused the interrupt
    P2IFG &= ~SWITCHES;      // Clear interrupt flags
    switch_interrupt_handler();
  }
 }

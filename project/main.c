#include <msp430.h>
#include <libTimer.h>
#include "lcdutils.h"
#include "lcddraw.h"
#include <stdlib.h>


// WARNING: LCD DISPLAY USES P1.0.  Do not touch!!!


#define LED BIT6/* note that bit zero req'd for display */

#define SW1 1
#define SW2 2
#define SW3 4
#define SW4 8

#define NOTE_WIDTH 10         // Width of a falling note
#define NOTE_HEIGHT 3         // Height of a falling note
#define TARGET_ROW (screenHeight - 10) // Row to hit notes
#define FALL_SPEED 1          // Speed of falling notes
#define SWITCHES 15


typedef struct {
  int col;       // Column position
  int row;       // Current row position
  char active;   // Whether the note is active
  char switchId; // Corresponding switch for this note (0 = SW1, 1 = SW2, etc.)
} Note;

// Global variables
Note note;                // Single note structure
int score = 0;            // Player score
int fallSpeed = FALL_SPEED; // Speed of falling notes
int switches = 0;         // Current switch states
short redrawScreen = 1;   // Flag to trigger screen redraw

// Function prototypes
void init_note();
void spawn_note();
void update_note();
void check_hits();
void draw_note();
void clear_note();
void update_shape();
void wdt_c_handler();
void switch_interrupt_handler();
static char switch_update_interrupt_sense();
void switch_init();
void __interrupt_vec(PORT2_VECTOR) Port_2();

// Initialize the note
void init_note() {
  note.active = 0; // Initially inactive
}

// Spawn a new note in a random column
void spawn_note() {
  if (!note.active) {
    int columnIndex = rand() % 4; // Random column (0 to 3)
    note.col = (screenWidth / 4) * columnIndex + (screenWidth / 8) - (NOTE_WIDTH / 2);
    note.row = 0;          // Start at the top
    note.active = 1;       // Activate note
    note.switchId = columnIndex; // Assign to corresponding switch
  }
}

// Update note position
void update_note() {
  if (note.active) {
    clear_note();          // Clear the note's previous position
    note.row += fallSpeed; // Move note down
    if (note.row >= screenHeight) { // Missed note
      note.active = 0;   // Deactivate note
      score -= 1;        // Decrease score
    }
  }
}

// Check if the active note was hit
void check_hits() {
  if (note.active && note.row >= TARGET_ROW - NOTE_HEIGHT &&
      note.row <= TARGET_ROW) {
    if (switches & (1 << note.switchId)) { // Switch matches note
      note.active = 0; // Deactivate note
      score += 1;      // Increase score
    }
  }
}

// Draw the active note
void draw_note() {
  if (note.active) {
    fillRectangle(note.col, note.row, NOTE_WIDTH, NOTE_HEIGHT, COLOR_WHITE);
  }
}

// Clear the note from the screen
void clear_note() {
  fillRectangle(note.col, note.row, NOTE_WIDTH, NOTE_HEIGHT, COLOR_BLACK);
}

// Update the shape (graphics)
void update_shape() {
  if (note.active) {
    draw_note(); // Draw the current note if active
  }

  // Display the score
  char scoreStr[10];
  sprintf(scoreStr, "Score: %d", score);
  drawString5x7(5, 5, scoreStr, COLOR_GREEN, COLOR_BLACK);
}

static char
switch_update_interrupt_sense()
{
  char p2val = P2IN;
  /* update switch interrupt to detect changes from current buttons */
  P2IES |= (p2val & SWITCHES);/* if switch up, sense down */
  P2IES &= (p2val | ~SWITCHES);/* if switch down, sense up */
  return p2val;
}


void
switch_init()/* setup switch */
{
  P2REN |= SWITCHES;/* enables resistors for switches */
  P2IE |= SWITCHES;/* enable interrupts from switches */
  P2OUT |= SWITCHES;/* pull-ups for switches */
  P2DIR &= ~SWITCHES;/* set switches' bits for input */
  switch_update_interrupt_sense();
}


void
switch_interrupt_handler()
{
  char p2val = switch_update_interrupt_sense();
  switches = ~p2val & SWITCHES;
  check_hits();            // Check for any hits after a switch press
}

// Watchdog timer interrupt handler
void wdt_c_handler() {
  static int spawnTimer = 0;

  spawnTimer++;
  if (spawnTimer >= 100) { // Spawn a new note every 1 second
    spawn_note();
    spawnTimer = 0;
  }

  update_note();  // Move the note down
  check_hits();   // Check for hits
  redrawScreen = 1; // Trigger screen redraw
}


// Main function
void main() {
  configureClocks();
  lcd_init();
  switch_init();
  init_note();

  enableWDTInterrupts(); // Enable periodic interrupts
  or_sr(0x8);            // Enable global interrupts (GIE)

  clearScreen(COLOR_BLACK); // Start with a black screen
  while (1) {
    if (redrawScreen) {
      redrawScreen = 0;
      update_shape(); // Update screen
    }
    or_sr(0x10); // Enter low-power mode
  }
}


void
__interrupt_vec(PORT2_VECTOR) Port_2() {
  if (P2IFG & SWITCHES) {
    P2IFG &= ~SWITCHES;      // Clear interrupt flags
    switch_interrupt_handler();
  }
}

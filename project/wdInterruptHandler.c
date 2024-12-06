#include <msp430.h>
#include "buzzer.h"

// Port 2 interrupt handler
void __interrupt_vec(PORT2_VECTOR) Port_2() {
  if (P2IFG & SWITCHES) {      // Check if a switch caused the interrupt
    P2IFG &= ~SWITCHES;      // Clear interrupt flags
    switch_interrupt_handler();
  }
}

#include "button.h"

int buttonIsShortPressed(unsigned long long buttonHist) {
    if ((buttonHist & 0b11) == 0b10) {                // 2 cycles
        return true;
    } else {
        return false;
    }
}

int buttonIsHeld(unsigned long long buttonHist) {
    if ((buttonHist & 0xFFFFFFFFF) == 0xFFFFFFFFF) {    // 64 cycles
        return true;
    } else {
        return false;
    }
}

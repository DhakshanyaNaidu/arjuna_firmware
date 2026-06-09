// ============================================================
// ARJUNA — keypad_driver.h  v4.0
// v4.0: Added DEST_CHAR for destination field (alphanumeric direct)
// ============================================================
#pragma once
#include <Arduino.h>
#include <Keypad.h>
#include "config.h"

enum class KPMode : uint8_t { MENU, COMPOSE, DEST };

enum class KeyEvt : uint8_t {
    NONE = 0,
    NAV_UP, NAV_DOWN, NAV_SELECT, NAV_BACK,
    OPEN_INBOX, OPEN_SENT, OPEN_OUTBOX, OPEN_NODES,
    NEW_MSG, EMERGENCY,
    CHAR,       // T9 character (compose body)
    DEST_CHAR,  // direct uppercase char for destination field
    SPACE, BACKSPACE, SEND, COMPOSE_BACK
};

struct KeyResult {
    KeyEvt evt;
    char   ch;
    bool   held;
};

namespace KPad {
    void      init();
    void      setMode(KPMode m);
    KPMode    getMode();
    KeyResult poll();
}

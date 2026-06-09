// ============================================================
// ARJUNA — keypad.cpp  v4.2
//
// FIXES in v4.2:
//   1. HOLD detection fixed: Keypad::getKey() only fires on PRESSED,
//      not HOLD. Must use getKeys() + scan key list for HOLD state.
//      0-long-hold now correctly triggers COMPOSE_BACK.
//   2. Dest mode: every key press appends one char to dest field.
//      Cycling same key within 800ms REPLACES the last char (T9).
//      Different key after 800ms appends new char — dest accepts all 4.
//   3. Double-char fix from v4.1 preserved.
//
// KEY LAYOUT:
//   MENU:    1=INBOX 2=UP   3=SENT  4=UP   5=NODES 6=DOWN
//            7=NEWMSG 8=DOWN 9=OUT  *=EMRG  0=BACK  #=OK
//   DEST:    1-9=T9 chars  *=ignore  0=backspace  0long=cancel  #=confirm
//   COMPOSE: 1-9=T9 alpha  *=space   0=backspace  0long=back    #=send
//            *long=EMERGENCY
// ============================================================
#include "keypad_driver.h"
#include <string.h>

static const char* T9_MAP[9] = {
    ".,!?1",  // 1
    "ABC2",   // 2
    "DEF3",   // 3
    "GHI4",   // 4
    "JKL5",   // 5
    "MNO6",   // 6
    "PQRS7",  // 7
    "TUV8",   // 8
    "WXYZ9",  // 9
};

static char KEYMAP[KP_ROWS][KP_COLS] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};

static uint8_t  rowPins[KP_ROWS];
static uint8_t  colPins[KP_COLS];
static Keypad*  kp     = nullptr;
static KPMode   kpMode = KPMode::MENU;

static char     t9Key  = '\0';
static uint8_t  t9Cnt  = 0;
static uint32_t t9Ms   = 0;
static const uint32_t T9_TIMEOUT = 800;

// Track hold state manually — Keypad::getKey() misses HOLD events
static char     holdKey    = '\0';
static uint32_t holdStart  = 0;
static bool     holdFired  = false;
static const uint32_t HOLD_MS = 700;

static char t9Char(char key, uint8_t tap) {
    if (key < '1' || key > '9') return key;
    const char* m = T9_MAP[key - '1'];
    return m[tap % (uint8_t)strlen(m)];
}

static KeyResult t9Emit(bool forDest) {
    KeyResult r = {KeyEvt::NONE, '\0', false};
    if (t9Key != '\0') {
        r.evt  = forDest ? KeyEvt::DEST_CHAR : KeyEvt::CHAR;
        r.ch   = t9Char(t9Key, t9Cnt - 1);
        r.held = (t9Cnt > 1);
        t9Key  = '\0';
        t9Cnt  = 0;
    }
    return r;
}

void KPad::init() {
    for (int i = 0; i < KP_ROWS; i++) rowPins[i] = KP_ROW_PINS[i];
    for (int i = 0; i < KP_COLS; i++) colPins[i] = KP_COL_PINS[i];
    kp = new Keypad(makeKeymap(KEYMAP), rowPins, colPins, KP_ROWS, KP_COLS);
    kp->setDebounceTime(50);
    kp->setHoldTime(HOLD_MS);
    kpMode    = KPMode::MENU;
    holdKey   = '\0';
    holdFired = false;
    Serial.println("[KPAD] v4.2 init OK");
}

void KPad::setMode(KPMode m) {
    t9Key     = '\0'; t9Cnt = 0;
    holdKey   = '\0'; holdFired = false;
    kpMode    = m;
}

KPMode KPad::getMode() { return kpMode; }

KeyResult KPad::poll() {
    KeyResult r = {KeyEvt::NONE, '\0', false};
    if (!kp) return r;

    bool isText = (kpMode == KPMode::COMPOSE || kpMode == KPMode::DEST);
    bool isDest = (kpMode == KPMode::DEST);

    // ── T9 timeout — emit pending char ───────────────────────
    if (isText && t9Key != '\0' && (millis() - t9Ms) >= T9_TIMEOUT) {
        return t9Emit(isDest);
    }

    // ── Manual hold detection via getKeys() ───────────────────
    // getKeys() updates all key slots; lets us see HOLD state.
    // We check this BEFORE getKey() so we can fire COMPOSE_BACK
    // on 0-hold and EMERGENCY on *-hold without losing the event.
    if (kpMode == KPMode::COMPOSE || kpMode == KPMode::DEST) {
        kp->getKeys(); // update key states
        for (int i = 0; i < LIST_MAX; i++) {
            Key k = kp->key[i];
            if (k.kchar == '\0') continue;
            if (k.kstate == PRESSED) {
                holdKey   = k.kchar;
                holdStart = millis();
                holdFired = false;
            }
            if (k.kstate == RELEASED || k.kstate == IDLE) {
                if (k.kchar == holdKey) { holdKey = '\0'; holdFired = false; }
            }
            if (k.kstate == HOLD && !holdFired) {
                holdFired = true;
                char hk = k.kchar;
                t9Key = '\0'; t9Cnt = 0; // discard any pending cycle on hold
                if (hk == '0') {
                    r.evt = KeyEvt::COMPOSE_BACK;
                    return r;
                }
                if (hk == '*' && kpMode == KPMode::COMPOSE) {
                    r.evt = KeyEvt::EMERGENCY;
                    return r;
                }
            }
        }
    }

    char raw = kp->getKey();
    if (raw == NO_KEY) return r;

    // If this is the hold key that already fired, skip short-press handling
    if (holdFired && raw == holdKey) return r;

    // ── MENU ─────────────────────────────────────────────────
    if (kpMode == KPMode::MENU) {
        switch (raw) {
            case '1': r.evt = KeyEvt::OPEN_INBOX;  break;
            case '2': r.evt = KeyEvt::NAV_UP;      break;
            case '3': r.evt = KeyEvt::OPEN_SENT;   break;
            case '4': r.evt = KeyEvt::NAV_UP;      break;
            case '5': r.evt = KeyEvt::OPEN_NODES;  break;
            case '6': r.evt = KeyEvt::NAV_DOWN;    break;
            case '7': r.evt = KeyEvt::NEW_MSG;     break;
            case '8': r.evt = KeyEvt::NAV_DOWN;    break;
            case '9': r.evt = KeyEvt::OPEN_OUTBOX; break;
            case '*': r.evt = KeyEvt::EMERGENCY;   break;
            case '0': r.evt = KeyEvt::NAV_BACK;    break;
            case '#': r.evt = KeyEvt::NAV_SELECT;  break;
        }
        return r;
    }

    // ── DEST ─────────────────────────────────────────────────
    if (kpMode == KPMode::DEST) {
        if (raw == '#') {
            if (t9Key != '\0') return t9Emit(true);
            r.evt = KeyEvt::SEND; return r;
        }
        if (raw == '0') {
            // Short press = backspace (hold already handled above)
            t9Key = '\0'; t9Cnt = 0;
            r.evt = KeyEvt::BACKSPACE; return r;
        }
        if (raw >= '1' && raw <= '9') {
            uint32_t now = millis();
            if (raw == t9Key && (now - t9Ms) < T9_TIMEOUT) {
                // Same key within timeout: cycle in place
                t9Cnt++; t9Ms = now;
                r.evt  = KeyEvt::DEST_CHAR;
                r.ch   = t9Char(raw, t9Cnt - 1);
                r.held = true;
                return r;
            }
            // Different key (or timeout expired): emit old, queue new
            if (t9Key != '\0') {
                KeyResult fl = t9Emit(true);
                t9Key = raw; t9Cnt = 1; t9Ms = now;
                return fl;
            }
            // Nothing pending: emit first char immediately
            t9Key = raw; t9Cnt = 1; t9Ms = now;
            r.evt  = KeyEvt::DEST_CHAR;
            r.ch   = t9Char(raw, 0);
            r.held = false;
            return r;
        }
        return r;
    }

    // ── COMPOSE ──────────────────────────────────────────────
    uint32_t now = millis();

    if (raw == '#') {
        if (t9Key != '\0') return t9Emit(false);
        r.evt = KeyEvt::SEND; return r;
    }
    if (raw == '*') {
        // Short press = space (hold = EMERGENCY, already handled above)
        if (t9Key != '\0') return t9Emit(false);
        r.evt = KeyEvt::SPACE; return r;
    }
    if (raw == '0') {
        // Short press = backspace/cancel-cycle (hold already handled above)
        if (t9Key != '\0') {
            t9Key = '\0'; t9Cnt = 0;
            return r; // NONE: cancelled in-progress cycle
        }
        r.evt = KeyEvt::BACKSPACE; return r;
    }
    if (raw >= '1' && raw <= '9') {
        if (raw == t9Key && (now - t9Ms) < T9_TIMEOUT) {
            t9Cnt++; t9Ms = now;
            r.evt  = KeyEvt::CHAR;
            r.ch   = t9Char(raw, t9Cnt - 1);
            r.held = true;
            return r;
        }
        if (t9Key != '\0') {
            KeyResult fl = t9Emit(false);
            t9Key = raw; t9Cnt = 1; t9Ms = now;
            return fl;
        }
        t9Key = raw; t9Cnt = 1; t9Ms = now;
        r.evt  = KeyEvt::CHAR;
        r.ch   = t9Char(raw, 0);
        r.held = false;
        return r;
    }
    return r;
}

// ============================================================
// ARJUNA — main.cpp  v4.0
// Target : Heltec WiFi LoRa 32 V3 (ESP32-S3FN8, SX1262)
// Baud   : 115200
//
// v4.0 fixes:
//   1. MSG_MAX_LEN = 40 chars (was effectively 1 due to T9 cycling bug)
//      T9 now commits previous char IMMEDIATELY on key change
//   2. Display: all screens fit 128x64 — no overflow
//   3. T9: simplified — just type, characters commit on key change
//      No complex shortcuts; # = send, * = space, 0 = backspace
//   4. SENT box: top = enter receiver ID (DEST mode), bottom = type msg
//      OUTBOX = sent messages, INBOX = received messages
//      (see Screen::COMPOSE flow: enterDst → body → send)
//   5. INBOX shows last 3 received messages (MSG_HISTORY=3)
//   6. OUTBOX (=SENT folder) shows last 3 sent messages
//   7. Time/Date: DS3231 RTC module for persistent accurate time
//      Falls back to NTP if RTC not set, then to hardcoded fallback
//      Time does NOT reset on power cycle (RTC battery backed)
//   8. Battery: non-blocking % shown in status bar always
//      Uses proper ADC_ATTEN_DB_2_5 + ADC_CTRL, Meshtastic formula
//      No popup blocking the screen — "LOW" blinks in status bar
//   9. Flash/Upload fixed: ARDUINO_USB_CDC_ON_BOOT=0 for CP2102
//      platformio.ini corrected for reliable flashing
//
// Boot sequence:
//   1. Serial 115200
//   2. GPIO36 (Vext) LOW  — power OLED + LoRa RF boost
//   3. GPIO37 (ADC_CTRL) LOW — enable battery ADC divider
//   4. NVS + Device ID
//   5. Buzzer init
//   6. OLED init + splash
//   7. RTC init — read time; if not set, try NTP, then fallback
//   8. Keypad init
//   9. RadioLib SX1262 init
//  10. Battery ADC configure
//  11. Main loop
// ============================================================
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <string.h>

// DS3231 RTC — lightweight, no WiFi needed, battery-backed
// RTClib from Adafruit: https://github.com/adafruit/RTClib
#include <RTClib.h>

#include "config.h"
#include "crypto.h"
#include "mesh.h"
#include "display.h"
#include "storage.h"
#include "keypad_driver.h"
#include "buzzer.h"
#include "emergency.h"

// ── RTC instance ──────────────────────────────────────────────
// DS3231 connected on GPIO41(SDA)/GPIO42(SCL) — general I2C bus
// NOT on GPIO17/18 which are OLED-only
static RTC_DS3231 rtc;
static bool rtcPresent = false;

// ── App state ────────────────────────────────────────────────
static Screen  curScreen = Screen::SPLASH;
static Folder  curFolder = Folder::INBOX;
static uint8_t menuSel   = 0;
static ArjMsg  viewMsg;
static bool    hasView   = false;

// Compose
// v4.0: MSG_MAX_LEN=40 — full message in one buffer
static char    compBuf[MSG_MAX_LEN + 1] = {0};
static uint8_t compLen    = 0;
static char    compDst[5] = {0};
static uint8_t compDstLen = 0;
static bool    enterDst   = true;
static bool    t9Cycling  = false; // true = current key is being cycled

// Crypto display
static char  cryptoHex[18] = "????????????????";
static bool  cryptoEnc     = true;

// Neighbour cache
static Neighbour nbTable[MAX_NEIGHBOURS];
static uint8_t   nbCount = 0;

// Emergency
static uint8_t emergSel = 0;

// Timers
static uint32_t beaconTmr = 0;
static uint32_t nbTmr     = 0;
static uint32_t dispTmr   = 0;
static uint32_t batTmr    = 0;

// Battery (cached, updated every 30s — no need to read every frame)
static uint8_t batPct = 100;

// ── Battery reading ───────────────────────────────────────────
// Meshtastic formula: VBAT = ADC_raw/4095 * 3.3 * 4.9 * 1.045
// ADC_2_5db keeps VADC in correct range for this divider (ESP32 Arduino core v6.x)
// GPIO37 (ADC_CTRL) must be LOW (done once in setup)
static uint8_t readBattery() {
    uint32_t raw  = analogRead(VBAT_PIN);
    float    vadc = (raw / 4095.0f) * 3.3f;
    float    vbat = vadc * VBAT_MULT;  // VBAT_MULT = 4.9 * 1.045 = 5.1205

    // OCV table interpolation (from Meshtastic power.h)
    // 4190mV=100%, 3100mV=0%
    float vbat_mv = vbat * 1000.0f;
    int pct;
    if      (vbat_mv >= 4190) pct = 100;
    else if (vbat_mv >= 4050) pct = 90 + (int)((vbat_mv - 4050) / (4190 - 4050) * 10);
    else if (vbat_mv >= 3990) pct = 80 + (int)((vbat_mv - 3990) / (4050 - 3990) * 10);
    else if (vbat_mv >= 3890) pct = 70 + (int)((vbat_mv - 3890) / (3990 - 3890) * 10);
    else if (vbat_mv >= 3800) pct = 60 + (int)((vbat_mv - 3800) / (3890 - 3800) * 10);
    else if (vbat_mv >= 3720) pct = 50 + (int)((vbat_mv - 3720) / (3800 - 3720) * 10);
    else if (vbat_mv >= 3630) pct = 40 + (int)((vbat_mv - 3630) / (3720 - 3630) * 10);
    else if (vbat_mv >= 3530) pct = 30 + (int)((vbat_mv - 3530) / (3630 - 3530) * 10);
    else if (vbat_mv >= 3420) pct = 20 + (int)((vbat_mv - 3420) / (3530 - 3420) * 10);
    else if (vbat_mv >= 3300) pct = 10 + (int)((vbat_mv - 3300) / (3420 - 3300) * 10);
    else if (vbat_mv >= 3100) pct =  0 + (int)((vbat_mv - 3100) / (3300 - 3100) * 10);
    else                      pct = 0;

    return (uint8_t)constrain(pct, 0, 100);
}

// ── RTC + Time init ───────────────────────────────────────────
// Strategy:
//   1. Try DS3231 on general I2C bus (GPIO41/42)
//   2. If RTC has valid time (after year 2024), use it
//   3. Otherwise try NTP (WiFi on ≤5s, then off)
//   4. If NTP fails, use hardcoded fallback
//   5. Always set system time from the best source available
//   6. If RTC was present but unset, write the new time to it
//
// Time then persists across power cycles via DS3231 coin cell.
static void initTime() {
    // Try DS3231 on the general I2C bus
    // Note: Wire2 not needed — RTClib uses Wire by default
    // We use Wire1 for DS3231 on GPIO41/42 (separate from OLED Wire on 17/18)
    Wire1.begin(41, 42);
    rtcPresent = rtc.begin(&Wire1);

    if (rtcPresent) {
        Serial.println("[RTC] DS3231 found");
        if (!rtc.lostPower()) {
            // RTC has been running — use its time
            DateTime now = rtc.now();
            if (now.year() >= 2024) {
                // Valid time — set system clock
                struct timeval tv = { .tv_sec = (time_t)now.unixtime(), .tv_usec = 0 };
                settimeofday(&tv, nullptr);
                // Apply IST offset (configTime sets TZ)
                configTime(IST_OFFSET_SEC, 0, "");
                Serial.printf("[RTC] Time from DS3231: %04d-%02d-%02d %02d:%02d:%02d\n",
                    now.year(), now.month(), now.day(),
                    now.hour(), now.minute(), now.second());
                return; // Done — no WiFi needed
            }
            Serial.println("[RTC] RTC has old time, resyncing...");
        } else {
            Serial.println("[RTC] RTC lost power, needs sync");
        }
    } else {
        Serial.println("[RTC] DS3231 NOT found — using NTP/fallback");
    }

    // RTC not available or unset — try NTP
    Serial.println("[NTP] Starting WiFi for time sync...");
    WiFi.mode(WIFI_STA);
    // No SSID — just use configTime to query NTP servers
    // This won't connect to anything without credentials, but we set
    // configTime here so that IF a network happens to be open/remembered,
    // it will sync. Otherwise we fall through to the hardcoded fallback.
    configTime(IST_OFFSET_SEC, 0, NTP_SERVER, NTP_FALLBACK_SERVER);

    uint32_t t0 = millis();
    struct tm ti;
    bool synced = false;
    while (!getLocalTime(&ti, 200) && (millis() - t0) < 5000) {
        delay(300);
    }
    if (getLocalTime(&ti, 100) && ti.tm_year > 120) {
        synced = true;
        Serial.println("[NTP] sync OK");
    }

    if (!synced) {
        // Hardcoded IST fallback — April 2026
        struct timeval tv = { .tv_sec = NTP_FALLBACK, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        configTime(IST_OFFSET_SEC, 0, "");
        Serial.println("[NTP] Using hardcoded fallback time");
    }

    // If RTC is present but was unset, write the synced/fallback time to it
    if (rtcPresent) {
        time_t now = time(nullptr);
        rtc.adjust(DateTime((uint32_t)now));
        Serial.println("[RTC] RTC updated with new time");
    }

    // Turn WiFi off — not needed after time sync
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    btStop();
    Serial.println("[NTP] WiFi + BT off");
}

// ── Get current unix timestamp ────────────────────────────────
// Prefer DS3231 for accuracy; fall back to system time()
static uint32_t getTime() {
    if (rtcPresent) {
        DateTime now = rtc.now();
        // rtc.now() returns UTC unixtime; add IST offset for display
        return (uint32_t)now.unixtime() + IST_OFFSET_SEC;
    }
    return (uint32_t)time(nullptr);
}

// ── Screen transition ─────────────────────────────────────────
static void goTo(Screen s) {
    curScreen = s;
    menuSel = 0;
    t9Cycling = false;

    if (s == Screen::COMPOSE) {
        KPad::setMode(enterDst ? KPMode::DEST : KPMode::COMPOSE);
    } else {
        KPad::setMode(KPMode::MENU);
    }
}

// ── Compose helpers ──────────────────────────────────────────
// v4.0: compAppend adds to the 40-char buffer freely
static void compAppend(char c) {
    if (compLen >= MSG_MAX_LEN) return;
    compBuf[compLen++] = c;
    compBuf[compLen]   = '\0';
}
static void compReplaceLast(char c) {
    if (!compLen) { compAppend(c); return; }
    compBuf[compLen - 1] = c;
}
static void compBackspace() {
    if (compLen) compBuf[--compLen] = '\0';
    t9Cycling = false;
}
static void compClear() {
    memset(compBuf, 0, sizeof(compBuf));
    compLen = 0;
    t9Cycling = false;
}

// handleChar: if 'cycling' (same key pressed again), replace last char
//             otherwise append new char
static void handleChar(char c, bool cycling) {
    if (cycling && t9Cycling && compLen) {
        compReplaceLast(c);
    } else {
        compAppend(c);
        t9Cycling = true;
    }
}

// ── Send message ─────────────────────────────────────────────
static void sendMessage() {
    t9Cycling = false;
    if (!compLen || compDstLen < 4) {
        Display::notify("ERROR", "Need dest+text", 1500);
        Buzzer::error();
        return;
    }

    // Show AES-128 encryption animation
    Display::cryptoAnim(true);

    // Get nonce hex for display
    uint8_t nonce[CTR_NONCE_SIZE], cipher[MSG_MAX_LEN];
    Crypto::encrypt(NETWORK_PSK,
                    (const uint8_t*)compBuf, compLen,
                    cipher, nonce);
    Crypto::toHex(nonce, 4, cryptoHex, sizeof(cryptoHex));

    // Show AES-128 active notice before sending
    Display::notify("AES-128 ACTIVE", cryptoHex, 900);

    // Send with ACK wait (blocks up to ACK_WAIT_MS = 2.5s)
    bool acked = Mesh::sendWithAck(compDst, compBuf);

    // Save to SENT regardless of ACK (we transmitted it)
    ArjMsg m;
    strncpy(m.from, DeviceID::id, 4); m.from[4] = '\0';
    strncpy(m.to,   compDst, 4);      m.to[4]   = '\0';
    strncpy(m.body, compBuf, MSG_MAX_LEN);
    m.ts    = getTime();
    m.rssi  = 0;
    m.valid = true;
    Storage::save(Folder::SENT, m);

    if (acked) {
        // Distance estimate from last known RSSI to destination node
        uint16_t dist = 0;
        for (uint8_t i = 0; i < nbCount; i++) {
            if (strncmp(nbTable[i].id, compDst, 4) == 0) {
                dist = Mesh::rssiToDistM(nbTable[i].rssiAvg);
                break;
            }
        }
        char ackLine[22];
        if (dist > 0) snprintf(ackLine, sizeof(ackLine), "ACK ~%4dm away", dist);
        else          snprintf(ackLine, sizeof(ackLine), "ACK CONFIRMED");
        Buzzer::ackOk();
        Display::notify("SENT+CONFIRMED", ackLine, 2000);
    } else {
        // No ACK — message may not have been received
        Buzzer::ackFail();
        Display::notify("NOT CONFIRMED", "No ACK - check dst", 2500);
    }

    compClear();
    memset(compDst, 0, sizeof(compDst));
    compDstLen = 0;
    enterDst   = true;
    goTo(Screen::HOME);
}


// ── Key handler ───────────────────────────────────────────────
static void handleKey(KeyResult kr) {
    if (kr.evt == KeyEvt::NONE) return;

    // Any non-CHAR event resets T9 cycling flag
    if (kr.evt != KeyEvt::CHAR && kr.evt != KeyEvt::DEST_CHAR) {
        t9Cycling = false;
    }

    switch (curScreen) {

    case Screen::HOME:
        switch (kr.evt) {
            case KeyEvt::OPEN_INBOX:
                curFolder = Folder::INBOX;  goTo(Screen::INBOX);   break;
            case KeyEvt::OPEN_SENT:
                curFolder = Folder::SENT;   goTo(Screen::SENT);    break;
            case KeyEvt::OPEN_OUTBOX:
                curFolder = Folder::OUTBOX; goTo(Screen::OUTBOX);  break;
            case KeyEvt::OPEN_NODES:
                goTo(Screen::NODE_LIST);    break;
            case KeyEvt::NEW_MSG:
            case KeyEvt::NAV_SELECT:
                compClear();
                memset(compDst, 0, sizeof(compDst));
                compDstLen = 0; enterDst = true;
                goTo(Screen::COMPOSE);      break;
            case KeyEvt::EMERGENCY:
                emergSel = 0; goTo(Screen::EMERGENCY); break;
            default: break;
        }
        break;

    case Screen::INBOX:
    case Screen::SENT:
    case Screen::OUTBOX:
        switch (kr.evt) {
            case KeyEvt::NAV_UP:
                if (menuSel > 0) menuSel--;
                break;
            case KeyEvt::NAV_DOWN:
                if (menuSel < MSG_HISTORY - 1) menuSel++;
                break;
            case KeyEvt::NAV_SELECT: {
                ArjMsg m;
                if (Storage::load(curFolder, menuSel, m)) {
                    viewMsg    = m;
                    hasView    = true;
                    cryptoEnc  = (curFolder != Folder::INBOX);
                    uint8_t tn[CTR_NONCE_SIZE], tc[MSG_MAX_LEN];
                    Crypto::encrypt(NETWORK_PSK,
                                    (const uint8_t*)m.body, strlen(m.body),
                                    tc, tn);
                    Crypto::toHex(tn, 4, cryptoHex, sizeof(cryptoHex));
                    goTo(Screen::MSG_VIEW);
                }
                break;
            }
            case KeyEvt::NAV_BACK:
                goTo(Screen::HOME); break;
            case KeyEvt::EMERGENCY:
                emergSel = 0; goTo(Screen::EMERGENCY); break;
            default: break;
        }
        break;

    case Screen::MSG_VIEW:
        switch (kr.evt) {
            case KeyEvt::NAV_BACK:
                goTo(curFolder == Folder::INBOX ? Screen::INBOX :
                     curFolder == Folder::SENT  ? Screen::SENT  : Screen::OUTBOX);
                break;
            case KeyEvt::NAV_SELECT:
                if (hasView) {
                    strncpy(compDst, viewMsg.from, 4); compDst[4] = '\0';
                    compDstLen = 4; compClear(); enterDst = false;
                    goTo(Screen::COMPOSE);
                }
                break;
            case KeyEvt::EMERGENCY:
                emergSel = 0; goTo(Screen::EMERGENCY); break;
            default: break;
        }
        break;

    case Screen::COMPOSE:
        if (enterDst) {
            // DEST MODE: entering the 4-char device ID
            // kr.held = true means keypad is cycling same key → replace last char
            switch (kr.evt) {
                case KeyEvt::DEST_CHAR:
                    if (compDstLen < 4) {
                        if (kr.held && compDstLen > 0) {
                            // Cycling same key: replace last dest char in place
                            compDst[compDstLen - 1] = toupper((uint8_t)kr.ch);
                            compDst[compDstLen]     = '\0';
                        } else {
                            // New key: append
                            compDst[compDstLen++] = toupper((uint8_t)kr.ch);
                            compDst[compDstLen]   = '\0';
                        }
                    }
                    // Auto-advance to body once 4 chars entered
                    if (compDstLen >= 4) {
                        enterDst  = false;
                        t9Cycling = false;
                        KPad::setMode(KPMode::COMPOSE);
                    }
                    break;
                case KeyEvt::BACKSPACE:
                    if (compDstLen > 0) {
                        compDst[--compDstLen] = '\0';
                        t9Cycling = false;
                    }
                    break;
                case KeyEvt::SEND:
                    if (compDstLen >= 4) {
                        enterDst  = false;
                        t9Cycling = false;
                        KPad::setMode(KPMode::COMPOSE);
                    }
                    break;
                case KeyEvt::COMPOSE_BACK:
                    goTo(Screen::HOME); break;
                default: break;
            }
        } else {
            // COMPOSE MODE: typing the message body
            // kr.held = true means keypad is cycling same key → replace last char
            switch (kr.evt) {
                case KeyEvt::CHAR:
                    handleChar(kr.ch, kr.held);
                    break;
                case KeyEvt::SPACE:
                    compAppend(' ');
                    t9Cycling = false;
                    break;
                case KeyEvt::BACKSPACE:
                    compBackspace(); break;
                case KeyEvt::SEND:
                    sendMessage();   break;
                case KeyEvt::COMPOSE_BACK:
                    enterDst  = true;
                    compClear();
                    t9Cycling = false;
                    KPad::setMode(KPMode::DEST);
                    break;
                case KeyEvt::EMERGENCY:
                    emergSel = 0; goTo(Screen::EMERGENCY); break;
                default: break;
            }
        }
        break;

    case Screen::EMERGENCY:
        switch (kr.evt) {
            case KeyEvt::NAV_UP:
                if (emergSel > 0) emergSel--;
                break;
            case KeyEvt::NAV_DOWN:
                if (emergSel < EMERGENCY_COUNT - 1) emergSel++;
                break;
            case KeyEvt::NAV_SELECT: {
                const char* txt = EMERGENCY_PHRASES[emergSel].text;
                bool sent = false;
                for (uint8_t i = 0; i < nbCount; i++) {
                    Mesh::send(nbTable[i].id, txt); sent = true;
                }
                if (!sent) Mesh::send("BCST", txt);
                Buzzer::sos();
                Display::notify("EMRG SENT",
                                EMERGENCY_PHRASES[emergSel].code, 1500);
                goTo(Screen::HOME);
                break;
            }
            case KeyEvt::NAV_BACK:
            case KeyEvt::EMERGENCY:
                goTo(Screen::HOME); break;
            default: break;
        }
        break;

    case Screen::NODE_LIST:
        switch (kr.evt) {
            case KeyEvt::NAV_UP:
                if (menuSel > 0) menuSel--;
                break;
            case KeyEvt::NAV_DOWN:
                if (nbCount > 0 && menuSel < nbCount - 1) menuSel++;
                break;
            case KeyEvt::NAV_SELECT:
                if (nbCount > 0 && menuSel < nbCount) {
                    strncpy(compDst, nbTable[menuSel].id, 4); compDst[4] = '\0';
                    compDstLen = 4; compClear(); enterDst = false;
                    KPad::setMode(KPMode::COMPOSE);
                    goTo(Screen::COMPOSE);
                }
                break;
            case KeyEvt::NAV_BACK:
                goTo(Screen::HOME); break;
            case KeyEvt::EMERGENCY:
                emergSel = 0; goTo(Screen::EMERGENCY); break;
            default: break;
        }
        break;

    default: break;
    }
}

// ── Draw current screen ───────────────────────────────────────
static void drawScreen() {
    uint32_t ts  = getTime();
    int8_t   rxR = Mesh::lastRxRssi();
    bool     mok = Mesh::isOk();

    switch (curScreen) {
        case Screen::HOME:
            Display::home(DeviceID::id, ts,
                          Storage::count(Folder::INBOX),
                          Storage::count(Folder::SENT),
                          rxR, batPct, mok);
            break;
        case Screen::INBOX:
        case Screen::SENT:
        case Screen::OUTBOX:
            Display::folder(curFolder, menuSel);
            break;
        case Screen::MSG_VIEW:
            if (hasView) Display::msgView(viewMsg, cryptoHex, cryptoEnc);
            break;
        case Screen::COMPOSE:
            Display::compose(compBuf, compLen, compDst, enterDst);
            break;
        case Screen::EMERGENCY:
            Display::emergencyMenu(emergSel);
            break;
        case Screen::NODE_LIST: {
            char   ids[MAX_NEIGHBOURS][5];
            int8_t rssi[MAX_NEIGHBOURS];
            char   prox[MAX_NEIGHBOURS][6];
            for (uint8_t i = 0; i < nbCount; i++) {
                strncpy(ids[i],  nbTable[i].id,   4); ids[i][4]  = '\0';
                rssi[i] = nbTable[i].rssiAvg;
                strncpy(prox[i], nbTable[i].prox, 5); prox[i][5] = '\0';
            }
            Display::nodeList(ids, rssi,
                              (const char(*)[6])prox,
                              nbCount, menuSel);
            break;
        }
        default: break;
    }
    // NOTE: no Display::lowBattery() call here — battery shown in status bar
    // "LOW" blinks in the status bar when batPct < VBAT_LOW_PCT
    // This prevents any popup from covering the screen
}

// ═════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n\n=== ARJUNA v4.1 boot ===");

    // ── STEP 1: Power Vext FIRST ─────────────────────────────
    // GPIO36 LOW = 3.3V rail for OLED + LoRa RF boost
    // MUST be done before OLED init and before any LoRa TX
    pinMode(VEXT_PIN, OUTPUT);
    digitalWrite(VEXT_PIN, VEXT_ON);
    delay(50); // rail stabilise
    Serial.println("[PWR] Vext ON (GPIO36 LOW)");

    // ── STEP 2: Battery ADC enable ───────────────────────────
    // GPIO37 LOW enables the battery voltage divider transistor
    pinMode(ADC_CTRL_PIN, OUTPUT);
    digitalWrite(ADC_CTRL_PIN, ADC_CTRL_ON);
    Serial.println("[PWR] ADC_CTRL ON (GPIO37 LOW)");

    // LED off
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // ── STEP 3: NVS + Device ID ──────────────────────────────
    Storage::init();
    DeviceID::load_or_create();

    // ── STEP 4: Buzzer ───────────────────────────────────────
    Buzzer::init();
    Buzzer::boot();

    // ── STEP 5: OLED init (Wire on GPIO17/18 = OLED bus) ────
    Display::init();
    Display::splash(DeviceID::id);

    // ── STEP 6: Time — DS3231 RTC first, then NTP fallback ──
    // Wire is OLED (GPIO17/18), Wire1 is general I2C (GPIO41/42)
    // DS3231 uses Wire1 in initTime()
    initTime();

    // ── STEP 7: Keypad ───────────────────────────────────────
    KPad::init();

    // ── STEP 8: RadioLib SX1262 ──────────────────────────────
    Mesh::init(DeviceID::id);
    if (!Mesh::isOk()) {
        Display::notify("LORA FAIL", "Check SX1262+Vext", 3000);
        Serial.println("[MESH] FAIL — check SX1262 and Vext wiring");
    }

    // ── STEP 9: Battery ADC calibration ──────────────────────
    // ESP32 Arduino core v6.x (espressif32 ^6.5): 
    //   analogSetPinAttenuation(pin, ADC_2_5db)  — pin-specific attenuation
    //   analogReadResolution(12)                 — replaces analogSetWidth()
    // ADC_2_5db keeps VADC in range for the Heltec V3 voltage divider
    // (4.2V bat → VADC ≈ 0.82V, well within ADC_2_5db max of 1.5V)
    analogReadResolution(12);                          // 12-bit, 0-4095
    analogSetPinAttenuation(VBAT_PIN, VBAT_ATTEN);    // ADC_2_5db on GPIO1

    // ── STEP 10: Initial battery read ────────────────────────
    batPct = readBattery();

    // ── STEP 11: Timers and HOME ─────────────────────────────
    beaconTmr = nbTmr = dispTmr = batTmr = millis();
    goTo(Screen::HOME);

    Serial.printf("[BOOT] Device ID: %s — ready\n", DeviceID::id);
    Serial.printf("[BOOT] Battery: %d%%\n", batPct);
    Serial.println("[BOOT] === Boot complete ===");
}

// ═════════════════════════════════════════════════════════════
// LOOP
// ═════════════════════════════════════════════════════════════
void loop() {
    // 1. Keypad
    KeyResult kr = KPad::poll();
    handleKey(kr);

    // 2. Mesh RX
    {
        ArjMsg rxMsg;
        int8_t rxR = -120;
        if (Mesh::poll(rxMsg, rxR)) {
            rxMsg.rssi = rxR;
            rxMsg.ts   = getTime();
            Storage::save(Folder::INBOX, rxMsg);

            // Show AES-128 decryption animation
            Display::cryptoAnim(false);

            // Line 1: who it's from
            char hdr[22];
            snprintf(hdr, sizeof(hdr), "FROM: %-4s AES128", rxMsg.from);

            // Line 2: distance estimate from received RSSI
            uint16_t dist = Mesh::rssiToDistM(rxR);
            char distLine[22];
            if (dist > 0) snprintf(distLine, sizeof(distLine), "DECRYPTED ~%4dm", dist);
            else          snprintf(distLine, sizeof(distLine), "AES-128 DECRYPTED");

            Display::notify(hdr, distLine, 2200);
        }
    }

    // 3. Beacon every 15s
    if ((millis() - beaconTmr) >= BEACON_MS) {
        Mesh::sendBeacon();
        beaconTmr = millis();
    }

    // 4. Neighbour expiry every 5s
    if ((millis() - nbTmr) >= 5000UL) {
        nbCount = Mesh::getNeighbours(nbTable);
        nbTmr   = millis();
    }

    // 5. Battery update every 30s (non-blocking, cached in batPct)
    if ((millis() - batTmr) >= 30000UL) {
        batPct = readBattery();
        batTmr = millis();
        Serial.printf("[BAT] %d%%\n", batPct);
    }

    // 6. Display refresh (20fps menu / 15fps compose)
    uint32_t interval = (curScreen == Screen::COMPOSE) ? COMPOSE_MS : DISP_MS;
    if ((millis() - dispTmr) >= interval) {
        drawScreen();
        dispTmr = millis();
    }
}

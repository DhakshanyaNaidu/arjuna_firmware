// ============================================================
// ARJUNA — buzzer.h  v4.1
// GPIO2 — ADC1_CH1, Touch2, no pin conflicts
//
// v4.1 sounds (all clearly distinct):
//   boot()    — single short 880Hz beep (device power on)
//   txAlert() — two ascending tones: 800Hz → 1400Hz (TX = going up)
//   rxAlert() — two descending tones: 1200Hz → 600Hz (RX = coming down)
//   ackOk()   — triple quick high beep (ACK received = success)
//   ackFail() — long low buzz (no ACK = failed)
//   sos()     — Morse ... --- ...
//   error()   — low buzz
// ============================================================
#pragma once
#include <Arduino.h>
#include "config.h"

namespace Buzzer {
    inline void init() {
        ledcSetup(BUZZER_CH, TONE_TX_HZ, 8);
        ledcAttachPin(BUZZER_PIN, BUZZER_CH);
        ledcWrite(BUZZER_CH, 0);
    }

    inline void _tone(uint32_t freq, uint32_t ms) {
        ledcWriteTone(BUZZER_CH, freq);
        ledcWrite(BUZZER_CH, 128);
        delay(ms);
        ledcWrite(BUZZER_CH, 0);
    }

    // Single short beep — boot only
    inline void boot() {
        _tone(880, 80);
    }

    // TX: two ascending tones (800 → 1400) — "sending up"
    inline void txAlert() {
        _tone(800, 80);
        delay(40);
        _tone(1400, 100);
    }

    // RX: two descending tones (1200 → 600) — "arriving down"
    inline void rxAlert() {
        _tone(1200, 80);
        delay(40);
        _tone(600, 120);
    }

    // ACK received OK — triple quick high beep
    inline void ackOk() {
        for (int i = 0; i < 3; i++) { _tone(1600, 50); delay(40); }
    }

    // ACK failed (message not delivered) — one long low buzz
    inline void ackFail() {
        _tone(300, 400);
    }

    // SOS Morse: ... --- ...
    inline void sos() {
        for(int i=0;i<3;i++){_tone(880,80);delay(80);}
        delay(180);
        for(int i=0;i<3;i++){_tone(880,240);delay(80);}
        delay(180);
        for(int i=0;i<3;i++){_tone(880,80);delay(80);}
    }

    inline void error() { _tone(350, 200); }
}

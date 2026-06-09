// ============================================================
// ARJUNA — display.h  v4.0
// v4.0 changes:
//   - Battery shown as percentage from RTClib/ropg ADC method
//     (no more blocking LOW BATTERY popup - shown in status bar only)
//   - lowBattery() removed — status bar handles it non-blocking
//   - compose() updated: buf is full 40-char, display wraps properly
//   - All compose display shows full 40-char buffer in 2 rows
// ============================================================
#pragma once
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config.h"
#include "storage.h"
#include "emergency.h"  // included here so display.cpp gets the type

enum class Screen : uint8_t {
    SPLASH, HOME, INBOX, SENT, OUTBOX,
    MSG_VIEW, COMPOSE, EMERGENCY, NODE_LIST
};

namespace Display {
    void init();
    void splash(const char* devId);
    void home(const char* devId, uint32_t ts,
              uint8_t inboxCnt, uint8_t sentCnt,
              int8_t rssi, uint8_t bat, bool meshOk);
    void folder(Folder f, uint8_t selected);
    void msgView(const ArjMsg& msg, const char* cryptoHex, bool encrypting);
    void compose(const char* buf, uint8_t curPos,
                 const char* dst, bool enterDst);
    void emergencyMenu(uint8_t selected);
    void nodeList(const char ids[][5], const int8_t rssi[],
                  const char prox[][6], uint8_t count, uint8_t sel);
    void notify(const char* l1, const char* l2, uint16_t ms);
    void cryptoAnim(bool encrypting);
    // NOTE: lowBattery() removed — battery % shown in status bar always
    //       This prevents the blocking popup from covering the screen
}

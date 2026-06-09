// ============================================================
// ARJUNA — config.h  v3.0
// ALL PINS VERIFIED AGAINST THREE SOURCES:
//   1. Heltec official pins_arduino.h (WiFi_Kit_series-master)
//   2. Meshtastic firmware variant.h (heltec_v3)
//   3. Official datasheet HTIT-WB32LA_V3_Rev1_1.pdf
//
// v3.0 critical corrections:
//   OLED SDA = GPIO17 (SDA_OLED) — v2.1 wrongly changed to 41
//   OLED SCL = GPIO18 (SCL_OLED) — v2.1 wrongly changed to 42
//   Vext GPIO36 LOW  ← MISSING in ALL prior versions (OLED no power!)
//   ADC_CTRL GPIO37  ← MISSING in ALL prior versions
//   DIO2 as RF switch ← MISSING (SX1262 TX path broken)
//   DIO3 TCXO 1.8V   ← MISSING (LoRa frequency unstable)
//   LoRaMesher → RadioLib direct (eliminates all mesh.cpp errors)
// ============================================================
#pragma once
#include <Arduino.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_mac.h>

#define ARJUNA_VERSION    "4.0.0"

// ── LoRa SX1262 SPI pins ──────────────────────────────────────
// Source: pins_arduino.h SS=8,MOSI=10,MISO=11,SCK=9
//         variant.h LORA_CS=8,LORA_DIO1=14,LORA_RESET=12,LORA_DIO2=13
#define LORA_CS           8
#define LORA_DIO1         14
#define LORA_RST          12
#define LORA_BUSY         13
#define LORA_MOSI         10
#define LORA_MISO         11
#define LORA_SCK          9

// SX1262 critical settings (from Meshtastic variant.h)
#define LORA_DIO2_RF_SW   true
#define LORA_TCXO_V       1.8f       // DIO3 TCXO voltage
#define LORA_FREQ         866.0f     // MHz — IN868 band
#define LORA_BW           125.0f     // kHz
#define LORA_SF           9
#define LORA_CR           5
#define LORA_SYNC_WORD    0x12
#define LORA_TX_DBM       14
#define LORA_PREAMBLE     8
#define LORA_OCP_MA       140.0f     // SX1262 current limit

// ── OLED SSD1306 ──────────────────────────────────────────────
// Source: pins_arduino.h SDA_OLED=17, SCL_OLED=18, RST_OLED=21
// GPIO 41/42 = external sensor I2C (SDA/SCL), NOT the OLED bus
#define OLED_SDA          17
#define OLED_SCL          18
#define OLED_RST          21
#define OLED_ADDR         0x3C
#define SCREEN_W          128
#define SCREEN_H          64

// ── Power Control ─────────────────────────────────────────────
// GPIO36 = Vext, active LOW (from pins_arduino.h + Meshtastic variant.h)
// Powers both the OLED display and the LoRa RF boost amplifier.
// Must be set LOW before any OLED or LoRa operation.
#define VEXT_PIN          36
#define VEXT_ON           LOW
#define VEXT_OFF          HIGH

// GPIO37 = ADC_CTRL (from Meshtastic variant.h, ADC_CTRL_ENABLED=LOW)
// Controls the battery voltage divider circuit enable transistor.
#define ADC_CTRL_PIN      37
#define ADC_CTRL_ON       LOW

// Battery ADC: GPIO1, divider 100k/(100k+390k) = /4.9
// Meshtastic multiplier: 4.9 * 1.045 (empirical calibration)
// Use ADC_ATTEN_DB_2_5 (divider keeps voltage in low range)
#define VBAT_PIN          1
#define VBAT_ATTEN        ADC_2_5db   // ESP32 Arduino core v6.x: ADC_2_5db (not ADC_ATTEN_DB_2_5)
#define VBAT_MULT         (4.9f * 1.045f)
#define VBAT_LOW_PCT      15

#define LED_PIN           35          // LED_BUILTIN per pins_arduino.h

// ── 4x3 Keypad ───────────────────────────────────────────────
// Verified safe GPIOs (no conflict with LoRa, OLED, UART, Vext):
// Rows (OUTPUT): GPIO 3,4,5,6 — ADC1/Touch, fully bidirectional
// Cols (INPUT_PULLUP): GPIO 33,34,38 — FSPI aux, usable as GPIO
// Buzzer: GPIO 2 — ADC1_CH1, Touch2, no conflicts
#define KP_ROWS           4
#define KP_COLS           3
const uint8_t KP_ROW_PINS[KP_ROWS] = {3, 4, 5, 6};
const uint8_t KP_COL_PINS[KP_COLS] = {33, 34, 38};

#define BUZZER_PIN        2
#define BUZZER_CH         0
#define TONE_TX_HZ        1200
#define TONE_RX_HZ        880
#define TONE_MS           120

// ── Crypto ───────────────────────────────────────────────────
#define AES_KEY_SIZE      16
#define CTR_NONCE_SIZE    8
#define MAC_SIZE          4
// *** CHANGE BEFORE DEPLOYMENT - same on every node in mesh ***
// AES-128: 16 bytes, lighter than AES-256, adequate for field comms
static const uint8_t NETWORK_PSK[AES_KEY_SIZE] = {
    0xA3,0x7F,0x2B,0xE1,0x94,0xC0,0xD8,0x5A,
    0x11,0x3E,0x6D,0xF2,0x88,0x47,0xBC,0x99
};

// ── Storage / Mesh ───────────────────────────────────────────
#define MSG_MAX_LEN       40   // v4.0: 40 chars per message (was 120 but T9 was only sending 1)
#define MSG_HISTORY       3
#define NVS_NS            "arjuna"
#define MESH_TTL          5
#define BEACON_MS         15000UL
#define MAX_NEIGHBOURS    8
#define SEEN_CACHE        16
#define RX_QUEUE          4

// ── Proximity thresholds ─────────────────────────────────────
#define PROX_NEAR_DBM     (-70)
#define PROX_MID_DBM      (-90)
#define PROX_FAR_DBM      (-110)
#define RSSI_AVG_N        4

// ── Time ─────────────────────────────────────────────────────
#define IST_OFFSET_SEC       19800
#define NTP_SERVER           "pool.ntp.org"
#define NTP_FALLBACK_SERVER  "time.google.com"
#define NTP_FALLBACK         1743523800L  // Apr 2026 IST approx

// ── UI timing ────────────────────────────────────────────────
#define SPLASH_MS         2000
#define CURSOR_MS         500
#define DISP_MS           50
#define COMPOSE_MS        67

// ── Device ID ────────────────────────────────────────────────
namespace DeviceID {
    static const char ALPHA[] = "0123456789ABCDEFGHJKLMNPQRSTUVWX";
    static char id[6] = {0};

    inline void generate(const uint8_t* mac) {
        uint32_t h = 2166136261UL;
        for (int i = 0; i < 6; i++) { h ^= mac[i]; h *= 16777619UL; }
        id[0] = ALPHA[(h>>15)&0x1F]; id[1] = ALPHA[(h>>10)&0x1F];
        id[2] = ALPHA[(h>> 5)&0x1F]; id[3] = ALPHA[(h>> 0)&0x1F];
        id[4] = '\0';
    }

    inline void load_or_create() {
        nvs_handle_t h;
        if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
        size_t sz = sizeof(id);
        if (nvs_get_str(h, "dev_id", id, &sz) != ESP_OK || !id[0]) {
            uint8_t mac[6]; esp_read_mac(mac, ESP_MAC_WIFI_STA);
            generate(mac);
            nvs_set_str(h, "dev_id", id); nvs_commit(h);
            Serial.printf("[ID] NEW: %s\n", id);
        } else Serial.printf("[ID] Loaded: %s\n", id);
        nvs_close(h);
    }
}

// ── Shared types ─────────────────────────────────────────────
enum class Folder : uint8_t { INBOX=0, SENT=1, OUTBOX=2 };

struct ArjMsg {
    char     from[5];
    char     to[5];
    char     body[MSG_MAX_LEN];
    uint32_t ts;
    int8_t   rssi;
    bool     valid;
};

struct Neighbour {
    char     id[5];
    int8_t   rssiAvg;
    int8_t   hist[RSSI_AVG_N];
    uint8_t  histIdx;
    uint32_t lastSeen;
    char     prox[6];  // "NEAR" "MID " "FAR " "DIST"
};

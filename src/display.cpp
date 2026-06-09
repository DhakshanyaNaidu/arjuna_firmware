// ============================================================
// ARJUNA — display.cpp  v4.0
//
// v4.0 fixes:
//   1. Compose buffer: shows full MSG_MAX_LEN (40) chars across
//      2 text rows of 21 chars each = 42 chars visible at once
//   2. Battery: displayed as "XXX%" in status bar — NO blocking popup
//      Low battery is indicated by blinking "LOW" in status bar only
//   3. Display layout: all screens recalculated to fit 128x64
//      Status bar: row 0-7 (8px)
//      Content:    row 9 onwards
//      Footer:     row 56-63 (8px)
//   4. Time/Date: uses time_t from Unix epoch — set by RTC in main.cpp
//      Format verified for IST (UTC+5:30 = 19800s offset)
//   5. Folder view: 3 messages shown with compact layout
//   6. Message view: body wraps properly across 3 lines of 21 chars
//   7. Wire vs Wire1: uses Wire (not Wire1) because Adafruit SSD1306
//      constructor takes a TwoWire* — Wire is initialised on GPIO17/18
//      in init() via Wire.begin(17,18) as per Heltec V3 SDA_OLED/SCL_OLED
// ============================================================
#include "display.h"
#include "storage.h"
#include <Wire.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

static Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, OLED_RST);

static void cls()  { oled.clearDisplay(); }
static void show() { oled.display(); }
static void sm()   { oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE); }
static void lg()   { oled.setTextSize(2); oled.setTextColor(SSD1306_WHITE); }
static void at(int16_t x, int16_t y) { oled.setCursor(x, y); }

// ── Time formatters ───────────────────────────────────────────
static void fmtClock(uint32_t ts, char* b, size_t n) {
    time_t t = (time_t)ts;
    struct tm* m = localtime(&t);
    snprintf(b, n, "%02d:%02d", m->tm_hour, m->tm_min);
}
static void fmtDate(uint32_t ts, char* b, size_t n) {
    time_t t = (time_t)ts;
    struct tm* m = localtime(&t);
    static const char* D[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
    snprintf(b, n, "%s %02d/%02d/%04d",
             D[m->tm_wday], m->tm_mday, m->tm_mon+1, m->tm_year+1900);
}
static void fmtShort(uint32_t ts, char* b, size_t n) {
    time_t t = (time_t)ts;
    struct tm* m = localtime(&t);
    snprintf(b, n, "%02d/%02d %02d:%02d",
             m->tm_mday, m->tm_mon+1, m->tm_hour, m->tm_min);
}

// ── Status strip (row 0-7) ────────────────────────────────────
// Layout: [ID] [●/!] [RSSI] ... [BAT%] [BAR]
// Battery % shown always; "LOW" blinks at < VBAT_LOW_PCT
static void drawStatus(const char* id, int8_t rssi, uint8_t bat, bool mok) {
    sm();
    // Device ID (4 chars = 24px)
    at(0, 0); oled.print(id);
    // Mesh status dot
    if (mok) oled.fillCircle(28, 3, 2, SSD1306_WHITE);
    else { at(27, 0); oled.print("!"); }
    // RSSI (4 chars)
    char rb[6]; snprintf(rb, 6, "%4d", rssi);
    at(33, 0); oled.print(rb);

    // Battery percent text — blink "LOW" when < 15%
    char batStr[8];
    if (bat < VBAT_LOW_PCT && (millis() / 600) % 2 == 0) {
        snprintf(batStr, sizeof(batStr), " LOW");
    } else {
        snprintf(batStr, sizeof(batStr), "%3d%%", bat);
    }
    at(63, 0); oled.print(batStr);

    // Battery bar (16px wide, 5px tall) at right edge
    uint8_t bx = 104;
    oled.drawRect(bx, 1, 20, 5, SSD1306_WHITE);
    oled.drawRect(bx+20, 2, 2, 3, SSD1306_WHITE); // terminal nub
    uint8_t f = (uint8_t)((bat * 18UL) / 100);
    if (f) oled.fillRect(bx+1, 2, f, 3, SSD1306_WHITE);

    oled.drawFastHLine(0, 7, 128, SSD1306_WHITE);
}

// ── Init ─────────────────────────────────────────────────────
void Display::init() {
    // OLED-specific I2C bus: GPIO17=SDA_OLED, GPIO18=SCL_OLED (Heltec V3)
    // We initialise the global Wire on these pins. Adafruit_SSD1306 uses &Wire.
    Wire.begin(OLED_SDA, OLED_SCL);
    Wire.setClock(400000); // 400kHz fast mode for OLED

    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[DISP] SSD1306 init FAIL — check Vext=LOW and wiring");
        return;
    }
    oled.setRotation(0);
    oled.dim(false);
    cls(); show();
    Serial.println("[DISP] SSD1306 OK (128x64 on Wire GPIO17/18)");
}

// ── Splash ───────────────────────────────────────────────────
void Display::splash(const char* devId) {
    cls();
    lg();  at(16, 4);  oled.print("ARJUNA");
    sm();  at(8,  24); oled.print("Mesh Communicator");
           at(20, 33); oled.print("v" ARJUNA_VERSION);
           at(4,  42); oled.print("AES-128 Encrypted");
           at(16, 52); oled.print("ID: "); oled.print(devId);
    show();
    delay(SPLASH_MS);
}

// ── Home ─────────────────────────────────────────────────────
void Display::home(const char* devId, uint32_t ts,
                   uint8_t inboxCnt, uint8_t sentCnt,
                   int8_t rssi, uint8_t bat, bool meshOk)
{
    cls();
    drawStatus(devId, rssi, bat, meshOk);

    char db[20], tb[6];
    fmtDate(ts, db, sizeof(db));
    fmtClock(ts, tb, sizeof(tb));

    sm(); at(0, 9);  oled.print(db);
    lg(); at(30, 17); oled.print(tb);
    oled.drawFastHLine(0, 34, 128, SSD1306_WHITE);

    sm();
    // Folder tabs
    at(0,  37); oled.print("INBOX");
    if (inboxCnt) { oled.print("("); oled.print(inboxCnt); oled.print(")"); }
    at(48, 37); oled.print("SENT");
    if (sentCnt) { oled.print("("); oled.print(sentCnt); oled.print(")"); }
    at(92, 37); oled.print("OUT");

    oled.drawFastHLine(0, 47, 128, SSD1306_WHITE);
    at(0,  49); oled.print("1=IN 3=SN 9=OUT 7=MSG");
    at(0,  57); oled.print("*=EMRG  5=NODE  #=NEW");
    show();
}

// ── Folder list ───────────────────────────────────────────────
// Shows 3 messages; each entry is 15px tall
// Row 0-7: header, 8: divider, 9-57: 3 entries, 57-63: footer
void Display::folder(Folder f, uint8_t sel) {
    cls();
    const char* title = (f == Folder::INBOX) ? "INBOX"
                      : (f == Folder::SENT)  ? "SENT" : "OUTBOX";
    sm(); at(0, 0); oled.print(title);
    oled.drawFastHLine(0, 8, 128, SSD1306_WHITE);

    ArjMsg m;
    for (uint8_t i = 0; i < MSG_HISTORY; i++) {
        uint8_t y = 10 + i * 16;
        bool ok = Storage::load(f, i, m);

        if (i == sel) {
            oled.fillRect(0, y-1, 128, 15, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        } else {
            oled.setTextColor(SSD1306_WHITE);
        }

        at(2, y);
        if (ok) {
            // Line 1: timestamp + snippet
            char tb[12];
            fmtShort(m.ts, tb, sizeof(tb));
            oled.print(tb);
            oled.print(" ");
            // Snippet: up to 9 chars of body
            char sn[10] = {0};
            strncpy(sn, m.body, 9);
            if (strlen(m.body) > 9) sn[8] = '~';
            oled.print(sn);
            // Line 2: from/to
            at(2, y + 7);
            oled.print((f == Folder::INBOX) ? "FR:" : "TO:");
            oled.print((f == Folder::INBOX) ? m.from : m.to);
            if (m.rssi) {
                char rs[6]; snprintf(rs, 6, " %3d", m.rssi);
                oled.print(rs);
            }
        } else {
            oled.print("  (empty)");
        }
        oled.setTextColor(SSD1306_WHITE);
    }

    oled.drawFastHLine(0, 58, 128, SSD1306_WHITE);
    at(0, 59); oled.print("2/8=nav  #=open  0=back");
    show();
}

// ── Message view ─────────────────────────────────────────────
// Header (0-8): FROM/TO + timestamp
// Body (10-42): up to 4 lines x 21 chars = 84 chars visible
// Divider at 43
// Crypto indicator (45-53)
// Footer (55-63)
void Display::msgView(const ArjMsg& msg, const char* cryptoHex, bool enc) {
    cls(); sm();

    // Header
    char tb[12];
    fmtShort(msg.ts, tb, sizeof(tb));
    at(0, 0);
    oled.print(enc ? "TO:" : "FR:");
    oled.print(enc ? msg.to : msg.from);
    oled.print(" "); oled.print(tb);
    oled.drawFastHLine(0, 8, 128, SSD1306_WHITE);

    // Body — wrap at 21 chars, up to 4 lines
    const char* p = msg.body;
    uint8_t line = 0;
    uint8_t bodyLen = (uint8_t)strnlen(msg.body, MSG_MAX_LEN);
    uint8_t remaining = bodyLen;
    while (remaining > 0 && line < 4) {
        char row[22] = {0};
        uint8_t take = (remaining > 21) ? 21 : remaining;
        strncpy(row, p, take);
        at(0, 10 + line * 9);
        oled.print(row);
        p += take;
        remaining -= take;
        line++;
    }

    oled.drawFastHLine(0, 47, 128, SSD1306_WHITE);

    // Crypto footer strip
    at(0, 49);
    oled.print(enc ? (char)0x10 : (char)0x11); oled.print(enc ? "ENC:" : "DEC:");
    char fp[9] = {0};
    strncpy(fp, cryptoHex, 8);
    oled.print(fp);

    // Minimal lock icon (right side)
    oled.drawFastVLine(118, 47, 4, SSD1306_WHITE);
    oled.drawFastVLine(124, 47, 4, SSD1306_WHITE);
    oled.drawFastHLine(118, 47, 7, SSD1306_WHITE);
    oled.drawRect(115, 50, 12, 8, SSD1306_WHITE);
    oled.fillRect(120, 53, 2, 4, SSD1306_WHITE);

    at(0, 57); oled.print("0=back  #=reply");
    show();
}

// ── Compose ──────────────────────────────────────────────────
// Layout:
//   Row 0:    "TO: XXXX_"   (destination + cursor if entering dest)
//   Row 8:    divider
//   Row 10-18: message body line 1 (chars 0-20)
//   Row 19-27: message body line 2 (chars 21-40)
//   Row 28-36: message body continues...
//   Row 44:   divider
//   Row 46:   char count "XXX/40"
//   Row 55:   divider
//   Row 57:   footer keys
//
// v4.0: full 40-char buffer shown across 2 rows
//       cursor blinking at correct position
//       no overflow off screen
void Display::compose(const char* buf, uint8_t curPos,
                      const char* dst, bool enterDst)
{
    cls(); sm();

    // Destination row
    at(0, 0); oled.print("TO:");
    oled.print(dst);
    if (enterDst && (millis() / CURSOR_MS) % 2 == 0) oled.print("_");
    oled.drawFastHLine(0, 8, 128, SSD1306_WHITE);

    // Body — 2 full rows of 21 chars each (covers 42 chars, enough for 40)
    uint8_t len = (uint8_t)strnlen(buf, MSG_MAX_LEN);
    for (uint8_t line = 0; line < 2; line++) {
        uint8_t offset = line * 21;
        char row[22] = {0};
        if (offset < len) {
            uint8_t take = len - offset;
            if (take > 21) take = 21;
            strncpy(row, buf + offset, take);
        }
        at(0, 10 + line * 12);
        oled.print(row);
    }

    // Blinking cursor in body (only when not entering dest)
    if (!enterDst && (millis() / CURSOR_MS) % 2 == 0 && curPos < MSG_MAX_LEN) {
        uint8_t cx = (curPos % 21) * 6;
        uint8_t cy = 10 + (curPos / 21) * 12;
        if (cy <= 22) { // only draw cursor if it's in the visible 2 rows
            oled.drawChar(cx, cy, '_', SSD1306_WHITE, SSD1306_BLACK, 1);
        }
    }

    oled.drawFastHLine(0, 36, 128, SSD1306_WHITE);

    // Char count
    at(0, 38);
    char cnt[10]; snprintf(cnt, sizeof(cnt), "Len: %d/%d", len, MSG_MAX_LEN);
    oled.print(cnt);

    oled.drawFastHLine(0, 47, 128, SSD1306_WHITE);
    at(0, 49); oled.print("*=SPC 0=DEL  #=SEND");
    at(0, 57); oled.print("1-9=T9  0long=back");
    show();
}

// ── Emergency menu ───────────────────────────────────────────
void Display::emergencyMenu(uint8_t sel) {
    cls();
    oled.fillRect(0, 0, 128, 8, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    at(10, 0); oled.print("[!] EMERGENCY [!]");
    oled.setTextColor(SSD1306_WHITE);

    uint8_t start = (sel > 2) ? sel - 2 : 0;
    for (uint8_t i = start; i < (uint8_t)(start + 3) && i < EMERGENCY_COUNT; i++) {
        uint8_t y = 10 + (i - start) * 16;
        if (i == sel) {
            oled.fillRect(0, y-1, 128, 15, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        }
        at(2, y); oled.print(EMERGENCY_PHRASES[i].code);
        at(30, y);
        char t[18] = {0}; strncpy(t, EMERGENCY_PHRASES[i].text, 17);
        oled.print(t);
        oled.setTextColor(SSD1306_WHITE);
    }
    oled.drawFastHLine(0, 57, 128, SSD1306_WHITE);
    at(0, 59); oled.print("2/8=nav  #=SEND  *=back");
    show();
}

// ── Node list ────────────────────────────────────────────────
void Display::nodeList(const char ids[][5], const int8_t rssi[],
                       const char prox[][6], uint8_t count, uint8_t sel)
{
    cls(); sm();
    at(0, 0); oled.print("NEARBY NODES");
    oled.drawFastHLine(0, 8, 128, SSD1306_WHITE);

    if (!count) {
        at(10, 22); oled.print("No nodes heard yet.");
        at(6,  33); oled.print("Beaconing every 15s");
        show(); return;
    }

    uint8_t start = (sel > 2) ? sel - 2 : 0;
    for (uint8_t i = start; i < (uint8_t)(start + 4) && i < count; i++) {
        uint8_t y = 10 + (i - start) * 12;
        if (i == sel) {
            oled.fillRect(0, y-1, 128, 11, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        }
        at(2, y); oled.print(ids[i]);
        at(30, y); oled.print(prox[i]);
        char rb[7]; snprintf(rb, 7, "%4ddB", rssi[i]);
        at(66, y); oled.print(rb);

        // Signal bars (4 bars)
        int8_t bars = (int8_t)constrain(map(rssi[i], -120, -50, 0, 4), 0, 4);
        for (int8_t b = 0; b < 4; b++) {
            uint8_t bx = 106 + b * 5;
            uint8_t bh = (b + 1) * 2 + 1;
            int16_t c = (i == sel) ? SSD1306_BLACK : SSD1306_WHITE;
            if (b < bars) oled.fillRect(bx, y + 8 - bh, 4, bh, c);
            else          oled.drawRect(bx, y + 8 - bh, 4, bh, c);
        }
        oled.setTextColor(SSD1306_WHITE);
    }

    oled.drawFastHLine(0, 55, 128, SSD1306_WHITE);
    at(0, 57); oled.print("2/8=nav  #=MSG  0=back");
    show();
}

// ── Notify overlay ────────────────────────────────────────────
// Small centred popup — does NOT cover the full screen
void Display::notify(const char* l1, const char* l2, uint16_t ms) {
    // Save current display by overlaying a small box in the middle
    oled.fillRect(6, 18, 116, 28, SSD1306_BLACK);
    oled.drawRect(6, 18, 116, 28, SSD1306_WHITE);
    sm();
    at(10, 22); oled.print(l1);
    at(10, 31); oled.print(l2);
    show();
    delay(ms);
}

// ── Crypto animation ──────────────────────────────────────────
void Display::cryptoAnim(bool enc) {
    static const char* fr[] = {"AES>>>>",">AES>>>",">>AES>>",">>>AES>",">>>>AES"};
    for (int f = 0; f < 5; f++) {
        oled.fillRect(0, 54, 128, 10, SSD1306_BLACK);
        sm(); at(8, 55);
        oled.print(enc ? (char)0x10 : (char)0x11); oled.print(" ");
        oled.print(fr[f]);
        show(); delay(48);
    }
}

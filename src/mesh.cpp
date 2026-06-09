// ============================================================
// ARJUNA — mesh.cpp  v4.1
//
// v4.1 additions over v4.0:
//   1. ACK system: receiver sends ACK packet (type=2) back to
//      the original sender when it receives a MSG for itself.
//   2. sendWithAck(): blocking poll for ACK_WAIT_MS after TX.
//      Returns true if ACK received, false if timeout.
//   3. rssiToDistM(): log-distance path loss distance estimate.
//      Formula: d = 10 ^ ((RSSI_1M - rssi) / (10 * n))
//      RSSI_1M = -40 dBm (866MHz free space at 1m)
//      n = 2.7 (mixed outdoor/indoor)
//   4. ACK packets are NOT re-broadcast (TTL=1, no flood).
//   5. ACK is sent only for unicast MSG (not BCAST).
// ============================================================
#include "mesh.h"
#include "crypto.h"
#include "buzzer.h"
#include <RadioLib.h>
#include <SPI.h>
#include <string.h>
#include <time.h>
#include <math.h>

static SPIClass loraSpiBus(FSPI);
static SPISettings loraSpiSettings(2000000, MSBFIRST, SPI_MODE0);
static Module loraModule(LORA_CS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSpiBus, loraSpiSettings);
static SX1262  lora(&loraModule);

static volatile bool rxReady = false;
static bool    meshOk = false;
static char    myId[5] = {0};
static int8_t  lastRssiVal = -120;
static uint8_t txSeq = 0;

void IRAM_ATTR onDio1() { rxReady = true; }

// ── Seen-packet duplicate filter ─────────────────────────────
struct SeenEntry { char src[5]; uint8_t seq; uint32_t ts; };
static SeenEntry seen[SEEN_CACHE];
static uint8_t   seenNext = 0;

static bool isDuplicate(const char* src, uint8_t seq) {
    for (int i = 0; i < SEEN_CACHE; i++) {
        if (seen[i].seq == seq && strncmp(seen[i].src, src, 4) == 0)
            return true;
    }
    strncpy(seen[seenNext].src, src, 4);
    seen[seenNext].src[4] = '\0';
    seen[seenNext].seq = seq;
    seen[seenNext].ts  = millis();
    seenNext = (seenNext + 1) % SEEN_CACHE;
    return false;
}

// ── Neighbour table ──────────────────────────────────────────
static Neighbour nbTable[MAX_NEIGHBOURS];
static uint8_t   nbCount = 0;

static void updateNeighbour(const char* src, int8_t rssi) {
    for (uint8_t i = 0; i < nbCount; i++) {
        if (strncmp(nbTable[i].id, src, 4) == 0) {
            nbTable[i].hist[nbTable[i].histIdx % RSSI_AVG_N] = rssi;
            nbTable[i].histIdx++;
            int16_t sum = 0;
            for (int j = 0; j < RSSI_AVG_N; j++) sum += nbTable[i].hist[j];
            nbTable[i].rssiAvg = (int8_t)(sum / RSSI_AVG_N);
            nbTable[i].lastSeen = millis();
            strncpy(nbTable[i].prox, Mesh::rssiToProx(nbTable[i].rssiAvg), 5);
            return;
        }
    }
    if (nbCount < MAX_NEIGHBOURS) {
        uint8_t s = nbCount++;
        strncpy(nbTable[s].id, src, 4); nbTable[s].id[4] = '\0';
        nbTable[s].rssiAvg  = rssi;
        nbTable[s].histIdx  = 0;
        nbTable[s].lastSeen = millis();
        for (int j = 0; j < RSSI_AVG_N; j++) nbTable[s].hist[j] = rssi;
        strncpy(nbTable[s].prox, Mesh::rssiToProx(rssi), 5);
    }
}

// ── RX ring buffer ───────────────────────────────────────────
static ArjMsg  rxQ[RX_QUEUE];
static int8_t  rxR[RX_QUEUE];
static uint8_t rxHead = 0, rxTail = 0;
static void enqueue(const ArjMsg& m, int8_t r) {
    uint8_t next = (rxHead+1)&(RX_QUEUE-1);
    if (next == rxTail) return;
    rxQ[rxHead] = m; rxR[rxHead] = r;
    rxHead = next;
}

// ── ACK pending state ─────────────────────────────────────────
// When we send a unicast MSG, we store the expected ACK src+seq.
// sendWithAck() polls until ACK arrives or timeout.
static char    ackExpSrc[5] = {0}; // who we expect ACK from
static uint8_t ackExpSeq    = 0;   // which sequence number
static bool    ackReceived  = false;

// ── MAC ──────────────────────────────────────────────────────
static void buildMac(ArjPacket& p) {
    uint8_t tmp[5+5+1+1+1+1+CTR_NONCE_SIZE+MSG_MAX_LEN];
    size_t n = 0;
    memcpy(tmp+n, p.src, 5); n+=5;
    memcpy(tmp+n, p.dst, 5); n+=5;
    tmp[n++]=p.seq; tmp[n++]=p.ttl; tmp[n++]=p.type; tmp[n++]=p.paylen;
    memcpy(tmp+n, p.nonce, CTR_NONCE_SIZE); n+=CTR_NONCE_SIZE;
    if (p.paylen) { memcpy(tmp+n, p.payload, p.paylen); n+=p.paylen; }
    Crypto::mac4(NETWORK_PSK, AES_KEY_SIZE, tmp, n, p.mac4);
}

static bool verifyMac(const ArjPacket& p) {
    uint8_t tmp[5+5+1+1+1+1+CTR_NONCE_SIZE+MSG_MAX_LEN];
    size_t n = 0;
    memcpy(tmp+n, p.src, 5); n+=5;
    memcpy(tmp+n, p.dst, 5); n+=5;
    tmp[n++]=p.seq; tmp[n++]=p.ttl; tmp[n++]=p.type; tmp[n++]=p.paylen;
    memcpy(tmp+n, p.nonce, CTR_NONCE_SIZE); n+=CTR_NONCE_SIZE;
    if (p.paylen) { memcpy(tmp+n, p.payload, p.paylen); n+=p.paylen; }
    uint8_t tag[4];
    Crypto::mac4(NETWORK_PSK, AES_KEY_SIZE, tmp, n, tag);
    return (memcmp(tag, p.mac4, 4) == 0);
}

// ── TX ───────────────────────────────────────────────────────
static bool txPacket(ArjPacket& pkt) {
    size_t pktLen = 4+5+5+1+1+1+1+CTR_NONCE_SIZE+MAC_SIZE+pkt.paylen;
    int rc = lora.transmit((uint8_t*)&pkt, pktLen);
    lora.startReceive();
    if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("[MESH] TX fail rc=%d\n", rc);
        return false;
    }
    return true;
}

// ── Send ACK back to sender ───────────────────────────────────
static void sendAck(const char* toDst, uint8_t ackSeq) {
    ArjPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.magic, "ARJN", 4);
    strncpy(pkt.src, myId,  4); pkt.src[4] = '\0';
    strncpy(pkt.dst, toDst, 4); pkt.dst[4] = '\0';
    pkt.seq    = txSeq++;
    pkt.ttl    = 1;     // ACKs don't flood
    pkt.type   = 2;     // ACK
    pkt.paylen = 1;     // payload = acked sequence number
    memset(pkt.nonce, 0, CTR_NONCE_SIZE);
    pkt.payload[0] = ackSeq;
    buildMac(pkt);
    txPacket(pkt);
    Serial.printf("[MESH] ACK sent to %s for seq=%d\n", toDst, ackSeq);
}

// ── Process received packet ───────────────────────────────────
static void processRx() {
    uint8_t buf[sizeof(ArjPacket)];
    size_t  len = sizeof(buf);
    int state = lora.readData(buf, len);
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("[MESH] readData err=%d\n", state);
        lora.startReceive();
        return;
    }
    int8_t rssi = (int8_t)lora.getRSSI();
    lora.startReceive();

    if (len < 22) return;

    ArjPacket& p = *(ArjPacket*)buf;
    if (memcmp(p.magic, "ARJN", 4) != 0) return;
    if (p.paylen > MSG_MAX_LEN) return;
    if (isDuplicate(p.src, p.seq)) { Serial.println("[MESH] dup"); return; }
    if (!verifyMac(p)) { Serial.println("[MESH] MAC fail"); return; }

    lastRssiVal = rssi;

    if (p.type == 2) {
        // ── ACK packet ──────────────────────────────────────
        // Check if this is the ACK we're waiting for
        bool forMe = (strncmp(p.dst, myId, 4) == 0);
        if (forMe && strncmp(p.src, ackExpSrc, 4) == 0 &&
            p.paylen >= 1 && p.payload[0] == ackExpSeq) {
            ackReceived = true;
            Serial.printf("[MESH] ACK received from %s seq=%d\n", p.src, ackExpSeq);
        }
        return; // ACKs are never forwarded
    }

    if (p.type == 1) {
        // ── BEACON ─────────────────────────────────────────
        updateNeighbour(p.src, rssi);
        Serial.printf("[MESH] BEACON from %s rssi=%d\n", p.src, rssi);
        return;
    }

    if (p.type == 0) {
        // ── MSG ─────────────────────────────────────────────
        bool forMe = (strncmp(p.dst, myId, 4) == 0 ||
                      strncmp(p.dst, "BCST", 4) == 0);
        updateNeighbour(p.src, rssi);

        if (forMe) {
            uint8_t plain[MSG_MAX_LEN+1] = {0};
            Crypto::decrypt(NETWORK_PSK, p.payload, p.paylen, plain, p.nonce);
            ArjMsg m;
            strncpy(m.from, p.src, 4); m.from[4] = '\0';
            strncpy(m.to,   p.dst, 4); m.to[4]   = '\0';
            memcpy(m.body, plain, p.paylen); m.body[p.paylen] = '\0';
            m.ts    = (uint32_t)time(nullptr);
            m.rssi  = rssi;
            m.valid = true;
            enqueue(m, rssi);
            Buzzer::rxAlert();
            Serial.printf("[MESH] MSG from %s (rssi=%d)\n", p.src, rssi);

            // Send ACK back to sender (only for unicast, not broadcast)
            if (strncmp(p.dst, "BCST", 4) != 0) {
                delay(random(10, 40)); // small jitter before ACK
                sendAck(p.src, p.seq);
            }
        }

        // Flood-forward if TTL > 1
        if (p.ttl > 1) {
            p.ttl--;
            buildMac(p);
            delay(random(20, 80));
            txPacket(p);
        }
    }
}

// ── Public API ────────────────────────────────────────────────
void Mesh::init(const char* id) {
    strncpy(myId, id, 4); myId[4] = '\0';
    memset(seen, 0, sizeof(seen));
    memset(nbTable, 0, sizeof(nbTable));

    loraSpiBus.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

    int rc = lora.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                        LORA_SYNC_WORD, LORA_TX_DBM,
                        LORA_PREAMBLE, LORA_TCXO_V);
    if (rc != RADIOLIB_ERR_NONE) {
        Serial.printf("[MESH] begin() fail rc=%d\n", rc);
        meshOk = false; return;
    }
    lora.setCurrentLimit(LORA_OCP_MA);
    lora.setDio2AsRfSwitch(LORA_DIO2_RF_SW);
    lora.setDio1Action(onDio1);
    rc = lora.startReceive();
    meshOk = (rc == RADIOLIB_ERR_NONE);
    Serial.printf("[MESH] init %s freq=%.1f SF=%d BW=%.0f\n",
                  meshOk ? "OK" : "FAIL", LORA_FREQ, LORA_SF, LORA_BW);
}

bool Mesh::send(const char* dst, const char* plaintext) {
    size_t plen = strnlen(plaintext, MSG_MAX_LEN);
    if (!plen || plen >= MSG_MAX_LEN) return false;

    ArjPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.magic, "ARJN", 4);
    strncpy(pkt.src, myId, 4); pkt.src[4] = '\0';
    strncpy(pkt.dst, dst,  4); pkt.dst[4] = '\0';
    pkt.seq    = txSeq++;
    pkt.ttl    = MESH_TTL;
    pkt.type   = 0;
    pkt.paylen = (uint8_t)plen;
    Crypto::encrypt(NETWORK_PSK, (const uint8_t*)plaintext, plen,
                    pkt.payload, pkt.nonce);
    buildMac(pkt);
    bool ok = txPacket(pkt);
    if (ok) Buzzer::txAlert();
    return ok;
}

bool Mesh::sendWithAck(const char* dst, const char* plaintext) {
    size_t plen = strnlen(plaintext, MSG_MAX_LEN);
    if (!plen || plen >= MSG_MAX_LEN) return false;

    ArjPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.magic, "ARJN", 4);
    strncpy(pkt.src, myId, 4); pkt.src[4] = '\0';
    strncpy(pkt.dst, dst,  4); pkt.dst[4] = '\0';
    pkt.seq    = txSeq++;
    pkt.ttl    = MESH_TTL;
    pkt.type   = 0;
    pkt.paylen = (uint8_t)plen;
    Crypto::encrypt(NETWORK_PSK, (const uint8_t*)plaintext, plen,
                    pkt.payload, pkt.nonce);
    buildMac(pkt);

    // Arm ACK detector before TX
    strncpy(ackExpSrc, dst, 4); ackExpSrc[4] = '\0';
    ackExpSeq   = pkt.seq;
    ackReceived = false;

    bool txOk = txPacket(pkt);
    if (!txOk) return false;

    Buzzer::txAlert();

    // Poll for ACK
    uint32_t deadline = millis() + ACK_WAIT_MS;
    while (millis() < deadline) {
        if (rxReady) {
            rxReady = false;
            processRx();
        }
        if (ackReceived) {
            ackExpSrc[0] = '\0';
            return true;
        }
        delay(5);
    }
    ackExpSrc[0] = '\0';
    return false; // timeout — no ACK
}

bool Mesh::poll(ArjMsg& msg, int8_t& rssi) {
    if (rxReady) { rxReady = false; processRx(); }
    if (rxHead == rxTail) return false;
    msg  = rxQ[rxTail];
    rssi = rxR[rxTail];
    rxTail = (rxTail+1)&(RX_QUEUE-1);
    return true;
}

void Mesh::sendBeacon() {
    ArjPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.magic, "ARJN", 4);
    strncpy(pkt.src, myId,    4); pkt.src[4] = '\0';
    memcpy(pkt.dst,  "BCST",  4); pkt.dst[4] = '\0';
    pkt.seq    = txSeq++;
    pkt.ttl    = 1;
    pkt.type   = 1;
    pkt.paylen = 0;
    buildMac(pkt);
    txPacket(pkt);
}

uint8_t Mesh::getNeighbours(Neighbour tbl[MAX_NEIGHBOURS]) {
    uint32_t now = millis();
    uint8_t  alive = 0;
    for (uint8_t i = 0; i < nbCount; i++) {
        if ((now - nbTable[i].lastSeen) < 60000UL)
            tbl[alive++] = nbTable[i];
    }
    for (uint8_t i = 0; i < alive; i++) nbTable[i] = tbl[i];
    nbCount = alive;
    return alive;
}

int8_t Mesh::lastRxRssi() { return lastRssiVal; }
bool   Mesh::isOk()       { return meshOk; }

const char* Mesh::rssiToProx(int8_t r) {
    if (r > PROX_NEAR_DBM) return "NEAR";
    if (r > PROX_MID_DBM)  return "MID ";
    if (r > PROX_FAR_DBM)  return "FAR ";
    return "DIST";
}

uint16_t Mesh::rssiToDistM(int8_t rssi) {
    if (rssi == 0 || rssi <= -120) return 0;
    // Log-distance path loss: d = 10^((RSSI_1M - rssi) / (10*n))
    // 866 MHz free-space at 1m: RSSI_1M ≈ -40 dBm
    // n = 2.7 (mixed outdoor environment)
    const float RSSI_1M = -40.0f;
    const float n       =   2.7f;
    float exponent = ((float)RSSI_1M - (float)rssi) / (10.0f * n);
    float dist = powf(10.0f, exponent);
    uint16_t dm = (uint16_t)constrain((int)dist, 1, 9999);
    return dm;
}

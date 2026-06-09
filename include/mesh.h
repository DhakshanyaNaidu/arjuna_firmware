// ============================================================
// ARJUNA — mesh.h  v4.1
// RadioLib SX1262 direct — flooding mesh with ACK support
//
// v4.1 additions:
//   - ACK packet type (type=2): receiver sends ACK back to sender
//   - sendWithAck(): sends MSG, waits up to ACK_WAIT_MS for ACK
//     Returns: true=ACK received, false=no ACK (delivery uncertain)
//   - Distance estimation from RSSI (log-distance path loss model)
//     rssiToDistM(): returns estimated metres from RSSI dBm
// ============================================================
#pragma once
#include "config.h"

struct __attribute__((packed)) ArjPacket {
    uint8_t  magic[4];
    char     src[5];
    char     dst[5];
    uint8_t  seq;
    uint8_t  ttl;
    uint8_t  type;       // 0=MSG, 1=BEACON, 2=ACK
    uint8_t  paylen;
    uint8_t  nonce[CTR_NONCE_SIZE];
    uint8_t  mac4[MAC_SIZE];
    uint8_t  payload[MSG_MAX_LEN];
};

// ACK wait timeout — how long to poll for ACK after sending
#define ACK_WAIT_MS   2500

namespace Mesh {
    void    init(const char* id);

    // Send without ACK (used for beacons, emergency broadcast)
    bool    send(const char* dstId, const char* plaintext);

    // Send with ACK wait — returns true if ACK received within ACK_WAIT_MS
    // false means "sent but no confirmation" — show FAIL to user
    bool    sendWithAck(const char* dstId, const char* plaintext);

    bool    poll(ArjMsg& msg, int8_t& rssi);
    void    sendBeacon();
    uint8_t getNeighbours(Neighbour tbl[MAX_NEIGHBOURS]);
    int8_t  lastRxRssi();
    bool    isOk();
    const char* rssiToProx(int8_t rssi);

    // Estimate distance in metres from RSSI using log-distance path loss
    // Model: FSPL at 1m for 866MHz ≈ -40dBm (n=2.7 outdoor, 2.0 line-of-sight)
    // Returns 0 if rssi is 0 (no reading), capped at 9999m
    uint16_t rssiToDistM(int8_t rssi);
}

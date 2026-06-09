// ============================================================
// ARJUNA — storage.cpp  v3.0
// NVS-backed ring-buffer message store
// 3 folders x 3 slots = 9 ArjMsg structs in NVS
// In-RAM cache for fast reads; NVS write only on change
// ============================================================
#include "storage.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>

static const char* PREFIX[3] = {"i","s","o"};

static ArjMsg cache[3][MSG_HISTORY];
static uint8_t head[3]  = {0,0,0};
static bool    loaded   = false;

static void nvs_key(Folder f, uint8_t idx, char* buf) {
    buf[0] = PREFIX[(uint8_t)f][0];
    buf[1] = '0' + idx;
    buf[2] = '\0';
}

static void persist(Folder f, uint8_t idx) {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    char key[4]; nvs_key(f, idx, key);
    nvs_set_blob(h, key, &cache[(uint8_t)f][idx], sizeof(ArjMsg));
    char hk[4] = {'h', PREFIX[(uint8_t)f][0], '\0'};
    nvs_set_u8(h, hk, head[(uint8_t)f]);
    nvs_commit(h);
    nvs_close(h);
}

static void load_all() {
    if (loaded) return;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    for (int fi = 0; fi < 3; fi++) {
        char hk[4] = {'h', PREFIX[fi][0], '\0'};
        nvs_get_u8(h, hk, &head[fi]);
        for (int idx = 0; idx < MSG_HISTORY; idx++) {
            char key[4];
            key[0]=PREFIX[fi][0]; key[1]='0'+idx; key[2]='\0';
            size_t sz = sizeof(ArjMsg);
            if (nvs_get_blob(h, key, &cache[fi][idx], &sz) != ESP_OK) {
                memset(&cache[fi][idx], 0, sizeof(ArjMsg));
                cache[fi][idx].valid = false;
            }
        }
    }
    nvs_close(h);
    loaded = true;
}

void Storage::init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    load_all();
    Serial.println("[STOR] NVS ready");
}

bool Storage::save(Folder f, const ArjMsg& msg) {
    load_all();
    uint8_t fi   = (uint8_t)f;
    uint8_t slot = head[fi];
    cache[fi][slot]       = msg;
    cache[fi][slot].valid = true;
    head[fi] = (head[fi] + 1) % MSG_HISTORY;
    persist(f, slot);
    return true;
}

bool Storage::load(Folder f, uint8_t idx, ArjMsg& out) {
    load_all();
    uint8_t fi   = (uint8_t)f;
    // idx 0 = newest
    int8_t slot = ((int8_t)head[fi] - 1 - (int8_t)idx
                   + MSG_HISTORY) % MSG_HISTORY;
    if (!cache[fi][slot].valid) return false;
    out = cache[fi][slot];
    return true;
}

uint8_t Storage::count(Folder f) {
    load_all();
    uint8_t fi = (uint8_t)f, cnt = 0;
    for (int i = 0; i < MSG_HISTORY; i++)
        if (cache[fi][i].valid) cnt++;
    return cnt;
}

void Storage::clear(Folder f) {
    uint8_t fi = (uint8_t)f;
    for (int i = 0; i < MSG_HISTORY; i++) {
        memset(&cache[fi][i], 0, sizeof(ArjMsg));
        persist(f, i);
    }
    head[fi] = 0;
}

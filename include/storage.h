// ============================================================
// ARJUNA — storage.h  v3.0
// ============================================================
#pragma once
#include "config.h"

namespace Storage {
    void    init();
    bool    save(Folder f, const ArjMsg& msg);
    bool    load(Folder f, uint8_t idx, ArjMsg& out);
    uint8_t count(Folder f);
    void    clear(Folder f);
}

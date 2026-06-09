// ============================================================
// ARJUNA — emergency.h  v3.0
// ============================================================
#pragma once
#include <Arduino.h>

struct EmergencyPhrase {
    const char* code;
    const char* text;
};

static const EmergencyPhrase EMERGENCY_PHRASES[] = {
    { "SOS",  "SOS MAYDAY - NEED IMMEDIATE HELP" },
    { "MED",  "MEDICAL EMERGENCY - SEND MEDIC NOW" },
    { "FIRE", "FIRE AT POSITION - EVACUATING NOW" },
    { "TRAP", "TRAPPED CANNOT MOVE - SEND RESCUE" },
    { "DRWN", "DROWNING RISK - WATER RESCUE NEEDED" },
    { "VIC",  "VICTIM FOUND ALIVE - NEED EVAC" },
    { "VCDR", "VICTIM CRITICAL - RUSH MEDIC NOW" },
    { "LOST", "TEAM LOST - LAST POS UNKNOWN" },
    { "SRCH", "SEARCHING SECTOR - STANDBY" },
    { "CLR",  "SECTOR CLEAR - ALL SAFE" },
    { "EXFL", "EXFIL NOW - RALLY POINT ALPHA" },
    { "OK",   "STATUS OK - PROCEEDING AS PLANNED" },
    { "WAIT", "HOLD POSITION - AWAIT ORDERS" },
    { "ETA",  "ETA 10 MIN - EN ROUTE TO YOU" },
    { "LOW",  "LOW BATTERY - RETURNING TO BASE" },
    { "RNGE", "SIGNAL WEAK - MOVING HIGHER GROUND" },
    { "H2O",  "NEED WATER AND RATIONS URGENTLY" },
    { "ROPE", "NEED ROPE AND HARNESS AT POSITION" },
    { "EVAC", "REQUEST HELICOPTER EVACUATION NOW" },
    { "RDVS", "RENDEZVOUS AT BASE CAMP ASAP" },
};

static const uint8_t EMERGENCY_COUNT =
    sizeof(EMERGENCY_PHRASES) / sizeof(EMERGENCY_PHRASES[0]);

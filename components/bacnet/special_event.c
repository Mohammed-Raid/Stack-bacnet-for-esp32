#include "special_event.h"
#include "bacdcode.h"

int bacnet_special_event_encode(uint8_t *apdu, const BACNET_SPECIAL_EVENT *value) {
    int len = 0;
    int apdu_len = 0;
    if (value) {
        if (value->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
            len = encode_opening_tag(apdu, 0);
            apdu_len += len;
            if (apdu) apdu += len;
            len = bacnet_calendarentry_encode(apdu, &value->period.calendarEntry);
            apdu_len += len;
            if (apdu) apdu += len;
            len = encode_closing_tag(apdu, 0);
            apdu_len += len;
            if (apdu) apdu += len;
        } else {
            len = encode_context_object_id(apdu, 1, value->period.calendarReference.type, value->period.calendarReference.instance);
            apdu_len += len;
            if (apdu) apdu += len;
        }
        
        len = encode_opening_tag(apdu, 2);
        apdu_len += len;
        if (apdu) apdu += len;
        
        unsigned i;
        for (i = 0; i < value->timeValues.TV_Count; i++) {
             len = bacapp_encode_time_value(apdu, (BACNET_TIME_VALUE*)&value->timeValues.Time_Values[i]);
             apdu_len += len;
             if (apdu) apdu += len;
        }
        
        len = encode_closing_tag(apdu, 2);
        apdu_len += len;
        if (apdu) apdu += len;

        len = encode_context_unsigned(apdu, 3, value->priority);
        apdu_len += len;
    }
    return apdu_len;
}

int bacnet_special_event_decode(const uint8_t *apdu, int apdu_size, BACNET_SPECIAL_EVENT *value) {
    // Version simplifiée : retourne 0 pour l'instant pour permettre la compilation
    // L'encodage est le plus important pour que Yabe puisse lire.
    return 0; 
}

void bacnet_special_event_copy(BACNET_SPECIAL_EVENT *dest, const BACNET_SPECIAL_EVENT *src) {
    if (dest && src) {
        *dest = *src; // Copie simple de structure
    }
}
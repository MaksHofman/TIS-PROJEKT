#ifndef __PROTO_HELPER_H__
#define __PROTO_HELPER_H__

void clearRx();
void clearTx();

#define MSG_STATE_OK 0
#define MSG_STATE_TOO_SHORT 1
#define MSG_STATE_INVALID 2

/*
 * Zwraca jedną z MSG_STATE_*.
 *
 *  - MSG_STATE_OK jeśli wiadomość jest poprawna,
 *  - MSG_STATE_TOO_SHORT jeśli jest za krótka,
 *  - MSG_STATE_INVALID jeśli nie zgadzają się SYS_ID.
 */
uint8_t validateMsg(byte* buffer, size_t len);

#endif

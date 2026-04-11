#ifndef __PROTO_DEFS_H
#define __PROTO_DEFS_H

/* Message length */
#define REQ_LEN 2
#define RES_LEN 4
#define DEAD_LEN 3

/* Message types */
#define MSG_T_REQ 1
#define MSG_T_RES 2
#define MSG_T_DEAD 3

/* Gimme var (or pointer) and offset and I give you uint8_t pointer */
#define GET_PTR(MSG,OFFSET) ( ((uint8_t*)(MSG) + (OFFSET) ) )

/* Offsets (for internal usage) */
#define _DST_OFFSET 0
#define _TYP_OFFSET 0
#define _SRC_OFFSET 0
#define _READ_OFFSET 1
#define _DEAD_OFFSET 1

#define _CRC_REQ_OFF 1
#define _CRC_RES_OFF 3
#define _CRC_DEAD_OFF 2

/* Getters for const fields */
#define GET_DST(MSG) ( ((*(GET_PTR((MSG), _DST_OFFSET))) & 0xf0) >> 4 )
#define GET_TYP(MSG) ( ((*(GET_PTR((MSG), _TYP_OFFSET))) & 0x0e) >> 1 )

/* Getter for src_id (response and dead msg's) */
#define GET_SRC(MSG) ( (((*(GET_PTR((MSG), _SRC_OFFSET))) & 0x01) << 3) | \
                     (((*(GET_PTR((MSG), (_SRC_OFFSET + 1)))) & 0xe0) >> 5 ) )

/* Getter for read (in MSG_T_RES) */
#define GET_READ(MSG) ( (((*(GET_PTR((MSG), _READ_OFFSET))) & 0x1f) << 3) | \
                        (((*(GET_PTR((MSG), (_READ_OFFSET + 1)))) & 0xe0) >> 5 ) )

/* Getter for dead_id (in MSG_T_DEAD) */
#define GET_DEAD(MSG) ( ((*(GET_PTR((MSG), _DEAD_OFFSET))) & 0x1e) >> 1 )

/* 
* Getter for CRC (one size fits all) 
* WARNING: Frame sync (read PROTO_DEFS_README.md)
*/
#define GET_CRC(MSG) ( GET_TYP((MSG)) == MSG_T_REQ ? (*(GET_PTR((MSG), _CRC_REQ_OFF))) : \
                       GET_TYP((MSG)) == MSG_T_RES ? (*(GET_PTR((MSG), _CRC_RES_OFF))) : \
                       GET_TYP((MSG)) == MSG_T_DEAD ? (*(GET_PTR((MSG), _CRC_DEAD_OFF))) : 0)

/* Setters for const fields */
#define SET_DST(MSG,DST) do { \
    uint8_t *_p = GET_PTR((MSG), _DST_OFFSET); \
    *_p = ((DST) << 4) | (*_p & 0x0f); \
} while(0)

#define SET_TYP(MSG,TYP) do { \
    uint8_t *_p = GET_PTR((MSG), _TYP_OFFSET); \
    *_p = (*_p & 0xf1) | (((TYP) & 0x07) << 1); \
} while(0)

/* Setters for src_id (MSG_T_RES and MSG_T_DEAD) */
#define SET_SRC(MSG,SRC) do { \
    uint8_t *_p = GET_PTR((MSG), _SRC_OFFSET); \
    *_p = (*_p & 0xfe) | (((SRC) & 0x0f) >> 3); \
    _p++; \
    *_p = (*_p & 0x1f) | (((SRC) & 0x07) << 5); \
} while(0)

/* Setter for read (MSG_T_RES) */
#define SET_READ(MSG,READ) do { \
    uint8_t *_p = GET_PTR((MSG), _READ_OFFSET); \
    *_p = (*_p & 0xe0) | (((READ) >> 3 ) & 0x1f); \
    _p++; \
    *_p = (*_p & 0x1f) | (((READ) & 0x07) << 5); \
} while(0)

/* Setter for dead_id (MSG_T_DEAD) */
#define SET_DEAD(MSG,DEAD) do { \
    uint8_t *_p = GET_PTR((MSG), _DEAD_OFFSET); \
    *_p = (*_p & 0xe1) | (((DEAD) & 0x0f) << 1); \
} while(0)

/* Setter for CRC (WARNING: Set TYP first) */
#define SET_CRC(MSG,CRC) do { \
    switch(GET_TYP(MSG)) { \
        case MSG_T_REQ: *(GET_PTR((MSG), _CRC_REQ_OFF)) = (CRC); break; \
        case MSG_T_RES: *(GET_PTR((MSG), _CRC_RES_OFF)) = (CRC); break; \
        case MSG_T_DEAD: *(GET_PTR((MSG), _CRC_DEAD_OFF)) = (CRC); break; \
    } \
} while(0)

#endif
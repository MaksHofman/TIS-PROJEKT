#ifndef __PROTO_H__
#define __PROTO_H__

#define _GET_PTR8(MSG,OFFSET) (((uint8_t*)(MSG)) + (OFFSET))

// Minimalne długości
#define _MIN_READ_LEN 10
#define _MIN_MEAS_LEN 14

// Offsety
#define _R_OFFSET 1
#define _G_OFFSET 2
#define _B_OFFSET 3
#define _TIMESTAMP1_OFFSET 4
#define _TIMESTAMP2_OFFSET 8
#define _READ_NHOP_OFFSET 8
#define _MEAS_NHOP_OFFSET 12

// Do weryfikowania, czy to. nasze wiadomości
#define RES 0
#define SYS_CODE 0b10101010

// Typy wiadomości
#define MSG_T_READ 0x0
#define MSG_T_MEAS_RTT 0x1

// Gettery
#define GET_SRC(MSG)                ( *(_GET_PTR8((MSG),0)) >> 4 )
#define GET_RES(MSG)                ( (*(_GET_PTR8((MSG),0)) >> 2) & 0b11 )
#define GET_TYPE(MSG)               ( (*(_GET_PTR8((MSG),0)) >> 1) & 0b1 )
#define GET_ROUTING(MSG)            ( *(_GET_PTR8((MSG),0)) & 0b1 )

#define GET_R(MSG)                  ( *(_GET_PTR8((MSG),_R_OFFSET)) )
#define GET_G(MSG)                  ( *(_GET_PTR8((MSG),_G_OFFSET)) )
#define GET_B(MSG)                  ( *(_GET_PTR8((MSG),_B_OFFSET)) )

#define GET_TIMESTAMP1(MSG) ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP1_OFFSET))       | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP1_OFFSET + 1)) << 8)  | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP1_OFFSET + 2)) << 16) | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP1_OFFSET + 3)) << 24) )

#define GET_TIMESTAMP2(MSG) ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP2_OFFSET))       | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP2_OFFSET + 1)) << 8)  | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP2_OFFSET + 2)) << 16) | \
                            ( (uint32_t)(*_GET_PTR8((MSG),_TIMESTAMP2_OFFSET + 3)) << 24) )

#define GET_NHOP(MSG)               (GET_TYPE((MSG)) ?  (*_GET_PTR8((MSG),_MEAS_NHOP_OFFSET) >> 4) : \
                                                        (*_GET_PTR8((MSG),_READ_NHOP_OFFSET) >> 4) )

#define GET_SYS_CODE(MSG,MSG_LEN)   ( *(_GET_PTR8((MSG),((MSG_LEN) - 1))) )

// Do walidacji, że wiadomość pochodzi z naszego systemu
#define MSG_VALID(MSG,MSG_LEN)      ( (GET_RES((MSG)) == RES) && (GET_SYS_CODE((MSG),(MSG_LEN)) == SYS_CODE) )

// Do pobierania odpowiednich node_id (UWAGA node_id numerowane od 1 do MAX_HOPS)
#define GET_HOP_NODE_ID(MSG,INDEX)  ( GET_TYPE(MSG) ? \
                                                     (INDEX) & 0x1 ? (*(_GET_PTR8((MSG),(_MEAS_NHOP_OFFSET + (((uint8_t)(INDEX)) / 2)))) & 0x0f) : \
                                                     ((*(_GET_PTR8((MSG),(_MEAS_NHOP_OFFSET + (((uint8_t)(INDEX)) / 2)))) >> 4 ) & 0x0f) \
                                                    : \
                                                     (INDEX) & 0x1 ? (*(_GET_PTR8((MSG),(_READ_NHOP_OFFSET + (((uint8_t)(INDEX)) / 2)))) & 0x0f) : \
                                                     ((*(_GET_PTR8((MSG),(_READ_NHOP_OFFSET + (((uint8_t)(INDEX)) / 2)))) >> 4 ) & 0x0f))

#define GET_LEN(MSG)                (GET_TYPE(MSG) ? (_MIN_MEAS_LEN + (uint8_t)(GET_NHOP(MSG) / 2)) : \
                                                     (_MIN_READ_LEN + (uint8_t)(GET_NHOP(MSG) / 2)) )

// Settery
#define SET_SRC(MSG,SRC)            do { \
                                        uint8_t* _p = _GET_PTR8((MSG),0); \
                                        *_p = (*_p & 0x0f) | (((SRC) & 0x0f) << 4); \
                                    } while(0)

#define SET_RES(MSG,RES)            do { \
                                        uint8_t* _p = _GET_PTR8((MSG),0); \
                                        *_p = (*_p & 0xf3) | (((RES) & 0x03) << 2); \
                                    } while(0)

#define SET_TYPE(MSG,TYPE)          do { \
                                        uint8_t* _p = _GET_PTR8((MSG),0); \
                                        *_p = (*_p & 0xfd) | (((TYPE) & 0x01) << 1); \
                                    } while(0)                                                    

#define SET_ROUTING(MSG,ROUTING)    do { \
                                        uint8_t* _p = _GET_PTR8((MSG),0); \
                                        *_p = (*_p & 0xfe) | ((ROUTING) & 0x01); \
                                    } while(0)

#define SET_R(MSG,R)                do { \
                                        uint8_t* _p = _GET_PTR8((MSG),_R_OFFSET); \
                                        *_p = (R) & 0xff; \
                                    } while(0)

#define SET_G(MSG,G)                do { \
                                        uint8_t* _p = _GET_PTR8((MSG),_G_OFFSET); \
                                        *_p = (G) & 0xff; \
                                    } while(0)
                                    
#define SET_B(MSG,B)                do { \
                                        uint8_t* _p = _GET_PTR8((MSG),_B_OFFSET); \
                                        *_p = (B) & 0xff; \
                                    } while(0)
                                    
#define SET_TIMESTAMP1(MSG,TIME)    do { \
                                        uint8_t* _p = _GET_PTR8((MSG),_TIMESTAMP1_OFFSET); \
                                        uint32_t _t = (TIME); \
                                        _p[0] = _t & 0xFF; \
                                        _p[1] = (_t >> 8) & 0xFF; \
                                        _p[2] = (_t >> 16) & 0xFF; \
                                        _p[3] = (_t >> 24) & 0xFF; \
                                    } while(0)

#define SET_TIMESTAMP2(MSG,TIME)    do { \
                                        uint8_t* _p = _GET_PTR8((MSG),_TIMESTAMP2_OFFSET); \
                                        uint32_t _t = (TIME); \
                                        _p[0] = _t & 0xFF; \
                                        _p[1] = (_t >> 8) & 0xFF; \
                                        _p[2] = (_t >> 16) & 0xFF; \
                                        _p[3] = (_t >> 24) & 0xFF; \
                                    } while(0)

#define SET_NHOP(MSG,NHOP)          do { \
                                        uint8_t* _p = GET_TYPE(MSG) ? _GET_PTR8((MSG),_MEAS_NHOP_OFFSET) : \
                                        _GET_PTR8((MSG),_READ_NHOP_OFFSET); \
                                        *_p = (*_p & 0x0f) | (((NHOP) & 0x0f) << 4); \
                                    } while(0)

#define SET_HOP(MSG,INDEX,HOP)      do { \
                                        uint8_t* _p = GET_TYPE(MSG) ? _GET_PTR8((MSG),_MEAS_NHOP_OFFSET) : \
                                        _GET_PTR8((MSG),_READ_NHOP_OFFSET); \
                                        _p += (uint8_t)((INDEX) / 2); \
                                        *_p = (INDEX) & 0x1 ? ((*_p & 0xf0) | ((HOP) & 0x0f)) : \
                                        ((*_p & 0x0f) | (((HOP) & 0x0f) << 4)); \
                                    } while(0)

#define SET_SYS_CODE(MSG,CODE)      do { \
                                        uint8_t* _p = _GET_PTR8((MSG),(GET_LEN((MSG))-1)); \
                                        *_p = (CODE) & 0xff; \
                                    } while(0)                                    
                                                
#endif
#ifndef CAN_APP_FRAME_TYPES_H_
#define CAN_APP_FRAME_TYPES_H_

#include <stdint.h>

typedef union
{
    uint8_t frame[1];
    struct
    {
        uint8_t value;
    };
} CAN_CounterFrame_t;

typedef union
{
    uint8_t frame[2];
    struct
    {
        uint8_t node;
        uint8_t counter;
    };
} CAN_HeartbeatFrame_t;

typedef union
{
    uint8_t frame[12];
    struct
    {
        uint16_t speed_kph_x10;
        uint32_t timestamp;
        uint8_t sample;
    };
} CAN_SpeedFrame_t;

typedef union
{
    uint8_t frame[24];
    struct
    {
        int16_t temperature_c_x10;
        uint16_t humidity_x10;
        uint32_t timestamp;
        uint8_t sample;
    };
} CAN_ClimateFrame_t;

typedef union
{
    uint8_t frame[16];
    struct
    {
        uint8_t counter;
        uint32_t timestamp;
        uint8_t pattern[8];
    };
} CAN_DebugFrame_t;

#endif /* CAN_APP_FRAME_TYPES_H_ */

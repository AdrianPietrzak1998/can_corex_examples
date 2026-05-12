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
    uint8_t frame[2];
    struct
    {
        uint16_t speed_kph_x10;
    };
} CAN_SpeedFrame_t;

typedef union
{
    uint8_t frame[4];
    struct
    {
        int16_t temperature_c_x10;
        uint16_t humidity_x10;
    };
} CAN_ClimateFrame_t;

typedef union
{
    uint8_t frame[5];
    struct
    {
        uint8_t counter;
        uint32_t timestamp;
    };
} CAN_DebugFrame_t;

#endif /* CAN_APP_FRAME_TYPES_H_ */

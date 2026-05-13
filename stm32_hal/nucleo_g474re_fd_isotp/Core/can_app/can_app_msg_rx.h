#ifndef CAN_APP_MSG_RX_H_
#define CAN_APP_MSG_RX_H_

#include "can_corex.h"
#include "can_corex_isotp.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    CAN1_RX_HEARTBEAT = 0,
    CAN1_RX_EXT_CLIMATE,
    CAN1_RX_MISSING_COUNTER,
    CAN1_RX_ISOTP1_FC,
    CAN1_RX_ISOTP2_DATA,
    CAN1_RX_ISOTP3_FC,
    CAN1_RX_END
};

enum
{
    CAN2_RX_COUNTER = 0,
    CAN2_RX_EXT_SPEED,
    CAN2_RX_ISOTP1_DATA,
    CAN2_RX_ISOTP2_FC,
    CAN2_RX_ISOTP3_DATA,
    CAN2_RX_END
};

extern CCX_RX_table_t can1_rx_table[CAN1_RX_END];
extern CCX_RX_table_t can2_rx_table[CAN2_RX_END];

void can1_rx_unregistered(const CCX_instance_t *instance, CCX_message_t *msg);
void can2_rx_unregistered(const CCX_instance_t *instance, CCX_message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_MSG_RX_H_ */

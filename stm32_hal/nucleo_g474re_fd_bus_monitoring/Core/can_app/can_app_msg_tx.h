#ifndef CAN_APP_MSG_TX_H_
#define CAN_APP_MSG_TX_H_

#include "can_corex.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    CAN1_TX_COUNTER = 0,
    CAN1_TX_EXT_SPEED,
    CAN1_TX_UNREGISTERED,
    CAN1_TX_END
};

enum
{
    CAN2_TX_HEARTBEAT = 0,
    CAN2_TX_EXT_CLIMATE,
    CAN2_TX_END
};

extern CCX_TX_table_t can1_tx_table[CAN1_TX_END];
extern CCX_TX_table_t can2_tx_table[CAN2_TX_END];

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_MSG_TX_H_ */

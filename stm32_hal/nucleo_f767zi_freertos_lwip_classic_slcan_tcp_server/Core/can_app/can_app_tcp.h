#ifndef CAN_APP_TCP_H_
#define CAN_APP_TCP_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "can_corex.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_APP_TCP_MAX_ENDPOINTS 4U

typedef struct
{
    const char *Name;
    uint16_t Port;
} can_app_tcp_endpoint_config_t;

void can_app_tcp_init(const can_app_tcp_endpoint_config_t *configs, uint16_t count);
void can_app_tcp_start(void);
BaseType_t can_app_tcp_send(uint16_t endpoint_index, const CCX_message_t *msg);
BaseType_t can_app_tcp_pop_rx(uint16_t endpoint_index, CCX_message_t *msg);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_TCP_H_ */

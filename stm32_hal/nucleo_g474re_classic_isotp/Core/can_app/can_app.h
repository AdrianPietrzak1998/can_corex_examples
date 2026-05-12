#ifndef CAN_APP_H_
#define CAN_APP_H_

#include "can_corex_isotp.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void can_app_init(void);
void can_app_poll(void);
void can_app_send_isotp(const uint8_t *data, CCX_ISOTP_Length_t length);

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_H_ */

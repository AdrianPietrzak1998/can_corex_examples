#ifndef CAN_APP_FRAMES_API_H_
#define CAN_APP_FRAMES_API_H_

#include "can_app_frame_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern CAN_CounterFrame_t CAN1_Counter;
extern CAN_SpeedFrame_t CAN1_Speed;
extern CAN_DebugFrame_t CAN1_Unregistered;

extern CAN_HeartbeatFrame_t CAN2_Heartbeat;
extern CAN_ClimateFrame_t CAN2_Climate;

extern CAN_HeartbeatFrame_t CAN1_RxHeartbeat;
extern CAN_ClimateFrame_t CAN1_RxClimate;

extern CAN_CounterFrame_t CAN2_RxCounter;
extern CAN_SpeedFrame_t CAN2_RxSpeed;

#ifdef __cplusplus
}
#endif

#endif /* CAN_APP_FRAMES_API_H_ */

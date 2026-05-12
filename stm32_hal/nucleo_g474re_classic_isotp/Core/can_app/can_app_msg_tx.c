#include "can_app_msg_tx.h"
#include "can_app_frames_api.h"
#include "logs.h"
#include "main.h"

CAN_CounterFrame_t CAN1_Counter = {0};
CAN_SpeedFrame_t CAN1_Speed = {0};
CAN_DebugFrame_t CAN1_Unregistered = {0};

CAN_HeartbeatFrame_t CAN2_Heartbeat = {0};
CAN_ClimateFrame_t CAN2_Climate = {0};

static void can_app_copy(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    for (uint8_t i = 0U; i < len; i++)
    {
        dst[i] = src[i];
    }
}

static void can1_counter_tx_parser(const CCX_instance_t *instance, uint8_t *data_to_send, uint16_t slot,
                                   void *user_data)
{
    CAN_CounterFrame_t *frame = (CAN_CounterFrame_t *)user_data;

    (void)instance;
    (void)slot;

    frame->value++;
    can_app_copy(data_to_send, frame->frame, 1U);

    /* LOG_Printf("CAN1 TX counter std id=0x101 dlc=1 value=%u\r\n", (unsigned int)frame->value); */
}

static void can1_speed_tx_parser(const CCX_instance_t *instance, uint8_t *data_to_send, uint16_t slot,
                                 void *user_data)
{
    CAN_SpeedFrame_t *frame = (CAN_SpeedFrame_t *)user_data;

    (void)instance;
    (void)slot;

    frame->speed_kph_x10 = (uint16_t)(500U + (HAL_GetTick() % 800U));
    can_app_copy(data_to_send, frame->frame, 2U);

    /*
    LOG_Printf("CAN1 TX speed ext id=0x18FF0101 dlc=2 speed_x10=%u\r\n",
               (unsigned int)frame->speed_kph_x10);
    */
}

static void can1_unregistered_tx_parser(const CCX_instance_t *instance, uint8_t *data_to_send, uint16_t slot,
                                        void *user_data)
{
    CAN_DebugFrame_t *frame = (CAN_DebugFrame_t *)user_data;

    (void)instance;
    (void)slot;

    frame->counter++;
    frame->timestamp = HAL_GetTick();
    can_app_copy(data_to_send, frame->frame, 5U);

    /*
    LOG_Printf("CAN1 TX unregistered demo std id=0x555 dlc=5 counter=%u tick=%lu\r\n",
               (unsigned int)frame->counter,
               (unsigned long)frame->timestamp);
    */
}

static void can2_heartbeat_tx_parser(const CCX_instance_t *instance, uint8_t *data_to_send, uint16_t slot,
                                     void *user_data)
{
    CAN_HeartbeatFrame_t *frame = (CAN_HeartbeatFrame_t *)user_data;

    (void)instance;
    (void)slot;

    frame->node = 2U;
    frame->counter++;
    can_app_copy(data_to_send, frame->frame, 2U);

    /*
    LOG_Printf("CAN2 TX heartbeat std id=0x201 dlc=2 node=%u counter=%u\r\n",
               (unsigned int)frame->node,
               (unsigned int)frame->counter);
    */
}

static void can2_climate_tx_parser(const CCX_instance_t *instance, uint8_t *data_to_send, uint16_t slot,
                                   void *user_data)
{
    CAN_ClimateFrame_t *frame = (CAN_ClimateFrame_t *)user_data;

    (void)instance;
    (void)slot;

    frame->temperature_c_x10 = (int16_t)(220 + (int16_t)(HAL_GetTick() % 60U));
    frame->humidity_x10 = (uint16_t)(450U + (HAL_GetTick() % 120U));
    can_app_copy(data_to_send, frame->frame, 4U);

    /*
    LOG_Printf("CAN2 TX climate ext id=0x18FF0202 dlc=4 temp_x10=%d humidity_x10=%u\r\n",
               (int)frame->temperature_c_x10,
               (unsigned int)frame->humidity_x10);
    */
}

CCX_TX_table_t can1_tx_table[CAN1_TX_END] =
{
    [CAN1_TX_COUNTER] =
    {
        .ID = 0x101U,
        .Data = CAN1_Counter.frame,
        .DLC = 1,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN1_Counter,
        .SendFreq = 500U,
        .Parser = can1_counter_tx_parser
    },
    [CAN1_TX_EXT_SPEED] =
    {
        .ID = 0x18FF0101U,
        .Data = CAN1_Speed.frame,
        .DLC = 2,
        .IDE_flag = CCX_ID_EXTENDED,
        .UserData = &CAN1_Speed,
        .SendFreq = 1000U,
        .Parser = can1_speed_tx_parser
    },
    [CAN1_TX_UNREGISTERED] =
    {
        .ID = 0x555U,
        .Data = CAN1_Unregistered.frame,
        .DLC = 5,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN1_Unregistered,
        .SendFreq = 1500U,
        .Parser = can1_unregistered_tx_parser
    }
};

CCX_TX_table_t can2_tx_table[CAN2_TX_END] =
{
    [CAN2_TX_HEARTBEAT] =
    {
        .ID = 0x201U,
        .Data = CAN2_Heartbeat.frame,
        .DLC = 2,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN2_Heartbeat,
        .SendFreq = 700U,
        .Parser = can2_heartbeat_tx_parser
    },
    [CAN2_TX_EXT_CLIMATE] =
    {
        .ID = 0x18FF0202U,
        .Data = CAN2_Climate.frame,
        .DLC = 4,
        .IDE_flag = CCX_ID_EXTENDED,
        .UserData = &CAN2_Climate,
        .SendFreq = 1200U,
        .Parser = can2_climate_tx_parser
    }
};

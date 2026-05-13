#include "can_app_msg_rx.h"
#include "can_app_frames_api.h"
#include "logs.h"
#include <string.h>

CAN_HeartbeatFrame_t CAN1_RxHeartbeat = {0};
CAN_ClimateFrame_t CAN1_RxClimate = {0};

CAN_CounterFrame_t CAN2_RxCounter = {0};
CAN_SpeedFrame_t CAN2_RxSpeed = {0};

extern CCX_ISOTP_TX_t isotp_tx1;
extern CCX_ISOTP_RX_t isotp_rx1;

static void can_app_copy_rx(uint8_t *dst, uint8_t dst_size, const uint8_t *src, uint8_t len)
{
    if ((dst == NULL) || (src == NULL))
    {
        return;
    }

    memset(dst, 0, dst_size);

    if (len > dst_size)
    {
        len = dst_size;
    }

    memcpy(dst, src, len);
}

static void can1_heartbeat_rx_parser(const CCX_instance_t *instance, CCX_message_t *msg, uint16_t slot,
                                     void *user_data)
{
    CAN_HeartbeatFrame_t *frame = (CAN_HeartbeatFrame_t *)user_data;

    (void)instance;
    (void)slot;

    can_app_copy_rx(frame->frame, sizeof(frame->frame), msg->Data, msg->DLC);
    /*
    LOG_Printf("CAN1 RX heartbeat from CAN2 std id=0x%03lX dlc=%u node=%u counter=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)frame->node,
               (unsigned int)frame->counter);
    */
}

static void can1_climate_rx_parser(const CCX_instance_t *instance, CCX_message_t *msg, uint16_t slot,
                                   void *user_data)
{
    CAN_ClimateFrame_t *frame = (CAN_ClimateFrame_t *)user_data;

    (void)instance;
    (void)slot;

    can_app_copy_rx(frame->frame, sizeof(frame->frame), msg->Data, msg->DLC);
    /*
    LOG_Printf("CAN1 RX climate from CAN2 ext id=0x%08lX dlc=%u temp_x10=%d humidity_x10=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (int)frame->temperature_c_x10,
               (unsigned int)frame->humidity_x10);
    */
}

static void can2_counter_rx_parser(const CCX_instance_t *instance, CCX_message_t *msg, uint16_t slot,
                                   void *user_data)
{
    CAN_CounterFrame_t *frame = (CAN_CounterFrame_t *)user_data;

    (void)instance;
    (void)slot;

    can_app_copy_rx(frame->frame, sizeof(frame->frame), msg->Data, msg->DLC);
    /*
    LOG_Printf("CAN2 RX counter from CAN1 std id=0x%03lX dlc=%u value=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)frame->value);
    */
}

static void can2_speed_rx_parser(const CCX_instance_t *instance, CCX_message_t *msg, uint16_t slot,
                                 void *user_data)
{
    CAN_SpeedFrame_t *frame = (CAN_SpeedFrame_t *)user_data;

    (void)instance;
    (void)slot;

    can_app_copy_rx(frame->frame, sizeof(frame->frame), msg->Data, msg->DLC);
    /*
    LOG_Printf("CAN2 RX speed from CAN1 ext id=0x%08lX dlc=%u speed_x10=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)frame->speed_kph_x10);
    */
}

static void can1_missing_counter_rx_parser(const CCX_instance_t *instance, CCX_message_t *msg, uint16_t slot,
                                           void *user_data)
{
    CAN_CounterFrame_t *frame = (CAN_CounterFrame_t *)user_data;

    (void)instance;
    (void)slot;

    can_app_copy_rx(frame->frame, sizeof(frame->frame), msg->Data, msg->DLC);
    /*
    LOG_Printf("CAN1 RX missing-counter std id=0x%03lX dlc=%u value=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)frame->value);
    */
}

static void can1_missing_counter_timeout(CCX_instance_t *instance, uint16_t slot, void *user_data)
{
    (void)instance;
    (void)slot;
    (void)user_data;

    /* LOG_Printf("CAN1 RX timeout slot=%u waiting for std id=0x333 dlc=1\r\n", (unsigned int)slot); */
}

CCX_RX_table_t can1_rx_table[CAN1_RX_END] =
{
    [CAN1_RX_HEARTBEAT] =
    {
        .ID = 0x201U,
        .DLC = 2,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN1_RxHeartbeat,
        .TimeOut = 0U,
        .Parser = can1_heartbeat_rx_parser,
        .TimeoutCallback = NULL
    },
    [CAN1_RX_EXT_CLIMATE] =
    {
        .ID = 0x18FF0202U,
        .DLC = 4,
        .IDE_flag = CCX_ID_EXTENDED,
        .UserData = &CAN1_RxClimate,
        .TimeOut = 0U,
        .Parser = can1_climate_rx_parser,
        .TimeoutCallback = NULL
    },
    [CAN1_RX_MISSING_COUNTER] =
    {
        .ID = 0x333U,
        .DLC = 1,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN2_RxCounter,
        .TimeOut = 3000U,
        .Parser = can1_missing_counter_rx_parser,
        .TimeoutCallback = can1_missing_counter_timeout
    },
    [CAN1_RX_ISOTP_FC] = CCX_ISOTP_TX_FC_TABLE_ENTRY(&isotp_tx1, 0x701U, CCX_ID_STANDARD)
};

CCX_RX_table_t can2_rx_table[CAN2_RX_END] =
{
    [CAN2_RX_COUNTER] =
    {
        .ID = 0x101U,
        .DLC = 1,
        .IDE_flag = CCX_ID_STANDARD,
        .UserData = &CAN2_RxCounter,
        .TimeOut = 0U,
        .Parser = can2_counter_rx_parser,
        .TimeoutCallback = NULL
    },
    [CAN2_RX_EXT_SPEED] =
    {
        .ID = 0x18FF0101U,
        .DLC = 2,
        .IDE_flag = CCX_ID_EXTENDED,
        .UserData = &CAN2_RxSpeed,
        .TimeOut = 0U,
        .Parser = can2_speed_rx_parser,
        .TimeoutCallback = NULL
    },
    [CAN2_RX_ISOTP_DATA] = CCX_ISOTP_RX_TABLE_ENTRY(&isotp_rx1, 0x700U, CCX_ID_STANDARD)
};

void can1_rx_unregistered(const CCX_instance_t *instance, CCX_message_t *msg)
{
    (void)instance;

    if (msg == NULL)
    {
        return;
    }

    /*
    LOG_Printf("CAN1 RX unregistered id=0x%08lX dlc=%u ide=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)msg->IDE_flag);
    */
}

void can2_rx_unregistered(const CCX_instance_t *instance, CCX_message_t *msg)
{
    (void)instance;

    if (msg == NULL)
    {
        return;
    }

    /*
    LOG_Printf("CAN2 RX unregistered id=0x%08lX dlc=%u ide=%u\r\n",
               (unsigned long)msg->ID,
               (unsigned int)msg->DLC,
               (unsigned int)msg->IDE_flag);
    */
}

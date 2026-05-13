#include "can_app.h"
#include "can_app_frames_api.h"
#include "can_app_msg_rx.h"
#include "can_app_msg_tx.h"
#include "fdcan.h"
#include "logs.h"
#include "main.h"
#include <string.h>

static CCX_instance_t can1;
static CCX_instance_t can2;

extern __IO uint32_t uwTick;

static uint32_t can_app_hal_dlc(uint8_t dlc)
{
    switch (dlc)
    {
        case 0U:
            return FDCAN_DLC_BYTES_0;
        case 1U:
            return FDCAN_DLC_BYTES_1;
        case 2U:
            return FDCAN_DLC_BYTES_2;
        case 3U:
            return FDCAN_DLC_BYTES_3;
        case 4U:
            return FDCAN_DLC_BYTES_4;
        case 5U:
            return FDCAN_DLC_BYTES_5;
        case 6U:
            return FDCAN_DLC_BYTES_6;
        case 7U:
            return FDCAN_DLC_BYTES_7;
        case 8U:
            return FDCAN_DLC_BYTES_8;
        case 9U:
            return FDCAN_DLC_BYTES_12;
        case 10U:
            return FDCAN_DLC_BYTES_16;
        case 11U:
            return FDCAN_DLC_BYTES_20;
        case 12U:
            return FDCAN_DLC_BYTES_24;
        case 13U:
            return FDCAN_DLC_BYTES_32;
        case 14U:
            return FDCAN_DLC_BYTES_48;
        default:
            return FDCAN_DLC_BYTES_64;
    }
}

static const char *can_app_frame_format_name(const CCX_message_t *msg)
{
    if (msg->FrameFormat == CCX_FRAME_FORMAT_FD_BRS)
    {
        return "fd-brs";
    }

    if (msg->FrameFormat == CCX_FRAME_FORMAT_FD)
    {
        return "fd";
    }

    return "classic";
}

static CCX_BusIsFree_t can1_bus_check(const CCX_instance_t *instance)
{
    (void)instance;

    return (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0U) ? CCX_BUS_FREE : CCX_BUS_BUSY;
}

static CCX_BusIsFree_t can2_bus_check(const CCX_instance_t *instance)
{
    (void)instance;

    return (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2) > 0U) ? CCX_BUS_FREE : CCX_BUS_BUSY;
}

static void can_app_send(const char *can_name, FDCAN_HandleTypeDef *hfdcan, const CCX_message_t *msg)
{
    FDCAN_TxHeaderTypeDef tx_header = {0};

    if (msg == NULL)
    {
        return;
    }

    tx_header.Identifier = msg->ID;
    tx_header.IdType = (msg->IDE_flag == CCX_ID_EXTENDED) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = can_app_hal_dlc(msg->DLC);
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = (msg->FrameFormat == CCX_FRAME_FORMAT_FD_BRS) ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    tx_header.FDFormat = (msg->FrameFormat == CCX_FRAME_FORMAT_CLASSIC) ? FDCAN_CLASSIC_CAN : FDCAN_FD_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0U;

    if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &tx_header, (uint8_t *)msg->Data) != HAL_OK)
    {
        LOG_Printf("%s TX failed %s id=0x%03lX dlc=%u\r\n",
                   can_name,
                   can_app_frame_format_name(msg),
                   (unsigned long)msg->ID,
                   (unsigned int)msg->DLC);
    }
}

static void can1_send(const CCX_instance_t *instance, const CCX_message_t *msg)
{
    (void)instance;

    can_app_send("CAN1", &hfdcan1, msg);
}

static void can2_send(const CCX_instance_t *instance, const CCX_message_t *msg)
{
    (void)instance;

    can_app_send("CAN2", &hfdcan2, msg);
}

static void can_app_handle_rx(FDCAN_HandleTypeDef *hfdcan, uint32_t rx_location, CCX_instance_t *instance)
{
    FDCAN_RxHeaderTypeDef rx_header = {0};
    CCX_message_t msg = {0};

    if (HAL_FDCAN_GetRxMessage(hfdcan, rx_location, &rx_header, msg.Data) != HAL_OK)
    {
        return;
    }

    if (rx_header.RxFrameType != FDCAN_DATA_FRAME)
    {
        return;
    }

    msg.ID = rx_header.Identifier;
    msg.DLC = (uint8_t)(rx_header.DataLength & 0x0FU);
    msg.IDE_flag = (rx_header.IdType == FDCAN_EXTENDED_ID) ? CCX_ID_EXTENDED : CCX_ID_STANDARD;
    msg.ESI = (rx_header.ErrorStateIndicator == FDCAN_ESI_PASSIVE) ? 1U : 0U;
    msg.FrameFormat = (rx_header.FDFormat == FDCAN_FD_CAN) ? CCX_FRAME_FORMAT_FD : CCX_FRAME_FORMAT_CLASSIC;
    if ((rx_header.FDFormat == FDCAN_FD_CAN) && (rx_header.BitRateSwitch == FDCAN_BRS_ON))
    {
        msg.FrameFormat = CCX_FRAME_FORMAT_FD_BRS;
    }

    (void)CCX_RX_PushMsg(instance, &msg);
}

void can_app_init(void)
{
    CCX_tick_variable_register((CCX_TIME_t *)&uwTick);

    (void)CCX_Init(&can1,
                   can1_rx_table,
                   can1_tx_table,
                   CAN1_RX_END,
                   CAN1_TX_END,
                   can1_send,
                   can1_bus_check,
                   can1_rx_unregistered);

    (void)CCX_Init(&can2,
                   can2_rx_table,
                   can2_tx_table,
                   CAN2_RX_END,
                   CAN2_TX_END,
                   can2_send,
                   can2_bus_check,
                   can2_rx_unregistered);

    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1,
                                  FDCAN_ACCEPT_IN_RX_FIFO0,
                                  FDCAN_ACCEPT_IN_RX_FIFO0,
                                  FDCAN_FILTER_REMOTE,
                                  FDCAN_FILTER_REMOTE);

    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2,
                                  FDCAN_ACCEPT_IN_RX_FIFO1,
                                  FDCAN_ACCEPT_IN_RX_FIFO1,
                                  FDCAN_FILTER_REMOTE,
                                  FDCAN_FILTER_REMOTE);

    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0U);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0U);

    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);

    LOG_WriteLine("CAN app init");
}

void can_app_poll(void)
{
    (void)CCX_Poll(&can1);
    (void)CCX_Poll(&can2);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == 0U)
    {
        return;
    }

    if (hfdcan->Instance == FDCAN1)
    {
        can_app_handle_rx(hfdcan, FDCAN_RX_FIFO0, &can1);
    }
}

void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
    if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) == 0U)
    {
        return;
    }

    if (hfdcan->Instance == FDCAN1)
    {
        return;
    }

    if (hfdcan->Instance == FDCAN2)
    {
        can_app_handle_rx(hfdcan, FDCAN_RX_FIFO1, &can2);
    }
}

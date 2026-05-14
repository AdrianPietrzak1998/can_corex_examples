#include "can_app.h"
#include "can_app_frames_api.h"
#include "can_app_msg_rx.h"
#include "can_app_msg_tx.h"
#include "can.h"
#include "logs.h"
#include "main.h"

static CCX_instance_t can1;
static CCX_instance_t can2;

extern __IO uint32_t uwTick;

static uint32_t can_app_hal_dlc(const CCX_message_t *msg)
{
    if (msg->DLC > 8U)
    {
        return 8U;
    }

    return msg->DLC;
}

static CCX_BusIsFree_t can1_bus_check(const CCX_instance_t *instance)
{
    (void)instance;

    return (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) > 0U) ? CCX_BUS_FREE : CCX_BUS_BUSY;
}

static CCX_BusIsFree_t can2_bus_check(const CCX_instance_t *instance)
{
    (void)instance;

    return (HAL_CAN_GetTxMailboxesFreeLevel(&hcan2) > 0U) ? CCX_BUS_FREE : CCX_BUS_BUSY;
}

static void can_app_send(const char *can_name, CAN_HandleTypeDef *hcan, const CCX_message_t *msg)
{
    CAN_TxHeaderTypeDef tx_header = {0};
    uint32_t tx_mailbox = 0U;

    if (msg == NULL)
    {
        return;
    }

    tx_header.IDE = (msg->IDE_flag == CCX_ID_EXTENDED) ? CAN_ID_EXT : CAN_ID_STD;
    if (tx_header.IDE == CAN_ID_EXT)
    {
        tx_header.ExtId = msg->ID;
    }
    else
    {
        tx_header.StdId = msg->ID;
    }
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = can_app_hal_dlc(msg);
    tx_header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(hcan, &tx_header, (uint8_t *)msg->Data, &tx_mailbox) != HAL_OK)
    {
        LOG_Printf("%s TX failed id=0x%03lX dlc=%u\r\n", can_name, (unsigned long)msg->ID, (unsigned int)msg->DLC);
    }
}

static void can1_send(const CCX_instance_t *instance, const CCX_message_t *msg)
{
    (void)instance;

    can_app_send("CAN1", &hcan1, msg);
}

static void can2_send(const CCX_instance_t *instance, const CCX_message_t *msg)
{
    (void)instance;

    can_app_send("CAN2", &hcan2, msg);
}

static void can_app_handle_rx(CAN_HandleTypeDef *hcan, uint32_t rx_fifo, CCX_instance_t *instance)
{
    CAN_RxHeaderTypeDef rx_header = {0};
    CCX_message_t msg = {0};

    if (HAL_CAN_GetRxMessage(hcan, rx_fifo, &rx_header, msg.Data) != HAL_OK)
    {
        return;
    }

    if (rx_header.RTR != CAN_RTR_DATA)
    {
        return;
    }

    msg.ID = (rx_header.IDE == CAN_ID_EXT) ? rx_header.ExtId : rx_header.StdId;
    msg.DLC = (uint8_t)rx_header.DLC;
    msg.IDE_flag = (rx_header.IDE == CAN_ID_EXT) ? CCX_ID_EXTENDED : CCX_ID_STANDARD;

    (void)CCX_RX_PushMsg(instance, &msg);
}

static void can_app_config_filter(CAN_HandleTypeDef *hcan, uint32_t filter_bank, uint32_t fifo_assignment)
{
    CAN_FilterTypeDef filter = {0};

    filter.FilterBank = filter_bank;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000U;
    filter.FilterIdLow = 0x0000U;
    filter.FilterMaskIdHigh = 0x0000U;
    filter.FilterMaskIdLow = 0x0000U;
    filter.FilterFIFOAssignment = fifo_assignment;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK)
    {
        LOG_Printf("%s filter config failed\r\n", (hcan->Instance == CAN1) ? "CAN1" : "CAN2");
    }
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

    can_app_config_filter(&hcan1, 0U, CAN_RX_FIFO0);
    can_app_config_filter(&hcan2, 14U, CAN_RX_FIFO1);

    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO1_MSG_PENDING);

    HAL_CAN_Start(&hcan1);
    HAL_CAN_Start(&hcan2);

    LOG_WriteLine("CAN app init");
}

void can_app_poll(void)
{
    (void)CCX_Poll(&can1);
    (void)CCX_Poll(&can2);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0U)
        {
            can_app_handle_rx(hcan, CAN_RX_FIFO0, &can1);
        }
    }
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN2)
    {
        while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO1) > 0U)
        {
            can_app_handle_rx(hcan, CAN_RX_FIFO1, &can2);
        }
    }
}

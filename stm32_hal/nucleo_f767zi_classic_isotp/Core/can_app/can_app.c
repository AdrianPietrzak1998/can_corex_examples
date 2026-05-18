#include "can_app.h"
#include "can_app_frames_api.h"
#include "can_app_msg_rx.h"
#include "can_app_msg_tx.h"
#include "can.h"
#include "logs.h"
#include "main.h"
#include "tim.h"

CCX_instance_t can1;
CCX_instance_t can2;
CCX_ISOTP_TX_t isotp_tx1;
CCX_ISOTP_RX_t isotp_rx1;
CCX_ISOTP_TX_t isotp_tx2;
CCX_ISOTP_RX_t isotp_rx2;

static uint8_t isotp_rx1_buffer[CCX_ISOTP_MAX_CLASSIC_DATA_SIZE];
static uint8_t isotp_rx2_buffer[CCX_ISOTP_MAX_CLASSIC_DATA_SIZE];

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

static void can_app_isotp_tx_complete(CCX_ISOTP_TX_t *instance, void *user_data)
{
    const char *name = (const char *)user_data;

    (void)instance;

    LOG_Printf("%s complete\r\n", name);
}

static void can_app_isotp_tx_error(CCX_ISOTP_TX_t *instance, CCX_ISOTP_Status_t error, void *user_data)
{
    const char *name = (const char *)user_data;

    (void)instance;

    LOG_Printf("%s error=%d\r\n", name, (int)error);
}

static void can_app_isotp_rx_complete(CCX_ISOTP_RX_t *instance, const uint8_t *data, CCX_ISOTP_Length_t length,
                                      void *user_data)
{
    const char *name = (const char *)user_data;

    (void)instance;

    LOG_Printf("%s complete len=%u first='%c' last='%c'\r\n",
               name,
               (unsigned int)length,
               (length > 0U) ? (char)data[0] : '-',
               (length > 0U) ? (char)data[length - 1U] : '-');
}

static void can_app_isotp_rx_error(CCX_ISOTP_RX_t *instance, CCX_ISOTP_Status_t error, void *user_data)
{
    const char *name = (const char *)user_data;

    (void)instance;

    LOG_Printf("%s error=%d\r\n", name, (int)error);
}

static void can_app_isotp_init(void)
{
    CCX_ISOTP_TX_Config_t tx_cfg;
    CCX_ISOTP_RX_Config_t rx_cfg;
    CCX_ISOTP_Status_t status;

    CCX_ISOTP_TX_Config_Init(&tx_cfg, 0x700U, CCX_ID_STANDARD, 1000U, 1000U, CCX_ISOTP_NO_PADDING,
                             "isotp_tx1", can_app_isotp_tx_complete, can_app_isotp_tx_error);
    status = CCX_ISOTP_TX_Init(&isotp_tx1, &can1, &tx_cfg);
    LOG_Printf("isotp_tx1 init status=%d\r\n", (int)status);

    CCX_ISOTP_RX_Config_Init(&rx_cfg, 0x701U, CCX_ID_STANDARD, 0U, CCX_ISOTP_STMIN_500US, 1000U,
                             isotp_rx1_buffer, sizeof(isotp_rx1_buffer), CCX_ISOTP_NO_PADDING, 0U,
                             "isotp_rx1", NULL, can_app_isotp_rx_complete, NULL, can_app_isotp_rx_error);
    status = CCX_ISOTP_RX_Init(&isotp_rx1, &can2, &rx_cfg);
    LOG_Printf("isotp_rx1 init status=%d\r\n", (int)status);

    CCX_ISOTP_TX_Config_Init(&tx_cfg, 0x710U, CCX_ID_STANDARD, 1000U, 1000U, CCX_ISOTP_NO_PADDING,
                             "isotp_tx2", can_app_isotp_tx_complete, can_app_isotp_tx_error);
    status = CCX_ISOTP_TX_Init(&isotp_tx2, &can2, &tx_cfg);
    LOG_Printf("isotp_tx2 init status=%d\r\n", (int)status);

    CCX_ISOTP_RX_Config_Init(&rx_cfg, 0x711U, CCX_ID_STANDARD, 0U, CCX_ISOTP_STMIN_500US, 1000U,
                             isotp_rx2_buffer, sizeof(isotp_rx2_buffer), CCX_ISOTP_NO_PADDING, 0U,
                             "isotp_rx2", NULL, can_app_isotp_rx_complete, NULL, can_app_isotp_rx_error);
    status = CCX_ISOTP_RX_Init(&isotp_rx2, &can1, &rx_cfg);
    LOG_Printf("isotp_rx2 init status=%d\r\n", (int)status);
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
#if CCX_ENABLE_CANFD
    msg.ESI = 0U;
    msg.FrameFormat = CCX_FRAME_FORMAT_CLASSIC;
#endif

    (void)CCX_RX_PushMsg(instance, &msg);
}

static void can_app_handle_rx_fifo(CAN_HandleTypeDef *hcan, uint32_t rx_fifo)
{
    CCX_instance_t *instance = NULL;

    if (hcan->Instance == CAN1)
    {
        instance = &can1;
    }
    else if (hcan->Instance == CAN2)
    {
        instance = &can2;
    }

    if (instance == NULL)
    {
        return;
    }

    while (HAL_CAN_GetRxFifoFillLevel(hcan, rx_fifo) > 0U)
    {
        can_app_handle_rx(hcan, rx_fifo, instance);
    }
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
    CCX_high_res_tick_variable_register((uint16_t *)&(htim13.Instance->CNT));
    HAL_TIM_Base_Start(&htim13);

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

    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING);
    HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING);

    HAL_CAN_Start(&hcan1);
    HAL_CAN_Start(&hcan2);

    can_app_isotp_init();

    LOG_WriteLine("CAN app init");
}

void can_app_poll(void)
{
    (void)CCX_Poll(&can1);
    (void)CCX_Poll(&can2);
    CCX_ISOTP_TX_Poll(&isotp_tx1);
    CCX_ISOTP_RX_Poll(&isotp_rx1);
    CCX_ISOTP_TX_Poll(&isotp_tx2);
    CCX_ISOTP_RX_Poll(&isotp_rx2);
}

void can_app_send_isotp_tx1(const uint8_t *data, CCX_ISOTP_Length_t length)
{
    CCX_ISOTP_Status_t status;

    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    status = CCX_ISOTP_Transmit(&isotp_tx1, data, length);
    if (status == CCX_ISOTP_OK)
    {
        LOG_Printf("isotp_tx1->isotp_rx1 start len=%u\r\n", (unsigned int)length);
    }
    else
    {
        LOG_Printf("isotp_tx1 start error=%d\r\n", (int)status);
    }
}

void can_app_send_isotp_tx2(const uint8_t *data, CCX_ISOTP_Length_t length)
{
    CCX_ISOTP_Status_t status;

    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    status = CCX_ISOTP_Transmit(&isotp_tx2, data, length);
    if (status == CCX_ISOTP_OK)
    {
        LOG_Printf("isotp_tx2->isotp_rx2 start len=%u\r\n", (unsigned int)length);
    }
    else
    {
        LOG_Printf("isotp_tx2 start error=%d\r\n", (int)status);
    }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    can_app_handle_rx_fifo(hcan, CAN_RX_FIFO0);
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    can_app_handle_rx_fifo(hcan, CAN_RX_FIFO1);
}

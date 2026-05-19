#include "can_app_tcp.h"
#include "can_corex_utils.h"
#include "logs.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/tcp.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define CAN_APP_TCP_QUEUE_LEN 128U
#define CAN_APP_TCP_STACK_SIZE 768U
#define CAN_APP_TCP_RECV_TIMEOUT_MS 10U
#define CAN_APP_TCP_LISTEN_BACKLOG 1U
#define CAN_APP_TCP_TX_BATCH_SIZE 512U

static uint8_t can_app_tcp_is_fatal_err(err_t err)
{
    return ((err == ERR_CLSD) || (err == ERR_RST) || (err == ERR_ABRT)) ? 1U : 0U;
}

typedef struct
{
    can_app_tcp_endpoint_config_t Config;
    QueueHandle_t TxQueue;
    QueueHandle_t RxQueue;
    StaticQueue_t TxQueueControl;
    StaticQueue_t RxQueueControl;
    uint8_t TxQueueStorage[CAN_APP_TCP_QUEUE_LEN * sizeof(CCX_message_t)];
    uint8_t RxQueueStorage[CAN_APP_TCP_QUEUE_LEN * sizeof(CCX_message_t)];
    StaticTask_t TaskControl;
    StackType_t TaskStack[CAN_APP_TCP_STACK_SIZE];
    TaskHandle_t TaskHandle;
    volatile uint8_t ClientActive;
    uint8_t Started;
} can_app_tcp_endpoint_t;

static can_app_tcp_endpoint_t endpoints[CAN_APP_TCP_MAX_ENDPOINTS];
static uint16_t endpoint_count = 0U;

static void can_app_tcp_close_conn(struct netconn *conn)
{
    if (conn != NULL)
    {
        (void)netconn_close(conn);
        netconn_delete(conn);
    }
}

static err_t can_app_tcp_drain_tx(can_app_tcp_endpoint_t *endpoint, struct netconn *client)
{
    CCX_message_t msg;
    char batch[CAN_APP_TCP_TX_BATCH_SIZE];
    size_t batch_len = 0U;
    err_t err = ERR_OK;

    while (xQueuePeek(endpoint->TxQueue, &msg, 0U) == pdPASS)
    {
        char line[CCX_SLCAN_CLASSIC_MAX_LINE_LEN];
        size_t line_len;

        if (CCX_SLCAN_Format(&msg, line, sizeof(line)) != CCX_UTILS_OK)
        {
            (void)xQueueReceive(endpoint->TxQueue, &msg, 0U);
            continue;
        }

        line_len = strlen(line);

        if ((batch_len > 0U) && ((batch_len + line_len) > sizeof(batch)))
        {
            err = netconn_write(client, batch, batch_len, NETCONN_COPY);
            if (err != ERR_OK)
            {
                break;
            }
            batch_len = 0U;
        }

        if (line_len <= sizeof(batch))
        {
            memcpy(&batch[batch_len], line, line_len);
            batch_len += line_len;
        }
        else
        {
            err = netconn_write(client, line, line_len, NETCONN_COPY);
            if (err != ERR_OK)
            {
                break;
            }
        }

        (void)xQueueReceive(endpoint->TxQueue, &msg, 0U);
    }

    if ((err == ERR_OK) && (batch_len > 0U))
    {
        err = netconn_write(client, batch, batch_len, NETCONN_COPY);
    }

    if (can_app_tcp_is_fatal_err(err) != 0U)
    {
        return err;
    }
    if (err != ERR_OK)
    {
        LOG_Printf("%s TCP write transient err=%d\r\n", endpoint->Config.Name, (int)err);
    }

    return ERR_OK;
}

static void can_app_tcp_handle_line(can_app_tcp_endpoint_t *endpoint, char *line)
{
    CCX_message_t msg;

    if (CCX_SLCAN_Parse(line, &msg) == CCX_UTILS_OK)
    {
        (void)xQueueSend(endpoint->RxQueue, &msg, 0U);
    }
}

static void can_app_tcp_handle_rx_chunk(can_app_tcp_endpoint_t *endpoint,
                                        const char *data,
                                        uint16_t len,
                                        char *line,
                                        uint16_t *line_len,
                                        uint8_t *discarding)
{
    for (uint16_t i = 0U; i < len; i++)
    {
        char ch = data[i];

        if ((ch == '\r') || (ch == '\n'))
        {
            if (*discarding != 0U)
            {
                *discarding = 0U;
                *line_len = 0U;
                continue;
            }

            if (*line_len > 0U)
            {
                line[*line_len] = '\0';
                can_app_tcp_handle_line(endpoint, line);
                *line_len = 0U;
            }
            continue;
        }

        if (*discarding != 0U)
        {
            continue;
        }

        if (*line_len >= (uint16_t)(CCX_SLCAN_CLASSIC_MAX_LINE_LEN - 1U))
        {
            *discarding = 1U;
            *line_len = 0U;
            continue;
        }

        line[*line_len] = ch;
        (*line_len)++;
    }
}

static void can_app_tcp_client_loop(can_app_tcp_endpoint_t *endpoint, struct netconn *client)
{
    char line[CCX_SLCAN_CLASSIC_MAX_LINE_LEN];
    uint16_t line_len = 0U;
    uint8_t discarding = 0U;

    netconn_set_recvtimeout(client, CAN_APP_TCP_RECV_TIMEOUT_MS);

    for (;;)
    {
        struct netbuf *buf = NULL;
        err_t err;

        if (can_app_tcp_drain_tx(endpoint, client) != ERR_OK)
        {
            break;
        }

        err = netconn_recv(client, &buf);
        if (err == ERR_TIMEOUT)
        {
            continue;
        }
        if (can_app_tcp_is_fatal_err(err) != 0U)
        {
            break;
        }
        if (err != ERR_OK)
        {
            LOG_Printf("%s TCP recv transient err=%d\r\n", endpoint->Config.Name, (int)err);
            continue;
        }

        do
        {
            void *data = NULL;
            uint16_t len = 0U;

            netbuf_data(buf, &data, &len);
            if ((data != NULL) && (len > 0U))
            {
                can_app_tcp_handle_rx_chunk(endpoint, (const char *)data, len, line, &line_len, &discarding);
            }
        } while (netbuf_next(buf) >= 0);

        netbuf_delete(buf);
    }
}

static void can_app_tcp_task(void *argument)
{
    can_app_tcp_endpoint_t *endpoint = (can_app_tcp_endpoint_t *)argument;

    for (;;)
    {
        struct netconn *listener = netconn_new(NETCONN_TCP);

        if (listener == NULL)
        {
            vTaskDelay(pdMS_TO_TICKS(1000U));
            continue;
        }

        if (netconn_bind(listener, IP_ADDR_ANY, endpoint->Config.Port) != ERR_OK)
        {
            LOG_Printf("%s TCP bind failed port=%u\r\n", endpoint->Config.Name, endpoint->Config.Port);
            netconn_delete(listener);
            vTaskDelay(pdMS_TO_TICKS(1000U));
            continue;
        }

        if (netconn_listen_with_backlog(listener, CAN_APP_TCP_LISTEN_BACKLOG) != ERR_OK)
        {
            LOG_Printf("%s TCP listen failed port=%u\r\n", endpoint->Config.Name, endpoint->Config.Port);
            netconn_delete(listener);
            vTaskDelay(pdMS_TO_TICKS(1000U));
            continue;
        }

        LOG_Printf("%s TCP listen port=%u\r\n", endpoint->Config.Name, endpoint->Config.Port);

        for (;;)
        {
            struct netconn *client = NULL;

            if (netconn_accept(listener, &client) == ERR_OK)
            {
                LOG_Printf("%s TCP client connected\r\n", endpoint->Config.Name);
                if ((client != NULL) && (client->pcb.tcp != NULL))
                {
                    tcp_nagle_disable(client->pcb.tcp);
                }
                (void)xQueueReset(endpoint->TxQueue);
                endpoint->ClientActive = 1U;
                can_app_tcp_client_loop(endpoint, client);
                endpoint->ClientActive = 0U;
                can_app_tcp_close_conn(client);
                (void)xQueueReset(endpoint->TxQueue);
                LOG_Printf("%s TCP client disconnected\r\n", endpoint->Config.Name);
            }
        }
    }
}

void can_app_tcp_init(const can_app_tcp_endpoint_config_t *configs, uint16_t count)
{
    if ((configs == NULL) || (count > CAN_APP_TCP_MAX_ENDPOINTS))
    {
        return;
    }

    endpoint_count = count;
    for (uint16_t i = 0U; i < endpoint_count; i++)
    {
        endpoints[i].Config = configs[i];
        endpoints[i].TxQueue = xQueueCreateStatic(CAN_APP_TCP_QUEUE_LEN,
                                                  sizeof(CCX_message_t),
                                                  endpoints[i].TxQueueStorage,
                                                  &endpoints[i].TxQueueControl);
        endpoints[i].RxQueue = xQueueCreateStatic(CAN_APP_TCP_QUEUE_LEN,
                                                  sizeof(CCX_message_t),
                                                  endpoints[i].RxQueueStorage,
                                                  &endpoints[i].RxQueueControl);
        endpoints[i].ClientActive = 0U;
        endpoints[i].Started = 0U;
    }
}

void can_app_tcp_start(void)
{
    for (uint16_t i = 0U; i < endpoint_count; i++)
    {
        char task_name[configMAX_TASK_NAME_LEN];

        if ((endpoints[i].Started != 0U) || (endpoints[i].TxQueue == NULL) || (endpoints[i].RxQueue == NULL))
        {
            continue;
        }

        (void)snprintf(task_name, sizeof(task_name), "%s_tcp", endpoints[i].Config.Name);
        endpoints[i].TaskHandle = xTaskCreateStatic(can_app_tcp_task,
                                                    task_name,
                                                    CAN_APP_TCP_STACK_SIZE,
                                                    &endpoints[i],
                                                    tskIDLE_PRIORITY + 1U,
                                                    endpoints[i].TaskStack,
                                                    &endpoints[i].TaskControl);
        if (endpoints[i].TaskHandle != NULL)
        {
            endpoints[i].Started = 1U;
        }
    }
}

BaseType_t can_app_tcp_send(uint16_t endpoint_index, const CCX_message_t *msg)
{
    if ((endpoint_index >= endpoint_count) || (msg == NULL) || (endpoints[endpoint_index].TxQueue == NULL))
    {
        return pdFAIL;
    }

    if (endpoints[endpoint_index].ClientActive == 0U)
    {
        return pdFAIL;
    }

    if (xQueueSend(endpoints[endpoint_index].TxQueue, msg, 0U) == pdPASS)
    {
        return pdPASS;
    }

    CCX_message_t dropped_msg;
    (void)xQueueReceive(endpoints[endpoint_index].TxQueue, &dropped_msg, 0U);
    return xQueueSend(endpoints[endpoint_index].TxQueue, msg, 0U);
}

BaseType_t can_app_tcp_pop_rx(uint16_t endpoint_index, CCX_message_t *msg)
{
    if ((endpoint_index >= endpoint_count) || (msg == NULL) || (endpoints[endpoint_index].RxQueue == NULL))
    {
        return pdFAIL;
    }

    return xQueueReceive(endpoints[endpoint_index].RxQueue, msg, 0U);
}

#include "can_handler.h"
#include "usb_cdc_handler.h"
#include <string.h>
#include <stdio.h>

extern CAN_HandleTypeDef hcan;

/* ── Module state ────────────────────────────────────────────────────────── */
static struct {
    bool      tx_active;
    bool      listen_mode;
    uint32_t  id;
    uint8_t   dlc;
    uint8_t   data[8];
    uint32_t  rate_hz;
    uint32_t  last_tx_tick;
    CAN_Stats_t stats;
} ctx = {
    .tx_active   = false,
    .listen_mode = false,
    .id          = CAN_DEFAULT_ID,
    .dlc         = CAN_DEFAULT_DLC,
    .data        = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00},
    .rate_hz     = CAN_DEFAULT_RATE_HZ,
};

/* ── Init: configure accept-all filter on FIFO1, start CAN ──────────────── */
void CAN_Handler_Init(void)
{
    CAN_FilterTypeDef f = {
        .FilterBank           = 0,
        .FilterMode           = CAN_FILTERMODE_IDMASK,
        .FilterScale          = CAN_FILTERSCALE_32BIT,
        .FilterIdHigh         = 0x0000U,
        .FilterIdLow          = 0x0000U,
        .FilterMaskIdHigh     = 0x0000U,   /* mask = 0 → accept all */
        .FilterMaskIdLow      = 0x0000U,
        .FilterFIFOAssignment = CAN_RX_FIFO1,   /* FIFO1 has its own IRQ */
        .FilterActivation     = ENABLE,
        .SlaveStartFilterBank = 14,
    };
    HAL_CAN_ConfigFilter(&hcan, &f);
    HAL_CAN_Start(&hcan);
    HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
}

/* ── Periodic TX – called from main loop ─────────────────────────────────── */
void CAN_Handler_Process(void)
{
    if (!ctx.tx_active) return;

    uint32_t now         = HAL_GetTick();
    uint32_t interval_ms = 1000U / ctx.rate_hz;

    if ((now - ctx.last_tx_tick) < interval_ms) return;
    ctx.last_tx_tick = now;

    CAN_TxHeaderTypeDef hdr = {
        .StdId              = ctx.id,
        .ExtId              = 0U,
        .IDE                = CAN_ID_STD,
        .RTR                = CAN_RTR_DATA,
        .DLC                = ctx.dlc,
        .TransmitGlobalTime = DISABLE,
    };

    uint32_t mailbox;
    if (HAL_CAN_AddTxMessage(&hcan, &hdr, ctx.data, &mailbox) == HAL_OK)
        ctx.stats.tx_count++;
    else
        ctx.stats.tx_errors++;
}

/* ── RX FIFO1 callback (from CAN1_RX1_IRQn, no USB conflict) ────────────── */
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan_ptr)
{
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8];

    while (HAL_CAN_GetRxMessage(hcan_ptr, CAN_RX_FIFO1, &hdr, data) == HAL_OK)
    {
        ctx.stats.rx_count++;

        if (!ctx.listen_mode) continue;

        char buf[72];
        int  n = snprintf(buf, sizeof(buf),
                          "\r\nRX  ID:0x%03lX  DLC:%u  DATA:",
                          (unsigned long)hdr.StdId, (unsigned)hdr.DLC);
        for (uint8_t i = 0; i < hdr.DLC && n < (int)sizeof(buf) - 4; i++)
            n += snprintf(buf + n, sizeof(buf) - (size_t)n, " %02X", data[i]);

        buf[n++] = '\r'; buf[n++] = '\n';
        CDC_Handler_Write((uint8_t *)buf, (uint16_t)n);
    }
}

/* ── Control ─────────────────────────────────────────────────────────────── */
void CAN_StartTx(void)    { ctx.tx_active   = true;  }
void CAN_StopTx(void)     { ctx.tx_active   = false; }
bool CAN_IsTxActive(void) { return ctx.tx_active; }

void CAN_SetId(uint32_t id)
{
    ctx.id = id & 0x7FFU;   /* clamp to 11-bit standard ID */
}

void CAN_SetDLC(uint8_t dlc)
{
    ctx.dlc = (dlc > 8U) ? 8U : dlc;
}

void CAN_SetData(const uint8_t *data, uint8_t len)
{
    uint8_t n = (len > 8U) ? 8U : len;
    memcpy(ctx.data, data, n);
}

void CAN_SetRateHz(uint32_t hz)
{
    ctx.rate_hz = (hz == 0U) ? 1U : hz;
}

uint32_t CAN_GetId(void)    { return ctx.id; }
uint8_t  CAN_GetDLC(void)   { return ctx.dlc; }
uint32_t CAN_GetRateHz(void){ return ctx.rate_hz; }

void CAN_GetData(uint8_t *out, uint8_t *len)
{
    memcpy(out, ctx.data, ctx.dlc);
    *len = ctx.dlc;
}

const CAN_Stats_t *CAN_GetStats(void)  { return &ctx.stats; }
void CAN_ResetStats(void)              { memset(&ctx.stats, 0, sizeof(ctx.stats)); }

void CAN_SetListenMode(bool en) { ctx.listen_mode = en; }
bool CAN_IsListenMode(void)     { return ctx.listen_mode; }

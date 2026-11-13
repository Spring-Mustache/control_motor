#include "m10_eth_bringup.h"
#include "m10_eth_protocol.h"
#include <string.h>

/*
 * M10 Ethernet-only bring-up. STM32F767ZI SRAM2 is 0x2007C000..0x2007FFFF,
 * 16 KiB; linker places all ETH descriptors and these buffers into SRAM2.
 * MPU marks the entire 16 KiB normal/shareable/noncacheable before HAL ETH
 * init, so DMA and Cortex-M7 see coherent RX/TX contents.
 *
 * HAL_ETH_Start() uses polling; no ETH IRQ or FreeRTOS is required.
 * Poll once per 5-ms baseline main loop, process at most one RX frame.
 * No I2C, UART, PS2, actuator ownership, PWM or motor access in this module.
 */
#define M10_RX_POOL ((uint32_t)ETH_RX_DESC_CNT + 2U)
#define M10_RX_BUF_SIZE 1536U
#define M10_PHY_ADDR 0U
#define M10_PHY_REG_BCR 0U
#define M10_PHY_REG_BSR 1U
#define M10_PHY_REG_STATUS 31U

static ETH_HandleTypeDef *s_eth;
static uint32_t s_last_phy_ms;
static uint32_t s_link_mode;
static uint8_t s_tx_ok = 1U;
static uint8_t s_allocated[M10_RX_POOL];
static ETH_BufferTypeDef s_rx_nodes[M10_RX_POOL];

/* All DMA-accessible payloads reside in dedicated linker SRAM2 output. */
static uint8_t s_rx_pool[M10_RX_POOL][M10_RX_BUF_SIZE]
    __attribute__((section(".m10_eth_dma"), aligned(32)));
static uint8_t s_tx_frame[M10_ETH_FRAME_CAP]
    __attribute__((section(".m10_eth_dma"), aligned(32)));

volatile uint32_t m10_eth_phy_link;
volatile uint32_t m10_eth_mac_started;
volatile uint32_t m10_eth_phy_status;
volatile uint32_t m10_eth_rx_frames;
volatile uint32_t m10_eth_tx_frames;
volatile uint32_t m10_eth_phy_errors;
volatile uint32_t m10_eth_tx_errors;
volatile uint32_t m10_eth_rx_drops;

void M10_ETH_ConfigMPU(void)
{
  MPU_Region_InitTypeDef mpu = {0};
  HAL_MPU_Disable();
  mpu.Enable = MPU_REGION_ENABLE;
  mpu.Number = MPU_REGION_NUMBER0;
  mpu.BaseAddress = 0x2007C000UL;
  mpu.Size = MPU_REGION_SIZE_16KB;
  mpu.SubRegionDisable = 0U;
  mpu.TypeExtField = MPU_TEX_LEVEL1; /* normal non-cacheable */
  mpu.AccessPermission = MPU_REGION_FULL_ACCESS;
  mpu.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  mpu.IsShareable = MPU_ACCESS_SHAREABLE;
  mpu.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  mpu.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  HAL_MPU_ConfigRegion(&mpu);
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

void M10_ETH_Init(ETH_HandleTypeDef *eth)
{
  if ((eth != NULL) && (HAL_ETH_GetState(eth) == HAL_ETH_STATE_READY))
  {
    s_eth = eth;
    /* HAL_ETH_Init does not program the MDIO CSR divider in this HAL.
     * At 96-MHz HCLK, choose the appropriate <=2.5-MHz MDIO clock first.
     */
    HAL_ETH_SetMDIOClockRange(eth);
    s_last_phy_ms = HAL_GetTick() - 1000U;
  }
  else
    m10_eth_phy_errors++;
}

/* HAL callbacks: only fixed SRAM2 buffers, no malloc and no blocking I/O.
 * Two spare RX slots let HAL replenish descriptors before handing a received
 * packet to M10_ETH_Poll(). The packet is returned promptly after processing.
 */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
  uint32_t i;
  if (buff == NULL)
    return;
  *buff = NULL;
  for (i = 0U; i < M10_RX_POOL; i++)
  {
    if (s_allocated[i] == 0U)
    {
      s_allocated[i] = 1U;
      s_rx_nodes[i].buffer = s_rx_pool[i];
      s_rx_nodes[i].len = 0U;
      s_rx_nodes[i].next = NULL;
      *buff = s_rx_pool[i];
      return;
    }
  }
  m10_eth_rx_drops++;
}

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd,
                            uint8_t *buff, uint16_t Length)
{
  uint32_t i;
  if ((pStart == NULL) || (pEnd == NULL))
    return;
  for (i = 0U; i < M10_RX_POOL; i++)
  {
    if (buff == s_rx_pool[i])
    {
      ETH_BufferTypeDef *node = &s_rx_nodes[i];
      node->len = Length;
      node->next = NULL;
      if (*pStart == NULL)
        *pStart = node;
      else
        ((ETH_BufferTypeDef *)*pEnd)->next = node;
      *pEnd = node;
      return;
    }
  }
  m10_eth_rx_drops++;
}

/* Polling transmit is synchronous; HAL never needs to own heap TX buffers. */
void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
  (void)buff;
}

/* Bound an MDIO read to 2 ms (HAL_ETH_ReadPHYRegister has a much longer
 * timeout, unsuitable for the existing 5-ms robot control loop).
 * Preserve the clock divider set by HAL and address PHY 0 selected in IOC.
 */
static uint8_t read_phy(uint32_t index, uint32_t *value)
{
  uint32_t reg;
  uint32_t start_ms;
  if ((s_eth == NULL) || (value == NULL))
    return 0U;
  reg = s_eth->Instance->MACMIIAR;
  reg &= ~(ETH_MACMIIAR_PA | ETH_MACMIIAR_MR |
           ETH_MACMIIAR_MW | ETH_MACMIIAR_MB);
  reg |= ((M10_PHY_ADDR << 11U) & ETH_MACMIIAR_PA);
  reg |= ((index << 6U) & ETH_MACMIIAR_MR);
  reg |= ETH_MACMIIAR_MB;
  s_eth->Instance->MACMIIAR = reg;

  start_ms = HAL_GetTick();
  while ((s_eth->Instance->MACMIIAR & ETH_MACMIIAR_MB) != 0U)
  {
    if ((uint32_t)(HAL_GetTick() - start_ms) >= 2U)
      return 0U;
  }
  *value = s_eth->Instance->MACMIIDR & 0xffffU;
  return 1U;
}

static void monitor_phy(void)
{
  uint32_t bsr;
  uint32_t bcr;
  uint32_t stat;
  uint32_t mode;
  uint32_t speed;
  uint32_t duplex;
  ETH_MACConfigTypeDef mac;

  /* BSR LINK is latched low: read it twice. */
  if ((read_phy(M10_PHY_REG_BSR, &bsr) == 0U) ||
      (read_phy(M10_PHY_REG_BSR, &bsr) == 0U))
  {
    m10_eth_phy_errors++;
    return;
  }
  if ((bsr & 0x0004U) == 0U)
  {
    m10_eth_phy_link = 0U;
    if (m10_eth_mac_started != 0U)
    {
      (void)HAL_ETH_Stop(s_eth);
      m10_eth_mac_started = 0U;
    }
    return;
  }
  m10_eth_phy_link = 1U;

  if (read_phy(M10_PHY_REG_BCR, &bcr) == 0U)
  {
    m10_eth_phy_errors++;
    return;
  }

  if ((bcr & 0x1000U) != 0U) /* PHY auto negotiation enabled */
  {
    if ((read_phy(M10_PHY_REG_STATUS, &stat) == 0U) ||
        ((stat & 0x1000U) == 0U))
      return; /* wait until 100/10 and duplex have resolved */
    m10_eth_phy_status = stat;
    mode = stat & 0x001cU;
    switch (mode)
    {
      case 0x0018U: /* LAN8742 100 Mbit full duplex */
        speed = ETH_SPEED_100M; duplex = ETH_FULLDUPLEX_MODE; break;
      case 0x0008U:
        speed = ETH_SPEED_100M; duplex = ETH_HALFDUPLEX_MODE; break;
      case 0x0014U:
        speed = ETH_SPEED_10M; duplex = ETH_FULLDUPLEX_MODE; break;
      case 0x0004U:
        speed = ETH_SPEED_10M; duplex = ETH_HALFDUPLEX_MODE; break;
      default:
        m10_eth_phy_errors++;
        return;
    }
  }
  else
  {
    speed = ((bcr & 0x2000U) != 0U) ? ETH_SPEED_100M : ETH_SPEED_10M;
    duplex = ((bcr & 0x0100U) != 0U) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
    mode = 0x100U | ((bcr >> 8U) & 0x21U);
    m10_eth_phy_status = bcr;
  }
  if ((m10_eth_mac_started != 0U) && (s_link_mode == mode))
    return;

  if (m10_eth_mac_started != 0U)
  {
    (void)HAL_ETH_Stop(s_eth);
    m10_eth_mac_started = 0U;
  }
  if ((HAL_ETH_GetState(s_eth) != HAL_ETH_STATE_READY) ||
      (HAL_ETH_GetMACConfig(s_eth, &mac) != HAL_OK))
  {
    m10_eth_phy_errors++;
    return;
  }
  mac.Speed = speed;
  mac.DuplexMode = duplex;
  /* Checksums are computed in m10_eth_protocol.c; no DMA checksum offload. */
  mac.ChecksumOffload = DISABLE;
  if ((HAL_ETH_SetMACConfig(s_eth, &mac) != HAL_OK) ||
      (HAL_ETH_Start(s_eth) != HAL_OK))
  {
    m10_eth_phy_errors++;
    return;
  }
  s_link_mode = mode;
  s_tx_ok = 1U;
  m10_eth_mac_started = 1U;
}

static void release_rx(ETH_BufferTypeDef *node)
{
  while (node != NULL)
  {
    ETH_BufferTypeDef *next = node->next;
    uint32_t i;
    for (i = 0U; i < M10_RX_POOL; i++)
    {
      if (&s_rx_nodes[i] == node)
      {
        s_allocated[i] = 0U;
        break;
      }
    }
    node = next;
  }
}

void M10_ETH_Poll(void)
{
  uint32_t now;
  void *packet = NULL;
  ETH_BufferTypeDef *rx;
  uint16_t reply_len;
  ETH_BufferTypeDef txnode;
  ETH_TxPacketConfigTypeDef txcfg;

  if (s_eth == NULL)
    return;
  now = HAL_GetTick();
  if ((uint32_t)(now - s_last_phy_ms) >= 1000U)
  {
    s_last_phy_ms = now;
    monitor_phy();
  }
  if (m10_eth_mac_started == 0U)
    return;

  /* A single bounded packet per control-loop iteration; no network commands. */
  if ((HAL_ETH_ReadData(s_eth, &packet) != HAL_OK) || (packet == NULL))
    return;
  rx = (ETH_BufferTypeDef *)packet;
  m10_eth_rx_frames++;
  if ((rx->next != NULL) || (rx->buffer == NULL) ||
      (rx->len > M10_ETH_FRAME_CAP))
  {
    m10_eth_rx_drops++;
    release_rx(rx);
    return;
  }

  reply_len = M10_ETH_MakeReply(rx->buffer, (uint16_t)rx->len,
                               s_tx_frame, M10_ETH_FRAME_CAP);
  release_rx(rx);
  if ((reply_len == 0U) || (s_tx_ok == 0U))
    return;

  txnode.buffer = s_tx_frame;
  txnode.len = reply_len;
  txnode.next = NULL;
  memset(&txcfg, 0, sizeof(txcfg));
  txcfg.TxBuffer = &txnode;
  txcfg.Length = reply_len;
  /* HAL starts TX descriptors with checksum insertion enabled. Explicitly
   * bypass DMA checksumming because M10 builds IPv4/ICMP/UDP checksums in SW.
   */
  txcfg.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD |
                     ETH_TX_PACKETS_FEATURES_CSUM;
  txcfg.ChecksumCtrl = ETH_CHECKSUM_DISABLE;
  txcfg.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  if (HAL_ETH_Transmit(s_eth, &txcfg, 2U) == HAL_OK)
    m10_eth_tx_frames++;
  else
  {
    /* The DMA may still own the TX payload after a timeout.
     * Never overwrite it until the MAC is stopped/restarted.
     */
    s_tx_ok = 0U;
    m10_eth_tx_errors++;
  }
}

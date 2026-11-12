#ifndef M10_ETH_BRINGUP_H
#define M10_ETH_BRINGUP_H

#include "stm32f7xx_hal.h"
#include <stdint.h>

/* Bench Ethernet service. No motor API or actuator command exists here. */
void M10_ETH_ConfigMPU(void);
void M10_ETH_Init(ETH_HandleTypeDef *eth);
void M10_ETH_Poll(void);

/* Read-only Live Expressions diagnostics; neither FLASHED nor PASS inferred. */
extern volatile uint32_t m10_eth_phy_link;
extern volatile uint32_t m10_eth_mac_started;
extern volatile uint32_t m10_eth_phy_status;
extern volatile uint32_t m10_eth_rx_frames;
extern volatile uint32_t m10_eth_tx_frames;
extern volatile uint32_t m10_eth_phy_errors;
extern volatile uint32_t m10_eth_tx_errors;
extern volatile uint32_t m10_eth_rx_drops;

#endif

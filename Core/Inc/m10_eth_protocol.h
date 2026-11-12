#ifndef M10_ETH_PROTOCOL_H
#define M10_ETH_PROTOCOL_H

#include <stdint.h>

/*
 * M10 bench-only Ethernet endpoint, deliberately NOT a motor command protocol.
 * IPv4 192.168.10.20/24, ICMP Echo, ARP, and UDP echo at port 5005.
 * No TCP, DHCP, fragmentation, broadcast UDP or control over actuators.
 */
#define M10_ETH_UDP_PORT       5005U
#define M10_ETH_MAX_UDP_DATA   512U
#define M10_ETH_FRAME_CAP      1536U

extern const uint8_t m10_eth_ip4[4];
extern const uint8_t m10_eth_mac[6];

/* Pure, bounded packet handler suitable for native host regression tests.
 * Returns number of response bytes (excluding hardware Ethernet FCS),
 * or zero for unsupported/invalid packets. Input and output must not alias.
 */
uint16_t M10_ETH_MakeReply(const uint8_t *frame, uint16_t frame_len,
                           uint8_t *reply, uint16_t reply_cap);

#endif

#include "m10_eth_protocol.h"
#include <string.h>

/* Locally administered, unicast MAC. Use only on the isolated bench link. */
const uint8_t m10_eth_mac[6] = {0x02U, 0x6dU, 0x31U, 0x30U, 0x00U, 0x01U};
const uint8_t m10_eth_ip4[4] = {192U, 168U, 10U, 20U};

static uint16_t get16(const uint8_t *p)
{
  return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void put16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}

/* Internet checksum, streaming-compatible with a previously even-length sum. */
static uint32_t add_words(uint32_t sum, const uint8_t *p, uint16_t len)
{
  uint16_t i;
  for (i = 0U; (uint16_t)(i + 1U) < len; i = (uint16_t)(i + 2U))
    sum += ((uint32_t)p[i] << 8) | p[i + 1U];
  if (i < len)
    sum += (uint32_t)p[i] << 8;
  return sum;
}

static uint16_t checksum(uint32_t sum)
{
  while ((sum >> 16) != 0U)
    sum = (sum & 0xffffU) + (sum >> 16);
  return (uint16_t)(~sum);
}

static uint16_t udp_checksum(const uint8_t *ip, const uint8_t *udp,
                             uint16_t udp_len)
{
  uint32_t sum = add_words(0U, ip + 12, 8U);
  sum += 17U;
  sum += udp_len;
  sum = add_words(sum, udp, udp_len);
  return checksum(sum);
}

static uint8_t for_this_mac(const uint8_t *frame)
{
  static const uint8_t broadcast[6] =
      {0xffU, 0xffU, 0xffU, 0xffU, 0xffU, 0xffU};
  return (uint8_t)((memcmp(frame, m10_eth_mac, 6U) == 0) ||
                   (memcmp(frame, broadcast, 6U) == 0));
}

static void reply_eth_header(const uint8_t *frame, uint8_t *reply,
                             uint16_t ethertype)
{
  memcpy(reply, frame + 6U, 6U);
  memcpy(reply + 6U, m10_eth_mac, 6U);
  put16(reply + 12U, ethertype);
}

static uint16_t reply_arp(const uint8_t *rx, uint16_t n, uint8_t *tx,
                          uint16_t cap)
{
  const uint8_t *arp;
  uint8_t *a;
  if ((n < 42U) || (cap < 42U))
    return 0U;
  arp = rx + 14U;
  if ((get16(arp) != 1U) || (get16(arp + 2U) != 0x0800U) ||
      (arp[4] != 6U) || (arp[5] != 4U) ||
      (get16(arp + 6U) != 1U) ||
      (memcmp(arp + 24U, m10_eth_ip4, 4U) != 0))
    return 0U;
  reply_eth_header(rx, tx, 0x0806U);
  a = tx + 14U;
  put16(a, 1U);
  put16(a + 2U, 0x0800U);
  a[4] = 6U; a[5] = 4U;
  put16(a + 6U, 2U);
  memcpy(a + 8U, m10_eth_mac, 6U);
  memcpy(a + 14U, m10_eth_ip4, 4U);
  memcpy(a + 18U, arp + 8U, 6U);
  memcpy(a + 24U, arp + 14U, 4U);
  return 42U;
}

static uint16_t reply_ipv4(const uint8_t *rx, uint16_t n, uint8_t *tx,
                           uint16_t cap)
{
  const uint8_t *ip;
  const uint8_t *payload;
  uint16_t iplen;
  uint16_t fragment;
  uint16_t paylen;
  uint16_t replylen;
  uint8_t *outip;
  uint8_t *outpay;

  if ((n < 34U) || (cap < 34U))
    return 0U;
  ip = rx + 14U;
  if ((ip[0] != 0x45U) || (ip[1] != 0U))
    return 0U; /* Fixed IPv4 header, no IP options or DSCP. */
  iplen = get16(ip + 2U);
  fragment = get16(ip + 6U);
  if ((iplen < 20U) || ((uint32_t)iplen > (uint32_t)n - 14U) ||
      ((uint32_t)iplen > (uint32_t)cap - 14U) ||
      ((fragment & 0x3fffU) != 0U) ||
      (memcmp(ip + 16U, m10_eth_ip4, 4U) != 0) ||
      (checksum(add_words(0U, ip, 20U)) != 0U))
    return 0U;

  paylen = (uint16_t)(iplen - 20U);
  payload = ip + 20U;
  if (ip[9] == 1U) /* ICMP echo */
  {
    if ((paylen < 8U) || (payload[0] != 8U) || (payload[1] != 0U) ||
        (checksum(add_words(0U, payload, paylen)) != 0U))
      return 0U;
    replylen = (uint16_t)(14U + iplen);
    reply_eth_header(rx, tx, 0x0800U);
    memcpy(tx + 14U, ip, iplen);
    outip = tx + 14U;
    outpay = outip + 20U;
    outpay[0] = 0U;
    put16(outpay + 2U, 0U);
    put16(outpay + 2U, checksum(add_words(0U, outpay, paylen)));
  }
  else if (ip[9] == 17U) /* UDP echo, no commands */
  {
    uint16_t udp_len;
    uint16_t udp_sum;
    if (paylen < 8U)
      return 0U;
    udp_len = get16(payload + 4U);
    if ((get16(payload + 2U) != M10_ETH_UDP_PORT) ||
        (udp_len < 8U) || (udp_len > paylen) ||
        ((uint16_t)(udp_len - 8U) > M10_ETH_MAX_UDP_DATA))
      return 0U;
    udp_sum = get16(payload + 6U);
    if ((udp_sum != 0U) && (udp_checksum(ip, payload, udp_len) != 0U))
      return 0U;

    /* Accept no padding/IP data past the UDP datagram as application bytes. */
    replylen = (uint16_t)(14U + 20U + udp_len);
    reply_eth_header(rx, tx, 0x0800U);
    memcpy(tx + 14U, ip, 20U + udp_len);
    outip = tx + 14U;
    outpay = outip + 20U;
    put16(outip + 2U, (uint16_t)(20U + udp_len));
    put16(outpay, M10_ETH_UDP_PORT);
    put16(outpay + 2U, get16(payload));
    put16(outpay + 6U, 0U);
  }
  else
    return 0U;

  /* Route the response directly back to the sender's MAC and source IP.
   * No gateway, routing, ARP cache or motor control is implemented.
   */
  memcpy(outip + 16U, ip + 12U, 4U);
  memcpy(outip + 12U, m10_eth_ip4, 4U);
  outip[8] = 64U;
  put16(outip + 10U, 0U);
  put16(outip + 10U, checksum(add_words(0U, outip, 20U)));
  if (ip[9] == 17U)
  {
    uint16_t c = udp_checksum(outip, outpay, get16(outpay + 4U));
    put16(outpay + 6U, c ? c : 0xffffU);
  }
  return replylen;
}

uint16_t M10_ETH_MakeReply(const uint8_t *frame, uint16_t frame_len,
                           uint8_t *reply, uint16_t reply_cap)
{
  if ((frame == NULL) || (reply == NULL) ||
      (frame_len < 14U) || (reply_cap < 42U) ||
      (frame_len > M10_ETH_FRAME_CAP) ||
      (reply_cap > M10_ETH_FRAME_CAP) ||
      (for_this_mac(frame) == 0U))
    return 0U;

  switch (get16(frame + 12U))
  {
    case 0x0806U:
      return reply_arp(frame, frame_len, reply, reply_cap);
    case 0x0800U:
      return reply_ipv4(frame, frame_len, reply, reply_cap);
    default:
      return 0U;
  }
}

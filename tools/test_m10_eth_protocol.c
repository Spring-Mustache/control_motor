#include "m10_eth_protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t rx[256];
static uint8_t tx[256];

static uint16_t be16(const uint8_t *p)
{
  return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}
static void put16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}
static uint32_t add(uint32_t sum, const uint8_t *p, unsigned n)
{
  unsigned i;
  for (i = 0U; i + 1U < n; i += 2U)
    sum += ((uint32_t)p[i] << 8) | p[i+1U];
  if (i < n) sum += (uint32_t)p[i] << 8;
  return sum;
}
static uint16_t ck(uint32_t sum)
{
  while (sum >> 16) sum = (sum & 65535U) + (sum >> 16);
  return (uint16_t)~sum;
}

static void ethernet(const uint8_t *dst)
{
  const uint8_t source[6] = {0x02,0x11,0x22,0x33,0x44,0x55};
  memset(rx, 0, sizeof(rx));
  memcpy(rx, dst, 6);
  memcpy(rx+6, source, 6);
}
static void ipv4(uint8_t protocol, uint16_t payload_len)
{
  uint8_t *ip = rx+14;
  put16(rx+12, 0x0800);
  ip[0] = 0x45;
  put16(ip+2, (uint16_t)(20 + payload_len));
  put16(ip+4, 0x0123);
  put16(ip+6, 0x4000);
  ip[8] = 64;
  ip[9] = protocol;
  ip[12]=192; ip[13]=168; ip[14]=10; ip[15]=10;
  memcpy(ip+16, m10_eth_ip4, 4);
  put16(ip+10, 0);
  put16(ip+10, ck(add(0, ip, 20)));
}
static void arp_test(void)
{
  static const uint8_t bcast[6] = {255,255,255,255,255,255};
  uint8_t *a;
  uint16_t n;
  ethernet(bcast);
  put16(rx+12, 0x0806);
  a=rx+14;
  put16(a,1);
  put16(a+2,0x0800);
  a[4]=6; a[5]=4;
  put16(a+6,1);
  memcpy(a+8,rx+6,6);
  a[14]=192; a[15]=168; a[16]=10; a[17]=10;
  memcpy(a+24,m10_eth_ip4,4);
  n=M10_ETH_MakeReply(rx,42,tx,sizeof(tx));
  assert(n==42);
  assert(be16(tx+12)==0x0806);
  assert(be16(tx+14+6)==2);
  assert(memcmp(tx+14+14,m10_eth_ip4,4)==0);
  assert(memcmp(tx+14+8,m10_eth_mac,6)==0);
  assert(memcmp(tx+14+24,a+14,4)==0);

  a[27]=21; /* wrong requested IP */
  assert(M10_ETH_MakeReply(rx,42,tx,sizeof(tx))==0);
}

static void ping_test(void)
{
  uint8_t *icmp;
  uint16_t n;
  ethernet(m10_eth_mac);
  ipv4(1,16);
  icmp=rx+34;
  icmp[0]=8; icmp[1]=0;
  put16(icmp+4,0x2468);
  put16(icmp+6,7);
  memcpy(icmp+8,"M10-ping",8);
  put16(icmp+2,ck(add(0,icmp,16)));
  n=M10_ETH_MakeReply(rx,50,tx,sizeof(tx));
  assert(n==50);
  assert(tx[34]==0 && tx[35]==0);
  assert(ck(add(0,tx+34,16))==0);
  assert(ck(add(0,tx+14,20))==0);
  assert(memcmp(tx+26,m10_eth_ip4,4)==0);
  assert(memcmp(tx+30,rx+26,4)==0);
  assert(memcmp(tx+42,"M10-ping",8)==0);

  icmp[11]^=1;
  assert(M10_ETH_MakeReply(rx,50,tx,sizeof(tx))==0); /* checksum */
}

static void udp_test(void)
{
  uint8_t *ip;
  uint8_t *udp;
  uint16_t n;
  uint32_t sum;
  ethernet(m10_eth_mac);
  ipv4(17,18);
  ip=rx+14; udp=rx+34;
  put16(udp,40000);
  put16(udp+2,M10_ETH_UDP_PORT);
  put16(udp+4,18);
  memcpy(udp+8,"M10-ECHO!!",10);
  sum=add(0,ip+12,8)+17+18;
  put16(udp+6,ck(add(sum,udp,18)));
  n=M10_ETH_MakeReply(rx,52,tx,sizeof(tx));
  assert(n==52);
  assert(be16(tx+34)==M10_ETH_UDP_PORT);
  assert(be16(tx+36)==40000);
  assert(memcmp(tx+42,udp+8,10)==0);
  assert(ck(add(0,tx+14,20))==0);
  sum=add(0,tx+26,8)+17+18;
  assert(ck(add(sum,tx+34,18))==0);

  put16(udp+2,6000); /* unknown port cannot control robot */
  assert(M10_ETH_MakeReply(rx,52,tx,sizeof(tx))==0);
  put16(udp+2,M10_ETH_UDP_PORT);
  rx[14+6]=0x20; /* Fragment offset unsupported */
  assert(M10_ETH_MakeReply(rx,52,tx,sizeof(tx))==0);
}

int main(void)
{
  arp_test();
  ping_test();
  udp_test();
  puts("PASS M10: ARP / ICMP checksum / UDP echo and negative cases");
  return 0;
}

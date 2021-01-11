/*
 * Copyright 2013 Google Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
/*
 * Author: ncardwell@google.com (Neal Cardwell)
 *
 * Interface and type declarations for a representation of TCP/IP packets.
 * Packets are represented in their wire format.
 */

#ifndef __PACKET_H__
#define __PACKET_H__

#include "types.h"

#include <sys/time.h>
#include "assert.h"
//#include "gre.h"
#include "header.h"
#include "icmp.h"
#include "icmpv6.h"
#include "ip.h"
#include "ipv6.h"
#include "tcp.h"
#include "udp.h"
#include "unaligned.h"

/* The data offset field is 4 bits, and specifies the length of the TCP header,
 * including options, in 32-bit words.
 */
#define MAX_TCP_HEADER_BYTES (15*4)

#define MAX_TCP_DATAGRAM_BYTES (64*1024)	/* for sanity-checking */
#define MAX_UDP_DATAGRAM_BYTES (64*1024)	/* for sanity-checking */

/* We allow reading pretty big packets, since some interface MTUs can
 * be pretty big (the Linux loopback MTU, for example, is typically
 * around 16KB).
 */
static const int PACKET_READ_BYTES = 64 * 1024;

/* Maximum number of headers. */
#define PACKET_MAX_HEADERS	6

/* Maximum number of bytes of headers. */
#define PACKET_MAX_HEADER_BYTES	256

/* TCP/UDP/IPv4 packet, including IPv4 header, TCP/UDP header, and data. There
 * may also be a link layer header between the 'buffer' and 'ip'
 * pointers, but we typically ignore that. The 'buffer_bytes' field
 * gives the total space in the buffer, which may be bigger than the
 * actual amount occupied by the packet data.
 */
struct packet {
  union {
      struct {
          struct ipv4 ip_hdr;
          union {
              struct tcp tcp_hdr;
              struct udp udp_hdr;
              struct icmpv4 icmp_hdr;
          } tcp_udp;
      } v4;
      struct {
          struct ipv6 ip_hdr;
          union {
              struct tcp tcp_hdr;
              struct udp udp_hdr;
              struct icmpv6 icmp_hdr;
          } tcp_udp;
      } v6;

  } hdr;
};

typedef enum {
    ip_ipv4 = 4,
    ip_ipv6 = 6
} ip_type_t;


#endif /* __PACKET_H__ */

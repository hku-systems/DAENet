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
 * Implementation for a module to checksum TCP/IP packets.
 */

#include "packet_checksum.h"

#include "checksum.h"
#include "icmp.h"
#include "icmpv6.h"
#include "ip.h"
#include "ipv6.h"
#include "tcp.h"
#include "packet.h"

static void checksum_ipv4_packet(struct packet *packet)
{
	struct ipv4 *ipv4 = &packet->hdr.v4.ip_hdr;

	/* Fill in IPv4 header checksum. */
	ipv4->check = 0;
	ipv4->check = ipv4_checksum(ipv4, ipv4_header_len(ipv4));
//	assert(packet->ip_bytes >= ntohs(ipv4->tot_len));

	/* Find the length of layer 4 header, options, and payload. */
	const int l4_bytes = ntohs(ipv4->tot_len) - ipv4_header_len(ipv4);
	assert(l4_bytes > 0);

	/* Fill in IPv4-based layer 4 checksum. */
	if (ipv4->protocol == IPPROTO_TCP) {
		struct tcp *tcp = (struct tcp*)((unsigned char*)packet + ipv4_header_len(ipv4));
		tcp->check = 0;
		tcp->check = tcp_udp_v4_checksum(ipv4->src_ip,
						 ipv4->dst_ip,
						 IPPROTO_TCP, tcp, l4_bytes);
	} else if (ipv4->protocol == IPPROTO_UDP) {
		struct udp *udp = (struct udp*)((unsigned char*)packet + ipv4_header_len(ipv4));
		udp->check = 0;
		udp->check = tcp_udp_v4_checksum(ipv4->src_ip,
						 ipv4->dst_ip,
						 IPPROTO_UDP, udp, l4_bytes);
	} else if (ipv4->protocol == IPPROTO_ICMP) {
		struct icmpv4 *icmpv4 = (struct icmpv4*)((unsigned char*)packet + ipv4_header_len(ipv4));
		icmpv4->checksum = 0;
		icmpv4->checksum = ipv4_checksum(icmpv4, l4_bytes);
	} else {
		assert(!"not TCP or ICMP");
	}
}

static void checksum_ipv6_packet(struct packet *packet)
{
	struct ipv6 *ipv6 = &packet->hdr.v6.ip_hdr;

	/* IPv6 has no header checksum. */
	/* For now we do not support IPv6 extension headers. */
//	assert(packet->ip_bytes >= sizeof(*ipv6) + ntohs(ipv6->payload_len));

	/* Find the length of layer 4 header, options, and payload. */
	const int l4_bytes = ntohs(ipv6->payload_len);
	assert(l4_bytes > 0);

	/* Fill in IPv6-based layer 4 checksum. */
	if (ipv6->next_header == IPPROTO_TCP) {
		struct tcp *tcp = &packet->hdr.v6.tcp_udp.tcp_hdr;
		tcp->check = 0;
		tcp->check = tcp_udp_v6_checksum(&ipv6->src_ip,
						 &ipv6->dst_ip,
						 IPPROTO_TCP, tcp, l4_bytes);
	} else if (ipv6->next_header == IPPROTO_UDP) {
		struct udp *udp = &packet->hdr.v6.tcp_udp.udp_hdr;
		udp->check = 0;
		udp->check = tcp_udp_v6_checksum(&ipv6->src_ip,
						 &ipv6->dst_ip,
						 IPPROTO_UDP, udp, l4_bytes);
	} else if (ipv6->next_header == IPPROTO_ICMPV6) {
		/* IPv6 ICMP has a pseudo-header checksum, like TCP. */
		struct icmpv6 *icmpv6 = &packet->hdr.v6.tcp_udp.icmp_hdr;
		icmpv6->checksum = 0;
		icmpv6->checksum =
			tcp_udp_v6_checksum(&ipv6->src_ip,
					    &ipv6->dst_ip,
					    IPPROTO_ICMPV6, icmpv6, l4_bytes);
	} else {
		assert(!"not TCP or ICMP");
	}
}

void checksum_packet(struct packet *packet)
{
	if (packet->hdr.v4.ip_hdr.version == ip_ipv4)
		return checksum_ipv4_packet(packet);
	else if (packet->hdr.v4.ip_hdr.version == ip_ipv6)
		return checksum_ipv6_packet(packet);
	else
		assert(!"bad ip version");
}

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
 * Declarations for types used widely throughout this tool.
 */

#ifndef __TYPES_H__
#define __TYPES_H__

/* The files that include this file need to include it before
 * including stdio.h in order to ensure that the declaration of
 * asprintf is visible. So our .h files attempt to follow a
 * convention of including types.h first, before everything else.
 */
#define _GNU_SOURCE		/* for asprintf */
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "assert.h"
#include "platforms.h"

/* We use some unconventional formatting here to avoid checkpatch.pl
 * warnings about having to use the __packed macro, which is typically
 * only available in the kernel.
 */
#ifndef __packed
#define __packed __attribute__ ((packed))
#endif

/* We use kernel-style names for standard integer types. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;


#define ARRAY_SIZE(array_name)  (sizeof(array_name) / sizeof(array_name[0]))

/* Most functions in this codebase return one of these two values to let the
 * caller know whether there was a problem.
 */
enum status_t {
	STATUS_OK  = 0,
	STATUS_ERR = -1,
	STATUS_WARN = -2,	/* a non-fatal error or warning */
};

/* The directions in which a packet may flow. */
enum direction_t {
	DIRECTION_INVALID,
	DIRECTION_INBOUND,	/* packet coming into the kernel under test */
	DIRECTION_OUTBOUND,	/* packet leaving the kernel under test */
};

/* Return the opposite direction. */
static inline enum direction_t reverse_direction(enum direction_t direction)
{
	if (direction == DIRECTION_INBOUND)
		return DIRECTION_OUTBOUND;
	else if (direction == DIRECTION_OUTBOUND)
		return DIRECTION_INBOUND;
	else
		assert(!"bad direction");
}

/* How to treat the TOS byte of a packet. */
enum tos_chk_t {
	TOS_CHECK_NONE,
	TOS_CHECK_ECN,
	TOS_CHECK_TOS,
};

struct tos_spec {
	enum tos_chk_t check;
	u8 value;
};

/* IPv4 ECN treatment for a packet. */
enum ip_ecn_t {
	ECN_NONE,
	ECN_ECT0,
	ECN_ECT1,
	ECN_CE,
	ECN_ECT01,
};

#define TTL_CHECK_NONE 255

struct ip_info {
	struct tos_spec tos;
	u32 flow_label;
	u8 ttl;
};

/* Length of output buffer for inet_ntop, plus prefix length (e.g. "/128"). */
#define ADDR_STR_LEN ((INET_ADDRSTRLEN + INET6_ADDRSTRLEN)+5)

/* Flavors of IP versions we support. */
enum ip_version_t {
	/* Native IPv4, with AF_INET sockets and IPv4 addresses. */
	IP_VERSION_4		= 0,

	/* IPv4-Mapped IPv6 addresses: (see RFC 4291 sec. 2.5.5.2) we
	 * use AF_INET6 sockets but all connect(), bind(), and
	 * accept() calls are for IPv4 addresses mapped into IPv6
	 * address space. So all interface addresses and packets on
	 * the wire are IPv4.
	 */
	IP_VERSION_4_MAPPED_6	= 1,

	/* Native IPv6, with AF_INET6 sockets and IPv6 addresses. */
	IP_VERSION_6		= 2,
};

extern struct in_addr in4addr_any;


#endif /* __TYPES_H__ */

/**
 * @file raw.c
 * @note Copyright (C) 2012 Richard Cochran <richardcochran@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <errno.h>
#include <fcntl.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>

#include "contain.h"
#include "ether.h"
#include "print.h"
#include "raw.h"
#include "sk.h"
#include "transport_private.h"

struct raw {
	struct transport t;
	struct eth_addr ptp_addr;
	struct eth_addr p2p_addr;
	int vlan;
};

#define OP_AND  (BPF_ALU | BPF_AND | BPF_K)
#define OP_JEQ  (BPF_JMP | BPF_JEQ | BPF_K)
#define OP_JUN  (BPF_JMP | BPF_JA)
#define OP_LDB  (BPF_LD  | BPF_B   | BPF_ABS)
#define OP_LDH  (BPF_LD  | BPF_H   | BPF_ABS)
#define OP_RETK (BPF_RET | BPF_K)

#define PTP_GEN_BIT 0x08 /* indicates general message, if set in message type */

#define N_RAW_FILTER    12
#define RAW_FILTER_TEST 9

static struct sock_filter raw_filter[N_RAW_FILTER] = {
	{OP_LDH,  0, 0, OFF_ETYPE   },
	{OP_JEQ,  0, 4, ETH_P_8021Q          }, /*f goto non-vlan block*/
	{OP_LDH,  0, 0, OFF_ETYPE + 4        },
	{OP_JEQ,  0, 7, ETH_P_1588           }, /*f goto reject*/
	{OP_LDB,  0, 0, ETH_HLEN + VLAN_HLEN },
	{OP_JUN,  0, 0, 2                    }, /*goto test general bit*/
	{OP_JEQ,  0, 4, ETH_P_1588  }, /*f goto reject*/
	{OP_LDB,  0, 0, ETH_HLEN    },
	{OP_AND,  0, 0, PTP_GEN_BIT }, /*test general bit*/
	{OP_JEQ,  0, 1, 0           }, /*0,1=accept event; 1,0=accept general*/
	{OP_RETK, 0, 0, 1500        }, /*accept*/
	{OP_RETK, 0, 0, 0           }, /*reject*/
};

static int raw_configure(int fd, int event, int index,
			 unsigned char *addr1, unsigned char *addr2, int enable)
{
	int err1, err2, filter_test, option;
	struct packet_mreq mreq;
	struct sock_fprog prg = { N_RAW_FILTER, raw_filter };

	filter_test = RAW_FILTER_TEST;
	if (event) {
		raw_filter[filter_test].jt = 0;
		raw_filter[filter_test].jf = 1;
	} else {
		raw_filter[filter_test].jt = 1;
		raw_filter[filter_test].jf = 0;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &prg, sizeof(prg))) {
		pr_err("setsockopt SO_ATTACH_FILTER failed: %m");
		return -1;
	}

	option = enable ? PACKET_ADD_MEMBERSHIP : PACKET_DROP_MEMBERSHIP;

	mreq.mr_ifindex = index;
	mreq.mr_type = PACKET_MR_MULTICAST;
	mreq.mr_alen = 6;
	memcpy(mreq.mr_address, addr1, 6);

	err1 = setsockopt(fd, SOL_PACKET, option, &mreq, sizeof(mreq));
	if (err1)
		pr_warning("setsockopt PACKET_MR_MULTICAST failed: %m");

	memcpy(mreq.mr_address, addr2, 6);

	err2 = setsockopt(fd, SOL_PACKET, option, &mreq, sizeof(mreq));
	if (err2)
		pr_warning("setsockopt PACKET_MR_MULTICAST failed: %m");

	if (!err1 && !err2)
		return 0;

	mreq.mr_ifindex = index;
	mreq.mr_type = PACKET_MR_ALLMULTI;
	mreq.mr_alen = 0;
	if (!setsockopt(fd, SOL_PACKET, option, &mreq, sizeof(mreq))) {
		return 0;
	}
	pr_warning("setsockopt PACKET_MR_ALLMULTI failed: %m");

	mreq.mr_ifindex = index;
	mreq.mr_type = PACKET_MR_PROMISC;
	mreq.mr_alen = 0;
	if (!setsockopt(fd, SOL_PACKET, option, &mreq, sizeof(mreq))) {
		return 0;
	}
	pr_warning("setsockopt PACKET_MR_PROMISC failed: %m");

	pr_err("all socket options failed");
	return -1;
}

static int raw_close(struct transport *t, struct fdarray *fda)
{
	close(fda->fd[0]);
	close(fda->fd[1]);
	return 0;
}

unsigned char ptp_dst_mac[MAC_LEN] = { PTP_DST_MAC };
unsigned char p2p_dst_mac[MAC_LEN] = { P2P_DST_MAC };

static int open_socket(char *name, int event)
{
	struct sockaddr_ll addr;
	int fd, index;

	fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd < 0) {
		pr_err("socket failed: %m");
		goto no_socket;
	}
	index = sk_interface_index(fd, name);
	if (index < 0)
		goto no_option;

	memset(&addr, 0, sizeof(addr));
	addr.sll_ifindex = index;
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETH_P_ALL);
	if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
		pr_err("bind failed: %m");
		goto no_option;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, name, strlen(name))) {
		pr_err("setsockopt SO_BINDTODEVICE failed: %m");
		goto no_option;
	}
	if (raw_configure(fd, event, index, ptp_dst_mac, p2p_dst_mac, 1))
		goto no_option;

	return fd;
no_option:
	close(fd);
no_socket:
	return -1;
}

static int raw_open(struct transport *t, char *name,
		    struct fdarray *fda, enum timestamp_type ts_type)
{
	struct raw *raw = container_of(t, struct raw, t);
	int efd, gfd;

	memcpy(raw->ptp_addr.dst, ptp_dst_mac, MAC_LEN);
	memcpy(raw->p2p_addr.dst, p2p_dst_mac, MAC_LEN);

	if (sk_interface_macaddr(name, raw->ptp_addr.src, MAC_LEN))
		goto no_mac;

	memcpy(raw->p2p_addr.src, raw->ptp_addr.src, MAC_LEN);

	efd = open_socket(name, 1);
	if (efd < 0)
		goto no_event;

	gfd = open_socket(name, 0);
	if (gfd < 0)
		goto no_general;

	if (sk_timestamping_init(efd, name, ts_type, TRANS_IEEE_802_3))
		goto no_timestamping;

	fda->fd[FD_EVENT] = efd;
	fda->fd[FD_GENERAL] = gfd;
	return 0;

no_timestamping:
	close(gfd);
no_general:
	close(efd);
no_event:
no_mac:
	return -1;
}

static int raw_recv(struct transport *t, int fd, void *buf, int buflen,
		    struct hw_timestamp *hwts)
{
	int cnt, hlen;
	unsigned char *ptr = buf;
	struct eth_hdr *hdr;
	struct raw *raw = container_of(t, struct raw, t);

	if (raw->vlan) {
		hlen = sizeof(struct vlan_hdr);
	} else {
		hlen = sizeof(struct eth_hdr);
	}
	ptr    -= hlen;
	buflen += hlen;
	hdr = (struct eth_hdr *) ptr;

	cnt = sk_receive(fd, ptr, buflen, hwts, 0);

	if (raw->vlan) {
		if (ETH_P_1588 == ntohs(hdr->type)) {
			pr_notice("raw: disabling VLAN mode");
			raw->vlan = 0;
		}
	} else {
		if (ETH_P_8021Q == ntohs(hdr->type)) {
			pr_notice("raw: switching to VLAN mode");
			raw->vlan = 1;
		}
	}
	if (cnt >= hlen)  {
		cnt -= hlen;
	}
	return cnt;
}

static int raw_send(struct transport *t, struct fdarray *fda, int event, int peer,
		    void *buf, int len, struct hw_timestamp *hwts)
{
	struct raw *raw = container_of(t, struct raw, t);
	ssize_t cnt;
	int fd = event ? fda->fd[FD_EVENT] : fda->fd[FD_GENERAL];
	unsigned char pkt[1600], *ptr = buf;
	struct eth_hdr *hdr;

	ptr -= sizeof(*hdr);
	len += sizeof(*hdr);

	hdr = (struct eth_hdr *) ptr;
	if (peer)
		memcpy(&hdr->mac, &raw->p2p_addr, sizeof(hdr->mac));
	else
		memcpy(&hdr->mac, &raw->ptp_addr, sizeof(hdr->mac));

	hdr->type = htons(ETH_P_1588);

	cnt = send(fd, ptr, len, 0);
	if (cnt < 1) {
		pr_err("send failed: %d %m", errno);
		return cnt;
	}
	/*
	 * Get the time stamp right away.
	 */
	return event == TRANS_EVENT ? sk_receive(fd, pkt, len, hwts, MSG_ERRQUEUE) : cnt;
}

static void raw_release(struct transport *t)
{
	struct raw *raw = container_of(t, struct raw, t);
	free(raw);
}

static int raw_physical_addr(struct transport *t, uint8_t *addr)
{
	struct raw *raw = container_of(t, struct raw, t);
	memcpy(addr, raw->ptp_addr.src, MAC_LEN);
	return MAC_LEN;
}

static int raw_protocol_addr(struct transport *t, uint8_t *addr)
{
	struct raw *raw = container_of(t, struct raw, t);
	memcpy(addr, raw->ptp_addr.src, MAC_LEN);
	return MAC_LEN;
}

struct transport *raw_transport_create(void)
{
	struct raw *raw;
	raw = calloc(1, sizeof(*raw));
	if (!raw)
		return NULL;
	raw->t.close   = raw_close;
	raw->t.open    = raw_open;
	raw->t.recv    = raw_recv;
	raw->t.send    = raw_send;
	raw->t.release = raw_release;
	raw->t.physical_addr = raw_physical_addr;
	raw->t.protocol_addr = raw_protocol_addr;
	return &raw->t;
}

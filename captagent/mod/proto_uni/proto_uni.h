/*
 * $Id$
 *
 *  captagent - Homer capture agent. Modular
 *  Duplicate SIP messages in Homer Encapulate Protocol [HEP] [ipv6 version]
 *
 *  Author: Alexandr Dubovikov <alexandr.dubovikov@gmail.com>
 *  (C) Homer Project 2012 (http://www.sipcapture.org)
 *
 * Homer capture agent is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version
 *
 * Homer capture agent is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
*/

#ifndef _PROTO_UNI_H
#define _PROTO_UNI_H

#include "../../src/xmlread.h"
  

#define FILTER_LEN 4080

/* SYNC this list: http://hep.sipcapture.org */
#define PROTO_SIP    0x01
#define PROTO_XMPP   0x02
#define PROTO_SDP    0x03
#define PROTO_RTP    0x04
#define PROTO_RTCP   0x05
#define PROTO_MGCP   0x06
#define PROTO_MEGACO 0x07
#define PROTO_M2UA   0x08
#define PROTO_M3UA   0x09
#define PROTO_IAX    0x0a
#define PROTO_H322   0x0b
#define PROTO_H321   0x0c

int port = 0; 
char *portrange = NULL;
char *userfilter=NULL;
char *ip_proto = NULL;
int proto_type = PROTO_SIP; /* DEFAULT SIP */
int promisc = 1;
int vlan = 0; /*vlan filter*/
char * sip_method = NULL;
int sip_method_not = 0;
int sip_parse = 0;
int rtcp_tracking = 0;
int send_enable = 1;
int validate_sip = 0;
int validate_len = 0;

#define EXPIRE_RTCP_HASH 80
#define EXPIRE_TIMER_ARRAY 80

int expire_timer_array = EXPIRE_TIMER_ARRAY;
int expire_hash_value = EXPIRE_RTCP_HASH;
int timer_loop_stop = 1;

/* ip reasm */
int reasm_enable = 0;
int tcpdefrag_enable = 0;
int buildin_reasm_filter = 0;
int debug_proto_uni_enable = 0;
struct reasm_ip *reasm = NULL;
struct tcpreasm_ip *tcpreasm = NULL;

static int sendPacketsCount=0;

extern char* usefile;
extern int handler(int value);

#define BPF_DEFRAGMENTION_FILTER "(ip[6:2] & 0x3fff != 0)"

/* header offsets */
#define ETHHDR_SIZE 14
#define TOKENRING_SIZE 22
#define PPPHDR_SIZE 4
#define SLIPHDR_SIZE 16
#define RAWHDR_SIZE 0
#define LOOPHDR_SIZE 4
#define FDDIHDR_SIZE 21
#define ISDNHDR_SIZE 16
#define IEEE80211HDR_SIZE 32
         
int load_module(xml_node *config);

int dump_proto_packet(struct pcap_pkthdr *, u_char *, uint8_t, unsigned char *, uint32_t,const char *,
            const char *, uint16_t, uint16_t, uint8_t,uint16_t, uint8_t, uint16_t, uint32_t, uint32_t);



#endif


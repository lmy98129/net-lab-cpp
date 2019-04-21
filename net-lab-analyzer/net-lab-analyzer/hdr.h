#pragma once
#include <WinSock2.h>

// MAC帧首部
typedef struct eth_hdr
{
	u_char dst_mac[6];
	u_char src_mac[6];
	u_short eth_type;
}eth_hdr;

// IP数据报首部
typedef struct ip_hdr {
	u_char ver_ihl; // 版本 (4 bits) + 首部长度 (4 bits)
	u_char tos; 
	u_short tlen;
	u_short identification;
	u_short flags_fo; 
	u_char ttl; 
	u_char protocol; 
	u_short crc; 
	u_char sourceIP[4];
	u_char destIP[4];
	// u_int op_pad; // 选项与填充(Option + Padding)
}ip_hdr;

// TCP首部
typedef struct tcp_hdr
{
	u_short sport;
	u_short dport;
	u_int seq;
	u_int ack;
	u_char head_len;
	u_char flags;
	u_short wind_size;
	u_short check_sum;
	u_short urg_ptr;
}tcp_hdr;

// UDP首部
typedef struct udp_hdr
{
	u_short sport;
	u_short dport;
	u_short tot_len;
	u_short check_sum;
}udp_hdr;

// arp首部
typedef struct arp_hdr {
	u_int16_t htype;
	u_int16_t ptype;
	u_char hlen;
	u_char plen;
	u_int16_t oper;
	u_char sha[6];
	u_char spa[4];
	u_char tha[6];
	u_char tpa[4];
}arp_hdr;

// icmp首部
typedef struct icmp_hdr {
	u_char type;
	u_char code;
	u_short checksum;
	u_short id;
	u_short sequence;
}icmp_hdr;

typedef struct tcp_psd_hdr{
	u_char s_ip[4];
	u_char d_ip[4];
	u_char padding;
	u_char ptcl;
	u_short plen;
}tcp_psd_hdr;

// 常用的包长度
#define ARP_LEN 60
#define TCP_LEN 54
#define ICMP_LEN 52
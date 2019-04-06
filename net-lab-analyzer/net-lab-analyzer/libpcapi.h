#pragma once
#include <string>
#define WIN32
#include <pcap.h>
#include <iphlpapi.h>

extern struct eth_hdr* ethernet;
extern struct ip_hdr* ip;
extern struct tcp_hdr* tcp;
extern struct udp_hdr* udp;
extern struct icmp_hdr* icmp;
extern struct arp_hdr* arp;
extern struct tcp_psd_hdr* tcp_psd;

using namespace std;

int list_device(pcap_if_t** alldevs, char* errbuf);
int init_device(char* device, pcap_t** adhandle, char* errbuf);

int init_capture(char* device, pcap_t** adhandle, bpf_u_int32* ipaddress, bpf_u_int32* ipmask, char* errbuf);
int set_filter(pcap_t** adhandle, char* packet_filter, bpf_u_int32* ipmask, char* errbuf);
void packet_handler(u_char* arg, const struct pcap_pkthdr* pkt_header, const u_char* pkt_data);
void write_log(char* data_buffer);

int set_mac_hdr(char* device, u_char* dst_mac, u_char* src_mac, u_short eth_type, u_char* packet);
int set_ip_hdr(char* device, u_char proto, u_char* sourceIP, u_char* destIP, u_char* packet, int tlen);

int get_local_ip(char* device, u_char* src_ip);
int get_local_mac(char* device, u_char* src_mac);

u_short check_sum(u_short *packet, int packlen);
unsigned short tcp_chksum();
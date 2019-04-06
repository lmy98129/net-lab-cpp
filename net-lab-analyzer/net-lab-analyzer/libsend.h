#pragma once
#include "libpcapi.h"

int get_local_mac(char* device, u_char* src_mac);
int get_local_ip(char* device, u_char* src_ip);
int set_mac_hdr(char* device, u_char* dst_mac, u_char* src_mac, u_short eth_type, u_char* packet);
int set_arp_hdr(char* device, u_char* sha, u_char* spa, u_char* tha, u_char* tpa, u_char* packet);
int arp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, char* errbuf);
int tcp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, char* errbuf);
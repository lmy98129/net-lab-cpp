#include "libpcapi.h"
#include "hdr.h"

int tcp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, int port, char* errbuf);
void set_tcp_hdr(u_char* packet, int port);
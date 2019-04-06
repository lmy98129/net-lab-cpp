#include "libpcapi.h"
#include "hdr.h"

int set_arp_hdr(char* device, u_char* sha, u_char* spa, u_char* tha, u_char* tpa, u_char* packet);
int arp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, char* errbuf);
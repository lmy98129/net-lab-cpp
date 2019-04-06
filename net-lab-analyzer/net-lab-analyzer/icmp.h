#include "libpcapi.h"
#include "hdr.h"

int icmp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, char* errbuf);
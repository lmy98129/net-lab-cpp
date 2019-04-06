#include "arp.h"

/**
* 设置ARP首部
*/
int set_arp_hdr(char* device, u_char* sha, u_char* spa, u_char* tha, u_char* tpa, u_char* packet) {
	arp = (arp_hdr *)(packet + sizeof(eth_hdr));

	arp->htype = htons(0x0001);
	// 硬件类型：ethernet 0x0001
	arp->ptype = htons(0x0800);
	// 协议类型：IPv4 0x0800
	arp->hlen = 0x06;
	// 硬件地址长度：MAC 6
	arp->plen = 0x04;
	// 协议地址长度: IP 4
	arp->oper = htons(0x0001);
	// arp请求：0x0001
	memcpy(arp->sha, sha, sizeof(arp->sha) / sizeof(u_char));
	// 源MAC地址
	if (spa != NULL) {
		memcpy(arp->spa, spa, sizeof(arp->spa) / sizeof(u_char));
		// 源IP地址
	}
	else {
		if (get_local_ip(device, arp->spa) == 1) {
			return 1;
		}
	}
	for (int i = 0; i<6; i++) {
		arp->tha[i] = (u_char)0x00;
		// 目的MAC地址
	}
	if (tpa != NULL) {
		memcpy(arp->tpa, tpa, sizeof(arp->tpa) / sizeof(u_char));
		// 目的IP地址
	}
	else {
		for (int i = 0; i<6; i++) {
			arp->tpa[i] = (u_char)0x00;
		}
	}

	return 0;
}

/**
* 发送ARP数据包
*/
int arp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, char* errbuf) {
	u_char packet[ARP_LEN] = { 0 };
	// 设置MAC帧首部，因为是请求帧，所以源MAC为本地网卡MAC地址，目的MAC为全1
	if (set_mac_hdr(device, NULL, NULL, (u_short)0x0806, packet) == 1) {
		return 1;
	}
	// 设置ARP首部
	if (set_arp_hdr(device, ethernet->src_mac, NULL, ethernet->dst_mac, dest_ip, packet) == 1) {
		return 1;
	}

	printf("当前数据包的内容如下：\n");
	for (int i = 0; i<(ARP_LEN) / sizeof(u_char); i++) {
		printf(" %02x", packet[i]);
		if ((i + 1) % 16 == 0) {
			printf("\n");
		}
	}
	printf("\n\n");

	bpf_u_int32 *ipaddress = (bpf_u_int32*)malloc(sizeof(bpf_u_int32)),
		*ipmask = (bpf_u_int32*)malloc(sizeof(bpf_u_int32));

	int id = 0;

	char packet_filter[70];
	snprintf(packet_filter, sizeof(char) * 70, "arp and (ether dst %02x-%02x-%02x-%02x-%02x-%02x)", arp->sha[0], arp->sha[1], arp->sha[2], arp->sha[3], arp->sha[4], arp->sha[5]);
	printf("设置过滤器规则：%s\n\n", packet_filter);

	// 初始化当前网卡，准备开始捕获
	/*
	* 之所以在发送之前就开始初始化捕获和过滤，
	* 是为了打一个提前量，ARP包收发较快容易在初始化捕获的过程中错过对方的回复
	*/
	if (init_capture(device, adhandle, ipaddress, ipmask, errbuf) <= 0) {
		printf("程序结束\n");
		return 1;
	}

	// 设置过滤器
	if (set_filter(adhandle, packet_filter, ipmask, errbuf) <= 0) {
		printf("程序结束\n");
		return 1;
	}

	if (pcap_sendpacket(*adhandle, packet, ARP_LEN) != 0) {
		printf("arp_sender - 发送ARP数据包出错: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("发送ARP数据包成功\n\n");
	}

	printf("开始监听对方是否回复\n\n");

	// 开始使用异步回调方式抓取数据包
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - 抓取数据包出错: %s\n", errbuf);
		return 1;
	}

	return 0;
}
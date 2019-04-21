#include "tcp.h"

/**
* 设置TCP数据报首部
*/
void set_tcp_hdr(u_char* packet, int port) {
	tcp = (tcp_hdr*)(packet + sizeof(eth_hdr) + sizeof(ip_hdr));
	srand(time(0));
	// 源端口是随机生成的
	tcp->sport = htons(rand() % 65535);
	tcp->dport = htons(port);
	// TCP首部设置到最长
	tcp->head_len = 0x50;
	// ACK=0, SYN=1
	tcp->flags = 0x02;
	tcp->wind_size = htons(0xffff);
}

/**
* 发送TCP数据包
*/
int tcp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, int port, char* errbuf) {
	u_char packet[TCP_LEN] = { 0 };
	// 设置MAC帧首部，源MAC为本地网卡MAC地址，目的MAC为网关MAC
	if (set_mac_hdr(device, dest_mac, NULL, (u_short)0x0800, packet) == 1) {
		printf("set_mac_hdr - 设置MAC帧首部出错: %s (errno: %d)\n", strerror(errno), errno);
		system("pause");
		return 1;
	}
	// 设置IP数据报首部，注意此时没有校验和
	if (set_ip_hdr(device, 0x06, NULL, dest_ip, packet, NULL) == 1) {
		printf("程序结束\n");
		system("pause");
		return 1;
	}
	// 计算IP数据报的校验和
	u_short crc = check_sum((u_short*)(packet + sizeof(eth_hdr)), sizeof(ip_hdr));
	if (crc != NULL) ip->crc = crc;

	// 设置TCP数据报、并添加伪首部计算校验和
	set_tcp_hdr(packet, port);
	tcp->check_sum = tcp_chksum();

	printf("当前数据包的内容如下：\n");
	for (int i = 0; i<(TCP_LEN) / sizeof(u_char); i++) {
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
	snprintf(packet_filter, sizeof(char) * 70, "tcp and (host %d.%d.%d.%d)", ip->destIP[0], ip->destIP[1], ip->destIP[2], ip->destIP[3]);
	printf("设置过滤器规则：%s\n\n", packet_filter);

	// 初始化当前网卡，准备开始捕获
	/*
	* 之所以在发送之前就开始初始化捕获和过滤，
	* 是为了打一个提前量，数据包收发较快容易在初始化捕获的过程中错过对方的回复
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

	if (pcap_sendpacket(*adhandle, packet, TCP_LEN) != 0) {
		printf("arp_sender - 发送TCP数据包出错: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("发送TCP数据包成功\n\n");
	}

	// 开始使用异步回调方式抓取数据包
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - 抓取数据包出错: %s\n", errbuf);
		return 1;
	}

	system("pause");

	return 0;
}
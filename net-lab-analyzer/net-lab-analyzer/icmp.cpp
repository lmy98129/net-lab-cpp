#include "icmp.h"

// ICMP类型字段
// 请求回显
#define ICMP_ECHO_REQUEST 8   
// 回显应答
#define ICMP_ECHO_REPLY 0     
// 传输超时
#define ICMP_TIMEOUT 11      

// ICMP报文默认数据字段长度
#define DEF_ICMP_DATA_SIZE 10    
// ICMP报文最大长度（包括报头）
#define MAX_ICMP_PACKET_SIZE 1024 
// 回显应答超时时间
#define DEF_ICMP_TIMEOUT 3000   
// 最大跳站数
#define DEF_MAX_HOP 4       


void set_icmp_hdr(u_char* packet) {
	icmp = (icmp_hdr*)(packet + sizeof(eth_hdr) + sizeof(ip_hdr));
	icmp->type = ICMP_ECHO_REQUEST;
	icmp->code = 0;
	icmp->id = GetCurrentProcessId();
	icmp->sequence = 0;
}

int icmp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, char* errbuf) {
	u_char packet[ICMP_LEN] = { 0 };
	// 设置MAC帧首部，源MAC为本地网卡MAC地址，目的MAC为网关MAC
	if (set_mac_hdr(device, dest_mac, NULL, (u_short)0x0800, packet) == 1) {
		printf("set_mac_hdr - 设置MAC帧首部出错: %s (errno: %d)\n", strerror(errno), errno);
		system("pause");
		return 1;
	}

	// 设置IP数据报首部，注意此时没有校验和
	if (set_ip_hdr(device, 0x01, NULL, dest_ip, packet, 38) == 1) {
		printf("程序结束\n");
		system("pause");
		return 1;
	}

	// 计算IP数据报的校验和
	u_short crc = check_sum((u_short*)(packet + sizeof(eth_hdr)), sizeof(ip_hdr));
	if (crc != NULL) ip->crc = crc;
	
	// 设置ICMP数据报、并计算校验和
	set_icmp_hdr(packet);
	memset(packet + sizeof(eth_hdr) + sizeof(ip_hdr) + sizeof(icmp_hdr), 'E', DEF_ICMP_DATA_SIZE);
	icmp->checksum = check_sum((u_short*)(packet + sizeof(eth_hdr) + sizeof(ip_hdr)), sizeof(icmp_hdr)+DEF_ICMP_DATA_SIZE);
	printf("icmp check sum: %x\n", ntohs(icmp->checksum));

	printf("当前数据包的内容如下：\n");
	for (int i = 0; i<(ICMP_LEN) / sizeof(u_char); i++) {
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
	snprintf(packet_filter, sizeof(char) * 70, "icmp and (host %d.%d.%d.%d)", ip->destIP[0], ip->destIP[1], ip->destIP[2], ip->destIP[3]);
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

	if (pcap_sendpacket(*adhandle, packet, ICMP_LEN) != 0) {
		printf("arp_sender - 发送ICMP数据包出错: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("发送ICMP数据包成功\n\n");
	}

	// 开始使用异步回调方式抓取数据包
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - 抓取数据包出错: %s\n", errbuf);
		return 1;
	}

	system("pause");

	return 0;
}
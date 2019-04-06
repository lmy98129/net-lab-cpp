#include "libpcapi.h"
#include "hdr.h"
//自定义的首部结构体，同时也通过包含in.h解决了u_类型问题

// 最大的打印输出长度
#define MAX_LEN 65535
// 打印输出缓冲区
char data_buffer[MAX_LEN];

// 最大文件名
#define MAX_FILENAME 50
// 文件名缓冲区（用于确定是否需要创建新的日志文件）
char filename[MAX_FILENAME];

// 网卡设备个数
#define ADAPTERNUM 15

// 时间戳字符串
#define TIME_LEN 50
char time_str[TIME_LEN] = "\0";
const char* time_format = "%Y-%m-%d:%H:%M:%S";

// IP服务类型
#define IP_TOS_TCP 0

// 实例化各数据报首部结构体指针
eth_hdr* ethernet;
ip_hdr* ip;
tcp_hdr* tcp;
udp_hdr* udp;
icmp_hdr* icmp;
arp_hdr* arp;
tcp_psd_hdr* tcp_psd;

/**
* 打印网卡设备列表
*/
int list_device(pcap_if_t** alldevs, char* errbuf) {
	// 设备数量计数
	int i = 0;
	// 设备结构体列表数组的指针
	pcap_if_t *d;

	// 使用pcap查找全部设备
	if (pcap_findalldevs_ex((char *)PCAP_SRC_IF_STRING, NULL, alldevs, errbuf) == -1) {
		return -1;
	}

	printf("当前网卡列表如下：\n");
	for (d = *alldevs; d; d = d->next) {
		printf("%d) %s", ++i, d->name);
		if (d->description)
			printf("-(%s)\n", d->description);
		else
			printf("\n");
	}

	if (i == 0) {
		return 0;
	}
	else return 1;

}

/**
* 获取数据包捕获描述符以查看网络上的数据包相关信息，这里主要查看当前网卡设备信息
* 65535：最大数据包长度
* 1：混杂模式，无论目的地是否是当前网卡都进行接收
* 1000：允许的最大延时（ms）
*/

int init_device(char* device, pcap_t** adhandle, char* errbuf) {

	*adhandle = pcap_open(device, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, NULL, errbuf);

	if (*adhandle == NULL) {
		printf("pcap_open_live - 获取当前网卡数据包捕获描述符出错: %s\n", errbuf);
		if (strstr(errbuf, "Operation not permitted")) {
			printf("请使用管理员模式（sudo）来启动本程序\n");
		}
		return 1;
	}

	return 0;
}

/**
* 初始化当前网卡，准备开始捕获
*/
int init_capture(char* device, pcap_t** adhandle, bpf_u_int32* ipaddress, bpf_u_int32* ipmask, char* errbuf) {

	struct in_addr addr;
	char *dev_ip, *dev_mask;

	if (pcap_lookupnet(device, ipaddress, ipmask, errbuf) == -1) {
		printf("pcap_lookupnet - 获取网卡的网络号和子网掩码出错: %s\n", errbuf);
		return -2;
	}

	return 1;
}

/**
* 设置过滤器
*/
int set_filter(pcap_t** adhandle, char* packet_filter, bpf_u_int32* ipmask, char* errbuf) {
	// fcode为过滤器结构体
	struct bpf_program fcode;

	// 将过滤字符串packet_filter编译进过滤信息
	if (pcap_compile(*adhandle, &fcode, packet_filter, 1, *ipmask) < 0) {
		printf("pcap_compile - 编译过滤信息出错: %s\n", errbuf);
		return -1;
	}

	if (pcap_setfilter(*adhandle, &fcode) < 0) {
		printf("pcap_setfilter - 设置过滤器出错: %s\n", errbuf);
		return -1;
	}

	return 1;
}

/**
*  捕获数据包后对数据包进行分析的回调函数
*/
void packet_handler(u_char* arg, const struct pcap_pkthdr* pkt_header, const u_char* pkt_data) {
	int* id = (int *)arg;
	snprintf(data_buffer, sizeof(char)*MAX_LEN, "数据包id = %d\n", ++(*id));

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "数据包长度: %d\n", pkt_header->len);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "字节长度: %d\n", pkt_header->caplen);

	// 时间戳格式转换
	time_t t = pkt_header->ts.tv_sec;
	struct tm *p = localtime(&t);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", p);

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "接收时间: %s\n", time_str);

	for (int i = 0; i<pkt_header->caplen; i++) {
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, " %02x", pkt_data[i]);
		if ((i + 1) % 16 == 0) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "\n");
		}
	}

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "\n\n");

	u_int eth_len = sizeof(struct eth_hdr);
	u_int ip_len = sizeof(struct ip_hdr);
	u_int tcp_len = sizeof(struct tcp_hdr);
	u_int udp_len = sizeof(struct udp_hdr);
	u_int arp_len = sizeof(struct arp_hdr);

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "数据包信息: \n\n");

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "MAC帧首部信息: \n");

	ethernet = (eth_hdr *)pkt_data;
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源MAC地址: %02x-%02x-%02x-%02x-%02x-%02x\n", ethernet->src_mac[0], ethernet->src_mac[1], ethernet->src_mac[2], ethernet->src_mac[3], ethernet->src_mac[4], ethernet->src_mac[5]);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的MAC地址: %02x-%02x-%02x-%02x-%02x-%02x\n", ethernet->dst_mac[0], ethernet->dst_mac[1], ethernet->dst_mac[2], ethernet->dst_mac[3], ethernet->dst_mac[4], ethernet->dst_mac[5]);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "协议类型号: %04x\n\n", ntohs(ethernet->eth_type));

	if (ntohs(ethernet->eth_type) == 0x0800) {
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "使用IPv4协议\n");
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "IPv4数据报首部信息:\n");
		ip = (ip_hdr*)(pkt_data + eth_len);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源IP地址: %d.%d.%d.%d\n", ip->sourceIP[0], ip->sourceIP[1], ip->sourceIP[2], ip->sourceIP[3]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的IP地址: %d.%d.%d.%d\n\n", ip->destIP[0], ip->destIP[1], ip->destIP[2], ip->destIP[3]);

		if (ip->protocol == 6) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "TCP数据包首部信息:\n");
			tcp = (tcp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源端口: %d\n", ntohs(tcp->sport));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的端口: %d\n", ntohs(tcp->dport));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "顺序号: %x\n", ntohl(tcp->seq));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "确认号: %x\n", ntohl(tcp->ack));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "头部长度: %u\n", tcp->head_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "保留字: %u\n", tcp->flags);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "窗口大小: %d\n", ntohs(tcp->wind_size));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "校验和: %04x\n", ntohs(tcp->check_sum));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "紧急指针: %d\n", ntohs(tcp->urg_ptr));
		}
		else if (ip->protocol == 17) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "UDP数据包首部信息:\n");
			udp = (udp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源端口: %u\n", udp->sport);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的端口: %u\n", udp->dport);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "数据包长度: %u\n", udp->tot_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "校验和: %u\n", udp->check_sum);
		}
		else if (ip->protocol == 1) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ICMP数据包首部信息:\n");
			icmp = (icmp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "类型: %u\n", icmp->type);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "代码: %u\n", icmp->code);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "校验和: %04x\n", ntohs(icmp->checksum));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "标识符: %d\n", ntohs(icmp->id));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "序列号: %d\n", ntohs(icmp->sequence));
		}
		else {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "使用了其他的传输层协议\n");
		}

	}
	else if (ntohs(ethernet->eth_type) == 0x0806) {
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "使用ARP协议\n");
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ARP数据报首部信息:\n\n");
		arp = (arp_hdr*)(pkt_data + eth_len);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "硬件类型号: %u\n", ntohs(arp->htype));
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "协议类型号: %u\n", arp->ptype);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "MAC地址长度: %u\n", arp->hlen);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "IP地址长度: %u\n", arp->plen);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "操作类型: %u\n", arp->oper);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源MAC地址: %02x-%02x-%02x-%02x-%02x-%02x\n", arp->sha[0], arp->sha[1], arp->sha[2], arp->sha[3], arp->sha[4], arp->sha[5]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "源IP地址: %d.%d.%d.%d\n", arp->spa[0], arp->spa[1], arp->spa[2], arp->spa[3]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的MAC地址: %02x-%02x-%02x-%02x-%02x-%02x\n", arp->tha[0], arp->tha[1], arp->tha[2], arp->tha[3], arp->tha[4], arp->tha[5]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "目的IP地址: %d.%d.%d.%d\n", arp->tpa[0], arp->tpa[1], arp->tpa[2], arp->tpa[3]);
	}

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "\n\n");
	printf("%s", data_buffer);

	write_log(data_buffer);

}

/**
* 日志文件记录
*/
void write_log(char* data_buffer) {
	time_t nowtime = time(NULL);
	struct tm *p;
	p = gmtime(&nowtime);

	memset(filename, 0, sizeof(char)*MAX_FILENAME);
	snprintf(filename, sizeof(char)*MAX_FILENAME, "log-%d-%d-%d.txt", 1900 + p->tm_year, 1 + p->tm_mon, p->tm_mday);

	int filed = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777);
	write(filed, data_buffer, strlen(data_buffer) * sizeof(char));
	close(filed);
}

/**
* 通过本机网卡列表获取当前网卡的MAC地址
*/
int get_local_mac(char* device, u_char* src_mac) {
	// 网卡列表
	PIP_ADAPTER_INFO pipAdapterInfo = new IP_ADAPTER_INFO[ADAPTERNUM];
	u_long stSize = sizeof(IP_ADAPTER_INFO)*ADAPTERNUM;
	int nRel = GetAdaptersInfo(pipAdapterInfo, &stSize);

	// 空间不足
	if (ERROR_BUFFER_OVERFLOW == nRel) {
		// 释放空间
		if (pipAdapterInfo != NULL) {
			delete[] pipAdapterInfo;
			return 1;
		}
	}

	PIP_ADAPTER_INFO cur = pipAdapterInfo;
	in_addr tmp_ip;

	for (cur; cur != NULL; cur = cur->Next) {
		if (strcmp(device + sizeof("rpcap://\Device\NPF_") / sizeof(char) + 1, cur->AdapterName) == 0) {
			memcpy(src_mac, cur->Address, cur->AddressLength);
		}
	}

	return 0;
}

/**
* 通过socket库获取当前网卡的IP地址
*/
int get_local_ip(char* device, u_char* src_ip) {
	// 网卡列表
	PIP_ADAPTER_INFO pipAdapterInfo = new IP_ADAPTER_INFO[ADAPTERNUM];
	u_long stSize = sizeof(IP_ADAPTER_INFO)*ADAPTERNUM;
	int nRel = GetAdaptersInfo(pipAdapterInfo, &stSize);

	// 空间不足
	if (ERROR_BUFFER_OVERFLOW == nRel) {
		// 释放空间
		if (pipAdapterInfo != NULL) {
			delete[] pipAdapterInfo;
			return 1;
		}
	}

	PIP_ADAPTER_INFO cur = pipAdapterInfo;
	in_addr tmp_ip;


	for (cur; cur != NULL; cur = cur->Next) {
		if (strcmp(device + sizeof("rpcap://\Device\NPF_") / sizeof(char) + 1, cur->AdapterName) == 0) {
			IP_ADDR_STRING* pipAddrString = &(cur->IpAddressList);
			if (inet_pton(AF_INET, pipAddrString->IpAddress.String, &(tmp_ip)) == 0) {
				printf("get_local_ip - 转换IP格式出错: %s (errno: %d)\n", strerror(errno), errno);
				return 1;
			}
			src_ip[0] = tmp_ip.s_addr & 0xff;
			src_ip[1] = (tmp_ip.s_addr >> 8) & 0xff;
			src_ip[2] = (tmp_ip.s_addr >> 16) & 0xff;
			src_ip[3] = (tmp_ip.s_addr >> 24) & 0xff;
			break;
		}
	}

	return 0;
}

/**
* 设置MAC帧首部
*/
int set_mac_hdr(char* device, u_char* dst_mac, u_char* src_mac, u_short eth_type, u_char* packet) {
	ethernet = (eth_hdr *)packet;
	if (dst_mac != NULL) {
		memcpy(ethernet->dst_mac, dst_mac, sizeof(ethernet->dst_mac) / sizeof(u_char));
	}
	else {
		for (int i = 0; i<6; i++) {
			ethernet->dst_mac[i] = (u_char)0xff;
		}
	}

	if (src_mac != NULL) {
		memcpy(ethernet->src_mac, src_mac, sizeof(ethernet->src_mac) / sizeof(u_char));
	}
	else {
		if (get_local_mac(device, ethernet->src_mac) == 1) {
			return 1;
		}

	}

	ethernet->eth_type = htons(eth_type);

	return 0;
}

/**
*	计算校验和
*/
u_short check_sum(u_short* packet, int size) {
	unsigned long cksum = 0;
	while (size > 1) {
		cksum += *packet++;
		size -= sizeof(u_short);
	}
	if (size) {
		// 长度为奇数的情况
		cksum += *(u_short*)packet;
	}
	// 将高位的进位加到低位去
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	
	// 最后要取反
	return (u_short)(~cksum);
}

/**
* 设置IP数据报首部
*/
int set_ip_hdr(char* device, u_char proto , u_char* sourceIP, u_char* destIP,  u_char* packet, int tlen) {
	ip = (ip_hdr*)(packet + sizeof(eth_hdr));
	// 版本以及首部长度
	ip->ver_ihl = 0x45;
	// 服务类型，普通的TCP/ICMP查询
	ip->tos = 0x00;
	// 总长度，默认设置为40
	if (tlen == NULL) tlen = 40;
	ip->tlen = htons(tlen);
	// 标识符，这里设置为0x0010
	ip->identification = htons(0x0000);
	// 生存时间，这里设置为255
	ip->ttl = 0xff;
	// 协议，有TCP/UDP/ICMP等，这里自行设置
	ip->protocol = proto;
	if (sourceIP != NULL) {
		memcpy(ip->sourceIP, sourceIP, sizeof(ip->sourceIP) / sizeof(u_char));
		// 源IP地址
	} else {
		if (get_local_ip(device, ip->sourceIP) == 1) {
			printf("set_ip_hdr - 设置IP数据报首部出错: %s (errno: %d)\n", strerror(errno), errno);
			return 1;
		}
	}
	if (destIP == NULL) return 1;
	else memcpy(ip->destIP, destIP, sizeof(ip->destIP) / sizeof(u_char));
	// 目的地址

	return 0;
}

//UDP/TCP校验和程序
//校验UDP/TCP首部首先需要封装一个伪首部，然后再校验，校验长度包括头和数据部分
//这个len是指除去伪首部的TCP的原来的长度
unsigned short tcp_chksum() {
	u_char psd_packet[1024];

	tcp_psd = (tcp_psd_hdr*)malloc(sizeof(tcp_psd_hdr));
	memset(tcp_psd, 0, sizeof(tcp_psd_hdr));

	memcpy(tcp_psd->d_ip, ip->sourceIP, sizeof(ip->sourceIP) / sizeof(u_char));
	memcpy(tcp_psd->s_ip, ip->destIP, sizeof(ip->destIP) / sizeof(u_char));
	tcp_psd->ptcl = 0x06;
	tcp_psd->plen = htons(sizeof(tcp_hdr));

	memcpy(psd_packet, tcp_psd, sizeof(tcp_psd_hdr));
	memcpy(psd_packet + sizeof(tcp_psd_hdr), tcp, sizeof(tcp_hdr));

	return check_sum((u_short *)psd_packet, sizeof(tcp_hdr) + sizeof(tcp_psd_hdr));
}

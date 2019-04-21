#include "libpcapi.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include <iostream>

// 最大输入长度
#define MAXLINE 4096

// 抓包模式和发包模式的序号
#define MODE_RECV 1
#define MODE_SEND 2

// 发包模式数据包类型
#define TYPE_ARP 1
#define TYPE_ICMP 2
#define TYPE_TCP 3

int recv_handler(char* device, pcap_t** adhandle, char* packet_filter, int id, char* errbuf);
int send_handler(char* device, pcap_t** adhandle, int type, char* errbuf);

int main() {
	// errbuf为错误信息缓冲, device为选中的网卡的设备名称
	char errbuf[PCAP_ERRBUF_SIZE], device[100];

	// 网卡设备结构体的指针
	// 其中alldevs为所有网卡列表，d为选中的网卡
	pcap_if_t *alldevs, *d;

	// 网卡的序号
	int device_id;

	// 网卡句柄
	pcap_t** adhandle = (pcap_t**) malloc(sizeof(pcap_t*));

	// 过滤条件字符串
	char packet_filter[MAXLINE];


	// 抓包时的数据包id，抓包/发包模式mode，数据包类型type
	int id = 0, mode = 0, type = 0;

	// 列出所有网卡列表
	switch (list_device(&alldevs, errbuf)) {
	case 0:
		printf("\n暂无网卡可选！程序结束。\n");
		return 1;
	case -1:
		printf("list_device - 查找网卡设备列表时出错: %s\n", errbuf);
		return 1;
	}

	// 输入网卡序号以选择一个网卡
	do {
		printf("\n请输入您选择的网卡序号：");
		scanf("%d", &device_id);
		// 网卡序号计数
		int i = 0;
		for (d = alldevs; d; d = d->next) {
			i++;
			if (i == device_id) {
				strcpy(device, d->name);
				break;
			}
		}

		if (d) {
			printf("您当前选择的是网卡%s\n\n", device);
			break;
		}
		else {
			printf("您输入的序号无效，请重新选择网卡\n");
		}
	} while (!d);

	// 释放设备列表，其实可以理解为释放内存空间
	pcap_freealldevs(alldevs);

	// 初始化网卡，可能会出现Device not exist的情况，多试几次就行
	if (init_device(device, adhandle, errbuf) == 1) {
		system("pause");
		return 1;
	}

	printf("1) 抓包模式\n2) 发包模式\n");
	do {
		printf("请选择抓包/发包模式：");
		scanf("%d", &mode);
		if (!mode || (mode != MODE_RECV && mode != MODE_SEND)) {
			printf("您输入的序号无效，请重新选择模式\n\n");
		}
		else break;
	} while (1);

	printf("\n");

	switch (mode) {
	case MODE_RECV:
		if (recv_handler(device, adhandle, packet_filter, id, errbuf) == 1) {
			return 1;
		}
		break;

	case MODE_SEND:
		if (send_handler(device, adhandle, type, errbuf) == 1) {
			return 1;
		}
		break;

	default:
		printf("您输入的模式无效，程序结束。\n");
		return 1;
	}

	return 0;

	system("pause");

	return 0;
}

int recv_handler(char* device, pcap_t** adhandle, char* packet_filter, int id, char* errbuf) {
	// 网卡的IP地址，子网掩码
	bpf_u_int32 *ipaddress = (bpf_u_int32*)malloc(sizeof(bpf_u_int32)),
		*ipmask = (bpf_u_int32*)malloc(sizeof(bpf_u_int32));

	// 设置过滤器
	printf("请输入过滤条件字符串：\n");
	cin.get();
	cin.get(packet_filter, MAXLINE);
	printf("%s\n", packet_filter);

	// 初始化当前网卡，准备开始捕获
	if (init_capture(device, adhandle, ipaddress, ipmask, errbuf) <= 0) {
		printf("程序结束\n");
		return 1;
	}

	if (set_filter(adhandle, packet_filter, ipmask, errbuf) <= 0) {
		printf("程序结束\n");
		return 1;
	}

	printf("\n");

	// 开始使用异步回调方式抓取数据包
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - 抓取数据包出错: %s\n", errbuf);
		return 1;
	}

	return 0;
}

int send_handler(char* device, pcap_t** adhandle, int type, char* errbuf) {
	printf("1) ARP\n2) ICMP\n3) TCP\n");
	do {
		printf("请选择将要发送的数据包类型：");
		scanf("%d", &type);
		if (!type || (type != TYPE_ARP && type != TYPE_ICMP && type != TYPE_TCP)) {
			printf("您输入的序号无效，请重新选择\n\n");
		}
		else break;
	} while (1);

	printf("\n");

	char ip_str[15];
	char mac_str[18];
	in_addr tmp_ip;
	u_char dest_ip[4];
	u_char dest_mac[7];
	int port;

	switch (type) {
		case TYPE_ARP:
			printf("将发送ARP请求帧，则MAC帧将使用默认首部：\n目的MAC地址为ff:ff:ff:ff:ff:ff\n源MAC地址为您所选网卡的MAC地址\n\n");
			do {
				printf("请输入目的IP地址：\n");
				scanf("%s", ip_str);
				if (inet_pton(AF_INET, ip_str, &(tmp_ip)) == 0) {
					printf("输入的IP非法，请重新输入\n");
				}
				else break;
			} while (1);

			printf("\n");

			// 强制转换为u_char数组
			dest_ip[0] = tmp_ip.s_addr & 0xff;
			dest_ip[1] = (tmp_ip.s_addr >> 8) & 0xff;
			dest_ip[2] = (tmp_ip.s_addr >> 16) & 0xff;
			dest_ip[3] = (tmp_ip.s_addr >> 24) & 0xff;
			if (arp_sender(device, adhandle, dest_ip, errbuf) == 1) {
				return 1;
			}
			break;

		case TYPE_ICMP:
		case TYPE_TCP:
			if (type == TYPE_TCP) {
				printf("将发送TCP的连接建立请求，");
			}
			else  if (type == TYPE_ICMP) {
				printf("将发送ICMP的询问报文，");
			}
			do {
				printf("请输入目的IP地址：\n");
				scanf("%s", ip_str);
				if (inet_pton(AF_INET, ip_str, &(tmp_ip)) == 0) {
					printf("输入的IP非法，请重新输入\n");
				}
				else break;
			} while (1);

			printf("\n");

			// 强制转换为u_char数组
			dest_ip[0] = tmp_ip.s_addr & 0xff;
			dest_ip[1] = (tmp_ip.s_addr >> 8) & 0xff;
			dest_ip[2] = (tmp_ip.s_addr >> 16) & 0xff;
			dest_ip[3] = (tmp_ip.s_addr >> 24) & 0xff;

			if (type == TYPE_TCP) {
				printf("请输入目的端口：");
				scanf("%d", &port);
				printf("\n");
			}

			printf("请输入目的MAC地址（建议输入当前网关的MAC地址）：\n");
			scanf("%s", mac_str);
			sscanf_s(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest_mac[0], &dest_mac[1], &dest_mac[2], &dest_mac[3], &dest_mac[4], &dest_mac[5]);

			printf("\n");


			if (type == TYPE_TCP) {
				if (tcp_sender(device, adhandle, dest_ip, dest_mac, port, errbuf) == 1) {
					return 1;
				}
			}
			else if (type == TYPE_ICMP) {
				if (icmp_sender(device, adhandle, dest_ip, dest_mac, errbuf) == 1) {
					return 1;
				}
			}

			break;
			
	}
	system("pause");
	return 0;
}
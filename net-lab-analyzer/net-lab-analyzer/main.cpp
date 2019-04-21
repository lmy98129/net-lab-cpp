#include "libpcapi.h"
#include "arp.h"
#include "tcp.h"
#include "icmp.h"
#include <iostream>

// ������볤��
#define MAXLINE 4096

// ץ��ģʽ�ͷ���ģʽ�����
#define MODE_RECV 1
#define MODE_SEND 2

// ����ģʽ���ݰ�����
#define TYPE_ARP 1
#define TYPE_ICMP 2
#define TYPE_TCP 3

int recv_handler(char* device, pcap_t** adhandle, char* packet_filter, int id, char* errbuf);
int send_handler(char* device, pcap_t** adhandle, int type, char* errbuf);

int main() {
	// errbufΪ������Ϣ����, deviceΪѡ�е��������豸����
	char errbuf[PCAP_ERRBUF_SIZE], device[100];

	// �����豸�ṹ���ָ��
	// ����alldevsΪ���������б�dΪѡ�е�����
	pcap_if_t *alldevs, *d;

	// ���������
	int device_id;

	// �������
	pcap_t** adhandle = (pcap_t**) malloc(sizeof(pcap_t*));

	// ���������ַ���
	char packet_filter[MAXLINE];


	// ץ��ʱ�����ݰ�id��ץ��/����ģʽmode�����ݰ�����type
	int id = 0, mode = 0, type = 0;

	// �г����������б�
	switch (list_device(&alldevs, errbuf)) {
	case 0:
		printf("\n����������ѡ�����������\n");
		return 1;
	case -1:
		printf("list_device - ���������豸�б�ʱ����: %s\n", errbuf);
		return 1;
	}

	// �������������ѡ��һ������
	do {
		printf("\n��������ѡ���������ţ�");
		scanf("%d", &device_id);
		// ������ż���
		int i = 0;
		for (d = alldevs; d; d = d->next) {
			i++;
			if (i == device_id) {
				strcpy(device, d->name);
				break;
			}
		}

		if (d) {
			printf("����ǰѡ���������%s\n\n", device);
			break;
		}
		else {
			printf("������������Ч��������ѡ������\n");
		}
	} while (!d);

	// �ͷ��豸�б���ʵ�������Ϊ�ͷ��ڴ�ռ�
	pcap_freealldevs(alldevs);

	// ��ʼ�����������ܻ����Device not exist����������Լ��ξ���
	if (init_device(device, adhandle, errbuf) == 1) {
		system("pause");
		return 1;
	}

	printf("1) ץ��ģʽ\n2) ����ģʽ\n");
	do {
		printf("��ѡ��ץ��/����ģʽ��");
		scanf("%d", &mode);
		if (!mode || (mode != MODE_RECV && mode != MODE_SEND)) {
			printf("������������Ч��������ѡ��ģʽ\n\n");
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
		printf("�������ģʽ��Ч�����������\n");
		return 1;
	}

	return 0;

	system("pause");

	return 0;
}

int recv_handler(char* device, pcap_t** adhandle, char* packet_filter, int id, char* errbuf) {
	// ������IP��ַ����������
	bpf_u_int32 *ipaddress = (bpf_u_int32*)malloc(sizeof(bpf_u_int32)),
		*ipmask = (bpf_u_int32*)malloc(sizeof(bpf_u_int32));

	// ���ù�����
	printf("��������������ַ�����\n");
	cin.get();
	cin.get(packet_filter, MAXLINE);
	printf("%s\n", packet_filter);

	// ��ʼ����ǰ������׼����ʼ����
	if (init_capture(device, adhandle, ipaddress, ipmask, errbuf) <= 0) {
		printf("�������\n");
		return 1;
	}

	if (set_filter(adhandle, packet_filter, ipmask, errbuf) <= 0) {
		printf("�������\n");
		return 1;
	}

	printf("\n");

	// ��ʼʹ���첽�ص���ʽץȡ���ݰ�
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - ץȡ���ݰ�����: %s\n", errbuf);
		return 1;
	}

	return 0;
}

int send_handler(char* device, pcap_t** adhandle, int type, char* errbuf) {
	printf("1) ARP\n2) ICMP\n3) TCP\n");
	do {
		printf("��ѡ��Ҫ���͵����ݰ����ͣ�");
		scanf("%d", &type);
		if (!type || (type != TYPE_ARP && type != TYPE_ICMP && type != TYPE_TCP)) {
			printf("������������Ч��������ѡ��\n\n");
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
			printf("������ARP����֡����MAC֡��ʹ��Ĭ���ײ���\nĿ��MAC��ַΪff:ff:ff:ff:ff:ff\nԴMAC��ַΪ����ѡ������MAC��ַ\n\n");
			do {
				printf("������Ŀ��IP��ַ��\n");
				scanf("%s", ip_str);
				if (inet_pton(AF_INET, ip_str, &(tmp_ip)) == 0) {
					printf("�����IP�Ƿ�������������\n");
				}
				else break;
			} while (1);

			printf("\n");

			// ǿ��ת��Ϊu_char����
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
				printf("������TCP�����ӽ�������");
			}
			else  if (type == TYPE_ICMP) {
				printf("������ICMP��ѯ�ʱ��ģ�");
			}
			do {
				printf("������Ŀ��IP��ַ��\n");
				scanf("%s", ip_str);
				if (inet_pton(AF_INET, ip_str, &(tmp_ip)) == 0) {
					printf("�����IP�Ƿ�������������\n");
				}
				else break;
			} while (1);

			printf("\n");

			// ǿ��ת��Ϊu_char����
			dest_ip[0] = tmp_ip.s_addr & 0xff;
			dest_ip[1] = (tmp_ip.s_addr >> 8) & 0xff;
			dest_ip[2] = (tmp_ip.s_addr >> 16) & 0xff;
			dest_ip[3] = (tmp_ip.s_addr >> 24) & 0xff;

			if (type == TYPE_TCP) {
				printf("������Ŀ�Ķ˿ڣ�");
				scanf("%d", &port);
				printf("\n");
			}

			printf("������Ŀ��MAC��ַ���������뵱ǰ���ص�MAC��ַ����\n");
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
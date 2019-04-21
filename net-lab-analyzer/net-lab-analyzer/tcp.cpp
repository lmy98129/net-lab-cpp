#include "tcp.h"

/**
* ����TCP���ݱ��ײ�
*/
void set_tcp_hdr(u_char* packet, int port) {
	tcp = (tcp_hdr*)(packet + sizeof(eth_hdr) + sizeof(ip_hdr));
	srand(time(0));
	// Դ�˿���������ɵ�
	tcp->sport = htons(rand() % 65535);
	tcp->dport = htons(port);
	// TCP�ײ����õ��
	tcp->head_len = 0x50;
	// ACK=0, SYN=1
	tcp->flags = 0x02;
	tcp->wind_size = htons(0xffff);
}

/**
* ����TCP���ݰ�
*/
int tcp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, u_char* dest_mac, int port, char* errbuf) {
	u_char packet[TCP_LEN] = { 0 };
	// ����MAC֡�ײ���ԴMACΪ��������MAC��ַ��Ŀ��MACΪ����MAC
	if (set_mac_hdr(device, dest_mac, NULL, (u_short)0x0800, packet) == 1) {
		printf("set_mac_hdr - ����MAC֡�ײ�����: %s (errno: %d)\n", strerror(errno), errno);
		system("pause");
		return 1;
	}
	// ����IP���ݱ��ײ���ע���ʱû��У���
	if (set_ip_hdr(device, 0x06, NULL, dest_ip, packet, NULL) == 1) {
		printf("�������\n");
		system("pause");
		return 1;
	}
	// ����IP���ݱ���У���
	u_short crc = check_sum((u_short*)(packet + sizeof(eth_hdr)), sizeof(ip_hdr));
	if (crc != NULL) ip->crc = crc;

	// ����TCP���ݱ��������α�ײ�����У���
	set_tcp_hdr(packet, port);
	tcp->check_sum = tcp_chksum();

	printf("��ǰ���ݰ����������£�\n");
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
	printf("���ù���������%s\n\n", packet_filter);

	// ��ʼ����ǰ������׼����ʼ����
	/*
	* ֮�����ڷ���֮ǰ�Ϳ�ʼ��ʼ������͹��ˣ�
	* ��Ϊ�˴�һ����ǰ�������ݰ��շ��Ͽ������ڳ�ʼ������Ĺ����д���Է��Ļظ�
	*/
	if (init_capture(device, adhandle, ipaddress, ipmask, errbuf) <= 0) {
		printf("�������\n");
		return 1;
	}

	// ���ù�����
	if (set_filter(adhandle, packet_filter, ipmask, errbuf) <= 0) {
		printf("�������\n");
		return 1;
	}

	if (pcap_sendpacket(*adhandle, packet, TCP_LEN) != 0) {
		printf("arp_sender - ����TCP���ݰ�����: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("����TCP���ݰ��ɹ�\n\n");
	}

	// ��ʼʹ���첽�ص���ʽץȡ���ݰ�
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - ץȡ���ݰ�����: %s\n", errbuf);
		return 1;
	}

	system("pause");

	return 0;
}
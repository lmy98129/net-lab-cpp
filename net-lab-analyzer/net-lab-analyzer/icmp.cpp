#include "icmp.h"

// ICMP�����ֶ�
// �������
#define ICMP_ECHO_REQUEST 8   
// ����Ӧ��
#define ICMP_ECHO_REPLY 0     
// ���䳬ʱ
#define ICMP_TIMEOUT 11      

// ICMP����Ĭ�������ֶγ���
#define DEF_ICMP_DATA_SIZE 10    
// ICMP������󳤶ȣ�������ͷ��
#define MAX_ICMP_PACKET_SIZE 1024 
// ����Ӧ��ʱʱ��
#define DEF_ICMP_TIMEOUT 3000   
// �����վ��
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
	// ����MAC֡�ײ���ԴMACΪ��������MAC��ַ��Ŀ��MACΪ����MAC
	if (set_mac_hdr(device, dest_mac, NULL, (u_short)0x0800, packet) == 1) {
		printf("set_mac_hdr - ����MAC֡�ײ�����: %s (errno: %d)\n", strerror(errno), errno);
		system("pause");
		return 1;
	}

	// ����IP���ݱ��ײ���ע���ʱû��У���
	if (set_ip_hdr(device, 0x01, NULL, dest_ip, packet, 38) == 1) {
		printf("�������\n");
		system("pause");
		return 1;
	}

	// ����IP���ݱ���У���
	u_short crc = check_sum((u_short*)(packet + sizeof(eth_hdr)), sizeof(ip_hdr));
	if (crc != NULL) ip->crc = crc;
	
	// ����ICMP���ݱ���������У���
	set_icmp_hdr(packet);
	memset(packet + sizeof(eth_hdr) + sizeof(ip_hdr) + sizeof(icmp_hdr), 'E', DEF_ICMP_DATA_SIZE);
	icmp->checksum = check_sum((u_short*)(packet + sizeof(eth_hdr) + sizeof(ip_hdr)), sizeof(icmp_hdr)+DEF_ICMP_DATA_SIZE);
	printf("icmp check sum: %x\n", ntohs(icmp->checksum));

	printf("��ǰ���ݰ����������£�\n");
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
	printf("���ù���������%s\n\n", packet_filter);

	// ��ʼ����ǰ������׼����ʼ����
	/*
	* ֮�����ڷ���֮ǰ�Ϳ�ʼ��ʼ������͹��ˣ�
	* ��Ϊ�˴�һ����ǰ����ARP���շ��Ͽ������ڳ�ʼ������Ĺ����д���Է��Ļظ�
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

	if (pcap_sendpacket(*adhandle, packet, ICMP_LEN) != 0) {
		printf("arp_sender - ����ICMP���ݰ�����: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("����ICMP���ݰ��ɹ�\n\n");
	}

	// ��ʼʹ���첽�ص���ʽץȡ���ݰ�
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - ץȡ���ݰ�����: %s\n", errbuf);
		return 1;
	}

	system("pause");

	return 0;
}
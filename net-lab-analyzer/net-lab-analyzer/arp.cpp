#include "arp.h"

/**
* ����ARP�ײ�
*/
int set_arp_hdr(char* device, u_char* sha, u_char* spa, u_char* tha, u_char* tpa, u_char* packet) {
	arp = (arp_hdr *)(packet + sizeof(eth_hdr));

	arp->htype = htons(0x0001);
	// Ӳ�����ͣ�ethernet 0x0001
	arp->ptype = htons(0x0800);
	// Э�����ͣ�IPv4 0x0800
	arp->hlen = 0x06;
	// Ӳ����ַ���ȣ�MAC 6
	arp->plen = 0x04;
	// Э���ַ����: IP 4
	arp->oper = htons(0x0001);
	// arp����0x0001
	memcpy(arp->sha, sha, sizeof(arp->sha) / sizeof(u_char));
	// ԴMAC��ַ
	if (spa != NULL) {
		memcpy(arp->spa, spa, sizeof(arp->spa) / sizeof(u_char));
		// ԴIP��ַ
	}
	else {
		if (get_local_ip(device, arp->spa) == 1) {
			return 1;
		}
	}
	for (int i = 0; i<6; i++) {
		arp->tha[i] = (u_char)0x00;
		// Ŀ��MAC��ַ
	}
	if (tpa != NULL) {
		memcpy(arp->tpa, tpa, sizeof(arp->tpa) / sizeof(u_char));
		// Ŀ��IP��ַ
	}
	else {
		for (int i = 0; i<6; i++) {
			arp->tpa[i] = (u_char)0x00;
		}
	}

	return 0;
}

/**
* ����ARP���ݰ�
*/
int arp_sender(char* device, pcap_t** adhandle, u_char* dest_ip, char* errbuf) {
	u_char packet[ARP_LEN] = { 0 };
	// ����MAC֡�ײ�����Ϊ������֡������ԴMACΪ��������MAC��ַ��Ŀ��MACΪȫ1
	if (set_mac_hdr(device, NULL, NULL, (u_short)0x0806, packet) == 1) {
		return 1;
	}
	// ����ARP�ײ�
	if (set_arp_hdr(device, ethernet->src_mac, NULL, ethernet->dst_mac, dest_ip, packet) == 1) {
		return 1;
	}

	printf("��ǰ���ݰ����������£�\n");
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

	if (pcap_sendpacket(*adhandle, packet, ARP_LEN) != 0) {
		printf("arp_sender - ����ARP���ݰ�����: %s (errno: %d)\n", strerror(errno), errno);
		return 1;
	}
	else {
		printf("����ARP���ݰ��ɹ�\n\n");
	}

	printf("��ʼ�����Է��Ƿ�ظ�\n\n");

	// ��ʼʹ���첽�ص���ʽץȡ���ݰ�
	if (pcap_loop(*adhandle, 0, packet_handler, (u_char *)&id) < 0) {
		printf("pcap_loop - ץȡ���ݰ�����: %s\n", errbuf);
		return 1;
	}

	return 0;
}
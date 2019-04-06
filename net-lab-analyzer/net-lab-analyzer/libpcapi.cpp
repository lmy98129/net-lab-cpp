#include "libpcapi.h"
#include "hdr.h"
//�Զ�����ײ��ṹ�壬ͬʱҲͨ������in.h�����u_��������

// ���Ĵ�ӡ�������
#define MAX_LEN 65535
// ��ӡ���������
char data_buffer[MAX_LEN];

// ����ļ���
#define MAX_FILENAME 50
// �ļ���������������ȷ���Ƿ���Ҫ�����µ���־�ļ���
char filename[MAX_FILENAME];

// �����豸����
#define ADAPTERNUM 15

// ʱ����ַ���
#define TIME_LEN 50
char time_str[TIME_LEN] = "\0";
const char* time_format = "%Y-%m-%d:%H:%M:%S";

// IP��������
#define IP_TOS_TCP 0

// ʵ���������ݱ��ײ��ṹ��ָ��
eth_hdr* ethernet;
ip_hdr* ip;
tcp_hdr* tcp;
udp_hdr* udp;
icmp_hdr* icmp;
arp_hdr* arp;
tcp_psd_hdr* tcp_psd;

/**
* ��ӡ�����豸�б�
*/
int list_device(pcap_if_t** alldevs, char* errbuf) {
	// �豸��������
	int i = 0;
	// �豸�ṹ���б������ָ��
	pcap_if_t *d;

	// ʹ��pcap����ȫ���豸
	if (pcap_findalldevs_ex((char *)PCAP_SRC_IF_STRING, NULL, alldevs, errbuf) == -1) {
		return -1;
	}

	printf("��ǰ�����б����£�\n");
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
* ��ȡ���ݰ������������Բ鿴�����ϵ����ݰ������Ϣ��������Ҫ�鿴��ǰ�����豸��Ϣ
* 65535��������ݰ�����
* 1������ģʽ������Ŀ�ĵ��Ƿ��ǵ�ǰ���������н���
* 1000������������ʱ��ms��
*/

int init_device(char* device, pcap_t** adhandle, char* errbuf) {

	*adhandle = pcap_open(device, 65536, PCAP_OPENFLAG_PROMISCUOUS, 1000, NULL, errbuf);

	if (*adhandle == NULL) {
		printf("pcap_open_live - ��ȡ��ǰ�������ݰ���������������: %s\n", errbuf);
		if (strstr(errbuf, "Operation not permitted")) {
			printf("��ʹ�ù���Աģʽ��sudo��������������\n");
		}
		return 1;
	}

	return 0;
}

/**
* ��ʼ����ǰ������׼����ʼ����
*/
int init_capture(char* device, pcap_t** adhandle, bpf_u_int32* ipaddress, bpf_u_int32* ipmask, char* errbuf) {

	struct in_addr addr;
	char *dev_ip, *dev_mask;

	if (pcap_lookupnet(device, ipaddress, ipmask, errbuf) == -1) {
		printf("pcap_lookupnet - ��ȡ����������ź������������: %s\n", errbuf);
		return -2;
	}

	return 1;
}

/**
* ���ù�����
*/
int set_filter(pcap_t** adhandle, char* packet_filter, bpf_u_int32* ipmask, char* errbuf) {
	// fcodeΪ�������ṹ��
	struct bpf_program fcode;

	// �������ַ���packet_filter�����������Ϣ
	if (pcap_compile(*adhandle, &fcode, packet_filter, 1, *ipmask) < 0) {
		printf("pcap_compile - ���������Ϣ����: %s\n", errbuf);
		return -1;
	}

	if (pcap_setfilter(*adhandle, &fcode) < 0) {
		printf("pcap_setfilter - ���ù���������: %s\n", errbuf);
		return -1;
	}

	return 1;
}

/**
*  �������ݰ�������ݰ����з����Ļص�����
*/
void packet_handler(u_char* arg, const struct pcap_pkthdr* pkt_header, const u_char* pkt_data) {
	int* id = (int *)arg;
	snprintf(data_buffer, sizeof(char)*MAX_LEN, "���ݰ�id = %d\n", ++(*id));

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "���ݰ�����: %d\n", pkt_header->len);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "�ֽڳ���: %d\n", pkt_header->caplen);

	// ʱ�����ʽת��
	time_t t = pkt_header->ts.tv_sec;
	struct tm *p = localtime(&t);
	strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", p);

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "����ʱ��: %s\n", time_str);

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

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "���ݰ���Ϣ: \n\n");

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "MAC֡�ײ���Ϣ: \n");

	ethernet = (eth_hdr *)pkt_data;
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ԴMAC��ַ: %02x-%02x-%02x-%02x-%02x-%02x\n", ethernet->src_mac[0], ethernet->src_mac[1], ethernet->src_mac[2], ethernet->src_mac[3], ethernet->src_mac[4], ethernet->src_mac[5]);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ��MAC��ַ: %02x-%02x-%02x-%02x-%02x-%02x\n", ethernet->dst_mac[0], ethernet->dst_mac[1], ethernet->dst_mac[2], ethernet->dst_mac[3], ethernet->dst_mac[4], ethernet->dst_mac[5]);
	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Э�����ͺ�: %04x\n\n", ntohs(ethernet->eth_type));

	if (ntohs(ethernet->eth_type) == 0x0800) {
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ʹ��IPv4Э��\n");
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "IPv4���ݱ��ײ���Ϣ:\n");
		ip = (ip_hdr*)(pkt_data + eth_len);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ԴIP��ַ: %d.%d.%d.%d\n", ip->sourceIP[0], ip->sourceIP[1], ip->sourceIP[2], ip->sourceIP[3]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ��IP��ַ: %d.%d.%d.%d\n\n", ip->destIP[0], ip->destIP[1], ip->destIP[2], ip->destIP[3]);

		if (ip->protocol == 6) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "TCP���ݰ��ײ���Ϣ:\n");
			tcp = (tcp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Դ�˿�: %d\n", ntohs(tcp->sport));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ�Ķ˿�: %d\n", ntohs(tcp->dport));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "˳���: %x\n", ntohl(tcp->seq));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ȷ�Ϻ�: %x\n", ntohl(tcp->ack));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ͷ������: %u\n", tcp->head_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "������: %u\n", tcp->flags);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "���ڴ�С: %d\n", ntohs(tcp->wind_size));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "У���: %04x\n", ntohs(tcp->check_sum));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "����ָ��: %d\n", ntohs(tcp->urg_ptr));
		}
		else if (ip->protocol == 17) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "UDP���ݰ��ײ���Ϣ:\n");
			udp = (udp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Դ�˿�: %u\n", udp->sport);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ�Ķ˿�: %u\n", udp->dport);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "���ݰ�����: %u\n", udp->tot_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "У���: %u\n", udp->check_sum);
		}
		else if (ip->protocol == 1) {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ICMP���ݰ��ײ���Ϣ:\n");
			icmp = (icmp_hdr*)(pkt_data + eth_len + ip_len);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "����: %u\n", icmp->type);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "����: %u\n", icmp->code);
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "У���: %04x\n", ntohs(icmp->checksum));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "��ʶ��: %d\n", ntohs(icmp->id));
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "���к�: %d\n", ntohs(icmp->sequence));
		}
		else {
			snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ʹ���������Ĵ����Э��\n");
		}

	}
	else if (ntohs(ethernet->eth_type) == 0x0806) {
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ʹ��ARPЭ��\n");
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ARP���ݱ��ײ���Ϣ:\n\n");
		arp = (arp_hdr*)(pkt_data + eth_len);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ӳ�����ͺ�: %u\n", ntohs(arp->htype));
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Э�����ͺ�: %u\n", arp->ptype);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "MAC��ַ����: %u\n", arp->hlen);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "IP��ַ����: %u\n", arp->plen);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "��������: %u\n", arp->oper);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ԴMAC��ַ: %02x-%02x-%02x-%02x-%02x-%02x\n", arp->sha[0], arp->sha[1], arp->sha[2], arp->sha[3], arp->sha[4], arp->sha[5]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "ԴIP��ַ: %d.%d.%d.%d\n", arp->spa[0], arp->spa[1], arp->spa[2], arp->spa[3]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ��MAC��ַ: %02x-%02x-%02x-%02x-%02x-%02x\n", arp->tha[0], arp->tha[1], arp->tha[2], arp->tha[3], arp->tha[4], arp->tha[5]);
		snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "Ŀ��IP��ַ: %d.%d.%d.%d\n", arp->tpa[0], arp->tpa[1], arp->tpa[2], arp->tpa[3]);
	}

	snprintf(data_buffer + strlen(data_buffer), sizeof(char)*MAX_LEN, "\n\n");
	printf("%s", data_buffer);

	write_log(data_buffer);

}

/**
* ��־�ļ���¼
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
* ͨ�����������б��ȡ��ǰ������MAC��ַ
*/
int get_local_mac(char* device, u_char* src_mac) {
	// �����б�
	PIP_ADAPTER_INFO pipAdapterInfo = new IP_ADAPTER_INFO[ADAPTERNUM];
	u_long stSize = sizeof(IP_ADAPTER_INFO)*ADAPTERNUM;
	int nRel = GetAdaptersInfo(pipAdapterInfo, &stSize);

	// �ռ䲻��
	if (ERROR_BUFFER_OVERFLOW == nRel) {
		// �ͷſռ�
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
* ͨ��socket���ȡ��ǰ������IP��ַ
*/
int get_local_ip(char* device, u_char* src_ip) {
	// �����б�
	PIP_ADAPTER_INFO pipAdapterInfo = new IP_ADAPTER_INFO[ADAPTERNUM];
	u_long stSize = sizeof(IP_ADAPTER_INFO)*ADAPTERNUM;
	int nRel = GetAdaptersInfo(pipAdapterInfo, &stSize);

	// �ռ䲻��
	if (ERROR_BUFFER_OVERFLOW == nRel) {
		// �ͷſռ�
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
				printf("get_local_ip - ת��IP��ʽ����: %s (errno: %d)\n", strerror(errno), errno);
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
* ����MAC֡�ײ�
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
*	����У���
*/
u_short check_sum(u_short* packet, int size) {
	unsigned long cksum = 0;
	while (size > 1) {
		cksum += *packet++;
		size -= sizeof(u_short);
	}
	if (size) {
		// ����Ϊ���������
		cksum += *(u_short*)packet;
	}
	// ����λ�Ľ�λ�ӵ���λȥ
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	
	// ���Ҫȡ��
	return (u_short)(~cksum);
}

/**
* ����IP���ݱ��ײ�
*/
int set_ip_hdr(char* device, u_char proto , u_char* sourceIP, u_char* destIP,  u_char* packet, int tlen) {
	ip = (ip_hdr*)(packet + sizeof(eth_hdr));
	// �汾�Լ��ײ�����
	ip->ver_ihl = 0x45;
	// �������ͣ���ͨ��TCP/ICMP��ѯ
	ip->tos = 0x00;
	// �ܳ��ȣ�Ĭ������Ϊ40
	if (tlen == NULL) tlen = 40;
	ip->tlen = htons(tlen);
	// ��ʶ������������Ϊ0x0010
	ip->identification = htons(0x0000);
	// ����ʱ�䣬��������Ϊ255
	ip->ttl = 0xff;
	// Э�飬��TCP/UDP/ICMP�ȣ�������������
	ip->protocol = proto;
	if (sourceIP != NULL) {
		memcpy(ip->sourceIP, sourceIP, sizeof(ip->sourceIP) / sizeof(u_char));
		// ԴIP��ַ
	} else {
		if (get_local_ip(device, ip->sourceIP) == 1) {
			printf("set_ip_hdr - ����IP���ݱ��ײ�����: %s (errno: %d)\n", strerror(errno), errno);
			return 1;
		}
	}
	if (destIP == NULL) return 1;
	else memcpy(ip->destIP, destIP, sizeof(ip->destIP) / sizeof(u_char));
	// Ŀ�ĵ�ַ

	return 0;
}

//UDP/TCPУ��ͳ���
//У��UDP/TCP�ײ�������Ҫ��װһ��α�ײ���Ȼ����У�飬У�鳤�Ȱ���ͷ�����ݲ���
//���len��ָ��ȥα�ײ���TCP��ԭ���ĳ���
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

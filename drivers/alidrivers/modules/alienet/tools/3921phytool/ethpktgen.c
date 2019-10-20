#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <netinet/in.h>//originally, it is net inet.

#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>   /* The L2 protocols */

#include "ethpktgen.h"

//unsigned char tcp_buf[TCP_SIZE];	
//unsigned char udp_buf[UDP_SIZE];

#define RESEND_TIMES 100000
#define ONE_SECOND   1000000

#define MY_SRC_ADDR "192.168.3.1"
#define HIS_DST_ADDR "192.168.3.78"
#define u32 unsigned int
#define u16 unsigned short 
#define u8 unsigned char 

u_int8_t LL_dell[6] = { 0x18, 0x03, 0x73, 0x58, 0xdb, 0xd4 };
u_int8_t LL_fpga[6] = { 0x66, 0x97, 0x65, 0x21, 0x1c, 0xcd };
u_int8_t LL_vb[6] = { 0x08, 0x00, 0x27, 0xe8, 0x2b, 0xa4 };

char IP6_dell[]={"2001:ac1d:2d64:1::3"};
char IP6_fpga[]={"2001:ac1d:2d64:1::1"};
char IP6_vb[]={"2001:ac1d:2d64:1::33"};
char IP6_rt[]={"2001:ac1d:2d64:1::99"};

char hop_dest_ext_hdr[]={0x0, 0x3, 0x1, 0x2, 0x0, 0x0, 0x2a, 0xc, 0x78, 0x56, 0x34, 0x12, 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x1, 0x0, 0x2b, 0x7, 0x1, 0x31, 0x13, 0x4, 0x3, 0x2, 0x1, 0x0};

typedef struct _input_param_t {
    int intl_time;
    int pkt_size;
    u32 send_times;
} input_param_t;

#define OPT_X	42
#define OPT_Y	43

#define CHECK()	assert(currentlen != -1)
//#define CHECK() printf("currentlen = %d, extlen = %d\n", currentlen, extlen);
void *extbuf;
socklen_t extlen = 0;

static u16 csum(u16 *buf, int len)
{
    int nleft = len;
    u16 * w = buf;
    u32 sum = 0;
    u16 answer = 0;
    
    while(nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(u8*)(&answer) = *(u8 *)w;
        sum += answer;
    }
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (u16)(~sum);
}

static unsigned short csum_2(unsigned long sum0, unsigned short *buf, int nwords)
{
	unsigned long sum;
		
	for (sum = sum0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return sum;
}

char * tolower_str (char * str) {
	int len;
	int i;
	len = strlen(str);
	for (i=0; i<len; i++) {
		str[i] = tolower(str[i]);
	}
	return str;
}

void init_input_param(input_param_t * param) {
    memset(param, 0, sizeof(input_param_t));
    param->intl_time = 10;
    param->pkt_size = 1500;
    param->send_times = 100000;
    return;
}
int parse_param (int argc, char **argv, input_param_t * param) {
    u32 intl_time = 0;
    int pkt_size = 0;
    u32 send_times = 0;  
    init_input_param(param);
	argc--; argv++;  /* skip app name */
	while (argc>0) {
		*argv = tolower_str(*argv);
		if (!strcmp(*argv, "-i")) { /* pkts interval time */
            argc--; argv++;
            intl_time = strtol(*argv, NULL, 0);
            if(intl_time < 10) {
                intl_time = 10;
                printf("intl_time too small, set to default value 10us\n");
            } 
            param->intl_time = intl_time;
        } else if (!strcmp(*argv, "-size")) { /* pkts size */
            argc--; argv++;
            pkt_size = strtol(*argv, NULL, 0);
            if(pkt_size < 50) {
                printf("pkt_size %d too small, at least 50, set pkt_size to 50\n", pkt_size);
                pkt_size = 50;
            } 
            if(pkt_size > 1500) {
                printf("pkt_size %d too large, at most 1500, set pkt_size to 1500\n", pkt_size);
                pkt_size = 1500;
            } 
            param->pkt_size = pkt_size;
        } else if (!strcmp(*argv, "-t")) { /* send times, default 10000 */
            argc--; argv++;
            send_times = strtol(*argv, NULL, 0);
            param->send_times = send_times; 
        }
        --argc; argv++;
    }
    return 0;
}

int ethpktgen_entry(int argc, char **argv) {
	struct sockaddr_in sin4, din4;
	struct sockaddr_in6 sin6, din6;
	struct in6_addr addr;
	char *host, *port;
    input_param_t input_param;
	int err;
	int ret, fd, i;
	unsigned long tmp, reconnect_cnt = 0;
	int time_cnt = 0;
	time_t cur_time;
	char *str_time;
	char datagram[UDP_SIZE];	
	char eth_pkt[ETH_SIZE];
    int ip_hdr_size = 0;
    int print_intl = 0;
    int send_size = 0;
    int is_rand2_pkt = 0;

	int direction_type;		
	
	u_int8_t *LL_dst;
	u_int8_t *LL_src;
	
	struct ethhdr	*pl2llhdr;
	struct iphdr 	*pl3ip4hdr;
	struct udphdr	*pl4udphdr;
	unsigned long hdr_offset = 0;
	struct sockaddr_ll sll;
	
    memset(&input_param, 0, sizeof(input_param));

	fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (fd == -1) {		
		perror("socket created error!\n");		
		exit(1);	
	}

	memset(&sll, 0, sizeof(sll)); 
	sll.sll_family = AF_PACKET;
	sll.sll_halen = ETH_ALEN;
	sll.sll_ifindex = if_nametoindex(argv[1]);

	LL_src = LL_fpga;
	LL_dst = LL_dell;	
	
	memcpy(&sll.sll_addr, LL_dst, ETH_ALEN);

	memset(eth_pkt, 0, sizeof(eth_pkt)); 
	if(!strcmp(argv[3], "payload_1")) {
		memset(eth_pkt, 0x01, sizeof(eth_pkt)); 
	} else if(!strcmp(argv[3], "payload_ff")) {
		memset(eth_pkt, 0xff, sizeof(eth_pkt)); 	
	} else if(!strcmp(argv[3], "payload_rand")) { 
		srand(100);
		for(i = 0; i < ETH_SIZE; i ++) {
			eth_pkt[i] = rand();;
		}	
	} else if (!strcmp(argv[3], "payload_rand2")) {
        is_rand2_pkt = 1;
		srand(100);
		for(i = 0; i < ETH_SIZE; i ++) {
			eth_pkt[i] = rand();;
		}	
    } else {
		printf("Oops...wrong param! correct usage is:\n");
		printf("%s eth0 -pktgen [payload_1|payload_ff|payload_rand|payload_rand2]\n", argv[0]);
		exit(1);
	}		

    parse_param(argc, argv, &input_param);
    /* pkt_size only include ip header, not include ether mac head */
	//step1. l2llhdr
	pl2llhdr = (struct ethhdr *)eth_pkt;

	memcpy(&pl2llhdr->h_dest, LL_dst, ETH_ALEN);
	memcpy(&pl2llhdr->h_source, LL_src, ETH_ALEN);
	pl2llhdr->h_proto = 0x08;

	//step2. l3ip4hdr
	hdr_offset = sizeof(struct ethhdr);
	pl3ip4hdr = (struct iphdr *)(eth_pkt + hdr_offset);
	pl3ip4hdr->ihl  = 5;
	pl3ip4hdr->version  = 4;
	pl3ip4hdr->tos  = 0;
	pl3ip4hdr->tot_len  = htons(input_param.pkt_size);//tbd
	pl3ip4hdr->id  = htonl (54321);	
	pl3ip4hdr->frag_off = 0;
	pl3ip4hdr->ttl  = 255;
	pl3ip4hdr->protocol  = 17;
	pl3ip4hdr->check = 0;

	inet_pton(AF_INET, "192.168.3.3", &pl3ip4hdr->saddr);
	inet_pton(AF_INET, "192.168.3.1", &pl3ip4hdr->daddr);	
	
    pl3ip4hdr->check = csum((unsigned short*)pl3ip4hdr, 4*pl3ip4hdr->ihl);

	//step3. l4udphdr
    ip_hdr_size = sizeof(struct iphdr);
	hdr_offset += ip_hdr_size;
	pl4udphdr = (struct udphdr *)(eth_pkt + hdr_offset);
	pl4udphdr->source = htons(8877);
	pl4udphdr->dest= htons(7788);
	pl4udphdr->len = htons(input_param.pkt_size - ip_hdr_size);
	pl4udphdr->check = 0; //tbd, let hardware do it.

	if(strcmp(argv[3], "payload_1")==0) {
		pl4udphdr->check = htons(0x490d);
	} else if(strcmp(argv[3], "payload_rand")==0) {
		pl4udphdr->check = htons(0xdeba);
	}
	
    printf("pkt_size %d intl_time %d send_times %d\n", input_param.pkt_size, input_param.intl_time, input_param.send_times);
    send_size = input_param.pkt_size + sizeof(struct ethhdr);
	for (reconnect_cnt = 0; reconnect_cnt < (input_param.send_times); ++reconnect_cnt) {
        if(is_rand2_pkt) {
            eth_pkt[send_size-1] += 1;
        }
		if(-1 == sendto(fd, eth_pkt, send_size, 0, (struct sockaddr *)&sll, sizeof(struct sockaddr_ll))) {
			close(fd);
			NET_TEST_PRINTF("Data sending failed. Have send %d pkts\n", reconnect_cnt); 
			return;
		} else {
            if(!((reconnect_cnt + 1) % 100)) {
				NET_TEST_PRINTF("Send pkt %d.\n", reconnect_cnt+1);
            }
			usleep(input_param.intl_time);
		}
	}
    NET_TEST_PRINTF("Totally send %d pkts\n", reconnect_cnt);
	close(fd);
    return 0;
}


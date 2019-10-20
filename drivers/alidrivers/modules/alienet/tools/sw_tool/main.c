#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
//#include <conio.h> 
#include <linux/sockios.h>
#include <linux/types.h>
#include <arpa/inet.h>

#define REG_READ_CMD		(SIOCDEVPRIVATE+1)
#define REG_WRITE_CMD		(SIOCDEVPRIVATE+2)
#define ALR_READ_CMD		(SIOCDEVPRIVATE+3)
#define ALR_WRITE_CMD		(SIOCDEVPRIVATE+4)
#define ALIREG_CMD          (SIOCDEVPRIVATE+5)
#define VLUT_READ_CMD		(SIOCDEVPRIVATE+7) /* use ifindex to differ VLUT and VPORT */
#define VLUT_WRITE_CMD	(SIOCDEVPRIVATE+6)
#define VPORT_READ_CMD	(SIOCDEVPRIVATE+7)
#define VPORT_WRITE_CMD	(SIOCDEVPRIVATE+8)
#define DBG_SETTING_CMD	(SIOCDEVPRIVATE+9)
#define EGRS_RATE_CMD		(SIOCDEVPRIVATE+10)
#define IGRS_RATE_CMD		(SIOCDEVPRIVATE+11)
#define FLOW_CTL_CMD		(SIOCDEVPRIVATE+12)
#define BAND_RATE_CMD		(SIOCDEVPRIVATE+13)
#define ACL_CTL_CMD		(SIOCDEVPRIVATE+14)
#define STP_CTL_CMD		(SIOCDEVPRIVATE+15)
#define MISC_CMD			(SIOCDEVPRIVATE+0)		//i run out of cmd.... :(


#define SW_PORT_MAX_NUM			(4)
#define SW_VPORT_TYPE				(4)
#define VLUT_TOTAL_ENTRY_NUM 		(16)
#define ALR_TOTAL_ENTRY_NUM		(68)
#define ALR_SHOW_ALL_ENTRY		(255)

//#define CLOCK_FREQ_HZ		(20) //means 20 MHz
int clock_freq_hz = 20;

struct reg_param {

    unsigned char   mux;
    unsigned short  addr;
    unsigned long   val;
};

struct alr_param {

	unsigned long mac_addr_high32;
	unsigned short mac_addr_low16;
	unsigned char ports:4;
	unsigned char filter_flag:1;
	unsigned char static_flag:1;
	unsigned char valid:1;
	unsigned char idx;

};

struct vlut_param {
	unsigned long vid:12;
	unsigned long port0_member:1;
	unsigned long port1_member:1;
	unsigned long port2_member:1;
	unsigned long port3_member:1;	
	unsigned long port0_tag:1;
	unsigned long port1_tag:1;
	unsigned long port2_tag:1;
	unsigned long port3_tag:1;
	unsigned long idx: 12;	
};

struct vport_param
{
	unsigned char port_no;
	unsigned char ppriority;
	unsigned char port_type;
	unsigned short pvid;
	unsigned char fff_chg:1;
	unsigned char igrs_filter:1;
	unsigned char tag_chg:1;
	unsigned char tag_only:1;
	unsigned char untag_only:1;
};

#define DEBUG_DRIVER_LEVEL			0
#define DISPLAY_MC_SNOOPING_LIST	1
struct dbg_param
{
	unsigned char dbg_no;
	unsigned char dbg_val;
	unsigned char dbg_cmd;
};

#define STP_OPERATION_DISABLE		0
#define STP_OPERATION_ENABLE		1
#define STP_OPERATION_INFO		2
#define STP_SET_PRIORITY			3
struct stp_param
{
	unsigned char stp_op;
	unsigned short priority_val;
};

#define EGRS_QUEUE0		0
#define EGRS_QUEUE1		1
#define EGRS_QUEUE2		2
#define EGRS_QUEUE3		3
#define EGRS_PORT		4
#define EGRS_NONE		5

struct egrs_param{
	unsigned char 	port_no;
	unsigned char 	mode;
	unsigned short 	scale;
};

#define IGRS_PORT		9
#define IGRS_NONE		10

#define PORT0_EN			1
#define PORT1_EN			2
#define PORT2_EN			4
#define PORT3_EN			8

#define IGRS_DSCP_EN			1
#define IGRS_ACL_EN			2
#define IGRS_VLAN_EN			4


struct igrs_param{
	unsigned char			priority_mode;
	unsigned char			igrs_mode;
	unsigned short 		p0_scale;
	unsigned long 		p1_cbs;
	unsigned long 		p1_ebs;	
	unsigned long 		p23_cbs;
	unsigned long 		p23_ebs;	
	unsigned short		p1_tc[16];
	unsigned short		p2_tc[16];
	unsigned short		p3_tc[16];
	unsigned char 		ports_enable;		
};

struct bandrate_param{
	unsigned char 	port_no;
	unsigned short 	interval;
};


#define ACL_WRITE_ENTRY	1
#define ACL_READ_ENTRY		0
#define ACL_MODE_SRC_SRC 	4
#define ACL_MODE_SRC_MSK	0
#define ACL_MODE_DST_DST	5
#define ACL_MODE_DST_MSK	1
#define ACL_PROTO_TCP	1
#define ACL_PROTO_UDP	2
#define ACL_PROTO_TCP_UDP	3
struct acl_param{
	unsigned char 	acl_proto;
	unsigned char 	acl_command;
	unsigned char 	acl_mode;
	unsigned char 	acl_idx;
	unsigned char 	ports_enable;
	unsigned char 	priority_sel;
	unsigned long		ip1;
	unsigned long		ip2;
	unsigned short 	port1_start;
	unsigned short 	port1_end;
	unsigned short 	port2_start;
	unsigned short 	port2_end;
};

#define P123_FLOWCTL_NONE			0
#define P123_FLOWCTL_PORT_ONLY	1
#define P123_FLOWCTL_PRIOR_ONLY	2
#define P123_FLOWCTL_PORT_PRIOR	3
struct flowctl_param{
	unsigned char 	port_no;
	unsigned char 	p0_blks;
	unsigned char 	p123_mode;
};

#define SHOW_DROP_STATISTICS		0
#define SHOW_LINK_STATUS			1
#define READ_PHY_MDIO				2
#define VLAN_OPERATION				3
#define ALR_AGING_TEST				4
#define QOS_ACL_SETTING			5
#define QOS_VLAN_SETTING			6
#define QOS_DSCP_SETTING			7
#define IGRS2_DBG_SETING			8
#define ROUTE_SETTING				9
#define ALIREG_SETTING            10
#define ALIREG_GETTING            11

struct phy_mdio_ctl {

	unsigned char port_no;
	unsigned char phy_reg_addr;
};

struct alr_age
{
	unsigned char aging_type;
	unsigned long aging_time;
};

struct igrs_param2
{
	unsigned char port;
	unsigned short tc;
	unsigned long cbs;
	unsigned long ebs;
};

#define WAN_PORT_SELECTION		1
#define ROUTE_FORWARDING_TEST	2
struct route_param{
	unsigned char route_cmd;
	union{
		unsigned char wan_port;
		unsigned short forward_ports;
	}un;
};

struct misc_param{
	unsigned char misc_cmd;
	union {
		struct phy_mdio_ctl 	mdio_val;
		unsigned char 		vlan_en;
		struct alr_age			age_val;
		unsigned char			qos_en;
		struct igrs_param2		igrs_val2;
		struct route_param	route_val;
	} misc_val;	
};

#define misc_ali_reg  misc_val.ali_reg

const char* sw_tool_version = "0.0.0.1 -- by peter";
long g_argc;
char** gp_argv;

struct ifreq ifr;

/* //code backup
char* g_argv[]={"hello","peter"};
	printf("argv=0x%x, gp_argv=0x%x, g_argv=0x%x\n", argv, gp_argv, g_argv);
	printf("argv=0x%x, gp_argv=0x%x\n", argv, gp_argv);
	printf("gp_argv[0]=%s\n", gp_argv[0]);
	printf("gp_argv[1]=%s\n", gp_argv[1]);
	printf("gp_argv[2]=%s\n", gp_argv[2]);
	printf("g_argv[0]=%s\n", g_argv[0]);
	printf("g_argv[1]=%s\n", g_argv[1]);
*/

void print_cmd_usage(char* name)
{
	//printf("******************************************************\n");
	printf( "\ncorrect command show:\n\n");
	//printf( "Version: %s\n", sw_tool_version);
	
	printf( "ali register reading:	%s eth0 alireg -r32 [reg_phy_addr]\n", name);
	printf( "ali register writing:	%s eth0 alireg -w32 [reg_phy_addr] [Value]\n", name);	
	printf( "register reading:	%s eth0 reg -r32 [Access] [Register]\n", name);
	printf( "register writing:	%s eth0 reg -w32 [Access] [Register] [Value]\n", name);	
	//printf( "\n");
	//printf( "read whole alr table: %s eth0 alr -r [all]\n", name);
	printf( "alr table reading:	%s eth0 alr -r all\n", name);
//	printf( "delete whole alr table: %s eth0 alr -d [all]\n", name);
//	printf( "read alr table entry: %s eth0 alr -r [index]\n", name);
	printf( "alr table writing:	%s eth0 alr -w [mac address] [ports] [static_flag] [filter_flag]\n", name);	
	printf( "alr table deleting:	%s eth0 alr -d [mac address]\n", name);
	printf( "alr table deleting:	%s eth0 alr age [aging_time]\n", name);
	//printf( "alr table deleting:	%s eth0 alr fastaging\n", name);
	

	printf( "egress rate limit:	%s eth0 egrs [port_no] (port/q0/q1/q2/q3/none) [Mbps]\n", name);
	printf( "ingress rate limit:	%s eth0 igrs\n", name);
	printf( "ingress rate limit2:	%s eth0 igrs2 (port1/port2/port3) [cbs] [ebs] [tc]\n", name);
	
	printf( "P0 flow control:	%s eth0 flowctl 0 [block_num]\n", name);
	printf( "P123 flow control:	%s eth0 flowctl [port_no] (port/prior/both/none)\n", name);
	printf( "stp operation:		%s eth0 stp (enable/disable/info/priority) [priority_val]\n", name);
	printf( "bandrate show:		%s eth0 bandrate [time]\n", name);
	printf( "pkt drop statistic:	%s eth0 drops\n", name);
	printf( "port link status:	%s eth0 link\n", name);
	printf( "QoS setting:		%s eth0 qos (acl/vlan/dscp) (enable/disable)\n", name);
	printf( "read phy via mdio:	%s eth0 phy [port_no] [phy_reg_addr]\n", name);

	printf( "vlan enable:		%s eth0 vlan (enable/disable)\n", name);
	printf( "vlan table reading:	%s eth0 vlut -r all\n", name);
	printf( "vlan table writing:	%s eth0 vlut -w [idx] [vid] [ports] [tagged ports]\n", name);
	printf( "vlan table deleting:	%s eth0 vlut -d [idx]\n", name);
	printf( "vlan ports reading:	%s eth0 vport -r all\n", name);
	printf( "vlan ports writing:	%s eth0 vport -w [no] (dump/access/trunk/hybird) [priority] [pvid] [fff_chg] [igrs_filter] [tag_chg] [tag_only] [untag_only]\n", name);

	printf( "acl table reading:	%s eth0 acl -r all\n", name);
	printf( "acl table writing:	%s eth0 acl -w [idx] (srcsrc/srcmsk/dstdst/dstmsk) [ip1] [ip2/mask] [ip1_port1] [ip1_port2] [ip2_port1] [ip2_port2] [ports] [prior] (tcp/udp/both)\n", name);

	printf("wan port selecting:	%s eth0 wan_port [port_no]\n", name);
	printf("wan port selecting:	%s eth0 forwards [ports]\n", name);
	printf( "have some fun:		%s eth0 peter\n", name);
	
	//printf( "driver dbg level:	%s eth0 dbg [0~7] on/off\n", name);

	printf("\n*** please be noted: [ ] accepts numeric variable, () accepts strings listed in the parenthesis.  ***\n");
	printf(" [port_no] is used to input ONE port, ranges from 0 to 3.\n");
	printf(" [ports]     is used to input SEVERAL ports, for example, '1101' means port3, port2, port0, but no port1.\n");
	printf(" you could always stop the APP by typing ctrl+c");
	printf( "\n");
	//printf("******************************************************\n");
}

int process_acl_r_cmd(long skfd)
{
	struct acl_param acl_val;
	ifr.ifr_data =  (void *)&acl_val;
	char *endptr = NULL;
	
	if(g_argc!=5)
	{
		printf( "invalid command in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	if(strcmp(gp_argv[4], "all") == 0){
		acl_val.acl_idx = 0x10;
	}
	else{
		acl_val.acl_idx = strtoul(gp_argv[4], &endptr, 0);
		if(acl_val.acl_idx>0xf){
			printf( "invalid param in %s!\n", __FUNCTION__ );	
			return -1;
	 	}
	}

	acl_val.acl_command = ACL_READ_ENTRY;
	if( ioctl( skfd, ACL_CTL_CMD, &ifr ) < 0 )
	{
		printf( "acl reading failed!\n" );
		//close( skfd );
		return -1;
	}	 

}

int process_acl_w_cmd(long skfd)
{
	struct acl_param acl_val;
	ifr.ifr_data =  (void *)&acl_val;

	char *endptr = NULL;
	unsigned short sw_ports_hex;
	unsigned char acl_idx, acl_priority;
	unsigned short ip1_port1, ip1_port2, ip2_port1, ip2_port2;
	struct in_addr ip1_addr, ip2_addr;
	unsigned char acl_mode;
	unsigned char acl_proto;
	if(g_argc!=15){
		printf( "invalid command in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}

	 if(strcmp(gp_argv[5], "srcsrc") == 0)
		acl_mode = ACL_MODE_SRC_SRC;
	 else if(strcmp(gp_argv[5], "srcmsk") == 0)
		acl_mode = ACL_MODE_SRC_MSK;
	 else if(strcmp(gp_argv[5], "dstdst") == 0)
		acl_mode = ACL_MODE_DST_DST;
	 else if(strcmp(gp_argv[5], "dstmsk") == 0)
		acl_mode = ACL_MODE_DST_MSK;
	 else{
		printf( "wrong acl mode in %s!\n", __FUNCTION__ );
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}	 	

	acl_idx = atol(gp_argv[4]);
	if(acl_idx>15){
		printf( "wrong acl idx in %s!\n", __FUNCTION__ );
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	if (inet_pton(AF_INET, gp_argv[6], &ip1_addr) <= 0) {
		printf("wrong IP group address!\n");
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}	
	if (inet_pton(AF_INET, gp_argv[7], &ip2_addr) <= 0) {
		printf("wrong IP group address!\n");
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}	
	
	ip1_port1 = (atol(gp_argv[8]));
	ip1_port2 = (atol(gp_argv[9]));
	ip2_port1 = (atol(gp_argv[10]));
	ip2_port2 = (atol(gp_argv[11]));

	sw_ports_hex = strtoul(gp_argv[12], &endptr, 16);
	if(sw_ports_hex&0xeeee){
		printf("input wrong!0x%x\n", sw_ports_hex);
		return -1;
	}

	acl_priority = atol(gp_argv[13]);

	if(strcmp(gp_argv[14], "tcp") == 0)
		acl_proto = ACL_PROTO_TCP;
	else 	if(strcmp(gp_argv[14], "udp") == 0)
		acl_proto = ACL_PROTO_UDP;
	else 	if(strcmp(gp_argv[14], "both") == 0)
		acl_proto = ACL_PROTO_TCP_UDP;
	else{
		printf("wrong protocol!\n");
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}	

	if(sw_ports_hex&0x1)
		acl_val.ports_enable = PORT0_EN;
	if(sw_ports_hex&0x10)
		acl_val.ports_enable |= PORT1_EN;
	if(sw_ports_hex&0x100)
		acl_val.ports_enable |= PORT2_EN;
	if(sw_ports_hex&0x1000)
		acl_val.ports_enable |= PORT3_EN;

	acl_val.acl_mode = acl_mode;
	acl_val.acl_idx = acl_idx;
	acl_val.priority_sel = acl_priority;
	acl_val.ip1 = ntohl((unsigned long)(ip1_addr.s_addr));
	acl_val.ip2 = ntohl((unsigned long)(ip2_addr.s_addr));
	acl_val.port1_start = ip1_port1;
	acl_val.port1_end = ip1_port2;
	acl_val.port2_start = ip2_port1;
	acl_val.port2_end = ip2_port2;
	acl_val.acl_proto = acl_proto;
	acl_val.acl_command = ACL_WRITE_ENTRY;
	

#if 1
	printf("acl mode: %d\n", acl_val.acl_mode);
	printf("acl idx: %d\n", acl_val.acl_idx);
	printf("acl priority: %d\n", acl_val.priority_sel);
	printf("acl ports: 0x%x\n", acl_val.ports_enable);
	printf("acl proto: 0x%x\n", acl_val.acl_proto);
	printf("acl ip1: 0x%x\n", acl_val.ip1);
	printf("acl ip2: 0x%x\n", acl_val.ip2);
	printf("acl port1_start: 0x%x\n", acl_val.port1_start);
	printf("acl port1_end: 0x%x\n", acl_val.port1_end);
	printf("acl port2_start: 0x%x\n", acl_val.port2_start);
	printf("acl port2_end: 0x%x\n", acl_val.port2_end);
	
#endif

	if( ioctl( skfd, ACL_CTL_CMD, &ifr ) < 0 )
	{
		printf( "acl writing failed!\n" );
		//close( skfd );
		return -1;
	}

}

int process_flowctl_cmd(long skfd)
{
	struct flowctl_param* p_fc = NULL;
	p_fc = (struct flowctl_param*)&ifr.ifr_data;
	char *endptr = NULL;

	if(g_argc<5)	{
		printf( "invalid command in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	p_fc->port_no = strtoul(gp_argv[3], &endptr, 0);
	if(p_fc->port_no > SW_PORT_MAX_NUM){
		printf( "invalid command: port_no out of range in %s!\n", __FUNCTION__ );		
		return -1;
	}


	if(strcmp(gp_argv[4], "none") != 0){
		if(p_fc->port_no==0){
			p_fc->p0_blks= strtoul(gp_argv[4], &endptr, 0);
			if(p_fc->p0_blks > 0xff){
				printf( "invalid command: p0_blks out of range in %s!\n", __FUNCTION__ );		
				return -1;
			}
		}else{
			if(strcmp(gp_argv[4], "port") == 0)
				p_fc->p123_mode = P123_FLOWCTL_PORT_ONLY;
			else if(strcmp(gp_argv[4], "prior") == 0)
				p_fc->p123_mode = P123_FLOWCTL_PRIOR_ONLY;
			else if(strcmp(gp_argv[4], "both") == 0)
				p_fc->p123_mode = P123_FLOWCTL_PORT_PRIOR;
			else{
				printf("wrong protocol!\n");
				//print_cmd_usage(gp_argv[0]);			
				return -1;
			}	
		}
	}else{
		p_fc->p123_mode = P123_FLOWCTL_NONE;
		p_fc->p0_blks = 0xff;
	}
	
	if( ioctl( skfd, FLOW_CTL_CMD, &ifr ) < 0 )
	{
		printf( "bandrate setting failed!\n" );
		//close( skfd );
		return -1;
	}		
	
}

int process_bandrate_cmd(long skfd)
{
	struct bandrate_param* p_br = NULL;
	p_br = (struct bandrate_param*)&ifr.ifr_data;
	char *endptr = NULL;
	//unsigned short tmp;

	if(gp_argv[3])
		p_br->interval = strtoul(gp_argv[3], &endptr, 0);
	else
		p_br->interval = 1024;
	
	printf("br_info: time_interval=%d\n", p_br->interval);
	p_br->port_no = SW_PORT_MAX_NUM;

	if( ioctl( skfd, BAND_RATE_CMD, &ifr ) < 0 )
	{
		printf( "bandrate setting failed!\n" );
		//close( skfd );
		return -1;
	}	
}


int process_igrs_cmd_ext(long skfd)
{
	struct igrs_param igrs_val;
	ifr.ifr_data = (void*)&igrs_val;
	char *endptr = NULL;
	unsigned short tmp;
	float cal;
	//char string[256];
	char *p_str_s, *p_str_e;
	char p0_speed_str[7]={0};
	char tmp_str[7]={0};
	char speed_str[17][7]={0};
	const char * string_none = "none";
	int  str_idx, str_char_idx, input_char_idx;
	char ch, last_ch, port_no;
	float speed_val[SW_PORT_MAX_NUM][17];
	float  max_speed_p23, p0_speed;
	const   float   f   =   0.00001; 
	float x;
	unsigned long real_speed;
	//unsigned short sw_ports_hex;

/*	
	p_igrs->port_no = strtoul(gp_argv[3], &endptr, 0);
	if(p_igrs->port_no > SW_PORT_MAX_NUM)
	{
		printf( "invalid command: port_no out of range in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
*/
	
	printf("mac clock frequence is %d\n", clock_freq_hz);
	printf("\n\nplease input port0's ingress speed (Mbps): ('enter' to skip and keep default value)\n");
	str_idx = 0;
	str_char_idx =0;
	ch = 0;
	last_ch = ch;
	for(input_char_idx=0; input_char_idx<10; input_char_idx++){
		ch=getchar();
		if((ch>='0'&&ch<='9')||(ch=='.'))
		{	
			p0_speed_str[str_char_idx++]=ch;
			if(str_char_idx==7)
			{	
				printf("wrong input, speed value > 1000.0!\n");
				return -1;
			}
		}
		/*
		else if(ch==' '){
			if((last_ch==' ')||(last_ch==0))
				continue;
			speed_str[str_idx][str_char_idx]=0;
			str_idx++;
			str_char_idx=0;
			if(str_idx>17)
			{	
				printf("wrong input, only 17 speed value!\n");
				return -1;
			}
		}	*/	
		else if(ch=='\n'){
			break;
		}			
	}
	
	printf("please follow the order below to input speed (Mbps): ('enter' to skip and keep default value)\n");
	printf("port priority15 priority14 priority13 ... priority1 priority0\n");
	for(port_no=1;port_no<SW_PORT_MAX_NUM; port_no++){
		printf("input port[%d]'s ingress speed (Mbps):\n", port_no);
		
		for(str_idx=0;str_idx<17;str_idx++){
			for(input_char_idx=0; input_char_idx<7; input_char_idx++)
				speed_str[str_idx][str_char_idx]=0;
		}
		
		str_idx = 0;
		str_char_idx =0;
		ch = 0;
		for(input_char_idx=0; input_char_idx<100; input_char_idx++){
			last_ch = ch;
			ch=getchar();
			if((ch>='0'&&ch<='9')||(ch=='.'))
			{	
				speed_str[str_idx][str_char_idx++]=ch;
				if(str_char_idx==6)
				{	
					printf("wrong input, format xxxx.x, no bigger than 1000.0!\n");
					return -1;
				}
			}
			else if(ch==' '){
				if((last_ch==' ')||(last_ch==0))
					continue;
				speed_str[str_idx][str_char_idx]=0;
				str_idx++;
				str_char_idx=0;
				if(str_idx>17)
				{	
					printf("wrong input, only 17 speed value!\n");
					return -1;
				}
			}
			else if(ch=='\n'){
				if(last_ch!=' '){
					speed_str[str_idx][str_char_idx]=0;
					str_idx++;
					str_char_idx=0;
					if(str_idx>17)
					{	
						printf("wrong input, only 17 speed value!\n");
						return -1;
					}		
				}
				break;
			}
			else{
				printf("wrong input, please type numbers!\n", ch);
				return -1;
			}
		}
		
		for(;str_idx<17;str_idx++){
			strncpy(speed_str[str_idx], string_none, 4);
		}

		
		for(str_idx=0;str_idx<17;str_idx++){
			//printf("%s\n", speed_str[str_idx]);
			if(strncmp(speed_str[str_idx], string_none, 4)==0)
				speed_val[port_no][str_idx]=9999;
			else{
				speed_val[port_no][str_idx]=atof(speed_str[str_idx]);

//				speed_val[port_no][str_idx]=(float)strtoul(speed_str[str_idx], &endptr, 16);
			}
		}
		if(speed_val[port_no][0]<=0.000001&&speed_val[port_no][0]>=-0.000001)
			speed_val[port_no][0]=9999;
		
	}
	
	for(port_no=1;port_no<SW_PORT_MAX_NUM; port_no++){
		for(str_idx=1;str_idx<17;str_idx++){
			x = speed_val[port_no][str_idx] - 9999;
			if(x>=-f && x<=f)
				continue;
			if(speed_val[port_no][str_idx]>speed_val[port_no][0]){
				printf("wrong param: port[%d]'s no(%d) priority ingress speed is bigger than its port ingress speed!\n", port_no, str_idx);
				return -1;
			}
		}
	}
	
	igrs_val.p1_cbs = (unsigned long)(15625*speed_val[1][0]);
	igrs_val.p1_ebs = (igrs_val.p1_cbs)<<1;
	max_speed_p23 = (speed_val[2][0]>speed_val[3][0])?speed_val[2][0]:speed_val[3][0];
	igrs_val.p23_cbs = (unsigned long)(15625*max_speed_p23);
	igrs_val.p23_ebs = (igrs_val.p23_cbs)<<1;
	
	for(str_idx=1;str_idx<17;str_idx++){
		x = speed_val[1][str_idx] - 9999;
		if(x>=-f && x<=f)
			igrs_val.p1_tc[16-str_idx]=1;
		else
			igrs_val.p1_tc[16-str_idx]=((float)125.0*speed_val[1][0]/speed_val[1][str_idx]);

		x = speed_val[2][str_idx] - 9999;
		if(x>=-f && x<=f)
			igrs_val.p2_tc[16-str_idx]=1;
		else
			igrs_val.p2_tc[16-str_idx]=((float)125.0*max_speed_p23/speed_val[2][str_idx]);

		x = speed_val[3][str_idx] - 9999;
		if(x>=-f && x<=f)
			igrs_val.p3_tc[16-str_idx]=1;
		else
			igrs_val.p3_tc[16-str_idx]=((float)125.0*max_speed_p23/speed_val[3][str_idx]);
	}

	p0_speed=atof(p0_speed_str);
	//p0_speed=(float)strtoul(p0_speed_str, &endptr, 0);
	//printf("%f, %s\n", p0_speed, p0_speed_str);
	if(p0_speed>=-f && p0_speed<=f)	{
		p0_speed=9999;
		igrs_val.p0_scale = 1;
	}
	else{
		if(p0_speed>1000.0){
			printf("p0 speed is too big, should < 1000!, %s, %f\n", p0_speed_str, p0_speed);
			return -1;
		}

		cal = ((float)8*clock_freq_hz/p0_speed);
		igrs_val.p0_scale = (unsigned long)(cal+0.5);
		if(igrs_val.p0_scale <1)
			igrs_val.p0_scale = 1;
	}
	real_speed = (unsigned long)((float)8.0*clock_freq_hz/igrs_val.p0_scale);

	tmp_str[0]='0';
	tmp_str[1]='x';
	printf("ports ingress limit enable? eg: type '1011' means enable port3, port1, port0, disable port2\n");
	scanf("%s", &tmp_str[2]);
	tmp = strtoul(tmp_str, &endptr, 16);
	if(tmp&0xeeee){
		printf("input wrong!0x%x\n", tmp);
		return -1;
	}
	if(tmp&0x1)
		igrs_val.ports_enable = PORT0_EN;
	if(tmp&0x10)
		igrs_val.ports_enable |= PORT1_EN;
	if(tmp&0x100)
		igrs_val.ports_enable |= PORT2_EN;
	if(tmp&0x1000)
		igrs_val.ports_enable |= PORT3_EN;
	
	//printf("%s, 0x%x, %d, 0x%x\n", tmp_str, tmp, tmp, p_igrs->ports_enable);

	printf("ingress limit mode? '0' for port only, '1' for priority only, '2' for port & priority\n");
	scanf("%d", &tmp);
	if(tmp>2)
	{	
		printf("wrong input, no bigger than 2!\n");
		return -1;
	}
	igrs_val.igrs_mode = tmp;

	printf("priority mode? DSCP, ACL, VLAN. eg: type '101' means enable DSCP, VLAN, disable ACL\n");
	scanf("%s", &tmp_str[2]);
	tmp = strtoul(tmp_str, &endptr, 16);
	if(tmp&0xeeee){
		printf("input wrong!0x%x\n", tmp);
		return -1;
	}
	if(tmp&0x1)
		igrs_val.priority_mode = IGRS_DSCP_EN;
	if(tmp&0x10)
		igrs_val.priority_mode |= IGRS_ACL_EN;
	if(tmp&0x100)
		igrs_val.priority_mode |= IGRS_VLAN_EN;	

	//printf("%s, 0x%x, %d, 0x%x\n", tmp_str, tmp, tmp, p_igrs->priority_mode);


#if 1
	printf("\nthe result is...\n");
	printf("port[0]'s ingress speed:\n");
	printf("p0_speed   =%6.1f	scale=%d(cal:%f, actual speed:%d)\n", p0_speed, igrs_val.p0_scale, cal, real_speed);
	printf("\n");
	
	printf("port[1]'s ingress speed:\n");
	printf("port_spd   =%6.1f	cbs=%d	ebs=%d\n", speed_val[1][0], igrs_val.p1_cbs, igrs_val.p1_ebs);
	for(str_idx=1;str_idx<17;str_idx++)
			printf("prior%02d_spd=%6.1f	tc=%d\n", 16-str_idx, speed_val[1][str_idx], igrs_val.p1_tc[16-str_idx]);
	printf("\n");
	printf("port[2]'s ingress speed:\n");
	printf("port_spd   =%6.1f	cbs=%d	ebs=%d\n", speed_val[2][0], igrs_val.p23_cbs, igrs_val.p23_ebs);
	for(str_idx=1;str_idx<17;str_idx++)
			printf("prior%02d_spd=%6.1f	tc=%d\n", 16-str_idx, speed_val[2][str_idx], igrs_val.p2_tc[16-str_idx]);
	printf("\n");
	printf("port[3]'s ingress speed:\n");
	printf("port_spd   =%6.1f	cbs=%d	ebs=%d\n", speed_val[3][0], igrs_val.p23_cbs, igrs_val.p23_ebs);
	for(str_idx=1;str_idx<17;str_idx++)
			printf("prior%02d_spd=%6.1f	tc=%d\n", 16-str_idx, speed_val[3][str_idx], igrs_val.p3_tc[16-str_idx]);
	printf("\n");
	printf("***pls note: if speed == 9999, it simply means the default speed is applied.***\n");
	printf("ports ingress limit enable: 0x%x\n", igrs_val.ports_enable);
	printf("ports ingress limit mode: %d\n", igrs_val.igrs_mode);
	printf("priority mode: %x\n\n", igrs_val.priority_mode);
	
#endif

	if( ioctl( skfd, IGRS_RATE_CMD, &ifr ) < 0 )
	{
		printf( "igrs setting failed!\n" );
		//close( skfd );
		return -1;
	}

}
	

int process_egrs_cmd(long skfd)
{
	struct egrs_param* p_egrs = NULL;
	p_egrs = (struct egrs_param*)&ifr.ifr_data;
	char *endptr = NULL;
	float tmp;
	float cal;
	unsigned long real_speed;

	if(g_argc<5)
	{
		printf( "invalid command: port_no out of range in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	p_egrs->port_no = strtoul(gp_argv[3], &endptr, 0);
	if(p_egrs->port_no > SW_PORT_MAX_NUM)
	{
		printf( "invalid command: port_no out of range in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}

	if(strcmp(gp_argv[4], "port") == 0)
		p_egrs->mode = EGRS_PORT;
	else if(strcmp(gp_argv[4], "q0") == 0)
		p_egrs->mode = EGRS_QUEUE0;
	else if(strcmp(gp_argv[4], "q1") == 0)
		p_egrs->mode = EGRS_QUEUE1;
	else if(strcmp(gp_argv[4], "q2") == 0)
		p_egrs->mode = EGRS_QUEUE2;
	else if(strcmp(gp_argv[4], "q3") == 0)
		p_egrs->mode = EGRS_QUEUE3;
	else if(strcmp(gp_argv[4], "none") == 0)
		p_egrs->mode = EGRS_NONE;
	else
	{
		printf( "invalid command: wrong mode in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}	
	
	if(p_egrs->mode != EGRS_NONE){
		if(gp_argv[5])
			tmp = atof(gp_argv[5]);

//			tmp = (float)strtoul(gp_argv[5], &endptr, 0);
	}
	
	if(tmp>1024)
	{
		printf( "invalid command: wrong Mbps in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	//cal = ((float)8*CLOCK_FREQ_HZ/tmp  - 1)*4;
	cal = ((float)8*clock_freq_hz/tmp)*4;
	p_egrs->scale = (unsigned long)(cal+0.5);

	//tmp = ((float)8*CLOCK_FREQ_HZ)/((float)(p_egrs->scale)/4.0+1)+0.5;
	tmp = ((float)8*clock_freq_hz)/((float)(p_egrs->scale)/4.0)+0.5;
	real_speed = (unsigned long)tmp;

	printf("egrs_info: port_no=%d, mode=%d, scale=%d(%f), actual speed=%d\n", p_egrs->port_no, p_egrs->mode, p_egrs->scale, cal, real_speed);
	if( ioctl( skfd, EGRS_RATE_CMD, &ifr ) < 0 )
	{
		printf( "egrs setting failed!\n" );
		//close( skfd );
		return -1;
	}
	printf( "egrs setting ok\n" );
}

int process_vlan_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	//unsigned char tmp;
	p_misc->misc_cmd = VLAN_OPERATION;

	if(strcmp(gp_argv[3], "enable") == 0)
		p_misc->misc_val.vlan_en = 1;
	else if(strcmp(gp_argv[3], "disable") == 0)
		p_misc->misc_val.vlan_en = 0;
	else{
		printf( "invalid command: wrong vlan operation!\n", __FUNCTION__);
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	//printf( "vlan_en=%d\n", p_misc->misc_val.vlan_en);
	
	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "vlan link display failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_link_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	//unsigned char tmp;

	p_misc->misc_cmd = SHOW_LINK_STATUS;

	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "port link display failed!\n" );
		//close( skfd );
		return -1;
	}
}

/*
*	used for TX of Port0, indicate which ports to forward.
*/
int process_forwards_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	unsigned short tmp;
	
	if (g_argc<4)
	{
		printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}

	p_misc->misc_cmd = ROUTE_SETTING;
	p_misc->misc_val.route_val.route_cmd = ROUTE_FORWARDING_TEST;

	tmp = strtoul(gp_argv[3], &endptr, 16);
	if((tmp&0x0eee)!=0){
		printf( "invalid command: wrong forwarding ports: %x, %x\n", tmp, tmp&0xeee);	
		return -1;
	}
	//printf( "%x\n", tmp&0xeee);	
	
	p_misc->misc_val.route_val.un.forward_ports = tmp;
	printf("setting forward_ports to %03x\n", p_misc->misc_val.route_val.un.forward_ports);
	
	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "forward_ports setting failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_wan_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	
	if (g_argc<4)
	{
		printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}

	p_misc->misc_cmd = ROUTE_SETTING;
	p_misc->misc_val.route_val.route_cmd = WAN_PORT_SELECTION;
	p_misc->misc_val.route_val.un.wan_port = strtoul(gp_argv[3], &endptr, 0);

	if(p_misc->misc_val.route_val.un.wan_port>3)
	{
		printf( "wan port error, should be ranged in 0~3!\n" );
		return -1;
	}
	printf("setting port[%d] to WAN port\n", p_misc->misc_val.route_val.un.wan_port);
	
	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "wan port setting failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_igrs2_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;

	if (g_argc<7)
	{
		printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	p_misc->misc_cmd = IGRS2_DBG_SETING;
	
	if(strcmp(gp_argv[3], "port1") == 0)
		p_misc->misc_val.igrs_val2.port = 1;
	else if(strcmp(gp_argv[3], "port2") == 0)
		p_misc->misc_val.igrs_val2.port = 2;
	else if(strcmp(gp_argv[3], "port3") == 0)
		p_misc->misc_val.igrs_val2.port = 3;
	else{
		printf( "invalid command: wrong port input in %s!\n", __FUNCTION__);
		//print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	p_misc->misc_val.igrs_val2.cbs = strtoul(gp_argv[4], &endptr, 0);
	p_misc->misc_val.igrs_val2.ebs = strtoul(gp_argv[5], &endptr, 0);
	p_misc->misc_val.igrs_val2.tc = strtoul(gp_argv[6], &endptr, 0);

	if(p_misc->misc_val.igrs_val2.cbs>0xffffffff)
		printf( "cbs error!\n");
	if(p_misc->misc_val.igrs_val2.ebs>0xffffffff)
		printf( "ebs error!\n");
	if(p_misc->misc_val.igrs_val2.tc>0xffff)
		printf( "tc error!\n");

	printf("cbs=%d, ebs=%d, tc=%d\n", p_misc->misc_val.igrs_val2.cbs, \
								p_misc->misc_val.igrs_val2.ebs, p_misc->misc_val.igrs_val2.tc);

	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "igrs2 setting failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_qos_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;

	if(strcmp(gp_argv[3], "acl") == 0)
		p_misc->misc_cmd = QOS_ACL_SETTING;
	else if(strcmp(gp_argv[3], "vlan") == 0)
		p_misc->misc_cmd = QOS_VLAN_SETTING;
	else if(strcmp(gp_argv[3], "dscp") == 0)
		p_misc->misc_cmd = QOS_DSCP_SETTING;
	else{
		printf( "invalid command: wrong qos operation!\n", __FUNCTION__);
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	if(strcmp(gp_argv[4], "enable") == 0)
		p_misc->misc_val.qos_en = 1;
	else if(strcmp(gp_argv[4], "disable") == 0)
		p_misc->misc_val.qos_en = 0;
	else{
		printf( "invalid command: wrong qos setting!\n", __FUNCTION__);
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
	
	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "qos setting failed!\n" );
		//close( skfd );
		return -1;
	}
	
}

int process_phy_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	//unsigned char tmp;

	p_misc->misc_cmd = READ_PHY_MDIO;

	p_misc->misc_val.mdio_val.port_no = strtoul(gp_argv[3], &endptr, 0);
	p_misc->misc_val.mdio_val.phy_reg_addr = strtoul(gp_argv[4], &endptr, 0);

	//printf("port_no=%d, phy_reg_addr=%d\n", p_misc->misc_val.mdio_val.port_no, p_misc->misc_val.mdio_val.phy_reg_addr);
	if((p_misc->misc_val.mdio_val.port_no>3||p_misc->misc_val.mdio_val.port_no==0)||(p_misc->misc_val.mdio_val.phy_reg_addr>31)){
		printf( "phy mdio param wrong!\n" );
		return -1;
	}

	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "phy mdio read failed!\n" );
		//close( skfd );
		return -1;
	}
}


int process_drop_cmd(long skfd)
{
	struct misc_param* p_misc = NULL;
	p_misc = (struct misc_param*)&ifr.ifr_data;
	char *endptr = NULL;
	//unsigned char tmp;

	p_misc->misc_cmd = SHOW_DROP_STATISTICS;

	if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
	{
		printf( "pkt drops display failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_stp_cmd(long skfd)
{
	struct stp_param* p_stp = NULL;
	p_stp = (void*)&ifr.ifr_data;
	char *endptr = NULL;
	unsigned char tmp;

	if(gp_argv[3]==NULL)	{
		printf( "invalid command in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
		
	if(strcmp(gp_argv[3], "enable") == 0)
		p_stp->stp_op = STP_OPERATION_ENABLE;
	else if(strcmp(gp_argv[3], "disable") == 0)
		p_stp->stp_op = STP_OPERATION_DISABLE;
	else if(strcmp(gp_argv[3], "info") == 0)
		p_stp->stp_op = STP_OPERATION_INFO;
	else if(strcmp(gp_argv[3], "priority") == 0){
		p_stp->stp_op = STP_SET_PRIORITY;	
		p_stp->priority_val = strtoul(gp_argv[4], &endptr, 16);
		//printf("stp_op=%d, priority_val=0x%x\n", p_stp->stp_op, p_stp->priority_val);
	}
	else	{
		printf( "invalid command: wrong mode in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}	

	//printf("stp_op=%d\n", p_stp->stp_op);
	
	if( ioctl( skfd, STP_CTL_CMD, &ifr ) < 0 )
	{
		printf( "stp setting failed!\n" );
		//close( skfd );
		return -1;
	}
}

int process_dbg_cmd(long skfd)
{
	struct dbg_param* p_dbg = NULL;
	p_dbg = (struct dbg_param*)&ifr.ifr_data;
	char *endptr = NULL;
	unsigned char tmp;
	
	tmp = strtoul(gp_argv[3], &endptr, 0);
	if(tmp>31)
	{
		printf( "invalid command: dbg no out of range in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;	
	}
	
	if((strcmp(gp_argv[4], "on") !=0)&&(strcmp(gp_argv[4], "off")!=0))
	{
		printf( "invalid command: wrong argument count in dbg of %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;	
	}

	p_dbg->dbg_no = tmp;
	p_dbg->dbg_val = strcmp(gp_argv[4], "on")? 0:1;

	tmp = 0;
	if(gp_argv[5])
		tmp = strtoul(gp_argv[5], &endptr, 0);
	p_dbg->dbg_cmd = tmp;
	

	printf("dbg_no=%d, dbg_val=%d, dbg_cmd=%d\n", p_dbg->dbg_no, p_dbg->dbg_val, p_dbg->dbg_cmd);

	if( ioctl( skfd, DBG_SETTING_CMD, &ifr ) < 0 )
	{
		printf( "dbg setting failed!\n" );
		//close( skfd );
		return -1;
	}	
}

int process_vport_cmd(long skfd)
{
	struct vport_param* p_vport = NULL;
	p_vport = (struct vport_param*)&ifr.ifr_data;
	char *endptr = NULL;

	if (g_argc<3)
	{
		printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);			
		return -1;
	}
		
	if(strcmp(gp_argv[3], "-w") == 0)
	{
		unsigned short tmp;
		
		if (g_argc<13)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		
		tmp = strtoul(gp_argv[4], &endptr, 0);
		if(tmp>SW_PORT_MAX_NUM)
		{
			printf( "invalid command: wrong port no(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vport->port_no = tmp;

		if(strcmp(gp_argv[5], "dump") == 0)
			tmp = 0;
		else if(strcmp(gp_argv[5], "access") == 0)
		 	tmp = 1;
		else if(strcmp(gp_argv[5], "trunk") == 0)
			tmp = 2;
		else if(strcmp(gp_argv[5], "hybird") == 0)
			tmp = 3;
		else{
			printf( "invalid command: wrong port type in branch of %s!\n", __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vport->port_type= tmp;

		tmp = strtoul(gp_argv[6], &endptr, 0);
		if(tmp>0x7)
		{
			printf( "invalid command: wrong port priority(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vport->ppriority= tmp;

		tmp = strtoul(gp_argv[7], &endptr, 0);
		//printf("tmp=%d\n", tmp);
		//if(tmp<1||tmp>4094)
		if(!(tmp>0&&tmp<4095))
		{
			printf( "invalid command: wrong port pvid(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vport->pvid= tmp;

		if(strcmp(gp_argv[8], "1") == 0)
			p_vport->fff_chg = 1;
		else if(strcmp(gp_argv[8], "0") == 0)
			p_vport->fff_chg = 0;
		else{
			printf( "invalid command: wrong fff_chg in branch of %s!\n", __FUNCTION__);	
			return -1;
		}
		
		if(strcmp(gp_argv[9], "1") == 0)
			p_vport->igrs_filter= 1;
		else if(strcmp(gp_argv[9], "0") == 0)
			p_vport->igrs_filter = 0;
		else{
			printf( "invalid command: wrong igrs_filter in branch of %s!\n", __FUNCTION__);	
			return -1;
		}
		
		if(strcmp(gp_argv[10], "1") == 0)
			p_vport->tag_chg= 1;
		else if(strcmp(gp_argv[10], "0") == 0)
			p_vport->tag_chg = 0;
		else{
			printf( "invalid command: wrong tag_chg in branch of %s!\n", __FUNCTION__);	
			return -1;
		}

		if(strcmp(gp_argv[11], "1") == 0)
			p_vport->tag_only= 1;
		else if(strcmp(gp_argv[11], "0") == 0)
			p_vport->tag_only = 0;
		else{
			printf( "invalid command: wrong tag_only in branch of %s!\n", __FUNCTION__);	
			return -1;
		}

		if(strcmp(gp_argv[12], "1") == 0)
			p_vport->untag_only= 1;
		else if(strcmp(gp_argv[12], "0") == 0)
			p_vport->untag_only = 0;
		else{
			printf( "invalid command: wrong untag_only in branch of %s!\n", __FUNCTION__);	
			return -1;
		}

#if 0
		printf("%d, %d, %d, %d, 0x%x\n", p_vport->port_no, p_vport->port_type, \
							p_vport->ppriority, p_vport->pvid, *p_vport);
		printf("%d, %d, %d, %d, %d\n", p_vport->fff_chg, p_vport->igrs_filter, p_vport->tag_chg, p_vport->tag_only, p_vport->untag_only);
		
#endif
				
		if( ioctl( skfd, VPORT_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "read switch vlan port failed!\n" );
			//close( skfd );
			return -1;
		}
	}
	else if(strcmp(gp_argv[3], "-r") == 0)
	{
		if (g_argc<5)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		if(strcmp(gp_argv[4], "all") != 0)
		{
			printf( "invalid command: wrong argument in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}	
		ifr.ifr_ifindex = 2;	
		if( ioctl( skfd, VPORT_READ_CMD, &ifr ) < 0 )
		{
			printf( "read switch vlan port failed!\n" );
			//close( skfd );
			return -1;
		}
	}	
	else
	{
		printf( "invalid command: unsupported argument in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);	
		return 0;
	}		
}



int process_vlut_cmd(long skfd)
{
	struct vlut_param* p_vlut = NULL;
	p_vlut = (struct vlut_param*)&ifr.ifr_data;
	char *endptr = NULL;

	//printf("%s, %d", __FUNCTION__, __LINE__);
	if(strcmp(gp_argv[3], "-w") == 0)
	{
		unsigned short tmp;
		
		if (g_argc<8)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		//printf("%s, %d", __FUNCTION__, __LINE__);
		tmp = strtoul(gp_argv[4], &endptr, 0);
		if(tmp>VLUT_TOTAL_ENTRY_NUM)
		{
			printf( "invalid command: wrong idx(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vlut->idx = tmp;

		//printf("%s, %d", __FUNCTION__, __LINE__);
		tmp= strtoul(gp_argv[5], &endptr, 0);
		//printf("tmp=%d\n", tmp);
		//if(tmp<1||tmp>4094)
		if(!(tmp>0&&tmp<4095))
		{
			printf( "invalid command: wrong vid(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}	
		p_vlut->vid = tmp;

		//printf("%s, %d", __FUNCTION__, __LINE__);
		tmp= strtoul(gp_argv[6], &endptr, 2);
		if(tmp&0x1)
			p_vlut->port0_member=1;
		else
			p_vlut->port0_member=0;
		
		if(tmp&0x2)
			p_vlut->port1_member=1;
		else
			p_vlut->port1_member=0;

		if(tmp&0x4)
			p_vlut->port2_member=1;
		else
			p_vlut->port2_member=0;

		if(tmp&0x8)
			p_vlut->port3_member=1;
		else
			p_vlut->port3_member=0;

		//printf("%s, %d", __FUNCTION__, __LINE__);
		tmp= strtoul(gp_argv[7], &endptr, 2);
		if(tmp&0x1)
			p_vlut->port0_tag=1;
		else
			p_vlut->port0_tag=0;
		
		if(tmp&0x2)
			p_vlut->port1_tag=1;
		else
			p_vlut->port1_tag=0;

		if(tmp&0x4)
			p_vlut->port2_tag=1;
		else
			p_vlut->port2_tag=0;

		if(tmp&0x8)
			p_vlut->port3_tag=1;
		else
			p_vlut->port3_tag=0;


		printf("%d, %d, %d%d%d%d, %d%d%d%d, 0x%x\n", p_vlut->idx, p_vlut->vid,\
			p_vlut->port3_member, p_vlut->port2_member, p_vlut->port1_member, p_vlut->port0_member, \
			p_vlut->port3_tag, p_vlut->port2_tag, p_vlut->port1_tag, p_vlut->port0_tag, *p_vlut);
		
		if( ioctl( skfd, VLUT_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "write switch vlut failed!\n" );
			//close( skfd );
			return -1;
		}	
		
	}
	else if(strcmp(gp_argv[3], "-d") == 0)
	{
		unsigned short tmp;
		
		if (g_argc<5)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		
		tmp = strtoul(gp_argv[4], &endptr, 0);
		if(tmp>VLUT_TOTAL_ENTRY_NUM)
		{
			printf( "invalid command: wrong idx(%d) in branch of %s!\n", tmp, __FUNCTION__);
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}
		p_vlut->idx = tmp;
		
		p_vlut->vid = 0;
		p_vlut->port0_member=0;
		p_vlut->port1_member=0;
		p_vlut->port2_member=0;
		p_vlut->port3_member=0;
		p_vlut->port0_tag=0;
		p_vlut->port1_tag=0;
		p_vlut->port2_tag=0;
		p_vlut->port3_tag=0;

		if( ioctl( skfd, VLUT_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "write switch vlut failed!\n" );
			//close( skfd );
			return -1;
		}	
	}
	
	else if(strcmp(gp_argv[3], "-r") == 0)
	{
		if (g_argc<5)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		if(strcmp(gp_argv[4], "all") != 0)
		{
			printf( "invalid command: wrong argument in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}	
		ifr.ifr_ifindex = 1; /* because no PRIVATE, just tmp use it to differ */			
		if( ioctl( skfd, VLUT_READ_CMD, &ifr ) < 0 )
		{
			printf( "read switch vlut failed!\n" );
			//close( skfd );
			return -1;
		}
	}
	else
	{
		printf( "invalid command: unsupported argument in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);	
		return 0;
	}	
}

int process_alr_cmd(long skfd)
{
	struct alr_param* p_alr = NULL;
	p_alr = (struct alr_param*)&ifr.ifr_data;
	char *endptr = NULL;
	
	//unsigned long mac_addr_temp_high32, mac_addr_temp_low16;
	
	if(strcmp(gp_argv[3], "-w") == 0)
	{
		char mac_addr_string_high32[9]={0};
		char mac_addr_string_low16[5]={0};
		unsigned char tmp;
		char forward_ports_string[5]={0};
		
		if (g_argc<6)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		//-------------------------------------------------------------------
		//printf("%d, %s\n", strlen(gp_argv[4]), gp_argv[4]);
		/*xx-xx-xx-xx-xx-xx*/
		if(strlen(gp_argv[4])!=17)
		{
			printf("mac address err: %s\n", gp_argv[4]);
			return -1;
		}
		if((gp_argv[4][2]!='-')||(gp_argv[4][5]!='-')||(gp_argv[4][8]!='-')\
			||(gp_argv[4][11]!='-')||(gp_argv[4][14]!='-'))
		{
			printf("mac address err: %s\n", gp_argv[4]);
			return -1;
		}
		
		mac_addr_string_high32[0]=gp_argv[4][0];
		mac_addr_string_high32[1]=gp_argv[4][1];
		mac_addr_string_high32[2]=gp_argv[4][3];
		mac_addr_string_high32[3]=gp_argv[4][4];
		mac_addr_string_high32[4]=gp_argv[4][6];
		mac_addr_string_high32[5]=gp_argv[4][7];
		mac_addr_string_high32[6]=gp_argv[4][9];
		mac_addr_string_high32[7]=gp_argv[4][10];
		mac_addr_string_low16[0]=gp_argv[4][12];
		mac_addr_string_low16[1]=gp_argv[4][13];
		mac_addr_string_low16[2]=gp_argv[4][15];
		mac_addr_string_low16[3]=gp_argv[4][16];

		//printf("refined mac_address: %s%s\n", mac_addr_string_high32, mac_addr_string_low16);
		p_alr->mac_addr_high32= strtoul(mac_addr_string_high32, &endptr, 16);
		p_alr->mac_addr_low16= strtoul(mac_addr_string_low16, &endptr, 16);
		printf("p_alr->mac_addr: %08x%04x\n", p_alr->mac_addr_high32, p_alr->mac_addr_low16);

		//-------------------------------------------------------------------
		if(strlen(gp_argv[5])!=4)
		{
			printf("forward ports err: %s\n", gp_argv[5]);
			return -1;
		}
		
		tmp= strtoul(gp_argv[5], &endptr, 2);
/*
		printf("tmp: %x\n", tmp);
		
		if((tmp&0x1>0x1)||(tmp&0x10>0x10)\
			||(tmp&0x100>0x100)||(tmp&0x1000>0x1000))
		{
			printf("forward ports err: %s\n", gp_argv[5]);
			return -1;
		}
*/
		if(tmp>0xf)
		{
			printf("forward ports err: %s\n", gp_argv[5]);
			return -1;
		}
		p_alr->ports = tmp;
		forward_ports_string[3] = (tmp&1)+'0';
		forward_ports_string[2] = ((tmp>>1)&1)+'0';
		forward_ports_string[1] = ((tmp>>2)&1)+'0';
		forward_ports_string[0] = ((tmp>>3)&1)+'0';
		
		printf("p_alr->ports: 0x%04x, %s\n", p_alr->ports, forward_ports_string);
		
		//-------------------------------------------------------------------
		p_alr->static_flag = 1;
		p_alr->filter_flag = 1;
		p_alr->valid= 1;
		
		if(g_argc>6)
		{
			tmp= strtoul(gp_argv[6], &endptr, 16);
			if(tmp>1)
			{
				printf("static_flag err: %s\n", gp_argv[6]);
				return -1;
			}
			p_alr->static_flag = tmp;
		}
		if(g_argc>7)
		{
			tmp= strtoul(gp_argv[7], &endptr, 16);
			if(tmp>1)
			{
				printf("static_flag err: %s\n", gp_argv[7]);
				return -1;
			}
			p_alr->filter_flag = tmp;
		}
		printf("p_alr->static_flag: %d\n", p_alr->static_flag);
		printf("p_alr->filter_flag: %d\n", p_alr->filter_flag);
		printf("p_alr->valid: %d\n", p_alr->valid);
		
		if( ioctl( skfd, ALR_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "write switch alr failed!\n" );
			//close( skfd );
			return -1;
		}		
		
	}
	else if(strcmp(gp_argv[3], "-d") == 0)
	{
		char mac_addr_string_high32[9]={0};
		char mac_addr_string_low16[5]={0};
		unsigned char tmp;
		//char forward_ports_string[5]={0};
		
		if (g_argc<5)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		//-------------------------------------------------------------------
		//printf("%d, %s\n", strlen(gp_argv[4]), gp_argv[4]);
		/*xx-xx-xx-xx-xx-xx*/
		if(strlen(gp_argv[4])!=17)
		{
			printf("mac address err: %s\n", gp_argv[4]);
			return -1;
		}
		if((gp_argv[4][2]!='-')||(gp_argv[4][5]!='-')||(gp_argv[4][8]!='-')\
			||(gp_argv[4][11]!='-')||(gp_argv[4][14]!='-'))
		{
			printf("mac address err: %s\n", gp_argv[4]);
			return -1;
		}
		
		mac_addr_string_high32[0]=gp_argv[4][0];
		mac_addr_string_high32[1]=gp_argv[4][1];
		mac_addr_string_high32[2]=gp_argv[4][3];
		mac_addr_string_high32[3]=gp_argv[4][4];
		mac_addr_string_high32[4]=gp_argv[4][6];
		mac_addr_string_high32[5]=gp_argv[4][7];
		mac_addr_string_high32[6]=gp_argv[4][9];
		mac_addr_string_high32[7]=gp_argv[4][10];
		mac_addr_string_low16[0]=gp_argv[4][12];
		mac_addr_string_low16[1]=gp_argv[4][13];
		mac_addr_string_low16[2]=gp_argv[4][15];
		mac_addr_string_low16[3]=gp_argv[4][16];

		//printf("refined mac_address: %s%s\n", mac_addr_string_high32, mac_addr_string_low16);
		p_alr->mac_addr_high32= strtoul(mac_addr_string_high32, &endptr, 16);
		p_alr->mac_addr_low16= strtoul(mac_addr_string_low16, &endptr, 16);
		printf("p_alr->mac_addr: %08x%04x\n", p_alr->mac_addr_high32, p_alr->mac_addr_low16);

		//-------------------------------------------------------------------
		p_alr->valid= 0;
		printf("p_alr->valid: %d\n", p_alr->valid);
		
		if( ioctl( skfd, ALR_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "delete switch alr failed!\n" );
			//close( skfd );
			return -1;
		}		
		
	}
	else if(strcmp(gp_argv[3], "-r") == 0)
	{
		if (g_argc<5)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);			
			return -1;
		}

		if(strcmp(gp_argv[4], "all") == 0)
		{
			p_alr->idx = ALR_SHOW_ALL_ENTRY;
			//printf("p_alr->idx: %d\n", p_alr->idx);
			if( ioctl( skfd, ALR_READ_CMD, &ifr ) < 0 )
			{
				printf( "read switch alr failed!\n" );
				//close( skfd );
				return -1;
			}
		}
		else
		{
			p_alr->idx = strtoul(gp_argv[4], &endptr, 10);
			if(p_alr->idx>ALR_TOTAL_ENTRY_NUM)
			{
				printf("invalid command: entry index out of range(%d)\n", gp_argv[4]);
				return -1;
			}
			
			printf("p_alr->idx: %d\n", p_alr->idx);
			
			if( ioctl( skfd, ALR_READ_CMD, &ifr ) < 0 )
			{
				printf( "read switch alr failed!\n" );
				//close( skfd );
				return -1;
			}
		}				
	
	}	

	else if(strcmp(gp_argv[3], "age") == 0)
	{
		struct misc_param* p_misc = NULL;
		p_misc = (struct misc_param*)&ifr.ifr_data;
		char *endptr = NULL;
		p_misc->misc_cmd = ALR_AGING_TEST;
		p_misc->misc_val.age_val.aging_type = 0;
		p_misc->misc_val.age_val.aging_time = strtoul(gp_argv[4], &endptr, 10);
		if(p_misc->misc_val.age_val.aging_time>0xfffffff){
			printf( "aging_time out of range!\n" );
			return -1;
		}
		if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
		{
			printf( "alr aging operation failed!\n" );
			return -1;
		}
	}
	else if(strcmp(gp_argv[3], "fastaging") == 0)
	{
		struct misc_param* p_misc = NULL;
		p_misc = (struct misc_param*)&ifr.ifr_data;
		char *endptr = NULL;
		p_misc->misc_cmd = ALR_AGING_TEST;
		p_misc->misc_val.age_val.aging_type = 1;

		if( ioctl( skfd, MISC_CMD, &ifr ) < 0 )
		{
			printf( "alr aging operation failed!\n" );
			return -1;
		}
	}
	else
	{
		printf( "invalid command: unsupported argument in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);	
		return 0;
	}
}

int get_clock_freq(long skfd) {
	struct reg_param* p_reg = NULL;
	int freq_hz = 0;
	struct ifreq freq_ifr;

	bzero(&freq_ifr, sizeof(freq_ifr));
	strncpy(freq_ifr.ifr_name, ifr.ifr_name, IFNAMSIZ - 1);
	freq_ifr.ifr_name[IFNAMSIZ - 1] = 0;

	p_reg = (struct reg_param*)&freq_ifr.ifr_data;
	p_reg->mux= 4;
	p_reg->addr = 0x6c;
	
	if(ioctl(skfd, REG_READ_CMD, &freq_ifr) < 0) {
		printf( "read switch register failed!\n" );
		return -1;
	}
	printf("11 p_reg->val %d\n", p_reg->val);
	freq_hz = p_reg->val & 0xff;
	return freq_hz;
}

int process_reg_cmd(long skfd)
{
	struct reg_param* p_reg = NULL;
	char *endptr = NULL;
	unsigned long mux = 0;
	unsigned long sw_reg = 0;
	unsigned long reg_val = 0;

	p_reg = (struct reg_param*)&ifr.ifr_data;
	//printf("&ifr=0x%x, &ifr.ifr_data=0x%x\n", &ifr, &ifr.ifr_data);
	
	if(strcmp(gp_argv[3], "-r32") == 0)
	{
		if (g_argc != 6)
		{
			printf( "invalid command: wrong argument count in reading branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			//close( skfd );			
			return -1;
		}

		mux = strtoul(gp_argv[4], &endptr, 0);
		sw_reg = strtoul(gp_argv[5], &endptr, 16);

		p_reg->mux= (unsigned char)mux;
		p_reg->addr = (unsigned short)sw_reg;
		
		if( ioctl( skfd, REG_READ_CMD, &ifr ) < 0 )
		{
			printf( "read switch register failed!\n" );
			//close( skfd );
			return -1;
		}

		if(p_reg->mux<4)
			printf("read switch port[%d] reg[0x%x] = 0x%08x\n", p_reg->mux, p_reg->addr, p_reg->val);//0x%08x
		else if(p_reg->mux == 4)
			printf("read switch fabric reg[0x%x] = 0x%08x\n", p_reg->addr, p_reg->val);
		else
		{
			printf( "read switch register failed!\n" );
			//close( skfd );
			return -1;
		}

	}
	else if (strcmp(gp_argv[3], "-w32") == 0)
	{
		if (g_argc != 7)
		{
			printf( "invalid command: wrong argument count in writing branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			//close( skfd );			
			return -1;
		}

		if((strcmp(gp_argv[4], "0") != 0)&&(strcmp(gp_argv[4], "1") != 0)&&(strcmp(gp_argv[4], "2") != 0)&&
			(strcmp(gp_argv[4], "3") != 0)&&(strcmp(gp_argv[4], "4") != 0))
		{
			printf( "invalid command: wrong mux selection in writing branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			//close( skfd );			
			return -1;
		}
			
		mux = strtoul(gp_argv[4], &endptr, 0);
		sw_reg = strtoul(gp_argv[5], &endptr, 16);
		reg_val = strtoul(gp_argv[6], &endptr, 16);

		p_reg->mux= (unsigned char)mux;
		p_reg->addr = (unsigned short)sw_reg;
		p_reg->val = (unsigned long)reg_val;
		
		if( ioctl( skfd, REG_WRITE_CMD, &ifr ) < 0 )
		{
			printf( "write switch register failed!\n" );
			//close( skfd );
			return -1;
		}

		if(p_reg->mux<4)
			printf("write switch port[%d] reg[0x%x] = 0x%08x\n", p_reg->mux, p_reg->addr, p_reg->val);
		else if(p_reg->mux == 4)
			printf("write switch fabric reg[0x%x] = 0x%08x\n", p_reg->addr, p_reg->val);
		else
		{
			printf( "write switch register failed!\n" );
			//close( skfd );
			return -1;
		}
		
	}
	else if(strcmp(gp_argv[3], "-t") == 0)
	{
		if (g_argc != 7)
		{
			printf( "invalid command: wrong argument count in branch of %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			//close( skfd );			
			return -1;
		}
		
		mux = strtoul(gp_argv[4], &endptr, 0);
		sw_reg = strtoul(gp_argv[5], &endptr, 16);
		reg_val = strtoul(gp_argv[6], &endptr, 16);

		p_reg->mux= (unsigned char)mux;
		p_reg->addr = (unsigned short)sw_reg;
		p_reg->val = (unsigned long)reg_val;
		
		printf("test switch port[%d] reg[0x%x] = 0x%08x\n", p_reg->mux, p_reg->addr, p_reg->val);
	}
	else
	{
		printf( "invalid command: unsupported argument in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);	
		return 0;
	}
}


/*##############################################*/
#define GIT_NAME "peter"

#define GTI_INIT_SPEED 10000 //10000000 

#ifndef GTI_SPEE
#define GTI_SPEED 1
#endif

int  term_width(void);
void init_space(void);
void move_to_top(void);
void line_at(int start_x, const char *s);
void draw_car(int x);
void clear_car(int x);
int git(int argc, char **argv);

int TERM_WIDTH;
int SLEEP_DELAY;

/* sw_tool eth0 alireg -r32 [alireg phy addr]
 * sw_tool eth0 alireg -w32 [alireg phy addr] [value]
 */
typedef struct _ali_reg_t {
	unsigned int cmd;
	unsigned int phy_addr; 
	unsigned int val;
} ali_reg_t;

void process_alireg_cmd(long skfd) {
	ali_reg_t ali_reg;
	ifr.ifr_data = (void *)&ali_reg;

	printf("%s\n", __FUNCTION__);
	if(strcmp(gp_argv[3], "-r32") == 0) {
		if (g_argc != 5) {
			printf( "%s wrong argument count %d, expect %d!\n", __FUNCTION__, g_argc, 5);
			return ;
		}
		ali_reg.cmd = 0;  /* 0 for reading */
		ali_reg.phy_addr = strtoul(gp_argv[4], NULL, 16);
		ali_reg.val = 0;
	} else if(strcmp(gp_argv[3], "-w32") == 0) {
		if (g_argc != 6) {
			printf( "%s wrong argument count %d, expect %d!\n", __FUNCTION__, g_argc, 6);
			return ;
		}
		ali_reg.cmd = 1;
		ali_reg.phy_addr = strtoul(gp_argv[4], NULL, 16);
		ali_reg.val = strtoul(gp_argv[5], NULL, 16);
	} else {
		printf("ali_reg only support -r32 or -w32\n");
		return;
	}

	if(ioctl(skfd, ALIREG_CMD, &ifr ) < 0 )
	{
		printf( "ali_reg ioctl failed!\n" );
		return;
	}
	return;
}

int main( long argc, char* argv[] )
{
	long skfd = 0;
	int ret_val;

	g_argc = argc;
	gp_argv =argv;

/*
	float a, b;
	char * str= "12345.67";
	char *endptr; 
	a = atof(str);
	printf("string = %s float = %f\n", str, a); 
	b = strtod(str,&endptr);
	printf("b=%lf\n", b); 
	printf("endptr=%s\n",endptr); 
*/	


	//printf("sizeof(ifr.ifr_ifru)=%d\n", sizeof(ifr.ifr_ifru));
	//printf("sizeof(struct igrs_param)=%d\n", sizeof(struct igrs_param));
	//printf("sizeof(struct acl_param)=%d\n", sizeof(struct acl_param));
	//ret_val=0x103a8c0;
	//printf("0x%x, 0x%x\n", ret_val, ntohl(ret_val));
/*	
	float a = 99.999;
	float b = 33.332;
	float c;	
	c = a/b;
	printf("a=%f, b=%f, c=%f\n", a, b, c);
*/	
	if ((g_argc < 3) || (strncmp(gp_argv[1], "eth", 3) != 0))
	{
		printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);
		return -1;
	}

	if( ( skfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 )
	{
		perror( "invalid socket" );
		return -1;
	}

	bzero( &ifr, sizeof( ifr ) );
	strncpy( ifr.ifr_name, gp_argv[1], IFNAMSIZ - 1 );
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	/* get mac frequence */
	clock_freq_hz = get_clock_freq(skfd);

	if(strcmp(gp_argv[2], "alireg") == 0) {
		process_alireg_cmd(skfd);	
	} else if(strcmp(gp_argv[2], "reg") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
	
		ret_val = process_reg_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}
	} else if(strcmp(gp_argv[2], "alr") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_alr_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}		
	else if(strcmp(gp_argv[2], "vlut") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}

		//printf("%s, %d", __FUNCTION__, __LINE__);
		ret_val = process_vlut_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}	
	else if(strcmp(gp_argv[2], "vport") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_vport_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}	
	else if(strcmp(gp_argv[2], "dbg") == 0)
	{
		ret_val = process_dbg_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}	
	else if(strcmp(gp_argv[2], "egrs") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_egrs_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}	
	else if(strcmp(gp_argv[2], "igrs") == 0)
	{
		ret_val = process_igrs_cmd_ext(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}

	else if(strcmp(gp_argv[2], "bandrate") == 0)
	{
		ret_val = process_bandrate_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}

	else if(strcmp(gp_argv[2], "acl") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		if(strcmp(gp_argv[3], "-w") == 0)
			ret_val =  process_acl_w_cmd(skfd);
		else if(strcmp(gp_argv[3], "-r") == 0)
			ret_val =  process_acl_r_cmd(skfd);
		else
		{
			printf( "invalid command!\n");
			print_cmd_usage(gp_argv[0]);
			close( skfd );
			return -1;
		}
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	else if(strcmp(gp_argv[2], "flowctl") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_flowctl_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	else if(strcmp(gp_argv[2], "stp") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_stp_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	else if(strcmp(gp_argv[2], "drops") == 0)
	{
		ret_val = process_drop_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	else if(strcmp(gp_argv[2], "link") == 0)
	{
		ret_val = process_link_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	else if(strcmp(gp_argv[2], "vlan") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_vlan_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	
	else if(strcmp(gp_argv[2], "phy") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_phy_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}

	else if(strcmp(gp_argv[2], "qos") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_qos_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}

	else if(strcmp(gp_argv[2], "igrs2") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_igrs2_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}
	
	else if(strcmp(gp_argv[2], "wan_port") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_wan_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}		
	}

	else if(strcmp(gp_argv[2], "forwards") == 0)
	{
		if (g_argc < 4){
			printf( "invalid command: wrong argument count in %s!\n", __FUNCTION__ );
			print_cmd_usage(gp_argv[0]);
			return -1;
		}
		
		ret_val = process_forwards_cmd(skfd);
		if(ret_val<0)
		{
			close( skfd );
			return -1;
		}
	}	
	else if(strcmp(gp_argv[2], "peter") == 0)
	{
		
		git(g_argc, NULL);
	}
	
	else
	{
		printf( "invalid command: unsupported argument in %s!\n", __FUNCTION__ );
		print_cmd_usage(gp_argv[0]);
		close( skfd );
		return -1;
	}
	close( skfd );
	return 0;
}

int git(int argc, char **argv)
{
    (void) argc;
    int i;
    TERM_WIDTH = term_width();
    SLEEP_DELAY = GTI_INIT_SPEED/ (TERM_WIDTH + GTI_SPEED);
	//printf("TERM_WIDTH=%u, SLEEP_DELAY=%u\n", TERM_WIDTH, SLEEP_DELAY);
	TERM_WIDTH = 100;
    
    init_space();
    for (i = -20; i < TERM_WIDTH; i++) {
        draw_car(i);
        usleep(SLEEP_DELAY);
        clear_car(i);
    }
    char *git_path = getenv("GIT");
    if (git_path) {
      execv(git_path, argv);
    } else {
      execvp(GIT_NAME, argv);
    }
    /* error in exec if we land here */
    //perror(GIT_NAME);
    return 1;
}

int term_width(void)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

void init_space(void)
{
    puts("\n\n\n\n\n\n"); /* 7 lines */
}

void move_to_top(void)
{
    printf("\033[7A");
}

void line_at(int start_x, const char *s)
{
    int x;
    size_t i;
    if (start_x > 1)
        printf("\033[%dC", start_x);
    for (x = start_x, i = 0; i < strlen(s); x++, i++) {
        if (x > 0 && x < TERM_WIDTH)
            putchar(s[i]);
    }
    putchar('\n');
}

void draw_car(int x)
{
    move_to_top();
    line_at(x, "   ,---------------.");
    line_at(x, "  /  /``````|``````\\\\");
    line_at(x, " /  /_______|_______\\\\________");
    line_at(x, "|]    Peter |'       |        |]");
    if (x % 2) {
    line_at(x, "=  .-:-.    |________|  .-:-.  =");
    line_at(x, " `  -+-  --------------  -+-  '");
    line_at(x, "   '-:-'                '-:-'  ");
    } else {
    line_at(x, "=  .:-:.    |________|  .:-:.  =");
    line_at(x, " `   X   --------------   X   '");
    line_at(x, "   ':-:'                ':-:'  ");
    }
}

void clear_car(int x)
{
    move_to_top();
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
    line_at(x, "  ");
}

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/dvb/frontend.h>


struct mac_data
{
    unsigned short  reg;
    unsigned long  value;
};

#define MAX_TX_DESC_CNT 62

struct ali_mac_xmit_io
{
	unsigned char  tx_rx;  //0: tx, 1: rx 保留用
	unsigned char  type;   //0: tcpv4, 1: udpv4, 2: tcpv6, 3: udpv6
	unsigned char  toe_tx; //0: disable, 1: enable
	unsigned char  tso_ufo;//是否使用tso、ufo。0: 不使用, 1: 使用

	unsigned long  len;    //数据包长度，包括所有层的包头

	unsigned long max_len;

	unsigned short  mtu;    //单个数据包的最大长度
	unsigned short  max_mtu;
	
	unsigned short  desc_min; //descriptor最小个数
	unsigned short  desc_max; // descriptor最大个数	
	
	unsigned short vlan;   // vlan的值，0表示不插入vlan tag	
	unsigned short  reserve_1;
	
	unsigned long  repeat;   //循环测试次数

	unsigned char  dest_type; //目的地址类型, 0：单播，1：多播，2：广播
	unsigned char  hop_hd_len;   //逐跳选项头的长度
	unsigned char  dst_hd_len;   //目的选项头的长度
	unsigned char  rt_hd_len;    //路由选项头的长度

	unsigned char  dst_2_hd_len;
	unsigned char  reserve_2;
	unsigned short reserve_3;
};


void show_help_msg( long argc, char* argv[] )
{	
	//printf( "Useage read register: %s eth0 -r [Register].\n", argv[0]);
	//printf( "Useage write register: %s eth0 -w32 [Register] [Value].\n", argv[0]);
			
	printf("Option: -tx usage:\n");
    printf( "%s eth0 -tx -type [tcpv4/udpv4/tcpv6/udpv6] [tso/ufo/toe] -dest [uni/mul/bro] -len [Min value] [Max value] -mtu [Min value] [Max value] -desc [start no.] [total number] -repeat [value] -vlan [tag] -extv6hoplen [value] -extv6destlen [value] -extv6rtlen [value] -extv6destlen2 [value] \n", argv[0]);

	printf("\nOption: -phy usage:\n");
	printf( "read register: %s eth0 -phy [phyaddr] -r [Register].\n", argv[0]);
	printf( "write register: %s eth0 -phy [phyaddr] -w [Register] [Value].\n", argv[0]);
	printf( "clean device counting: %s eth0 -phy [phyaddr] -clean.\n", argv[0]);
	printf( "nibble error monitor: %s eth0 -phy [phyaddr] -nm [time].\n", argv[0]);

	printf("\nOption: -pktgen usage:\n");
    printf("%s eth0 -pktgen [payload_1|payload_ff|payload_rand|payload_rand2][-t times][-i interval][-size pkt_size]\n", argv[0]);
    printf("       times: default 1000000, interval default 10 {10, }, pkt_size default 1500, {50, 1500}\n");
    printf("       payload_1 : byte is 0x01, payload_ff: byte is 0xff, payload_rand: byte is rand, all pkts are same\n");
    printf("       payload_rand2 : byte is rand, last byte of every pkt is different\n");
	printf("\nOption: sys regs R/W usage:\n");
	printf( "* read sys register:  %s sys -r [Register].\n", argv[0]);
    printf( "* write sys register: %s sys -w [Register] [Value].\n", argv[0]);
}

#define ALI_SYS_REG_RW  _IOWR('o', 104, __u32)
struct reg_rw_cmd_t {
	unsigned int offset_addr;
	unsigned char reg_rw_cmd[16];
};

static int nim_reg_read(int fd, unsigned int offsetAddr, unsigned char *pData)
{
	struct reg_rw_cmd_t reg_st;
	int ret;
	reg_st.offset_addr = offsetAddr;
	reg_st.reg_rw_cmd[0] = 1;//READ
	reg_st.reg_rw_cmd[1] = 4;

	ret = ioctl(fd, ALI_SYS_REG_RW, &reg_st);

	if (0 == ret)
	{
		memcpy(pData, &(reg_st.reg_rw_cmd[2]), 4);
	}
	return ret;
}

static int nim_reg_write(int fd, unsigned int offsetAddr, unsigned long pData)
{
	struct reg_rw_cmd_t reg_st;
	reg_st.offset_addr = offsetAddr;
	reg_st.reg_rw_cmd[0] = 2;//WRITE
	reg_st.reg_rw_cmd[1] = 4;
	//memcpy(&(reg_st.reg_rw_cmd[2]), pData, 4);
	reg_st.reg_rw_cmd[2] = (unsigned char)(pData&0xFF);
	reg_st.reg_rw_cmd[3] = (unsigned char)(pData>>8);
	reg_st.reg_rw_cmd[4] = (unsigned char)(pData>>16);
	reg_st.reg_rw_cmd[5] = (unsigned char)(pData>>24);

	printf("write value = 0x%08x\n", pData);
	return ioctl(fd, ALI_SYS_REG_RW, &reg_st);
}

int main( long argc, char* argv[] )
{
	long skfd = 0;
	struct ifreq ifr;
	struct ali_mac_xmit_io* cmd = NULL;
	long valid_cmd_line = 1, i=0;
	struct ali_mac_xmit_io cmd_obj;

	struct mac_data* mac = NULL;
	char *endptr = NULL;
	unsigned long mac_register = 0;
	unsigned long value = 0;

	long nimfd = 0;
	unsigned char regVal[16];
	unsigned int ret, reg_num = 0;

	if (strcmp(argv[1], "sys") == 0)
	{
		nimfd = open("/dev/ali_m3200_nim0", O_RDWR);
		if (nimfd == 0)
		{
			perror("[WARN] open NIM handle failed\n");
			return -1;
		}
		if (strcmp(argv[2], "-r") == 0)
		{
			reg_num = strtoul(argv[3], &endptr, 16);
			ret = nim_reg_read(nimfd, reg_num, regVal);
			if (ret < 0)
			{
				printf("Read SYS register[0x%08x] FAILED!\n", reg_num);
				close( nimfd );
				return -1;
			}
			printf("Read sysreg[0x%08x]: 0x[%02x %02x %02x %02x]\n", reg_num, regVal[0], regVal[1], regVal[2], regVal[3]);
		}
		else if (strcmp(argv[2], "-w") == 0)
		{
			if (argc != 5)
			{
				perror( "Invalid command\n" );
				close( nimfd );
				return -1;
			}
			reg_num = strtoul(argv[3], &endptr, 16);
			value = strtoul(argv[4], &endptr, 16);
			//printf("write value = 0x%08x\n", value);

			//regVal[3] = (unsigned char)(value>>24);
			//regVal[2] = (unsigned char)(value>>16);
			//regVal[1] = (unsigned char)(value>>8);
			//regVal[0] = (unsigned char)(value&0xFF);
			ret = nim_reg_write(nimfd, reg_num, value);
			if (ret < 0)
			{
				printf("Write SYS register[0x%08x] FAILED!\n", reg_num);
				close( nimfd );
				return -1;
			}
			//printf("Write sysreg[0x%08x], 0x[%02x,%02x,%02x,%02x]\n", reg_num, regVal[0], regVal[1], regVal[2], regVal[3]);
		}
		close( nimfd );
		return 0;
	}

	if ((argc < 4) || (strncmp(argv[1], "eth", 3) != 0))
	{
		show_help_msg(argc, argv);
		return 0;
	}

	if (strcmp(argv[2], "-phy") == 0)
	{
		return phyrw_entry(argc, argv);
	}
	if (strcmp(argv[2], "-pktgen") == 0)
	{
		return ethpktgen_entry(argc, argv);
	}

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror( "Invalid socket" );
		return -1;
	}

	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	printf("%s %d: sizeof(struct ifreq)=%d, sizeof(struct ali_mac_xmit_io)=%d\n", __FUNCTION__, __LINE__, sizeof(struct ifreq),sizeof(struct ali_mac_xmit_io));
	if(strcmp(argv[2], "-tx") == 0)
	{
		//cmd = (struct ali_mac_xmit_io*)&ifr.ifr_data;
		//cmd = (struct ali_mac_xmit_io*)(&(ifr.ifr_name[8]));
		cmd = &cmd_obj;
		ifr.ifr_data = (void*)cmd;
		memset(cmd, 0, sizeof(struct ali_mac_xmit_io));
		cmd->len=1500;
		cmd->max_len=1500;
		cmd->mtu=1510;
		cmd->max_mtu=1510;
		cmd->desc_min=1;
		cmd->desc_max=1;
		cmd->repeat=1;
		
		i=3;
		while(i<argc)
		{
			if(strcmp(argv[i], "-type") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					if(strcmp(argv[i], "tcpv4") == 0)
						cmd->type=0;
					else if(strcmp(argv[i], "udpv4") == 0)
						cmd->type=1;
					else if(strcmp(argv[i], "tcpv6") == 0)
						cmd->type=2;
					else if(strcmp(argv[i], "udpv6") == 0)
						cmd->type=3;
					else
					{
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
					if(i<argc)
					{
						printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
						if(strcmp(argv[i], "tso") == 0 || strcmp(argv[i], "ufo") == 0)
						{
							cmd->toe_tx=1;
							cmd->tso_ufo=1;
							printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
							i++;							
						}
						else if(strcmp(argv[i], "toe") == 0)
						{
							cmd->toe_tx=1;
							printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
							i++;							
						}
					}
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-dest") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					if(strcmp(argv[i], "uni") == 0)
						cmd->dest_type=0;
					else if(strcmp(argv[i], "mul") == 0)
						cmd->dest_type=1;
					else if(strcmp(argv[i], "bro") == 0)
						cmd->dest_type=2;
					else
					{
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-len") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->len = strtoul(argv[i], &endptr, 10);
					if(cmd->len==0 || cmd->len>(64*1024))
					{
						printf("Invalid packet len value!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					cmd->max_len = cmd->len;
					i++;
					if(i<argc && argv[i][0]!='-')
					{
						printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
						cmd->max_len= strtoul(argv[i], &endptr, 10);
						if(cmd->max_len<cmd->len || cmd->max_len>(64*1024))
						{
							printf("Invalid desc [Max Len value]!\n");
							valid_cmd_line = 0;
							printf("Invalid Command line: %d.\n", __LINE__);
							break;
						}
						i++;
					}
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-mtu") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->mtu = strtoul(argv[i], &endptr, 10);
					if(cmd->mtu==0 || cmd->mtu>1510)
					{
						printf("Invalid mtu value!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					cmd->max_mtu = cmd->mtu;
					i++;
					if(i<argc && argv[i][0]!='-')
					{
						printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
						cmd->max_mtu= strtoul(argv[i], &endptr, 10);
						if(cmd->max_mtu<cmd->mtu || cmd->max_mtu>1510)
						{
							printf("Invalid desc [Max Len value]!\n");
							valid_cmd_line = 0;
							printf("Invalid Command line: %d.\n", __LINE__);
							break;
						}
						i++;
					}
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-desc") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->desc_min = strtoul(argv[i], &endptr, 10);
					if(cmd->desc_min==0 || cmd->desc_min>MAX_TX_DESC_CNT)
					{
						printf("Invalid desc [start no.]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					cmd->desc_max = cmd->desc_min;
					i++;
					if(i<argc && argv[i][0]!='-')
					{
						printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
						cmd->desc_max = strtoul(argv[i], &endptr, 10);
						if(cmd->desc_max<cmd->desc_min || cmd->desc_max>MAX_TX_DESC_CNT)
						{
							printf("Invalid desc [total number]!\n");
							valid_cmd_line = 0;
							printf("Invalid Command line: %d.\n", __LINE__);
							break;
						}
						i++;
					}
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-repeat") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->repeat = strtoul(argv[i], &endptr, 10);
					if(cmd->repeat==0)
					{
						printf("Invalid repeat [value]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-vlan") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->vlan = strtoul(argv[i], &endptr, 16);
					if(cmd->vlan==0)
					{
						printf("Invalid vlan [tag]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-extv6hoplen") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->hop_hd_len = strtoul(argv[i], &endptr, 10);
					if(cmd->hop_hd_len==0)
					{
						printf("Invalid extv6hoplen [value]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-extv6destlen") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->dst_hd_len = strtoul(argv[i], &endptr, 10);
					if(cmd->dst_hd_len==0)
					{
						printf("Invalid extv6destlen [value]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-extv6rtlen") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->rt_hd_len = strtoul(argv[i], &endptr, 10);
					if(cmd->rt_hd_len==0)
					{
						printf("Invalid extv6rtlen [value]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else if(strcmp(argv[i], "-extv6destlen2") == 0)
			{
				printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
				if((++i)<argc)
				{
					printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
					cmd->dst_2_hd_len = strtoul(argv[i], &endptr, 10);
					if(cmd->dst_2_hd_len==0)
					{
						printf("Invalid extv6destlen2 [value]!\n");
						valid_cmd_line = 0;
						printf("Invalid Command line: %d.\n", __LINE__);
						break;
					}
					i++;
				}
				else
				{
					valid_cmd_line = 0;
					printf("Invalid Command line: %d.\n", __LINE__);
					break;
				}
			}
			else
			{
				valid_cmd_line = 0;
				printf("Invalid Command line: i=%d, %d.\n", i,__LINE__);
				break;
			}
		}
		if(!valid_cmd_line)
		{
			printf("Invalid Command line: %d.\n", __LINE__);
			show_help_msg(argc, argv);	
			return -1;
		}
		if(!(cmd->tso_ufo) && cmd->len>cmd->mtu)
		{
			printf("len param invalid: %d.\n", __LINE__);
			return -1;
		}
		if(cmd->len<cmd->desc_max)
		{
			printf("len & desc_max param invalid: %d.\n", __LINE__);
			return -1;
		}
		printf("%s %d: tx_rx = 0x%08X\n",__FUNCTION__,__LINE__, cmd->tx_rx);
		printf("%s %d: type = 0x%08X\n",__FUNCTION__,__LINE__, cmd->type);
		printf("%s %d: toe_tx = 0x%08X\n",__FUNCTION__,__LINE__, cmd->toe_tx);
		printf("%s %d: tso_ufo = 0x%08X\n",__FUNCTION__,__LINE__, cmd->tso_ufo);
		printf("%s %d: len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->len);
		printf("%s %d: max_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->max_len);
		printf("%s %d: mtu = 0x%08X\n",__FUNCTION__,__LINE__, cmd->mtu);
		printf("%s %d: max_mtu = 0x%08X\n",__FUNCTION__,__LINE__, cmd->max_mtu);
		printf("%s %d: desc_min = 0x%08X\n",__FUNCTION__,__LINE__, cmd->desc_min);
		printf("%s %d: desc_max = 0x%08X\n",__FUNCTION__,__LINE__, cmd->desc_max);
		printf("%s %d: vlan = 0x%08X\n",__FUNCTION__,__LINE__, cmd->vlan);
		printf("%s %d: repeat = 0x%08X\n",__FUNCTION__,__LINE__, cmd->repeat);
		printf("%s %d: dest_type = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dest_type);
		printf("%s %d: hop_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->hop_hd_len);
		printf("%s %d: dst_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dst_hd_len);
		printf("%s %d: rt_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->rt_hd_len);
		printf("%s %d: dst_2_hd_len = 0x%08X\n",__FUNCTION__,__LINE__, cmd->dst_2_hd_len);
		if( ioctl( skfd, SIOCDEVPRIVATE+3, &ifr ) < 0 )
		{
			perror( "tx start cmd failed!\n" );
			close( skfd );
			return -1;
		}
		else
			printf("%s %d: DONE!\n",__FUNCTION__,__LINE__);
	}
	else if  (strcmp(argv[2], "-r") == 0)
	{
		mac = (struct mac_data*)&ifr.ifr_data;
		if (argc != 4)
		{
			perror( "Invalid command\n" );
			close( skfd );			
			return -1;
		}
	
		mac_register = strtoul(argv[3], &endptr, 16);
		if (mac_register > 0x98)
		{
			perror( "Invalid register address" );
			close( skfd );			
			return -1;
		}

		mac->reg = (unsigned long)mac_register;
		
		if( ioctl( skfd, SIOCDEVPRIVATE+1, &ifr ) < 0 )
		{
			perror( "Read Mac register failed!\n" );
			close( skfd );
			return -1;
		}

		printf("Read Mac register 0x%x, value = 0x%x\n", mac_register, mac->value);

	}
	else if (strcmp(argv[2], "-w32") == 0)
	{
		mac = (struct mac_data*)&ifr.ifr_data;
		if (argc != 5)
		{
			perror( "Invalid command\n" );
			close( skfd );			
			return -1;
		}
	
		mac_register = strtoul(argv[3], &endptr, 16);
		value = strtoul(argv[4], &endptr, 16);
		if (mac_register > 0x98)
		{
			perror( "Invalid register address" );
			close( skfd );			
			return -1;
		}

/*
		if (value > 0xffff)
		{
			perror( "Invalid register value" );
			close( skfd );			
			return -1;
		}
*/
		mac->reg = (unsigned long)mac_register;
		mac->value = (unsigned long)value;
		
		if( ioctl( skfd, SIOCDEVPRIVATE+2, &ifr ) < 0 )
		{
			perror( "Write Mac register failed!\n" );
			close( skfd );
			return -1;
		}

		printf("Write Mac register 0x%x, value = 0x%x\n", mac_register, value);

	}
	else
	{
		printf("Unknown Command!!\n");
		show_help_msg(argc, argv);	
		return 0;
	}

	
	close( skfd );
	return 0;
}



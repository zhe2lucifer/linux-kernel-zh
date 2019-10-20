#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>

struct mii_data
{
    unsigned short phy_id;
    unsigned short  reg_num;
    unsigned short  val_in;
    unsigned short  val_out;
};

char *tool_name;
const char* tool_version = "0.0.0.4 --maintained by peter";

void show_usage()
{
	printf( "Version: %s\n", tool_version);
	printf( "read register: %s eth0 -phy [phyaddr] -r [Register].\n", tool_name);
    printf( "write register: %s eth0 -phy [phyaddr] -w [Register] [Value].\n", tool_name);
    printf( "clean device counting: %s eth0 -phy [phyaddr] -clean.\n", tool_name);
    printf( "nibble error monitor: %s eth0 -phy [phyaddr] -nm [time].\n", tool_name);
}

long clean_device_counting(long skfd, char *eth_if)
{
	struct ifreq ifr;
	if ((skfd<0) || (!eth_if))
	{
		printf("Invlid parameter in function %s \n", __FUNCTION__);
		return -1;
	}
	bzero( &ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, eth_if, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	
	if( ioctl( skfd, SIOCDEVPRIVATE+3, &ifr ) < 0 )
	{
		printf("%s ():   ioctl error \n", __FUNCTION__);
		return -1;
	}
	printf("device counting cleaned for %s.\n", eth_if);

	return 1;
}

long do_get_phy_addr(long skfd, char *eth_if, unsigned short *phy_id)
{
	struct ifreq ifr;
	struct mii_data* mii;

	if ((skfd<0) || (!eth_if) || (!phy_id))
	{
		printf("Invlid parameter in function %s \n", __FUNCTION__);
		return -1;
	}
	
	bzero( &ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, eth_if, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;
	if( ioctl( skfd, SIOCGMIIPHY, &ifr ) < 0 )
	{
		printf("%s ():   ioctl error \n", __FUNCTION__);
		return -1;
	}

	mii = (struct mii_data*)&ifr.ifr_data;
	printf("Phy address for interface:%s is %d \n", eth_if, mii->phy_id);
	
	*phy_id = mii->phy_id;

	return 1;
}

long do_get_phy_reg(long skfd, char *eth_if, struct mii_data* mii)
{
	struct ifreq ifr;
	struct mii_data* temp_mii;

	if ((skfd<0) || (!eth_if) || (!mii))
	{
		printf("Invlid parameter in function %s \n", __FUNCTION__);
		return -1;
	}
	
	bzero( &ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, eth_if, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	temp_mii = (struct mii_data*)&ifr.ifr_data;
	*temp_mii = *mii;
	
	if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0)
	{
		printf( "Read Phy register failed!\n" );
		return -1;
	}

	*mii = *temp_mii;

	return 1;
}

long do_set_phy_reg(long skfd, char *eth_if, struct mii_data* mii)
{
	struct ifreq ifr;
	struct mii_data* temp_mii;

	if ((skfd<0) || (!eth_if) || (!mii))
	{
		printf("Invlid parameter in function %s \n", __FUNCTION__);
		return -1;
	}
	
	bzero( &ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, eth_if, IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

	temp_mii = (struct mii_data*)&ifr.ifr_data;
	*temp_mii = *mii;
	
	if (ioctl(skfd, SIOCSMIIREG, &ifr) < 0)
	{
		printf( "Write Phy register failed!\n" );
		return -1;
	}

	*mii = *temp_mii;	

	return 1;
}

/*
1.1	Initial 
1. write 0Xffff to PHY_REG_0X14 (Enable function & set calculate period.(around 85s in 100bt mode))
2. write 0x0000 to PHY_REG_0X15 (clear report register & restart function)

1.2	Loop 
A.	Read PHY_REG_0X15
B.	If (PHY_REG_0X15_BIT15 == 0) goto A.   If(PHY_REG_0X15_BIT15==1) goto C
C.	Read PHY_REG_0X16. Print ERROR_CNT[30:0] by UART.
D.	Write 0x0000 to PHY_REG_0X15 (clear report register & restart function)
E.	Wait sometime(60s ~ 120s is preferred) goto A
*/
long do_nibble_monitor(long skfd, char *eth_if, struct mii_data* mii, unsigned long time_to_sleep)
{
	long ret;
	unsigned long temp;
	
	if ((skfd<0) || (!eth_if) || (!mii))
	{
		printf("Invlid parameter in function %s \n", __FUNCTION__);
		return -1;
	}

	mii->reg_num = 0x14;
	mii->val_in = 0xffff;
	ret = do_set_phy_reg(skfd, eth_if, mii);
	if (ret < 0)
	{
		return -1;
	}

	mii->reg_num = 0x15;
	mii->val_in = 0;
	ret = do_set_phy_reg(skfd, eth_if, mii);
	if (ret < 0)
	{
		return -1;
	}


	while (1)
	{
		mii->reg_num = 0x15;
		mii->val_out = 0;
		ret = do_get_phy_reg(skfd, eth_if, mii);
		if (ret < 0)
		{
			return -1;
		}

		
		if ((mii->val_out && 0x8000) == 1)
		{
			temp = (mii->val_out & 0x7fff) << 16;
		
			mii->reg_num = 0x16;
			mii->val_out = 0;
			ret = do_get_phy_reg(skfd, eth_if, mii);
			if (ret < 0)
			{
				return -1;
			}

			temp |= mii->val_out;

			printf("Nibble error : %d\n", temp);

			mii->reg_num = 0x15;
			mii->val_in = 0;
			ret = do_set_phy_reg(skfd, eth_if, mii);
			if (ret < 0)
			{
				return -1;
			}
		}
		sleep(time_to_sleep);
	}
}

int phyrw_entry(long argc, char* argv[])
{
	long skfd = 0;
	long ret;
	struct mii_data mii;
	char *endptr = NULL;
	unsigned long phy_register = 0;
	unsigned long value = 0;
	
	tool_name = argv[0];

	/*if ((argc < 3)|| (strncmp(argv[1], "eth", 3) != 0))
	{
		show_usage();
		return 0;
	}*/

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror( "Invalid socket" );
		return -1;
	}

	//if (argc != 5)
	{
		//ret = do_get_phy_addr(skfd, argv[1], &mii.phy_id);
		/*if (ret < 0)
		{
			close( skfd );
			return -1;
		}*/
		mii.phy_id = strtoul(argv[3], &endptr, 16);
		printf("U set Phy address for interface:%s is %d \n", argv[1], mii.phy_id);
	}

	if (strcmp(argv[4], "-r") == 0)
	{
		if (argc != 6)
		{
			perror( "Invalid command\n" );
			close( skfd );			
			return -1;
		}

		if (strcmp(argv[5], "all") == 0)
		{
			printf("Read Phy register 0~31:\n");				
			for (mii.reg_num = 0; mii.reg_num < 32; ++(mii.reg_num))
			{
				ret = do_get_phy_reg(skfd, argv[1], &mii);
				if (ret < 0)
				{
					close( skfd );
					return -1;
				}
				printf("\t0x%02x: 0x%04x\n", mii.reg_num, mii.val_out);				
			}
		}
		else
		{
			phy_register = strtoul(argv[5], &endptr, 16);
			if (phy_register > 32)
			{
				perror( "Invalid register address" );
				close( skfd );	
				return -1;
			}

			mii.reg_num = (unsigned short)phy_register;

			ret = do_get_phy_reg(skfd, argv[1], &mii);
			if (ret < 0)
			{
				close( skfd );
				return -1;
			}

			printf("Read Phy register 0x%02x, value = 0x%04x\n", mii.reg_num, mii.val_out);			
		}
	}
	else if (strcmp(argv[4], "-w") == 0)
	{
		if (argc != 7)
		{
			perror( "Invalid command\n" );
			close( skfd );			
			return -1;
		}
	
		phy_register = strtoul(argv[5], &endptr, 16);
		value = strtoul(argv[6], &endptr, 16);
		if (phy_register > 32)
		{
			perror( "Invalid register address" );
			close( skfd );			
			return -1;
		}

		if (value > 0xffff)
		{
			perror( "Invalid register value" );
			close( skfd );			
			return -1;
		}

		mii.reg_num = (unsigned short)phy_register;
		mii.val_in = (unsigned short)value;

		ret = do_set_phy_reg(skfd, argv[1], &mii);
		if (ret<0)
		{
			close( skfd );
			return -1;
		}
		
		printf("Write Phy register 0x%x, value = 0x%x\n", mii.reg_num, mii.val_in);
	}
	else if (strcmp(argv[4], "-nm") == 0)
	{
		if (argc != 6)
		{
			perror( "Invalid command, please enter polling time only!\n" );
			close( skfd );			
			return -1;
		}

		value = strtoul(argv[5], &endptr, 10);
		printf("Output nibble error every %d seconds! \n", value);
		
		ret = do_nibble_monitor(skfd, argv[1], &mii, value);
		if (ret<0)
		{
			close( skfd );
			return -1;
		}
	}
	else if (strcmp(argv[4], "-clean") == 0)
	{
		if (argc != 5)
		{
			perror( "Invalid command, please enter polling time only!\n" );
			close( skfd );			
			return -1;
		}
		ret = clean_device_counting(skfd, argv[1]);
		if (ret<0)
		{
			close( skfd );
			return -1;
		}
	}
	else
	{
		printf("Unknown Command!!\n");
		show_usage();
	}
	
	close( skfd );
	return 0;
}


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include <sys/wait.h>
#include "mac_tool.h"

const u32 major_ver = 0;
const u32 minor_ver = 2;

void show_help(int argc, char *argv[]) {
    printf("Version: %d.%d\n", major_ver, minor_ver);
    printf("%s eth0 -r32 [Register]             read mac register.\n", argv[0]);
    printf("%s eth0 -w32 [Register] [Value]   write mac register.\n", argv[0]);	
    printf("%s soc -r32 [Register]              read soc register.\n", argv[0]);
    printf("%s soc -w32 [Register] [Value]    write soc register.\n", argv[0]);	
    printf("%s biu -r32 [Register]              read biu register 4bytes .\n", argv[0]);
    printf("%s biu -w32 [Register] [Value]    write biu register 4bytes.\n", argv[0]);	
    printf("%s biu -r16 [Register]              read biu register 2bytes.\n", argv[0]);
    printf("%s biu -w16 [Register] [Value]    write biu register 2bytes\n", argv[0]);	
    printf("%s biu -r8 [Register]              read biu register 1byte.\n", argv[0]);
    printf("%s biu -w8 [Register] [Value]    write biu register 1byte.\n", argv[0]);	
    printf("%s dbg [bit_no] [on|off].         enable/disable dbg\n", argv[0]);	
    printf("--------------------------------------------------------\n");
    printf("%s eth0 -tx -type [tcpv4/udpv4/tcpv6/udpv6] [tso/ufo/toe] \n\
                    -dest [uni/mul/bro] -len [Min] [Max] -mtu [Min] [Max] \n\
                    -desc [start no] [total number] -repeat [value] \n\
                    -vlan [tag] -extv6hoplen [value] -extv6destlen [value]\n\
                    -extv6rtlen [value] -extv6destlen2 [value] \n", argv[0]);
    printf("--------------------------------------------------------\n");
    return;
}

void failed_show(int argc, char *argv[]) {
    int i;
    printf("Cmd Failed!!!\n");
    printf("    ");
    for(i=0; i<argc; i++) {
       printf("%s ", argv[i]);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
	int skfd = 0;
	struct ifreq ifr;
	cmd_param_t * cmd_param = NULL;
	char *endptr = NULL;
	u32 reg = 0;
	u32 value = 0;
    int ret = -1;
	struct ali_mac_xmit_io tx_cmd;

	if ((argc < 3) ||((strncmp(argv[1], "eth", 3) != 0) && \
                      (strncmp(argv[1], "soc", 3) != 0) && \
                      (strncmp(argv[1], "dbg", 3) != 0))) {
        show_help(argc, argv);
		return ret;
	}

	if((skfd = socket(AF_INET, SOCK_DGRAM,0)) < 0) {
		perror("Invalid socket");
		return ret;
	}

	bzero(&ifr, sizeof(ifr));
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);
	ifr.ifr_name[IFNAMSIZ - 1] = 0;

    if(sizeof(ifr) < sizeof(cmd_param_t)) {
        printf("Warn!!! ifr size %d < cmd_param_t size %d\n", sizeof(ifr), sizeof(cmd_param_t));
        return ret;
    }
	cmd_param = (cmd_param_t *)&ifr.ifr_data;
	
	if(!strncmp(argv[1], "eth", 3)) {
        if (!strcmp(argv[2], "-r32")) {
            if (argc != 4) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
            cmd_param->cmd = READ_MAC_REG;
        } else if (!strcmp(argv[2], "-w32")) {
            if (argc != 5) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
			cmd_param->value = strtoul(argv[4], &endptr, 16);
            cmd_param->cmd = WRITE_MAC_REG;
        } else if (!strcmp(argv[2], "-tx")) {
            if (process_tx_cmd(argc, argv, &tx_cmd) < 0) {
                perror("process_tx_cmd failed\n");
                goto failed;
            }
            cmd_param->cmd = TX_PKTS;
            cmd_param->data = &tx_cmd;
        } else {
            printf("eth0 doesn't support %s\n", argv[2]);
            goto failed;
        }
	} else if (!strncmp(argv[1], "soc", 3)) {
		if (!strcmp(argv[2], "-r32")) {
            if (argc != 4) {
                perror("Invalid command\n");
                goto failed;
            }
		
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
            cmd_param->cmd = READ_SOC_REG;
		} else if (!strcmp(argv[2], "-w32")) {
            if (argc != 5) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
			cmd_param->value = strtoul(argv[4], &endptr, 16);
            cmd_param->cmd = WRITE_SOC_REG;
        } else {
            printf("soc doesn't support %s\n", argv[2]);
            goto failed;
		}
	} else if (!strncmp(argv[1], "biu", 3)) {
		if (!strcmp(argv[2], "-r32")) {
            if (argc != 4) {
                perror("Invalid command\n");
                goto failed;
            }
		
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
            cmd_param->cmd = READ_BIU_4BYTE;
		} else if (!strcmp(argv[2], "-r16")) {
            if (argc != 4) {
                perror("Invalid command\n");
                goto failed;
            }
		
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
            cmd_param->cmd = READ_BIU_2BYTE;
		} else if (!strcmp(argv[2], "-r8")) {
            if (argc != 4) {
                perror("Invalid command\n");
                goto failed;
            }
		
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
            cmd_param->cmd = READ_BIU_1BYTE;
		} else if (!strcmp(argv[2], "-w32")) {
            if (argc != 5) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
			cmd_param->value = strtoul(argv[4], &endptr, 16);
            cmd_param->cmd = WRITE_BIU_4BYTE;
		} else if (!strcmp(argv[2], "-w16")) {
            if (argc != 5) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
			cmd_param->value = strtoul(argv[4], &endptr, 16);
            cmd_param->cmd = WRITE_BIU_2BYTE;
		} else if (!strcmp(argv[2], "-w8")) {
            if (argc != 5) {
                perror("Invalid command\n");
                goto failed;
            }
            cmd_param->reg = strtoul(argv[3], &endptr, 16);
            if (reg > REG_MAX_VALUE) {
                perror("Invalid register address\n");
                goto failed;
            }
			cmd_param->value = strtoul(argv[4], &endptr, 16);
            cmd_param->cmd = WRITE_BIU_1BYTE;
        } else {
            printf("soc doesn't support %s\n", argv[2]);
            goto failed;
		}
	} else if (!strncmp(argv[1], "dbg", 3)) {
        cmd_param->reg = strtoul(argv[2], &endptr, 10);
        printf("reg is %d\n", cmd_param->reg);
        if (cmd_param->reg > 31) {
            printf("Invalid dbg level %d\n",cmd_param->reg);
            goto failed;
        }
        cmd_param->value = strcmp(argv[3], "on")? 0:1;
        cmd_param->cmd = DEBUG_DRIVER_LEVEL;
	} else {
        printf("doesn't support %s\n", argv[1]);
        goto failed;
    }
    if (ioctl(skfd, MAC_TOOL_IOCTL_CMD, &ifr) < 0) {
        printf("MAC_TOOL_IOCTL_CMD failed!!!\n");
        goto failed;
    } 
    close(skfd);
    return 0;	
failed:
    failed_show(argc, argv);
	close(skfd);
	return ret;
}

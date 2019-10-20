#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include "mac_tool.h"

#define MAX_TX_DESC_CNT 62

int process_tx_cmd(long argc, char* argv[], struct ali_mac_xmit_io *cmd) {
	long valid_cmd_line = 1, i=0;
	char *endptr = NULL;

    memset(cmd, 0, sizeof(struct ali_mac_xmit_io));
    cmd->len=1500;
    cmd->max_len=1500;
    cmd->mtu=1510;
    cmd->max_mtu=1510;
    cmd->desc_min=1;
    cmd->desc_max=1;
    cmd->repeat=1;
		
    i=3;
    while(i<argc) {
        if(strcmp(argv[i], "-type") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                if(strcmp(argv[i], "tcpv4") == 0) {
                    cmd->type=0;
                } else if(strcmp(argv[i], "udpv4") == 0) {
                    cmd->type=1;
                } else if(strcmp(argv[i], "tcpv6") == 0) {
                    cmd->type=2;
                } else if(strcmp(argv[i], "udpv6") == 0) {
                    cmd->type=3;
                } else {
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
                if(i<argc) {
                    printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                    if(strcmp(argv[i], "tso") == 0 || strcmp(argv[i], "ufo") == 0) {
                        cmd->toe_tx=1;
                        cmd->tso_ufo=1;
                        printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                        i++;							
                    } else if(strcmp(argv[i], "toe") == 0) {
                        cmd->toe_tx=1;
                        printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                        i++;							
                    }
                }
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*type*/} else if(strcmp(argv[i], "-dest") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                if(strcmp(argv[i], "uni") == 0) {
                    cmd->dest_type=0;
                } else if(strcmp(argv[i], "mul") == 0) {
                    cmd->dest_type=1;
                } else if(strcmp(argv[i], "bro") == 0) {
                    cmd->dest_type=2;
                } else {
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*dest*/} else if(strcmp(argv[i], "-len") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->len = strtoul(argv[i], &endptr, 10);
                if(cmd->len==0 || cmd->len>(64*1024)) {
                    printf("Invalid packet len value!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                cmd->max_len = cmd->len;
                i++;
                if(i<argc && argv[i][0]!='-') {
                    printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                    cmd->max_len= strtoul(argv[i], &endptr, 10);
                    if(cmd->max_len<cmd->len || cmd->max_len>(64*1024)) {
                        printf("Invalid desc [Max Len value]!\n");
                        valid_cmd_line = 0;
                        printf("Invalid Command line: %d.\n", __LINE__);
                        break;
                    }
                    i++;
                }
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*len*/ } else if(strcmp(argv[i], "-mtu") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->mtu = strtoul(argv[i], &endptr, 10);
                if(cmd->mtu==0 || cmd->mtu>1510) {
                    printf("Invalid mtu value!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                cmd->max_mtu = cmd->mtu;
                i++;
                if(i<argc && argv[i][0]!='-') {
                    printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                    cmd->max_mtu= strtoul(argv[i], &endptr, 10);
                    if(cmd->max_mtu<cmd->mtu || cmd->max_mtu>1510) {
                        printf("Invalid desc [Max Len value]!\n");
                        valid_cmd_line = 0;
                        printf("Invalid Command line: %d.\n", __LINE__);
                        break;
                    }
                    i++;
                }
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*mtu*/ } else if(strcmp(argv[i], "-desc") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
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
                if(i<argc && argv[i][0]!='-') {
                    printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                    cmd->desc_max = strtoul(argv[i], &endptr, 10);
                    if(cmd->desc_max<cmd->desc_min || cmd->desc_max>MAX_TX_DESC_CNT) {
                        printf("Invalid desc [total number]!\n");
                        valid_cmd_line = 0;
                        printf("Invalid Command line: %d.\n", __LINE__);
                        break;
                    }
                    i++;
                }
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*desc*/} else if(strcmp(argv[i], "-repeat") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->repeat = strtoul(argv[i], &endptr, 10);
                if(cmd->repeat==0) {
                    printf("Invalid repeat [value]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
/*repeat*/  }
        } else if(strcmp(argv[i], "-vlan") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->vlan = strtoul(argv[i], &endptr, 16);
                if(cmd->vlan==0) {
                    printf("Invalid vlan [tag]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }
/*vlan*/} else if(strcmp(argv[i], "-extv6hoplen") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->hop_hd_len = strtoul(argv[i], &endptr, 10);
                if(cmd->hop_hd_len==0) {
                    printf("Invalid extv6hoplen [value]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            }  /*extv6hoplen*/
        } else if(strcmp(argv[i], "-extv6destlen") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->dst_hd_len = strtoul(argv[i], &endptr, 10);
                if(cmd->dst_hd_len==0) {
                    printf("Invalid extv6destlen [value]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            } /*extv6destlen*/
        } else if(strcmp(argv[i], "-extv6rtlen") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->rt_hd_len = strtoul(argv[i], &endptr, 10);
                if(cmd->rt_hd_len==0) {
                    printf("Invalid extv6rtlen [value]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            } /*extv6rtlen*/
        } else if(strcmp(argv[i], "-extv6destlen2") == 0) {
            printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
            if((++i)<argc) {
                printf("%s %d: i=%d, arg=%s\n", __FUNCTION__, __LINE__, i, argv[i]);
                cmd->dst_2_hd_len = strtoul(argv[i], &endptr, 10);
                if(cmd->dst_2_hd_len==0) {
                    printf("Invalid extv6destlen2 [value]!\n");
                    valid_cmd_line = 0;
                    printf("Invalid Command line: %d.\n", __LINE__);
                    break;
                }
                i++;
            } else {
                valid_cmd_line = 0;
                printf("Invalid Command line: %d.\n", __LINE__);
                break;
            } /* extv6destlen2 */
        } else {
            valid_cmd_line = 0;
            printf("Invalid Command line: i=%d, %d.\n", i,__LINE__);
            break;
        }
    } /* while (i < argc) */
    if(!valid_cmd_line) {
        printf("Invalid Command line: %d.\n", __LINE__);
        return -1;
    }
    if(!(cmd->tso_ufo) && cmd->len>cmd->mtu) {
        printf("len param invalid: %d.\n", __LINE__);
        return -1;
    }
    if(cmd->len<cmd->desc_max) {
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
	return 0;
}



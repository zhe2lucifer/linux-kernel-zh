/** 
 *	@file: 		drv_flash_test.c
 *    @brief:   flash driver test
 *    @author:  hank.chou
 *    @date:	2016-06-24
 *    @version:	1.0.0
 *    @note:    ALi corp. all rights reserved. 2016 copyright (C)
 */
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <mtd/mtd-user.h>

#include <time.h> 


void usage()
{
    printf("Usage: mtd_write\n"
           "-p [path] -i [idx_block]\n"
           "ALL WRITE RANDOM VARIABLE\n");
}

//static int FUNC_FLASH_TEST(cmd_data* data) 
int main(int argc, char* argv[])
{
    int i, j, opt, fd, ret, idx_block, start, len;
    char *path;
    unsigned char *buf;
    mtd_info_t mtd_info;
    erase_info_t ei; 
    
    while ((opt = getopt(argc, argv, "p:i:")) != -1) {
        switch(opt){
            case 'i':
                idx_block = atoi(optarg);
                break;
            case 'p':
                path = optarg;
                break;
            default:
                usage();
                return 0;
        }
    }
    
    fd = open(path, O_RDWR);
    if(fd < 0){
        printf("can't open %s\n", path);
        usage();
        return -1;
    }
    
    ioctl(fd, MEMGETINFO, &mtd_info);
    
    printf("MTD Path: %s\nMTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
            path, mtd_info.type, mtd_info.size, mtd_info.erasesize);
    printf("MTD flags: %x\nMTD writesize: %x bytes\nMTD oobsize: %x bytes\n",
            mtd_info.flags, mtd_info.writesize, mtd_info.oobsize);
    
    start = (idx_block)*mtd_info.erasesize;
    len = mtd_info.erasesize;

    buf = (unsigned char*)malloc(sizeof(unsigned char)*mtd_info.writesize);
    
    memset(buf, 0x22, mtd_info.writesize);
    
    ei.start = start;
    ei.length = len;   //set the erase block size
    
    printf("start:0x%x, len:0x%x\n", start, len);
    
    ret = ioctl(fd, MEMERASE, &ei);
    if(ret)
        printf("erase failed, ret:%d\n", ret);
    
    lseek(fd, start, SEEK_SET);
    for(i = 0; i < (mtd_info.erasesize/mtd_info.writesize); i++){
        lseek(fd, start+(i*mtd_info.writesize), SEEK_SET);
        
        ret = write(fd, (void*)buf, mtd_info.writesize);
        if(mtd_info.writesize != ret){
            printf("write failed ret:%d, writeisize:%d\n", ret, mtd_info.writesize);
            goto done;
        }
        
        lseek(fd, start+(i*mtd_info.writesize), SEEK_SET);
        ret = read(fd, buf, mtd_info.writesize);
        if(ret != mtd_info.writesize)
            printf("ret:%d\n", ret);
        for(j = 0; j < mtd_info.writesize; j++){
            if(buf[j] != 0x22){
                printf("compare failed buf[%d]:%d\n", j, buf[j]);
                printf("stop testing!!\n");
                goto done;
            }
        }
    }
    printf("write success!\n");
done:
    free(buf);
    buf = NULL;
    close(fd);
	return ret;
}

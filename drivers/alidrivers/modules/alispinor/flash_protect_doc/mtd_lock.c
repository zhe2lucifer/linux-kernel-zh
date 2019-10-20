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

#define MEMISLOCKED		_IOR('M', 23, struct erase_info_user)
/*
 * Enumeration conversion functions
 */
typedef enum{
  lock,
  unlock,
  is_lock
}e_command;

void usage()
{
    printf("Usage: mtd_lock\n"
           "-u:unlock\n"
           "-i:is_lock\n"
           "default:lock\n"
           "-p [path] -s [start] -l [len]\n");
    
    
}

//static int FUNC_FLASH_TEST(cmd_data* data) 
int main(int argc, char* argv[])
{
    int opt, fd, ret;
    char *path;
    erase_info_t s_lock;
    mtd_info_t mtd_info;
    e_command cmd;
    cmd = lock;
    
    while ((opt = getopt(argc, argv, "p:s:l:ui")) != -1) {
        switch(opt){
            case 's':
                s_lock.start = atoi(optarg);
                break;
            case 'l':
                s_lock.length = atoi(optarg);
                break;
            case 'p':
                path = optarg;
                break;
            case 'u':
                cmd = unlock;
                break;
            case 'i':
                cmd = is_lock;
                break;
            default:
                usage();
                return 0;
        }
    }
    
    fd = open(path, O_RDWR);
    if(fd < 0){
        printf("can't open %s\n", path);
        return -1;
    }
    
    ioctl(fd, MEMGETINFO, &mtd_info);
    
    printf("MTD Path: %s\nMTD Type: %x\nMTD total size: %x bytes\nMTD erase size: %x bytes\n",
            path, mtd_info.type, mtd_info.size, mtd_info.erasesize);
    printf("MTD flags: %x\nMTD writesize: %x bytes\nMTD oobsize: %x bytes\n",
            mtd_info.flags, mtd_info.writesize, mtd_info.oobsize);
    
    
    if(lock == cmd){
        ret = ioctl(fd, MEMLOCK, &s_lock);   // get the device info
        printf("ret:%d\n", ret);
    }else if(unlock == cmd){
        ret = ioctl(fd, MEMUNLOCK, &s_lock);   // get the device info
        printf("ret:%d\n", ret);
    }else if(is_lock == cmd){
        ret = ioctl(fd, MEMISLOCKED, &s_lock);   // get the device info
        printf("ret:%d\n", ret);
    }else{
        usage();
    }
    
    
    
    
    close(fd);
	return ret;
}

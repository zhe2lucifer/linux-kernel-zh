cp -rfp ../linux-stable/* .
cp -fp ../linux-kernel-bak/security/Makefile   												security/Makefile                     
cp -fp ../linux-kernel-bak/security/Kconfig                           security/Kconfig                      
cp -fp ../linux-kernel-bak/kernel/printk/printk.c                     kernel/printk/printk.c                
cp -fp ../linux-kernel-bak/init/main.c                                init/main.c                           
cp -fp ../linux-kernel-bak/include/uapi/mtd/mtd-abi.h                 include/uapi/mtd/mtd-abi.h            
cp -fp ../linux-kernel-bak/include/uapi/linux/watchdog.h              include/uapi/linux/watchdog.h         
cp -fp ../linux-kernel-bak/include/uapi/linux/netlink.h               include/uapi/linux/netlink.h          
cp -fp ../linux-kernel-bak/include/uapi/linux/fb.h                    include/uapi/linux/fb.h               
cp -fp ../linux-kernel-bak/include/uapi/linux/dvb/frontend.h          include/uapi/linux/dvb/frontend.h     
cp -fp ../linux-kernel-bak/include/uapi/linux/dvb/ca.h                include/uapi/linux/dvb/ca.h           
cp -fp ../linux-kernel-bak/include/linux/vermagic.h                   include/linux/vermagic.h              
cp -fp ../linux-kernel-bak/include/linux/spi/spi.h                    include/linux/spi/spi.h               
cp -fp ../linux-kernel-bak/include/linux/security.h                   include/linux/security.h              
cp -fp ../linux-kernel-bak/include/linux/mtd/nand.h                   include/linux/mtd/nand.h              
cp -fp ../linux-kernel-bak/include/linux/mtd/mtd.h                    include/linux/mtd/mtd.h               
cp -fp ../linux-kernel-bak/include/linux/mmc/host.h                   include/linux/mmc/host.h              
cp -fp ../linux-kernel-bak/include/linux/init.h                       include/linux/init.h                  
cp -fp ../linux-kernel-bak/include/asm-generic/vmlinux.lds.h          include/asm-generic/vmlinux.lds.h     
cp -fp ../linux-kernel-bak/fs/ubifs/recovery.c                        fs/ubifs/recovery.c                   
cp -fp ../linux-kernel-bak/fs/proc/root.c                             fs/proc/root.c                        
cp -fp ../linux-kernel-bak/fs/Makefile                                fs/Makefile                           
cp -fp ../linux-kernel-bak/fs/Kconfig                                 fs/Kconfig                            
cp -fp ../linux-kernel-bak/drivers/usb/host/ohci-hcd.c                drivers/usb/host/ohci-hcd.c           
cp -fp ../linux-kernel-bak/drivers/usb/host/ehci-q.c                  drivers/usb/host/ehci-q.c             
cp -fp ../linux-kernel-bak/drivers/usb/host/ehci-hcd.c                drivers/usb/host/ehci-hcd.c           
cp -fp ../linux-kernel-bak/drivers/usb/host/Kconfig                   drivers/usb/host/Kconfig              
cp -fp ../linux-kernel-bak/drivers/usb/gadget/Makefile                drivers/usb/gadget/Makefile           
cp -fp ../linux-kernel-bak/drivers/usb/gadget/Kconfig                 drivers/usb/gadget/Kconfig            
cp -fp ../linux-kernel-bak/drivers/tty/serial/Makefile                drivers/tty/serial/Makefile           
cp -fp ../linux-kernel-bak/drivers/tty/serial/Kconfig                 drivers/tty/serial/Kconfig            
cp -fp ../linux-kernel-bak/drivers/spi/spi.c                          drivers/spi/spi.c                     
cp -fp ../linux-kernel-bak/drivers/spi/Kconfig                        drivers/spi/Kconfig                   
cp -fp ../linux-kernel-bak/drivers/regulator/Makefile                drivers/regulator/Makefile          
cp -fp ../linux-kernel-bak/drivers/regulator/Kconfig                  drivers/regulator/Kconfig             
cp -fp ../linux-kernel-bak/drivers/net/Kconfig                        drivers/net/Kconfig                   
cp -fp ../linux-kernel-bak/drivers/mtd/ubi/wl.c                       drivers/mtd/ubi/wl.c                  
cp -fp ../linux-kernel-bak/drivers/mtd/nand/nand_bbt.c                drivers/mtd/nand/nand_bbt.c           
cp -fp ../linux-kernel-bak/drivers/mtd/nand/nand_base.c               drivers/mtd/nand/nand_base.c          
cp -fp ../linux-kernel-bak/drivers/mtd/mtdpart.c                      drivers/mtd/mtdpart.c                 
cp -fp ../linux-kernel-bak/drivers/mtd/mtdcore.c                      drivers/mtd/mtdcore.c                 
cp -fp ../linux-kernel-bak/drivers/mtd/mtdchar.c                      drivers/mtd/mtdchar.c                 
cp -fp ../linux-kernel-bak/drivers/mtd/Makefile                       drivers/mtd/Makefile                  
cp -fp ../linux-kernel-bak/drivers/mtd/Kconfig                        drivers/mtd/Kconfig                   
cp -fp ../linux-kernel-bak/drivers/mmc/core/Makefile                  drivers/mmc/core/Makefile             
cp -fp ../linux-kernel-bak/drivers/misc/Makefile                      drivers/misc/Makefile                 
cp -fp ../linux-kernel-bak/drivers/misc/Kconfig                       drivers/misc/Kconfig                  
cp -fp ../linux-kernel-bak/drivers/mfd/Kconfig                        drivers/mfd/Kconfig                   
cp -fp ../linux-kernel-bak/drivers/input/Makefile                     drivers/input/Makefile                
cp -fp ../linux-kernel-bak/drivers/input/Kconfig                      drivers/input/Kconfig                 
cp -fp ../linux-kernel-bak/drivers/i2c/busses/Makefile                drivers/i2c/busses/Makefile           
cp -fp ../linux-kernel-bak/drivers/i2c/busses/Kconfig                 drivers/i2c/busses/Kconfig            
cp -fp ../linux-kernel-bak/drivers/gpio/Makefile                      drivers/gpio/Makefile                 
cp -fp ../linux-kernel-bak/drivers/cpufreq/Makefile                   drivers/cpufreq/Makefile              
cp -fp ../linux-kernel-bak/drivers/cpufreq/Kconfig.arm                drivers/cpufreq/Kconfig.arm           
cp -fp ../linux-kernel-bak/drivers/Makefile                           drivers/Makefile                      
cp -fp ../linux-kernel-bak/drivers/Kconfig                            drivers/Kconfig                       
cp -fp ../linux-kernel-bak/crypto/Kconfig                             crypto/Kconfig                        
cp -fp ../linux-kernel-bak/arch/mips/mm/init.c                        arch/mips/mm/init.c                   
cp -fp ../linux-kernel-bak/arch/mips/mm/fault.c                       arch/mips/mm/fault.c                  
cp -fp ../linux-kernel-bak/arch/mips/mm/cache.c                       arch/mips/mm/cache.c                  
cp -fp ../linux-kernel-bak/arch/mips/mm/c-r4k.c                       arch/mips/mm/c-r4k.c                  
cp -fp ../linux-kernel-bak/arch/mips/lib/delay.c                      arch/mips/lib/delay.c                 
cp -fp ../linux-kernel-bak/arch/mips/kernel/vmlinux.lds.S             arch/mips/kernel/vmlinux.lds.S        
cp -fp ../linux-kernel-bak/arch/mips/kernel/traps.c                   arch/mips/kernel/traps.c              
cp -fp ../linux-kernel-bak/arch/mips/kernel/setup.c                   arch/mips/kernel/setup.c              
cp -fp ../linux-kernel-bak/arch/mips/kernel/head.S                    arch/mips/kernel/head.S               
cp -fp ../linux-kernel-bak/arch/mips/kernel/genex.S                   arch/mips/kernel/genex.S              
cp -fp ../linux-kernel-bak/arch/mips/kernel/entry.S                   arch/mips/kernel/entry.S              
cp -fp ../linux-kernel-bak/arch/mips/include/asm/stackframe.h         arch/mips/include/asm/stackframe.h    
cp -fp ../linux-kernel-bak/arch/mips/include/asm/io.h                 arch/mips/include/asm/io.h            
cp -fp ../linux-kernel-bak/arch/mips/include/asm/delay.h              arch/mips/include/asm/delay.h         
cp -fp ../linux-kernel-bak/arch/mips/include/asm/cacheflush.h         arch/mips/include/asm/cacheflush.h    
cp -fp ../linux-kernel-bak/arch/mips/include/asm/bootinfo.h           arch/mips/include/asm/bootinfo.h      
cp -fp ../linux-kernel-bak/arch/mips/Kconfig                          arch/mips/Kconfig                     
cp -fp ../linux-kernel-bak/arch/mips/Kbuild.platforms                 arch/mips/Kbuild.platforms            
cp -fp ../linux-kernel-bak/arch/arm/tools/mach-types                  arch/arm/tools/mach-types             
cp -fp ../linux-kernel-bak/arch/arm/mm/mmu.c                          arch/arm/mm/mmu.c                     
cp -fp ../linux-kernel-bak/arch/arm/kernel/vmlinux.lds.S              arch/arm/kernel/vmlinux.lds.S         
cp -fp ../linux-kernel-bak/arch/arm/kernel/setup.c                    arch/arm/kernel/setup.c               
cp -fp ../linux-kernel-bak/arch/arm/kernel/head.S                     arch/arm/kernel/head.S                
cp -fp ../linux-kernel-bak/arch/arm/Makefile                          arch/arm/Makefile                     
cp -fp ../linux-kernel-bak/arch/arm/Kconfig                           arch/arm/Kconfig    
git add  arch/*
git add  block/*
git add  crypto/*
git add  Documentation/*
git add  drivers/*
git add  firmware/*
git add  fs/*
git add  include/*
git add  init/*
git add  ipc/*
git add  kernel/*
git add  lib/* 
git add  mm/*   
git add  net/* 
git add  samples/* 
git add  scripts/* 
git add  security/* 
git add  sound/* 
git add  tools/* 
git add  usr/* 
git add  virt/*

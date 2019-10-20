/*platform header for ALi's SOC setting */

/*CHIP config*/
#define CHIP_C3921
//#define CHIP_S3922
/*#define CHIP_S3922C*/

/*os config*/
#define	LINUX_PDK
//#define	LINUX_rosetop
/*#define	LINUX_buildroot_S3922*/


/*GPU controll reg*/
#ifdef	CHIP_C3921
#define	MALI_IO_BASE	0x19000000
#define	ALI_GPU_CTRL_REG	0x7C
#define	ALI_GPU_CTRL_mask	0xffcfffff /*clk = reg[21:20]*/
#endif

#ifdef	CHIP_S3922
#define	MALI_IO_BASE	0x1A200000
#define	ALI_GPU_CTRL_REG	0x310
#define	ALI_GPU_CTRL_mask	0xfff8ffff /*in S3922 SOC ver. release, clk control = [0x310]reg[18:16]*/

#endif

#ifdef	CHIP_S3922C
#define	MALI_IO_BASE	0x1A200000
#define	ALI_GPU_CTRL_REG	0xA0
#define	ALI_GPU_CTRL_mask	0xfffff8ff /*in S3922C SOC ver. mid, clk control = [0x2CA0]reg[10:8]*/

#endif


/*<ALi>20160822 Yashi add for DFS test*/
/*#define ALI_INTERNAL_GATED_TEST*/

/*include ALI_HWREG_SET_UINT32*/
#ifdef LINUX_PDK
#include <../arch/arm/mach-ali3921/include/mach/ali-s3921.h>
#endif

#ifdef LINUX_rosetop
#include <../arch/arm/mach-ali/include/mach/m3733.h>
#endif

#ifdef	LINUX_buildroot_S3922
/*<ALi>20161118 Yashi add for extract fb0 info from device tree*/
#include <linux/of.h>
#include <linux/of_device.h>

unsigned int ali_sysreg_mapping_init( void );
unsigned int ali_read_sysreg( unsigned int reg);
void ali_write_sysreg( unsigned int val, unsigned int reg);

#define	ALI_HWREG_SET_UINT32	ali_write_sysreg
#define	ALI_HWREG_GET_UINT32	ali_read_sysreg

#endif

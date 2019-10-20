#ifndef __CA_KL_VSC_H__
#define __CA_KL_VSC_H__

/*!@addtogroup KeyLadder
 *  @{
    @~
 */
 
#include <linux/ioctl.h>

#define KL_DEV_BASE 0xc0
/*!< KL ioctl cmd base.*/

#define KL_VSC_REC_KEY_SIZE (16)
/*!< KL pvr record encrypted key size*/

#define KL_VSC_UK_KEY_SIZE (48)
/*!< KL encrypted uk key size*/


/*! @struct kl_vsc_rec_en_key
 *   @brief encrypted key for running the vsc dedicated key ladder
 *			on the PVR record.
 */
struct kl_vsc_rec_en_key {
	char en_key[KL_VSC_REC_KEY_SIZE];
	/*!< Array containing encrypted CW key for decrypting.*/
	int parity;
	/*!< Content key parity mode,
		#KL_CK_PARITY_EVEN for even parity,
		#KL_CK_PARITY_ODD for odd parity.
		#KL_CK_PARITY_ODD_EVEN for default */
};

struct kl_vsc_uk_en_key {
	char en_key[KL_VSC_UK_KEY_SIZE];
	/*!< Array containing encrypted key for uk*/
};

/* Used for KL ioctl
*/


#define KL_VSC_REC_EN_KEY         _IOW(KL_DEV_BASE, 0x50, struct kl_vsc_rec_en_key)
/*!< 
	This command would send the encyrpted key for the PVR record
	in the vsc dedicated key ladder.

    int ioctl(dev, #KL_VSC_REC_EN_KEY, struct *kl_vsc_rec_en_key);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

#define KL_VSC_UK_EN_KEY         _IOW(KL_DEV_BASE, 0x51, struct kl_vsc_uk_en_key)
/*!< 
	This command would send the encyrpted key for the UK
	in the vsc dedicated key ladder.

    int ioctl(dev, #KL_VSC_UK_EN_KEY, struct *kl_vsc_uk_en_key);

    It return 0 if successful and -1 if error.
    Variable errno contains the detailed error.
*/

/*!
@}
*/
#endif

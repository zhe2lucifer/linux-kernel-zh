#ifndef __CA_OTP_DTS_H__
#define __CA_OTP_DTS_H__

#include <linux/of.h>


/*!@addtogroup OTP_KERNEL
 *  @{
    @~
 */	

#define OTP_IDX_MAX_NUM (16)

/*! @struct of_ali_otp_val
 *   @brief Parameters for query OTP information define in DTS.
 */
struct of_ali_otp_val {
	char *label;
	/*!< string contains the OTP info to be queried.*/
	int *valptr;
	/*!< buffer for the queried value.*/
};

/**
 * of_parse_ali_otp - Obtain key memory address from key ladder file 
descriptor
 * @dn: Device node of the requester
 * @label: String contains the OTP information to be queried
 * @val: Buffer for the queried value
 *
 * The return value is 0 in the case of success or a negative error code.
 */
int of_parse_ali_otp(struct device_node *dn, char *label, int *val);

/**
 * of_parse_ali_otp - Obtain key memory address from key ladder file 
descriptor
 * @dn: Device node of the requester
 * @otps: Strings contain OTP information to be queried
 * @num_otps: number of the strings
 *
 * The return value is 0 in the case of success or a negative error code.
 */
int of_parse_ali_otp_list(struct device_node *dn,
			  struct of_ali_otp_val *otps, int num_otps);


#endif /*__CA_OTP_DTS_H__*/


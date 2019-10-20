#ifndef _CA_ASA_H
#define _CA_ASA_H

#include <ca_dsc.h>

/*!@addtogroup CA_ASA
 *	@{
	@~
 */

/*! @page p_history_asa ASA Changes history
 *	- <b> 1.1 - 14-Dec-2015 Youri Zhang </b>
 *	  - Update the ASA device name from ca_asa to dsc2
 *	- <b> 1.0 - 4-May-2015 Youri Zhang </b>
 *	  - Initial Release
*/

#define CA_ASA_DEV "/dev/dsc2"
/*!< CERT-ASA device node, used for accessing the CERT-ASA logic.
	ASA is using the same ioctls as DSC, only part of the ioctls defined in
	ca_dsc.h are supported by ASA.

	List as below:
		CA_SET_FORMAT
		CA_CREATE_KL_KEY
		CA_ADD_PIDS
		CA_DEL_PIDS
		CA_DELETE_KEY
		CA_DIO_WRITE_READ
*/

#define CA_ALGO_ASA (0x10)
/*!< CERT-ASA algorithm identifier. */

/*!
@}
*/

#endif

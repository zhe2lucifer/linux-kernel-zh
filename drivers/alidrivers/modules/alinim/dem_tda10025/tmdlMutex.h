/* 
* NXP TDA10025 silicon tuner driver 
* 
* Copyright (C) 2016 <A.TANT> 
* 
*    This program is free software; you can redistribute it and/or modify 
*    it under the terms of the GNU General Public License as published by 
*    the Free Software Foundation; either version 2 of the License, or 
*    (at your option) any later version. 
* 
*    This program is distributed in the hope that it will be useful, 
*    but WITHOUT ANY WARRANTY; without even the implied warranty of 
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
*    GNU General Public License for more details. 
* 
*    You should have received a copy of the GNU General Public License along 
*    with this program; if not, write to the Free Software Foundation, Inc., 
*    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
*/ 

#ifndef TMDLMUTEX_H
#define TMDLMUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================================*/
/*                       INCLUDE FILES                                        */
/*============================================================================*/

/*============================================================================*/
/*                       MACRO DEFINITION                                     */
/*============================================================================*/

/* SW Error codes */
#define MUTEX_ERR_BASE               (CID_COMP_TIMER | CID_LAYER_HWAPI)
#define MUTEX_ERR_COMP               (CID_COMP_TIMER | CID_LAYER_HWAPI | TM_ERR_COMP_UNIQUE_START)

#define MUTEX_ERR_INIT_FAILED        (MUTEX_ERR_BASE + TM_ERR_INIT_FAILED)
#define MUTEX_ERR_BAD_PARAMETER      (MUTEX_ERR_BASE + TM_ERR_BAD_PARAMETER)
#define MUTEX_ERR_NOT_SUPPORTED      (MUTEX_ERR_BASE + TM_ERR_NOT_SUPPORTED)
#define MUTEX_ERR_NOT_INSTALLED      (MUTEX_ERR_COMP + 0x0001)
#define MUTEX_ERR_ABANDONED          (MUTEX_ERR_COMP + 0x0002)


/*============================================================================*/
/*                       ENUM OR TYPE DEFINITION                              */
/*============================================================================*/


/*============================================================================*/
/*                       EXTERN DATA DEFINITION                               */
/*============================================================================*/



/*============================================================================*/
/*                       EXTERN FUNCTION PROTOTYPES                           */
/*============================================================================*/

extern tmErrorCode_t
tmdlMutexInit
(
    ptmbslFrontEndMutexHandle *ppMutex
);

extern tmErrorCode_t
tmdlMutexDeInit
(
    ptmbslFrontEndMutexHandle pMutex
);

extern tmErrorCode_t
tmdlMutexAcquire
(
    ptmbslFrontEndMutexHandle	pMutex,
    UInt32									 	timeOut    
);

extern tmErrorCode_t
tmdlMutexRelease
(
    ptmbslFrontEndMutexHandle pMutex
);

#ifdef __cplusplus
}
#endif

#endif /* TMDLMUTEX_H */
/*============================================================================*/
/*                            END OF FILE                                     */
/*============================================================================*/


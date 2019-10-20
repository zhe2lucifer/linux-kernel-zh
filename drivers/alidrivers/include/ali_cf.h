/*
**                      DO NOT REMOVE THIS NOTICE
**
** This file is Copyright 2010 by Cryptography Research, Inc.
** All rights reserved.  Unauthorized use (including, without
** limitation, distribution and copying) of this file or derivatives
** is strictly prohibited.  Only persons and companies with explicit
** written authorization and nondisclosure agreements (Agreements) with
** Cryptography Research may use or possess this file, and all use is
** subject to the terms of the Agreements.  For additional information,
** please call +1-415-397-0123 or write to info@cryptography.com.
** This source code is provided AS-IS.
*/

/**
 * \file ali_cf.h
 * \brief Linux device driver ioctl definitions header
 * \details Define data structures and ioctls for ALi Transport CryptoFirewall
 *
 * \defgroup ali_cf_linux ALi Transport CF Linux Device Driver Interface
 */

/**
 * \mainpage ALi Transport CryptoFirewall Linux Device Driver
 *
 *  \section ali_cf_intro_sec Introduction
 *   This document describes the interfaces of the Linux device driver for the
 *   ALi Transport CryptoFirewall (CF).
 *
 *  \section ali_cf_feature_sec Driver Features
 *   The device driver provides file operation support for applications that
 *   wish to access the ALi Transport CF device.
 *
 *      \subsection ali_cf_open_close Open and Close
 *       Below is example code that shows how to open and close the device
 *       driver.
 *
 *      \code
 *
 *      #include <stdio.h>
 *      #include <sys/ioctl.h>
 *      #include <fcntl.h>
 *      #include <unistd.h>
 *      #include <string.h>
 *      #include <errno.h>
 *      #include <ali_cf.h>
 *
 *      #include "cfTypes.h"
 *
 *      #define TCFDEV "/dev/ali_cf0"
 *
 *      int fd;
 *      int retval = 0;
 *
 *      ...
 *
 *      fd = open(TCFDEV, O_RDWR);
 *      if (fd < 0) {
 *          printf("Failed to open Transport CF device \'%s\'.\n", TCFDEV);
 *          return -1;
 *      } else {
 *          printf("Opened Transport CF device \'%s\'.\n", TCFDEV);
 *      }
 *
 *      ...
 *
 *      close(fd);

 *      return 0;
 *
 *      \endcode
 *
 *      \subsection ali_cf_init Initialization
 *       Below is example code that shows how to initialize the Transport CF for
 *       normal operation.
 *
 *      \code
 *
 *      ALI_CF_VERSION version;
 *      ALI_CF_FEATURE feature;
 *      ALI_CF_CF_STATUS cf_status;
 *      ALI_CF_TRANS_STATUS trans_status;
 *      ALI_CF_DECM_STATUS decm_status;
 *      ALI_CF_OPERATION feature_op;
 *      ALI_CF_OPERATION diff_op;
 *      ALI_CF_RESULT result;
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_VERSION_INFO, &version);
 *      if(retval < 0) {
 *          printf("Version ioctl failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: Version Epoch %d\n"
 *                 "              Manufacturer Id %d\n"
 *                 "              Netlist Version %d\n"
 *                 "              Build Id %d.\n",
 *                 version.versionEpoch, version.manufacturerId,
 *                 version.netlistVersion, version.versionBuildId);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_CF_STATUS, &cf_status);
 *      if(retval < 0) {
 *          printf("CF status ioctl failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: NVM Status 0x%01x\n"
 *                 "              Differentiation Status %01x\n"
 *                 "              Recent Reset %01x\n"
 *                 "              CF Alert %01x\n"
 *                 "              Development Mode %01x\n"
 *                 "              Fuse Activate %01x\n"
 *                 "              Fuse Block %01x\n",
 *                 cf_status.nvmStatus,
 *                 cf_status.differentiationStatus,
 *                 cf_status.recentReset,
 *                 cf_status.cfAlert,
 *                 cf_status.developmentMode,
 *                 cf_status.fuseActivate,
 *                 cf_status.fuseBlock);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_FEATURE_VECTOR, &feature);
 *      if(retval < 0) {
 *          printf("Feature vector ioctl failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: Feature Vector 0x%08x.\n",
 *                 feature.featureVector);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_ISSUE_OP_FEATURE, &feature_op);
 *      if(retval < 0) {
 *          printf("Feature operation ioctl failed with errno %d.\n", errno);
 *      }
 *
 *      retval = ioctl(fd, ALI_CF_IOC_READ_OP_RESULT, &result);
 *      if(retval < 0) {
 *          printf("Read result ioctl failed with errno %d.\n", errno);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_ISSUE_OP_DIFF, &diff_op);
 *      if(retval < 0) {
 *          printf("Diff operation ioctl failed with errno %d.\n", errno);
 *      }
 *
 *      retval = ioctl(fd, ALI_CF_IOC_READ_OP_RESULT, &result);
 *      if(retval < 0) {
 *          printf("Read result ioctl failed with errno %d.\n", errno);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_CF_STATUS, &cf_status);
 *      if(retval < 0) {
 *          printf("CF status ioctl failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: NVM Status 0x%01x\n"
 *                 "              Differentiation Status %01x\n"
 *                 "              Recent Reset %01x\n"
 *                 "              CF Alert %01x\n"
 *                 "              Development Mode %01x\n"
 *                 "              Fuse Activate %01x\n"
 *                 "              Fuse Block %01x\n",
 *                 cf_status.nvmStatus,
 *                 cf_status.differentiationStatus,
 *                 cf_status.recentReset,
 *                 cf_status.cfAlert,
 *                 cf_status.developmentMode,
 *                 cf_status.fuseActivate,
 *                 cf_status.fuseBlock);
 *      }
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_FEATURE_VECTOR, &feature);
 *      if(retval < 0) {
 *          printf("Feature vector ioctl failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: Feature Vector 0x%08x.\n",
 *                 feature.featureVector);
 *      }
 *
 *      ...
 *
 *      \endcode
 *
 *      \subsection ali_cf_normal Normal Operation
 *       Below is example code that shows how to issue CWCs to the Transport CF
 *       during normal operation, this scenario assumes the hardware DECM valid
 *       signal must be tracked by the Transport CF.
 *
 *      \code
 *
 *      ALI_CF_OPERATION cwc_op;
 *      ALI_CF_RESULT result;
 *
 *      ...
 *
 *      retval = write(fd, &cwc_op, sizeof(ALI_CF_OPERATION));
 *      if(retval < 0) {
 *          printf("CWC operation failed with errno %d.\n", errno);
 *      }
 *
 *      retval = read(fd, &result, sizeof(ALI_CF_RESULT));
 *      if(retval < 0) {
 *          printf("Read result failed with errno %d.\n", errno);
 *      } else {
 *          printf("Transport CF: Transaction Result"
 *                 "              Operation Type 0x%01x\n"
 *                 "              Transaction status 0x%01x\n"
 *                 "              CWC Valid %01x\n"
 *                 "              SHV[0] 0x%08x\n"
 *                 "              SHV[1] 0x%08x\n"
 *                 "              SHV[2] 0x%08x\n"
 *                 "              SHV[3] 0x%08x\n",
 *                 result.operationType,
 *                 result.transactionStatus,
 *                 result.cwcValid,
 *                 result.shv[0], result.shv[1],
 *                 result.shv[2], result.shv[3]);
 *      }
 *
 *      ...
 *
 *      \endcode
 *
 *       Below is example code that shows how to issue CWCs to the Transport CF
 *       during normal operation, this scenario assumes the hardware DECM valid
 *       signal is not tracked by another device.
 *
 *      \code
 *
 *      ALI_CF_OPERATION cwc_op;
 *      ALI_CF_RESULT result;
 *      ALI_CF_DECM_STATUS decm_status;
 *
 *      ...
 *
 *      retval = ioctl(fd, ALI_CF_IOC_DECM_STATUS, &decm_status);
 *      if(retval < 0) {
 *          printf("DECM status vector ioctl failed with errno %d.\n", errno);
 *      }
 *
 *      if (decm_status.hwDecmValid) {
 *          retval = write(fd, &cwc_op, sizeof(ALI_CF_OPERATION));
 *          if(retval < 0) {
 *              printf("CWC operation failed with errno %d.\n", errno);
 *          }
 *
 *          retval = ioctl(fd, ALI_CF_IOC_DECM_STATUS, &decm_status);
 *          if(retval < 0) {
 *              printf("DECM status vector ioctl failed with errno %d.\n", errno);
 *          } else {
 *              if (decm_status.hwDecmError) {
 *                  printf("Transport CF: Error sampling Hardware DECM for CWC.\n");
 *              }
 *          }
 *
 *          retval = read(fd, &result, sizeof(ALI_CF_RESULT));
 *          if(retval < 0) {
 *              printf("Read result failed with errno %d.\n", errno);
 *          } else {
 *              printf("Transport CF: Transaction Result"
 *                     "              Operation Type 0x%01x\n"
 *                     "              Transaction status 0x%01x\n"
 *                     "              CWC Valid %01x\n"
 *                     "              SHV[0] 0x%08x\n"
 *                     "              SHV[1] 0x%08x\n"
 *                     "              SHV[2] 0x%08x\n"
 *                     "              SHV[3] 0x%08x\n",
 *                     result.operationType,
 *                     result.transactionStatus,
 *                     result.cwcValid,
 *                     result.shv[0], result.shv[1],
 *                     result.shv[2], result.shv[3]);
 *          }
 *      } else {
 *          printf("Unable to issue CWC, Hardware DECM not valid.\n");
 *      }
 *
 *      ...
 *
 *      \endcode
 */

#ifndef _ALI_CF_H
#define _ALI_CF_H

/* -------------------------------------------------------------------------- */

/** @page p_history Changes history
 *
 *  - <b> 1.0.0 - 6-April-2016 Youri Zhang </b>
 *    - Initial Release
*/

/* -------------------------------------------------------------------------- */

/** @mainpage Overview
 *  - @subpage p_history
 *  - @subpage p_preface
 *
 *  <hr>Copyright &copy; 2015 ALi Corporation. All rights reserved.\n
 *  ZhuHai, China\n
 *  Tel: +86 756 3392000 \n
 *  http://www.alitech.com
 *
 *  All trademarks and registered trademarks are the property of their
 *  respective owners.
 *
 *  This document is supplied with an understanding that the notice(s)
 *  herein or any other contractual agreement(s) made that instigated
 *  the delivery of a hard copy, electronic copy, facsimile or file transfer
 *  of this document are strictly observed and maintained.
 *
 *  The information contained in this document is subject to change without
 *  notice.
 *
 *  <b>Security Policy of ALi Corporation</b>\n
 *  Any recipient of this document, without exception, is subject to a
 *  Non-Disclosure Agreement (NDA) and access authorization.
*/

/* -------------------------------------------------------------------------- */

/** @page p_preface Preface
 *  <h2>Objectives</h2>
 *  This document specifies the API that gives access to the CRI CryptoFirewall
 *  IP (CF).
 *
 *
 *  <hr><h2>Audience</h2>
 *  This document is intended for developers in charge of implementing or
 *  maintaining the CF API and as well as the developers that
 *  intend to develop the CF related applications.
 * <hr><h2>References</h2>
 *  - [CF] CF Programming Guide, Version 1.0.0\n
*/

/*!@addtogroup ali_cf_linux
 *  @{
    @~
 */

#define CF_DEV "/dev/ali_cf0"
/*!< CF device node, used for accessing the CF logic.*/
#define CF_DEV_TARGET "/dev/ali_cf_target"
/*!< CF device node, used for set the CF_CWC target key information.
 Only supports the ioctl: #ALI_CF_IOC_SET_CWC_TARGET.
*/

/**
 * \ingroup ali_cf_linux
 * \brief Version values parsed from the \link #CF_REG_ADDR_VERSION \endlink
 *        register
 * \details Read via the \link #ALI_CF_IOC_VERSION_INFO \endlink ioctl.
 */
typedef struct ali_cf_version {
    /**
     *  Version Epoch, from field (\link #CF_VERSION_VERSION_EPOCH_MASK \endlink <<
     *  \link #CF_VERSION_VERSION_EPOCH_POS \endlink)
     */
    unsigned char versionEpoch;
    /**
     *  Manufacturer Identifier, from field (\link #CF_VERSION_MANUFACTURER_ID_MASK
     *  \endlink << \link #CF_VERSION_MANUFACTURER_ID_POS \endlink)
     */
    unsigned char manufacturerId;
    /**
     *  Netlist Version, from field (\link #CF_VERSION_NETLIST_VERSION_MASK
     *  \endlink << \link #CF_VERSION_NETLIST_VERSION_POS \endlink)
     */
    unsigned char netlistVersion;
    /**
     *  Build Identifier, from field (\link #CF_VERSION_BUILD_ID_MASK \endlink <<
     *  \link #CF_VERSION_BUILD_ID_POS \endlink)
     */
    unsigned char versionBuildId;
} ALI_CF_VERSION;

/**
 * \ingroup ali_cf_linux
 * \brief Feature Vector value from the \link #CF_REG_ADDR_FEATURE \endlink
 *        register
 * \details Read via the \link #ALI_CF_IOC_FEATURE_VECTOR \endlink ioctl.
 */
typedef struct ali_cf_feature {
    /**
     *  Current feature vector, set via the \link #ALI_CF_IOC_ISSUE_OP_FEATURE
     *  \endlink ioctl.
     */
    unsigned int featureVector;
} ALI_CF_FEATURE;

/**
 * \ingroup ali_cf_linux
 * \brief Overall status values parsed from the \link #CF_REG_ADDR_STATUS
 *        \endlink and  \link #CF_REG_ADDR_PLATFORM \endlink registers
 * \details Read via the \link #ALI_CF_IOC_CF_STATUS \endlink ioctl.
 */
typedef struct ali_cf_cf_status {
    /**
     *  Current status of Non-Volatile Memory, from field (\link #CF_NVM_STATUS_MASK
     *  \endlink << \link #CF_NVM_STATUS_POS \endlink) in the \link #CF_REG_ADDR_STATUS
     *  \endlink register. Valid values are: \link #CF_NVM_STATUS_VALUES \endlink.
     */
    unsigned char nvmStatus;
    /**
     *  Flag indicating Differentiation status of the Transport CF, from field
     *  (\link #CF_DIFF_STATUS_MASK \endlink << \link #CF_DIFF_STATUS_POS \endlink)
     *  in the \link #CF_REG_ADDR_STATUS \endlink register. Valid values are:
     *  \link #CF_DIFF_STATUS_VALUES \endlink.  Flag is set after a successful
     *  \link #ALI_CF_IOC_ISSUE_OP_DIFF \endlink ioctl.
     */
    unsigned char differentiationStatus;
    /**
     *  Flag indicating Transport CF was recently reset (ie no transactions have
     *  been issued since reset), from field (\link #CF_PLATFORM_RECENT_RESET_MASK
     *  \endlink << \link #CF_PLATFORM_RECENT_RESET_POS \endlink) in the
     *  \link #CF_REG_ADDR_PLATFORM \endlink register.
     */
    unsigned char recentReset;
    /**
     *  Flag indicating Transport CF alert has been tripped, from field
     *  (\link #CF_PLATFORM_CF_ALERT_MASK \endlink << \link #CF_PLATFORM_CF_ALERT_POS
     *  \endlink) in the \link #CF_REG_ADDR_PLATFORM \endlink register.
     */
    unsigned char cfAlert;
    /**
     *  Flag indicating Transport CF is in development mode, from field
     *  (\link #CF_PLATFORM_DEVELOPMENT_MODE_MASK \endlink <<
     *  \link #CF_PLATFORM_DEVELOPMENT_MODE_POS \endlink) in the
     *  \link #CF_REG_ADDR_PLATFORM \endlink register.
     */
    unsigned char developmentMode;
    /**
     *  Flag indicating state of Transport CF activation fuse, from field
     *  (\link #CF_PLATFORM_CF_ACTIVATED_MASK \endlink <<
     *  \link #CF_PLATFORM_CF_ACTIVATED_POS \endlink) in the
     *  \link #CF_REG_ADDR_PLATFORM \endlink register.
     */
    unsigned char fuseActivate;
    /**
     *  Flag indicating state of Transport CF block fuse, from the
     *  \link #CF_REG_ADDR_PLATFORM \endlink register.
     */
    unsigned char fuseBlock;
} ALI_CF_CF_STATUS;

/**
 * \ingroup ali_cf_linux
 * \brief Transaction status values parsed from the \link #CF_REG_ADDR_STATUS
 *        \endlink register
 * \details Read via the \link #ALI_CF_IOC_TRANS_STATUS \endlink ioctl.
 */
typedef struct ali_cf_trans_status {
    /**
     *  Current status of transaction, from field (\link #CF_TRANS_STATUS_MASK
     *  \endlink << \link #CF_TRANS_STATUS_POS \endlink). Valid values are:
     *  \link #CF_TRANS_STATUS_VALUES \endlink.
     */
    unsigned char transactionStatus;
    /**
     *  Enforced value of the useNvmKey flag for the current transaction, from
     *  field (\link #CF_REQUESTED_USE_NVM_KEY_MASK \endlink <<
     *  \link #CF_REQUESTED_USE_NVM_KEY_POS \endlink).
     */
    unsigned char useNvmKey;
    /**
     *  Enforced value of the operationType for the current transaction, from
     *  field (\link #CF_REQUESTED_OPERATION_TYPE_MASK \endlink <<
     *  \link #CF_REQUESTED_OPERATION_TYPE_POS \endlink). Valid values are:
     *  \link #CF_OPERATION_TYPE_VALUES \endlink.
     */
    unsigned char operationType;
    /**
     *  Enforced value of the decmSource for the current transaction, from
     *  field (\link #CF_REQUESTED_DECM_SOURCE_MASK \endlink <<
     *  \link #CF_REQUESTED_DECM_SOURCE_POS \endlink). Valid values are:
     *  \link #CF_DECM_SOURCE_VALUES \endlink.
     */
    unsigned char decmSource;
    /**
     *  Enforced value of the outputUsage for the current transaction, from
     *  field (\link #CF_REQUESTED_OUTPUT_USAGE_MASK \endlink <<
     *  \link #CF_REQUESTED_OUTPUT_USAGE_POS \endlink). Valid values are:
     *  \link #CF_OUTPUT_USAGE_VALUES \endlink.
     */
    unsigned char outputUsage;
    /**
     *  Enforced value of the productRange flag for the current transaction, from
     *  field (\link #CF_REQUESTED_PRODUCT_RANGE_MASK \endlink <<
     *  \link #CF_REQUESTED_PRODUCT_RANGE_POS \endlink).
     */
    unsigned char productRange;
    /**
     *  Enforced value of the productOffset for the current transaction, from
     *  field (\link #CF_REQUESTED_PRODUCT_OFFSET_MASK \endlink <<
     *  \link #CF_REQUESTED_PRODUCT_OFFSET_POS \endlink).
     */
    unsigned char productOffset;
} ALI_CF_TRANS_STATUS;

/**
 * \ingroup ali_cf_linux
 * \brief DECM status values parsed from the \link #CF_REG_ADDR_PLATFORM
 *        \endlink register
 * \details Read via the \link #ALI_CF_IOC_DECM_STATUS \endlink ioctl.
 */
typedef struct ali_cf_decm_status {
    /**
     *  Flag indicating Transport CF hardware DECM valid input signal is asserted,
     *  from field (\link #CF_PLATFORM_HW_DECM_VALID_MASK \endlink <<
     *  \link #CF_PLATFORM_HW_DECM_VALID_POS \endlink).
     */
    unsigned char hwDecmValid;
    /**
     *  Flag indicating Transport CF hardware DECM valid input signal was not
     *  asserted during read of hardware DECM for the current transaction, from
     *  field (\link #CF_PLATFORM_HW_DECM_ERROR_MASK \endlink <<
     *  \link #CF_PLATFORM_HW_DECM_ERROR_POS \endlink).
     */
    unsigned char hwDecmError;
} ALI_CF_DECM_STATUS;

/**
 * \ingroup ali_cf_linux
 * \brief Transaction inputs.
 * \details Written via the \link #ALI_CF_IOC_ISSUE_OP_FEATURE \endlink,
 *  \link #ALI_CF_IOC_ISSUE_OP_DIFF \endlink, \link #ALI_CF_IOC_ISSUE_OP_CWCORSHV
 *  \endlink, and \link #ALI_CF_IOC_ISSUE_OP \endlink ioctls and the write function.
 */
typedef struct ali_cf_operation {
    /**
     * Operation input payload, written to the \link #CF_REG_ADDR_INPUT \endlink
     * register.
     */
    unsigned int operation[21];
} ALI_CF_OPERATION;

/**
 * \ingroup ali_cf_linux
 * \brief Transaction results.
 * \details Read via the \link #ALI_CF_IOC_READ_OP_RESULT \endlink ioctl and the
 *  read function.
 */
typedef struct ali_cf_result {
    /**
     *  Enforced value of the operationType for the current transaction, from
     *  field (\link #CF_REQUESTED_OPERATION_TYPE_MASK \endlink <<
     *  \link #CF_REQUESTED_OPERATION_TYPE_POS \endlink) in the \link #CF_REG_ADDR_STATUS
     *  \endlink register. Valid values are: \link #CF_OPERATION_TYPE_VALUES \endlink.
     */
    unsigned char  operationType;
    /**
     *  Current status of transaction, from field (\link #CF_TRANS_STATUS_MASK
     *  \endlink << \link #CF_TRANS_STATUS_POS \endlink) in the \link #CF_REG_ADDR_STATUS
     *  \endlink register. Valid values are: \link #CF_TRANS_STATUS_VALUES \endlink.
     *  Note: Value should be \link #CF_TRANS_STATUS_DONE_OK \endlink upon successful
     *  completion of the transaction.
     */
    unsigned char  transactionStatus;
    /**
     *  Flag indicating Transport CF CWC valid output signal was asserted, from
     *  field (\link #CF_PLATFORM_HW_DECM_VALID_MASK \endlink <<
     *  \link #CF_PLATFORM_HW_DECM_VALID_POS \endlink).
     */
    unsigned char  cwcValid;
    /**
     *  Resulting secure hash value if operation type was \link #CF_OPERATION_TYPE_OP_CWC
     *  \endlink with output usage set to \link #CF_OUTPUT_USAGE_SHV \endlink, or all zeros
     *  for all other operations. Values read from the \link #CF_REG_ADDR_OUTPUT \endlink
     *  register.
     */
    unsigned int shv[4];
} ALI_CF_RESULT;

/**
 * \ingroup ali_cf_linux
 * \brief Valid transaction status field values
 */
#define CF_TRANS_STATUS_BUSY_INIT (0x0) /**< Reset from power on condition. */
#define CF_TRANS_STATUS_BUSY_SETUP (0x1)  /**< Reset after a command has completed. */
#define CF_TRANS_STATUS_READY (0x2)  /**< CF ready to receive a new transaction. */
#define CF_TRANS_STATUS_INPUT (0x3)  /**< CF is receiving a new transaction. */
#define CF_TRANS_STATUS_BUSY_OP (0x4) /**< CF is processing transaction. */
#define CF_TRANS_STATUS_DONE_OK (0x8) /**< Transaction was completed successfully. */
#define CF_TRANS_STATUS_DONE_ERROR (0x9)  /**< Transaction has completed with an error. */
#define CF_TRANS_STATUS_DONE_MFR_TEST (0xE)  /**< Special case of successful test unlock. */

/**
 * \ingroup ali_cf_linux
 * \brief Valid NVM status field values
 */
#define CF_NVM_STATUS_READY (0x0) /**< NVM personalized and ready. */
#define CF_NVM_STATUS_UNBISTED (0x1) /**< NVM is unbisted.  Currently unsupported state and should be treated as an error. */
#define CF_NVM_STATUS_UNPERSO (0x2) /**< NVM is pre-personalized. */
#define CF_NVM_STATUS_ERROR (0x3) /**< NVM personalized and ready. */

/**
 * \ingroup ali_cf_linux
 * \brief Valid differentiation status field values
 */
#define CF_DIFF_STATUS_UNDIFFERENTIATED (0x0)    /**< CF is undifferentiated. */
#define CF_DIFF_STATUS_DIFFERENTIATED (0x1)     /**< CF is differentiated. */

/**
 * \ingroup ali_cf_linux
 * \brief Valid operation type request field values
 */
#define CF_OPERATION_TYPE_OP_CWC (0x0) /**< Request an CWC generation operation. */
#define CF_OPERATION_TYPE_OP_PERSO (0x1) /**< Request a Personalization operation. */
#define CF_OPERATION_TYPE_OP_DIFF (0x2) /**< Request a Differentiation operation. */
#define CF_OPERATION_TYPE_OP_FEATURE (0x3) /**< Request a Feature operation. */
#define CF_OPERATION_TYPE_OP_MFR_TEST (0x4) /**< Request a Manufacturer Test operation. */
#define CF_OPERATION_TYPE_OP_UNDEFINED1 (0x5) /**< Reserved operation. */
#define CF_OPERATION_TYPE_OP_UNDEFINED2 (0x6) /**< Reserved operation. */
#define CF_OPERATION_TYPE_OP_UNDEFINED3 (0x7) /**< Reserved operation. */

/**
 * \ingroup ali_cf_linux
 * \brief Valid output usage request field values
 */
#define CF_OUTPUT_USAGE_SHV (0x0) /**< Output a secure hash value. */
#define CF_OUTPUT_USAGE_CWC_DIRECT (0x1) /**< Direct transfer of CWC to key table. */
#define CF_OUTPUT_USAGE_CWC_XOR (0x2) /**< XOR of key table entry with CWC. */
#define CF_OUTPUT_USAGE_CWC_AES_KEY (0x3) /**< 128-bit AES decrypt of key table entry with CWC. */

/**
 * \ingroup ali_cf_linux
 * \brief Valid DECM source request field values
 */
#define CF_DECM_SOURCE_NONE (0x0) /**< Invalid DECM source. Should not be used in normal operation. */
#define CF_DECM_SOURCE_SW (0x1) /**< DECM source from software. */
#define CF_DECM_SOURCE_KEY (0x2) /**< DECM source from key table. */
#define CF_DECM_SOURCE_MIX (0x3) /**< Combine DECM input from software and key table. */

/**
 * \ingroup ali_cf_linux
 * \brief Even or Odd parity when #ALI_CF_IOC_SET_CWC_TARGET
 */
#define CF_PARITY_ODD (1<<0) /**< Store to odd key SRAM. */
#define CF_PARITY_EVEN (1<<1) /**< Store to even key SRAM. */

/**
 * \ingroup ali_cf_linux
 * \brief Set CF CWC target information.
 * \details Written via the \link #ALI_CF_IOC_SET_CWC_TARGET \endlink,
 * ioctl..
 */
struct ali_cf_cwc_target {
    /**
     * fd contains the KL even/odd SRAM(s) for storing the CWC output.
     */
    int fd;
    /**
     * CWC output will be stored to even or odd SRAM.
     * If the fd is configured for the crypto algorithms using 64-bit key,
     * this parameter will be ignored. Valid values are:
     * #CF_PARITY_ODD or #CF_PARITY_EVEN.
     */
    int parity;
};

/*
 * Ioctl definitions
 */
#define ALI_CF_IOC_MAGIC  (0xCF)
/*!< CF IO cmd base in kernel.*/

/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF version information ioctl command.
 * \details Values returned in \link #ALI_CF_VERSION \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          </ul>
 */
#define ALI_CF_IOC_VERSION_INFO _IOR(ALI_CF_IOC_MAGIC, 0, struct ali_cf_version)

/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF feature vector ioctl command.
 * \details Values returned in \link #ALI_CF_FEATURE \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          </ul>
 */
#define ALI_CF_IOC_FEATURE_VECTOR _IOR(ALI_CF_IOC_MAGIC, 1, struct ali_cf_feature)
/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF status ioctl command.
 * \details Values returned in \link #ALI_CF_CF_STATUS \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          </ul>
 */
#define ALI_CF_IOC_CF_STATUS _IOR(ALI_CF_IOC_MAGIC, 2, struct ali_cf_cf_status)
/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF transaction status ioctl command.
 * \details Values returned in \link #ALI_CF_TRANS_STATUS \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          </ul>
 */
#define ALI_CF_IOC_TRANS_STATUS _IOR(ALI_CF_IOC_MAGIC, 3, struct ali_cf_trans_status)
/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF DECM status ioctl command.
 * \details Values returned in \link #ALI_CF_DECM_STATUS \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          </ul>
 */
#define ALI_CF_IOC_DECM_STATUS _IOR(ALI_CF_IOC_MAGIC, 4, struct ali_cf_decm_status)
/**
 * \ingroup ali_cf_linux
 * \brief Issue Feature operation Transport CF transaction.
 * \details Operation payload sent in \link #ALI_CF_OPERATION \endlink structure.
 * Command word must be set to \link #CF_DEFAULT_OP_FEATURE_CMD \endlink.<br>
 * On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to read values from argument.
 *          <li> EOPNOTSUPP - command word is not a Feature operation.
 *          <li> EIO - error writing operation payload to Transport CF.
 *          <li> EBUSY - Transport CF is busy with another operation.
 *          <li> EACCES - Transport CF is in the wrong state.
 *          </ul>
 */
#define ALI_CF_IOC_ISSUE_OP_FEATURE  _IOW(ALI_CF_IOC_MAGIC, 5, struct ali_cf_operation)
/**
 * \ingroup ali_cf_linux
 * \brief Issue Differentiation operation Transport CF transaction.
 * \details Operation payload sent in \link #ALI_CF_OPERATION \endlink structure.
 * Command word must be set to \link #CF_DEFAULT_OP_DIFF_CMD \endlink.<br>
 * On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to read values from argument.
 *          <li> EOPNOTSUPP - command word is not a Differentiation operation.
 *          <li> EIO - error writing operation payload to Transport CF.
 *          <li> EBUSY - Transport CF is busy with another operation.
 *          <li> EACCES - Transport CF is in the wrong state.
 *          </ul>
 */
#define ALI_CF_IOC_ISSUE_OP_DIFF  _IOW(ALI_CF_IOC_MAGIC, 6, struct ali_cf_operation)
/**
 * \ingroup ali_cf_linux
 * \brief Issue CWC or SHV operation Transport CF transaction.
 * \details Operation payload sent in \link #ALI_CF_OPERATION \endlink structure.
 *          Operation type field in command word must be set to
 *          \link #CF_OPERATION_TYPE_OP_CWC \endlink.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to read values from argument.
 *          <li> EOPNOTSUPP - command word is not a CWC or SHV operation.
 *          <li> EIO - error writing operation payload to Transport CF.
 *          <li> EBUSY - Transport CF is busy with another operation.
 *          <li> EACCES - Transport CF is in the wrong state.
 *          </ul>
 *          Note: Same functionality is also implemented in the write function.
 */
#define ALI_CF_IOC_ISSUE_OP_CWCORSHV _IOW(ALI_CF_IOC_MAGIC, 7, struct ali_cf_operation)
/**
 * \ingroup ali_cf_linux
 * \brief Issue generic Transport CF transaction.
 * \details Operation payload sent in \link #ALI_CF_OPERATION \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - not in DEBUG mode or invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to read values from argument.
 *          <li> EIO - error writing operation payload to Transport CF.
 *          <li> EBUSY - Transport CF is busy with another operation.
 *          <li> EACCES - Transport CF is in the wrong state.
 *          </ul>
 *          Note: This ioctl is only available in when driver is in DEBUG mode.
 */
#define ALI_CF_IOC_ISSUE_OP _IOW(ALI_CF_IOC_MAGIC, 8, struct ali_cf_operation)
/**
 * \ingroup ali_cf_linux
 * \brief Read Transport CF transaction results.
 * \details Block while outstanding transaction completes, read results of completed
 * operation, and advance Transport CF state machine for next transaction
 * ioctl command. Values returned in \link #ALI_CF_DECM_STATUS \endlink
 * structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - not in DEBUG mode or invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          <li> ETIMEDOUT - timed out waiting for transaction to complete.
 *          <li> EACCES - Transport CF has no outstanding transaction.
 *          </ul>
 *          Note: Same functionality is also implemented in the read function.
 */
#define ALI_CF_IOC_READ_OP_RESULT _IOR(ALI_CF_IOC_MAGIC, 9, struct ali_cf_result)

/**
 * \ingroup ali_cf_linux
 * \brief Wait for outstanding CF transaction to complete and return status ioctl
 *        command.
 * \details Block while outstanding transaction completes and return transaction
 *          status ioctl command. Values returned in \link #ALI_CF_TRANS_STATUS
 *          \endlink structure.<br>
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> ENOTTY - not in DEBUG mode or invalid ioctl command.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          <li> ETIMEDOUT - timed out waiting for transaction to complete.
 *          <li> EACCES - Transport CF has no outstanding transaction.
 *          </ul>
 */
#define ALI_CF_IOC_WAIT_OP_DONE _IOR (ALI_CF_IOC_MAGIC, 10, struct ali_cf_trans_status)

/**
 * \ingroup ali_cf_linux
 * \brief Set the CWC target fd (file descriptor) which is associated with the key table.
 * \details A valid target fd should be set to CF before issuing the CWC operation,
 *      CWC operation needs to know where to store the output.
 *          On ioctl failure, errno is set to:
 *          <ul>
 *          <li> ENODEV - no character or Transport CF device available.
 *          <li> EBADF - Invalid file descriptor.
 *          <li> ENOTTY - not in DEBUG mode or invalid ioctl command.
 *          <li> EINVAL - invalid argument pointer.
 *          <li> EFAULT - unable to write results to argument.
 *          <li> EACCES - Transport CF has no outstanding transaction.
 *          </ul>
 *          Note: Same functionality is also implemented in the read function.
 */
#define ALI_CF_IOC_SET_CWC_TARGET _IOW(ALI_CF_IOC_MAGIC, 20, struct ali_cf_cwc_target)

/*!
@}
*/

#endif


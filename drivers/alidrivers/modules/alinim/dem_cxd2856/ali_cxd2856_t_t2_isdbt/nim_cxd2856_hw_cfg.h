#ifndef NIM_CXD2856_HW_CFG_H
#define NIM_CXD2856_HW_CFG_H
#if 1
/*****************************************************************************
* int nim_cxd2856_open(struct nim_device *dev)
* Description: real initialize cxd2856 hardware
*
* Arguments: 
*  	nim_device :nim private data
* Return Value: success if ok
*****************************************************************************/
int		nim_cxd2856_open(struct nim_device *dev);


/*****************************************************************************
* int nim_cxd2856_close(struct nim_device *dev)
* Description: shutdown demod and tuner
*
* Arguments:
* 	nim_device :nim private data
* Return Value: success
*****************************************************************************/
int 	nim_cxd2856_close(struct nim_device *dev);

/*****************************************************************************
* void nim_set_dev_config(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
* Description: set dts param  to dev private data and set the common function point for tuner
*			
* Arguments: 
*	nim_device:nim private data
*	nim_cfg:
*  
* Return Value: success if ok
*****************************************************************************/
int 	nim_set_dev_config(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg);

/*****************************************************************************
* static void nim_cxd2838_hwreset(void)
* Description: cxd2838 device reset
*
* Arguments:
*  	nim_device :nim private data
* Return Value: success
*****************************************************************************/
int 	nim_cxd2856_hwreset(struct nim_device *dev);

/*****************************************************************************
* INT32 ali_cxd2856_nim_hw_initialize(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg)
* Description: init nim 
*
* Arguments:
* 	nim_device:nim private data
*	nim_cfg:param of demod and tuner
* Return Value: success if ok
*****************************************************************************/
int 	ali_cxd2856_nim_hw_initialize(struct nim_device *dev, struct ali_nim_mn88436_cfg *nim_cfg);
#endif
#endif 

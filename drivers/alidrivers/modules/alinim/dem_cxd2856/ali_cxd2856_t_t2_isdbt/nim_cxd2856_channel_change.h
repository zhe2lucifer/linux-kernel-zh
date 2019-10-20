#ifndef NIM_CXD2856_CHANNEL_CHNAGE_H
#define NIM_CXD2856_CHANNEL_CHNAGE_H
enum
{
	TUNE,
	BLIND_TUNE
};
int nim_cxd2856_channel_change(struct nim_device *dev,struct NIM_CHANNEL_CHANGE *change_para);

#endif 

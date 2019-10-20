This is the source code for tuner RT710 or RT720.

I named the interface as tun_rt7x0_xxxx. 

The x means 1,2,..., and may be it will mean 3 in the future.

In the RT710.c, it recognized which tuner is used automatic.

/*****************************************************************************************
I2C_Read_Len(tuner_id,&RT710_Read_Len) ; // read data length
if((RT710_Read_Len.Data[3]&0xF0)==0x70) //TRUE:RT710(R3[7:4]=7) ; FALSE;RT720(R3[7:4]=0)
	Chip_type_flag=TRUE;
else
	Chip_type_flag=FALSE;
****************************************************************************************/
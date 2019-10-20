#ifndef _ALI_CIC_H_
#define _ALI_CIC_H_

#define CIC_MAX_SLOT_NUM	1
#define CIC_MSG_MAX_LEN		512

#define CIC_GET_SLOT_INFO  _IOR('o', 130, struct cic_slot_info)
#define CIC_SET_SLOT_INFO  _IOR('o', 131, struct cic_slot_info)
#define CIC_GET_MSG        _IOR('o', 132, struct cic_msg)
#define CIC_SEND_MSG       _IOW('o', 133, struct cic_msg)
//#define CIC_SET_PORT_ID _IOW('o', 134, unsigned long)
#define CIC_GET_GET_KUMSGQ 	_IOR('o', 135, unsigned int)

#define CIC_CAM_REMOVED 0x00
#define CIC_CAM_INSERTED 0x03

enum cic_msg_type {
	CIC_DATA = 0,	/* CI data register */
	CIC_CSR,	/* CI command/stauts register */
	CIC_SIZELS,	/* CI size register low bytes */
	CIC_SIZEMS,	/* CI size register high bytes */
	CIC_MEMORY,	/* CI memory space*/
	CIC_BLOCK,	/* CI block data Read/Write */
};

enum cic_signal_type {
	CIC_ENSTREAM = 0,	/* Emanciption stream (bypass stream to CAM) */
	CIC_ENSLOT,		/* Enable slot */
	CIC_RSTSLOT,		/* Reset slot */
	CIC_IOMEM,		/* Enable IO & memory space */
	CIC_SLOTSEL,		/* Select slot */
	CIC_CARD_DETECT,	/* CD information */
	CIC_CARD_READY		/* Ready/busy information */
};

/* slot interface types and info */
struct cic_slot_info {
	int num;		/* slot number */
	int type;		/* CA interface this slot supports */
	unsigned int flags;
};

/* a message to/from a CI-CAM */
struct cic_msg {
	unsigned int index;
	unsigned int type;
	unsigned int length;
	unsigned char msg[CIC_MSG_MAX_LEN];
};

typedef enum 
{
	AUI_CIC_CB_READY,		/* ready flags */
	AUI_CIC_CB_CAM_DETECT,	/* insert or detach CAM card */
	AUI_CIC_CB_NB
}cic_cb_para_type;

struct cic_cb_para
{
	int slot;				/* slot 0 or 1,only slot = 0 for the present */
	cic_cb_para_type flags; 			/* parameter type,see */
	unsigned int status;				/* message buffer */
};

#endif

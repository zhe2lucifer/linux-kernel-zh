#ifndef __LLD_NIM_DVBS_C3505_SEARCH_PLSN_H__
#define __LLD_NIM_DVBS_C3505_SEARCH_PLSN_H__

#include "../porting_linux_header.h"

//#define SEARCH_PLSN_DEBUG

#ifdef SEARCH_PLSN_DEBUG
    #define SEARCH_PLSN_PRINTF(fmt, args...)  libc_printf("SEARCH_PLSN_PRINTF: " fmt, ##args)
#else
	#define SEARCH_PLSN_PRINTF(...)
#endif

INT32 nim_c3505_generate_table(struct nim_device *dev);
INT32 nim_c3505_start_generate(struct nim_device *dev, struct ali_plsn_address *p_addr);
INT32 nim_c3505_release_table(struct nim_device *dev);
INT32 nim_c3505_search_plsn_exit(struct nim_device *dev);
INT32 nim_c3505_search_plsn_pilot(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 *plsn_pilot_i, INT32 *plsn_pilot_q, INT32 plsn_pilot_cnt, INT32 goldn_now);
INT32 nim_c3505_search_plsn(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 *valid_frame_s, INT32 *valid_frame_m);
INT32 nim_c3505_search_plsn_top(struct nim_device *dev);
void nim_c3505_pl_scrambling(INT32 *plds_i, INT32 *plds_q, INT32 plds_len, INT32 start_addr, INT32 goldn_now, INT32 plsn_search_type);
void nim_c3505_get_bbs_sq(INT32 plsn_search_type, INT32 bbs_sq_start, INT32 bbs_sq_end, UINT32 *para_data);
void nim_c3505_pl_descrambling(INT32 *plds_out_i, INT32 *plds_out_q, INT32 *plds_in_i, INT32 *plds_in_q, INT32 plds_len, INT32 gold_n, INT32 start_addr);
INT32 nim_c3505_get_map_table(INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *p0_num, INT32 *p1_num, INT32 *p2_num);
void nim_c3505_demap_point_hard_decison(INT32 plsn_search_type, INT32 gold_n, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 point_num, INT32 start_addr);
void nim_c3505_demap_area_hard_decison(INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_in_i, INT32 *plsn_in_q, INT32 plf_len, INT32 cmp_start_addr);
INT32 nim_c3505_search_plsn_null(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 goldn_now);
INT32 nim_c3505_search_plsn_bbh(struct nim_device *dev, UINT8 data_src, INT32 plsn_search_type, INT32 frame_mode, INT32 modu_type, INT32 coderate, INT32 *plsn_data_i, INT32 *plsn_data_q, INT32 plsn_data_cnt, INT32 goldn_now);
INT32 nim_c3505_plsn_gold_to_root(INT32 plsn_gold);
INT32 nim_c3505_plsn_root_to_gold(INT32 plsn_root);

#endif

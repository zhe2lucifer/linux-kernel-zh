#ifndef __LLD_NIM_DVBS_C3505_PLSN_PLSYNC_H__
#define __LLD_NIM_DVBS_C3505_PLSN_PLSYNC_H__

#include "../porting_linux_header.h"

//#define NIM_PLSYNC_DEBUG

#ifdef NIM_PLSYNC_DEBUG
    #define NIM_PLSYNC_PRINTF(fmt, args...)  printk("NIM_PLSYNC_PRINTF: " fmt, ##args)
#else
	#define NIM_PLSYNC_PRINTF(...)
#endif


    // --------------------------------- //

    #define  NUM_FRAME      22
    #define  NUM_SCRMB      8

    #define  NO    0
    #define  YES   1

    #define  UNKNOWN        0x3
    #define  UNKNOWN_BIT    0x3

    #define  CAPTURE   0
    #define  TRACK     1

    #define  CAP_ACM   0
    #define  CAP_CCM   1

    #define  SOF_SIZE             26
    #define  RM_PLSC_SIZE         64
    #define  CONV_PLSC_SIZE       154
    #define  RM_HEADER_SIZE      (SOF_SIZE + RM_PLSC_SIZE)
    #define  CONV_HEADER_SIZE    RM_HEADER_SIZE//(SOF_SIZE + CONV_PLSC_SIZE)
    #define  VLSNR_HEADER_SIZE    900

    #define  DATA                  0
    #define  PILOT_32              1
    #define  PILOT_34              2
    #define  PILOT_36              3
    #define  RM_HEADER_90          4
    #define  CONV_HEADER_180       5
    #define  VLSNR_HEADER_900      6
    #define  UNKNOWN_SYM_TYPE      7

    #ifndef  PI
        #define  PI   (3.14159265358979f)
    #endif

    // determind by hardware delay;
    #define  FHT_SYMBOL_DELAY             0// 64
    #define  RM_DECODER_SYMBOL_DELAY      0//(SOF_SIZE + (2 * FHT_SYMBOL_DELAY))
    #define  CONV_DECODER_SYMBOL_DELAY    0// 300

    // May use other value instead of 5;
    #define  PLS_DECODER_SYMBOL_DELAY     186//(((CONV_DECODER_SYMBOL_DELAY > RM_DECODER_SYMBOL_DELAY) 
                                             //? (CONV_DECODER_SYMBOL_DELAY) : (RM_DECODER_SYMBOL_DELAY)) + 5)

    // May use other value instead of 50;
    //#define  DECODER_RESULT_IS_READY      (PLS_DECODER_SYMBOL_DELAY + 0)

    #define  NUM_PATH             2
    #define  LAG                  3

    #define  PHASE_FIFO_SIZE  (CONV_HEADER_SIZE - RM_HEADER_SIZE + 5)//((CONV_HEADER_SIZE >> 1) + 5)

    #define  MAX_FRAME_SIZE     33282
    #define  MIN_FRAME_SIZE     3330
    #define  DUMMY_FRAME_SIZE   3330

    #define  SET1        0
    #define  SET2        1

    #define  NORMAL      0
    #define  SHORT       1
    #define  MEDIUM      2
    #define  LONG        NORMAL

    #define  PILOT_ON    1
    #define  PILOT_OFF   0

    #define  M_DUMMY     0
    #define  M_BPSK      1
    #define  M_QPSK      2
    #define  M_8PSK      3
    #define  M_16APSK    4
    #define  M_32APSK    5
    #define  M_64APSK    6
    #define  M_128APSK   7
    #define  M_256APSK   8
    #define  M_1024ARY   10
    #define  M_8APSK     M_8PSK
    #define  M_8ARY      M_8PSK
    #define  M_16ARY     M_16APSK
    #define  M_32ARY     M_32APSK
    #define  M_64ARY     M_64APSK
    #define  M_256ARY    M_256APSK


    // S2 legacy LDPC code identifier (normal or short)
    #define  CODE_ID_1_4     0
    #define  CODE_ID_1_3     1
    #define  CODE_ID_2_5     2
    #define  CODE_ID_1_2     3
    #define  CODE_ID_3_5     4
    #define  CODE_ID_2_3     5
    #define  CODE_ID_3_4     6
    #define  CODE_ID_4_5     7
    #define  CODE_ID_5_6     8
    #define  CODE_ID_8_9     9
    #define  CODE_ID_9_10    10


    // LDPC code identifier (normal)
    // see Table 5a in S2 standard (ETSI EN 302 307-1)
    // and Table 4 in S2X standard (ETSI EN 302 307-2);
    #define  NORMAL_CODE_ID_1_4       CODE_ID_1_4
    #define  NORMAL_CODE_ID_1_3       CODE_ID_1_3
    #define  NORMAL_CODE_ID_2_5       CODE_ID_2_5
    #define  NORMAL_CODE_ID_1_2       CODE_ID_1_2
    #define  NORMAL_CODE_ID_3_5       CODE_ID_3_5
    #define  NORMAL_CODE_ID_2_3       CODE_ID_2_3
    #define  NORMAL_CODE_ID_3_4       CODE_ID_3_4
    #define  NORMAL_CODE_ID_4_5       CODE_ID_4_5
    #define  NORMAL_CODE_ID_5_6       CODE_ID_5_6
    #define  NORMAL_CODE_ID_8_9       CODE_ID_8_9
    #define  NORMAL_CODE_ID_9_10      CODE_ID_9_10
    #define  NORMAL_CODE_ID_2_9       11
    #define  NORMAL_CODE_ID_13_45     12
    #define  NORMAL_CODE_ID_9_20      13
    #define  NORMAL_CODE_ID_90_180    14
    #define  NORMAL_CODE_ID_96_180    15
    #define  NORMAL_CODE_ID_11_20     16
    #define  NORMAL_CODE_ID_100_180   17
    #define  NORMAL_CODE_ID_26_45     18
    #define  NORMAL_CODE_ID_104_180   19
    #define  NORMAL_CODE_ID_18_30     20
    #define  NORMAL_CODE_ID_28_45     21
    #define  NORMAL_CODE_ID_23_36     22
    #define  NORMAL_CODE_ID_116_180   23
    #define  NORMAL_CODE_ID_20_30     24
    #define  NORMAL_CODE_ID_124_180   25
    #define  NORMAL_CODE_ID_25_36     26
    #define  NORMAL_CODE_ID_128_180   27
    #define  NORMAL_CODE_ID_13_18     28
    #define  NORMAL_CODE_ID_22_30     29
    #define  NORMAL_CODE_ID_132_180   30
    #define  NORMAL_CODE_ID_135_180   31
    #define  NORMAL_CODE_ID_7_9       32
    #define  NORMAL_CODE_ID_140_180   33
    #define  NORMAL_CODE_ID_154_180   34


    // LDPC code identifier (short)
    // see Table 5b in S2 standard (ETSI EN 302 307-1)
    // and Table 6 in S2X standard (ETSI EN 302 307-2);
    #define  SHORT_CODE_ID_1_5        CODE_ID_1_4
    #define  SHORT_CODE_ID_1_4        CODE_ID_1_4
    #define  SHORT_CODE_ID_1_3        CODE_ID_1_3
    #define  SHORT_CODE_ID_2_5        CODE_ID_2_5
    #define  SHORT_CODE_ID_1_2        CODE_ID_1_2
    #define  SHORT_CODE_ID_3_5        CODE_ID_3_5
    #define  SHORT_CODE_ID_2_3        CODE_ID_2_3
    #define  SHORT_CODE_ID_3_4        CODE_ID_3_4
    #define  SHORT_CODE_ID_4_5        CODE_ID_4_5
    #define  SHORT_CODE_ID_5_6        CODE_ID_5_6
    #define  SHORT_CODE_ID_8_9        CODE_ID_8_9
    #define  SHORT_CODE_ID_11_45      10
    #define  SHORT_CODE_ID_4_15       11
    #define  SHORT_CODE_ID_14_45      12
    #define  SHORT_CODE_ID_7_15       13
    #define  SHORT_CODE_ID_8_15       14
    #define  SHORT_CODE_ID_26_45      15
    #define  SHORT_CODE_ID_32_45      16


    // LDPC code identifier (medium)
    // see Table 5 in S2X standard (ETSI EN 302 307-2);
    #define  MEDIUM_CODE_ID_1_5       0
    #define  MEDIUM_CODE_ID_11_45     1
    #define  MEDIUM_CODE_ID_1_3       2



void pl_frame_sync_init(void);

void pl_frame_sync(
	INT32 din_i,
	INT32 din_q,
	INT32 *success,
	INT32 *start,
	INT32 *is_header,
	INT32 *is_data,
	INT32 *is_pilot,
	INT32 *sym_type,
	INT32 *dout_i,
	INT32 *dout_q,
	INT32 *frame_len,
	INT32 *modu_type,
	INT32 *pilot_on,
	INT32 *modcod,
	INT32 *code_rate,
	INT32 *fecframe_size,
	INT32 *dummy_flag,
	INT32 data_sel          //1-C3505_CR_OUT, 0-others(C3501H_EQ_OUT,C3501H_CR_OUT,C3505_EQ_OUT)
);

void acm_frame_capture(
    INT32       curr_metric_val,
    INT32       curr_b0,
    INT32       curr_b7,
    const INT32 curr_header_fifo_i[CONV_HEADER_SIZE],
    const INT32 curr_header_fifo_q[CONV_HEADER_SIZE],
    INT32       *capture,
    INT32       *cap_frame_size,
    INT32       *cap_frame_pilot_flag
);

void acm_frame_capture_init(void);

void angle_by_cordic(
    INT32 data_i,
    INT32 data_q,
    INT32 *phase
);

void combine_sof_and_plsc(
    INT32 sof_corr_i,
    INT32 sof_corr_q,
    INT32 plsc_corr_i,
    INT32 plsc_corr_q,
    INT32 *sub_flag,
    INT32 *metric_max
);


void get_cap_flag(
    INT32       curr_b7,
    const INT32 path_cap_flag[NUM_PATH],
    const INT32 path_max_metric_val[NUM_PATH],
    INT32       *cap_flag,
    INT32       *cap_idx
);


INT32 get_cos_val( INT32 angle );
INT32 get_sin_val( INT32 angle );

void metric(
    const INT32  curr_header_fifo_i[CONV_HEADER_SIZE],
    const INT32  curr_header_fifo_q[CONV_HEADER_SIZE],
    INT32        *curr_b0,
    INT32        *curr_b7,
    INT32        *curr_metric_val
);

void metric_init(void);

void acc_rm_plsc_lag1_corr( INT32 *out_i, INT32 *out_q );

void acc_rm_plsc_lag2_corr( INT32 b0, INT32 *out_i, INT32 *out_q );

void acc_rm_plsc_lag4_corr( INT32 b0, INT32 *out_i, INT32 *out_q );

void acc_sof_lag1_corr( INT32 *out_i, INT32 *out_q );

void acc_sof_lag2_corr( INT32 *out_i, INT32 *out_q );

void acc_sof_lag4_corr( INT32 *out_i, INT32 *out_q );


void reset_path(
    INT32  *path_b0,
    INT32  *path_b7,
    INT32  *path_lookup_idx,
    INT32  *path_sym_cnt,
    INT32  *path_max_metric_val,
    INT32  *path_local_max_metric_val
);


void search_each_path(
    INT32       curr_metric_val,
    INT32       last_cap_frame_size,
    INT32       path_b0,
    INT32       path_b7,
    INT32       path_max_metric_val,
    INT32       *path_reset,
    INT32       *path_cap_flag,
    INT32       *path_lookup_idx,
    INT32       *path_sym_cnt,
    INT32       *path_local_max_metric_val,
    INT32       *path_cap_frame_size,
    INT32       *path_cap_frame_pilot_flag
);


void update_diff_corr_fifo(
    const INT32 curr_header_fifo_i[CONV_HEADER_SIZE],
    const INT32 curr_header_fifo_q[CONV_HEADER_SIZE]
);

void update_path(
    INT32       curr_b0,
    INT32       curr_b7,
    INT32       curr_metric_val,
    INT32       *path_b0,
    INT32       *path_b7,
    INT32       *path_lookup_idx,
    INT32       *path_sym_cnt,
    INT32       *path_max_metric_val,
    INT32       *path_local_max_metric_val
);

void pl_signalling(
    INT32    plfs_out_i,
    INT32    plfs_out_q,
    INT32    plfs_decoder_start,
    INT32    plfs_check_cnt,
    INT32    *pls_out_i,
    INT32    *pls_out_q,
    INT32    *pls_sym_type,
    INT32    *pls_frame_start,
    INT32    *pls_fec_frame_type,
    INT32    *pls_modulation,
    INT32    *pls_code_id,
    INT32    *pls_pilot_flag,
    INT32    *pls_dummy_flag,
    INT32    *pls_frame_size,
    INT32    *pls_modcod_type);

void pl_signalling_init(void);

void check_decode_result(
    INT32    curr_metric_val,
    INT32    pls_frame_size,
    INT32    frame_synch_lock,
    INT32    *start_decoder,
    INT32    *check_en,
    INT32    *check_cnt,
    INT32    *sym_cnt,
    INT32    *local_max_val );

void frame_track(
    INT32    curr_metric_val,
    INT32    capture,
    INT32    pls_frame_size,
    INT32    frame_synch_lock,
    INT32    *decoder_start,
    INT32    *check_cnt);

void frame_track_init(void);

#define  UNKNOWN_CODE_ID    -1
#define  REG_CODE_ID_128    UNKNOWN_CODE_ID
#define  REG_CODE_ID_130    UNKNOWN_CODE_ID
#define  REG_CODE_ID_176    UNKNOWN_CODE_ID
#define  REG_CODE_ID_177    UNKNOWN_CODE_ID
#define  REG_CODE_ID_188    UNKNOWN_CODE_ID
#define  REG_CODE_ID_189    UNKNOWN_CODE_ID
#define  REG_CODE_ID_192    UNKNOWN_CODE_ID
#define  REG_CODE_ID_193    UNKNOWN_CODE_ID
#define  REG_CODE_ID_196    UNKNOWN_CODE_ID
#define  REG_CODE_ID_197    UNKNOWN_CODE_ID
#define  REG_CODE_ID_250    UNKNOWN_CODE_ID
#define  REG_CODE_ID_251    UNKNOWN_CODE_ID
#define  REG_CODE_ID_252    UNKNOWN_CODE_ID
#define  REG_CODE_ID_253    UNKNOWN_CODE_ID
#define  REG_CODE_ID_254    UNKNOWN_CODE_ID
#define  REG_CODE_ID_255    UNKNOWN_CODE_ID

INT32 get_pls_infor(
    INT32 modcod_type,
    INT32 s2_legacy_vlsnr_en,
    INT32 s2_legacy_vlsnr_modcod_type,
    INT32 *output_modulation,
    INT32 *output_code_id,
    INT32 *output_fec_frame_type,
    INT32 *output_pilot_flag,
    INT32 *output_dummy_flag,
    INT32 *output_vlsnr_flag,
    INT32 *output_vlsnr_set,
    INT32 *output_frame_size );

void get_symbol_type(
    INT32 reset,
    INT32 wideband_flag,
    INT32 pilot_flag,
    INT32 vlsnr_flag,
    INT32 vlsnr_set,
    INT32 *sym_type );

void get_symbol_type_init(void);

INT32 rm_decoder(
    INT32       data_in_i,
    INT32       data_in_q,
    INT32       sym_delay_fifo_i[PLS_DECODER_SYMBOL_DELAY],
    INT32       sym_delay_fifo_q[PLS_DECODER_SYMBOL_DELAY],
    const INT32 sof_bits[SOF_SIZE],
    const INT32 rm_plscode_scrambler[RM_PLSC_SIZE],
    const INT32 first_row_of_generator_matrix[RM_PLSC_SIZE / 2],
    INT32       preset_b0_of_modcod_type,
    INT32       preset_b6_of_modcod_type,
    INT32       preset_b7_of_modcod_type,
    INT32       *out_valid,
    INT32       *rm_decoder_out );

void rm_decoder_init(void);

void fast_hadamard_transform(
    INT32 fht_upper_i,
    INT32 fht_upper_q,
    INT32 fht_lower_i,
    INT32 fht_lower_q,
    INT32 *out_valid );

void sof_remapping(
    INT32 sof_sym_index,
    INT32 sof_sym_i,
    INT32 sof_sym_q,
    INT32 *mapped_sof_sym_i,
    INT32 *mapped_sof_sym_q );

#endif


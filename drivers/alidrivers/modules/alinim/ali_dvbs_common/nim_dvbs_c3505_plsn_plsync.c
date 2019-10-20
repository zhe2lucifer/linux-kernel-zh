#include "nim_dvbs_c3505_plsn_plsync.h"
#include "../dem_c3505/nim_dvbs_c3505.h"

static INT32 g_curr_header_fifo_i[CONV_HEADER_SIZE] = {0};  // Fixed( 1, 1,  6 );
static INT32 g_curr_header_fifo_q[CONV_HEADER_SIZE] = {0};  // Fixed( 1, 1,  6 );
static const INT32 g_search_range[NUM_FRAME] = {51,51, 51,51,51,51, 31,31,31,31, 31,31,25,25, 25,25,25,25, 20,20,20,20};  //#INT32
static const INT32 g_valid_frame_size[NUM_FRAME] = {1,1, 1,1,1,1, 1,0,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};  //#INT32

static const INT32 g_frame_size_set[NUM_FRAME] =     // Fixed( 0, 16, 0 );
{
	3330,     // 0;  pilot off; short  32APSK; Dummy frame;
	3402,     // 1;  pilot on;  short  32APSK;
	4140,     // 2;  pilot off; short  16APSK;
	4212,     // 3;  pilot on;  short  16APSK;
	5490,     // 4;  pilot off; short  8PSK or 8APSK;
	5598,     // 5;  pilot on;  short  8PSK or 8APSK;
	6570,     // 6;  pilot off  normal 1024APSK (Undefine yet);
	6714,     // 7;  pilot on;  normal 1024APSK;
	8190,     // 8;  pilot off; short  QPSK; normal 256APSK;
	8370,     // 9;  pilot on;  short  QPSK; normal 256APSK;
	9360,     // 10; pilot off; normal 128APSK;
	9576,     // 11; pilot on;  normal 128APSK;
	10890,    // 12; pilot off; normal 64APSK;
	11142,    // 13; pilot on;  normal 64APSK;
	13050,    // 14; pilot off; normal 32APSK;
	13338,    // 15; pilot on;  normal 32APSK;
	16290,    // 16; pilot off; normal 16APSK;
	16686,    // 17; pilot on;  normal 16APSK;
	21690,    // 18; pilot off; normal 8PSKK or 8APSK;
	22194,    // 19; pilot on;  normal 8PSK or 8APSK;
	32490,    // 20; pilot off; normal QPSK;
	33282     // 21; pilot on;  normal QPSK;
};

static INT32 g_local_weight     = 14;//(INT32)(16 * 0.875);
static INT32 g_path_weight      =  8;//(INT32)(16 * 0.5);
static INT32 g_metric_threshold =  4*1024*256/16;//(INT32)(16 * 0.25);

static INT32 g_cap_frame_size = 0;               // Fixed( 0, 16, 0 );  # feedback signal;
//static INT32 g_cap_frame_size_pre = 0;           // Fixed( 0, 16, 0 );  # feedback signal;
static INT32 g_cap_frame_pilot_flag = 0;         // Fixed( 0, 1,  0 );

static INT32 g_cap_cnt  = 0;
static INT32 g_sym_cnt = 0;
static INT32 g_TotSymCnt = 0;

static INT32 g_is_header = 0;
static INT32 g_is_data = 0;
static INT32 g_is_pilot = 0;

static INT32 g_pl_synch_lock       = NO;  // Fixed( 0, 1,  0 );
//static INT32 g_pl_last_synch_lock  = NO;  // Fixed( 0, 1,  0 ); //@
//static INT32 g_pl_lose_lock        = NO;  // Fixed( 0, 1,  0 );
static INT32 g_frame_synch_state   = CAPTURE;  // Fixed( 0, 1,  0 );
static INT32 g_check_cnt = 0;                  // Fixed( 0, 8,  0 );

static INT32 g_pls_frame_size = 0;             // Fixed( 0, 16, 0 );
static INT32 g_success = 0;


void pl_frame_sync_init()
{
	g_cap_cnt  = 0;
	g_sym_cnt = 0;
	g_check_cnt = 0;
	g_TotSymCnt = 0;
	
	g_frame_synch_state = 0;
	g_pl_synch_lock = 0;

	g_is_header = 0;
	g_is_data = 0;
	g_is_pilot = 0;
	g_success = 0;
	
	metric_init();
	acm_frame_capture_init();
	frame_track_init();
	pl_signalling_init();
}


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
)
{
	INT32 n = 0;
    INT32 curr_b0 = 0;
    INT32 curr_b7 = 0;
    INT32 curr_metric_val = 0;
	INT32 cap_flag = 0;
	INT32 tmp_flag = 0;

    INT32 cnt_value_when_cap_to_track = 3;
    INT32 cnt_value_when_plfs_lock    = 5;
	INT32 metric_idle = 0;
	static INT32 metric_idle_cnt = 0;

	//--------------------------------------------------------------------------------
    INT32    plfs_check_cnt = 0;                 // Fixed( 0, 8,  0 );
	INT32    lose_lock      = NO;				 // Fixed( 0, 1,  0 );
	INT32    track_start    = 0;
	INT32    pls_start      = 0;
    INT32    pls_din_i      = 0;                 // Fixed( 1, 1,  6 );
    INT32    pls_din_q      = 0;                 // Fixed( 1, 1,  6 );

    // Output of pl_signalling.c
    INT32    pls_out_i = 0;                      // Fixed( 1, 1,  6 );
    INT32    pls_out_q = 0;                      // Fixed( 1, 1,  6 );
    INT32    pls_sym_type = 0;                   // Fixed( 0, 3,  0 );
    INT32    pls_frame_start = 0;                // Fixed( 0, 1,  0 );
    INT32    pls_fec_frame_type = 0;             // Fixed( 0, 2,  0 );
    INT32    pls_modulation = 0;                 // Fixed( 0, 4,  0 );
    INT32    pls_code_id = 0;                    // Fixed( 0, 6,  0 );
    INT32    pls_pilot_flag = 0;                 // Fixed( 0, 1,  0 );
    INT32    pls_dummy_flag = 0;                 // Fixed( 0, 1,  0 );
    //INT32    pls_frame_size = 0;                 // Fixed( 0, 16, 0 );
    INT32    pls_modcod_type = 0;                // Fixed( 0, 8,  0 );
	//--------------------------------------------------------------------------------

	tmp_flag = g_success;
	g_TotSymCnt ++;

	for (n = 0; n < (CONV_HEADER_SIZE - 1); n++)
	{
		g_curr_header_fifo_i[n] = g_curr_header_fifo_i[n + 1];
		g_curr_header_fifo_q[n] = g_curr_header_fifo_q[n + 1];
	}
	g_curr_header_fifo_i[CONV_HEADER_SIZE - 1] = din_i;
	g_curr_header_fifo_q[CONV_HEADER_SIZE - 1] = din_q;

	if (g_frame_synch_state)
	{
		metric_idle_cnt ++;
		metric_idle = (metric_idle_cnt < g_pls_frame_size - 500)|| (metric_idle_cnt > g_pls_frame_size - 180);
	}
	else
	{
		metric_idle_cnt = 0;
		metric_idle = 0;
	}

	if (metric_idle)
	{
		curr_b0 = 0;
		curr_b7 = 0;
		curr_metric_val = 0;
	}
	else
	{
		metric(
			g_curr_header_fifo_i,
			g_curr_header_fifo_q,
			&curr_b0,
			&curr_b7,
			&curr_metric_val
		);
	}

	cap_flag = 0;
	acm_frame_capture(
		curr_metric_val,
		curr_b0,
		curr_b7,
		g_curr_header_fifo_i,
		g_curr_header_fifo_q,
		&cap_flag,
		&g_cap_frame_size,
		&g_cap_frame_pilot_flag
	);

	frame_track(
		curr_metric_val,
		cap_flag,
		g_pls_frame_size,
		g_pl_synch_lock,
		&track_start,
		&g_check_cnt);

	//void update_frame_synch_state(...)
    if (TRACK != g_frame_synch_state)
    {
        if (g_check_cnt >= cnt_value_when_cap_to_track)
        {
            g_frame_synch_state = TRACK;
        }
    }
    else
    {
        if ((g_check_cnt < cnt_value_when_cap_to_track) && (NO == g_pl_synch_lock))
        {
            g_frame_synch_state = CAPTURE;
        }
        if ((g_check_cnt >= cnt_value_when_plfs_lock))
        {
            g_pl_synch_lock = YES;
        }
        if ((YES == g_pl_synch_lock) &&(g_check_cnt < 1))  // Synch lost !
        {
            lose_lock = YES;
        }
    }

	if (g_frame_synch_state)
		pls_start = track_start;
	else
		pls_start = cap_flag;

	if (pls_start)
		g_sym_cnt = 0;
	else
		g_sym_cnt ++;

	pls_din_i = g_curr_header_fifo_i[0];
	pls_din_q = g_curr_header_fifo_q[0];
	if ((data_sel == 1) && (g_sym_cnt < 90) && (g_sym_cnt & 0x1))
	{
		pls_din_i = -g_curr_header_fifo_q[0];
		pls_din_q =  g_curr_header_fifo_i[0];
	}

    pl_signalling(
        pls_din_i,
        pls_din_q,
        pls_start,
        plfs_check_cnt,
        &pls_out_i,
        &pls_out_q,
        &pls_sym_type,
        &pls_frame_start,
        &pls_fec_frame_type,
        &pls_modulation,
        &pls_code_id,
        &pls_pilot_flag,
        &pls_dummy_flag,
        &g_pls_frame_size,
        &pls_modcod_type);

	g_success = g_pl_synch_lock;

	if (pls_frame_start)
		metric_idle_cnt = 0;
	if (lose_lock)
		pl_frame_sync_init();

	//Output
	*success       = g_success;
	*start         = pls_frame_start;
	*is_header     = (pls_sym_type == RM_HEADER_90);
	*is_data       = (pls_sym_type == DATA);
	*is_pilot      = (pls_sym_type == PILOT_36);
	*sym_type      = pls_sym_type;
	*dout_i        = pls_out_i;
	*dout_q        = pls_out_q;
	*frame_len     = g_pls_frame_size;
	*pilot_on      = pls_pilot_flag;
	*modu_type     = pls_modulation;
	*code_rate     = pls_code_id;
	*modcod        = pls_modcod_type;
	*fecframe_size = pls_fec_frame_type;
	*dummy_flag    = pls_dummy_flag;

	if ((*success) && (tmp_flag == 0))
	{
		NIM_PLSYNC_PRINTF("PL_SYNC %6d: Lock!\n", g_TotSymCnt);
	}

	if (*start)
	{
		NIM_PLSYNC_PRINTF("PL_SYNC %6d: modcod=%d, modu_type=%d, pilot=%d, code_rate=%d, frame_len=%d\n",
		                   g_TotSymCnt, *modcod>>2, *modu_type,   *pilot_on, *code_rate,  *frame_len);
	}
}


static INT32 g_path_b0[NUM_PATH] = {0};                    // Fixed( 0, 2,  0 );
static INT32 g_path_b7[NUM_PATH] = {0};                    // Fixed( 0, 2,  0 );
static INT32 g_path_lookup_idx[NUM_PATH] = {0};            // Fixed( 0, 5,  0 );
static INT32 g_path_sym_cnt[NUM_PATH] = {0};               // Fixed( 0, 16, 0 );
static INT32 g_path_max_metric_val[NUM_PATH] = {0};        // Fixed( 0, 1,  7 );
static INT32 g_path_local_max_metric_val[NUM_PATH] = {0};  // Fixed( 0, 1,  7 );
static INT32 g_path_cap_frame_size[NUM_PATH] = {0};        // Fixed( 0, 16, 0 );
static INT32 g_path_cap_frame_pilot_flag[NUM_PATH] = {NO}; // Fixed( 0, 1,  0 );
static INT32 g_last_cap_frame_size = 0;                    // Fixed( 0, 16, 0 );

void acm_frame_capture(
    INT32       curr_metric_val,                       // Fixed( 0, 1,  7 );
    INT32       curr_b0,                               // Fixed( 0, 2,  0 );
    INT32       curr_b7,                               // Fixed( 0, 2,  0 );
    const INT32 curr_header_fifo_i[CONV_HEADER_SIZE],  // Fixed( 1, 1,  6 );
    const INT32 curr_header_fifo_q[CONV_HEADER_SIZE],  // Fixed( 1, 1,  6 );
    INT32       *capture,                              // Fixed( 0, 1,  0 );
    INT32       *cap_frame_size,                       // Fixed( 0, 16, 0 );  no use yet;
    INT32       *cap_frame_pilot_flag                  // Fixed( 0, 1,  0 );  no use yet;
)
{

    INT32    n;
    INT32    cap_flag = NO;                   // Fixed( 0, 1, 0 );
    INT32    update_en = NO;                  // Fixed( 0, 1, 0 );
    INT32    cap_flag_path = NO;              // Fixed( 0, 1, 0 );

    INT32    cap_idx = 0;                     // Fixed( 0, 1, 0 ); if NUM_PATH = 2;
    INT32    min_idx;                         // Fixed( 0, 1, 0 ); if NUM_PATH = 2;
    INT32    update_idx = 0;                  // Fixed( 0, 1, 0 ); if NUM_PATH = 2;

    INT32    path_reset[NUM_PATH] = {NO};     // Fixed( 0, 1,  0 );
    INT32    path_cap_flag[NUM_PATH] = {NO};  // Fixed( 0, 1,  0 );
    INT32    min_metric_val_in_all_path;      // Fixed( 0, 1,  7 );

    // ========================================================================== //
    // Check path by path to decide whether caputre frame header or not.
    // ========================================================================== //
    for (n = 0; n < NUM_PATH; n++)
    {
        search_each_path(
            curr_metric_val,
            g_last_cap_frame_size,
            g_path_b0[n],
            g_path_b7[n],
            g_path_max_metric_val[n],
           &path_reset[n],
           &path_cap_flag[n],
           &g_path_lookup_idx[n],
           &g_path_sym_cnt[n],
           &g_path_local_max_metric_val[n],
           &g_path_cap_frame_size[n],
           &g_path_cap_frame_pilot_flag[n] );
    }

    get_cap_flag(
        curr_b7,
        path_cap_flag,
        g_path_max_metric_val,
        &cap_flag,
        &cap_idx );

    if (YES == cap_flag)
    {
        *capture = YES;
        *cap_frame_size = g_path_cap_frame_size[cap_idx];
        *cap_frame_pilot_flag = g_path_cap_frame_pilot_flag[cap_idx];
        g_last_cap_frame_size = g_path_cap_frame_size[cap_idx];
    }

    // ========================================================================== //
    // Path reset.
    // ========================================================================== //
    for (n = 0; n < NUM_PATH; n++)
    {
        cap_flag_path = (YES == cap_flag) && (n == cap_idx);
        if ((YES == path_reset[n]) && (!cap_flag_path))
        {
            reset_path(
                &g_path_b0[n],
                &g_path_b7[n],
                &g_path_lookup_idx[n],
                &g_path_sym_cnt[n],
                &g_path_max_metric_val[n],
                &g_path_local_max_metric_val[n]
			);
        }
    }

    // ========================================================================== //
    // Path update.
    // ========================================================================== //
    if (NO == cap_flag)
    {
		min_idx = g_path_max_metric_val[0] <= g_path_max_metric_val[1];
		min_metric_val_in_all_path = (min_idx == 0) ? g_path_max_metric_val[0] : g_path_max_metric_val[1];

        if (curr_metric_val > min_metric_val_in_all_path)
        {
            if (NO == path_reset[min_idx])
            {
                update_en  = YES;
                update_idx = min_idx;
            }
        }
    }
    else
    {
        update_en  = YES;
        update_idx = cap_idx;
    }

    if (YES == update_en)
    {
        update_path(
            curr_b0,
            curr_b7,
            curr_metric_val,
            &g_path_b0[update_idx],
            &g_path_b7[update_idx],
            &g_path_lookup_idx[update_idx],
            &g_path_sym_cnt[update_idx],
            &g_path_max_metric_val[update_idx],
            &g_path_local_max_metric_val[update_idx]
		);
    }
}


void acm_frame_capture_init()
{
    INT32 n;

    for (n = 0; n < NUM_PATH; n++)
    {
        g_path_b0[n] = 0;
        g_path_b7[n] = 0;
        g_path_lookup_idx[n] = 0;
        g_path_sym_cnt[n] = 0;
        g_path_max_metric_val[n] = 0;
        g_path_local_max_metric_val[n] = 0;
        g_path_cap_frame_size[n] = 0;
        g_path_cap_frame_pilot_flag[n] = 0;
    }

    g_last_cap_frame_size = 0;
}

void angle_by_cordic(
    INT32 data_i,     // Fixed( 1, 1, 6 );
    INT32 data_q,     // Fixed( 1, 1, 6 );
    INT32 *phase )    // Fixed( 0, 9, 0 );
{

    INT32 n;
    INT32 rotate_i;       // Fixed( 1, 7, ~ );
    INT32 rotate_q;       // Fixed( 1, 7, ~ );
    INT32 update_i;       // Fixed( 1, 7, ~ );
    INT32 update_q;       // Fixed( 1, 7, ~ );
    INT32 mid_i;          // Fixed( 1, 7, ~ );
    INT32 mid_q;          // Fixed( 1, 7, ~ );

    INT32 cordic_phase;   // Fixed( 1, 7, 2 );
    INT32 phase_out;      // Fixed( 0, 9, 0 );

    static const INT32 s_cordic_table[6] =
    { 45, 27, 14, 7, 4, 2 };  // Fixed( 0, 6, 0 );

	data_i = data_i << (6+8);
	data_q = data_q << (6+8);

    rotate_i = (data_i > 0) ? data_i : (-data_i);
    rotate_q = (data_q > 0) ? data_q : (-data_q);
    update_i = rotate_i;
    update_q = rotate_q;
    cordic_phase = 0;
    for (n = 0; n < 6; n++)
    {
        //mid_i = rotate_i / pow( 2.0, n );
        //mid_q = rotate_q / pow( 2.0, n );
        mid_i = rotate_i >> n;
        mid_q = rotate_q >> n;

        if (update_q > 0)
        {
            // Clockwise rotation
            update_i = rotate_i + mid_q;
            update_q = rotate_q - mid_i;

            // Angle update
            cordic_phase += s_cordic_table[n];
        }
        else
        {
            // Counterclockwise rotation
            update_i = rotate_i - mid_q;
            update_q = rotate_q + mid_i;

            // Angle update
            cordic_phase -= s_cordic_table[n];
        }

        rotate_i = update_i;
        rotate_q = update_q;
    }

    if (cordic_phase < 0)
    {
        cordic_phase = 0;
    }
    else if (cordic_phase > 90)
    {
        cordic_phase = 90;
    }

    if ((data_i > 0) && (data_q > 0))
    {
        phase_out = cordic_phase;
    }
    else if ((data_i < 0) && (data_q > 0))
    {
        phase_out = 180 - cordic_phase;
    }
    else if ((data_i < 0) && (data_q < 0))
    {
        phase_out = 180 + cordic_phase;
    }
    else if ((data_i > 0) && (data_q < 0))
    {
        phase_out = 360 - cordic_phase;
    }
    else if ((0 == data_i) && (data_q > 0))
    {
        phase_out = 90;
    }
    else if ((0 == data_i) && (data_q < 0))
    {
        phase_out = 270;
    }
    else if ((0 == data_q) && (data_i > 0))
    {
        phase_out = 0;
    }
    else if ((0 == data_q) && (data_i < 0))
    {
        phase_out = 180;
    }
    else if ((0 == data_q) && (0 == data_i))
    {
        phase_out = 0;
    }
    else
    {
    }

    *phase = phase_out;
}


void combine_sof_and_plsc(
    // Input
    INT32 sof_corr_i,    // Fixed( 1, 5, 4 );
    INT32 sof_corr_q,    // Fixed( 1, 5, 4 );
    INT32 plsc_corr_i,   // Fixed( 1, 5, 4 );
    INT32 plsc_corr_q,   // Fixed( 1, 5, 4 );
    // Output
    INT32 *sub_flag,     // Fixed( 0, 1, 0 );
    INT32 *metric_max )  // Fixed( 0, 7, 7 );
{
    INT32 modulus_add;  // Fixed( 0, 7, 7 );
    INT32 modulus_sub;  // Fixed( 0, 7, 7 );

    //modulus_add = rm_modulus( sof_corr_i + plsc_corr_i, sof_corr_q + plsc_corr_q );
    INT32 large    = 0;   // Fixed( 0, 6, 4 );
    INT32 small    = 0;   // Fixed( 0, 6, 4 );
    INT32 temp     = 0;   // Fixed( 0, 7, 7 );
    INT32 real     = sof_corr_i + plsc_corr_i;
    INT32 image    = sof_corr_q + plsc_corr_q;
    INT32 abs_real = (real  > 0) ? real  : (-real);
    INT32 abs_imag = (image > 0) ? image : (-image);	
    if (abs_real > abs_imag)
    {
        large = abs_real;
        small = abs_imag;
    }
    else
    {
        large = abs_imag;
        small = abs_real;
    }
	temp = (large << 3) - large + (small << 2);	//7*large+4*small
	large <<= 3;
    modulus_add = (temp > large) ? temp : large;

    //modulus_sub = rm_modulus( sof_corr_i - plsc_corr_i, sof_corr_q - plsc_corr_q );
    real     = sof_corr_i - plsc_corr_i;
    image    = sof_corr_q - plsc_corr_q;
    abs_real = (real  > 0) ? real  : (-real);
    abs_imag = (image > 0) ? image : (-image);	
    if (abs_real > abs_imag)
    {
        large = abs_real;
        small = abs_imag;
    }
    else
    {
        large = abs_imag;
        small = abs_real;
    }
	temp = (large << 3) - large + (small << 2);
	large <<= 3;
    modulus_sub = (temp > large) ? temp : large;

	*sub_flag   = (modulus_add <= modulus_sub);
	*metric_max = (*sub_flag) ? modulus_sub : modulus_add;
}


void get_cap_flag(
    INT32 curr_b7,                       // Fixed( 0, 1,  0 );
    const INT32 path_cap_flag[NUM_PATH],       // Fixed( 0, 1,  0 );
    const INT32 path_max_metric_val[NUM_PATH], // Fixed( 0, 1,  7 );
    INT32 *cap_flag,                     // Fixed( 0, 1,  0 );
    INT32 *cap_idx )                     // Fixed( 0, 1,  0 );  if NUM_PATH = 2.
{
    INT32  n;
    INT32  valid = NO;              // Fixed( 0, 1, 0 );
    INT32  max_idx = 0;             // Fixed( 0, 1, 0 );  if NUM_PATH = 2;
    INT32  max_val = 0;             // Fixed( 0, 1, 7 );
    INT32  max_metric_in_all_path;  // Fixed( 0, 1, 7 );
    INT32  path_ref_val;            // Fixed( 0, 1, 7 );

    for (n = 0; n < NUM_PATH; n++)
    {
        if (YES == path_cap_flag[n])
        {
            if (path_max_metric_val[n] > max_val)
            {
                max_val = path_max_metric_val[n];
                max_idx = n;
            }
            valid = YES;
        }
    }

    if (YES == valid)
    {
		max_metric_in_all_path = path_max_metric_val[0] >= path_max_metric_val[1] ? 
			                     path_max_metric_val[0] : path_max_metric_val[1];

		path_ref_val = g_path_weight * max_metric_in_all_path;  //(0,1,4)*(0,1,7) -> (0,2,11)
		path_ref_val >>= 4;

        if (path_max_metric_val[max_idx] >= path_ref_val)
        {
            *cap_flag = YES;
            *cap_idx  = max_idx;
        }
    }
    else
    {
        *cap_flag = NO;
        *cap_idx  = 0;
    }
}


INT32 g_cos_table[91] =   // Fixed( 0, 0, 4 );
{
    15,//(INT32)(16 * 0.9375),  //  0 degree;
    15,//(INT32)(16 * 0.9375),  //  1 degree;
    15,//(INT32)(16 * 0.9375),  //  2 degree;
    15,//(INT32)(16 * 0.9375),  //  3 degree;
    15,//(INT32)(16 * 0.9375),  //  4 degree;
    15,//(INT32)(16 * 0.9375),  //  5 degree;
    15,//(INT32)(16 * 0.9375),  //  6 degree;
    15,//(INT32)(16 * 0.9375),  //  7 degree;
    15,//(INT32)(16 * 0.9375),  //  8 degree;
    15,//(INT32)(16 * 0.9375),  //  9 degree;
    15,//(INT32)(16 * 0.9375),  // 10 degree;
    15,//(INT32)(16 * 0.9375),  // 11 degree;
    15,//(INT32)(16 * 0.9375),  // 12 degree;
    15,//(INT32)(16 * 0.9375),  // 13 degree;
    15,//(INT32)(16 * 0.9375),  // 14 degree;
    14,//(INT32)(16 * 0.8750),  // 15 degree;
    14,//(INT32)(16 * 0.8750),  // 16 degree;
    14,//(INT32)(16 * 0.8750),  // 17 degree;
    14,//(INT32)(16 * 0.8750),  // 18 degree;
    14,//(INT32)(16 * 0.8750),  // 19 degree;
    14,//(INT32)(16 * 0.8750),  // 20 degree;
    14,//(INT32)(16 * 0.8750),  // 21 degree;
    14,//(INT32)(16 * 0.8750),  // 22 degree;
    14,//(INT32)(16 * 0.8750),  // 23 degree;
    14,//(INT32)(16 * 0.8750),  // 24 degree;
    14,//(INT32)(16 * 0.8750),  // 25 degree;
    13,//(INT32)(16 * 0.8125),  // 26 degree;
    13,//(INT32)(16 * 0.8125),  // 27 degree;
    13,//(INT32)(16 * 0.8125),  // 28 degree;
    13,//(INT32)(16 * 0.8125),  // 29 degree;
    13,//(INT32)(16 * 0.8125),  // 30 degree;
    13,//(INT32)(16 * 0.8125),  // 31 degree;
    13,//(INT32)(16 * 0.8125),  // 32 degree;
    13,//(INT32)(16 * 0.8125),  // 33 degree;
    12,//(INT32)(16 * 0.7500),  // 34 degree;
    12,//(INT32)(16 * 0.7500),  // 35 degree;
    12,//(INT32)(16 * 0.7500),  // 36 degree;
    12,//(INT32)(16 * 0.7500),  // 37 degree;
    12,//(INT32)(16 * 0.7500),  // 38 degree;
    12,//(INT32)(16 * 0.7500),  // 39 degree;
    11,//(INT32)(16 * 0.6875),  // 40 degree;
    11,//(INT32)(16 * 0.6875),  // 41 degree;
    11,//(INT32)(16 * 0.6875),  // 42 degree;
    11,//(INT32)(16 * 0.6875),  // 43 degree;
    11,//(INT32)(16 * 0.6875),  // 44 degree;
    11,//(INT32)(16 * 0.6875),  // 45 degree;
    10,//(INT32)(16 * 0.6250),  // 46 degree;
    10,//(INT32)(16 * 0.6250),  // 47 degree;
    10,//(INT32)(16 * 0.6250),  // 48 degree;
    10,//(INT32)(16 * 0.6250),  // 49 degree;
    10,//(INT32)(16 * 0.6250),  // 50 degree;
    9,//(INT32)(16 * 0.5625),  // 51 degree;
    9,//(INT32)(16 * 0.5625),  // 52 degree;
    9,//(INT32)(16 * 0.5625),  // 53 degree;
    9,//(INT32)(16 * 0.5625),  // 54 degree;
    9,//(INT32)(16 * 0.5625),  // 55 degree;
    8,//(INT32)(16 * 0.5000),  // 56 degree;
    8,//(INT32)(16 * 0.5000),  // 57 degree;
    8,//(INT32)(16 * 0.5000),  // 58 degree;
    8,//(INT32)(16 * 0.5000),  // 59 degree;
    7,//(INT32)(16 * 0.4375),  // 60 degree;
    7,//(INT32)(16 * 0.4375),  // 61 degree;
    7,//(INT32)(16 * 0.4375),  // 62 degree;
    7,//(INT32)(16 * 0.4375),  // 63 degree;
    7,//(INT32)(16 * 0.4375),  // 64 degree;
    6,//(INT32)(16 * 0.3750),  // 65 degree;
    6,//(INT32)(16 * 0.3750),  // 66 degree;
    6,//(INT32)(16 * 0.3750),  // 67 degree;
    6,//(INT32)(16 * 0.3750),  // 68 degree;
    5,//(INT32)(16 * 0.3125),  // 69 degree;
    5,//(INT32)(16 * 0.3125),  // 70 degree;
    5,//(INT32)(16 * 0.3125),  // 71 degree;
    5,//(INT32)(16 * 0.3125),  // 72 degree;
    4,//(INT32)(16 * 0.2500),  // 73 degree;
    4,//(INT32)(16 * 0.2500),  // 74 degree;
    4,//(INT32)(16 * 0.2500),  // 75 degree;
    4,//(INT32)(16 * 0.2500),  // 76 degree;
    3,//(INT32)(16 * 0.1875),  // 77 degree;
    3,//(INT32)(16 * 0.1875),  // 78 degree;
    3,//(INT32)(16 * 0.1875),  // 79 degree;
    3,//(INT32)(16 * 0.1875),  // 80 degree;
    2,//(INT32)(16 * 0.1250),  // 81 degree;
    2,//(INT32)(16 * 0.1250),  // 82 degree;
    2,//(INT32)(16 * 0.1250),  // 83 degree;
    2,//(INT32)(16 * 0.1250),  // 84 degree;
    1,//(INT32)(16 * 0.0625),  // 85 degree;
    1,//(INT32)(16 * 0.0625),  // 86 degree;
    1,//(INT32)(16 * 0.0625),  // 87 degree;
    1,//(INT32)(16 * 0.0625),  // 88 degree;
    0,//(INT32)(16 * 0.0000),  // 89 degree;
    0 //(INT32)(16 * 0.0000)   // 90 degree;
};


INT32 get_cos_val( INT32 angle )  // Fixed( 1, 9, 0 );
{
    INT32 positive;      // Fixed( 0, 1, 0 );
    INT32 angle_abs;     // Fixed( 0, 9, 0 );
    INT32 mapped_angle;  // Fixed( 0, 7, 0 );

	if (angle >= 0)
	{
		if (angle >= 360)
			angle_abs = angle - 360;
		else
			angle_abs = angle;
	}
	else
	{
		if (angle < -360)
		   angle_abs = angle + 720;
		else
		   angle_abs = angle + 360;
	}

    if (angle_abs < 180)
    {
		if (angle_abs < 90)
		{
			mapped_angle = angle_abs;
			positive = 1;
		}
		else
		{
			mapped_angle = 180 - angle_abs;
			positive = 0;		
		}

    }
    else
    {
		if (angle_abs < 270)
		{
			mapped_angle = angle_abs - 180;
			positive = 0;
		}
        else
		{
			mapped_angle = 360 - angle_abs;
			positive = 1;		
		}
    }

	if (positive)
		return  g_cos_table[mapped_angle];
	else
		return -g_cos_table[mapped_angle];
}

INT32 g_sin_table[91] =    // Fixed( 0, 0, 4 );
{
    0,//(INT32)(16 * 0     ),    //  0 degree;
    0,//(INT32)(16 * 0     ),    //  1 degree;
    1,//(INT32)(16 * 0.0625),    //  2 degree;
    1,//(INT32)(16 * 0.0625),    //  3 degree;
    1,//(INT32)(16 * 0.0625),    //  4 degree;
    1,//(INT32)(16 * 0.0625),    //  5 degree;
    2,//(INT32)(16 * 0.1250),    //  6 degree;
    2,//(INT32)(16 * 0.1250),    //  7 degree;
    2,//(INT32)(16 * 0.1250),    //  8 degree;
    2,//(INT32)(16 * 0.1250),    //  9 degree;
    3,//(INT32)(16 * 0.1875),    // 10 degree;
    3,//(INT32)(16 * 0.1875),    // 11 degree;
    3,//(INT32)(16 * 0.1875),    // 12 degree;
    3,//(INT32)(16 * 0.1875),    // 13 degree;
    4,//(INT32)(16 * 0.2500),    // 14 degree;
    4,//(INT32)(16 * 0.2500),    // 15 degree;
    4,//(INT32)(16 * 0.2500),    // 16 degree;
    4,//(INT32)(16 * 0.2500),    // 17 degree;
    5,//(INT32)(16 * 0.3125),    // 18 degree;
    5,//(INT32)(16 * 0.3125),    // 19 degree;
    5,//(INT32)(16 * 0.3125),    // 20 degree;
    5,//(INT32)(16 * 0.3125),    // 21 degree;
    6,//(INT32)(16 * 0.3750),    // 22 degree;
    6,//(INT32)(16 * 0.3750),    // 23 degree;
    6,//(INT32)(16 * 0.3750),    // 24 degree;
    6,//(INT32)(16 * 0.3750),    // 25 degree;
    7,//(INT32)(16 * 0.4375),    // 26 degree;
    7,//(INT32)(16 * 0.4375),    // 27 degree;
    7,//(INT32)(16 * 0.4375),    // 28 degree;
    7,//(INT32)(16 * 0.4375),    // 29 degree;
    7,//(INT32)(16 * 0.4375),    // 30 degree;
    8,//(INT32)(16 * 0.5000),    // 31 degree;
    8,//(INT32)(16 * 0.5000),    // 32 degree;
    8,//(INT32)(16 * 0.5000),    // 33 degree;
    8,//(INT32)(16 * 0.5000),    // 34 degree;
    9,//(INT32)(16 * 0.5625),    // 35 degree;
    9,//(INT32)(16 * 0.5625),    // 36 degree;
    9,//(INT32)(16 * 0.5625),    // 37 degree;
    9,//(INT32)(16 * 0.5625),    // 38 degree;
    9,//(INT32)(16 * 0.5625),    // 39 degree;
    10,//(INT32)(16 * 0.6250),    // 40 degree;
    10,//(INT32)(16 * 0.6250),    // 41 degree;
    10,//(INT32)(16 * 0.6250),    // 42 degree;
    10,//(INT32)(16 * 0.6250),    // 43 degree;
    10,//(INT32)(16 * 0.6250),    // 44 degree;
    11,//(INT32)(16 * 0.6875),    // 45 degree;
    11,//(INT32)(16 * 0.6875),    // 46 degree;
    11,//(INT32)(16 * 0.6875),    // 47 degree;
    11,//(INT32)(16 * 0.6875),    // 48 degree;
    11,//(INT32)(16 * 0.6875),    // 49 degree;
    11,//(INT32)(16 * 0.6875),    // 50 degree;
    12,//(INT32)(16 * 0.7500),    // 51 degree;
    12,//(INT32)(16 * 0.7500),    // 52 degree;
    12,//(INT32)(16 * 0.7500),    // 53 degree;
    12,//(INT32)(16 * 0.7500),    // 54 degree;
    12,//(INT32)(16 * 0.7500),    // 55 degree;
    12,//(INT32)(16 * 0.7500),    // 56 degree;
    13,//(INT32)(16 * 0.8125),    // 57 degree;
    13,//(INT32)(16 * 0.8125),    // 58 degree;
    13,//(INT32)(16 * 0.8125),    // 59 degree;
    13,//(INT32)(16 * 0.8125),    // 60 degree;
    13,//(INT32)(16 * 0.8125),    // 61 degree;
    13,//(INT32)(16 * 0.8125),    // 62 degree;
    13,//(INT32)(16 * 0.8125),    // 63 degree;
    13,//(INT32)(16 * 0.8125),    // 64 degree;
    14,//(INT32)(16 * 0.8750),    // 65 degree;
    14,//(INT32)(16 * 0.8750),    // 66 degree;
    14,//(INT32)(16 * 0.8750),    // 67 degree;
    14,//(INT32)(16 * 0.8750),    // 68 degree;
    14,//(INT32)(16 * 0.8750),    // 69 degree;
    14,//(INT32)(16 * 0.8750),    // 70 degree;
    14,//(INT32)(16 * 0.8750),    // 71 degree;
    14,//(INT32)(16 * 0.8750),    // 72 degree;
    14,//(INT32)(16 * 0.8750),    // 73 degree;
    14,//(INT32)(16 * 0.8750),    // 74 degree;
    14,//(INT32)(16 * 0.8750),    // 75 degree;
    15,//(INT32)(16 * 0.9375),    // 76 degree;
    15,//(INT32)(16 * 0.9375),    // 77 degree;
    15,//(INT32)(16 * 0.9375),    // 78 degree;
    15,//(INT32)(16 * 0.9375),    // 79 degree;
    15,//(INT32)(16 * 0.9375),    // 80 degree;
    15,//(INT32)(16 * 0.9375),    // 81 degree;
    15,//(INT32)(16 * 0.9375),    // 82 degree;
    15,//(INT32)(16 * 0.9375),    // 83 degree;
    15,//(INT32)(16 * 0.9375),    // 84 degree;
    15,//(INT32)(16 * 0.9375),    // 85 degree;
    15,//(INT32)(16 * 0.9375),    // 86 degree;
    15,//(INT32)(16 * 0.9375),    // 87 degree;
    15,//(INT32)(16 * 0.9375),    // 88 degree;
    15,//(INT32)(16 * 0.9375),    // 89 degree;
    15 //(INT32)(16 * 0.9375)    // 90 degree;
};

INT32 get_sin_val( INT32 angle )  // Fixed( 1, 10, 0 );
{
    INT32 positive;      // Fixed( 0, 1, 0 );
    INT32 angle_abs;     // Fixed( 0, 9, 0 );
    INT32 mapped_angle;  // Fixed( 0, 7, 0 );

	if (angle >= 0)
	{
		if (angle >= 360)
			angle_abs = angle - 360;
		else
			angle_abs = angle;
	}
	else
	{
		if (angle < -360)
		   angle_abs = angle + 720;
		else
		   angle_abs = angle + 360;
	}

    if (angle_abs < 180)
    {
		if (angle_abs < 90)
		{
			mapped_angle = angle_abs;
			positive = 1;
		}
		else
		{
			mapped_angle = 180 - angle_abs;
			positive = 1;
		}
    }
    else
    {
		if (angle_abs < 270)
		{
			mapped_angle = angle_abs - 180;
			positive = 0;
		}
		else
		{
			mapped_angle = 360 - angle_abs;
			positive = 0;
		}
    }

	if (positive)
		return  g_sin_table[mapped_angle];
	else
		return -g_sin_table[mapped_angle];
}


INT32 g_phase_fifo[PHASE_FIFO_SIZE] = {0};                  // Fixed( 0, 9, 0 );
INT32 g_lag1_diff_corr_fifo_i[CONV_HEADER_SIZE - 1] = {0};  // Fixed( 1, 0, 4 );
INT32 g_lag1_diff_corr_fifo_q[CONV_HEADER_SIZE - 1] = {0};  // Fixed( 1, 0, 4 );
INT32 g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 2]   = {0};  // Fixed( 1, 0, 4 );
INT32 g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 2]   = {0};  // Fixed( 1, 0, 4 );

void metric(
    const INT32  curr_header_fifo_i[CONV_HEADER_SIZE], // Fixed( 1, 1, 6 );
    const INT32  curr_header_fifo_q[CONV_HEADER_SIZE], // Fixed( 1, 1, 6 );
    INT32        *curr_b0,                             // Fixed( 0, 2, 0 );
    INT32        *curr_b7,                             // Fixed( 0, 2, 0 );
    INT32        *curr_metric_val )                    // Fixed( 0, 1, 7 );
{

    //INT32 b0;                   // Fixed( 0, 1, 0 );
    INT32 b7;                   // Fixed( 0, 1, 0 );

    INT32 sof_lag1_i;           // Fixed( 1, 5, 4 );
    INT32 sof_lag1_q;           // Fixed( 1, 5, 4 );
    INT32 sof_lag2_i;           // Fixed( 1, 5, 4 );
    INT32 sof_lag2_q;           // Fixed( 1, 5, 4 );

    INT32 rm_lag1_i;            // Fixed( 1, 5, 4 );
    INT32 rm_lag1_q;            // Fixed( 1, 5, 4 );
    INT32 rm_msb0_lag2_i;       // Fixed( 1, 5, 4 );
    INT32 rm_msb0_lag2_q;       // Fixed( 1, 5, 4 );

    INT32 rm_lag1_metric;       // Fixed( 0, 7, 7 );
    INT32 rm_msb0_lag2_metric;  // Fixed( 0, 7, 7 );

    INT32 sub_flag;             // Fixed( 0, 1, 0 );
    //INT32 rm_scale;             // Fixed( 0, 0, 11 );
    //INT32 rm_metric;            // Fixed( 0, 1,  7 );
    INT32 metric_val;           // Fixed( 0, 1,  7 );

    // ========================================================================================= //
    // Update lag correlation FIFO buffers;
    // ========================================================================================= //
    update_diff_corr_fifo(curr_header_fifo_i, curr_header_fifo_q);

    acc_sof_lag1_corr( &sof_lag1_i, &sof_lag1_q );
	acc_rm_plsc_lag1_corr( &rm_lag1_i, &rm_lag1_q );
    combine_sof_and_plsc(
        sof_lag1_i,
        sof_lag1_q,
        rm_lag1_i,
        rm_lag1_q,
        &b7,
        &rm_lag1_metric );

	acc_sof_lag2_corr( &sof_lag2_i, &sof_lag2_q );
	acc_rm_plsc_lag2_corr( 0, &rm_msb0_lag2_i, &rm_msb0_lag2_q );
    combine_sof_and_plsc(
        sof_lag2_i,
        sof_lag2_q,
        rm_msb0_lag2_i,
        rm_msb0_lag2_q,
        &sub_flag,
        &rm_msb0_lag2_metric );

	metric_val = rm_lag1_metric + rm_msb0_lag2_metric;

    *curr_b0 = 0;	//DVB-S2, no DVB-S2X
    *curr_b7 = b7;
    *curr_metric_val = (metric_val << 4) + (metric_val << 1);//(metric_val * 18);
}


void metric_init()
{
    INT32 n;

    for (n = 0; n < PHASE_FIFO_SIZE; n++)
    {
        g_phase_fifo[n] = 0;
    }

    for (n = 0; n < (CONV_HEADER_SIZE - 1); n++)
    {
        g_lag1_diff_corr_fifo_i[n] = 0;
        g_lag1_diff_corr_fifo_q[n] = 0;
    }

    for (n = 0; n < (RM_HEADER_SIZE - 2); n++)
    {
        g_lag2_diff_corr_fifo_i[n] = 0;
        g_lag2_diff_corr_fifo_q[n] = 0;
    }
}

// Correlation on Reed-Muller code PLSCode
void acc_rm_plsc_lag1_corr(
    INT32 *out_i,   // Fixed( 1, 5, 4 );
    INT32 *out_q )  // Fixed( 1, 5, 4 );
{
    static const INT32 s_rm_plsc_lag1_coef[RM_PLSC_SIZE / 2] =
    { 0,1,1,0, 0,0,1,0, 0,1,1,1, 1,1,0,0,
      0,0,1,1, 0,1,1,0, 1,0,1,0, 1,1,0,0 };   // 0 --> j, 1 --> -j;  Fixed( 0, 1, 0 );

    INT32 n;
    INT32 real;    // Fixed( 1, 0, 4 );
    INT32 image;   // Fixed( 1, 0, 4 );
    INT32 temp_i = 0;  // Fixed( 1, 5, 4 );
    INT32 temp_q = 0;  // Fixed( 1, 5, 4 );

    for (n = 0; n < (RM_PLSC_SIZE / 2); n++)
    {
        real  = g_lag1_diff_corr_fifo_i[SOF_SIZE + 2 * n];
        image = g_lag1_diff_corr_fifo_q[SOF_SIZE + 2 * n];
        if (0 == s_rm_plsc_lag1_coef[n])
        {
            temp_i -= image;
            temp_q += real;
        }
        else
        {
            temp_i += image;
            temp_q -= real;
        }
    }

    *out_i = temp_i;
    *out_q = temp_q;
}


void acc_rm_plsc_lag2_corr(
    // Input
    INT32    b0,       // Fixed( 0, 1, 0 );
    // Output
    INT32 *out_i,   // Fixed( 1, 5, 4 );
    INT32 *out_q )  // Fixed( 1, 5, 4 );
{
    // Reed-Mullder PLSCode 2 lag correlation coefficient, when b0 = 0.
    static const INT32 s_rm_plsc_lag2_coef_0[RM_PLSC_SIZE / 2] =
    { 1,0, 0,1, 1,1, 1,0, 1,0, 1,1, 1,1, 1,1,
      0,0, 1,1, 0,1, 1,0, 1,0, 1,0, 0,0, 0,0 };   // 0 --> 1, 1 --> -1;   // Fixed( 0, 1, 0 );

    // Reed-Mullder PLSCode 2 lag correlation coefficient, when b0 = 1.
    static const INT32 s_rm_plsc_lag2_coef_1[RM_PLSC_SIZE / 2] =
    { 0,1, 1,0, 1,1, 1,0, 0,1, 0,0, 1,1, 1,1,
      0,0, 0,0, 0,1, 0,1, 1,0, 0,1, 0,0, 1,1 };   // 0 --> 1, 1 --> -1;   // Fixed( 0, 1, 0 );

    INT32 n;
    INT32 coef_0;      // Fixed( 0, 1, 0 );
    INT32 coef_1;      // Fixed( 0, 1, 0 );
    INT32 real_0;   // Fixed( 1, 0, 4 );
    INT32 real_1;   // Fixed( 1, 0, 4 );
    INT32 image_0;  // Fixed( 1, 0, 4 );
    INT32 image_1;  // Fixed( 1, 0, 4 );
    INT32 temp_i = 0;   // Fixed( 1, 5, 4 );  5 + ceil(log2(32)) = 10 bits;
    INT32 temp_q = 0;   // Fixed( 1, 5, 4 );  5 + ceil(log2(32)) = 10 bits;

    for (n = 0; n < (RM_PLSC_SIZE / 4); n++)
    {
        if (0 == b0)
        {
            coef_0 = s_rm_plsc_lag2_coef_0[2 * n + 0];
            coef_1 = s_rm_plsc_lag2_coef_0[2 * n + 1];
        }
        else
        {
            coef_0 = s_rm_plsc_lag2_coef_1[2 * n + 0];
            coef_1 = s_rm_plsc_lag2_coef_1[2 * n + 1];
        }

        real_0  = g_lag2_diff_corr_fifo_i[SOF_SIZE + 4 * n + 0];
        image_0 = g_lag2_diff_corr_fifo_q[SOF_SIZE + 4 * n + 0];
        if (0 == coef_0)
        {
            temp_i += real_0;
            temp_q += image_0;
        }
        else
        {
            temp_i -= real_0;
            temp_q -= image_0;
        }

        real_1  = g_lag2_diff_corr_fifo_i[SOF_SIZE + 4 * n + 1];
        image_1 = g_lag2_diff_corr_fifo_q[SOF_SIZE + 4 * n + 1];
        if (0 == coef_1)
        {
            temp_i += real_1;
            temp_q += image_1;
        }
        else
        {
            temp_i -= real_1;
            temp_q -= image_1;
        }
    }

    *out_i = temp_i;
    *out_q = temp_q;
}

// Correlation on SOF (Start Of Frame)
void acc_sof_lag1_corr(
    INT32 *out_i,   // Fixed( 1, 5, 4 );
    INT32 *out_q )  // Fixed( 1, 5, 4 );
{
    static const INT32 s_sof_lag1_coef[SOF_SIZE - 1] =            // Fixed( 0, 1, 0 );
    { 0,0,0,0,1, 1,1,1,0,1, 1,1,0,1,1, 0,0,1,0,0, 1,0,1,1,0 };  // 0 -----> j,  1 -----> -j;

    INT32 n;
    INT32 real;    // Fixed( 1, 0, 4 );
    INT32 image;   // Fixed( 1, 0, 4 );
    INT32 temp_i = 0;  // Fixed( 1, 5, 4 );
    INT32 temp_q = 0;  // Fixed( 1, 5, 4 );

    for (n = 0; n < (SOF_SIZE - 1); n ++)
    {
        real  = g_lag1_diff_corr_fifo_i[n];
        image = g_lag1_diff_corr_fifo_q[n];
        if (0 == s_sof_lag1_coef[n])
        {
            temp_i -= image;
            temp_q += real;
        }
        else
        {
            temp_i += image;
            temp_q -= real;
        }
    }

    *out_i = temp_i;
    *out_q = temp_q;
}


void acc_sof_lag2_corr(
    INT32 *out_i,   // Fixed( 1, 5, 4 );
    INT32 *out_q )  // Fixed( 1, 5, 4 );
{
    static const INT32 s_sof_lag2_coef[SOF_SIZE - 2] =            // Fixed( 0, 1, 0 );
    { 1,1,1,0, 1,1,1,0, 0,1,1,0, 0,1,0,1, 0,0,1,0, 0,0,1,0 };   // 0 -----> 1,  1 -----> -1;

    INT32 n;
    INT32 real;    // Fixed( 1, 0, 4 );
    INT32 image;   // Fixed( 1, 0, 4 );
    INT32 temp_i = 0;  // Fixed( 1, 5, 4 );
    INT32 temp_q = 0;  // Fixed( 1, 5, 4 );

    for (n = 0; n < (SOF_SIZE - 2); n ++)
    {
        real  = g_lag2_diff_corr_fifo_i[n];
        image = g_lag2_diff_corr_fifo_q[n];
        if (0 == s_sof_lag2_coef[n])
        {
            temp_i += real;
            temp_q += image;
        }
        else
        {
            temp_i -= real;
            temp_q -= image;
        }
    }

    *out_i = temp_i;
    *out_q = temp_q;
}


void reset_path(
    INT32    *path_b0,                    // Fixed( 0, 1,  0 );
    INT32    *path_b7,                    // Fixed( 0, 1,  0 );
    INT32    *path_lookup_idx,            // Fixed( 0, 5,  0 );
    INT32    *path_sym_cnt,               // Fixed( 0, 16, 0 );
    INT32    *path_max_metric_val,        // Fixed( 0, 1,  7 );
    INT32    *path_local_max_metric_val   // Fixed( 0, 1,  7 );
)
{
    *path_b0 = 0;
    *path_b7 = 0;
    *path_lookup_idx = 0;
    *path_sym_cnt = 0;
    *path_max_metric_val = 0;
    *path_local_max_metric_val = 0;
}


void search_each_path(
    INT32       curr_metric_val,                   // Fixed( 0, 1,  7 );
    INT32       last_cap_frame_size,               // Fixed( 0, 16, 0 );
    INT32       path_b0,                           // Fixed( 0, 1,  0 );
    INT32       path_b7,                           // Fixed( 0, 1,  0 );
    INT32       path_max_metric_val,               // Fixed( 0, 1,  7 );
    INT32       *path_reset,                        // Fixed( 0, 1,  0 );
    INT32       *path_cap_flag,                     // Fixed( 0, 1,  0 );
    INT32       *path_lookup_idx,                   // Fixed( 0, 5,  0 );
    INT32       *path_sym_cnt,                      // Fixed( 0, 16, 0 );
    INT32       *path_local_max_metric_val,         // Fixed( 0, 1,  7 );
    INT32       *path_cap_frame_size,               // Fixed( 0, 16, 0 );
    INT32       *path_cap_frame_pilot_flag )        // Fixed( 0, 1,  0 );
{

    INT32 idx;           // Fixed( 0, 5,  0 );
    INT32 sym_cnt;       // Fixed( 0, 16, 0 );
    INT32 range;         // Fixed( 0, 16, 0 );
    INT32 start_idx;     // Fixed( 0, 16, 0 );
    INT32 target_idx;    // Fixed( 0, 16, 0 );
    INT32 local_ref_val; // Fixed( 0, 1,  7 );

    *path_sym_cnt = (*path_sym_cnt) + 1;
    idx     = (*path_lookup_idx);
    sym_cnt = (*path_sym_cnt);

    if (NO == g_valid_frame_size[idx])
    {
		*path_reset = ((*path_lookup_idx) + 2) > (NUM_FRAME - 1);
		*path_lookup_idx = (*path_reset) ? *path_lookup_idx : *path_lookup_idx + 2;
    }
    else
    {
        target_idx = g_frame_size_set[idx];
        range = (g_search_range[idx] << 6);

        if (target_idx < range)
        {
            start_idx = 1;
        }
        else
        {
            start_idx = target_idx - range;
        }

        if (sym_cnt >= start_idx)
        {
            if (sym_cnt < target_idx)
            {
                if (curr_metric_val > (*path_local_max_metric_val))
                {
                    *path_local_max_metric_val = curr_metric_val;
                }
            }
            else if (sym_cnt == target_idx)
            {
				local_ref_val = *path_local_max_metric_val * g_local_weight;
				curr_metric_val <<= 4;

                if ((curr_metric_val >= local_ref_val)
                    && (curr_metric_val >= g_metric_threshold))
                {
                    *path_cap_flag = YES;
                    *path_cap_frame_size = target_idx;
                    *path_cap_frame_pilot_flag = idx & 0x1;
                }

				*path_reset = ((*path_lookup_idx) + 2) > (NUM_FRAME - 1);
				*path_lookup_idx = (*path_reset) ? *path_lookup_idx : *path_lookup_idx + 2;
            }
            else
            {
                *path_reset = YES;
            }  // End of if (sym_cnt[n] < target_idx)
        }
        else
        {
            ; // Do nothing.
        }
    }
}


void update_diff_corr_fifo(
    const INT32 curr_header_fifo_i[CONV_HEADER_SIZE],  // Fixed( 1, 1, 6 );
    const INT32 curr_header_fifo_q[CONV_HEADER_SIZE]   // Fixed( 1, 1, 6 );
){

    INT32 n, m;
    INT32 index;            // Fixed( 0, 7, 0 );
    INT32 lag1_pha_offset;  // Fixed( 1, 9, 0 );
    INT32 lag2_pha_offset;  // Fixed( 1, 9, 0 );
    //INT32 lag4_pha_offset;  // Fixed( 1, 9, 0 );

    for (n = 0; n < (PHASE_FIFO_SIZE - 1); n++)
    {
        g_phase_fifo[n] = g_phase_fifo[n + 1];
    }

    angle_by_cordic(
		curr_header_fifo_i[CONV_HEADER_SIZE - 1],
		curr_header_fifo_q[CONV_HEADER_SIZE - 1],
		&g_phase_fifo[PHASE_FIFO_SIZE - 1]
	);

    for (n = 0; n < (RM_HEADER_SIZE - 5); n++)
    {
		m = n + 1;
        g_lag1_diff_corr_fifo_i[n] = g_lag1_diff_corr_fifo_i[m];
        g_lag1_diff_corr_fifo_q[n] = g_lag1_diff_corr_fifo_q[m];
        g_lag2_diff_corr_fifo_i[n] = g_lag2_diff_corr_fifo_i[m];
        g_lag2_diff_corr_fifo_q[n] = g_lag2_diff_corr_fifo_q[m];
    }

	g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 5] = g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 5 + 1];
	g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 5] = g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 5 + 1];
	g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 4] = g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 4 + 1];
	g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 4] = g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 4 + 1];
	g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 3] = g_lag1_diff_corr_fifo_i[RM_HEADER_SIZE - 3 + 1];
	g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 3] = g_lag1_diff_corr_fifo_q[RM_HEADER_SIZE - 3 + 1];

    g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 5] = g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 5 + 1];
    g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 5] = g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 5 + 1];
    g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 4] = g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 4 + 1];
    g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 4] = g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 4 + 1];


    // Update 1 lag differential correlation FIFO buffer;
    lag1_pha_offset = g_phase_fifo[PHASE_FIFO_SIZE - 1] - g_phase_fifo[PHASE_FIFO_SIZE - 2];
    g_lag1_diff_corr_fifo_i[CONV_HEADER_SIZE - 2] = get_cos_val( lag1_pha_offset );
    g_lag1_diff_corr_fifo_q[CONV_HEADER_SIZE - 2] = get_sin_val( lag1_pha_offset );

    // Update 2 lags differential correlation FIFO buffer;
    index = PHASE_FIFO_SIZE - (CONV_HEADER_SIZE - RM_HEADER_SIZE) - 1;//(CONV_HEADER_SIZE >> 1) - 1;

    lag2_pha_offset = g_phase_fifo[index] - g_phase_fifo[index - 2];
    g_lag2_diff_corr_fifo_i[RM_HEADER_SIZE - 3] = get_cos_val( lag2_pha_offset );
    g_lag2_diff_corr_fifo_q[RM_HEADER_SIZE - 3] = get_sin_val( lag2_pha_offset );
}

void update_path(
    INT32       curr_b0,                              // Fixed( 0, 1,  0 );
    INT32       curr_b7,                              // Fixed( 0, 1,  0 );
    INT32       curr_metric_val,                      // Fixed( 0, 1,  7 );
    INT32       *path_b0,                             // Fixed( 0, 1,  0 );
    INT32       *path_b7,                             // Fixed( 0, 1,  0 );
    INT32       *path_lookup_idx,                     // Fixed( 0, 5,  0 );
    INT32       *path_sym_cnt,                        // Fixed( 0, 16, 0 );
    INT32       *path_max_metric_val,                 // Fixed( 0, 1,  7 );
    INT32       *path_local_max_metric_val            // Fixed( 0, 1,  7 );
){
    *path_b0 = curr_b0;
    *path_b7 = curr_b7;
    *path_lookup_idx = (1 == curr_b7) ? 1 : 0;
    *path_sym_cnt = 0;
    *path_max_metric_val = curr_metric_val;
    *path_local_max_metric_val = 0;
}



static const INT32 g_sof_bits[SOF_SIZE] =                                          // Fixed( 0, 1, 0 );
    { 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0 };

static const INT32 g_rm_plscode_scrambler[RM_PLSC_SIZE] =                          // Fixed( 0, 1, 0 );
    { 0, 1, 1, 1,  0, 0, 0, 1,  1, 0, 0, 1,  1, 1, 0, 1,
      1, 0, 0, 0,  0, 0, 1, 1,  1, 1, 0, 0,  1, 0, 0, 1,
      0, 1, 0, 1,  0, 0, 1, 1,  0, 1, 0, 0,  0, 0, 1, 0,
      0, 0, 1, 0,  1, 1, 0, 1,  1, 1, 1, 1,  1, 0, 1, 0 };

static const INT32 g_first_row_of_generator_matrix[RM_PLSC_SIZE / 2] =             // Fixed( 0, 1, 0 );
    { 1,0,0,1, 0,0,0,0, 1,0,1,0, 1,1,0,0, 0,0,1,0, 1,1,0,1, 1,1,0,1, 1,1,0,1 };

static const INT32 g_conv_plscode_scrambler[CONV_PLSC_SIZE] =                      // Fixed( 0, 1, 0 );
    { 1, 0, 1, 1,  1, 1, 0, 0,  0, 0, 0, 1,  0, 0, 0, 1,  1, 0, 0, 0,  0, 0,
      0, 0, 1, 0,  1, 0, 0, 1,  0, 1, 0, 1,  0, 1, 0, 0,  0, 1, 0, 1,  1, 1,
      0, 1, 1, 1,  0, 1, 0, 1,  1, 1, 0, 0,  1, 0, 0, 1,  1, 1, 0, 1,  1, 0,
      0, 0, 0, 1,  0, 1, 1, 0,  0, 0, 1, 1,  1, 1, 1, 1,  0, 1, 1, 0,  1, 0,
      1, 1, 0, 0,  1, 1, 0, 1,  1, 0, 1, 1,  1, 0, 0, 0,  0, 1, 1, 1,  0, 0,
      0, 1, 1, 0,  1, 0, 1, 0,  0, 1, 1, 1,  1, 1, 0, 0,  0, 1, 0, 0,  0, 0,
      1, 1, 0, 0,  1, 0, 1, 0,  0, 0, 0, 0,  0, 1, 1, 1,  1, 0, 1, 1,  1, 1 };

static const INT32 g_vlsnr_matrix[VLSNR_HEADER_SIZE - 4] =                         // Fixed( 0, 1, 0 );
    { 1,1,1,1, 1,0,1,1, 1,1,1,1, 0,0,1,0, 0,0,1,1, 1,1,1,0, 1,0,0,0, 0,0,1,1, 0,1,1,1, 1,1,1,1, 1,0,0,1, 1,0,1,1, 1,1,0,0, 0,1,0,0,
      1,0,0,1, 1,0,0,0, 0,1,1,1, 0,0,0,0, 1,0,0,0, 1,1,1,0, 0,0,0,0, 1,0,1,1, 0,0,1,1, 1,0,0,1, 0,0,1,1, 0,1,0,0, 0,1,0,1, 1,1,1,0,
      1,1,1,1, 0,1,1,0, 1,0,1,0, 0,0,1,0, 1,1,0,0, 1,0,0,1, 1,1,1,1, 1,1,1,0, 0,0,0,1, 1,0,1,1, 0,0,0,1, 0,1,1,1, 0,0,1,1, 0,1,1,1,
      1,0,0,0, 0,1,0,0, 0,0,0,1, 1,0,0,0, 1,1,0,1, 1,0,0,1, 0,1,0,1, 1,0,1,0, 0,1,1,0, 1,1,1,1, 1,0,0,1, 1,0,0,1, 0,1,1,1, 1,0,1,0,

      0,1,1,1, 1,0,1,1, 0,1,1,1, 1,1,0,1, 0,1,1,1, 1,0,1,1, 0,0,1,1, 1,1,1,0, 1,0,0,1, 1,1,1,1, 1,1,0,0, 1,0,0,1, 1,1,1,0, 1,0,1,0,
      0,1,0,1, 1,1,1,0, 0,1,1,1, 1,0,0,0, 1,0,1,1, 1,0,1,0, 0,0,0,0, 0,0,1,1, 1,0,1,0, 0,1,1,0, 1,1,0,1, 0,1,0,1, 0,0,0,1, 1,0,1,0,
      0,0,1,0, 0,1,1,1, 1,0,0,1, 1,1,0,0, 1,1,0,0, 0,0,1,0, 0,1,1,0, 0,1,0,1, 0,1,0,0, 0,0,1,1, 1,1,1,0, 1,1,0,0, 1,1,0,1, 0,0,0,0,
      0,0,1,1, 0,1,0,0, 0,0,1,0, 1,0,1,1, 0,0,0,0, 0,1,0,0, 1,0,0,1, 1,0,0,0, 1,0,1,1, 1,1,1,1, 0,0,1,1, 1,1,0,1, 0,1,1,1, 1,1,0,1,

      1,0,1,0, 1,1,0,1, 1,1,0,1, 0,0,0,0, 0,0,1,1, 0,1,1,0, 1,1,1,0, 1,0,0,1, 1,1,0,1, 0,1,0,1, 0,0,1,1, 0,0,0,1, 0,0,1,0, 1,1,1,1,
      0,0,0,1, 0,0,0,0, 0,1,1,0, 0,0,0,1, 1,1,0,0, 0,1,1,0, 1,1,0,1, 1,1,1,1, 1,0,0,0, 0,0,1,0, 0,1,1,0, 0,0,1,0, 0,0,1,1, 0,1,1,1,
      0,1,1,1, 0,0,1,0, 1,1,0,1, 0,0,1,1, 1,1,1,0, 0,0,0,0, 1,0,0,1, 0,0,0,0, 0,1,1,1, 0,0,1,1, 1,0,0,0, 0,1,0,0, 1,1,0,0, 0,1,1,1,
      0,0,1,1, 1,0,1,1, 1,1,0,1, 0,1,0,1, 1,0,1,0, 1,1,0,0, 1,1,1,0, 1,1,1,0, 0,0,1,0, 0,1,0,1, 1,1,1,0, 0,0,1,0, 1,1,0,0, 1,0,0,1,

      0,1,0,1, 1,0,0,1, 0,0,0,0, 1,0,0,0, 0,1,1,1, 1,1,0,1, 1,0,0,0, 0,0,1,0, 0,1,1,0, 0,0,0,1, 0,1,0,1, 1,0,1,0, 1,1,0,1, 1,0,1,0,
      1,1,1,0, 1,0,0,1, 1,0,1,0, 1,1,1,1, 0,0,0,0, 0,0,0,1, 0,1,1,1, 0,0,1,0, 1,1,0,0, 1,1,1,1, 1,0,0,1, 1,1,0,1, 1,0,1,0, 0,1,1,1,
      0,0,1,1, 1,1,1,1, 0,1,0,0, 1,0,0,0, 0,0,1,1, 0,1,0,1, 1,0,1,0, 0,1,0,0, 0,0,0,0, 0,1,1,0, 0,0,1,1, 1,1,1,1, 0,0,0,0, 0,1,1,1,
      0,0,1,0, 0,0,1,1, 1,1,0,0, 1,0,0,1, 1,0,1,0, 1,1,1,0, 1,1,1,0, 1,1,0,0, 1,1,1,1, 0,0,1,0, 1,1,1,0, 1,1,0,1, 0,1,0,0, 0,0,0,1 };


static INT32 g_sym_delay_fifo_i[PLS_DECODER_SYMBOL_DELAY] = {0};  // Fixed( 1, 1, 6 );
static INT32 g_sym_delay_fifo_q[PLS_DECODER_SYMBOL_DELAY] = {0};  // Fixed( 1, 1, 6 );

static INT32 g_rm_decoder_out = 0;      // Fixed( 0, 8,  0 );
static INT32 g_conv_decoder_out = 0;    // Fixed( 0, 16, 0 );

static INT32 g_modulation = 0;          // Modulation type                                        Fixed( 0, 4,  0 );
static INT32 g_code_id = 0;             // LDPC code identifier                                   Fixed( 0, 6,  0 );
static INT32 g_fec_frame_type = 0;      // LDPC frame type;  0: Normal;  1: Short;  2: Medium;    Fixed( 0, 2,  0 );
static INT32 g_pilot_flag = NO;         // Pilot on frame or not;                                 Fixed( 0, 1,  0 );
static INT32 g_dummy_flag = NO;         // Dummy frame or not;                                    Fixed( 0, 1,  0 );
static INT32 g_vlsnr_flag = NO;         // VL-SNR frame or not;                                   Fixed( 0, 1,  0 );
static INT32 g_vlsnr_set = 0;           // Effective only in VL-SNR frame;  0: Set 1;  1: Set 2;  Fixed( 0, 1,  0 );
static INT32 g_frame_size = 0;          // Number of symbols in a PL frame;                       Fixed( 0, 16, 0 );
static INT32 g_modcod_type = 0;         // MODCOD_TYPE                                            Fixed( 0, 8,  0 );
//static INT32 g_is_reserved_modcod = 0;     // indicate if it is S2X reserved MODCOD               Fixed( 0, 1,  0 );  Chase @20161017
static INT32 g_is_s2_reserved_modcod = 0;  // indicate if it is S2 reserved MODCOD                Fixed( 0, 1,  0 );  Chase @20161027
static INT32 g_last_modcod_type = 0;    // MODCOD_TYPE of previous frame;                         Fixed( 0, 8,  0 );
static INT32 g_tsn = 0;                 // TSN                                                    Fixed( 0, 8,  0 );
static INT32 g_decode_enhance = NO;     // Decoder result has been refine or not;                 Fixed( 0, 1,  0 );
static INT32 gg_sym_type = DATA;         // Symbol type;                                           Fixed( 0, 3,  0 );

static INT32 g_plsd_start = NO;         // physical layer signalling decoder (plsd);  Fixed( 0, 1, 0 );
static INT32 g_plsd_finish = NO;        // Fixed( 0, 1, 0 );
static INT32 g_plsd_sym_cnt = 0;        // Fixed( 0, 9, 0 );  @
static INT32 g_plsd_wideband_mode = NO; // Fixed( 0, 1, 0 );

static INT32 g_phase_offset = 0;     // Fixed( 0, 9, 0 );  In radian;
static INT32 g_cos_val = 0;          // Fixed( 1, 0, 8 );
static INT32 g_sin_val = 0;          // Fixed( 1, 0, 8 );

void pl_signalling(
    // Input
    INT32    plfs_out_i,                      // Fixed( 1, 1,  6 );
    INT32    plfs_out_q,                      // Fixed( 1, 1,  6 );
    INT32    plfs_decoder_start,              // Fixed( 0, 1,  0 );
    INT32    plfs_check_cnt,                  // Fixed( 0, 8,  0 );  For test only
    // Output
    INT32    *pls_out_i,                      // Fixed( 1, 1,  6 );
    INT32    *pls_out_q,                      // Fixed( 1, 1,  6 );
    INT32    *pls_sym_type,                   // Fixed( 0, 3,  0 );
    INT32    *pls_frame_start,                // Fixed( 0, 1,  0 );
    INT32    *pls_fec_frame_type,             // Fixed( 0, 2,  0 );
    INT32    *pls_modulation,                 // Fixed( 0, 4,  0 );
    INT32    *pls_code_id,                    // Fixed( 0, 6,  0 );
    INT32    *pls_pilot_flag,                 // Fixed( 0, 1,  0 );
    INT32    *pls_dummy_flag,                 // Fixed( 0, 1,  0 );
    INT32    *pls_frame_size,                 // Fixed( 0, 16, 0 );
    INT32    *pls_modcod_type                 // Fixed( 0, 8,  0 );
){
    
    INT32 n;
    INT32 valid;                     // Fixed( 0, 1, 0 );
    INT32 is_first_header_sym = NO;  // Fixed( 0, 1, 0 );
    //INT32 header_scrmb_seq  = 0;     // Fixed( 0, 2, 0 );
    //INT32 mapped_sym_i = 0;          // Fixed( 1, 1, 6 );
    //INT32 mapped_sym_q = 0;          // Fixed( 1, 1, 6 );

    for (n = 0; n < (PLS_DECODER_SYMBOL_DELAY - 1); n++)
    {
        g_sym_delay_fifo_i[n] = g_sym_delay_fifo_i[n + 1];
        g_sym_delay_fifo_q[n] = g_sym_delay_fifo_q[n + 1];
    }
    g_sym_delay_fifo_i[PLS_DECODER_SYMBOL_DELAY - 1] = plfs_out_i;
    g_sym_delay_fifo_q[PLS_DECODER_SYMBOL_DELAY - 1] = plfs_out_q;

    if (YES == plfs_decoder_start)
    {
        g_plsd_start         = YES;
        g_plsd_finish        = NO;
        g_plsd_sym_cnt       = 0;
    }

    if (YES == g_plsd_start)
    {
        if (NO == g_plsd_finish)
        {
            rm_decoder(
                plfs_out_i,         // (1,1,6)
                plfs_out_q,         // (1,1,6)
                g_sym_delay_fifo_i, // (1,1,6)
                g_sym_delay_fifo_q, // (1,1,6)
                g_sof_bits,
                g_rm_plscode_scrambler,
                g_first_row_of_generator_matrix,
                0,//preset_b0_of_modcod_type,
                3,//preset_b6_of_modcod_type,
                3,//preset_b7_of_modcod_type,
                &g_plsd_finish,
                &g_rm_decoder_out );// (0,8,0)
        }

        if (YES == g_plsd_finish)
        {
	        g_is_s2_reserved_modcod = ((g_rm_decoder_out >> 2) == 29) ||
						              ((g_rm_decoder_out >> 2) == 30) ||
									  ((g_rm_decoder_out >> 2) == 31);

			if (g_is_s2_reserved_modcod)
				g_modcod_type = g_last_modcod_type;
			else
				g_modcod_type = g_rm_decoder_out;

            g_last_modcod_type = g_modcod_type;
            g_plsd_start = NO;
        }
    }

    g_plsd_sym_cnt ++;

    if ((PLS_DECODER_SYMBOL_DELAY == g_plsd_sym_cnt) && (YES == g_plsd_finish))
    {
        valid = get_pls_infor(
            g_modcod_type,
            0,//s2_legacy_vlsnr_en,
            0,//s2_legacy_vlsnr_modcod_type,
            &g_modulation,
            &g_code_id,
            &g_fec_frame_type,
            &g_pilot_flag,
            &g_dummy_flag,
            &g_vlsnr_flag,
            &g_vlsnr_set,
            &g_frame_size );

        if (NO == valid)
        {
            pl_signalling_init( );
        }
        else
        {
            is_first_header_sym = YES;
        }
    }

    get_symbol_type(
        is_first_header_sym,
        g_plsd_wideband_mode,
        g_pilot_flag,
        g_vlsnr_flag,
        g_vlsnr_set,
        &gg_sym_type );

    // Output
    *pls_out_i          = g_sym_delay_fifo_i[0];
    *pls_out_q          = g_sym_delay_fifo_q[0];
    *pls_sym_type       = gg_sym_type;
    *pls_frame_start    = is_first_header_sym;
    *pls_fec_frame_type = g_fec_frame_type;
    *pls_modulation     = g_modulation;
    *pls_code_id        = g_code_id;
    *pls_pilot_flag     = g_pilot_flag;
    *pls_dummy_flag     = g_dummy_flag;
    *pls_frame_size     = g_frame_size;
    *pls_modcod_type    = g_modcod_type;
}


void pl_signalling_init()
{
    INT32 n;

    for (n = 0; n < PLS_DECODER_SYMBOL_DELAY; n++)
    {
        g_sym_delay_fifo_i[n] = 0;
        g_sym_delay_fifo_q[n] = 0;
    }

    g_rm_decoder_out = 0;
    g_conv_decoder_out = 0;

    g_modulation = 0;
    g_code_id = 0;
    g_fec_frame_type = 0;
    g_pilot_flag = NO;
    g_dummy_flag = NO;
    g_vlsnr_flag = NO;
    g_vlsnr_set = 0;
    g_frame_size = 0;
    g_modcod_type = 0;
    g_last_modcod_type = 0;
    g_tsn = 0;
    g_decode_enhance = NO;
    gg_sym_type = DATA;

    g_plsd_start = NO;
    g_plsd_finish = NO;
    g_plsd_sym_cnt = 0;
    g_plsd_wideband_mode = NO;

    g_phase_offset = 0;
    g_cos_val = 0;
    g_sin_val = 0;

    rm_decoder_init( );
    get_symbol_type_init( );
}



void check_decode_result(
    // Input
    INT32    curr_metric_val,                // Fixed( 0, 1,  7 );
    INT32    pls_frame_size,                 // Fixed( 0, 16, 0 );
    INT32    frame_synch_lock,               // Fixed( 0, 1,  0 );
    // Output
    INT32    *start_decoder,                 // Fixed( 0, 1,  0 );
    INT32    *check_en,                      // Fixed( 0, 1,  0 );
    INT32    *check_cnt,                     // Fixed( 0, 8,  0 );
    INT32    *sym_cnt,                       // Fixed( 0, 16, 0 );
    INT32    *local_max_val )                // Fixed( 0, 1,  7 );
{
    INT32    start_index;    // Fixed( 0, 16, 0 );
    INT32    local_ref_val;  // Fixed( 0, 1, 10 );

    *sym_cnt = (*sym_cnt) + 1;
	//range = (20 << 4);  //20*16 = 320
    start_index  = pls_frame_size - 320;

    if ((*sym_cnt) > start_index)
    {
        if ((*sym_cnt) < pls_frame_size)
        {
            if (curr_metric_val > (*local_max_val))
            {
                *local_max_val = curr_metric_val;
            }
        }
        else if ((*sym_cnt) == pls_frame_size)
        {
            //local_ref_val = (*local_max_val) * (0.875);
			local_ref_val = (*local_max_val) * 7;

            if ((curr_metric_val << 3) > local_ref_val)
            {
                if ((*check_cnt) < 10)
                {
                    (*check_cnt) ++;
                }
            }
            else
            {
                if (NO == frame_synch_lock)
                {
                    if ((*check_cnt) > 5)
                    {
                        *check_cnt = (*check_cnt) - 2;
                    }
                    else if ((*check_cnt) > 1)
                    {
                        *check_cnt = (*check_cnt) - 1;
                    }
                    else
                    {
                        *check_cnt = 0;
                    }
                }
                else
                {
                    *check_cnt = (*check_cnt) - 1;
                }
            }

            if ((*check_cnt) > 0)
            {
                *check_en = YES;
                *start_decoder = YES;
            }
            else
            {
                *check_en = NO;
                *start_decoder = NO;
            }

            *sym_cnt = 0;
            *local_max_val = 0;
        }
        else {;}
    }
}


static INT32    g_check_en      = NO;  // Fixed( 0, 1,  0 );  # feedback signal;
static INT32    gg_sym_cnt      = 0;   // Fixed( 0, 16, 0 );  # feedback signal;
static INT32    g_local_ref_val = 0;   // Fixed( 0, 1,  7 );  # feedback signal;
static INT32    g_adjust_flag   = NO;  // Fixed( 0, 1,  0 );

void frame_track(
    INT32    curr_metric_val,                // Fixed( 0, 1,  7 );
    INT32    capture,                        // Fixed( 0, 1,  0 );
    INT32    pls_frame_size,                 // Fixed( 0, 16, 0 );
    INT32    frame_synch_lock,               // Fixed( 0, 1,  0 );
    INT32    *decoder_start,                 // Fixed( 0, 1,  0 );
    INT32    *check_cnt                      // Fixed( 0, 8,  0 );
)
{

    INT32 start = NO;

    if (YES == g_check_en)
    {
        check_decode_result(
           curr_metric_val,
           pls_frame_size,
           frame_synch_lock,
           &start,
           &g_check_en,
           check_cnt,
           &gg_sym_cnt,
           &g_local_ref_val );
    }

    // ==================================================================== //
    // Reset PLS decoder;
    // ==================================================================== //
    if ((YES == capture) && (NO == start))
    {
        if ((*check_cnt) < 1)
        {
            start = YES;
            g_check_en = YES;
            gg_sym_cnt = 0;
            g_local_ref_val = 0;
        }
        else
        {
            g_adjust_flag = YES;
        }
    }

    if (YES == g_adjust_flag)
    {
        if ((*check_cnt) > 4)
        {
            *check_cnt = (*check_cnt) - 2;    //@ Punish by 2 ?
        }
        else if ((*check_cnt) > 0)
        {
            (*check_cnt) --;    //@ Punish by 2 ?
        }        
		g_adjust_flag = NO;
    }

    *decoder_start = start;
}


void frame_track_init()
{
    g_check_en      = NO;
    gg_sym_cnt      = 0;
    g_local_ref_val = 0;
    g_adjust_flag   = NO;
}


INT32 get_pls_infor(
    // Input
    INT32 modcod_type,                  // Fixed( 0, 8,  0 );
    INT32 s2_legacy_vlsnr_en,           // Fixed( 0, 1,  0 );
    INT32 s2_legacy_vlsnr_modcod_type,  // Fixed( 0, 8,  0 );
    // Output
    INT32 *output_modulation,           // Fixed( 0, 4,  0 );
    INT32 *output_code_id,              // Fixed( 0, 6,  0 );
    INT32 *output_fec_frame_type,       // Fixed( 0, 2,  0 );
    INT32 *output_pilot_flag,           // Fixed( 0, 1,  0 );
    INT32 *output_dummy_flag,           // Fixed( 0, 1,  0 );
    INT32 *output_vlsnr_flag,           // Fixed( 0, 1,  0 );
    INT32 *output_vlsnr_set,            // Fixed( 0, 1,  0 );
    INT32 *output_frame_size )          // Fixed( 0, 16, 0 );
{

    INT32 valid;            // Fixed( 0, 1,  0 );
    INT32 modcod;           // Fixed( 0, 8,  0 );
    INT32 fec_frame_type;   // Fixed( 0, 2,  0 );
    INT32 dummy_flag;       // Fixed( 0, 1,  0 );
    INT32 pilot_flag;       // Fixed( 0, 1,  0 );
    INT32 code_id;          // Fixed( 0, 6,  0 );
    INT32 modulation;       // Fixed( 0, 4,  0 );
    INT32 frame_size;       // Fixed( 0, 16, 0 );
    INT32 vlsnr_flag;       // Fixed( 0, 1,  0 );
    INT32 vlsnr_set;        // Fixed( 0, 1,  0 );

    valid = YES;
    dummy_flag = NO;
    vlsnr_flag = NO;
    vlsnr_set  = 0;

    if (modcod_type < 128)  // S2 legacy MODCOD_TYPE, See Table 12
    {                       // in DVB-S2 standard (ETSI EN 302-307-1).
        modcod = modcod_type >> 2;
        pilot_flag = modcod_type & 0x1;
        fec_frame_type = (modcod_type >> 1) & 0x1;

        if (0 == modcod)          // Dummy
        {
            dummy_flag = YES;
            modulation = M_DUMMY;
            frame_size = 3330;
            code_id    = 0;           // Undefine code identifier;
            pilot_flag = NO;          // set PILOT_OFF mandatory!
            fec_frame_type = NORMAL;  // set Normal or leave it unchanged ?
        }
        else if (modcod < 12)     // QPSK
        {
            modulation = M_QPSK;
            frame_size = fec_frame_type ? (pilot_flag ? 8370 : 8190)
                                        : (pilot_flag ? 33282 : 32490);

            switch (modcod)
            {
                case 1:
                    code_id = CODE_ID_1_4;
                    break;

                case 2:
                    code_id = CODE_ID_1_3;
                    break;

                case 3:
                    code_id = CODE_ID_2_5;
                    break;

                case 4:
                    code_id = CODE_ID_1_2;
                    break;

                case 5:
                    code_id = CODE_ID_3_5;
                    break;

                case 6:
                    code_id = CODE_ID_2_3;
                    break;

                case 7:
                    code_id = CODE_ID_3_4;
                    break;

                case 8:
                    code_id = CODE_ID_4_5;
                    break;

                case 9:
                    code_id = CODE_ID_5_6;
                    break;

                case 10:
                    code_id = CODE_ID_8_9;
                    break;

                case 11:
                    code_id = CODE_ID_9_10;
                    break;

                default:
                    //exit( -1 );
                    break;

            }  // End of switch (modcod)
        }
        else if (modcod < 18)     // 8PSK
        {
            modulation = M_8PSK;
            frame_size = fec_frame_type ? (pilot_flag ? 5598 : 5490)
                                        : (pilot_flag ? 22194 : 21690);

            switch (modcod)
            {
                case 12:
                    code_id = CODE_ID_3_5;
                    break;

                case 13:
                    code_id = CODE_ID_2_3;
                    break;

                case 14:
                    code_id = CODE_ID_3_4;
                    break;

                case 15:
                    code_id = CODE_ID_5_6;
                    break;

                case 16:
                    code_id = CODE_ID_8_9;
                    break;

                case 17:
                    code_id = CODE_ID_9_10;
                    break;

                default:
                    //exit( -1 );
                    break;
            }
        }
        else if (modcod < 24)     // 16APSK
        {
            modulation = M_16APSK;
            frame_size = fec_frame_type ? (pilot_flag ? 4212 : 4140)
                                        : (pilot_flag ? 16686 : 16290);

            switch (modcod)
            {
                case 18:
                    code_id = CODE_ID_2_3;
                    break;

                case 19:
                    code_id = CODE_ID_3_4;
                    break;

                case 20:
                    code_id = CODE_ID_4_5;
                    break;

                case 21:
                    code_id = CODE_ID_5_6;
                    break;

                case 22:
                    code_id = CODE_ID_8_9;
                    break;

                case 23:
                    code_id = CODE_ID_9_10;
                    break;

                default:
                    //exit( -1 );
                    break;
            }
        }
        else if (modcod < 29)     // 32APSK
        {
            modulation = M_32APSK;
            frame_size = fec_frame_type ? (pilot_flag ? 3402 : 3330)
                                        : (pilot_flag ? 13338 : 13050);

            switch (modcod)
            {
                case 24:
                    code_id = CODE_ID_3_4;
                    break;

                case 25:
                    code_id = CODE_ID_4_5;
                    break;

                case 26:
                    code_id = CODE_ID_5_6;
                    break;

                case 27:
                    code_id = CODE_ID_8_9;
                    break;

                case 28:
                    code_id = CODE_ID_9_10;
                    break;

                default:
                    //exit( -1 );
                    break;
            }
        }
        else                      // Reserved
        {
            frame_size = 0;   // Undefine frame size; Use 0 instead;
            modulation = 0;   // Undefine modulation; Use 0 instead;
            code_id    = 0;   // Undefine code identifer; Use 0 instead;
            valid = NO;

        }  //  End of if (0 == modcod)

        if (YES == s2_legacy_vlsnr_en)
        {
            if (modcod_type == s2_legacy_vlsnr_modcod_type)
            {
                if ((modcod >= 1)
                    && (modcod <= 11)
                    && (NORMAL == fec_frame_type)
                    && (PILOT_ON == pilot_flag))
                {
                    vlsnr_flag = YES;
                    vlsnr_set  = SET1;
                    fec_frame_type = NORMAL;        // Decide by VL-SNR decoder, and will be updated later;
                    pilot_flag = PILOT_ON;
                    modulation = M_QPSK;            // Decide by VL-SNR decoder, and will be updated later;
                    frame_size = 33282;
                    code_id = NORMAL_CODE_ID_2_9;   // Decide by VL-SNR decoder, and will be updated later;
                }
                else if ((modcod >= 18)
                    && (modcod <= 23)
                    && (NORMAL == fec_frame_type)
                    && (PILOT_ON == pilot_flag))
                {
                    vlsnr_flag = YES;
                    vlsnr_set  = SET2;
                    fec_frame_type = SHORT;
                    pilot_flag = PILOT_ON;
                    modulation = M_BPSK;
                    frame_size = 16686;
                    code_id = SHORT_CODE_ID_1_5;    // Decide by VL-SNR decoder, and will be updated later;
                }
            }
        }
    }
    else                    // S2X MODCOD coding scheme, see Table 17
    {                       // in DVB-S2X standard (ETSI EN 302-307-2).
        if ((modcod_type >= 132) && (modcod_type < 216))       // Normal
        {
            fec_frame_type = NORMAL;
            pilot_flag = modcod_type & 0x1;

            if (modcod_type < 138)        // QPSK
            {
                modulation = M_QPSK;
                frame_size = pilot_flag ? 33282 : 32490;

                if (modcod_type < 134)
                {
                    code_id = NORMAL_CODE_ID_13_45;
                }
                else if (modcod_type < 136)
                {
                    code_id = NORMAL_CODE_ID_9_20;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_11_20;
                }
            }
            else if (modcod_type < 148)   // 8APSK or 8PSK
            {
                modulation = M_8APSK;
                frame_size = pilot_flag ? 22194 : 21690;

                if (modcod_type < 140)
                {
                    code_id = NORMAL_CODE_ID_100_180;
                }
                else if (modcod_type < 142)
                {
                    code_id = NORMAL_CODE_ID_104_180;
                }
                else if (modcod_type < 144)
                {
                    code_id = NORMAL_CODE_ID_23_36;
                }
                else if (modcod_type < 146)
                {
                    code_id = NORMAL_CODE_ID_25_36;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_13_18;
                }
            }
            else if (modcod_type < 174)   // 16APSK
            {
                modulation = M_16APSK;
                frame_size = pilot_flag ? 16686 : 16290;

                if (modcod_type < 150)
                {
                    code_id = NORMAL_CODE_ID_90_180;
                }
                else if (modcod_type < 152)
                {
                    code_id = NORMAL_CODE_ID_96_180;
                }
                else if (modcod_type < 154)
                {
                    code_id = NORMAL_CODE_ID_100_180;
                }
                else if (modcod_type < 156)
                {
                    code_id = NORMAL_CODE_ID_26_45;
                }
                else if (modcod_type < 158)
                {
                    code_id = NORMAL_CODE_ID_3_5;
                }
                else if (modcod_type < 160)
                {
                    code_id = NORMAL_CODE_ID_18_30;
                }
                else if (modcod_type < 162)
                {
                    code_id = NORMAL_CODE_ID_28_45;
                }
                else if (modcod_type < 164)
                {
                    code_id = NORMAL_CODE_ID_23_36;
                }
                else if (modcod_type < 166)
                {
                    code_id = NORMAL_CODE_ID_20_30;
                }
                else if (modcod_type < 168)
                {
                    code_id = NORMAL_CODE_ID_25_36;
                }
                else if (modcod_type < 170)
                {
                    code_id = NORMAL_CODE_ID_13_18;
                }
                else if (modcod_type < 172)
                {
                    code_id = NORMAL_CODE_ID_140_180;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_154_180;
                }
            }
            else if (modcod_type < 184)   // 32APSK or 32-ary
            {
                modulation = M_32APSK;
                frame_size = pilot_flag ? 13338 : 13050;

                if (modcod_type < 176)
                {
                    code_id = NORMAL_CODE_ID_2_3;
                }
                else if (modcod_type < 178)  // 32-ary
                {
                    code_id = (176 == modcod_type) ? REG_CODE_ID_176 : REG_CODE_ID_177;
                }
                else if (modcod_type < 180)
                {
                    code_id = NORMAL_CODE_ID_128_180;
                }
                else if (modcod_type < 182)
                {
                    code_id = NORMAL_CODE_ID_132_180;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_140_180;
                }
            }
            else if (modcod_type < 200)   // 64APSK or 64-ary
            {
                modulation = M_64APSK;
                frame_size = pilot_flag ? 11142 : 10890;

                if (modcod_type < 186)
                {
                    code_id = NORMAL_CODE_ID_128_180;
                }
                else if (modcod_type < 188)
                {
                    code_id = NORMAL_CODE_ID_132_180;
                }
                else if (modcod_type < 190)  // 64-ary
                {
                    code_id = (188 == modcod_type) ? REG_CODE_ID_188 : REG_CODE_ID_189;
                }
                else if (modcod_type < 192)
                {
                    code_id = NORMAL_CODE_ID_7_9;
                }
                else if (modcod_type < 194)  // 64-ary
                {
                    code_id = (192 == modcod_type) ? REG_CODE_ID_192 : REG_CODE_ID_193;
                }
                else if (modcod_type < 196)
                {
                    code_id = NORMAL_CODE_ID_4_5;
                }
                else if (modcod_type < 198)  // 64-ary
                {
                    code_id = (196 == modcod_type) ? REG_CODE_ID_196 : REG_CODE_ID_197;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_5_6;
                }
            }
            else if (modcod_type < 204)   // 128APSK
            {
                modulation = M_128APSK;
                frame_size = pilot_flag ? 9576 : 9360;

                if (modcod_type < 202)
                {
                    code_id = NORMAL_CODE_ID_135_180;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_140_180;
                }
            }
            else                          // 256APSK
            {
                modulation = M_256APSK;
                frame_size = pilot_flag ? 8370 : 8190;

                if (modcod_type < 206)
                {
                    code_id = NORMAL_CODE_ID_116_180;
                }
                else if (modcod_type < 208)
                {
                    code_id = NORMAL_CODE_ID_20_30;
                }
                else if (modcod_type < 210)
                {
                    code_id = NORMAL_CODE_ID_124_180;
                }
                else if (modcod_type < 212)
                {
                    code_id = NORMAL_CODE_ID_128_180;
                }
                else if (modcod_type < 214)
                {
                    code_id = NORMAL_CODE_ID_22_30;
                }
                else
                {
                    code_id = NORMAL_CODE_ID_135_180;
                }

            }  // End of if (modcod_type < 138)
        }
        else if ((modcod_type >= 216) && (modcod_type < 250))  // Short
        {
            fec_frame_type = SHORT;
            pilot_flag = modcod_type & 0x1;

            if (modcod_type < 228)        // QPSK
            {
                modulation = M_QPSK;
                frame_size = pilot_flag ? 8370 : 8190;

                if (modcod_type < 218)
                {
                    code_id = SHORT_CODE_ID_11_45;
                }
                else if (modcod_type < 220)
                {
                    code_id = SHORT_CODE_ID_4_15;
                }
                else if (modcod_type < 222)
                {
                    code_id = SHORT_CODE_ID_14_45;
                }
                else if (modcod_type < 224)
                {
                    code_id = SHORT_CODE_ID_7_15;
                }
                else if (modcod_type < 226)
                {
                    code_id = SHORT_CODE_ID_8_15;
                }
                else
                {
                    code_id = SHORT_CODE_ID_32_45;
                }
            }
            else if (modcod_type < 236)   // 8PSK
            {
                modulation = M_8PSK;
                frame_size = pilot_flag ? 5598 : 5490;

                if (modcod_type < 230)
                {
                    code_id = SHORT_CODE_ID_7_15;
                }
                else if (modcod_type < 232)
                {
                    code_id = SHORT_CODE_ID_8_15;
                }
                else if (modcod_type < 234)
                {
                    code_id = SHORT_CODE_ID_26_45;
                }
                else
                {
                    code_id = SHORT_CODE_ID_32_45;
                }
            }
            else if (modcod_type < 246)   // 16APSK
            {
                modulation = M_16APSK;
                frame_size = pilot_flag ? 4212 : 4140;

                if (modcod_type < 238)
                {
                    code_id = SHORT_CODE_ID_7_15;
                }
                else if (modcod_type < 240)
                {
                    code_id = SHORT_CODE_ID_8_15;
                }
                else if (modcod_type < 242)
                {
                    code_id = SHORT_CODE_ID_26_45;
                }
                else if (modcod_type < 244)
                {
                    code_id = SHORT_CODE_ID_3_5;
                }
                else
                {
                    code_id = SHORT_CODE_ID_32_45;
                }
            }
            else                          // 32APSK
            {
                modulation = M_32APSK;
                frame_size = pilot_flag ? 3402 : 3330;

                if (modcod_type < 248)
                {
                    code_id = SHORT_CODE_ID_2_3;
                }
                else
                {
                    code_id = SHORT_CODE_ID_32_45;
                }

            }  // End of if (modcod_type < 228)
        }
        else if (modcod_type < 132)
        {
            if (129 == modcod_type)       // VL-SNR, set 1
            {
                vlsnr_flag = YES;
                vlsnr_set  = SET1;
                fec_frame_type = NORMAL;              // Decide by VL-SNR decoder, and will be updated later;
                pilot_flag = PILOT_ON;
                modulation = M_QPSK;                  // Decide by VL-SNR decoder, and will be updated later;
                frame_size = 33282;
                code_id = NORMAL_CODE_ID_2_9;         // Decide by VL-SNR decoder, and will be updated later;
            }
            else if (131 == modcod_type)  // VL-SNR, set 2
            {
                vlsnr_flag = YES;
                vlsnr_set  = SET2;
                fec_frame_type = SHORT;
                pilot_flag = PILOT_ON;
                modulation = M_BPSK;                  // Decide by VL-SNR decoder, and will be updated later;
                frame_size = 16686;
                code_id = SHORT_CODE_ID_1_5;          // Decide by VL-SNR decoder, and will be updated later;
            }
            else if (128 == modcod_type)  // 8-ary
            {
                fec_frame_type = NORMAL;
                pilot_flag = PILOT_OFF;
                modulation = M_8ARY;
                frame_size = 21690;
                code_id = REG_CODE_ID_128;
            }
            else                          // 16-ary
            {
                fec_frame_type = NORMAL;
                pilot_flag = PILOT_OFF;
                modulation = M_16ARY;
                frame_size = 16290;
                code_id = REG_CODE_ID_130;
            }
        }
        else
        {
            fec_frame_type = NORMAL;
            pilot_flag = PILOT_ON;
            if (250 == modcod_type)       // 8-ary, pilot on
            {
                modulation = M_8ARY;
                frame_size = 22194;
                code_id = REG_CODE_ID_250;
            }
            else if (251 == modcod_type)  // 16-ary
            {
                modulation = M_16ARY;
                frame_size = 16686;
                code_id = REG_CODE_ID_251;
            }
            else if (252 == modcod_type)  // 32-ary, pilot on
            {
                modulation = M_32ARY;
                frame_size = 13338;
                code_id = REG_CODE_ID_252;
            }
            else if (253 == modcod_type)  // 64-ary
            {
                modulation = M_64ARY;
                frame_size = 11142;
                code_id = REG_CODE_ID_253;
            }
            else if (254 == modcod_type)  // 256-ary, pilot on
            {
                modulation = M_256ARY;
                frame_size = 8370;
                code_id = REG_CODE_ID_254;
            }
            else                          // 1024-ary
            {
                modulation = M_1024ARY;
                frame_size = 6714;
                code_id = REG_CODE_ID_255;
            }
        }
    }

    *output_modulation     = modulation;
    *output_code_id        = code_id;
    *output_fec_frame_type = fec_frame_type;
    *output_pilot_flag     = pilot_flag;
    *output_dummy_flag     = dummy_flag;
    *output_vlsnr_flag     = vlsnr_flag;
    *output_vlsnr_set      = vlsnr_set;
    *output_frame_size     = frame_size;

    return valid;
}



static INT32 g_type_cnt = 0;          // Fixed( 0, 11, 0 );
static INT32 g_sym_type = DATA;       // Fixed( 0, 3,  0 );
static INT32 g_group_id = 0;          // Fixed( 0, 5,  0 );
static INT32 g_try_next_group = NO;   // Fixed( 0, 1,  0 );

void get_symbol_type(
    // Input
    INT32 reset,          // Fixed( 0, 1, 0 );
    INT32 wideband_flag,  // Fixed( 0, 1, 0 );
    INT32 pilot_flag,     // Fixed( 0, 1, 0 );
    INT32 vlsnr_flag,     // Fixed( 0, 1, 0 );
    INT32 vlsnr_set,      // Fixed( 0, 1, 0 );
    // Output
    INT32 *sym_type )     // Fixed( 0, 3, 0 );
{

    if (YES == reset)
    {
        g_type_cnt = 0;
        g_sym_type = wideband_flag ? CONV_HEADER_180 : RM_HEADER_90;
        g_group_id = 0;
        g_try_next_group = NO;
    }

    switch (g_sym_type)
    {
        case DATA:  // Data symbol

            if (YES == pilot_flag)
            {
                if (NO == vlsnr_flag)
                {
                    if (1440 == g_type_cnt)  // 1440 equal to 16 SLOT symbols.
                    {
                        g_type_cnt = 0;
                        g_sym_type = PILOT_36;
                    }
                }
                else   // VL-SNR frame, enable extra pilot symbols.
                {
                    if (SET1 == vlsnr_set)
                    {
                        if (0 == g_group_id)
                        {
                            if (540 == g_type_cnt)
                            {
                                g_type_cnt = 0;
                                g_sym_type = PILOT_36;
                                g_group_id ++;
                            }
                        }
                        else if (g_group_id < 19)
                        {
                            if (703 == g_type_cnt)
                            {
                                if (NO == g_try_next_group)
                                {
                                    g_type_cnt = 0;
                                    g_sym_type = PILOT_34;
                                }
                                else
                                {
                                    g_type_cnt = 0;
                                    g_sym_type = PILOT_36;
                                }

                                if (YES == g_try_next_group)
                                {
                                    g_group_id ++;
                                }
                                g_try_next_group = (!g_try_next_group);
                            }
                        }
                        else if (g_group_id < 22)
                        {
                            if (702 == g_type_cnt)
                            {
                                g_type_cnt = 0;
                                g_sym_type = PILOT_36;

                                if (YES == g_try_next_group)
                                {
                                    g_group_id ++;
                                }
                                g_try_next_group = (!g_try_next_group);
                            }
                        }
                        else
                        {
                            ;
                        }
                    }
                    else   // VL-SNR, Set 2
                    {
                        if (0 == g_group_id)
                        {
                            if (540 == g_type_cnt)
                            {
                                g_type_cnt = 0;
                                g_sym_type = PILOT_36;
                                g_group_id ++;
                            }
                        }
                        else if (g_group_id < 10)
                        {
                            if (704 == g_type_cnt)
                            {
                                if (NO == g_try_next_group)
                                {
                                    g_type_cnt = 0;
                                    g_sym_type = PILOT_32;
                                }
                                else
                                {
                                    g_type_cnt = 0;
                                    g_sym_type = PILOT_36;
                                }

                                if (YES == g_try_next_group)
                                {
                                    g_group_id ++;
                                }
                                g_try_next_group = (!g_try_next_group);
                            }
                        }
                        else if (10 == g_group_id)
                        {
                            if (702 == g_type_cnt)
                            {
                                g_type_cnt = 0;
                                g_sym_type = PILOT_36;

                                if (YES == g_try_next_group)
                                {
                                    g_group_id ++;
                                }
                                g_try_next_group = (!g_try_next_group);
                            }
                        }
                        else
                        {
                            ;  //printf( "DATA\n" );

                        }  // if (0 == g_group_id)

                    }  // End of if (SET1 == vlsnr_set)

                }  // End of if ((NO == vlsnr_flag) || (NO == g_vlsnr_extra_pilot_en))

            }  // End of if (YES == pilot_flag)
            break;

        case PILOT_36:         // Pilot block with 36 symbols.
            if (36 == g_type_cnt)
            {
                g_type_cnt = 0;
                g_sym_type = DATA;
            }
            break;

        case RM_HEADER_90:     // Reed-Muller header
            if (RM_HEADER_SIZE == g_type_cnt)
            {
                if (YES == vlsnr_flag)
                {
                    g_type_cnt = 0;
                    g_sym_type = VLSNR_HEADER_900;
                }
                else
                {
                    g_type_cnt = 0;
                    g_sym_type = DATA;
                }
            }
            break;

        case CONV_HEADER_180:  // Convolutional header
            if (CONV_HEADER_SIZE == g_type_cnt)
            {
                if (YES == vlsnr_flag)
                {
                    g_type_cnt = 0;
                    g_sym_type = VLSNR_HEADER_900;
                }
                else
                {
                    g_type_cnt = 0;
                    g_sym_type = DATA;
                }
            }
            break;

        case PILOT_34:         // Pilot block with 34 symbols.
            if (34 == g_type_cnt)
            {
                g_type_cnt = 0;
                g_sym_type = DATA;
            }
            break;

        case PILOT_32:         // Pilot block with 32 symbols.
            if (32 == g_type_cnt)
            {
                g_type_cnt = 0;
                g_sym_type = DATA;
            }
            break;

        case VLSNR_HEADER_900:  // VL-SNR header
            if (VLSNR_HEADER_SIZE == g_type_cnt)
            {
                g_type_cnt = 0;
                g_sym_type = DATA;
            }
            break;

        default:
            g_type_cnt = 0;
            g_sym_type = DATA;
            break;

    }

    g_type_cnt ++;
    *sym_type = g_sym_type;
}


void get_symbol_type_init()
{
    g_type_cnt = 0;
    g_sym_type = DATA;

    g_group_id = 0;
    g_try_next_group = NO;
}


// Delay (RM_PLSC_SIZE / 4 = 16) symbols in first stage.
static INT32  g_stage1_upper_buff_i[16] = {0};    // Fixed( 1, 2, 6 );
static INT32  g_stage1_upper_buff_q[16] = {0};    // Fixed( 1, 2, 6 );
static INT32  g_stage1_lower_buff_i[16] = {0};    // Fixed( 1, 2, 6 );
static INT32  g_stage1_lower_buff_q[16] = {0};    // Fixed( 1, 2, 6 );

// Delay (RM_PLSC_SIZE / 8 = 8) symbols in second stage.
static INT32  g_stage2_upper_buff_i[8] = {0};     // Fixed( 1, 3, 6 );
static INT32  g_stage2_upper_buff_q[8] = {0};     // Fixed( 1, 3, 6 );
static INT32  g_stage2_lower_buff_i[8] = {0};     // Fixed( 1, 3, 6 );
static INT32  g_stage2_lower_buff_q[8] = {0};     // Fixed( 1, 3, 6 );

// Delay (RM_PLSC_SIZE / 16 = 4) symbols in third stage.
static INT32  g_stage3_upper_buff_i[4] = {0};     // Fixed( 1, 4, 6 );
static INT32  g_stage3_upper_buff_q[4] = {0};     // Fixed( 1, 4, 6 );
static INT32  g_stage3_lower_buff_i[4] = {0};     // Fixed( 1, 4, 6 );
static INT32  g_stage3_lower_buff_q[4] = {0};     // Fixed( 1, 4, 6 );

// Delay (RM_PLSC_SIZE / 32 = 2) symbols in fourth stage.
static INT32  g_stage4_upper_buff_i[2] = {0};     // Fixed( 1, 5, 6 );
static INT32  g_stage4_upper_buff_q[2] = {0};     // Fixed( 1, 5, 6 );
static INT32  g_stage4_lower_buff_i[2] = {0};     // Fixed( 1, 5, 6 );
static INT32  g_stage4_lower_buff_q[2] = {0};     // Fixed( 1, 5, 6 );

// Delay (RM_PLSC_SIZE / 64 = 1) symbols in fifth stage.
static INT32  g_stage5_upper_buff_i;              // Fixed( 1, 6, 6 );
static INT32  g_stage5_upper_buff_q;              // Fixed( 1, 6, 6 );
static INT32  g_stage5_lower_buff_i;              // Fixed( 1, 6, 6 );
static INT32  g_stage5_lower_buff_q;              // Fixed( 1, 6, 6 );

static INT32  g_fht_count = 0;            // Fixed( 0, 7,  0 );   The n-th symbol of PLSCode (64 symbols);
static INT32  g_fht_opt_idx = 0;          // Fixed( 0, 6,  0 );   optimum index;
static INT32  g_fht_max = 0;              // Fixed( 0, 16, 6 );
static INT32  g_fht_max_i = 0;            // Fixed( 1, 7,  6 );
static INT32  g_fht_max_q = 0;            // Fixed( 1, 7,  6 );

static INT32  g_acc_sof_i = 0;            // Fixed( 1, 6, 6 );
static INT32  g_acc_sof_q = 0;            // Fixed( 1, 6, 6 );
static INT32  g_rm_decoder_sym_cnt = 0;   // Fixed( 0, 9, 0 );

INT32 rm_decoder(
    // Input
    INT32       data_in_i,                                         // Fixed( 1, 1, 6 );
    INT32       data_in_q,                                         // Fixed( 1, 1, 6 );
    INT32       sym_delay_fifo_i[PLS_DECODER_SYMBOL_DELAY],        // Fixed( 1, 1, 6 );
    INT32       sym_delay_fifo_q[PLS_DECODER_SYMBOL_DELAY],        // Fixed( 1, 1, 6 );
    const INT32 sof_bits[SOF_SIZE],                                // Fixed( 0, 1, 0 );
    const INT32 rm_plscode_scrambler[RM_PLSC_SIZE],                // Fixed( 0, 1, 0 );
    const INT32 first_row_of_generator_matrix[RM_PLSC_SIZE / 2],   // Fixed( 0, 1, 0 );
    INT32       preset_b0_of_modcod_type,                          // Fixed( 0, 2, 0 );
    INT32       preset_b6_of_modcod_type,                          // Fixed( 0, 2, 0 );
    INT32       preset_b7_of_modcod_type,                          // Fixed( 0, 2, 0 );
    // Output
    INT32       *out_valid,                                        // Fixed( 0, 1, 0 );
    INT32       *rm_decoder_out )                                  // Fixed( 0, 8, 0 );
{

    INT32 mapped_sof_sym_i;  // Fixed( 1, 1, 6 );
    INT32 mapped_sof_sym_q;  // Fixed( 1, 1, 6 );

    INT32 lower_i = 0;           // Fixed( 1, 1, 6 );
    INT32 lower_q = 0;           // Fixed( 1, 1, 6 );
    INT32 upper_i = 0;           // Fixed( 1, 1, 6 );
    INT32 upper_q = 0;           // Fixed( 1, 1, 6 );

    INT32 up_idx;               // Fixed( 0, 5, 0 );
    INT32 low_idx;              // Fixed( 0, 5, 0 );

    INT32 lower_xor;            // Fixed( 0, 1, 0 );
    INT32 upper_xor;            // Fixed( 0, 1, 0 );

    INT32 b0;                   // Fixed( 0, 1, 0 );
    INT32 b1;                   // Fixed( 0, 1, 0 );
    INT32 b2;                   // Fixed( 0, 1, 0 );
    INT32 b3;                   // Fixed( 0, 1, 0 );
    INT32 b4;                   // Fixed( 0, 1, 0 );
    INT32 b5;                   // Fixed( 0, 1, 0 );
    INT32 b6;                   // Fixed( 0, 1, 0 );
    INT32 b7;                   // Fixed( 0, 1, 0 );

    INT32 temp_add_i;        // Fixed( 1,  8,  4 );
    INT32 temp_add_q;        // Fixed( 1,  8,  4 );
    INT32 temp_sub_i;        // Fixed( 1,  8,  4 );
    INT32 temp_sub_q;        // Fixed( 1,  8,  4 );
    INT32 power_add;         // Fixed( 0, 18,  8 );
    INT32 power_sub;         // Fixed( 0, 18,  8 );

    INT32 max_i;             // Fixed( 1, 7, 6 );
    INT32 max_q;             // Fixed( 1, 7, 6 );

    INT32 index;                // Fixed( 0, 6, 0 );
    INT32 fht_out_valid = NO;   // Fixed( 0, 1, 0 );
    INT32 sof_idx;              // Fixed( 0, 5, 0 );
    INT32 plsc_idx;             // Fixed( 0, 6, 0 );

    INT32 finish = NO;          // Fixed( 0, 1, 0 );
    INT32 modcod_type = 0;      // Fixed( 0, 8, 0 );

    g_rm_decoder_sym_cnt ++;

    // Get one pair of PLSCode symbols;

    if ((g_rm_decoder_sym_cnt <= SOF_SIZE)
        && (UNKNOWN_BIT == preset_b6_of_modcod_type))
    {
        sof_idx = g_rm_decoder_sym_cnt - 1;
        sof_remapping(
            sof_idx,
            data_in_i,
            data_in_q,
            &mapped_sof_sym_i,
            &mapped_sof_sym_q );

        // Accumulate SOF symbols
		// g_acc_sof_i : (1,7,6) = (1,6,6)+(1,1,6)
        g_acc_sof_i += mapped_sof_sym_i;
		// g_acc_sof_q : (1,7,6) = (1,6,6)+(1,1,6)
        g_acc_sof_q += mapped_sof_sym_q;
    }

    if (g_rm_decoder_sym_cnt >= 59)  // 59 = (SOF_SIZE + ((RM_PLSC_SIZE / 2) + 1));
    {
        plsc_idx = g_rm_decoder_sym_cnt - 59;
        up_idx   = (PLS_DECODER_SYMBOL_DELAY - 1) - (RM_PLSC_SIZE >> 1);
        low_idx  = up_idx + (RM_PLSC_SIZE >> 1);
        if (plsc_idx & 0x1)  // Remove pi/2 rotation introduced by pi/2 BPSK;
        {
            upper_i =  sym_delay_fifo_q[up_idx];
            upper_q = -sym_delay_fifo_i[up_idx];

            lower_i =  sym_delay_fifo_q[low_idx];
            lower_q = -sym_delay_fifo_i[low_idx];
        }
        else
        {
            upper_i =  sym_delay_fifo_i[up_idx];
            upper_q =  sym_delay_fifo_q[up_idx];

            lower_i =  sym_delay_fifo_i[low_idx];
            lower_q =  sym_delay_fifo_q[low_idx];
        }

        // Assuming that b0 = 0, thus, only scramber is effective;
        upper_xor = rm_plscode_scrambler[plsc_idx];
        if (1 == upper_xor)
        {
            upper_i = -upper_i;
            upper_q = -upper_q;
        }

        lower_xor = rm_plscode_scrambler[plsc_idx + (RM_PLSC_SIZE >> 1)];
        if (1 == lower_xor)
        {
            lower_i = -lower_i;
            lower_q = -lower_q;
        }
    }
    else
    {
        // Wait for the first ((RM_PLSC_SIZE / 2) + 1) PLSCode symbols;
        return 0;			//;
    }
		
    fast_hadamard_transform(
        upper_i,
        upper_q,
        lower_i,
        lower_q,
        &fht_out_valid );

	if (YES == fht_out_valid)
	{
        b0 = 0;
        max_i = g_fht_max_i;			// (1,7,6)
        max_q = g_fht_max_q;			// (1,7,6)
        index = g_fht_opt_idx;
		finish = YES;
	}

    if (YES == finish)
    {
        b5 = (index >> 5) & 0x1;    // b5
        b4 = (index >> 4) & 0x1;    // b4
        b3 = (index >> 3) & 0x1;    // b3
        b2 = (index >> 2) & 0x1;    // b2
        b1 = (index >> 1) & 0x1;    // b1
        b7 = (index >> 0) & 0x1;    // b7

        //if (UNKNOWN_BIT == preset_b6_of_modcod_type)
        if ((0 != preset_b6_of_modcod_type)
            && (1 != preset_b6_of_modcod_type))
        {
            // The method of decoding b6 will not be injured by
            // phase offset; see design spec for more details.
            temp_add_i = g_acc_sof_i + max_i;			// (1,8,6) = (1,6,6) + (1,7,6)
            temp_add_q = g_acc_sof_q + max_q;			// (1,8,6) = (1,6,6) + (1,7,6)
            temp_sub_i = g_acc_sof_i - max_i;			// (1,8,6) = (1,6,6) + (1,7,6)
            temp_sub_q = g_acc_sof_q - max_q;			// (1,8,6) = (1,6,6) + (1,7,6)

			// power_add:(0,18,8) = (1,8,4)*(1,8,4) + (1,8,4)*(1,8,4)
            power_add = (temp_add_i * temp_add_i) + (temp_add_q * temp_add_q);
			// power_sub:(0,18,8) = (1,8,4)*(1,8,4) + (1,8,4)*(1,8,4)
            power_sub = (temp_sub_i * temp_sub_i) + (temp_sub_q * temp_sub_q);

            b6 = (power_add < power_sub);  // b6
        }
        else
        {
            b6 = preset_b6_of_modcod_type;
        }

        modcod_type = (b0 << 7) | (b1 << 6) | (b2 << 5) | (b3 << 4)
                    | (b4 << 3) | (b5 << 2) | (b6 << 1) | (b7 << 0);

        g_acc_sof_i = 0;
        g_acc_sof_q = 0;
        g_rm_decoder_sym_cnt = 0;

        g_fht_count = 0;
        g_fht_opt_idx = 0;
        g_fht_max = 0;
        g_fht_max_i = 0;
        g_fht_max_q = 0;
    }

    *out_valid = finish;
    *rm_decoder_out = modcod_type;
    return 0;
}


void rm_decoder_init()
{
    INT32  n;

    for (n = 0; n < 16; n++)
    {
        g_stage1_upper_buff_i[n] = 0;
        g_stage1_upper_buff_q[n] = 0;
        g_stage1_lower_buff_i[n] = 0;
        g_stage1_lower_buff_q[n] = 0;
    }

    for (n = 0; n < 8; n++)
    {
        g_stage2_upper_buff_i[n] = 0;
        g_stage2_upper_buff_q[n] = 0;
        g_stage2_lower_buff_i[n] = 0;
        g_stage2_lower_buff_q[n] = 0;
    }

    for (n = 0; n < 4; n++)
    {
        g_stage3_upper_buff_i[n] = 0;
        g_stage3_upper_buff_q[n] = 0;
        g_stage3_lower_buff_i[n] = 0;
        g_stage3_lower_buff_q[n] = 0;
    }

    for (n = 0; n < 2; n++)
    {
        g_stage4_upper_buff_i[n] = 0;
        g_stage4_upper_buff_q[n] = 0;
        g_stage4_lower_buff_i[n] = 0;
        g_stage4_lower_buff_q[n] = 0;
    }

    g_stage5_upper_buff_i = 0;
    g_stage5_upper_buff_q = 0;
    g_stage5_lower_buff_i = 0;
    g_stage5_lower_buff_q = 0;

    g_fht_count = 0;
    g_fht_opt_idx = 0;
    g_fht_max = 0;
    g_fht_max_i = 0;
    g_fht_max_q = 0;

    g_acc_sof_i = 0;
    g_acc_sof_q = 0;
    g_rm_decoder_sym_cnt = 0;
}


void fast_hadamard_transform(
    // Input
    INT32 fht_upper_i,  // Fixed( 1, 1, 6 );
    INT32 fht_upper_q,  // Fixed( 1, 1, 6 );
    INT32 fht_lower_i,  // Fixed( 1, 1, 6 );
    INT32 fht_lower_q,  // Fixed( 1, 1, 6 );
    // Output
    INT32 *out_valid )  // Fixed( 0, 1, 0 );
{

    INT32 n;

    INT32 c0;   // Fixed( 0, 1, 0 );
    INT32 c1;   // Fixed( 0, 1, 0 );
    INT32 c2;   // Fixed( 0, 1, 0 );
    INT32 c3;   // Fixed( 0, 1, 0 );
    INT32 c4;   // Fixed( 0, 1, 0 );

    INT32  stage1_upper_i = 0;   // Fixed( 1, 2, 6 );
    INT32  stage1_upper_q = 0;   // Fixed( 1, 2, 6 );
    INT32  stage1_lower_i = 0;   // Fixed( 1, 2, 6 );
    INT32  stage1_lower_q = 0;   // Fixed( 1, 2, 6 );

    INT32  stage2_upper_i = 0;   // Fixed( 1, 3, 6 );
    INT32  stage2_upper_q = 0;   // Fixed( 1, 3, 6 );
    INT32  stage2_lower_i = 0;   // Fixed( 1, 3, 6 );
    INT32  stage2_lower_q = 0;   // Fixed( 1, 3, 6 );

    INT32  stage3_upper_i = 0;   // Fixed( 1, 4, 6 );
    INT32  stage3_upper_q = 0;   // Fixed( 1, 4, 6 );
    INT32  stage3_lower_i = 0;   // Fixed( 1, 4, 6 );
    INT32  stage3_lower_q = 0;   // Fixed( 1, 4, 6 );

    INT32  stage4_upper_i = 0;   // Fixed( 1, 5, 6 );
    INT32  stage4_upper_q = 0;   // Fixed( 1, 5, 6 );
    INT32  stage4_lower_i = 0;   // Fixed( 1, 5, 6 );
    INT32  stage4_lower_q = 0;   // Fixed( 1, 5, 6 );

    INT32  stage5_upper_i = 0;   // Fixed( 1, 6, 6 );
    INT32  stage5_upper_q = 0;   // Fixed( 1, 6, 6 );
    INT32  stage5_lower_i = 0;   // Fixed( 1, 6, 6 );
    INT32  stage5_lower_q = 0;   // Fixed( 1, 6, 6 );

    INT32  stage6_upper_i = 0;   // Fixed( 1, 7, 6 );
    INT32  stage6_upper_q = 0;   // Fixed( 1, 7, 6 );
    INT32  stage6_lower_i = 0;   // Fixed( 1, 7, 6 );
    INT32  stage6_lower_q = 0;   // Fixed( 1, 7, 6 );

    INT32  stage2_upper_input_i;   // Fixed( 1, 2, 6 );
    INT32  stage2_upper_input_q;   // Fixed( 1, 2, 6 );
    INT32  stage2_lower_input_i;   // Fixed( 1, 2, 6 );
    INT32  stage2_lower_input_q;   // Fixed( 1, 2, 6 );

    INT32  stage3_upper_input_i;   // Fixed( 1, 3, 6 );
    INT32  stage3_upper_input_q;   // Fixed( 1, 3, 6 );
    INT32  stage3_lower_input_i;   // Fixed( 1, 3, 6 );
    INT32  stage3_lower_input_q;   // Fixed( 1, 3, 6 );

    INT32  stage4_upper_input_i;   // Fixed( 1, 4, 6 );
    INT32  stage4_upper_input_q;   // Fixed( 1, 4, 6 );
    INT32  stage4_lower_input_i;   // Fixed( 1, 4, 6 );
    INT32  stage4_lower_input_q;   // Fixed( 1, 4, 6 );

    INT32  stage5_upper_input_i;   // Fixed( 1, 5, 6 );
    INT32  stage5_upper_input_q;   // Fixed( 1, 5, 6 );
    INT32  stage5_lower_input_i;   // Fixed( 1, 5, 6 );
    INT32  stage5_lower_input_q;   // Fixed( 1, 5, 6 );

    INT32  stage6_upper_input_i;   // Fixed( 1, 6, 6 );
    INT32  stage6_upper_input_q;   // Fixed( 1, 6, 6 );
    INT32  stage6_lower_input_i;   // Fixed( 1, 6, 6 );
    INT32  stage6_lower_input_q;   // Fixed( 1, 6, 6 );

    INT32  temp_upper_i = 0;       // Fixed( 1, 7, 4 );
    INT32  temp_upper_q = 0;       // Fixed( 1, 7, 4 );
    INT32  temp_lower_i = 0;       // Fixed( 1, 7, 4 );
    INT32  temp_lower_q = 0;       // Fixed( 1, 7, 4 );

    INT32  power_upper = 0;        // Fixed( 0, 16, 6 );
    INT32  power_lower = 0;        // Fixed( 0, 16, 6 );
    INT32  tmp_max = 0;            // Fixed( 0, 16, 6 );
    INT32  tmp_max_i = 0;          // Fixed( 1, 7,  6 );
    INT32  tmp_max_q = 0;          // Fixed( 1, 7,  6 );
    INT32  tmp_index = 0;          // Fixed( 0, 7,  0 );


    //=================================================================================================
    // Stage 1: calculate correlation coefficient
    //=================================================================================================
    if (g_fht_count < 32)
    {   // (1,2,6) = (1,1,6) + (1,1,6)
        stage1_upper_i = fht_upper_i + fht_lower_i;
		// (1,2,6) = (1,1,6) + (1,1,6)
        stage1_upper_q = fht_upper_q + fht_lower_q;

		// (1,2,6) = (1,1,6) + (1,1,6)
        stage1_lower_i = fht_upper_i - fht_lower_i;
		// (1,2,6) = (1,1,6) + (1,1,6)
        stage1_lower_q = fht_upper_q - fht_lower_q;
    }
    else
    {
        stage1_upper_i = 0;
        stage1_upper_q = 0;

        stage1_lower_i = 0;
        stage1_lower_q = 0;
    }


    //=================================================================================================
    // Stage 2: calculate correlation coefficient
    //=================================================================================================
    c4 = (g_fht_count >> 4) & 0x1;
	// (1,2,6)
    stage2_upper_input_i = !(c4) ? g_stage1_lower_buff_i[15] : g_stage1_upper_buff_i[15];
	// (1,2,6)
    stage2_upper_input_q = !(c4) ? g_stage1_lower_buff_q[15] : g_stage1_upper_buff_q[15];

	// (1,2,6)
    stage2_lower_input_i = !(c4) ? g_stage1_upper_buff_i[15] : stage1_upper_i;
	// (1,2,6)
    stage2_lower_input_q = !(c4) ? g_stage1_upper_buff_q[15] : stage1_upper_q;

	// (1,3,6) = (1,2,6) + (1,2,6)
    stage2_upper_i = stage2_upper_input_i + stage2_lower_input_i;  //@ Output of stage 1;
	// (1,3,6) = (1,2,6) + (1,2,6)
    stage2_upper_q = stage2_upper_input_q + stage2_lower_input_q;

	// (1,3,6) = (1,2,6) + (1,2,6)
    stage2_lower_i = stage2_upper_input_i - stage2_lower_input_i;
	// (1,3,6) = (1,2,6) + (1,2,6)
    stage2_lower_q = stage2_upper_input_q - stage2_lower_input_q;

    for (n = 15; n > 0; n--)
    {   // (1,2,6)
        g_stage1_upper_buff_i[n] = g_stage1_upper_buff_i[n - 1];
		// (1,2,6)
        g_stage1_upper_buff_q[n] = g_stage1_upper_buff_q[n - 1];
    }
	// (1,2,6)
    g_stage1_upper_buff_i[0] = (c4) ? stage1_lower_i : stage1_upper_i;
	// (1,2,6)
    g_stage1_upper_buff_q[0] = (c4) ? stage1_lower_q : stage1_upper_q;

    if (!(c4))
    {
        for (n = 15; n > 0; n--)
        {   // (1,2,6)
            g_stage1_lower_buff_i[n] = g_stage1_lower_buff_i[n - 1];
			// (1,2,6)
            g_stage1_lower_buff_q[n] = g_stage1_lower_buff_q[n - 1];
        }
        g_stage1_lower_buff_i[0] = stage1_lower_i;
        g_stage1_lower_buff_q[0] = stage1_lower_q;
    }


    //=================================================================================================
    // Stage 3: calculate correlation coefficient
    //=================================================================================================
    c3 = (g_fht_count >> 3) & 0x1;
	// (1,3,6)
    stage3_upper_input_i = !(c3) ? g_stage2_lower_buff_i[7] : g_stage2_upper_buff_i[7];
	// (1,3,6)
    stage3_upper_input_q = !(c3) ? g_stage2_lower_buff_q[7] : g_stage2_upper_buff_q[7];

	// (1,3,6)
    stage3_lower_input_i = !(c3) ? g_stage2_upper_buff_i[7] : stage2_upper_i;
	// (1,3,6)
    stage3_lower_input_q = !(c3) ? g_stage2_upper_buff_q[7] : stage2_upper_q;

	// (1,4,6) = (1,3,6) + (1,3,6)
    stage3_upper_i = stage3_upper_input_i + stage3_lower_input_i;
	// (1,4,6) = (1,3,6) + (1,3,6)
    stage3_upper_q = stage3_upper_input_q + stage3_lower_input_q;

	// (1,4,6) = (1,3,6) + (1,3,6)
    stage3_lower_i = stage3_upper_input_i - stage3_lower_input_i;
	// (1,4,6) = (1,3,6) + (1,3,6)
    stage3_lower_q = stage3_upper_input_q - stage3_lower_input_q;

    if (g_fht_count >= 16)  // 16
    {
        for (n = 7; n > 0; n--)
        {   // (1,3,6)
            g_stage2_upper_buff_i[n] = g_stage2_upper_buff_i[n - 1];
			// (1,3,6)
            g_stage2_upper_buff_q[n] = g_stage2_upper_buff_q[n - 1];
        }
		// (1,3,6)
        g_stage2_upper_buff_i[0] = (c3) ? stage2_lower_i : stage2_upper_i; //@
		// (1,3,6)
        g_stage2_upper_buff_q[0] = (c3) ? stage2_lower_q : stage2_upper_q; //@
    }

    if (!(c3))
    {
        for (n = 7; n > 0; n--)
        {   // (1,3,6)
            g_stage2_lower_buff_i[n] = g_stage2_lower_buff_i[n - 1];
			// (1,3,6)
            g_stage2_lower_buff_q[n] = g_stage2_lower_buff_q[n - 1];
        }
		// (1,3,6)
        g_stage2_lower_buff_i[0] = stage2_lower_i;
		// (1,3,6)
        g_stage2_lower_buff_q[0] = stage2_lower_q;
    }


    //=================================================================================================
    // Stage 4: calculate correlation coefficient
    //=================================================================================================
    c2 = (g_fht_count >> 2) & 0x1;
	// (1,4,6)
    stage4_upper_input_i = !(c2) ? g_stage3_lower_buff_i[3] : g_stage3_upper_buff_i[3];
	// (1,4,6)
    stage4_upper_input_q = !(c2) ? g_stage3_lower_buff_q[3] : g_stage3_upper_buff_q[3];

	// (1,4,6)
    stage4_lower_input_i = !(c2) ? g_stage3_upper_buff_i[3] : stage3_upper_i;
	// (1,4,6)
    stage4_lower_input_q = !(c2) ? g_stage3_upper_buff_q[3] : stage3_upper_q;

	// (1,5,6) = (1,4,6) + (1,4,6)
    stage4_upper_i = stage4_upper_input_i + stage4_lower_input_i;
	// (1,5,6) = (1,4,6) + (1,4,6)
    stage4_upper_q = stage4_upper_input_q + stage4_lower_input_q;

	// (1,5,6) = (1,4,6) + (1,4,6)
    stage4_lower_i = stage4_upper_input_i - stage4_lower_input_i;
	// (1,5,6) = (1,4,6) + (1,4,6)
    stage4_lower_q = stage4_upper_input_q - stage4_lower_input_q;

    if (g_fht_count >= 24)  // 24 = 16 + 8;
    {
        for (n = 3; n > 0; n--)
        {
			// (1,4,6)
            g_stage3_upper_buff_i[n] = g_stage3_upper_buff_i[n - 1];
			// (1,4,6)
            g_stage3_upper_buff_q[n] = g_stage3_upper_buff_q[n - 1];
        }

		// (1,4,6)
        g_stage3_upper_buff_i[0] = (c2) ? stage3_lower_i : stage3_upper_i; //@
		// (1,4,6)
        g_stage3_upper_buff_q[0] = (c2) ? stage3_lower_q : stage3_upper_q; //@
    }

    if (!(c2))
    {
        for (n = 3; n > 0; n--)
        {   // (1,4,6)
            g_stage3_lower_buff_i[n] = g_stage3_lower_buff_i[n - 1];
			// (1,4,6)
            g_stage3_lower_buff_q[n] = g_stage3_lower_buff_q[n - 1];
        }
		// (1,4,6)
        g_stage3_lower_buff_i[0] = stage3_lower_i;
		// (1,4,6)
        g_stage3_lower_buff_q[0] = stage3_lower_q;
    }


    //=================================================================================================
    // Stage 5: calculate correlation coefficient
    //=================================================================================================
    c1 = (g_fht_count >> 1) & 0x1;
	
	// (1,5,6)
    stage5_upper_input_i = !(c1) ? g_stage4_lower_buff_i[1] : g_stage4_upper_buff_i[1];
    stage5_upper_input_q = !(c1) ? g_stage4_lower_buff_q[1] : g_stage4_upper_buff_q[1];

	// (1,5,6)
    stage5_lower_input_i = !(c1) ? g_stage4_upper_buff_i[1] : stage4_upper_i;
    stage5_lower_input_q = !(c1) ? g_stage4_upper_buff_q[1] : stage4_upper_q;

	// (1,6,6) = (1,5,6)+(1,5,6)
    stage5_upper_i = stage5_upper_input_i + stage5_lower_input_i;
    stage5_upper_q = stage5_upper_input_q + stage5_lower_input_q;

	// (1,6,6) = (1,5,6)+(1,5,6)
    stage5_lower_i = stage5_upper_input_i - stage5_lower_input_i;
    stage5_lower_q = stage5_upper_input_q - stage5_lower_input_q;

    if (g_fht_count >= 28)  // 28 = 16 + 8 + 4;
    {
        for (n = 1; n > 0; n--)
        {
			// (1,5,6)
            g_stage4_upper_buff_i[n] = g_stage4_upper_buff_i[n - 1];
            g_stage4_upper_buff_q[n] = g_stage4_upper_buff_q[n - 1];
        }

		// (1,5,6)
        g_stage4_upper_buff_i[0] = (c1) ? stage4_lower_i : stage4_upper_i; //@
        g_stage4_upper_buff_q[0] = (c1) ? stage4_lower_q : stage4_upper_q; //@
    }

    if (!(c1))
    {
        for (n = 1; n > 0; n--)
        {   // (1,5,6)
            g_stage4_lower_buff_i[n] = g_stage4_lower_buff_i[n - 1];
            g_stage4_lower_buff_q[n] = g_stage4_lower_buff_q[n - 1];
        }

		// (1,5,6)
        g_stage4_lower_buff_i[0] = stage4_lower_i;
        g_stage4_lower_buff_q[0] = stage4_lower_q;
    }

    //==================================================================================================
    // Stage 6: calculate correlation coefficient
    //==================================================================================================
    c0 = (g_fht_count >> 0) & 0x1;

	// (1,6,6)
    stage6_upper_input_i = !(c0) ? g_stage5_lower_buff_i : g_stage5_upper_buff_i;
	// (1,6,6)
    stage6_upper_input_q = !(c0) ? g_stage5_lower_buff_q : g_stage5_upper_buff_q;

	// (1,6,6)
    stage6_lower_input_i = !(c0) ? g_stage5_upper_buff_i : stage5_upper_i;
	// (1,6,6)
    stage6_lower_input_q = !(c0) ? g_stage5_upper_buff_q : stage5_upper_q;

	// (1,7,6) = (1,6,6) + (1,6,6)
    stage6_upper_i = stage6_upper_input_i + stage6_lower_input_i;
    stage6_upper_q = stage6_upper_input_q + stage6_lower_input_q;

	// (1,7,6) = (1,6,6) + (1,6,6)
    stage6_lower_i = stage6_upper_input_i - stage6_lower_input_i;
    stage6_lower_q = stage6_upper_input_q - stage6_lower_input_q;

    if (g_fht_count >= 30)  // 30 = 16 + 8 + 4 + 2;
    {   // (1,6,6)
        g_stage5_upper_buff_i = (c0) ? stage5_lower_i : stage5_upper_i;
        g_stage5_upper_buff_q = (c0) ? stage5_lower_q : stage5_upper_q;
    }

    if (!(c0))
    {   // (1,6,6)
        g_stage5_lower_buff_i = stage5_lower_i;
        g_stage5_lower_buff_q = stage5_lower_q;
    }

    //==================================================================================================
    // Search maximum correlation coefficient
    //==================================================================================================

    if (g_fht_count >= 31)
    {   // (1,7,6)
        temp_upper_i = stage6_upper_i;
		// (1,7,6)
        temp_upper_q = stage6_upper_q;
		// (1,7,6)
        temp_lower_i = stage6_lower_i;
		// (1,7,6)
        temp_lower_q = stage6_lower_q;

		// (0,16,8) = (1,7,4)*(1,7,4) + (1,7,4)*(1,7,4)
        power_upper = (temp_upper_i * temp_upper_i) + (temp_upper_q * temp_upper_q);
		// (0,16,8) = (1,7,4)*(1,7,4) + (1,7,4)*(1,7,4)
        power_lower = (temp_lower_i * temp_lower_i) + (temp_lower_q * temp_lower_q);

        if (power_upper > power_lower)
        {
            tmp_index = (g_fht_count - 31) * 2;
            tmp_max_i = stage6_upper_i;
            tmp_max_q = stage6_upper_q;
            tmp_max = power_upper;
        }
        else
        {
            tmp_index = (g_fht_count - 31) * 2 + 1;
            tmp_max_i = stage6_lower_i;
            tmp_max_q = stage6_lower_q;
            tmp_max = power_lower;
        }

        if (g_fht_max < tmp_max)
        {
            g_fht_opt_idx = tmp_index;
            g_fht_max_i   = tmp_max_i;
            g_fht_max_q   = tmp_max_q;
            g_fht_max     = tmp_max;
        }
    }
    g_fht_count ++;

    *out_valid = (RM_PLSC_SIZE == g_fht_count);
}



void sof_remapping(
    INT32 sof_sym_index,       // Fixed( 0, 5, 0 );
    INT32 sof_sym_i,           // Fixed( 1, 1, 6 );
    INT32 sof_sym_q,           // Fixed( 1, 1, 6 );
    INT32 *mapped_sof_sym_i,   // Fixed( 1, 1, 6 );
    INT32 *mapped_sof_sym_q )  // Fixed( 1, 1, 6 );
{

    static const INT32 s_sof_bits[SOF_SIZE] =
    { 0, 1, 1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 1,
      0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0 };  // Fixed( 0, 1, 0 );

    INT32 index;   // Fixed( 0, 5, 0 );
    INT32 temp_i;  // Fixed( 1, 2, 6 );
    INT32 temp_q;  // Fixed( 1, 2, 6 );

    index = sof_sym_index;
    if (index & 0x1)
    {
        if (0 == s_sof_bits[index])   // Multiply by -j;
        {
            temp_i =  sof_sym_q;
            temp_q = -sof_sym_i;
        }
        else                          // Multiply by  j;
        {
            temp_i = -sof_sym_q;
            temp_q =  sof_sym_i;
        }
    }
    else
    {
        if (0 == s_sof_bits[index])   // Multiply by  1
        {
            temp_i = sof_sym_i;
            temp_q = sof_sym_q;
        }
        else                          // Multiply by -1;
        {
            temp_i = -sof_sym_i;
            temp_q = -sof_sym_q;
        }
    }

    *mapped_sof_sym_i = temp_i;
    *mapped_sof_sym_q = temp_q;
}

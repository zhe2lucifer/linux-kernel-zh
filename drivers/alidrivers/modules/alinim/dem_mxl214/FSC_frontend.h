#ifdef FSC_PROJECT

typedef struct
{
    int     channel_id;//simliar tuner_id;    
    int     demo_id; //demo_id == tuner for mxl214
    int     state;//0:uninit //1:free //2 used
    int     ssi_num;//ts interface num
    TSI_INF ssi_array[];
}NIM_FE;

typedef struct
{
    int     tsi_id;//ssi0,ssi1,ssi2,ssi3,assi,spi,. 
    int     state;//0:uninit //1:free //2 used
}TSI_INF;


typedef struct
{
    int     ts_id;//ts_a,ts_b,ts_c,ts_d
    int     state;//0:uninit //1:free //2 used
}TS_INF;

typedef struct
{
    int     dmx_id;//dmx0,dmx1,
    void    *device;
    int     state;//0:uninit //1:free //2 used
}DMX_INF;

typedef struct
{
    MULTIPLAY_TS_ROUTE  *next;
    unsigned int    prog_id;//prog_id
    unsigned int    tp_id;//TP id
    NIM_FE          nim_fe;
    TSI_INF         ssi_interfase;
    TS_INF          ts_way;
    DMX_INF         dmx_dev;
    int             real_play;//video show screen.non pre-play.
    int             state;////0:uninit //1:free //2 used
}MULTIPLAY_TS_ROUTE;

typedef struct
{
    int                mp_ts_num;
    MULTIPLAY_TS_ROUTE *mp_ts_route;
}MULTI_PLAY_PROG;

#endif

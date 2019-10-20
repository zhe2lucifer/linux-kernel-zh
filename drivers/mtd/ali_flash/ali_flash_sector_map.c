/*
 * Copyright 2014 Ali Corporation Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


/* Item 0 in following array is just an position holder. */
/* total sector number */
const unsigned short ali_flash_sectors[] =
{
	0, /* 0 */
	11, 11, 19, 19, 35, 35, 8, /* 1 */
	23, 23, /* 8 */
	128, 256, 512, /* 10 */
	16, /* 13 */  /*reserved for 39VF088 */
	32, /* 14 */
	39, 39, /* 15 */
	64, 71, 71, /* 17 */
    263, 270, /* 20 */
    128, /* 22 */
    512, /* 23 */
    12, 12, /* 24*/
    20, 20, /* 26 */
    36, 36, 32, /* 26 */
    128, 64, 68, 256,  /* 31 */
    40, 72, 132 , 128 /*35*/
};


/* Item 0 in following array is just an position holder. */
/* index of  flash_sector_map to present flash sector array structure.*/
const unsigned char ali_flash_sector_begin[] =
{
	0, /* 0 */
	0, 6, 0, 5, 0, 4, 3, /* 1 */
	10, 14, /* 8 */
	16, 16, 16, /* 10 */
	4, /* 13 */  /*reserved for 39VF088 */
	3, /* 14 */
	10, 13, /* 15 */
	11, 10, 12, /* 17 */
	20, 25, /* 20 */
	24, /* 22 */
	40, /* 23 */
	28, 35, /* 24*/
	28, 34, /* 26 */
	28, 33, 12,/* 28 */
	21, 43, 44, 21, /* 31 */
	57, 62, 67 ,21 /*35*/
};

//sector size shift bits
#define _256K	18
#define _128K	17
#define _64K    16
#define _32K    15
#define _16K    14
#define _8K     13
#define _4K     12
#define _1K	10

const unsigned char ali_flash_sector_map[] =
{
          1, _16K,   2,  _8K,   1, _32K,  7, _64K,  /* 0 */
          16, _64K,   8, _64K,                       /* 4 */
          7, _64K,   1, _32K,   2,  _8K,  1, _16K,  /* 6 */
          8,  _8K,  15, _64K,                       /* 10 */
          32, _64K,  16, _64K,                       /* 12 */
          15, _64K,   8,  _8K,				/* 14 */
          128,  _4K, 128,  _4K, 128, _4K, 128,  _4K,  /* 16 */
          8, _8K,  193, _64K,  63, _64K,  8,  _8K,	/* 20 */
          128, _128K,  8,  _8K, 254, _64K,  8,  _8K,	/* 24 */
          2, _4K, 1, _8K, 1, _16K,1, _32K,  7, _64K,	/* 28 */
          16, _64K,   8, _64K,                          /* 33 */
          7, _64K,   1, _32K,   1,  _16K,  1, _8K, 2, _4K, /* 35 */
          255, _1K, 255, _1K,2, _1K,			/* 40 */
          64, _256K,						/* 43 */
          2, _4K, 1, _8K, 1, _16K, 1, _32K, 7, _64K,	/* 44 */
          32, _64K, 16, _64K, 8, _64K,			/* 49 */
          7, _64K, 1, _32K, 1, _16K, 1, _8K, 2, _4K, /* 52 */
          4, _8K, 1, _32K, 30, _64K, 1, _32K, 4, _8K, /* 57 */
          4, _8K, 1, _32K, 62, _64K, 1, _32K, 4, _8K, /* 62 */
          2,_4K,  1,_8K , 1,_16K, 1,_32K, 127,_64K, /*67*/
};



/* Copyright (c) 2009-2014 Micron Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/spi/spi.h>
#include <linux/mtd/spi-nand.h>
#include <linux/spi/flash.h>

enum spi_nand_device_variant {
       SPI_NAND_GENERIC,
       SPI_NAND_MT29F,
       SPI_NAND_GD5F,
};

/**
*  Default OOB area specification layout
*/
static struct nand_ecclayout micron_ecc_layout_64 = {
       .eccbytes = 32,
       .eccpos = {
                  8, 9, 10, 11, 12, 13, 14, 15,
                  24, 25, 26, 27, 28, 29, 30, 21,
                  40, 41, 42, 43, 44, 45, 46, 47,
                  56, 57, 58, 59, 60, 61, 62, 63},
       .oobavail = 30,
       .oobfree = {
               {.offset = 2,
                .length = 6},
               {.offset = 16,
                .length = 8},
               {.offset = 32,
                .length = 8},
               {.offset = 48,
                .length = 8}, }
};

static struct nand_ecclayout gd5f_ecc_layout_256 = {
       .eccbytes = 128,
       .eccpos = {
               128, 129, 130, 131, 132, 133, 134, 135,
               136, 137, 138, 139, 140, 141, 142, 143,
               144, 145, 146, 147, 148, 149, 150, 151,
               152, 153, 154, 155, 156, 157, 158, 159,
               160, 161, 162, 163, 164, 165, 166, 167,
               168, 169, 170, 171, 172, 173, 174, 175,
               176, 177, 178, 179, 180, 181, 182, 183,
               184, 185, 186, 187, 188, 189, 190, 191,
               192, 193, 194, 195, 196, 197, 198, 199,
               200, 201, 202, 203, 204, 205, 206, 207,
               208, 209, 210, 211, 212, 213, 214, 215,
               216, 217, 218, 219, 220, 221, 222, 223,
               224, 225, 226, 227, 228, 229, 230, 231,
               232, 233, 234, 235, 236, 237, 238, 239,
               240, 241, 242, 243, 244, 245, 246, 247,
               248, 249, 250, 251, 252, 253, 254, 255
       },
       .oobavail = 127,
       .oobfree = { {1, 127} }
};

static struct nand_ecclayout gd5f_ecc_layout_128 = {
       .eccbytes = 64,
       .eccpos = {
               64, 65, 66, 67, 68, 69, 70, 72,
               72, 73, 74, 75, 76, 77, 78, 79,
               80, 81, 82, 83, 84, 85, 86, 87,
               88, 89, 90, 91, 92, 93, 94, 95,
               96, 97, 98, 99, 100, 101, 102, 103,
               104, 105, 106, 107, 108, 109, 110, 111,
               112, 113, 114, 115, 116, 117, 118, 119,
               120, 121, 122, 123, 124, 125, 126, 127,
       },
       .oobavail = 62,
       .oobfree = { {2, 63} }
};

static int spi_nand_gd5f_read_id(struct spi_nand_chip *chip, u8 *buf)
{
       struct spi_device *spi = chip->spi;
       struct spi_nand_cmd cmd = {0};

       cmd.cmd = SPINAND_CMD_READ_ID;
       cmd.n_addr = 1;
       cmd.addr[0] = 0x00;   
       cmd.n_rx = 2;
       cmd.rx_buf = buf;

       return spi_nand_send_cmd(spi, &cmd);
}

static int spi_nand_mt29f_read_id(struct spi_nand_chip *chip, u8 *buf)
{
       struct spi_device *spi = chip->spi;
       struct spi_nand_cmd cmd = {0};
       u8 dummy = 0;

       cmd.cmd = SPINAND_CMD_READ_ID;
       cmd.n_tx = 1;
       cmd.tx_buf = &dummy;
       cmd.n_rx = 2;
       cmd.rx_buf = buf;

       return spi_nand_send_cmd(spi, &cmd);
}

static void spi_nand_mt29f_ecc_status(unsigned int status,
                                       unsigned int *corrected,
                                       unsigned int *ecc_error)
{
       unsigned int ecc_status = (status >> SPI_NAND_MT29F_ECC_SHIFT) &
                                            SPI_NAND_MT29F_ECC_MASK;

       *ecc_error = (ecc_status == SPI_NAND_MT29F_ECC_UNCORR);
       if (*ecc_error == 0)
               *corrected = ecc_status;
}

static void spi_nand_gd5f_ecc_status(unsigned int status,
                                    unsigned int *corrected,
                                    unsigned int *ecc_error)
{
       unsigned int ecc_status = (status >> SPI_NAND_GD5F_ECC_SHIFT) &
                                            SPI_NAND_GD5F_ECC_MASK;

       *ecc_error = (ecc_status == SPI_NAND_GD5F_ECC_UNCORR);
       /*TODO fix corrected bits*/
       if (*ecc_error == 0)
               *corrected = ecc_status;
}

static int spi_nand_manufacture_init(struct spi_nand_chip *chip)
{
       switch (chip->mfr_id) {
       case SPINAND_MFR_MICRON:
               chip->get_ecc_status = spi_nand_mt29f_ecc_status;

               if (chip->page_spare_size == 64)
                       chip->ecclayout = &micron_ecc_layout_64;

               chip->bbt_options |= NAND_BBT_NO_OOB;
               break;
       case SPINAND_MFR_GIGADEVICE:
               chip->get_ecc_status = spi_nand_gd5f_ecc_status;
               chip->read_cache = spi_nand_read_from_cache_snor_protocol;
               chip->ecc_strength_ds = 8;
               chip->ecc_step_ds = chip->page_size >> 2;
               if (chip->page_spare_size == 128)
                       chip->ecclayout = &gd5f_ecc_layout_128;
               else if (chip->page_spare_size == 256)
                       chip->ecclayout = &gd5f_ecc_layout_256;

               break;
       default:
               break;
       }

       return 0;
}

static int spi_nand_device_probe(struct spi_device *spi)
{
       struct spi_nand_chip *chip;
       enum spi_nand_device_variant variant;
       struct mtd_info *mtd;
       struct mtd_part_parser_data ppdata;
       int ret;

       chip = kzalloc(sizeof(struct spi_nand_chip), GFP_KERNEL);
       if (!chip) {
               ret = -ENOMEM;
               goto err1;
       }
       chip->spi = spi;

       mtd = kzalloc(sizeof(struct mtd_info), GFP_KERNEL);
       if (!mtd) {
               ret = -ENOMEM;
               goto err2;
       }
       mtd->priv = chip;
       chip->mtd = mtd;
       spi_set_drvdata(spi, chip);
       /*
        * read ID command format might be different for different manufactory
        * such as, Micron SPI NAND need extra one dummy byte after perform
        * read ID command but Giga device don't need.
        *
        * So, specify manufactory of device in device tree is obligatory
        */
       variant = spi_get_device_id(spi)->driver_data;
       switch (variant) {
       case SPI_NAND_MT29F:
               chip->read_id = spi_nand_mt29f_read_id;
               break;
       case SPI_NAND_GD5F:
               chip->read_id = spi_nand_gd5f_read_id;
#if 1/* barryadd++ set read form cache *2*/		   
	       //chip->spi->mode |=  SPI_RX_DUAL;
	       chip->spi->mode |=  SPI_TX_QUAD | SPI_RX_QUAD;
#endif /* barryadd-- */
	       break;
       default:
               dev_err(&spi->dev, "unknown device\n");
               ret = -ENODEV;
               goto err3;
       }

       ret = spi_nand_scan_ident(mtd);
       if (ret)
               goto err3;

       spi_nand_manufacture_init(chip);

       ret = spi_nand_scan_tail(mtd);
       if (ret)
               goto err4;

       ppdata.of_node = chip->spi->dev.of_node;

       ret = mtd_device_parse_register(mtd, NULL, &ppdata, NULL, 0);
       if (!ret)
               return 0;

       spi_nand_scan_tail_release(mtd);
err4:
       spi_nand_scan_ident_release(mtd);
err3:
       kfree(mtd);
err2:
       kfree(chip);
err1:
       return ret;
}


int spi_nand_device_remove(struct spi_device *spi)
{
       struct spi_nand_chip *chip = spi_get_drvdata(spi);
       struct mtd_info *mtd = chip->mtd;

       spi_nand_release(mtd);
       kfree(mtd);
       kfree(chip);

       return 0;
}


const struct spi_device_id spi_nand_id_table[] = {
       { "spi-nand", SPI_NAND_GENERIC },
       { "mt29f", SPI_NAND_MT29F },
       { "gd5f", SPI_NAND_GD5F },
       { },
};
MODULE_DEVICE_TABLE(spi, spi_nand_id_table);

static struct spi_driver spi_nand_device_driver = {
       .driver = {
               .name   = "spi_nand_device",
               .owner  = THIS_MODULE,
       },
       .id_table = spi_nand_id_table,
       .probe  = spi_nand_device_probe,
       .remove = spi_nand_device_remove,
};

#if 0 //Barry++
module_spi_driver(spi_nand_device_driver);
#else

static struct flash_platform_data ali_spi_nand_data = {
	.name = "gd5f",//"mt29f",
};

static struct spi_board_info ali_spi_board_info __initdata = {
		/* the modalias must be the same as spi device driver name */
		.modalias = "gd5f",//"mt29f", /* Name of spi_driver for this device */
		.max_speed_hz = 25000000,
		.bus_num = 1,
		.chip_select = 1,
		.platform_data = &ali_spi_nand_data,
		.controller_data = NULL,
		.mode = SPI_MODE_0,
};

static int spi_nand_device_init(void)
{	
	spi_register_board_info(&ali_spi_board_info, 1);
	return spi_register_driver(&spi_nand_device_driver);
}

static void spi_nand_device_exit(void)
{
	spi_unregister_driver(&spi_nand_device_driver);
}
module_init(spi_nand_device_init);
module_exit(spi_nand_device_exit);

#endif //Barry--

MODULE_DESCRIPTION("SPI NAND device");
MODULE_AUTHOR("Peter Pan<peterpand...@micron.com>");
MODULE_AUTHOR("Ezequiel Garcia <ezequiel.gar...@imgtec.com>");
MODULE_LICENSE("GPL v2");


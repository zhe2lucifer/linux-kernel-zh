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

#include <linux/platform_device.h>
#include <linux/err.h>

struct see_bus {
	struct platform_device *pdev;
	int    rcd;
	struct list_head clients;
	int see_fw_version;
};

int see_client_service_get(int service_id, struct see_service *service);
int see_fw_version_get(void);

int init_see_connection(struct platform_device *pdev);

int get_rpc_bus_id(int rcd);

struct see_service *open_see_service(struct see_bus *bus, int service_id);

void close_see_service(struct see_service *srv);

/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 * examples/lwm2m/lwm2m_main.c
 *
 *   Copyright (C) 2016 SAMSUNG ELECTRONICS CO., LTD. All rights reserved.
 *   Author: Jihun Ahn <jhun.ahn@samsung.com>
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <tinyara/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "lwm2mclient.h"

#define AKC_UUID_LEN    32

static object_security_server_t akc_server = {
    "coaps+tcp://coaps-api.artik.cloud:5689", /* serverUri */
    "<Artik Cloud device ID>",                /* pskId : DEVICE ID */
    "<Artik Cloud device token>",             /* psk : DEVICE TOKEN */
    "<Artik Cloud device ID>",                /* name : DEVICE ID */
    30,                                       /* lifetime */
    0,                                        /* battery */
    123,                                      /* serverId */
    true,                                     /* verifyCert */
    0                                         /* localPort */
};

static object_device_t default_device = {
    "SAMSUNG",                /* PRV_MANUFACTURER */
    "A053", /* PRV_MODEL_NUMBER */
    "345000123",              /* PRV_SERIAL_NUMBER */
    "1.0",                    /* PRV_FIRMWARE_VERSION */
    1,                        /* PRV_POWER_SOURCE_1 */
    5,                        /* PRV_POWER_SOURCE_2 */
    3800,                     /* PRV_POWER_VOLTAGE_1 */
    5000,                     /* PRV_POWER_VOLTAGE_2 */
    125,                      /* PRV_POWER_CURRENT_1 */
    900,                      /* PRV_POWER_CURRENT_2 */
    100,                      /* PRV_BATTERY_LEVEL */
    15,                       /* PRV_MEMORY_FREE */
    "Europe/Paris",           /* PRV_TIME_ZONE */
    "+02:00",                 /* PRV_UTC_OFFSET */
    "U",                      /* PRV_BINDING_MODE */
    "DeviceType C SDK",       /* PRV_DEVICE_TYPE */
    "Hardware C SDK",         /* PRV_HARDWARE_VERSION */
    "Software C SDK",         /* PRV_SOFTWARE_VERSION */
    6,                        /* PRV_BATTERY_STATUS */
    128                       /* PRV_MEMORY_TOTAL */
};

static object_firmware_t default_firmware ={
    false,         /* SUPPORTED */
    "PKG Name",    /* PKG_NAME */
    "PKG Version", /* PKG_VERSION */
};

static bool quit = false;

static void usage(void)
{
    fprintf(stdout, "Usage: akc_client [options]\r\n");
    fprintf(stdout, "\t-d <device ID> : AKC device ID\r\n");
    fprintf(stdout, "\t-t <device token> : AKC device token\r\n");
    fprintf(stdout, "\t-n : don't verify SSL certificate\r\n");
    fprintf(stdout, "\t-h : display help\r\n");
}

static void on_reboot(void *param, void *extra)
{
    fprintf(stdout, "REBOOT\r\n");
}

static void on_firmware_update(void *param, void *extra)
{
    fprintf(stdout, "FIRMWARE UPDATE\r\n");
}

static void on_factory_reset(void *param, void *extra)
{
    fprintf(stdout, "FACTORY RESET\r\n");
}

static void on_resource_changed(void *param, void *extra)
{
    client_handle_t *client = (client_handle_t*)param;
    lwm2m_resource_t *params = (lwm2m_resource_t*)extra;

    fprintf(stdout, "Resource Changed: %s\r\n", params->uri);

    if (!strncmp(params->uri, LWM2M_URI_FIRMWARE_PACKAGE_URI, LWM2M_MAX_URI_LEN))
    {
        char *filename;
        lwm2m_resource_t state;

        strncpy(state.uri, LWM2M_URI_FIRMWARE_STATE, strlen(LWM2M_URI_FIRMWARE_STATE));
        state.length = strlen(LWM2M_FIRMWARE_STATE_DOWNLOADING);
        state.buffer = (uint8_t*)strndup(LWM2M_FIRMWARE_STATE_DOWNLOADING, state.length);

        /* Change state */
        lwm2m_write_resource(client, &state);
        free(state.buffer);

        /*
         * Download the package and update status at each step
         */
        filename = strndup((char*)params->buffer, params->length);
        fprintf(stdout, "Downloading: %s\r\n", filename);
        free(filename);
    }
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int lwm2m_main(int argc, char *argv[])
#endif
{
	int opt;
	object_container_t init_val_ob;
	client_handle_t *client = NULL;

	fprintf(stdout, "%s %d: hello world\n", __func__, __LINE__);

	while ((opt = getopt(argc, argv, "d:t:nh")) != -1) {
		switch (opt) {
		case 'd':
			if (strlen(optarg) != AKC_UUID_LEN)
			{
				fprintf(stderr, "Wrong device ID parameter\r\n");
				usage();
				return -1;
			}

			strncpy(akc_server.bsPskId, optarg, AKC_UUID_LEN);
			strncpy(akc_server.client_name, optarg, AKC_UUID_LEN);
			break;
		case 't':
			if (strlen(optarg) != AKC_UUID_LEN)
			{
				fprintf(stderr, "Wrong device token parameter\r\n");
				usage();
				return -1;
			}

			strncpy(akc_server.psk, optarg, AKC_UUID_LEN);
			break;
		case 'n':
			akc_server.verifyCert = false;
			break;
		case 'h':
			usage();
			return 0;
		default:
			usage();
			return -1;
		}
	}

	memset(&init_val_ob, 0, sizeof(init_val_ob));
	init_val_ob.server = &akc_server;
	init_val_ob.device = &default_device;
	init_val_ob.firmware = &default_firmware;

	client = lwm2m_client_start(&init_val_ob);
	if (!client)
	{
		fprintf(stderr, "Failed to start client\n");
		return -1;
	}

	cmdline_init(client);

	lwm2m_register_callback(client, LWM2M_EXE_FACTORY_RESET, on_factory_reset,
			(void*) client);
	lwm2m_register_callback(client, LWM2M_EXE_DEVICE_REBOOT, on_reboot,
			(void*) client);
	lwm2m_register_callback(client, LWM2M_EXE_FIRMWARE_UPDATE,
			on_firmware_update, (void*) client);
	lwm2m_register_callback(client, LWM2M_NOTIFY_RESOURCE_CHANGED,
			on_resource_changed, (void*) client);

	while (!quit)
	{
		int ret = lwm2m_client_service(client, 100);
		if ((ret == LWM2M_CLIENT_QUIT) || (ret == LWM2M_CLIENT_ERROR))
			break;

		ret = cmdline_process(ret);
		if ((ret == LWM2M_CLIENT_QUIT) || (ret == LWM2M_CLIENT_ERROR))
		break;
	}

	lwm2m_client_stop(client);

	return 0;
}

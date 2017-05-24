#include <stdio.h>
#include <string.h>
#include <sys/boardctl.h>

#include <artik_module.h>
#include <artik_cloud.h>
#include <artik_lwm2m.h>

#include "command.h"

#define	FAIL_AND_EXIT(x)  { fprintf(stderr, x); usage(argv[1], cloud_commands); ret = -1; goto exit; }

#define UUID_MAX_LEN                64
#define LWM2M_RES_DEVICE_REBOOT     "/3/0/4"

static int device_command(int argc, char *argv[]);
static int devices_command(int argc, char *argv[]);
static int message_command(int argc, char *argv[]);
static int connect_command(int argc, char *argv[]);
static int disconnect_command(int argc, char *argv[]);
static int send_command(int argc, char *argv[]);
static int sdr_command(int argc, char *argv[]);
static int dm_command(int argc, char *argv[]);

static artik_websocket_handle ws_handle = NULL;
static artik_lwm2m_config *g_dm_config = NULL;
static artik_lwm2m_handle g_dm_client = NULL;

const struct command cloud_commands[] = {
	{ "device", "device <token> <device id> [<properties>]", device_command },
	{ "devices", "<token> <user id> [<count> <offset> <properties>]", devices_command },
	{ "message", "<token> <device id> <message>", message_command },
	{ "connect", "<token> <device id> [use_se]", connect_command },
	{ "disconnect", "", disconnect_command },
	{ "send", "<message>", send_command },
	{ "sdr", "start|status|complete <dtid> <vdid>|<regid>|<regid> <nonce>", sdr_command },
	{ "dm", "connect|read|change|disconnect <token> <did>|<uri>|<uri> <value>", dm_command },
	{ "", "", NULL }
};

static void websocket_rx_callback(void *user_data, void *result)
{
	if (result) {
		fprintf(stderr, "RX: %s\n", (char*)result);
		free(result);
	}
}

static int device_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	bool properties = false;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 5) FAIL_AND_EXIT("Wrong number of arguments\n")

	/* Parse optional arguments */
	if (argc > 5) {
		properties = (atoi(argv[5]) > 0);
	}

	err = cloud->get_device(argv[3], argv[4], properties, &response);
	if (err != S_OK) FAIL_AND_EXIT("Failed to get user devices\n")

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int devices_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;
	int count = 10;
	bool properties = false;
	int offset = 0;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 5) FAIL_AND_EXIT("Wrong number of arguments\n")

	/* Parse optional arguments */
	if (argc > 5) {
		count = atoi(argv[5]);
		if (argc > 6) {
			offset = atoi(argv[6]);
			if (argc > 7)
				properties = (atoi(argv[7]) > 0);
		}
	}

	err = cloud->get_user_devices(argv[3], count, properties, 0, argv[4], &response);
	if (err != S_OK) FAIL_AND_EXIT("Failed to get user devices\n")

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int message_command(int argc, char *argv[])
{
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	int ret = 0;
	char *response = NULL;
	artik_error err = S_OK;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	/* Check number of arguments */
	if (argc < 6) FAIL_AND_EXIT("Wrong number of arguments\n")

	err = cloud->send_message(argv[3], argv[4], argv[5], &response);
	if (err != S_OK) FAIL_AND_EXIT("Failed to send message\n")

	if (response) {
		fprintf(stdout, "Response: %s\n", response);
		free(response);
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int connect_command(int argc, char *argv[])
{
	int ret = 0;
	artik_error err = S_OK;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	bool use_se = false;

	/* Check number of arguments */
	if (argc < 5) {
		fprintf(stderr, "Wrong number of arguments\n");
		ret = -1;
		goto exit;
	}

	if ((argc == 6) && !strncmp(argv[5], "use_se", strlen("use_se")))
		use_se = true;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		ret = -1;
		goto exit;
	}

	err = cloud->websocket_open_stream(&ws_handle, argv[3], argv[4], use_se);
	if (err != S_OK) {
		fprintf(stderr, "Failed to connect websocket\n");
		ws_handle = NULL;
		ret = -1;
		goto exit;
	}

	err = cloud->websocket_set_receive_callback(ws_handle, websocket_rx_callback, NULL);
	if (err != S_OK) {
		fprintf(stderr, "Failed to set websocket receive callback\n");
		ws_handle = NULL;
		ret = -1;
		goto exit;
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static int disconnect_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	 }

	if (!ws_handle) {
	fprintf(stderr, "Websocket to cloud is not connected\n");
	goto exit;
	}

	cloud->websocket_close_stream(ws_handle);

exit:
	artik_release_api_module(cloud);

	return ret;
}

static int send_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = (artik_cloud_module *)artik_request_api_module("cloud");
	artik_error err = S_OK;

	if (!cloud) {
		fprintf(stderr, "Failed to request cloud module\n");
		return -1;
	}

	if (!ws_handle) {
		fprintf(stderr, "Websocket to cloud is not connected\n");
		goto exit;
	}

	/* Check number of arguments */
	if (argc < 4) FAIL_AND_EXIT("Wrong number of arguments\n")

	fprintf(stderr, "Sending %s\n", argv[3]);

	err = cloud->websocket_send_message(ws_handle, argv[3]);
	if (err != S_OK) FAIL_AND_EXIT("Failed to send message to cloud through websocket\n")

exit:
	artik_release_api_module(cloud);

	return ret;
}

static int sdr_command(int argc, char *argv[])
{
	int ret = 0;
	artik_cloud_module *cloud = NULL;

	if (!strcmp(argv[3], "start")) {
		char *response = NULL;

		if (argc < 6) FAIL_AND_EXIT("Wrong number of arguments\n")

			cloud = (artik_cloud_module *)artik_request_api_module("cloud");
			if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

			cloud->sdr_start_registration(argv[4], argv[5], &response);

			if (response) {
				fprintf(stdout, "Response: %s\n", response);
				free(response);
			}
	} else if (!strcmp(argv[3], "status")) {
		char *response = NULL;

		if (argc < 5) FAIL_AND_EXIT("Wrong number of arguments\n")

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

		cloud->sdr_registration_status(argv[4], &response);

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
	} else if (!strcmp(argv[3], "complete")) {
		char *response = NULL;

		if (argc < 6) FAIL_AND_EXIT("Wrong number of arguments\n")

		cloud = (artik_cloud_module *)artik_request_api_module("cloud");
		if (!cloud) FAIL_AND_EXIT("Failed to request cloud module\n")

		cloud->sdr_complete_registration(argv[4], argv[5], &response);

		if (response) {
			fprintf(stdout, "Response: %s\n", response);
			free(response);
		}
	} else {
		fprintf(stdout, "Unknown command: sdr %s\n", argv[3]);
		usage(argv[1], cloud_commands);
		ret = -1;
		goto exit;
	}

exit:
	if (cloud)
		artik_release_api_module(cloud);

	return ret;
}

static void dm_on_error(void *data, void *user_data)
{
	artik_error err = (artik_error)data;

	fprintf(stderr, "LWM2M error: %s\n", error_msg(err));
}

static pthread_addr_t delayed_reboot(pthread_addr_t arg)
{
	sleep(3);
	boardctl(BOARDIOC_RESET, EXIT_SUCCESS);

	return NULL;
}

static void dm_on_execute_resource(void *data, void *user_data)
{
	char *uri = (char *)data;

	fprintf(stderr, "LWM2M resource execute: %s\n", uri);

	if (!strncmp(uri, LWM2M_RES_DEVICE_REBOOT, strlen(LWM2M_RES_DEVICE_REBOOT))) {
		pthread_t tid;
		pthread_create(&tid, NULL, delayed_reboot, NULL);
		pthread_detach(tid);
		fprintf(stderr, "Rebooting in 3 seconds\n");
	}
}

static void dm_on_changed_resource(void *data, void *user_data)
{
	char *uri = (char *)data;

	fprintf(stderr, "LWM2M resource changed: %s\n", uri);
}

static int dm_command(int argc, char *argv[])
{
	int ret = 0;
	artik_lwm2m_module *lwm2m = NULL;

	if (!strcmp(argv[3], "connect")) {

		if (g_dm_client) {
			fprintf(stderr, "DM Client is already started, stop it first\n");
			ret = -1;
			goto exit;
		}

		if (argc < 6) FAIL_AND_EXIT("Wrong number of arguments\n")

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) FAIL_AND_EXIT("Failed to request lwm2m module\n")

		g_dm_config = zalloc(sizeof(artik_lwm2m_config));
		if (!g_dm_config) {
			fprintf(stderr, "Failed to allocate memory for DM config\n");
			ret = -1;
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) {
			fprintf(stderr, "Failed to request LWM2M module\n");
			free(g_dm_config);
			ret = -1;
			goto exit;
		}

		g_dm_config->server_id = 123;
		g_dm_config->server_uri = "coaps+tcp://coaps-api.artik.cloud:5689";
		g_dm_config->lifetime = 30;
		g_dm_config->name = strndup(argv[5], UUID_MAX_LEN);
		g_dm_config->tls_psk_identity = g_dm_config->name;
		g_dm_config->tls_psk_key = strndup(argv[4], UUID_MAX_LEN);
		g_dm_config->objects[ARTIK_LWM2M_OBJECT_DEVICE] =
		lwm2m->create_device_object("Samsung", "ARTIK053", "1234567890", "1.0",
				"1.0", "1.0", "A053", 0, 5000, 1500, 100, 1000000, 200000,
				"Europe/Paris", "+01:00", "U");
		if (!g_dm_config->objects[ARTIK_LWM2M_OBJECT_DEVICE]) {
			fprintf(stderr, "Failed to allocate memory for object device.");
			free(g_dm_config);
			ret = -1;
			goto exit;
		}

		ret = lwm2m->client_connect(&g_dm_client, g_dm_config);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to connect to the DM server (%d)\n", ret);
			lwm2m->free_object(g_dm_config->objects[ARTIK_LWM2M_OBJECT_DEVICE]);
			free(g_dm_config);
			ret = -1;
			goto exit;
		}

		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_ERROR,
				dm_on_error, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_RESOURCE_EXECUTE,
				dm_on_execute_resource, (void *)g_dm_client);
		lwm2m->set_callback(g_dm_client, ARTIK_LWM2M_EVENT_RESOURCE_CHANGED,
				dm_on_changed_resource, (void *)g_dm_client);

	} else if (!strcmp(argv[3], "disconnect")) {

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) FAIL_AND_EXIT("Failed to request lwm2m module\n")

		lwm2m->client_disconnect(g_dm_client);
		lwm2m->free_object(g_dm_config->objects[ARTIK_LWM2M_OBJECT_DEVICE]);
		free(g_dm_config->name);
		free(g_dm_config->tls_psk_key);
		free(g_dm_config);

		g_dm_client = NULL;
		g_dm_config = NULL;

	} else if (!strcmp(argv[3], "read")) {
		char value[256]= {0};
		int len = 256;

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		if (argc < 5) FAIL_AND_EXIT("Wrong number of arguments\n")

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) FAIL_AND_EXIT("Failed to request lwm2m module\n")

		ret = lwm2m->client_read_resource(g_dm_client, argv[4], (unsigned char*)value, &len);
		if (ret != S_OK) {
			fprintf(stderr, "Failed to read %s\n", argv[4]);
			ret = -1;
			goto exit;
		}

		printf("URI: %s - Value: %s\n", argv[4], value);

	} else if (!strcmp(argv[3], "change")) {

		if (!g_dm_client) {
			fprintf(stderr, "DM Client is not started\n");
			ret = -1;
			goto exit;
		}

		if (argc < 6) FAIL_AND_EXIT("Wrong number of arguments\n")

		lwm2m = (artik_lwm2m_module *)artik_request_api_module("lwm2m");
		if (!lwm2m) FAIL_AND_EXIT("Failed to request lwm2m module\n")

		ret = lwm2m->client_write_resource(g_dm_client, argv[4], (unsigned char*)argv[5],
				strlen(argv[5]));
		if (ret != S_OK) {
			fprintf(stderr, "Failed to write %s", argv[4]);
			ret = -1;
			goto exit;
		}

	} else {
		fprintf(stdout, "Unknown command: dm %s\n", argv[3]);
		usage(argv[1], cloud_commands);
		ret = -1;
		goto exit;
	}

exit:
	if (lwm2m)
		artik_release_api_module(lwm2m);

	return ret;
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int cloud_main(int argc, char *argv[])
#endif
{
	return commands_parser(argc, argv, cloud_commands);
}

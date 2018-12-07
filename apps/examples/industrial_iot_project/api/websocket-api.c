#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_websocket.h>

#define	FAIL_AND_EXIT(x)		{ fprintf(stderr, x); usage(); ret = -1; goto exit; }

static artik_websocket_handle g_handle = NULL;
static artik_websocket_module *g_websocket = NULL;

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\twebsocket connect <uri>\n");
    fprintf(stderr, "\t\t<uri> - example: wss://echo.websocket.org/\n");
    fprintf(stderr, "\twebsocket disconnect\n");
    fprintf(stderr, "\twebsocket send <message>\n");
}

static void websocket_rx_callback(void *user_data, void *result)
{
    if (result) {
        fprintf(stderr, "RX: %s\n", (char*)result);
        free(result);
    }
}

static void websocket_connection_callback(void *user_data, void *result)
{
    artik_websocket_connection_state state = (artik_websocket_connection_state)result;

    if (state == ARTIK_WEBSOCKET_CLOSED)
        fprintf(stderr, "Websocket connection has been closed\n");
    else if (state == ARTIK_WEBSOCKET_CONNECTED)
        fprintf(stderr, "Websocket is connected\n");
}

static void websocket_connect(char *uri)
{
    artik_websocket_config config;
    char *tmp = NULL;
    artik_error ret = S_OK;
    int idx = 0;

    g_websocket = (artik_websocket_module *)artik_request_api_module("websocket");
    if (!g_websocket) {
        fprintf(stderr, "Websocket module is not available\n");
        return;
    }

    /* Parse URI to figure out config options */
    memset(&config, 0, sizeof(artik_websocket_config));
    if (!strncmp(uri, "ws://", strlen("ws://"))) {
        config.port = 80;
        config.use_tls = false;
        idx += strlen("ws://");
    } else if (!strncmp(uri, "wss://", strlen("wss://"))) {
        config.port = 443;
        config.use_tls = true;
        config.ssl_config.verify_cert = ARTIK_SSL_VERIFY_NONE;
        idx += strlen("wss://");
    } else {
        fprintf(stderr, "Malformed URI: should start with 'ws://' or 'wss://'\n");
        usage();
        return;
    }

    tmp = strchr(uri + idx, '/');
    if (!tmp) {
        fprintf(stderr, "Malformed URI: missing path after hostname\n");
        usage();
        return;
    }

    config.host = strndup(uri + idx, tmp - (uri + idx));
    config.uri = strndup(tmp, strlen(tmp));

    ret = g_websocket->websocket_request(&g_handle, &config);
    if (ret != S_OK) {
        fprintf(stderr, "Failed to request websocket (ret=%d)\n", ret);
        goto exit;
    }

    ret = g_websocket->websocket_open_stream(g_handle);
    if (ret != S_OK) {
        fprintf(stderr, "Failed to open websocket (ret=%d)\n", ret);
        goto exit;
    }

    ret = g_websocket->websocket_set_receive_callback(g_handle, websocket_rx_callback, NULL);
    if (ret != S_OK) {
        fprintf(stderr, "Failed to register rx callback (ret=%d)\n", ret);
        goto exit;
    }

    ret = g_websocket->websocket_set_connection_callback(g_handle, websocket_connection_callback, NULL);
    if (ret != S_OK) {
        fprintf(stderr, "Failed to register connection callback (ret=%d)\n", ret);
        goto exit;
    }

 exit:
    if (config.host)
        free(config.host);
    if (config.uri)
        free(config.uri);
}

static void websocket_disconnect(void)
{
    if (!g_websocket || !g_handle) {
        fprintf(stderr, "Websocket is not connected\n");
        return;
    }

    g_websocket->websocket_close_stream(g_handle);

    artik_release_api_module(g_websocket);
    g_websocket = NULL;
    g_handle = NULL;
}

static void websocket_send(char *message)
{
    if (!g_websocket || !g_handle) {
        fprintf(stderr, "Websocket is not connected\n");
        return;
    }

    g_websocket->websocket_write_stream(g_handle, message);
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int websocket_main(int argc, char *argv[])
#endif
{
    if (argc < 2) {
        fprintf(stderr, "Not enough arguments\n");
        usage();
        return -1;
    }

    if (!strncmp(argv[1], "connect", strlen("connect"))) {
        if (argc < 3) {
            fprintf(stderr, "Missing uri parameter\n");
            usage();
            return -1;
        }

        websocket_connect(argv[2]);

    } else if (!strncmp(argv[1], "disconnect", strlen("disconnect"))) {

        websocket_disconnect();

    } else if (!strncmp(argv[1], "send", strlen("send"))) {

        if (argc < 3) {
            fprintf(stderr, "Missing message parameter\n");
            usage();
            return -1;
        }

        websocket_send(argv[2]);

    } else {
        fprintf(stderr, "Wrong arguments\n");
        usage();
        return -1;
    }

    return 0;
}

#include <stdio.h>
#include <string.h>

#include <artik_module.h>
#include <artik_http.h>

static void usage(void)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\thttp get <url>\n");
    fprintf(stderr, "\thttp post <url> <body>\n");
    fprintf(stderr, "\thttp put <url> <body>\n");
    fprintf(stderr, "\thttp delete <url>\n");
}

static int http_get(const char* url)
{
	artik_http_module *http = (artik_http_module *)artik_request_api_module("http");
	artik_error ret = S_OK;
	char *response = NULL;
	int status = 0;
	artik_http_headers headers;
	artik_http_header_field fields[] = {
		{"Connect", "close"},
		{"User-Agent", "Artik browser"},
		{"Accept-Language", "en-US,en;q=0.8"},
	};

	headers.fields = fields;
	headers.num_fields = sizeof(fields) / sizeof(fields[0]);

	ret = http->get(url, &headers, &response, &status, NULL);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to get %s (err:%s)\n", url, error_msg(ret));
		goto exit;
	}

	if (response) {
		fprintf(stderr, "HTTP %d - %s\n", status, response);
		free(response);
	}

exit:
	artik_release_api_module(http);
	return (ret == S_OK);
}

static int http_post(const char *url, const char *body)
{
	artik_http_module *http = (artik_http_module *)artik_request_api_module("http");
	artik_error ret = S_OK;
	char *response = NULL;
	int status = 0;
	artik_http_headers headers;
	artik_http_header_field fields[] = {
		{"Connect", "close"},
		{"User-Agent", "Artik browser"},
		{"Accept-Language", "en-US,en;q=0.8"},
	};

	headers.fields = fields;
	headers.num_fields = sizeof(fields) / sizeof(fields[0]);

	ret = http->post(url, &headers, body, &response, &status, NULL);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to get %s (err:%s)\n", url, error_msg(ret));
		goto exit;
	}

	if (response) {
		fprintf(stderr, "HTTP %d - %s\n", status, response);
		free(response);
	}

exit:
	artik_release_api_module(http);
	return (ret == S_OK);
}

static int http_put(const char *url, const char *body)
{
	artik_http_module *http = (artik_http_module *)artik_request_api_module("http");
	artik_error ret = S_OK;
	char *response = NULL;
	int status = 0;
	artik_http_headers headers;
	artik_http_header_field fields[] = {
		{"Connect", "close"},
		{"User-Agent", "Artik browser"},
		{"Accept-Language", "en-US,en;q=0.8"},
	};

	headers.fields = fields;
	headers.num_fields = sizeof(fields) / sizeof(fields[0]);

	ret = http->put(url, &headers, body, &response, &status, NULL);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to get %s (err:%s)\n", url, error_msg(ret));
		goto exit;
	}

	if (response) {
		fprintf(stderr, "HTTP %d - %s\n", status, response);
		free(response);
	}

exit:
	artik_release_api_module(http);
	return (ret == S_OK);
}

static int http_delete(const char *url)
{
	artik_http_module *http = (artik_http_module *)artik_request_api_module("http");
	artik_error ret = S_OK;
	char *response = NULL;
	int status = 0;
	artik_http_headers headers;
	artik_http_header_field fields[] = {
		{"Connect", "close"},
		{"User-Agent", "Artik browser"},
		{"Accept-Language", "en-US,en;q=0.8"},
	};

	headers.fields = fields;
	headers.num_fields = sizeof(fields) / sizeof(fields[0]);

	ret = http->del(url, &headers, &response, &status, NULL);
	if (ret != S_OK) {
		fprintf(stderr, "Failed to get %s (err:%s)\n", url, error_msg(ret));
		goto exit;
	}

	if (response) {
		fprintf(stderr, "HTTP %d - %s\n", status, response);
		free(response);
	}

exit:
	artik_release_api_module(http);
	return (ret == S_OK);
}

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int http_main(int argc, char *argv[])
#endif
{
	int ret = 0;

    if (argc < 3)
		goto error;

    if (!strncmp(argv[1], "get", 8)) {
        ret = http_get(argv[2]);
    } else if (!strncmp(argv[1], "post", 8)) {
        if (argc < 4)
            goto error;

        ret = http_post(argv[2], argv[3]);
    } else if (!strncmp(argv[1], "put", 8)) {
        if (argc < 4)
            goto error;

        ret = http_put(argv[2], argv[3]);
    } else if (!strncmp(argv[1], "delete", 8)) {
        ret = http_delete(argv[2]);
    } else {
		goto error;
    }

    return ret;

error:
    usage();
	return -1;
}

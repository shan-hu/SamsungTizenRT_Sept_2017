#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <artik_error.h>

#include "industrial_iot_utils.h"

#define TEST_DEVICE 0

#if (defined TEST_DEVICE && TEST_DEVICE)
    // TEST_DEVICE_053
    char did[] = "7245dcffd8e64eb393ef7110dd31dcab";
    char dtoken[] = "1b7eb06e7bed4767ae0f8f23a4a3f089";
    char dtypeid[] = "dt57c1d1cfcbd641b9b3ceb36f70e7b4c6";
    char dpin[] = "dt57c1d1cfcbd641b9b3ceb36f70e7b4c6";
#else
    // iiot_Artik053_1
    char did[] = "3ccdc206f13e4b97949b9baaca7d9117";
    char dtoken[] = "b55014092b5f4411abf620409271e98c";
    char dtypeid[] = "dt57c1d1cfcbd641b9b3ceb36f70e7b4c6";
    char dpin[] = "dt57c1d1cfcbd641b9b3ceb36f70e7b4c6";
#endif

// "ID8_HP5" causes interference on the vibration reading!
char ssid[] = "ID8_HP1";
char psk[] = "lab95134";

//char ssid[] = "ID8";
//char psk[] = "lab95134";
//char ssid[] = "ARTIK-S";
//char psk[] = "@rtikLS8";
//char ssid[] = "SPY500";
//char psk[] = "Cwhisper810528";
//char ssid[] = "ARTIK";
//char psk[] = "ARTIK-ACE-123";

static char config_file[16] = "/mnt/config";

static void PrintConfig(void)
{
    printf("Wifi:\n");
    printf("\tssid: %s\n", wifi_config.ssid);
    printf("\tpassphrase: %s\n", wifi_config.passphrase);
    printf("\tsecure: %s\n", wifi_config.secure ? "true" : "false");

    printf("Cloud:\n");
    printf("\tdevice_id: %s\n", cloud_config.device_id);
    printf("\tdevice_token: %s\n", cloud_config.device_token);
    printf("\tdevice_type_id: %s\n", cloud_config.device_type_id);
    printf("\tdevice_pin: %s\n", cloud_config.device_pin);
}

artik_error InitConfiguration(void)
{/*
    PrintConfig();
    struct stat st;
    int ret = 0;
    int fd = 0;
    artik_error err = S_OK;

    ret = fs_initiate("/dev/smart1", "smartfs");
    if (ret < 0) {
        printf("fs_initiate error!(%d)\n", ret);
        err = E_ACCESS_DENIED;
        goto exit;
    }

    stat(config_file, &st);

    if (st.st_size != (sizeof(wifi_config) + sizeof(cloud_config)))
    {
        printf("Invalid configuration, creating default one\n");

        err = ResetConfiguration();
    } else {

        fd = open(config_file, O_RDONLY);
        if (fd == -1) {
            printf("Unable to open configuration file (errno=%d)\n", errno);
            err = E_ACCESS_DENIED;
            goto exit;
        }

        lseek(fd, 0, SEEK_SET);
        read(fd, (void *)&wifi_config, sizeof(wifi_config));
        read(fd, (void *)&cloud_config, sizeof(cloud_config));

        close(fd);
    }

exit:
    //PrintConfig();

    return err;*/
    //strncpy(wifi_config.ssid, "ARTIK-S", sizeof(wifi_config.ssid));
    //strncpy(wifi_config.passphrase, "@rtikLS8", sizeof(wifi_config.passphrase));
    //strncpy(wifi_config.ssid, "SPY500", sizeof(wifi_config.ssid));
    //strncpy(wifi_config.passphrase, "Cwhisper810528", sizeof(wifi_config.passphrase)); 
    strncpy(wifi_config.ssid, ssid, sizeof(wifi_config.ssid));
    strncpy(wifi_config.passphrase, psk, sizeof(wifi_config.passphrase));
    wifi_config.secure = 1;

    strncpy(cloud_config.device_id, did, sizeof(cloud_config.device_id));
    strncpy(cloud_config.device_token, dtoken, sizeof(cloud_config.device_token));
    strncpy(cloud_config.device_type_id, dtypeid, sizeof(cloud_config.device_type_id));
    strncpy(cloud_config.device_pin, dpin, sizeof(cloud_config.device_pin));

    return S_OK;
}

artik_error SaveConfiguration(void)
{
    int ret = 0;
    int fd = 0;

    fd = open(config_file, O_WRONLY|O_CREAT);
    if (fd == -1) {
        printf("Unable to open configuration file (errno=%d)\n", errno);
        return E_ACCESS_DENIED;
    }

    lseek(fd, 0, SEEK_SET);
    ret = write(fd, (const void *)&wifi_config, sizeof(wifi_config));
    if (ret != sizeof(wifi_config))
        printf("Failed to write wifi config (%d - errno=%d)\n", ret, errno);
    ret = write(fd, (const void *)&cloud_config, sizeof(cloud_config));
    if (ret != sizeof(cloud_config))
        printf("Failed to write wifi config (%d - errno=%d)\n", ret, errno);

    close(fd);

    return S_OK;
}

artik_error ResetConfiguration(void)
{
    int ret = 0;
    int fd = 0;

    fd = open(config_file, O_WRONLY|O_CREAT);
    if (fd == -1) {
        printf("Unable to open configuration file (errno=%d)\n", errno);
        return E_ACCESS_DENIED;
    }

    WifiResetConfig();
    CloudResetConfig();

    lseek(fd, 0, SEEK_SET);
    ret = write(fd, (const void *)&wifi_config, sizeof(wifi_config));
    if (ret != sizeof(wifi_config))
        printf("Failed to write wifi config (%d - errno=%d)\n", ret, errno);
    ret = write(fd, (const void *)&cloud_config, sizeof(cloud_config));
    if (ret != sizeof(cloud_config))
        printf("Failed to write wifi config (%d - errno=%d)\n", ret, errno);

    close(fd);

    return S_OK;
}







/*===========================================================================
                       linuxÏµÍ³Í·ÎÄŒþ
===========================================================================*/
#ifdef WLAN_AT_API_APP_BUILD
    #include <unistd.h>
    #include <sys/wait.h>
#else
    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/delay.h>
    #include <linux/ctype.h>
    #include <linux/fs.h>
    #include <asm/uaccess.h>
    #include "wlan_if.h"
#endif /*WLAN_AT_API_APP_BUILD*/

/*===========================================================================
                       ÆœÌšÍ·ÎÄŒþ
===========================================================================*/
#include "wlan_at.h"
#include "wlan_utils.h"
#include "bsp_version.h"

/*===========================================================================
                       ºê¶šÒå
===========================================================================*/
#define WLAN_AT_SSID_SUPPORT    1           /* Ö§³ÖµÄSSID×éÊý */
#define WLAN_AT_KEY_SUPPORT     5           /* Ö§³ÖµÄ·Ö×éÊý */
#define WLAN_AT_MODE_SUPPORT    "2,3,4"   /* Ö§³ÖµÄÄ£Êœ(cw:0/a:1/b:2/g:3/n:4/ac:5) */
#define WLAN_AT_BAND_SUPPORT    "0,1"       /* Ö§³ÖµÄŽø¿í(20M:0/40M:1/80M:2/160M:3) */
#if defined(FEATURE_HUAWEI_MBB_RTL8189)
    #define WLAN_AT_TSELRF_SUPPORT  "0"           /* Ö§³Ö1*1µ¥ÌìÏß */
#elif defined(FEATURE_HUAWEI_MBB_RTL8192)
    #define WLAN_AT_TSELRF_SUPPORT  "0,1"         /* Ö§³Ö2*2Ë«ÌìÏß */
#endif

#define MAX_POW_INDEX    63           /* ×îŽó¹ŠÂÊindexÖµ */

/* ×îŽóÖ§³ÖÐÅµÀÊýÁ¿ */
#define WLAN_RTL_CHANNEL_24G_MAX (14)

/*WIFI¹ŠÂÊµÄÉÏÏÂÏÞ*/
#define WLAN_AT_WIFI_POWER_MIN           (-15)
#define WLAN_AT_WIFI_POWER_MAX           (30)

/*cmd×Ö·ûŽ®³€¶È*/
#define WIFI_CMD_MAX_SIZE   256
#define WIFI_AP_INTERFACE   "wlan0"

/* ¹ýCEÈÏÖ€£¬ÅäÖÃTXÕŒ¿Õ±È100% */
#define WLAN_AT_TX_FULL_DUTY_RATIO 255

/*WiFiÐŸÆ¬Ê¹ÄÜ¶ÏÑÔŒì²â*/
#define IS_CW_MODE(wifi_mode) ((AT_WIMODE_80211b == wifi_mode) ? AT_FEATURE_ENABLE : AT_FEATURE_DISABLE)

/*ÏòWiFiÐŸÆ¬ÏÂ·¢ÅäÖÃÃüÁî*/
#define WIFI_TEST_CMD(cmd) do { \
    if (WLAN_SUCCESS != wlan_run_cmd(cmd)) \
    { \
        PLAT_WLAN_INFO("Run CMD Error!!!"); \
        return AT_RETURN_FAILURE; \
    } \
    if (NULL != strstr(cmd, " down")) \
    { \
        PLAT_WLAN_INFO("RX TX Status Clear!!!"); \
        g_wlan_at_data.WiTX = AT_FEATURE_DISABLE; \
        g_wlan_at_data.WiRX.onoff = AT_FEATURE_DISABLE; \
    } \
}while(0)

/*Ïòµ¥°åÏÂ·¢shellÃüÁî*/
#define WIFI_SHELL_CMD(shell) do { \
    if (WLAN_SUCCESS != wlan_run_shell(shell)) \
    { \
        PLAT_WLAN_INFO("Run SHELL Error!!!"); \
        return AT_RETURN_FAILURE; \
    } \
}while(0)

/*WiFiÐŸÆ¬Ê¹ÄÜ¶ÏÑÔŒì²â*/
#define ASSERT_WiFi_OFF(ret) do { \
    if (AT_WIENABLE_OFF == g_wlan_at_data.WiEnable) \
    { \
        PLAT_WLAN_INFO("Exit on WiFi OFF"); \
        return ret; \
    } \
}while(0)


//custom
#define ASSERT_WiFi_INVALID(ret) { \
    /*if (0 != check_wifi_valid()) \
    { \
        PLAT_WLAN_INFO("check_wifi_valid failed!"); \
        return ret; \
    } \*/ \
}

#define WIFI_RUN_CMD(cmd) \
    if (WLAN_SUCCESS != wlan_run_cmd(cmd)) \
    { \
        PLAT_WLAN_INFO("Run CMD Error!!!"); \
        return AT_RETURN_FAILURE; \
    }
///


#define WIFI_TOOL_IWPRIV     "/system/bin/iwpriv"            /* iwpriv¹€Ÿß */
#define WIFI_TOOL_IFCONFIG   "/system/bin/busybox ifconfig"  /* ifconfig¹€Ÿß */
#define WIFI_BCRMTOOL_PATH   "/system/bin/wifi_brcm/exe/%s"

#define WIFI_POWER_DIFF_MIN (-7) 
#define WIFI_POWER_DIFF_MAX (8)

/*#if defined(FEATURE_HUAWEI_MBB_RTL8189)
    #define IS_TSELRF_VAILD(tselrf) ((AT_TSELRF_A == tselrf))
#elif defined(FEATURE_HUAWEI_MBB_RTL8192)
    #define IS_TSELRF_VAILD(tselrf) ((AT_TSELRF_A == tselrf) || (AT_TSELRF_B == tselrf))
#else
    #error "ERROR: NO DEFINED VAILD CHIP!!"
#endif*/

#ifdef MBB_ROUTER_QCT
    #define WLAN_CURRENT_TSELRF (IS_TSELRF_VAILD(WlanATGetTSELRF())? WlanATGetTSELRF() : AT_TSELRF_A)
#else
    #define WLAN_CURRENT_TSELRF (IS_TSELRF_VAILD(g_wlan_at_data.WiTselrf)? g_wlan_at_data.WiTselrf : AT_TSELRF_A)
#endif /* MBB_ROUTER_QCT */

/* ÅäÖÃÃüÁîÇ°±£Ö€RX/TXŽŠÓÚÍ£Ö¹×ŽÌ¬ */
#define WLAN_SET_RXTX_STOP() do { \
    if (AT_FEATURE_DISABLE != g_wlan_at_data.WiRX.onoff) \
    { \
        WIFI_TEST_CMD(WIFI_TOOL_IWPRIV" "WIFI_AP_INTERFACE" mp_arx stop"); \
        g_wlan_at_data.WiRX.onoff = AT_FEATURE_DISABLE; \
    } \
    if (AT_FEATURE_DISABLE != g_wlan_at_data.WiTX) \
    { \
        WIFI_TEST_CMD(WIFI_TOOL_IWPRIV" "WIFI_AP_INTERFACE" mp_ctx stop"); \
        g_wlan_at_data.WiTX = AT_FEATURE_DISABLE; \
    } \
}while(0)

/*===========================================================================
                       ÀàÐÍ¶šÒå
===========================================================================*/
typedef enum
{
    AT_TSELRF_A   = 0,  /*ÉäÆµA Â·*/
    AT_TSELRF_B   = 1,  /*ÉäÆµB Â·*/
    AT_TSELRF_MAX      /*×îŽóÖµ*/
}WLAN_AT_TSELRF_TYPE;
typedef struct __attribute__((__packed__)) _AT_WIMODE_GLOBAL
{
    //WLAN_AT_FEATURE_TYPE    cw_enable;  /* cwÄ£ÊœÊ¹ÄÜ */
    WLAN_AT_WIMODE_TYPE     mode;       /*wifiÐ­ÒéÄ£Êœ*/
}AT_WIMODE_GLOBAL_TYPE;
typedef struct __attribute__((__packed__)) _AT_WIFREQ_GLOBAL
{
    //uint32               channel;       /*wifiÆµµã¶ÔÓŠÐÅµÀ*/
    WLAN_AT_WIFREQ_STRU  freq;          /*wifiÆµµãÐÅÏ¢*/
    uint32               channel;       /*wifiÆµµã¶ÔÓŠÐÅµÀ*/
}AT_WIFREQ_GLOBAL_TYPE;
typedef struct __attribute__((__packed__)) _AT_WICAL_CH_POWER
{
    uint8 ch;
    uint8 data;
}AT_WICAL_CH_POWER_TYPE;
typedef struct __attribute__((__packed__)) _AT_WICAL_POWER
{
    AT_WICAL_CH_POWER_TYPE   cck[3];  /* 802.11b, ²úÏßÐ£×ŒµÍÖÐžßÐÅµÀ */
    AT_WICAL_CH_POWER_TYPE   ht40[3]; /* 40M, ²úÏßÐ£×ŒµÍÖÐžßÐÅµÀ */
    uint8   diffht20;                 /* 20M  diff*/
    uint8   diffofdm;                 /* ofdm  diff*/
    uint8   diffht40_2s;              /* 40M  2s  diff */
}AT_WICAL_POWER_TYPE;
typedef struct __attribute__((__packed__)) _AT_WICAL_GLOBAL
{
    WLAN_AT_FEATURE_TYPE    onoff;      /* Ð£×ŒÇÐ»»¿ª¹Ø */
    
    AT_WICAL_POWER_TYPE    power[AT_TSELRF_MAX];
    uint8 power_onoff[AT_TSELRF_MAX];   /* ¹ŠÂÊefuse ¿ÉÐŽ±êÖŸÎ» */
    
    uint8   crystal;                    /* ÆµÂÊ²¹³¥ */
    uint8 crystal_onoff;                /* ÆµÂÊ efuse ¿ÉÐŽ±êÖŸÎ» */
    
    uint8   thermal;                    /* ÎÂ¶È²¹³¥ */
    uint8 thermal_onoff;                /* ÎÂ¶Èefuse ¿ÉÐŽ±êÖŸÎ» */
}AT_WICAL_GLOBAL_TYPE;
typedef struct __attribute__((__packed__)) tagWlanATGlobal
{
    WLAN_AT_WIENABLE_TYPE   WiEnable;       /*Ä¬ÈÏŒÓÔØ²âÊÔÄ£Êœ*/
    AT_WIMODE_GLOBAL_TYPE   WiMode;
    WLAN_AT_WIBAND_TYPE     WiBand;         /* wifiÐ­ÒéÖÆÊœ*/
    AT_WIFREQ_GLOBAL_TYPE   WiFreq;         /* ÆµÂÊ */
    uint32                  WiDataRate;     /* wifi·¢ÉäËÙÂÊ*/
    int32                   WiPow;          /* wifi·¢Éä¹ŠÂÊ*/
    WLAN_AT_FEATURE_TYPE    WiTX;           /* wifi·¢Éä»ú×ŽÌ¬*/
    WLAN_AT_WIRX_STRU       WiRX;           /* wifiœÓÊÕ»ú×ŽÌ¬*/
    WLAN_AT_WiPARANGE_TYPE  WiParange;      /*wifi¹ŠÂÊÔöÒæÄ£Êœ*/
    
} WLAN_AT_GLOBAL_ST;

typedef struct __attribute__((__packed__)) _WLAN_AT_WICAL_PWR_PARAM_SET
{
    const char * cck;
    const char * diff_th20;
    const char * diff_th20_ofdm;
    const char * diff_th40;
    const char * th40_1s;
    const char * diff_th40_2s;
}WLAN_AT_WICAL_PWR_PARAM_SET_TYPE;

/*===========================================================================
                       È«ŸÖÐÅÏ¢
===========================================================================*/
/*¿ªÆôWiFiµÄÄ¬ÈÏ²ÎÊý*/
STATIC WLAN_AT_GLOBAL_ST g_wlan_at_data =
{   .WiEnable   = AT_WIENABLE_TEST,
    //.WiMode     = {AT_FEATURE_DISABLE, AT_WIMODE_80211n},
    .WiMode     = {AT_WIMODE_80211n},
    .WiBand     = AT_WIBAND_20M,
    .WiFreq     = {{2412, 0}, 1}, /* ³õÊŒ»¯Æµµã */
    .WiDataRate = 6500,            /* ³õÊŒ»¯ËÙÂÊ */
    .WiPow      = 3175,            /* ³õÊŒ»¯¹ŠÂÊ */
    .WiTX       = AT_FEATURE_DISABLE,
    .WiRX       = {AT_FEATURE_DISABLE, {0}, {0}},
    .WiParange  = AT_WiPARANGE_HIGH,
};

/*ÐÅµÀÁÐ±í*/
const uint16 g_ausChannels[] = {2412,2417,2422,2427,2432,2437,2442,2447,2452,2457,2462,2467,2472,2484};

/*===========================================================================
                       º¯ÊýÊµÏÖ
===========================================================================*/


//found in the original kernel, needed for the original BCMDHD module
uint32_t g_wifi_packet_new_rep[4] = {0};
uint32_t g_wifi_packet_report[4] = {0};
void wlan_at_get_packet_report (uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4);
//

/*===========================================================================
 (1)^WIENABLE ÉèÖÃWiFiÄ£¿éÊ¹ÄÜ
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiEnable
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFiÄ£¿éÊ¹ÄÜ
 ÊäÈë²ÎÊý  : onoff:Ê¹ÄÜ¿ª¹Ø
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : int32
*****************************************************************************/
STATIC int32 ATSetWifiEnable(WLAN_AT_WIENABLE_TYPE onoff)
{
    char buf[WIFI_CMD_MAX_SIZE];

    ASSERT_WiFi_INVALID(AT_RETURN_SUCCESS);

    ////PLAT_WLAN_INFO("Enter [onoff=%d]", onoff);

    switch (onoff)
    {
    case AT_WIENABLE_ON:
        PLAT_WLAN_INFO("Set wifi to normal mode");

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wifi_poweroff_43241.sh");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wifi_poweron_factory_43241.sh");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl down");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        break;
    case AT_WIENABLE_OFF:
        PLAT_WLAN_INFO("Set wifi to off mode");

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl down");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        msleep(2000);
        break;
    case AT_WIENABLE_TEST:
        PLAT_WLAN_INFO("Set wifi to test mode");

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wifi_poweroff_43241.sh");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wifi_poweron_factory_43241.sh");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        memset(buf, 0x00, sizeof(buf));
        SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl down");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
        WIFI_RUN_CMD(buf);
        msleep(100);

        break;
    default:
        PLAT_WLAN_INFO("Exit on PARAMS FAILED");
        return (AT_RETURN_FAILURE);
    }

    g_wlan_at_data.WiEnable = onoff;
    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiEnable
 ¹ŠÄÜÃèÊö  : »ñÈ¡µ±Ç°µÄWiFiÄ£¿éÊ¹ÄÜ×ŽÌ¬
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_WIENABLE_TYPE
*****************************************************************************/
STATIC WLAN_AT_WIENABLE_TYPE ATGetWifiEnable(void)
{
    char buf[WIFI_CMD_MAX_SIZE];

    ASSERT_WiFi_INVALID(AT_RETURN_SUCCESS);

    memset(buf, 0x00, sizeof(buf));
    SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl down");

    PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);

    WIFI_RUN_CMD(buf)

    return (g_wlan_at_data.WiEnable);
}

/*===========================================================================
 (2)^WIMODE ÉèÖÃWiFiÄ£Êœ²ÎÊý Ä¿Ç°ŸùÎªµ¥Ä£Êœ²âÊÔ
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiMode
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFi APÖ§³ÖµÄÁ¬œÓÄ£Êœ
 ÊäÈë²ÎÊý  : mode:Á¬œÓÄ£Êœ
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiMode(WLAN_AT_WIMODE_TYPE mode)
{
    char buf[WIFI_CMD_MAX_SIZE];

    PLAT_WLAN_INFO("enter,mode = %d", mode);

    switch (mode)
    {
        case AT_WIMODE_80211a:
        {
            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl band a");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl nmode 0");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl gmode 0");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            g_wlan_at_data.WiMode.mode = AT_WIMODE_80211a;
            break;
        }
        case AT_WIMODE_80211b:
        {
            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl nmode 0");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl gmode 0");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            g_wlan_at_data.WiMode.mode = mode;
            break;
        }
        case AT_WIMODE_80211g:
        {
            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl nmode 0");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl gmode 2");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            g_wlan_at_data.WiMode.mode = mode;
            break;
        }
        default:
        {
            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl nmode 1");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            memset(buf, 0x00, sizeof(buf));
            SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl gmode 1");
            PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);
            WIFI_RUN_CMD(buf);
            msleep(100);

            g_wlan_at_data.WiMode.mode = mode;
            break;
        }
    }

    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiMode
 ¹ŠÄÜÃèÊö  : »ñÈ¡µ±Ç°WiFiÖ§³ÖµÄÁ¬œÓÄ£Êœ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:µ±Ç°Ä£Êœ£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: 2
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiMode(WLAN_AT_BUFFER_STRU *strBuf)
{
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    SIZEOF_SNPRINTF(strBuf->buf, "%d"
        , g_wlan_at_data.WiMode.mode);

    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiModeSupport
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFi APÐŸÆ¬Ö§³ÖµÄËùÓÐÁ¬œÓÄ£Êœ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:Ö§³ÖµÄËùÓÐÄ£Êœ£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: 2,3,4
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiModeSupport(WLAN_AT_BUFFER_STRU *strBuf)
{
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    SIZEOF_SNPRINTF(strBuf->buf, "%s", WLAN_AT_MODE_SUPPORT);
    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (3)^WIBAND ÉèÖÃWiFiŽø¿í²ÎÊý
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiBand
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFiŽø¿í²ÎÊý
 ÊäÈë²ÎÊý  : width:Žø¿í
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiBand(WLAN_AT_WIBAND_TYPE width)
{
    PLAT_WLAN_INFO("enter,band = %d", width);

    switch(width)
    {
    case AT_WIBAND_20M:
        break;
    case AT_WIBAND_40M:
        break;
    default:
        PLAT_WLAN_INFO("Error wifi mode,must be in n mode");
        return AT_RETURN_FAILURE;
    }

    g_wlan_at_data.WiBand = width;
    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiBand
 ¹ŠÄÜÃèÊö  : »ñÈ¡µ±Ç°Žø¿íÅäÖÃ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:µ±Ç°Žø¿í£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: 0
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiBand(WLAN_AT_BUFFER_STRU *strBuf)
{
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    SIZEOF_SNPRINTF(strBuf->buf, "%d", g_wlan_at_data.WiBand);
    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiBandSupport
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFi APÖ§³ÖµÄŽø¿íÅäÖÃ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:Ö§³ÖŽø¿í£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: 0,1
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiBandSupport(WLAN_AT_BUFFER_STRU *strBuf)
{
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    SIZEOF_SNPRINTF(strBuf->buf, "%s", WLAN_AT_BAND_SUPPORT);
    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (4)^WIFREQ ÉèÖÃWiFiÆµµã
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiFreq
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFiÆµµã
 ÊäÈë²ÎÊý  : pFreq:ÆµµãÐÅÏ¢
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiFreq(WLAN_AT_WIFREQ_STRU *pFreq)
{
    return (AT_RETURN_FAILURE);
    int8 acCmd[WIFI_CMD_MAX_SIZE] = {0};
    uint32 channel = 0;

    ASSERT_NULL_POINTER(pFreq, AT_RETURN_FAILURE);
    ASSERT_WiFi_OFF(AT_RETURN_FAILURE);

    if (g_wlan_at_data.WiBand != 0) {
        g_wlan_at_data.WiFreq.channel = channel;
        g_wlan_at_data.WiFreq.freq.value = pFreq->value;
        g_wlan_at_data.WiFreq.freq.offset = pFreq->offset;
        return AT_RETURN_FAILURE;
    }

    for (channel = 0; channel < ARRAY_SIZE(g_ausChannels); channel++)
    {
        if (pFreq->value == g_ausChannels[channel])
        {
            channel++;
            break;
        }
    }

    if (ARRAY_SIZE(g_ausChannels) <= (channel-1))
    {
        return (AT_RETURN_FAILURE);
    }

    PLAT_WLAN_INFO("Target Channel = %d", pFreq->value, (int)(channel));

    WLAN_SET_RXTX_STOP();

    channel++;
    SIZEOF_SNPRINTF(acCmd, WIFI_TOOL_IWPRIV" "WIFI_AP_INTERFACE" mp_channel %d", (int)channel);
    WIFI_TEST_CMD(acCmd);

    g_wlan_at_data.WiFreq.channel = channel;
    g_wlan_at_data.WiFreq.freq.value = pFreq->value;
    g_wlan_at_data.WiFreq.freq.offset = pFreq->offset;
    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiFreq
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFiÆµµã
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : pFreq:µ±Ç°Æµµã
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiFreq(WLAN_AT_WIFREQ_STRU *pFreq)
{
    ASSERT_NULL_POINTER(pFreq, AT_RETURN_FAILURE);

    memcpy(pFreq, &(g_wlan_at_data.WiFreq.freq), sizeof(*pFreq));
    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (5)^WIDATARATE ÉèÖÃºÍ²éÑ¯µ±Ç°WiFiËÙÂÊŒ¯ËÙÂÊ
  WiFiËÙÂÊ£¬µ¥Î»Îª0.01Mb/s£¬È¡Öµ·¶Î§Îª0¡«65535
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiDataRate
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFiËÙÂÊŒ¯ËÙÂÊ
 ÊäÈë²ÎÊý  : rate:ËÙÂÊÖµ
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiDataRate(uint32 rate)
{
    return (AT_RETURN_FAILURE);
    int8    acCmd[WIFI_CMD_MAX_SIZE] = {0};
    uint32  ulNRate = 0;
    const uint32  aucAtWifiNRate_Table[] = {650, 1300, 1950, 2600, 3900, 5200, 5850, 6500};          /*20M DataRate*/
    const uint32  aucAtWifiNRate_40M_Table[] = {1350, 2700, 4050, 5400, 8100, 10800, 12150, 13500}; /*40M DataRate*/

    ASSERT_WiFi_OFF(AT_RETURN_FAILURE);
    PLAT_WLAN_INFO("Enter [mode=%d rate=%d]", g_wlan_at_data.WiMode.mode, (int)rate);

    WLAN_SET_RXTX_STOP();

    switch (g_wlan_at_data.WiMode.mode)
    {
    case AT_WIMODE_80211b:
    case AT_WIMODE_80211g:
        SIZEOF_SNPRINTF(acCmd, WIFI_TOOL_IWPRIV" "WIFI_AP_INTERFACE" mp_rate %d", (int)(rate / 50)); /*DataRate*/
        break;
    case AT_WIMODE_80211n:
        for (ulNRate = 0; ulNRate < ARRAY_SIZE(aucAtWifiNRate_Table); ulNRate++)
        {
            if (aucAtWifiNRate_Table[ulNRate] == rate)
            {
                break;
            }
            if (aucAtWifiNRate_40M_Table[ulNRate] == rate)
            {
                break;
            }
        }
        if (ARRAY_SIZE(aucAtWifiNRate_Table) <= ulNRate)
        {
            return (AT_RETURN_FAILURE);
        }
        SIZEOF_SNPRINTF(acCmd, WIFI_TOOL_IWPRIV" "WIFI_AP_INTERFACE" mp_rate %d", (int)(128 + ulNRate));/* ËÙÂÊ */
        break;
    default:
        return (AT_RETURN_FAILURE);
    }
    WIFI_TEST_CMD(acCmd);

    g_wlan_at_data.WiDataRate = rate;

    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiDataRate
 ¹ŠÄÜÃèÊö  : ²éÑ¯µ±Ç°WiFiËÙÂÊŒ¯ËÙÂÊ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : uint32:ËÙÂÊÖµ
*****************************************************************************/
STATIC uint32 ATGetWifiDataRate(void)
{
    return g_wlan_at_data.WiDataRate;
}

/*===========================================================================
 (6)^WIPOW ÀŽÉèÖÃWiFi·¢Éä¹ŠÂÊ
   WiFi·¢Éä¹ŠÂÊ£¬µ¥Î»Îª0.01dBm£¬È¡Öµ·¶Î§Îª -32768¡«32767
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiPOW
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFi·¢Éä¹ŠÂÊ
 ÊäÈë²ÎÊý  : power_dBm_percent:¹ŠÂÊÖµ
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiPOW(int32 power_dBm_percent)
{
    int8   acCmd[WIFI_CMD_MAX_SIZE] = {0};
    int8   acCmd2[WIFI_CMD_MAX_SIZE] = {0};
    int32  lWifiPower = (power_dBm_percent / 100); /* dBm */
#ifdef WLAN_NV_READ_POW
    WLAN_AT_WIINFO_POWER_STRU stGetPwrWiinfo;
#endif /*WLAN_NV_READ_POW*/
    static struct _target_power_
    {
        uint8 init;
        uint8 dot11b;
        uint8 dot11g;
        uint8 dot11n;
    }s_target_power = {0, 13, 11, 9}; /* power init */

    if (power_dBm_percent / 100 + 0xfU < 0x2e) {
    } else {
        PLAT_WLAN_INFO("Invalid argument");
        return AT_RETURN_FAILURE;
    }

    memset(acCmd, 0x00, sizeof(acCmd));
    memset(acCmd2, 0x00, sizeof(acCmd2));
    SIZEOF_SNPRINTF(acCmd2, "wl txpwr1 -d -o %u", lWifiPower);
    SIZEOF_SNPRINTF(acCmd, WIFI_BCRMTOOL_PATH, acCmd2);
    PLAT_WLAN_INFO("[ret=%d] %s", 0, acCmd);
    WIFI_RUN_CMD(acCmd);
    msleep(100);

    g_wlan_at_data.WiPow = power_dBm_percent;
    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiPOW
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFiµ±Ç°·¢Éä¹ŠÂÊ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : int32:¹ŠÂÊÖµ
*****************************************************************************/
STATIC int32 ATGetWifiPOW(void)
{
    return g_wlan_at_data.WiPow;
}

/*===========================================================================
 (7)^WITX ÉèÖÃWiFi·¢Éä»ú×ŽÌ¬
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiTX
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFi·¢Éä»ú×ŽÌ¬
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiTX(WLAN_AT_FEATURE_TYPE onoff)
{
    return (AT_RETURN_FAILURE);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiTX
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFi·¢Éä»ú×ŽÌ¬
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_FEATURE_TYPE
*****************************************************************************/
STATIC WLAN_AT_FEATURE_TYPE ATGetWifiTX(void)
{
    return g_wlan_at_data.WiTX;
}

/*===========================================================================
 (8)^WIRX ÉèÖÃWiFiœÓÊÕ»ú¿ª¹Ø
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiRX
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFiœÓÊÕ»ú¿ª¹Ø
 ÊäÈë²ÎÊý  : params:µ±Ç°ÐèÒªÅäÖÃµÄRX²ÎÊý
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiRX(WLAN_AT_WIRX_STRU *params)
{
    return (AT_RETURN_FAILURE);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiRX
 ¹ŠÄÜÃèÊö  : »ñÈ¡WiFiœÓ¿Ú»úÉèÖÃ×ŽÌ¬
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : params:µ±Ç°ÅäÖÃµÄRX²ÎÊý
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiRX(WLAN_AT_WIRX_STRU *params)
{
    ASSERT_NULL_POINTER(params, AT_RETURN_FAILURE);

    memcpy(params, &g_wlan_at_data.WiRX, sizeof(*params));

    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (9)^WIRPCKG ²éÑ¯WiFiœÓÊÕ»úÎó°üÂë£¬ÉÏ±šœÓÊÕµœµÄ°üµÄÊýÁ¿
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiRPCKG
 ¹ŠÄÜÃèÊö  : WiFiÉÏ±šœÓÊÕµœµÄ°üµÄÊýÁ¿ÇåÁã
 ÊäÈë²ÎÊý  : flag:·¢°üÍ³ŒÆÇåÁã
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiRPCKG(int32 flag)
{
    int8   acCmd[WIFI_CMD_MAX_SIZE] = {0};
    ASSERT_WiFi_OFF(AT_RETURN_FAILURE);

    if (0 != flag)
    {
        memset(acCmd, 0x00, sizeof(acCmd));
        SIZEOF_SNPRINTF(acCmd, WIFI_BCRMTOOL_PATH, "wl counters");
        PLAT_WLAN_INFO("[ret=%d] %s", 0, acCmd);
        WIFI_RUN_CMD(acCmd);
        msleep(100);

	g_wifi_packet_report[0] = g_wifi_packet_new_rep[0];
        g_wifi_packet_report[1] = g_wifi_packet_new_rep[1];
        g_wifi_packet_report[2] = g_wifi_packet_new_rep[2];
        g_wifi_packet_report[3] = g_wifi_packet_new_rep[3];

        return (AT_RETURN_SUCCESS);
    }

    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiRPCKG
 ¹ŠÄÜÃèÊö  : ²éÑ¯WiFiœÓÊÕ»úÎó°üÂë£¬ÉÏ±šœÓÊÕµœµÄ°üµÄÊýÁ¿
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : params:ÊýŸÝ°üÍ³ŒÆÐÅÏ¢
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiRPCKG(WLAN_AT_WIRPCKG_STRU *params)
{
    char buf[WIFI_CMD_MAX_SIZE] = {0};
    char *p_buf = NULL;
    const char *info_key = "Rx OK:";
    int staPkgs = 0;

    ASSERT_NULL_POINTER(params, AT_RETURN_FAILURE);
    ASSERT_WiFi_OFF(AT_RETURN_FAILURE);

    memset(buf, 0, sizeof(buf));
    SIZEOF_SNPRINTF(buf, WIFI_BCRMTOOL_PATH, "wl counters");

    PLAT_WLAN_INFO("[ret=%d] %s", 0, buf);

    WIFI_RUN_CMD(buf)

    msleep(100);

    short new = g_wifi_packet_new_rep[2] & 0xFFFF;
    short old = g_wifi_packet_report[2] & 0xFFFF;

    PLAT_WLAN_INFO("Enter [old = %d, new = %d]", old, new);
    params[1] = (WLAN_AT_WIRPCKG_STRU) {0, 0};
    params[0] = (WLAN_AT_WIRPCKG_STRU) {(old - new),0};

    PLAT_WLAN_INFO("Exit [good=%d,bad=%d]", (int)params->good_result, (int)params->bad_result);
    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (11)^WIPLATFORM ²éÑ¯WiFi·œ°žÆœÌš¹©ÓŠÉÌÐÅÏ¢
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiPlatform
 ¹ŠÄÜÃèÊö  : ²éÑ¯WiFi·œ°žÆœÌš¹©ÓŠÉÌÐÅÏ¢
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_WIPLATFORM_TYPE
*****************************************************************************/
STATIC WLAN_AT_WIPLATFORM_TYPE ATGetWifiPlatform(void)
{
    return (AT_WIPLATFORM_BROADCOM);
}

/*===========================================================================
 (12)^TSELRF ²éÑ¯ÉèÖÃµ¥°åµÄWiFiÉäÆµÍšÂ·
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetTSELRF
 ¹ŠÄÜÃèÊö  : ÉèÖÃÌìÏß£¬·Ç¶àÍšÂ·Ž«0
 ÊäÈë²ÎÊý  : group:ÌìÏßË÷Òý
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : int32
*****************************************************************************/
STATIC int32 ATSetTSELRF(uint32 group)
{
    return AT_RETURN_FAILURE;
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetTSELRFSupport
 ¹ŠÄÜÃèÊö  : Ö§³ÖµÄÌìÏßË÷ÒýÐòÁÐ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:ÌìÏßË÷Òý£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: 0,1,2,3
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetTSELRFSupport(WLAN_AT_BUFFER_STRU *strBuf)
{
    uint32_t ver = *((uint32_t *)0xf561eddc);
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    if (((ver != 0x306 && ver != 0x400) && (ver != 0x401)) && (ver != 0x707)) {
        SIZEOF_SNPRINTF(strBuf->buf, "%s", "0,1,2,3");
    } else {
        SIZEOF_SNPRINTF(strBuf->buf, "%s", "0,1");
    }

    return (AT_RETURN_SUCCESS);
}

/*===========================================================================
 (13)^WiPARANGEÉèÖÃ¡¢¶ÁÈ¡WiFi PAµÄÔöÒæÇé¿ö
===========================================================================*/
/*****************************************************************************
 º¯ÊýÃû³Æ  : ATSetWifiParange
 ¹ŠÄÜÃèÊö  : ÉèÖÃWiFi PAµÄÔöÒæÇé¿ö
 ÊäÈë²ÎÊý  : pa_type:ÔöÒæÖµÀàÐÍ
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATSetWifiParange(WLAN_AT_WiPARANGE_TYPE pa_type)
{
    if (AT_WiPARANGE_HIGH != pa_type) /*œöÖ§³ÖÄÚÖÃPA*/
    {
        return AT_RETURN_FAILURE;
    }

    return (AT_RETURN_SUCCESS);
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiParange
 ¹ŠÄÜÃèÊö  : ¶ÁÈ¡WiFi PAµÄÔöÒæÇé¿ö
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : NA
 ·µ »Ø Öµ  : WLAN_AT_WiPARANGE_TYPE
*****************************************************************************/
STATIC WLAN_AT_WiPARANGE_TYPE ATGetWifiParange(void)
{
    return AT_WiPARANGE_HIGH;
}

/*****************************************************************************
 º¯ÊýÃû³Æ  : ATGetWifiParangeSupport
 ¹ŠÄÜÃèÊö  : Ö§³ÖµÄpaÄ£ÊœÐòÁÐ
 ÊäÈë²ÎÊý  : NA
 Êä³ö²ÎÊý  : strBuf:Ö§³ÖµÄÔöÒæÐòÁÐ£¬ÒÔ×Ö·ûŽ®ÐÎÊœ·µ»Øeg: l,h
 ·µ »Ø Öµ  : WLAN_AT_RETURN_TYPE
*****************************************************************************/
STATIC int32 ATGetWifiParangeSupport(WLAN_AT_BUFFER_STRU *strBuf)
{
    ASSERT_NULL_POINTER(strBuf, AT_RETURN_FAILURE);

    SIZEOF_SNPRINTF(strBuf->buf, "%c", AT_WiPARANGE_HIGH);

    return (AT_RETURN_SUCCESS);
}


STATIC WLAN_CHIP_OPS bcm43241_ops = 
{
    .WlanATSetWifiEnable = ATSetWifiEnable, ////////
    .WlanATGetWifiEnable = ATGetWifiEnable, ////////
    
    .WlanATSetWifiMode   = ATSetWifiMode, ////////
    .WlanATGetWifiMode   = ATGetWifiMode, ////////
    .WlanATGetWifiModeSupport = ATGetWifiModeSupport, ////////

    .WlanATSetWifiBand = ATSetWifiBand, ////////
    .WlanATGetWifiBand = ATGetWifiBand, ////////
    .WlanATGetWifiBandSupport = ATGetWifiBandSupport, ////////

    .WlanATSetWifiFreq = ATSetWifiFreq, ///////#
    .WlanATGetWifiFreq = ATGetWifiFreq, ////////

    .WlanATSetWifiDataRate = ATSetWifiDataRate, ///////#
    .WlanATGetWifiDataRate = ATGetWifiDataRate, ////////

    .WlanATSetWifiPOW = ATSetWifiPOW, ////////
    .WlanATGetWifiPOW = ATGetWifiPOW, ////////

    .WlanATSetWifiTX = ATSetWifiTX, ///////#
    .WlanATGetWifiTX = ATGetWifiTX, ////////

    .WlanATSetWifiRX = ATSetWifiRX, ///////#
    .WlanATGetWifiRX = ATGetWifiRX, ////////

    .WlanATSetWifiRPCKG = ATSetWifiRPCKG, ///////
    .WlanATGetWifiRPCKG = ATGetWifiRPCKG, ///////

    .WlanATGetWifiInfo  = NULL,

    .WlanATGetWifiPlatform = ATGetWifiPlatform, ////////
    
    .WlanATGetTSELRF = NULL,
    .WlanATSetTSELRF = ATSetTSELRF, ///////#
    .WlanATGetTSELRFSupport = ATGetTSELRFSupport, ////////

    .WlanATSetWifiParange = ATSetWifiParange, ////////
    .WlanATGetWifiParange = ATGetWifiParange, ////////

    .WlanATGetWifiParangeSupport = ATGetWifiParangeSupport, ////////
    
    .WlanATGetWifiCalTemp = NULL,
    .WlanATSetWifiCalTemp = NULL,
    
    .WlanATSetWifiCalData = NULL,
    .WlanATGetWifiCalData = NULL,
    
    .WlanATSetWifiCal = NULL,
    .WlanATGetWifiCal = NULL,
    .WlanATGetWifiCalSupport = NULL,
    
    .WlanATSetWifiCalFreq = NULL,
    .WlanATGetWifiCalFreq = NULL,
    
    .WlanATSetWifiCalPOW = NULL,
    .WlanATGetWifiCalPOW = NULL
};

/*****************************************************************************
º¯ÊýÃû³Æ  : wlan_at_init_rtl_def
¹ŠÄÜÃèÊö  : rtl8192ÐŸÆ¬×°±žœÓ¿Ú³õÊŒ»¯º¯Êý
ÊäÈë²ÎÊý  : NA
Êä³ö²ÎÊý  : NA
·µ »Ø Öµ  : int
*****************************************************************************/
int __init wlan_at_init_bcm43241(void)
{
    PLAT_WLAN_INFO("enter");
    wlan_at_reg_chip(bcm43241, &bcm43241_ops);
    PLAT_WLAN_INFO("exit");
    return AT_RETURN_SUCCESS;
}
module_init(wlan_at_init_bcm43241); /*lint !e529*/


//found in the original kernel, needed for the original BCMDHD module
void wlan_at_get_packet_report (uint32_t param_1, uint32_t param_2, uint32_t param_3, uint32_t param_4) {
	PLAT_WLAN_INFO("enter");

	g_wifi_packet_new_rep[0] = param1;
	g_wifi_packet_new_rep[1] = param2;
	g_wifi_packet_new_rep[2] = param3;
	g_wifi_packet_new_rep[3] = param4;

	/* g_wifi_packet_new_rep: wlan_at_get_packet_report, ATSetWifiRPCKG, ATSetWifiRX
	   g_wifi_packet_rep:                                ATSetWifiRPCKG, ATSetWifiRX */
}


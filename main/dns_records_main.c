/** WiFi station and DNS record retieval Example
  *
  * This is the main program it calls DNS information retrieval Functions
  * contained in sti_resolve.h to show how those functions can be used to retrieve
  * DNS type "A" and "SRV" Resource Records.
  *
  * This software was copied from Espressif's IDF downwload. The original code
  * contained the following licensing information:
  *
  * This example code is in the Public Domain (or CC0 licensed, at your option.)
  *
  * Unless required by applicable law or agreed to in writing, this
  * software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  * CONDITIONS OF ANY KIND, either express or implied.
  *
  * The code has been modified by Jim Sutton 2021. The modified code in this file
  * is in the Public Domain (or CC0 licensed, at your option.) Other files in This
  * project that are original, are licensed using the standard MIT license. Refer to
  * the license information in each file.
  */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "sti_resolv.h"

/* The examples use WiFi configuration that you can set via project configuration menu
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* tHE HOSTNAME that DNS records are desired for is set in the config file as well
   It can easily be overriden here as well*/

#ifdef CONFIG_FULL_HOSTNAME
#define EXAMPLE_FULL_HOSTNAME      CONFIG_FULL_HOSTNAME
#else
#define EXAMPLE_FULL_HOSTNAME "XMPP.DISMAIL.DE"
#endif

#ifdef CONFIG_FULL_XMPP_SRV_HOST
#define EXAMPLE_FULL_XMPP_SRV_HOST  CONFIG_FULL_XMPP_SRV_HOST
#else
#define EXAMPLE_FULL_XMPP_SRV_HOST "_xmpp-client._tcp.dismail.de"
#endif

#ifdef CONFIG_PRIMARY_DNS_SERVER
#define PRIMARY_DNS_SERVER  CONFIG_PRIMARY_DNS_SERVER
#else
#define EXAMPLE_PRIMARY_DNS_SERVER "8.8.8.8"
#endif

#define MESSAGE_T_A 1 /* Message Type request is for type A DNS record*/
#define MESSAGE_T_SRV 33 /* Message Type Request is for SRV records*/
#define MESSAGE_C_IN 1 /* Message class is Internet */
#define UDP_BUFFER_SIZE 100

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void sti_cb (char *name, struct ip4_addr *addr){
    static const char *TAG = "sti_cb     ";
    ESP_LOGI(TAG, "...DNS information for %s IP is: "IPSTR"", name, IP2STR(addr));
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // sti Create two pointers that we will use to get information
    // about the connection
    //
    esp_netif_t *esp_netif_handle;
    esp_netif_ip_info_t ap_info;

    esp_netif_handle = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }


    // Now get information on the ip connection from the netif instance
    // pointed to by esp_netif_handle. I used esp_netif.h to find the
    // calls. I also used the netif test program which is located in the
    // components/esp_netif/test directory to resolve a problem i had with
    // the get_hostname fnction. In any case, First see if netif is up
    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, ".Information on Netif connection");
    if (esp_netif_is_netif_up(esp_netif_handle)){
      ESP_LOGI(TAG, "...Netif is running");
    }
    else {
      ESP_LOGI(TAG, "...Netif is not running");
    }
    // get ip information
    ESP_ERROR_CHECK(esp_netif_get_ip_info(esp_netif_handle, &ap_info));
    ESP_LOGI(TAG, "...Current IP from netif      : "IPSTR"", IP2STR(&ap_info.ip));

    //get hostname information
    const char *hostname = NULL;
    ESP_ERROR_CHECK(esp_netif_get_hostname(esp_netif_handle, &hostname));
    ESP_LOGI(TAG, "...Current Hostname from netif: %s", hostname);

    //get information on the DNS connections established when WiFI was configured
    esp_netif_dns_type_t ask_for_primary = ESP_NETIF_DNS_MAIN;
    esp_netif_dns_info_t dns_info;

    esp_netif_get_dns_info(esp_netif_handle, ask_for_primary, &dns_info);
    ESP_LOGI(TAG, "...Name Server Primary (netif): " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));

    err_t ret;
    ip_addr_t dnsserver_ip_addr;
    dnsserver_ip_addr.type = IPADDR_TYPE_V4;
    dnsserver_ip_addr.u_addr.ip4.addr = dns_info.ip.u_addr.ip4.addr;

    ip_addr_t primary_dns_server;
    primary_dns_server.type = IPADDR_TYPE_V4;
    primary_dns_server.u_addr.ip4.addr = inet_addr((const char *) EXAMPLE_PRIMARY_DNS_SERVER);

    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, ".Initialize the Resolver");

    //ret = resolv_init(&dnsserver_ip_addr);
    ret = resolv_init(&primary_dns_server);
    if (ret < 0 ){
      ESP_LOGI(TAG, "... Error initializing resolver " );
    }

    struct hostent *hp;
    struct ip4_addr *ip4_addr;

    char full_hostname_1[] = EXAMPLE_FULL_HOSTNAME;
    char full_hostname_2[] = EXAMPLE_FULL_XMPP_SRV_HOST;

    unsigned char an[UDP_BUFFER_SIZE];
    memset(an,0,UDP_BUFFER_SIZE);
    int anslen = UDP_BUFFER_SIZE;
    int res;

    // Now do DNS request for a type "A" record
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "...Start of res_query for A records");

    res = res_query(full_hostname_1, MESSAGE_C_IN, MESSAGE_T_A, an, anslen);
    ESP_LOGI(TAG, "...length of returned buffer is %d", res);
    print_buf(an,res);
    ESP_LOGI(TAG, "...End res_query for type A records");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Now do an SRV record
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "...Start of res_query for SRV records");
    memset(an,0,100); // reset an to zero

    res = res_query(full_hostname_2, MESSAGE_C_IN, MESSAGE_T_SRV, an, anslen);
    ESP_LOGI(TAG, "...length of res_query returned buffer %d", res);
    print_buf(an,res);
    ESP_LOGI(TAG, "...End res_query for SRV records");

    ret = resolv_close(); //close the UDP port and free memory
    if (ret < 0 ){
      ESP_LOGI(TAG, "... Error closing resolver UDP connection" );
    }

    ESP_LOGI(TAG, "\n");
    ESP_LOGI(TAG, ".Begin gethostbyname");
    hp = gethostbyname(full_hostname_1);

    ip4_addr = (struct ip4_addr *)hp->h_addr_list[0];

    ESP_LOGI(TAG, "...Gathering DNS records for %s ", full_hostname_1);
    ESP_LOGI(TAG, "...Address No. 0 from DNS: " IPSTR, IP2STR(ip4_addr));

    ESP_LOGI(TAG, "Done with connection... Now shutdown handlers");


    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}

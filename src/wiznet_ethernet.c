#include "wiznet_ethernet.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "dhcp.h"
#include "port_common.h"
#include "timer.h"
#include "wizchip_conf.h"
#include "wizchip_spi.h"

#define SOCKET_DHCP 0
#define DHCP_BUF_SIZE 2048
#define DHCP_RETRY_LIMIT 5

static wiz_NetInfo net_info = {
    /*
     * Filled at startup from the Pico's unique board ID.
     */
    .mac = {0, 0, 0, 0, 0, 0},
    .ip = {0, 0, 0, 0},
    .sn = {0, 0, 0, 0},
    .gw = {0, 0, 0, 0},
    .dns = {0, 0, 0, 0},
    .dhcp = NETINFO_DHCP
};

static uint8_t dhcp_buf[DHCP_BUF_SIZE];

static volatile uint16_t ms_count = 0;
static bool has_ip = false;
static uint8_t dhcp_retry_count = 0;

static void dhcp_assign_callback(void);
static void dhcp_conflict_callback(void);
static void wiznet_timer_callback(void);

static void make_mac_from_pico_unique_id(uint8_t mac[6]) {
    pico_unique_board_id_t board_id;
    pico_get_unique_board_id(&board_id);

    /*
     * Locally administered unicast MAC:
     * bit 0 = 0 means unicast
     * bit 1 = 1 means locally administered
     *
     * 02:50:43 = local prefix chosen for this Pico Clock project.
     */
    mac[0] = 0x02;
    mac[1] = 0x50; /* 'P' */
    mac[2] = 0x43; /* 'C' */

    /*
     * Fold the 64-bit Pico unique ID into the final 3 MAC bytes.
     * This is not globally assigned by IEEE, but should be stable
     * and very unlikely to collide on a home/lab LAN.
     */
    mac[3] = board_id.id[0] ^ board_id.id[3] ^ board_id.id[6];
    mac[4] = board_id.id[1] ^ board_id.id[4] ^ board_id.id[7];
    mac[5] = board_id.id[2] ^ board_id.id[5];

    if (mac[3] == 0x00 && mac[4] == 0x00 && mac[5] == 0x00) {
        mac[3] = 0x02;
        mac[4] = 0x23;
        mac[5] = 0x42;
    }
}

void wiznet_ethernet_init(void) {
    printf("wiznet: initializing SPI and W5100S\n");

    make_mac_from_pico_unique_id(net_info.mac);

    printf("wiznet: generated local MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
           net_info.mac[0],
           net_info.mac[1],
           net_info.mac[2],
           net_info.mac[3],
           net_info.mac[4],
           net_info.mac[5]);

    wizchip_spi_initialize();
    wizchip_cris_initialize();
    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    /*
     * Write the generated MAC address into the WIZnet chip before DHCP starts.
     * IP/subnet/gateway/DNS are still zero here and will be filled by DHCP later.
     */
    network_initialize(net_info);

    wizchip_1ms_timer_initialize(wiznet_timer_callback);

    printf("wiznet: starting DHCP client\n");

    DHCP_init(SOCKET_DHCP, dhcp_buf);
    reg_dhcp_cbfunc(dhcp_assign_callback, dhcp_assign_callback, dhcp_conflict_callback);
}

void wiznet_ethernet_poll(void) {
    uint8_t result = DHCP_run();

    if (result == DHCP_IP_LEASED) {
        if (!has_ip) {
            has_ip = true;
            dhcp_retry_count = 0;
            printf("wiznet: DHCP lease acquired\n");
        }
    } else if (result == DHCP_FAILED) {
        has_ip = false;
        dhcp_retry_count++;

        printf("wiznet: DHCP failed/timeout, retry %u of %u\n",
               dhcp_retry_count,
               DHCP_RETRY_LIMIT);

        if (dhcp_retry_count >= DHCP_RETRY_LIMIT) {
            printf("wiznet: DHCP retry limit reached; stopping DHCP client\n");
            DHCP_stop();
        }
    }
}

bool wiznet_ethernet_has_ip(void) {
    return has_ip;
}

static void dhcp_assign_callback(void) {
    getIPfromDHCP(net_info.ip);
    getGWfromDHCP(net_info.gw);
    getSNfromDHCP(net_info.sn);
    getDNSfromDHCP(net_info.dns);

    net_info.dhcp = NETINFO_DHCP;

    network_initialize(net_info);
    print_network_information(net_info);
}

static void dhcp_conflict_callback(void) {
    has_ip = false;
    printf("wiznet: DHCP IP conflict detected\n");
}

static void wiznet_timer_callback(void) {
    ms_count++;

    if (ms_count >= 1000) {
        ms_count = 0;
        DHCP_time_handler();
    }
}

#include <stdio.h>
#include "htool_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "esp_event.h"
#include "hagl/color.h"
#include "htool_api.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <font6x9.h>
#include <font5x7.h>
#include <aps.h>
#include <fps.h>
#include <hagl_hal.h>
#include <hagl.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "htool_api.h"
#include "htool_wifi.h"

static EventGroupHandle_t event;
static hagl_backend_t *display;

display_states cur_handling_state = ST_STARTUP;

volatile uint64_t last_timestamp = 0;
volatile uint64_t last_long_press_left_timestamp = 0;
volatile uint64_t last_long_press_right_timestamp = 0;
volatile bool long_press_right = false;
volatile bool long_press_left = false;

uint64_t pause_timestamp = 0;
bool first_scan = true;

uint8_t animation = 0;

wchar_t evil_twin_ssid[26] = {0};

color_t color_all_scans[16]; // TODO: remove


const wchar_t header[23] = u"Samurai by Azyz";
const wchar_t menu[60] = u"Menu:\nSol: ↑ / Sag: ↓\nSağa Uzun Bas: git";
const wchar_t scan[40] = u"-) Ağ Tarama";
const wchar_t deauth[40] = u"-) Deauth Saldırısı";
const wchar_t beacon[40] = u"-) Ağ Spammer";
const wchar_t c_portal[40] = u"-) Captive Portal";
const wchar_t evil_twin[40] = u"-) Evil Twin";
const wchar_t ble_spoof[40] = u"-) BLE Patlatıcı";
uint8_t printy = 0;
uint8_t length = 0;
wchar_t scans[10][50] = {0};
char scan_auth[5] = {};


static void menu_task() {
    color_t color_header = hagl_color(display, 20, 50, 250);
    color_t color_passive = hagl_color(display, 0, 0, 0);
    color_t color_green = hagl_color(display, 0, 255, 0);
    color_t color_red = hagl_color(display, 255, 0, 0);

    menu_cnt = 0;

    uint8_t min_y = 0;
    uint8_t max_y = 10;
    uint8_t line_y = 0;
    color_t color_dark_green = hagl_color(display, 0, 100, 0);
    color_t color_darker_green = hagl_color(display, 0, 50, 0);

    hagl_flush(display);
    hagl_clear(display);

    while (true) {
        hagl_put_text(display, header, 0, 0, color_header, font6x9);
        switch (cur_handling_state) {
            case ST_STARTUP:
                hagl_clear(display);
                hagl_flush(display);
                while (true) {
                    line_y++;
                    if (line_y > 10) {
                        line_y = 0;
                        min_y = min_y + 10;
                        max_y = max_y + 10;
                        if (max_y > 240) {
                            break;
                        }
                    }
                    hagl_put_text(display, u"Samurai", 35, 110, color_green, font6x9);
                    hagl_put_text(display, u"by Azyz", 40, 120, color_green, font6x9);
                    uint8_t x0 = esp_random() % 135;
                    uint16_t y0 = (esp_random() % (max_y - min_y + 1)) + min_y;
                    char random_bit = (char)((esp_random() & 0x01) + '0');
                    hagl_put_char(display, random_bit, x0, y0, color_green, font6x9);
                    hagl_put_char(display, random_bit, x0-5, y0+15, color_dark_green, font6x9);
                    hagl_put_char(display, random_bit, x0, min_y - 10, color_dark_green, font6x9);
                    hagl_put_char(display, random_bit, x0, min_y - 20, color_darker_green, font6x9);
                    hagl_flush(display);
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                hagl_flush(display);
                hagl_clear(display);
                cur_handling_state = ST_MENU;
                break;
            case ST_MENU:
                hagl_put_text(display, menu, 0, 10, color_header, font6x9);
                if (menu_cnt == 0) {
                    hagl_put_text(display, scan, 0, 40, color_red, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_green, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_green, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_green, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_green, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_green, font6x9);
                }
                else if (menu_cnt == 1) {
                    hagl_put_text(display, scan, 0, 40, color_green, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_red, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_green, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_green, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_green, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_green, font6x9);
                }
                else if (menu_cnt == 2) {
                    hagl_put_text(display, scan, 0, 40, color_green, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_green, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_red, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_green, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_green, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_green, font6x9);
                }
                else if (menu_cnt == 3) {
                    hagl_put_text(display, scan, 0, 40, color_green, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_green, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_green, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_red, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_green, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_green, font6x9);
                }
                else if (menu_cnt == 4) {
                    hagl_put_text(display, scan, 0, 40, color_green, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_green, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_green, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_green, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_red, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_green, font6x9);
                }
                else if (menu_cnt == 5) {
                    hagl_put_text(display, scan, 0, 40, color_green, font6x9);
                    hagl_put_text(display, deauth, 0, 50, color_green, font6x9);
                    hagl_put_text(display, beacon, 0, 60, color_green, font6x9);
                    hagl_put_text(display, c_portal, 0, 70, color_green, font6x9);
                    hagl_put_text(display, evil_twin, 0, 80, color_green, font6x9);
                    hagl_put_text(display, ble_spoof, 0, 90, color_red, font6x9);
                }
                if (long_press_right) {
                    long_press_right = false;
                    cur_handling_state = menu_cnt + 1;
                    if (cur_handling_state == ST_BLE_SPOOF) {
                        htool_api_ble_init();
                    }
                    menu_cnt = 0;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                if (long_press_left) {
                    long_press_left = false;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                break;
            case ST_SCAN:
                printy = 40;
                hagl_put_text(display, u"Tara:\nSola Uzun Bas: GERİ", 0, 10, color_header, font6x9);
                    if ((esp_timer_get_time() - pause_timestamp > 15000000) || first_scan) {
                        if (first_scan) {
                            htool_api_start_active_scan();
                            first_scan = false;
                        }
                        else {
                            htool_api_start_passive_scan();
                        }
                        scan_started = true;
                        pause_timestamp = esp_timer_get_time();
                    }
                    else {
                        if (!scan_started) {
                            hagl_put_text(display, u"Tarama Listesi:", 0, 28, color_header, font6x9);
                            for (uint8_t i = 0; i < (global_scans_count > 8 ? 8: global_scans_count); i++) {
                                length = strlen((const char *) global_scans[i].ssid);
                                if (length > 15) {
                                    length = 15;
                                }
                                if (global_scans[i].authmode == 0) { // TODO: use helper pointer return function
                                    //OPEN
                                    sprintf(scan_auth, "OPEN");
                                }
                                else if (global_scans[i].authmode == 1) {
                                    //WEP
                                    sprintf(scan_auth, "WEP");
                                }
                                else {
                                    //WPA or versions of it
                                    sprintf(scan_auth, "WPA");
                                }
                                swprintf(scans[i], sizeof(scans[i]), u"%.*s %d %s\n%02x:%02x:%02x:%02x:%02x:%02x ch: %d/%d",length, global_scans[i].ssid, global_scans[i].rssi, scan_auth, global_scans[i].bssid[0],
                                         global_scans[i].bssid[1], global_scans[i].bssid[2], global_scans[i].bssid[3], global_scans[i].bssid[4], global_scans[i].bssid[5], global_scans[i].second, global_scans[i].primary);
                                hagl_put_text(display, scans[i], 0, printy, hagl_color(display, 0, 255, 0), font5x7);
                                printy += 20;

                            }
                        }
                        else {
                            if (animation == 0) {
                                hagl_put_text(display, u"Taranıyor .  ", 0, 28, color_header, font6x9);
                            }
                            else if (animation == 1) {
                                hagl_put_text(display, u"Taranıyor .. ", 0, 28, color_header, font6x9);
                            }
                            else if (animation == 2) {
                                hagl_put_text(display, u"Taranıyor ... ", 0, 28, color_header, font6x9);
                            }
                            else if (animation == 3) {
                                hagl_put_text(display, u"Taranıyor  .. ", 0, 28, color_header, font6x9);
                            }
                            else if (animation == 4) {
                                hagl_put_text(display, u"Taranıyor   . ", 0, 28, color_header, font6x9);
                            }
                            else if (animation == 5) {
                                hagl_put_text(display, u"Taranıyor     ", 0, 28, color_header, font6x9);
                            }
                            animation++;
                            if (animation == 6) {
                                animation = 0;
                            }
                            for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                                hagl_put_text(display, scans[i], 0, printy, hagl_color(display, 0, 255, 0), font5x7);
                                printy += 20;
                            }
                        }
                    }
                if (long_press_left) {
                    long_press_left = false;
                    menu_cnt = 0;
                    if (scan_started) {
                        esp_wifi_scan_stop();
                        scan_started = false;
                    }
                    first_scan = true;
                    for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                        memset(scans[i], 0, sizeof(scans[i]));
                    }
                    cur_handling_state = ST_MENU;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_DEAUTH:
                printy = 55;
                hagl_put_text(display, u"Deauth:\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Sağa Uzun Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAŞLAT / DURDUR", 0, 35, color_header, font5x7);
                if (!htool_api_is_deauther_running()) {
                    if ((esp_timer_get_time() - pause_timestamp > 15000000) || first_scan) {
                        if (first_scan) {
                            htool_api_start_active_scan();
                            first_scan = false;
                        }
                        else {
                            htool_api_start_passive_scan();
                            pause_timestamp = esp_timer_get_time();
                        }
                        scan_started = true;
                        pause_timestamp = esp_timer_get_time();
                    }
                    else {
                        if (!scan_started) {
                            hagl_put_text(display, u"WİFİ Seç:", 0, 43, color_header, font6x9);
                            for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                                length = strlen((const char *) global_scans[i].ssid);
                                if (length > 26) {
                                    length = 26;
                                }
                                swprintf(scans[i], sizeof(scans[i]), u"%.*s", length, global_scans[i].ssid);
                            }
                        }
                        else {
                            if (animation == 0) {
                                hagl_put_text(display, u"Taranıyor .  ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 1) {
                                hagl_put_text(display, u"Taranıyor .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 2) {
                                hagl_put_text(display, u"Taranıyor ... ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 3) {
                                hagl_put_text(display, u"Taranıyor  .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 4) {
                                hagl_put_text(display, u"Taranıyor   . ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 5) {
                                hagl_put_text(display, u"Taranıyor     ", 0, 43, color_header, font6x9);
                            }
                            animation++;
                            if (animation == 6) {
                                animation = 0;
                            }
                        }
                    }
                    hagl_put_text(display, u"[Durduruldu]", 78, 43, color_red, font6x9);
                }
                else {
                    hagl_put_text(display, u"WiFi Seç:", 0, 43, color_header, font6x9);
                    hagl_put_text(display, u"[Çalışıyor]", 78, 43, color_green, font6x9);
                }
                for (uint8_t i = 0; i < 11; i++) {
                    color_all_scans[i] = hagl_color(display, 0, 255, 0); //TODO: change handling only use 2 variables
                }
                color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);

                for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                    if (i == menu_cnt) {
                        hagl_put_text(display, scans[i], 0, printy, color_all_scans[menu_cnt], font5x7);
                        printy = printy + 10;
                    }
                    else {
                        hagl_put_text(display, scans[i], 0, printy, color_all_scans[i], font5x7);
                        printy = printy + 10;
                    }
                }
                if (scans[0][0] != 0) {
                    if (global_scans_count == menu_cnt) {
                    hagl_put_text(display, u"Hepsine saldır", 0, printy, color_all_scans[menu_cnt], font5x7);
                    }
                    else {
                        hagl_put_text(display, u"Hepsine saldır", 0, printy, color_green, font5x7);
                    }
                }
                if (long_press_right) {
                    long_press_right = false;
                    pause_timestamp = 0;
                    if (htool_api_is_deauther_running()) {
                        htool_api_stop_deauther();
                        first_scan = true;
                    }
                    else {
                        htool_api_start_deauther();
                    }
                }
                if (long_press_left) {
                    menu_cnt = 0;
                    pause_timestamp = 0;
                    long_press_left = false;
                    htool_api_stop_deauther();
                    first_scan = true;
                    if (scan_started) {
                        esp_wifi_scan_stop();
                        scan_started = false;
                    }
                    for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                        memset(scans[i], 0, sizeof(scans[i]));
                    }
                    cur_handling_state = ST_MENU;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_BEACON:
                hagl_put_text(display, u"Ağ Spamlayıcı\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7); //TOOO: change header
                hagl_put_text(display, u"Sağa Uzun Bas", 0, 26, color_header, font5x7);
                for (uint8_t i = 0; i < 4; i++) {
                    color_all_scans[i] = hagl_color(display, 0, 255, 0);
                }
                color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);

                hagl_put_text(display, u"-) Rastgele (hızlı)", 0, 46, color_all_scans[0], font5x7);
                hagl_put_text(display, u"-) WiFi Seç (rastgele mac)", 0, 56, color_all_scans[1], font5x7);
                hagl_put_text(display, u"-) WiFi Seç (aynı mac)", 0, 66, color_all_scans[2], font5x7);
                hagl_put_text(display, u"-) troll (yavaş)", 0, 76, color_all_scans[3], font5x7);

                if (long_press_right) {
                    long_press_right = false;
                    if (menu_cnt == 1 || menu_cnt == 2) {
                        htool_api_stop_beacon_spammer();
                        beacon_task_args.beacon_index = menu_cnt;
                        cur_handling_state = ST_BEACON_SUBMENU;
                        menu_cnt = 0;
                        break;
                    }
                    if (htool_api_is_beacon_spammer_running()) {
                        htool_api_stop_beacon_spammer();
                    }
                    else {
                        htool_api_start_beacon_spammer(menu_cnt);
                    }
                }
                if (long_press_left) {
                    long_press_left = false;
                    menu_cnt = 0;
                    htool_api_stop_beacon_spammer();
                    cur_handling_state = ST_MENU;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                if (htool_api_is_beacon_spammer_running()) {
                    hagl_put_text(display, u"DUR!", 100, 26, color_red, font6x9);
                    if (animation == 0) {
                        hagl_put_text(display, u"Spamlanıyor .  ", 0, 34, color_header, font6x9);
                    }
                    else if (animation == 1) {
                        hagl_put_text(display, u"Spamlanıyor .. ", 0, 34, color_header, font6x9);
                    }
                    else if (animation == 2) {
                        hagl_put_text(display, u"Spamlanıyor ... ", 0, 34, color_header, font6x9);
                    }
                    else if (animation == 3) {
                        hagl_put_text(display, u"Spamlanıyor  .. ", 0, 34, color_header, font6x9);
                    }
                    else if (animation == 4) {
                        hagl_put_text(display, u"Spamlanıyor   . ", 0, 34, color_header, font6x9);
                    }
                    else if (animation == 5) {
                        hagl_put_text(display, u"Spamlanıyor     ", 0, 34, color_header, font6x9);
                    }
                    animation++;
                    if (animation == 6) {
                        animation = 0;
                    }
                }
                else {
                    hagl_put_text(display, u"SPAMLA!", 100, 26, color_green, font6x9);
                    hagl_put_text(display, u"Spamlanıyor     ", 0, 34, color_passive, font6x9);
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_BEACON_SUBMENU:
                printy = 55;
                hagl_put_text(display, u"AĞ Spamlayıcı:\nSola Uzun Bas: Geri ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Sağa Uzun Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"Başlat / Dur", 0, 35, color_header, font5x7);
                if (!htool_api_is_beacon_spammer_running()) {
                    if ((esp_timer_get_time() - pause_timestamp > 15000000) || first_scan) {
                        if (first_scan) {
                            htool_api_start_active_scan();
                            first_scan = false;
                        }
                        else {
                            htool_api_start_passive_scan();
                            pause_timestamp = esp_timer_get_time();
                        }
                        scan_started = true;
                        pause_timestamp = esp_timer_get_time();
                    }
                    else {
                        if (!scan_started) {
                            hagl_put_text(display, u"WiFi Seç:", 0, 43, color_header, font6x9);
                            for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                                length = strlen((const char *) global_scans[i].ssid);
                                if (length > 26) {
                                    length = 26;
                                }
                                swprintf(scans[i], sizeof(scans[i]), u"%.*s", length, global_scans[i].ssid);
                            }
                        }
                        else {
                            if (animation == 0) {
                                hagl_put_text(display, u"Taranıyor .  ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 1) {
                                hagl_put_text(display, u"Taranıyor .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 2) {
                                hagl_put_text(display, u"Taranıyor ... ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 3) {
                                hagl_put_text(display, u"Taranıyor  .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 4) {
                                hagl_put_text(display, u"Taranıyor   . ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 5) {
                                hagl_put_text(display, u"Taranıyor     ", 0, 43, color_header, font6x9);
                            }
                            animation++;
                            if (animation == 6) {
                                animation = 0;
                            }
                        }
                    }
                    hagl_put_text(display, u"[DURDU]", 78, 43, color_red, font6x9);
                }
                else {
                    hagl_put_text(display, u"WiFi Seç:", 0, 43, color_header, font6x9);
                    hagl_put_text(display, u"[ÇALIŞIYOR]", 78, 43, color_green, font6x9);
                }
                for (uint8_t i = 0; i < 11; i++) {
                    color_all_scans[i] = hagl_color(display, 0, 255, 0);
                }
                color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);

                for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                    if (i == menu_cnt) {
                        hagl_put_text(display, scans[i], 0, printy, color_all_scans[menu_cnt], font5x7);
                        printy = printy + 10;
                    }
                    else {
                        hagl_put_text(display, scans[i], 0, printy, color_all_scans[i], font5x7);
                        printy = printy + 10;
                    }
                }
                if (scans[0][0] != 0) {
                    if (global_scans_count == menu_cnt) {
                        hagl_put_text(display, u"Tüm WiFileri Spamla", 0, printy, color_all_scans[menu_cnt], font5x7);
                    }
                    else {
                        hagl_put_text(display, u"Tüm WiFileri Spamla", 0, printy, color_green, font5x7);
                    }
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (htool_api_is_beacon_spammer_running()) {
                        htool_api_stop_beacon_spammer();
                    }
                    else {
                        htool_api_start_beacon_spammer(beacon_task_args.beacon_index);
                    }
                }
                if (long_press_left) {
                    long_press_left = false;
                    menu_cnt = 0;
                    htool_api_stop_beacon_spammer();
                    cur_handling_state = ST_BEACON;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }

                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_C_PORTAL:
                printy = 55;
                hagl_put_text(display, u"Captive Portal:\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Sağa Uzun Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAŞLAT / DUR", 0, 35, color_header, font5x7);

                if (htool_api_is_captive_portal_running()) {
                    if (animation == 0) {
                        hagl_put_text(display, u"Creds Bekleniyor .  ", 0, 45, color_green, font6x9);
                    }
                    else if (animation == 1) {
                        hagl_put_text(display, u"Creds Bekleniyor .. ", 0, 45, color_green, font6x9);
                    }
                    else if (animation == 2) {
                        hagl_put_text(display, u"Creds Bekleniyor ... ", 0, 45, color_green, font6x9);
                    }
                    else if (animation == 3) {
                        hagl_put_text(display, u"Creds Bekleniyor  .. ", 0, 45, color_green, font6x9);
                    }
                    else if (animation == 4) {
                        hagl_put_text(display, u"Creds Bekleniyor   . ", 0, 45, color_green, font6x9);
                    }
                    else if (animation == 5) {
                        hagl_put_text(display, u"Creds Bekleniyor     ", 0, 45, color_green, font6x9);
                    }
                    animation++;
                    if (animation == 6) {
                        animation = 0;
                    }
                    if (htool_wifi_get_user_cred_len()) {
                        hagl_put_text(display, u"Kullanıcı adı:", 0, printy, color_red, font6x9);
                        printy += 10;
                        uint32_t printed_size = 0;
                        uint32_t size = htool_wifi_get_user_cred_len();
                        while (printed_size < size) {
                        uint32_t chunk_size;
                            chunk_size = 22;
                            if ((size - printed_size) < chunk_size) {
                                chunk_size = size - printed_size;
                            }
                            wchar_t *ws = malloc(chunk_size * sizeof(wchar_t));
                            swprintf(ws, chunk_size, L"%hs", htool_wifi_get_user_cred() + printed_size);
                            printed_size += chunk_size;
                            hagl_put_text(display, ws, 0, printy, color_green, font6x9);
                            FREE_MEM(ws);
                            printy += 10;
                        }
                    }
                    if (htool_wifi_get_pw_cred_len()) {
                        hagl_put_text(display, u"Şifre:", 0, printy, color_red, font6x9);
                        printy += 10;
                        uint32_t printed_size = 0;
                        uint32_t size = htool_wifi_get_pw_cred_len();
                        while (printed_size < size) {
                        uint32_t chunk_size;
                            chunk_size = 22;
                            if ((size - printed_size) < chunk_size) {
                                chunk_size = size - printed_size;
                            }
                            wchar_t *ws = malloc(chunk_size * sizeof(wchar_t));
                            swprintf(ws, chunk_size, L"%hs", htool_wifi_get_pw_cred() + printed_size);
                            printed_size += chunk_size;
                            hagl_put_text(display, ws, 0, printy, color_green, font6x9);
                            FREE_MEM(ws);
                            printy += 10;
                        }

                    }
                    hagl_put_text(display, u"[ÇALSIYOR]", 78, 35, color_green, font6x9);
                }
                else {
                    hagl_put_text(display, u"[DURDURULDU]", 78, 35, color_red, font6x9);
                    for (uint8_t i = 0; i < 4; i++) {
                        color_all_scans[i] = hagl_color(display, 0, 255, 0);
                    }
                    color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);
                    hagl_put_text(display, u"-) Google", 0, 55, color_all_scans[0], font5x7);
                    hagl_put_text(display, u"-) McDonald's", 0, 65, color_all_scans[1], font5x7);
                    hagl_put_text(display, u"-) Facebook", 0, 75, color_all_scans[2], font5x7);
                    hagl_put_text(display, u"-) Apple", 0, 85, color_all_scans[3], font5x7);
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (htool_api_is_captive_portal_running()) {
                        htool_wifi_reset_creds();
                        htool_api_stop_captive_portal();
                    }
                    else {
                        vTaskDelay(pdMS_TO_TICKS(200));
                        htool_api_start_captive_portal(menu_cnt);
                    }
                }
                if (long_press_left) {
                    long_press_left = false;
                    cur_handling_state = ST_MENU;
                    menu_cnt = 0;
                    if (htool_api_is_captive_portal_running()) {
                        htool_wifi_reset_creds();
                        htool_api_stop_captive_portal();
                    }
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_EVIL_TWIN:
                hagl_put_text(display, u"Evil Twin:\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Uzun Sağa Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"Marka Seç", 0, 35, color_header, font5x7);
                for (uint8_t i = 0; i < 16; i++) {
                    color_all_scans[i] = hagl_color(display, 0, 255, 0);
                }
                color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);

                hagl_put_text(display, u"-) General", 0, 46, color_all_scans[0], font5x7);
                hagl_put_text(display, u"-) Huawei", 0, 56, color_all_scans[1], font5x7);
                hagl_put_text(display, u"-) ASUS", 0, 66, color_all_scans[2], font5x7);
                hagl_put_text(display, u"-) Tp-Link", 0, 76, color_all_scans[3], font5x7);
                hagl_put_text(display, u"-) Netgear", 0, 86, color_all_scans[4], font5x7);
                hagl_put_text(display, u"-) o2", 0, 96, color_all_scans[5], font5x7);
                hagl_put_text(display, u"-) FritzBox", 0, 106, color_all_scans[6], font5x7);
                hagl_put_text(display, u"-) Vodafone", 0, 116, color_all_scans[7], font5x7);
                hagl_put_text(display, u"-) Magenta", 0, 126, color_all_scans[8], font5x7);
                hagl_put_text(display, u"-) 1&1", 0, 136, color_all_scans[9], font5x7);
                hagl_put_text(display, u"-) A1", 0, 146, color_all_scans[10], font5x7);
                hagl_put_text(display, u"-) Globe", 0, 156, color_all_scans[11], font5x7);
                hagl_put_text(display, u"-) PLDT", 0, 166, color_all_scans[12], font5x7);
                hagl_put_text(display, u"-) AT&T", 0, 176, color_all_scans[13], font5x7);
                hagl_put_text(display, u"-) Swisscom", 0, 186, color_all_scans[14], font5x7);
                hagl_put_text(display, u"-) Verizon", 0, 196, color_all_scans[15], font5x7);


                if (long_press_right) {
                    long_press_right = false;
                    captive_portal_task_args.is_evil_twin = true;
                    captive_portal_task_args.cp_index = menu_cnt;
                    cur_handling_state = ST_EVIL_TWIN_SUBMENU;
                    menu_cnt = 0;
                    break;
                }
                if (long_press_left) {
                    long_press_left = false;
                    menu_cnt = 0;
                    cur_handling_state = ST_MENU;
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_EVIL_TWIN_SUBMENU:
                printy = 75;
                hagl_put_text(display, u"Evil Twin:\nUzun Sola Bas: GERi", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Sağa Uzun Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAŞLAT / DUR", 0, 35, color_header, font5x7);

                if (htool_api_is_evil_twin_running()) {
                    if (animation == 0) {
                        hagl_put_text(display, u"Creds Bekleniyor .  ", 0, printy-10, color_green, font6x9);
                    }
                    else if (animation == 1) {
                        hagl_put_text(display, u"Creds Bekleniyor .. ", 0, printy-10, color_green, font6x9);
                    }
                    else if (animation == 2) {
                        hagl_put_text(display, u"Creds Bekleniyor ... ", 0, printy-10, color_green, font6x9);
                    }
                    else if (animation == 3) {
                        hagl_put_text(display, u"Creds Bekleniyor  .. ", 0, printy-10, color_green, font6x9);
                    }
                    else if (animation == 4) {
                        hagl_put_text(display, u"Creds Bekleniyor   . ", 0, printy-10, color_green, font6x9);
                    }
                    else if (animation == 5) {
                        hagl_put_text(display, u"Creds Bekleniyor     ", 0, printy-10, color_green, font6x9);
                    }
                    animation++;
                    if (animation == 6) {
                        animation = 0;
                    }

                    hagl_put_text(display, u"Uzun Sola Bas: GERİ:", 0, 45, color_green, font6x9);
                    hagl_put_text(display, evil_twin_ssid, 0, 55, color_green, font6x9);

                    if (htool_wifi_get_user_cred_len()) {
                        hagl_put_text(display, u"Şifre:", 0, printy, color_red, font6x9);
                        printy += 10;
                        uint32_t printed_size = 0;
                        uint32_t size = htool_wifi_get_user_cred_len();
                        while (printed_size < size) {
                        uint32_t chunk_size;
                            chunk_size = 22;
                            if ((size - printed_size) < chunk_size) {
                                chunk_size = size - printed_size;
                            }
                            wchar_t *ws = malloc(chunk_size * sizeof(wchar_t));
                            swprintf(ws, chunk_size, L"%hs", htool_wifi_get_user_cred() + printed_size);
                            printed_size += chunk_size;
                            hagl_put_text(display, ws, 0, printy, color_green, font6x9);
                            FREE_MEM(ws);
                            printy += 10;
                        }
                    }
                    hagl_put_text(display, u"[ÇALIŞIYOR]", 78, 35, color_green, font6x9);
                }
                else {
                    printy = 55;
                    for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                        color_all_scans[i] = hagl_color(display, 0, 255, 0);
                    }
                    color_all_scans[menu_cnt] = hagl_color(display, 255, 0, 0);

                    for (uint8_t i = 0; i < global_scans_count; i++) {
                        if (i == menu_cnt) {
                            hagl_put_text(display, scans[i], 0, printy, color_all_scans[menu_cnt], font5x7);
                            printy = printy + 10;
                        }
                        else {
                            hagl_put_text(display, scans[i], 0, printy, color_all_scans[i], font5x7);
                            printy = printy + 10;
                        }
                    }
                    if ((esp_timer_get_time() - pause_timestamp > 15000000) || first_scan) {
                        if (first_scan) {
                            htool_api_start_active_scan();
                            first_scan = false;
                        }
                        else {
                            htool_api_start_passive_scan();
                            pause_timestamp = esp_timer_get_time();
                        }
                        scan_started = true;
                        pause_timestamp = esp_timer_get_time();
                    }
                    else {
                        if (!scan_started) {
                            hagl_put_text(display, u"Wifi Seç:", 0, 43, color_header, font6x9);
                            for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                                length = strlen((const char *) global_scans[i].ssid);
                                if (length > 26) {
                                    length = 26;
                                }
                                swprintf(scans[i], sizeof(scans[i]), u"%.*s", length, global_scans[i].ssid);
                            }
                        }
                        else {
                            if (animation == 0) {
                                hagl_put_text(display, u"Taraniyor .  ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 1) {
                                hagl_put_text(display, u"Taraniyor .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 2) {
                                hagl_put_text(display, u"Taraniyor ... ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 3) {
                                hagl_put_text(display, u"Taraniyor  .. ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 4) {
                                hagl_put_text(display, u"Taraniyor   . ", 0, 43, color_header, font6x9);
                            }
                            else if (animation == 5) {
                                hagl_put_text(display, u"Taraniyor     ", 0, 43, color_header, font6x9);
                            }
                            animation++;
                            if (animation == 6) {
                                animation = 0;
                            }
                        }
                    }
                    hagl_put_text(display, u"[DURDURULDU]", 78, 35, color_red, font6x9);
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (htool_api_is_evil_twin_running()) {
                        htool_wifi_reset_creds();
                        htool_api_stop_evil_twin();
                    }
                    else {
                        swprintf(evil_twin_ssid, sizeof(evil_twin_ssid), u"%.*s", strlen((const char*)global_scans[menu_cnt].ssid) > 26 ? 26 : strlen((const char*)global_scans[menu_cnt].ssid), global_scans[menu_cnt].ssid);
                        htool_api_start_evil_twin(menu_cnt, captive_portal_task_args.cp_index);
                    }
                }
                if (long_press_left) {
                    long_press_left = false;
                    cur_handling_state = ST_EVIL_TWIN;
                    first_scan = true;
                    menu_cnt = 0;
                    if (htool_api_is_evil_twin_running()) {
                        htool_wifi_reset_creds();
                        htool_api_stop_evil_twin();
                    }
                    for (uint8_t i = 0; i < (global_scans_count > 8 ? 8 : global_scans_count); i++) {
                        memset(scans[i], 0, sizeof(scans[i]));
                    }
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
                hagl_flush(display);
                hagl_clear(display);
                break;
            case ST_BLE_SPOOF:
                hagl_put_text(display, u"BLE Patlatıcı [1]:\nUzun Sola Bas: GERI", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Uzun Sağa Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAşLAT / DUR", 0, 35, color_header, font5x7);

                hagl_put_text(display, u"-) Rastgele Döngü (Sağlam)", 0, 46, menu_cnt == 0 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Rastgele Apple Döngüsü", 0, 56, menu_cnt == 1 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods", 0, 66, menu_cnt == 2 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods Pro", 0, 76, menu_cnt == 3 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods Max", 0, 86, menu_cnt == 4 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods Gen2", 0, 96, menu_cnt == 5 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods Gen3", 0, 106, menu_cnt == 6 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AirPods Pro Gen2", 0, 116, menu_cnt == 7 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Power Beats", 0, 126, menu_cnt == 8 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Power Beats Pro", 0, 136, menu_cnt == 9 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Solo Pro", 0, 146, menu_cnt == 10 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Buds", 0, 156, menu_cnt == 11 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Flex", 0, 166, menu_cnt == 12 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats X", 0, 176, menu_cnt == 13 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Solo3", 0, 186, menu_cnt == 14 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Studio 3", 0, 196, menu_cnt == 15 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Studio Pro", 0, 206, menu_cnt == 16 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Fit Pro", 0, 216, menu_cnt == 17 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Sonraki Sayfa", 0, 226, menu_cnt == 18 ? color_red : color_green, font5x7);
                if (htool_api_ble_adv_running()) {
                    hagl_put_text(display, u"[ÇALIŞIYOR]", 78, 35, color_green, font6x9);
                    if (menu_cnt == 0) {
                        htool_api_set_ble_adv(39); // set random adv
                    }
                    else if (menu_cnt != 18) {
                        htool_api_set_ble_adv(menu_cnt - 1);
                    }
                }
                else {
                    hagl_put_text(display, u"[DURDU]", 78, 35, color_red, font6x9);
                }
                if (long_press_left) {
                    long_press_left = false;
                    cur_handling_state = ST_MENU;
                    menu_cnt = 0;
                    htool_api_ble_stop_adv();
                    htool_api_ble_deinit();
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (menu_cnt == 18) {
                        cur_handling_state = ST_BLE_SPOOF_SUBMENU1;
                        menu_cnt = 0;
                        htool_api_ble_stop_adv();
                        hagl_flush(display);
                        hagl_clear(display);
                        break;
                    }
                    else {
                        if (htool_api_ble_adv_running()) {
                            htool_api_ble_stop_adv();
                        }
                        else {
                            if (menu_cnt == 0) {
                                htool_api_set_ble_adv(39); // set random adv
                            }
                            else if (menu_cnt != 18) {
                                htool_api_set_ble_adv(menu_cnt - 1);
                            }
                            htool_api_ble_start_adv();
                        }
                    }
                }
                hagl_flush(display);
                hagl_clear(display);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            case ST_BLE_SPOOF_SUBMENU1:
                hagl_put_text(display, u"BLE Patlatıcı [2]:\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Uzun Sağa Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAŞLAT / DURDUR", 0, 35, color_header, font5x7);

                hagl_put_text(display, u"-) Öncekı Sayfa", 0, 46, menu_cnt == 0 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Beats Buds Plus", 0, 56, menu_cnt == 1 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV Setup", 0, 66, menu_cnt == 2 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV Pair", 0, 76, menu_cnt == 3 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV New User", 0, 86, menu_cnt == 4 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV ID Setup", 0, 96, menu_cnt == 5 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV AudioSync", 0, 106, menu_cnt == 6 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV Homekit Setup", 0, 116, menu_cnt == 7 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV Keyboard", 0, 126, menu_cnt == 8 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) AppleTV Connect Network", 0, 136, menu_cnt == 9 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) HomePod Setup", 0, 146, menu_cnt == 10 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Setup New Phone", 0, 156, menu_cnt == 11 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Transfer Number", 0, 166, menu_cnt == 12 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) TV Color Balance", 0, 176, menu_cnt == 13 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Google", 0, 186, menu_cnt == 14 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Random cyclic", 0, 196, menu_cnt == 15 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Watch4", 0, 206, menu_cnt == 16 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung French Watch4", 0, 216, menu_cnt == 17 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Sonraki Sayfa", 0, 226, menu_cnt == 18 ? color_red : color_green, font5x7);
                if (htool_api_ble_adv_running()) {
                    hagl_put_text(display, u"[ÇALIŞIYOR]", 78, 35, color_green, font6x9);
                    if (menu_cnt != 0 && menu_cnt != 18) {
                        htool_api_set_ble_adv(menu_cnt + 16);
                    }
                }
                else {
                    hagl_put_text(display, u"[Durduruldu]", 78, 35, color_red, font6x9);
                }
                if (long_press_left) {
                    long_press_left = false;
                    cur_handling_state = ST_BLE_SPOOF;
                    menu_cnt = 0;
                    htool_api_ble_stop_adv();
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (menu_cnt == 18) {
                        cur_handling_state = ST_BLE_SPOOF_SUBMENU2;
                        menu_cnt = 0;
                        htool_api_ble_stop_adv();
                        hagl_flush(display);
                        hagl_clear(display);
                        break;
                    }
                    if (menu_cnt != 0) {
                        if (htool_api_ble_adv_running()) {
                            htool_api_ble_stop_adv();
                        }
                        else {
                            htool_api_set_ble_adv(menu_cnt + 16);
                            htool_api_ble_start_adv();
                        }
                    }
                    else {
                        cur_handling_state = ST_BLE_SPOOF;
                        menu_cnt = 0;
                        htool_api_ble_stop_adv();
                        hagl_flush(display);
                        hagl_clear(display);
                        break;
                    }
                }
                hagl_flush(display);
                hagl_clear(display);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            case ST_BLE_SPOOF_SUBMENU2:
                hagl_put_text(display, u"BLE Patlatıcı [3]:\nUzun Sola Bas: GERİ", 0, 10, color_header, font5x7);
                hagl_put_text(display, u"Uzun Sağa Bas:", 0, 25, color_header, font5x7);
                hagl_put_text(display, u"BAŞLAT / DURDUR", 0, 35, color_header, font5x7);

                hagl_put_text(display, u"-) Önceki Sayfa", 0, 46, menu_cnt == 0 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Fox Watch5", 0, 56, menu_cnt == 1 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Watch5", 0, 66, menu_cnt == 2 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Watch5 Pro", 0, 76, menu_cnt == 3 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Samsung Watch6", 0, 86, menu_cnt == 4 ? color_red : color_green, font5x7);
                hagl_put_text(display, u"-) Microsoft", 0, 96, menu_cnt == 5 ? color_red : color_green, font5x7);
                if (htool_api_ble_adv_running()) {
                    hagl_put_text(display, u"[ÇALIŞIYOR]", 78, 35, color_green, font6x9);
                    if (menu_cnt != 0) {
                        htool_api_set_ble_adv(menu_cnt + 33);
                    }
                }
                else {
                    hagl_put_text(display, u"[DURDURULDU]", 78, 35, color_red, font6x9);
                }
                if (long_press_left) {
                    long_press_left = false;
                    cur_handling_state = ST_BLE_SPOOF_SUBMENU1;
                    menu_cnt = 0;
                    htool_api_ble_stop_adv();
                    hagl_flush(display);
                    hagl_clear(display);
                    break;
                }
                if (long_press_right) {
                    long_press_right = false;
                    if (menu_cnt != 0) {
                        if (htool_api_ble_adv_running()) {
                            htool_api_ble_stop_adv();
                        }
                        else {
                            htool_api_set_ble_adv(menu_cnt + 33);
                            htool_api_ble_start_adv();
                        }
                    }
                    else {
                        cur_handling_state = ST_BLE_SPOOF_SUBMENU1;
                        menu_cnt = 0;
                        htool_api_ble_stop_adv();
                        hagl_flush(display);
                        hagl_clear(display);
                        break;
                    }
                }
                hagl_flush(display);
                hagl_clear(display);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            default:
                break;
        }
    }
}

static void IRAM_ATTR gpio_interrupt_handler(void *args) {
    if (esp_timer_get_time() >= last_timestamp+350000) { //debounce
        last_timestamp = esp_timer_get_time();
        if (args == 0) {
            if (gpio_get_level(0)) {
                long_press_left = true;
            }
        }
        else {
            if (gpio_get_level(35)) {
                long_press_right = true;
            }
        }
    }
    else if ((int)args == 35) {
        if (long_press_right && esp_timer_get_time() <= last_timestamp+350000) {
            return;
        }
        if (esp_timer_get_time() >= last_long_press_right_timestamp+200000) { //debounce
            long_press_left = false;
            long_press_right = false;
            last_long_press_right_timestamp = esp_timer_get_time();
            if (cur_handling_state == ST_DEAUTH || cur_handling_state == ST_BEACON_SUBMENU) {
                menu_cnt++;
                if (menu_cnt > global_scans_count) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_EVIL_TWIN_SUBMENU) {
                menu_cnt++;
                if (menu_cnt > global_scans_count - 1) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_EVIL_TWIN) {
                menu_cnt++;
                if (menu_cnt > 15) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_C_PORTAL) {
                menu_cnt++;
                if (menu_cnt > 3) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_MENU) {
                menu_cnt++;
                if (menu_cnt > 5) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_BEACON) {
                menu_cnt++;
                if (menu_cnt > 3) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF) {
                menu_cnt++;
                if (menu_cnt > 18) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF_SUBMENU1) {
                menu_cnt++;
                if (menu_cnt > 18) {
                    menu_cnt = 0;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF_SUBMENU2) {
                menu_cnt++;
                if (menu_cnt > 5) {
                    menu_cnt = 0;
                }
            }
        }
    }
    else if ((int)args == 0) {
        if (long_press_left && esp_timer_get_time() <= last_timestamp+350000) {
            return;
        }
        if (esp_timer_get_time() >= last_long_press_left_timestamp + 200000) { //debounce
            long_press_left = false;
            long_press_right = false;
            last_long_press_left_timestamp = esp_timer_get_time();
            if (cur_handling_state == ST_DEAUTH || cur_handling_state == ST_BEACON_SUBMENU) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = global_scans_count;
                }
            }
            else if (cur_handling_state == ST_EVIL_TWIN_SUBMENU) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = global_scans_count - 1;
                }
            }
            else if (cur_handling_state == ST_EVIL_TWIN) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 15;
                }
            }
            else if (cur_handling_state == ST_C_PORTAL) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 3;
                }
            }
            else if (cur_handling_state == ST_MENU) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 5;
                }
            }
            else if (cur_handling_state == ST_BEACON) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 3;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 18;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF_SUBMENU1) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 18;
                }
            }
            else if (cur_handling_state == ST_BLE_SPOOF_SUBMENU2) {
                if (menu_cnt != 0) {
                    menu_cnt--;
                }
                else {
                    menu_cnt = 5;
                }
            }
        }
    }
}

void htool_display_init() {
    event = xEventGroupCreate();
    gpio_set_direction(0, GPIO_MODE_INPUT);
    gpio_set_direction(35, GPIO_MODE_INPUT);
    gpio_pullup_en(35);
    gpio_pulldown_en(35);
    display = hagl_init();
    gpio_install_isr_service(0);
    gpio_set_intr_type(0,GPIO_INTR_ANYEDGE);
    gpio_intr_enable(0);
    gpio_set_intr_type(35,GPIO_INTR_ANYEDGE);
    gpio_intr_enable(35);
    gpio_isr_handler_add(35, gpio_interrupt_handler, (void *)35);
    gpio_isr_handler_add(0, gpio_interrupt_handler, (void *)0);
}

void htool_display_start() {
    xTaskCreatePinnedToCore(menu_task, "test", (1024*6), NULL, 1, NULL, 1);
}



#include <stdint.h>
#include <string.h>

#include "raat.hpp"
#include "raat-buffer.hpp"
#include "raat-logging.hpp"

#include "http-get-server.hpp"

#include "raat-oneshot-timer.hpp"
#include "raat-oneshot-task.hpp"
#include "raat-task.hpp"

#include "MCP41XXX.hpp"

#include "application.hpp"

static HTTPGetServer s_server(false);
static const raat_devices_struct * pDevices;
static const raat_params_struct * pParams;

static uint16_t spin_degrees = 0;
static uint8_t spin_counter = 0;

static void eyes_reset(MCP41XXX * xaxis, MCP41XXX * yaxis);
static void eyes_set_degrees(uint16_t degrees, MCP41XXX * xaxis, MCP41XXX * yaxis);

static void spin_task_fn(RAATOneShotTask& ThisTask, __attribute__((unused)) void * pTaskData)
{   
    bool restart_task = true;

    eyes_set_degrees(spin_degrees, pDevices->pxaxis, pDevices->pyaxis);
    spin_degrees++;
    
    if (spin_degrees >= 360)
    {
        if (spin_counter--)
        {
            spin_degrees = 0;       
        }
        else
        {
            restart_task = false;
            eyes_reset(pDevices->pxaxis, pDevices->pyaxis);
        }
    }

    if (restart_task)
    {
        ThisTask.start();
    }
}
static RAATOneShotTask s_spin_task(1, spin_task_fn, NULL);

static void eyes_set_degrees(uint16_t degrees, MCP41XXX * xaxis, MCP41XXX * yaxis)
{
    float radians = (degrees * 2 * 3.14159f) / 360.0f;

    int16_t x = (int16_t)(128.0f + (128.0f * cos(radians)));
    int16_t y = (int16_t)(128.0f + (128.0f * sin(radians)));

    x = max(min(x, 255), 0);
    y = max(min(y, 255), 0);
    
    if (x>0)
    {
        x = x * 0.8f;
    }

    raat_logln_P(LOG_APP, PSTR("Setting %d: x=%d, y=%d"), degrees, x, y);
    xaxis->set_wiper(x);
    yaxis->set_wiper(y);
}

static void eyes_reset(MCP41XXX * xaxis, MCP41XXX * yaxis)
{
    xaxis->set_wiper(128);
    yaxis->set_wiper(128);
}

void eyes_open_close(bool close)
{
    pDevices->pblink->set(!close);
}

void eyes_set_direction(eEyesDirection eyesDirection)
{
    switch(eyesDirection)
    {
    case DIR_UP:
        eyes_set_degrees(0, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_UPRIGHT:
        eyes_set_degrees(45, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_RIGHT:
        eyes_set_degrees(85, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_DNRIGHT:
        eyes_set_degrees(135, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_DN:
        eyes_set_degrees(180, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_DNLEFT:
        eyes_set_degrees(225, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_LEFT:
        eyes_set_degrees(275, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_UPLEFT:
        eyes_set_degrees(315, pDevices->pxaxis, pDevices->pyaxis);
        break;
    case DIR_FORWARD:
        eyes_reset(pDevices->pxaxis, pDevices->pyaxis);
        break;
    }
}

static void send_standard_erm_response()
{
    s_server.set_response_code("200 OK");
    s_server.set_header("Access-Control-Allow-Origin", "*");
    s_server.finish_headers();
}

static void handle_config_url(char const * const url)
{
    raat_logln_P(LOG_APP, PSTR("Handling %s"), url);
    pParams->pletter_mapping->set(url+8);
    raat_logln_P(LOG_APP, PSTR("New mapping: %s"), pParams->pletter_mapping->get());
    send_standard_erm_response();
}

static void handle_spell_url(char const * const url)
{
    char word[MAXIMUM_WORD_LENGTH+1];
    raat_logln_P(LOG_APP, PSTR("Handling %s"), url+7);

    char const * const word_start = url + 7;
    char const * const word_end = strchr(word_start, '/');

    if (word_end)
    {
        uint8_t length = word_end - word_start;
        
        memcpy(word, word_start, length);
        word[length] = '\0';

        uint16_t spell_params[3];

        raat_parse_delimited_numerics<uint16_t>(word_end+1, spell_params, '/', 3);

        raat_logln_P(LOG_APP, PSTR("Got %u chars: '%s'"), length, word);
        raat_logln_P(LOG_APP, PSTR("Params: %u, %u, %u"),
            spell_params[0], spell_params[1], spell_params[2]);

        eyes_open_close(false);
        eyes_reset(pDevices->pxaxis, pDevices->pyaxis);
        spell_word(word, spell_params[0], spell_params[1], spell_params[2]);
    }

    send_standard_erm_response();

}

static void handle_open_url(char const * const url)
{
    (void)url;
    eyes_open_close(false);
    send_standard_erm_response();
}

static void handle_close_url(char const * const url)
{
    eyes_open_close(true);
    send_standard_erm_response();
}

static void handle_move_url(char const * const url)
{
    int32_t move_value;
    if (raat_parse_single_numeric(url+6, move_value, NULL))
    {
        move_value = move_value % 360;
        eyes_set_degrees((uint16_t)move_value, pDevices->pxaxis, pDevices->pyaxis);
    }

    send_standard_erm_response();
}

static void handle_blink_url(char const * const url)
{
    uint16_t blink_params[3];
    raat_logln_P(LOG_APP, PSTR("Handling %s"), url+7);
    raat_parse_delimited_numerics<uint16_t>(url+7, blink_params, '/', 3);

    raat_logln_P(LOG_APP, PSTR("Blinking %d times (%d on, %d off)"), blink_params[0], blink_params[1], blink_params[2]);

    start_blink(blink_params[0], blink_params[1], blink_params[2]);

    send_standard_erm_response();
}

static void handle_reset_url(char const * const url)
{
    (void)url;
    eyes_open_close(false);
    eyes_reset(pDevices->pxaxis, pDevices->pyaxis);

    send_standard_erm_response();
}

static void handle_spin_url(char const * const url)
{
    uint16_t spin_params[1];

    raat_logln_P(LOG_APP, PSTR("Handling %s"), url+6);
    raat_parse_delimited_numerics<uint16_t>(url+6, spin_params, '/', 3);
    raat_logln_P(LOG_APP, PSTR("Spinning %d times"), spin_params[0]);

    spin_degrees = 0;
    spin_counter = spin_params[0];
    s_spin_task.start();
    send_standard_erm_response();    
}

static const char CONFIG_URL[] PROGMEM = "/config";
static const char SPELL_WORD_URL[] PROGMEM = "/spell";
static const char BLINK_URL[] PROGMEM = "/blink";
static const char MOVE_URL[] PROGMEM = "/move";
static const char OPEN_URL[] PROGMEM = "/open";
static const char CLOSE_URL[] PROGMEM = "/close";
static const char RESET_URL[] PROGMEM = "/reset";
static const char SPIN_URL[] PROGMEM = "/spin";

static http_get_handler s_handlers[] = 
{
    {CONFIG_URL, handle_config_url},
    {SPELL_WORD_URL, handle_spell_url},
    {MOVE_URL, handle_move_url},
    {OPEN_URL, handle_open_url},
    {CLOSE_URL, handle_close_url},
    {BLINK_URL, handle_blink_url},
    {RESET_URL, handle_reset_url},
    {SPIN_URL, handle_spin_url},
    {"", NULL}
};

void ethernet_packet_handler(char * req)
{
	s_server.handle_req(s_handlers, req);
}

char * ethernet_response_provider()
{
	return s_server.get_response();
}

char const * get_spelling_map(void)
{
    return pParams->pletter_mapping->get();
}

void raat_custom_setup(const raat_devices_struct& devices, const raat_params_struct& params)
{
    pDevices = &devices;
    pParams = &params;

    params.ptarget_degrees->set(-1);
    eyes_reset(devices.pxaxis, devices.pyaxis);
    eyes_open_close(true);

    raat_logln_P(LOG_APP, PSTR("Cave Escape Mona Lisa"));
    raat_logln_P(LOG_APP, PSTR("Mapping: %s"), pParams->pletter_mapping->get());

}

void raat_custom_loop(const raat_devices_struct& devices, const raat_params_struct& params)
{
    (void)devices;

    if (params.ptarget_degrees->get() != -1)
    {
        eyes_set_degrees(params.ptarget_degrees->get(), devices.pxaxis, devices.pyaxis);
        params.ptarget_degrees->set(-1);
    }

    s_spin_task.run();
    run_blink();
    run_speller();
}

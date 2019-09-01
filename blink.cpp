#include <stdint.h>
#include <string.h>

#include "raat.hpp"

#include "raat-oneshot-timer.hpp"
#include "raat-oneshot-task.hpp"
#include "raat-task.hpp"

#include "application.hpp"

enum blink_state
{
    STATE_IDLE,
    STATE_BLINK,
    STATE_INTERBLINK
};

static enum blink_state s_blink_state = STATE_IDLE;

static uint8_t s_nblinks = 0;
static uint16_t s_blink_time = 0;
static uint16_t s_interblink_time = 0;

static void blink_task_fn(RAATOneShotTask& ThisTask, __attribute__((unused)) void * pTaskData
)
{
    switch(s_blink_state)
    {
    case STATE_BLINK:
        eyes_open_close(false);
        s_nblinks--;
        if (s_nblinks > 0)
        {
            s_blink_state = STATE_INTERBLINK;
            ThisTask.start(s_interblink_time);
        }
        else
        {
            s_blink_state = STATE_IDLE;
        }
        break;
    case STATE_INTERBLINK:
        eyes_open_close(true);
        s_blink_state = STATE_BLINK;
        ThisTask.start(s_blink_time);
        break;
    default:
        break;
    }
}
static RAATOneShotTask s_blink_task(50, blink_task_fn, NULL);

void start_blink(uint8_t nblinks, uint16_t blink_time, uint16_t interblink_time)
{
    s_nblinks = nblinks;
    s_blink_time = (blink_time / 50) * 50;
    s_interblink_time = (interblink_time / 50) * 50;

    if (s_nblinks)
    {
        eyes_open_close(true);
        s_blink_task.start(s_blink_time);
        s_blink_state = STATE_BLINK;
    }
}

void run_blink()
{
    s_blink_task.run();
}

#include <stdint.h>
#include <string.h>

#include "raat.hpp"
#include "raat-logging.hpp"

#include "raat-oneshot-timer.hpp"
#include "raat-oneshot-task.hpp"
#include "raat-task.hpp"

#include "application.hpp"

enum spell_state
{
    STATE_IDLE,
    STATE_SPELL,
    STATE_INTERMOVE,
    STATE_INTERLETTER
};

static enum spell_state s_spell_state = STATE_IDLE;

static uint8_t s_index = 0;
static eEyesDirection s_this_letter_direction;
static uint8_t s_number_of_moves = 0;

static uint16_t s_letter_time = 0;
static uint16_t s_intermove_time = 0;
static uint16_t s_interletter_time = 0;

static char s_word_to_spell[MAXIMUM_WORD_LENGTH+1];


static void start_new_letter(char letter)
{
    char const * pmapping = get_spelling_map();

    char const * plocation_in_map = strchr(pmapping, letter);

    if (plocation_in_map)
    {
        uint8_t index = plocation_in_map - pmapping;
        s_this_letter_direction = (eEyesDirection)(index / 3);

        s_number_of_moves = (index % 3) + 1;
        raat_logln_P(LOG_APP, PSTR("Spelling letter %c, %u moves, direction %d"),
            letter, s_number_of_moves, s_this_letter_direction);
    }

}

static void spell_task_fn(RAATOneShotTask& ThisTask, __attribute__((unused)) void * pTaskData)
{
    switch(s_spell_state)
    {
    case STATE_SPELL:
        eyes_set_direction(DIR_FORWARD);
        s_number_of_moves--;
        if (s_number_of_moves)
        {
            s_spell_state = STATE_INTERMOVE;
            ThisTask.start(s_intermove_time);
        }
        else
        {
            s_spell_state = STATE_INTERLETTER;
            ThisTask.start(s_interletter_time);
        }
        break;
    case STATE_INTERMOVE:
        eyes_set_direction(s_this_letter_direction);
        s_spell_state = STATE_SPELL;
        ThisTask.start(s_letter_time);
        break;
    case STATE_INTERLETTER:
        s_index++;
        if (s_word_to_spell[s_index])
        {
            start_new_letter(s_word_to_spell[s_index]);
            eyes_set_direction(s_this_letter_direction);
            ThisTask.start(s_letter_time);
            s_spell_state = STATE_SPELL;
        }
        else
        {
            s_spell_state = STATE_IDLE;
        }
    default:
        break;
    }
}
static RAATOneShotTask s_spell_task(50, spell_task_fn, NULL);

void spell_word(char const * const word, uint16_t letter_time, uint16_t intermove_time, uint16_t interletter_time)
{
    strncpy(s_word_to_spell, word, MAXIMUM_WORD_LENGTH);
    s_index = 0;

    s_letter_time = (letter_time / 50) * 50;
    s_intermove_time = (intermove_time / 50) * 50;
    s_interletter_time = (interletter_time / 50) * 50;

    if (strlen(s_word_to_spell))
    {
        start_new_letter(s_word_to_spell[0]);
        eyes_set_direction(s_this_letter_direction);
        s_spell_task.start(s_letter_time);
        s_spell_state = STATE_SPELL;
    }
}

void run_speller()
{
    s_spell_task.run();
}

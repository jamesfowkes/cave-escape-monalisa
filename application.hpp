#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#define MAXIMUM_WORD_LENGTH (12)

typedef enum EyesDirection
{
	DIR_UP,
	DIR_UPRIGHT,
	DIR_RIGHT,
	DIR_DNRIGHT,
	DIR_DN,
	DIR_DNLEFT,
	DIR_LEFT,
	DIR_UPLEFT,
	DIR_FORWARD
} eEyesDirection;

// Eye control

void eyes_open_close(bool open);
void eyes_set_direction(eEyesDirection eyesDirection);

// Blink related functionality

void start_blink(uint8_t nblinks, uint16_t blink_time, uint16_t interblink_time);
void run_blink();

// Sepllling Releated Functionality
void spell_word(char const * const word, uint16_t letter_time, uint16_t intermove_time, uint16_t interletter_time);
char const * get_spelling_map(void);
void run_speller(void);

#endif

#ifndef __ANSICODE_H    /*  An extra safeguard to prevent this header from  */
#define __ANSICODE_H    /*  being included twice in the same source file    */

/* Escape deliminator */
#define ESC 0x1B

/* ANSI escape sequences */
#define CURSOR_POSITION       'H'
#define CURSOR_POSITION_ALT   'f'     /* equivalent to 'H' */
#define CURSOR_UP             'A'
#define CURSOR_DOWN           'B'
#define CURSOR_FORWARD        'C'
#define CURSOR_BACKWARD       'D'
#define SAVE_CURSOR_POS       's'
#define RESTORE_CURSOR_POS    'u'
#define ERASE_DISPLAY         'J'     /* 2J */
#define ERASE_TO_EOL          'K'
#define SET_GRAPHICS_MODE     'm'
#define SET_MODE              'h'
#define RESET_MODE            'l'
#define SET_KEYBOARD_STRINGS  'p'

/* Not in Use Anymore

// Text attributes
#define ALL_ATTRIBUTES_OFF 0
#define BOLD_ON            1
#define UNDERSCORE         4
#define BLINK_ON           5
#define REVERSE_VIDEO_ON   7
#define CONCEALED_ON       8

*/

#endif

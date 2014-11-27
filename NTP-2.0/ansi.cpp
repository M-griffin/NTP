#ifndef __ANSI_CPP    /*  An extra safeguard to prevent this header from  */
#define __ANSI_CPP    /*  being included twice in the same source file    */

/*--------------------------------------------------------------------------------*/
// Ansi Parser - Michael Griffin (c) 2003
// Inspired from Saeger's Source Code. - But had to be completely Rewritten!
/*--------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ansicode.h"
#include "wincon.h"
#include "ansi.h"

/*--------------------------------------------------------------------------------*/
// Globals
/*--------------------------------------------------------------------------------*/

#define NUM_LINES   25     // change to 50 for 80x50 ANSI's
#define MAX_PARAMS  10

enum { FALSE, TRUE };
//enum { progname, ansifile };

/*--------------------------------------------------------------------------------*/
// Start of Main Program and Ansi Parsing - *Note : Break off into Function*
/*--------------------------------------------------------------------------------*/

void ansiparse ( char *buff )
{

    static SCREENINFO scrn;

    // Set Default Ansi Colors
    scrn.fc   = 7;
    scrn.bc   = 0;
    scrn.addi = 0;

    //FILE *fp;
    //union REGS regs;   // Not in use, Dos Screen Registers

    unsigned char c;
    char param[MAX_PARAMS];
    char p, dig;
    char i;

    unsigned char curr_clr = 0;
    //char curr_x = wherex (), curr_y = wherey ();

    char saved_cursor_x;
    char saved_cursor_y;

    char line_wrapping = TRUE;
    char more_params;
    char at_least_one_digit;
    char first_param_implied;

    if ( line_wrapping )
    {
        line_wrapping = line_wrapping;    /* satisfy compiler */
    }

    for ( int z = 0; z <= strlen ( buff ); z++ )
    {
        c = buff[z];

        //Handle escape sequence
        if ( c == ESC )
        {
            c = buff[++z]; //getc (fp);    // grab the left bracket

            more_params = TRUE;
            first_param_implied = FALSE;
            p = 0;

            while ( more_params == TRUE )
            {
                at_least_one_digit = FALSE;
                ++z;
                for ( dig = 0; ( isdigit ( c = buff[z] ) ) && ( dig < 3 ); dig++ )
                {
                    at_least_one_digit = TRUE;

                    // 3 digits at most (255) in a byte size decimal number */
                    if ( dig == 0 )
                    {
                        param[p] = c - '0';
                    }
                    else if ( dig == 1 )
                    {
                        param[p] *= 10;
                        param[p] += c - '0';
                    }
                    else
                    {
                        param[p] *= 100;
                        param[p] += c - '0';
                    }
                    z++;
                }
                //   ESC[C     p should = 0
                //   ESC[6C    p should = 1
                //   ESC[1;1H  p should = 2
                //   ESC[;79H  p should = 2
                if ( c != '?' )  // this is apprently an ANSI bug. ESC[?7h - Skip
                {
                    if ( ( at_least_one_digit == TRUE ) && ( c == ';' ) )
                        p++;
                    else if ( ( ! ( at_least_one_digit == TRUE ) ) && ( c == ';' ) )
                    {
                        p++;
                        first_param_implied = TRUE;
                    }
                    else if ( at_least_one_digit )
                    {
                        p++;
                        more_params = FALSE;
                    }
                    else
                        more_params = FALSE;
                }
            } // End While (more_params)

            // Handle specific escape sequences
            switch ( c )
            {
                case CURSOR_POSITION:
                case CURSOR_POSITION_ALT:
                    if ( p == 0 )
                        gotoxy ( 1, 1 );
                    else if ( p == 1 )
                        gotoxy ( 1, param[0] );
                    else if ( first_param_implied )
                        gotoxy ( param[1], wherey () );
                    else
                        gotoxy ( param[1], param[0] );
                    break;

                case CURSOR_UP:
                    if ( p == 0 )
                    {
                        if ( wherey () > 1 )
                            gotoxy ( wherex (), wherey () -1 );
                    }
                    else
                    {
                        if ( param[0] > wherey () )
                            gotoxy ( wherex (), 1 );
                        else
                            gotoxy ( wherex (), wherey () - param[0] );
                    }
                    break;

                case CURSOR_DOWN:
                    if ( p == 0 )
                    {
                        if ( wherey () < NUM_LINES )
                            gotoxy ( wherex (), wherey () + 1 );
                    }
                    else
                    {
                        if ( param[0] > ( NUM_LINES ) - wherey () )
                            gotoxy ( wherex (), NUM_LINES - 1 );
                        else
                            gotoxy ( wherex (), wherey () + param[0] );
                    }
                    break;

                case CURSOR_FORWARD:
                    if ( p == 0 )
                    {
                        if ( wherex () < 80 )
                            gotoxy ( wherex () + 1, wherey () );
                    }
                    else
                    {
                        if ( param[0] > 80 - wherex () )
                            gotoxy ( 80, wherey () );
                        else
                            gotoxy ( wherex () + param[0], wherey () );
                    }
                    break;

                case CURSOR_BACKWARD:
                    if ( p == 0 )
                    {
                        if ( wherex () > 1 )
                            gotoxy ( wherex () - 1, wherey () );
                    }
                    else
                    {
                        if ( param[0] > wherex () )
                            gotoxy ( 0, wherey () );
                        else
                            gotoxy ( wherex () - param[0], wherey () );
                    }
                    break;

                case SAVE_CURSOR_POS:
                    saved_cursor_x = wherex ();
                    saved_cursor_y = wherey ();
                    break;

                case RESTORE_CURSOR_POS:
                    gotoxy ( saved_cursor_x, saved_cursor_y );
                    break;

                case ERASE_DISPLAY:
                    if ( param[0] == 2 )
                        clrscr ();
                    break;

                case ERASE_TO_EOL:
                    clreol ();
                    break;

                case SET_GRAPHICS_MODE:

                    if ( p == 0 ) // Change text attributes / All Attributes off
                    {
                        scrn.fc   = 7;
                        scrn.bc   = 0;
                        scrn.addi = 0;
                    }
                    else
                    {
                        for ( i = 0; i < p; i++ )
                        {
                            switch ( param[i] )
                            {
                                case 0: // All Attributes off
                                    scrn.fc   = 7;
                                    scrn.bc   = 0;
                                    scrn.addi = 0;
                                    break;

                                case 1: // BOLD_ON
                                    if ( scrn.addi == 0 | scrn.addi == 16 )
                                    {
                                        scrn.fc   += 8;
                                        scrn.addi += 8;
                                        break;
                                    }
                                    break;

                                case 4:	// UNDERSCORE
                                    break;

                                case 5: // BLINK_ON
                                    if ( scrn.fc < 16 )
                                    {
                                        scrn.fc   += 16;
                                        scrn.addi += 16;
                                        break;
                                    }
                                    break;

                                case 7: // REVERSE_VIDEO_ON
                                    break;

                                case 8: // CONCEALED_ON
                                    break;

                                case 30: // FG_BLACK
                                    scrn.fc = scrn.addi;
                                    break;

                                case 31: // FG_RED
                                    scrn.fc = scrn.addi + 4;
                                    break;

                                case 32: // FG_GREEN
                                    scrn.fc = scrn.addi + 2;
                                    break;

                                case 33: // FG_YELLOW
                                    scrn.fc = scrn.addi + 6;
                                    break;

                                case 34: // FG_BLUE
                                    scrn.fc = scrn.addi + 1;
                                    break;

                                case 35: // FG_MAGENTA
                                    scrn.fc = scrn.addi + 5;
                                    break;

                                case 36: // FG_CYAN
                                    scrn.fc = scrn.addi + 3;
                                    break;

                                case 37: // FG_WHITE
                                    scrn.fc = scrn.addi + 7;
                                    break;

                                case 40: // BG_BLACK
                                    scrn.bc = 0;
                                    break;

                                case 41: // BG_RED
                                    scrn.bc = 4;
                                    break;

                                case 42: // BG_GREEN
                                    scrn.bc = 2;
                                    break;

                                case 43: // BG_YELLOW
                                    scrn.bc = 6;
                                    break;

                                case 44: // BG_BLUE
                                    scrn.bc = 1;
                                    break;

                                case 45: // BG_MAGENTA
                                    scrn.bc = 5;
                                    break;

                                case 46: // BG_CYAN
                                    scrn.bc = 3;
                                    break;

                                case 47: // BG_WHITE
                                    scrn.bc = 7;
                                    break;

                                default :
                                    break;

                            } // End Switch
                        } // End For
                    } // End Else

                    // Sets color attributes to Screen Before Drawing Characters
                    setcolor ( scrn.fc,scrn.bc );

                case RESET_MODE:
                    if ( param[0] == 7 )
                        line_wrapping = FALSE;

                case SET_MODE:
                    if ( param[0] <= 6 || ( param[0] >= 8 && param[0] <= 16 ) )
                    {
                        // Remove Screen mode Setting, not setable under windows!
                        /*
                        regs.h.ah = 0;
                        regs.h.al = param[0];
                        int86 (0x10, &regs, &regs);
                        */
                    }
                    else if ( param[0] == 7 )
                    {
                        line_wrapping = TRUE;
                    }
                    break;

                case SET_KEYBOARD_STRINGS:
                    break;

                default:
                    break;

            } // End of Switch(c) Case Statements

        } // end of main escape sequence handler
        else   // otherwise output character using current color */
        {
            /* 	if (c == '\n')
                	printf ("\n\r");
             	else */
            printf ( "%c", c );
        }
    }   // end while !feof
    setcolor ( 0,0 );
    return;

}

#endif

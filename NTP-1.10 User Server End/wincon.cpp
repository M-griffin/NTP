#ifndef __WINCON_CPP    /*  An extra safeguard to prevent this header from  */
#define __WINCON_CPP    /*  being included twice in the same source file    */

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN_AND_MEAN

#include <stdio.h>
#include <windows.h>
#include "wincon.h"

using namespace std;

POINT screensize;

#define BLINK 0

static int __BACKGROUND = 0;
static int __FOREGROUND = 7;

/*--------------------------------------------------------------------------------*/
// WINDOWS CONSOLE TEXT FUNCTIONS
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
// Function for Printing out to the screen
void setcolor ( int fg, int bg )
{

    int _attr;
    color ( fg ); // Set Foreground

    if ( fg > 15 ) _attr = 1;
    else _attr = 0;
    SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), _attr );

    bgcolor ( bg ); // Set Background
    return;
}

/*--------------------------------------------------------------------------------*/
// Clears the Screen
void clrscr()
{

    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize;
    HANDLE hConsole = GetStdHandle ( STD_OUTPUT_HANDLE );

    GetConsoleScreenBufferInfo ( hConsole, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    screensize.x = csbi.dwSize.X;
    screensize.y = csbi.dwSize.Y;
    FillConsoleOutputCharacter ( hConsole, TEXT ( ' ' ), dwConSize, coordScreen, &cCharsWritten );
    GetConsoleScreenBufferInfo ( hConsole, &csbi );
    FillConsoleOutputAttribute ( hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten );
    SetConsoleCursorPosition ( hConsole, coordScreen );
}

/*--------------------------------------------------------------------------------*/
// Sets Cursor Position via x,y, coords.
void gotoxy ( int x, int y )
{

    COORD point;
    if ( ( x < 0 || x > screensize.x ) || ( y < 0 || y > screensize.y ) )
        return;
    point.X = ( x -1 );
    point.Y = ( y -1 ); // -1 compisates for Bad Windows API
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
}

/*--------------------------------------------------------------------------------*/
// Delete Line
void delline()
{

    COORD coord;
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                                 &info );
    coord.X = info.dwCursorPosition.X;
    coord.Y = info.dwCursorPosition.Y;

    FillConsoleOutputCharacter ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                                 ' ', info.dwSize.X * info.dwCursorPosition.Y, coord, &written );
    gotoxy ( info.dwCursorPosition.X + 1,
             info.dwCursorPosition.Y + 1 );
}

/*--------------------------------------------------------------------------------*/
// Clear to End of Line
void clreol ()
{

    COORD coord;
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                                 &info );
    coord.X = info.dwCursorPosition.X;
    coord.Y = info.dwCursorPosition.Y;

    FillConsoleOutputCharacter ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                                 ' ', info.dwSize.X - info.dwCursorPosition.X, coord, &written );
    gotoxy ( coord.X, coord.Y );
}

/*--------------------------------------------------------------------------------*/
// Where X
int wherex()
{

    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo ( GetStdHandle ( STD_OUTPUT_HANDLE ), &info );
    return info.dwCursorPosition.X + 1;
}

/*--------------------------------------------------------------------------------*/
// Where Y
int wherey()
{

    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo ( GetStdHandle ( STD_OUTPUT_HANDLE ), &info );
    return info.dwCursorPosition.Y + 1;
}

/*--------------------------------------------------------------------------------*/
// Background Color
void bgcolor ( int color )
{

    __BACKGROUND = color;
    SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                              __FOREGROUND + ( color << 4 ) );
}

/*--------------------------------------------------------------------------------*/
// Foreground Color
void color ( int color )
{

    __FOREGROUND = color;
    SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ),
                              color + ( __BACKGROUND << 4 ) );
}

/*--------------------------------------------------------------------------------*/
// Text Attribute
void textattr ( int _attr )
{

    SetConsoleTextAttribute ( GetStdHandle ( STD_OUTPUT_HANDLE ), _attr );
}


/*--------------------------------------------------------------------------------*/
// GUI Screen Drawing Function for Windows
/*--------------------------------------------------------------------------------*/


// Message Status Window


/*--------------------------------------------------------------------------------*/
// 1st Line in Message Window
void drawinit ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 5;
    point.Y = 10;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// 2nd Line in Message Window
void drawinit2 ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 7;
    point.Y = 11;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// 3rd Line in Message Window
void drawinit3 ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 5;
    point.Y = 13;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// 4th Line in Message Window
void drawinit4 ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 7;
    point.Y = 14;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// 5th Line in Message Window
void drawinit5 ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 7;
    point.Y = 15;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// 5th Line in Message Window
void drawinit6 ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 7;
    point.Y = 16;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                                          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 7 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Remote Users IP in the GUI
void drawip ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 27;
    point.Y = 3;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "               " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Filename in GUI
void drawfilename ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 9;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                                             " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Filesize in GUI
void drawfilesize ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 10;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Bytes Received in GUI
void drawreceived ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 11;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Bytes Left in GUI
void drawleft ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 12;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Speed in GUI
void drawspeed ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 49;
    point.Y = 12;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                   " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Time Left in GUI
void drawtleft ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 14;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Time Elapsed in GUI
void drawtelapse ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 23;
    point.Y = 13;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "          " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Draws Time Left in GUI
void queuestatus ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 49;
    point.Y = 13;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                   " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Transfer Status
void drawstatus ( char *text, int fc, int bc )
{

    COORD point;
    color ( fc );
    bgcolor ( bc );
    point.X = 49;
    point.Y = 14;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "                   " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( text );
    color ( 0 );
    bgcolor ( 0 );
    return;
}


/*--------------------------------------------------------------------------------*/
// Main Title Portion for text in GUI
void drawpercent ( long double percentage )
{

    COORD point;
    color ( 0 );
    bgcolor ( 7 );
    point.X = 64;
    point.Y = 10;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "    " );
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );
    printf ( "%.f%%", ( float ) percentage );
    color ( 0 );
    bgcolor ( 0 );
    return;
}

/*--------------------------------------------------------------------------------*/
// Top Percentage Bar
void percenttop ( long double percentage )
{

    COORD point;
    color ( 11 );
    bgcolor ( 3 );
    point.X = 41;
    point.Y = 10;
    SetConsoleCursorPosition ( GetStdHandle ( STD_OUTPUT_HANDLE ), point );

    char per1[21]= {0};
    char per2[21]= {0};

    per2[0] = 176;
    per2[1] = 177;
    per2[2] = 178;
    per2[3] = 176;
    per2[4] = 177;
    per2[5] = 178;
    per2[6] = 176;
    per2[7] = 177;
    per2[8] = 178;
    per2[9] = 176;
    per2[10] = 177;
    per2[11] = 178;
    per2[12] = 176;
    per2[13] = 177;
    per2[14] = 178;
    per2[15] = 176;
    per2[16] = 177;
    per2[17] = 178;
    per2[18] = 176;
    per2[19] = 177;

    if ( percentage < 10 )
    {
        sprintf ( per1, "%c" ,per2[0] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 20 )
    {
        sprintf ( per1,"%c%c%c",per2[0],per2[1],per2[2] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 30 )
    {
        sprintf ( per1,"%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 40 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 50 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 60 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 70 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11],per2[12],per2[13] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 80 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11],per2[12],per2[13],per2[14],per2[15] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 90 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11],per2[12],per2[13],per2[14],per2[15],per2[16],per2[17] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage < 100 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11],per2[12],per2[13],per2[14],per2[15],per2[16],per2[17],per2[18] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    else if ( percentage = 100 )
    {
        sprintf ( per1,"%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",per2[0],per2[1],per2[2],per2[3],per2[4],per2[5],per2[6],per2[7],per2[8],per2[9],per2[10],per2[11],per2[12],per2[13],per2[14],per2[15],per2[16],per2[17],per2[18],per2[19] );
        printf ( per1 );
        color ( 0 );
        bgcolor ( 0 );
        return;
    }
    return;
}

#endif

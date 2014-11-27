#ifndef __WINCON_H    /*  An extra safeguard to prevent this header from  */
#define __WINCON_H    /*  being included twice in the same source file    */

using namespace std;

#ifdef __cplusplus
extern "C" {
#endif

char*	_cgets ( char* );
int	_cprintf ( const char*, ... );
int	_cputs ( const char* );
int	_cscanf ( char*, ... );

int	_getch ( void );
int	_getche ( void );
int	_kbhit ( void );
int	_putch ( int );
int	_ungetch ( int );


int	getch ( void );
int	getche ( void );
int	kbhit ( void );
int	putch ( int );
int	ungetch ( int );


#ifdef __cplusplus
}
#endif

/*--------------------------------------------------------------------------------*/
// Function for Printing out to the screen
void setcolor ( int fg, int bg );

void clrscr();
void gotoxy ( int, int );
void delline();
void clreol ();
void setrgb ( int );
int wherex();
int wherey();
void bgcolor ( int color );
void color ( int color );
void textattr ( int _attr );



/*--------------------------------------------------------------------------------*/
// GUI Screen Drawing Function for Windows
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
// Main Title Portion for text in GUI
void drawtop ( char *text, int fc, int bc, int x );

/*--------------------------------------------------------------------------------*/
// Draws Remote Users IP in the GUI
void drawip ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Filename in GUI
void drawfilename ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Filesize in GUI
void drawfilesize ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Bytes Received in GUI
void drawreceived ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Bytes Left in GUI
void drawleft ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Speed in GUI
void drawspeed ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Time Left in GUI
void queuestatus ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Time Left in GUI
void drawtleft ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Draws Time Elapsed in GUI
void drawtelapse ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Transfer Status
void drawstatus ( char *text, int fc, int bc );

/*--------------------------------------------------------------------------------*/
// Main Title Portion for text in GUI
void drawpercent ( long double percentage );

/*--------------------------------------------------------------------------------*/
// Top Percentage Bar
void percenttop ( long double percentage );


#endif

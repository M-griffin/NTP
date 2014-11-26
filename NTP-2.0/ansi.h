#ifndef __ANSI_H    /*  An extra safeguard to prevent this header from  */
#define __ANSI_H    /*  being included twice in the same source file    */

/*--------------------------------------------------------------------------------*/
// Ansi Parser - Michael Griffin (c) 2003
// Inspired Saeger's Source Code.
/*--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------*/
// Structure for Keeping track of Screen Colors
/*--------------------------------------------------------------------------------*/
typedef struct
{
  	char fc;
   	char bc;
  	char addi;

} SCREENINFO;


/*--------------------------------------------------------------------------------*/
// Ansi Parsing Function
/*--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------*/
// Main Ansi Parsing Function
void ansiparse(char *buff);

#endif

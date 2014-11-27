/*

 Server.cpp NTP-USER 1.10.1B
 09-20-03 + Windows Port Completed!
 10-01-03 + Ported Pthreads to Windows for Cross platform
          - Porting General Sockets & Console to OS/2 And Linux
 10-24-03 + Added String Encryption to Resume Function so Far
 11-28-03 + Fixed some Minor Bugs and updated to 1.8b
 01-04-04 + Fix GUI HH[0] = 0 oops..
 01-12-04 + Rebuilding to v1.08, Fixed Hour Bug in Transfer GUI
          - added new file transfer GUI. Cleared Up Source Code.
          - Adding extra Error Checking WSAGetLastError()
          - Finished all tweaks and code clean up's
 01-21-04 + working on 1.09b update and fixes, adding ask port to init startup
          - Fixed Retries on all Send() Recv() functions,
          - Also fixed end of file buffering
 01-24-04 - Few more fixes, added end of transfer exit program,
            broke off Invalid Socket and Socket Error, both were -1,
            added LOST_SOCKET = 0 to keep track of disconnections
          - Add check for receiving filesize of 0, exit!
 02-18-04 + Working on 2.10.1 Update, Fixed Accept Error message on Exit,
          - Need to Fix Exit on File already found! make it goto next file Blah!
          - Also Addin 120 Second Timeout needed for Linux end..

          For linux you must link  g++  -lpthread -static
*/

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "config.h"

#ifdef _WIN32
#include <process.h>
#include <winsock.h>
#include "ansi.h"
#include "wincon.h"

#else  // Linux & OS/2
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>

#ifndef OS2 // Just linux
#include <curses.h>
#include <arpa/inet.h>
#endif

#include <sys/socket.h>
#include <netdb.h>

typedef int SOCKET;
typedef sockaddr_in SOCKADDR_IN;
#define INVALID_SOCKET -1
#endif

// Disconnected
#define LOST_SOCKET     0

typedef struct
{
    char     bbsver[10];           // Client Version
    char     filename[255];        // File Name
    char     bqueue[255];          // Batch Queue 1/4 etc..
    long int size;                 // File Size in Bytes
    long int bRecv;                // Bytes Received
    long int lRecv;                // Last Bytes Received
    long int bSec;                 // Bytes Received Each Second
    long int speed;                // Transfer Speed
    char     status[50];           // Transfer Status
    char     telapsed[50];         // Time Elasped   [Buffer]
} FILEINFO;

FILEINFO finfo;                    // Handle to File Info
FILEINFO finfo2;                   // Handle to File Info After Tranfer, Message Window

struct sockaddr_in their_addr;

typedef struct
{
    SOCKET rsock;
    SOCKET lsock;
} MSOCK;

MSOCK msock;                       // Handle to Sockets

long flsz;          	           // Local Filesize for Resume
bool resum;                        // Resume True/ flase
bool erbreak;                      // error break for gui

char szDateFormat[128];		       // System Date
char szTimeFormat[128];			   // System Time

char DLPATH[255]= {0};             // Download Path
char PATH[255]  = {0};             // Programs Path
bool LOGGING    = true;            // Transfer / Error Logging
bool tstate     = false;           // Transfer State
bool F1         = false;           // Togle Exit After Transfer
bool e1         = false;           // Program Exit

#ifdef _WIN32
// Inital Startup Screen with Message Window
char *aBuf1= {"[40m[2J[13;40H[0mÛÛÛ[4DÛ[3CÛ[12;41HÛ[14;41HÛ[13;38HÛ[5CÛ[12;39HÛÛ[CÛÛ[14;39HÛÛ[CÛÛ[13;37HÛ[7CÛ[12;38HÛ[5CÛ[14;38HÛ[5CÛ[11;41HÛ[15;41HÛ[13;36HÛ[9CÛ[12;37HÛ[7CÛ[14;37HÛ[7CÛ[11;39HÛÛ[CÛÛ[15;39HÛÛ[CÛÛ[13;35HÛ[11CÛ[12;36HÛ[9CÛ[14;36HÛ[9CÛ[11;37HÛÛ[5CÛÛ[15;37HÛÛ[5CÛÛ[10;41HÛ[16;41HÛ[13;34HÛ[13CÛ[12;35HÛ[11CÛ[14;35HÛ[11CÛ[11;36HÛ[9CÛ[15;36HÛ[9CÛ[10;38HÛÛÛ[CÛÛÛ[16;38HÛÛÛ[CÛÛÛ[13;33HÛ[15CÛ[12;34HÛ[13CÛ[14;34HÛ[13CÛ[11;35HÛ[11CÛ[15;35HÛ[11CÛ[10;36HÛÛ[7CÛÛ[16;36HÛÛ[7CÛÛ[9;41H[1;47mÄ[17;41H[0mÛ[13;32HÛ[17CÛ[12;33HÛ[15CÛ[14;33HÛ[15CÛ[11;33HÛÛ[13CÛÛ[15;33HÛÛ[13CÛÛ[10;35HÛ[11CÛ[16;35HÛ[11CÛ[9;37H[1;47mÄÄÄÄ[CÄÄÄÄ[17;37H[0mÛÛÛÛ[CÛÛÛÛ[13;31HÛ[19CÛ[12;32HÛ[17CÛ[14;32HÛ[17CÛ[11;32HÛ[17CÛ[15;32HÛ[17CÛ[10;34HÛ[13CÛ[16;34HÛ[13CÛ[9;36H[1;47mÄ[9CÄ[17;36H[0mÛ[9CÛ[8;41HÛ[18;41H[1;47m [13;30H[0mÛ[21CÛ[12;31HÛ[19CÛ[14;31HÛ[19CÛ[11;31HÛ[19CÛ[15;31HÛ[19CÛ[10;32HÛÛ[15CÛÛ[16;32HÛÛ[15CÛÛ[9;34H[1;47mÄÄ[11CÄÄ[17;34H[0mÛÛ[11CÛÛ[8;37HÛÛÛÛ[CÛÛÛÛ[18;37H[1;47m    [C    [13;29H[0mÛ[23CÛ[12;30HÛ[21CÛ[14;30HÛ[21CÛ[11;30HÛ[21CÛ[15;30HÛ[21CÛ[10;31HÛ[19CÛ[16;31HÛ[19CÛ[9;33H[1;47mÄ[15CÄ[17;33H[0mÛ[15CÛ[8;35HÛÛ[9CÛÛ[18;35H[1;47m  [9C  [7;41H[30;40mß[19;41H[0;30;47mÄ[13;28H[37;40mÛ[25CÛ[12;29HÛ[23CÛ[14;29HÛ[23CÛ[11;29HÛ[23CÛ[15;29HÛ[23CÛ[10;30HÛ[21CÛ[16;30HÛ[21CÛ[9;31H[1;47mÄÄ[17CÄ´[17;31H[0mÛÛ[17CÛÛ[8;33HÛÛ[13CÜÛ[18;33H[1;47m  [13C  [7;37H[30;40mßßßß[Cßßßß[19;37H[0;30;47mÄÄÄÄ[CÄÄÄÄ[13;27H[37;40mÛ[27CÛ[12;28HÛ[25CÛ[14;28HÛ[25CÛ[11;28HÛ[25CÛ[15;28HÛ[25CÛ[10;29HÛ[23CÛ[16;29HÛ[23CÛ[9;30H[1;47mÄ[21C[35;45m [17;30H[0mÛ[21CÛ[8;32HÛ[17CÛ[18;32H[1;47m [17C [7;34H[30;40mßßß[9Cßß[19;34H[0;30;47mÄÄÄ[9CÄÄÄ[20;41H[1;37;44m)[13;26H[0mÛ[29CÛ[12;27HÛ[27CÛ[14;27HÛ[27CÛ[11;27HÛ[27CÛ[15;27HÛ[27CÛ[10;28HÛ[25CÛ[16;28HÛ[25CÛ[9;29H[1;47mÄ[23C[45mM[17;29H[0mÛ[23CÛ[8;30HÜÛ[19CÛÛ[18;30H[1;47m  [19C  [7;33H[30;40mß[19;33H[0;30;47mÄ[15CÄ[20;36H[37;44mi[1;34mt[37m ( [C A[0;44mft[1;34me[13;25H[0mÛ[31CÛ[12;26HÛ[29CÛ[14;26HÛ[29CÛ[11;26HÛ[29CÛ[15;26HÛ[29CÛ[10;27HÛ[27CÛ[16;27HÛ[27CÛ[9;28H[1;47mÄ[25C[0;45me[17;28H[40mÛ[25CÛ[8;29HÜ[23CÛ[18;29H[1;47m [23C [7;31H[30;40mßß[18Cß[19;31H[0;30;47mÄÄ[17CÄÄ[20;34H[1;37;44mE[0;44mx[11C[1;34mr[37m [5;41H[34;47m²[21;41H[0mß[13;24HÛ[33CÛ[12;25HÛ[31CÛ[14;25HÛ[31CÛ[11;25HÛ[31CÛ[15;25HÛ[31CÛ[10;26HÛ[29CÛ[16;26HÛ[29CÛ[9;27H[1;47mÄ[27C[0;45ms[17;27H[40mÛ[27CÛ[8;54HÛ[18;28H[1;47m [25C [7;30H[30;40mß[21Cßß[19;29H[0;30;47mÄÄ[21CÄÄ[6;49H[1;34;44mß[0;34mß[20;32H[1;44me[37m [15CT[0;44mr[5;39H[1;34;40mÜÛ[C[47mÛ[40mÛÛ[47m²²[21;36H[0mßßßßß[Cßßßßß[13;23HÛ[35CÛ[12;24HÛ[33CÛ[14;24HÛ[33CÛ[11;24HÛ[33CÛ[15;24HÛ[33CÛ[10;25HÛ[31CÛ[16;25HÛ[31CÛ[9;25H[1;47mÄÄ[29C[35;45msa[17;25H[0mÛÛ[29CÛÛ[8;55HÛ[18;27H[1;47m [27C [7;28H[30;40mß[25Cß[19;28H[0;30;47mÄ[25CÄ[20;30H[1;34;44mgl[19C[0;44ma[1;34mn[5;47H[40mÛÛÛ[21;33H[0mßßß[11Cßßß[4;41H[1;34mÜ[22;41H[30mß[13;22H[0mÛ[37CÛ[12;23HÛ[35CÛ[14;23HÛ[35CÛ[11;23HÛ[35CÛ[15;23HÛ[35CÛ[10;24HÛ[33CÛ[16;24HÛ[33CÛ[9;24H[1;47mÄ[33C[35;45mg[17;24H[0mÛ[33CÛ[8;56HÛÛ[18;25H[1;47m  [29C  [7;27H[30;40mÛ[27Cß[19;27H[0;30;47mÄ[27CÄ[6;29H[1;40mÝ[20;29H[0;44mg[23C[1;34ms[5;50H[40mÛ²[21;31H[0mßß[17Cßß[4;38H[1;30msx[2C[34mÛ[44mß[0;34mß[22;36H[1;30mßßßßß[Cßßßßß[13;21H[0mÛ[39CÛ[12;22HÛ[37CÛ[14;22HÛ[37CÛ[11;22HÛ[37CÛ[15;22HÛ[37CÛ[10;22HÛÛ[35CÛÛ[16;22HÛÛ[35CÛÛ[9;23H[1;47mÄ[35C[35;45me[17;23H[0mÛ[35CÛ[8;58HÛ[18;24H[1;47m [33C [7;26H[30;40mÛ[29Cß[19;26H[0;30;47mÄ[29CÄ[6;27H[1m±[40mÛ[20;27H[37;44mT[0;44mo[25C[1;34mfe[5;52H[0;34m±[21;30H[37mß[21Cß[4;49H[1;34mÛ[22;33H[30mßßß[11Cßßß[3;41H[34;47m²[13;20H[0mÛ[41CÛ[12;21HÛ[39CÛ[14;21HÛ[39CÛ[11;21HÛ[39CÛ[15;21HÛ[39CÛ[10;21HÛ[39CÛ[16;21HÛ[39CÛ[9;22H[1;47mÄ[37C[35;45m [17;22H[0mÛ[37CÛ[8;23HÜ[35CÛ[18;23H[1;47m [35C [7;24H[30;40mßÛ[31Cßß[19;24H[0;30;47mÄÄ[31CÄÄ[6;26H[1m±[20;26H[37;44m [29C[34mr[5;28H[30;40mÜ[25C[0mu[21;28Hßß[23Cßß[4;50H[1;34mÜ[22;31H[30mßß[17Cßß[3;35H[34mÛÛÛ[47m²²²[C[40mÛÛÛ[44mß[0;34mß[13;19H[37mÛ[43CÛ[12;20HÛ[41C[30;47m [14;20H[37;40mÛ[41CÛ[11;20HÛ[41CÛ[15;20HÛ[41CÛ[10;20HÛ[41CÛ[16;20HÛ[41CÛ[9;21H[1;47mÄ[39C[45mW[17;21H[0mÛ[39CÛ[8;22HÜ[37CÛ[18;22H[1;47m [37C [7;59H[30;40mß[19;23H[0;30;47mÄ[35CÄ[6;25H[1m²[20;25H[37;44m=[31C[30mÞ[5;27H[47mß[27C[0ms[21;27Hß[27Cß[22;29H[1;30mßß[21Cßß[3;33H[34mÛ[47m²[2;41H[37;40mo[13;18H[0mÛ[45CÛ[12;19H[30;47m [43C[37;40mÛ[14;19HÛ[43CÛ[11;19HÛ[43CÛ[15;19HÛ[43CÛ[10;19HÛ[43CÛ[16;19HÛ[43CÛ[9;20H[1;47mÄ[41C[0;45mi[17;20H[40mÛ[41CÛ[8;21HÜ[39CÛ[18;21H[1;47m [39C [7;22H[30;40mß[37Cß[19;22H[0;30;47mÄ[37CÄ[6;23H[1;40mÛ[47m²[20;23H[37;44m1 [33C[47mÃ[30mÄ[5;25HÛß[29C[0min[21;25Hßß[29Cßß[22;27H[1;30mßß[25Cßß[3;30H[34mÛÛÛ[2;35H[37mfer[Cpr[Ctocol[13;17H[0mÛ[47CÛ[12;18HÛ[45CÛ[14;18HÛ[45CÛ[11;18HÛ[45CÛ[15;18HÛ[45CÛ[10;18HÛ[45CÛ[16;18HÛ[45CÛ[9;19H[1;47mÄ[43C[0;45mn[17;19H[40mÛ[43CÛ[8;20HÜ[41CÛ[18;20H[1;47m [41C [7;21H[30;40mß[39Cß[19;21H[0;30;47mÄ[39CÄ[6;22H[1;40mÛ[20;22H[37;44mF[37C[30;47mÄ[5;24H[40mÜ[33C[0mg[21;24Hß[33Cß[22;26H[1;30mß[29Cß[3;28H[34mÛÛ[24C[30mÄ[2;32H[37mans[13;16H[0mÛ[49CÛ[12;17HÛ[47CÛ[14;17HÛ[47CÛ[11;17HÛ[47C[30;47m [15;17H[37;40mÛ[47CÛ[10;17HÛ[47C[1;47m [16;17H[0mÛ[47CÛ[9;18H[1;47mÄ[45C[35;45md[17;18H[0mÛ[45CÛ[8;19HÛ[43CÛ[18;19H[1;47m [43C [7;62H[30;40mß[19;20H[0;30;47mÄ[41CÄ[6;21H[1mß[20;21H[37;44mÝ[39C[0;30;47mÄ[5;22H[1mÛ[40mß[36C[0mp[21;22Hßß[35Cßß[4;24H[1;30mÜ[22;24Hßß[31Cßß[3;27H[34mÛ[27C[0mÄ[2;30H[1mtr[13;15H[0mÛ[51CÛ[12;16HÛ[49CÛ[14;16HÛ[49CÛ[11;16HÛ[49C[30;47m [15;16H[37;40mÛ[49CÛ[10;16HÛ[49C[1;47m [16;16H[0mÛ[49CÛ[9;17H[1;47mÄ[47C[35;45mo[17;17H[0mÛ[47CÛ[8;18HÛ[45CÛ[18;18H[1;47m [45C [7;63H[30;40mß[19;19H[0;30;47mÄ[43CÄ[6;20H[1;40mÜ[20;20H[0;30;47m [41CÄ[5;21H[1mÛ[39C[0mo[21;21Hß[39Cß[4;23H[1;30mÜ[22;23Hß[35Cß[3;25H[34;44m²[40mÛ[29C[37mÄÄ[2;28Hs[25C[30m([13;14H[0mÛ[53CÛ[12;15HÛ[51CÛ[14;15HÛ[51CÛ[11;15HÛ[51C[30;47m [15;15H[37;40mÛ[51CÛ[10;15HÛ[51C[1;47m [16;15H[0mÛ[51CÛ[9;16H[1;47mÄ[49C[35;45mw[17;16H[0mÛ[49CÛ[8;16HÜÜ[47CÛÛ[18;16H[1;47m  [47C  [7;17H[30;40mß[46Cßß[19;17H[0;30;47mÄÄ[45CÄÄ[20;19H[1;44mÞ[43C[0;30;47mÄ[5;20H[1;40mÛ[41C[0mr[21;20Hß[41Cß[4;21H[1;30mÜÜ[22;21Hßß[37Cßß[3;23H[34;44m±²[33C[37;40mÄÄ[2;26Hsi[27Cc[30m)[13;13H[0mÛ[55C[30;47m³[12;14H[37;40mÛ[53CÛ[14;14HÛ[53CÛ[11;14HÛ[53C[30;47m [15;14H[37;40mÛ[53CÛ[10;14HÛ[53C[1;47m [16;14H[0mÛ[53CÛ[9;15H[1;47mÄ[51C[35;45m [17;15H[0mÛ[51CÛ[8;15HÜ[51CÛ[18;15H[1;47m [51C [7;66H[30;40mß[19;16H[0;30;47mÄ[49CÄ[6;17H[1;40mßß[20;17H[0;44mi[1;34mt[45C[0;30;47mÄÄ[5;19H[1m²[43C[0mt[21;19Hß[43Cß[4;20H[1;30mÜ[22;20Hß[41Cß[3;22H[34;44m°[37C[37;40mÄ[2;24Hme[32C[30mM[13;12H[0mÛ[57CÛ[12;13HÛ[55C[30;47m³[14;13H[37;40mÛ[55C[30;47m³[11;13H[37;40mÛ[55C[30;47m³[15;13H[37;40mÛ[55C[30;47m³[10;13H[37;40mÛ[55C[30;47m³[16;13H[37;40mÛ[55C[30;47m³[9;14H[1;37mÄ[53C[0;30;47mÃ[17;14H[37;40mÛ[53CÛ[8;14HÛ[53CÛ[18;14H[1;47m [53C [7;67H[30;40mß[19;15H[0;30;47mÄ[51CÄ[6;16H[1;40mß[20;16H[0;44mx[49C[30;47mÄ[5;17H[1;40mÜÛ[21;17H[0mßß[45Cßß[4;19H[1;30mÜ[22;19Hß[43Cß[3;21H[0;34mÜ[39C[1;37m´[2;23He[35C[0;36me[13;11H[37mÛ[1AÛ[57CÛ[14;12HÛ[57CÛ[11;12HÛ[57CÛ[15;12HÛ[57CÛ[10;12HÛ[57CÛ[16;12HÛ[57CÛ[9;13H[1;47mÄ[55C[0;30;47m¿[17;13H[37;40mÛ[55C[30;47m³[8;13H[37;40mÛ[55CÛ[18;13H[1;47m [55C[0;30;47m³[7;14H[1;40mß[53Cß[19;14H[0;30;47mÄ[53CÄ[6;15H[1;40mÜ[20;15H[37;44mE[51C[0;30;47mÄ[5;16H[1;40mÜ[21;16H[0mß[49Cß[4;18H[1;30mÜ[22;18Hß[45Cß[3;19H[37mß[43C[30mA[2;22H[37mn[37C[0;36mr[1mc[13;10H[0mÛ[61C[1;30mÛ[12;11H[0mÛ[14;11HÛ[11;11HÛ[15;11HÛ[10;11HÛ[16;11HÛ[7A[1;47mÄ[57C[0mÛ[17;12HÛ[57CÛ[8;12HÛ[57CÛ[18;12H[1;47m [57C[0mÛ[7;13H[1;30mß[55Cß[19;13H[0;30;47mÄ[55CÙ[6;14H[1;40mÛ[20;14H[37;44m [53C[0;30;47mÄ[5;15H[1;40mÜ[21;15H[0mß[51Cß[4;16H[1;47mÝ[22;16H[30;40mßß[47Cßß[3;18H[37mÛ[45C[0;36mc[2;20H[1;37m°[41C[36my[13;9H[0mÛ[1AÛ[61C[1;30mÛ[14;10H[0mÛ[61C[1;30mÛ[11;10H[0mÛ[61C[1;30mÛ[15;10H[0mÛ[61C[1;30mÛ[10;10H[0mÛ[61C[1;30mÛ[16;10H[0mÛ[61C[1;30mÛ[9;11H[37;47mÄ[17;11H[0mÛ[8;11HÛ[18;11H[1;47m [7;70H[30;40mß[19;12H[0;30;47mÄ[57C[37;40mÛ[6;13H[1;30mÛ[20;13H[37;44m=[55C[0;30;47mÄ[5;14H[1;40mÜ[21;14H[0mß[53Cß[4;15H[1mÛ[22;15H[30mß[51Cß[3;17H[37mÜ[47C[0;36mi[2;18H[1;37mÛ²[43Cfu[13;8H[0mÛ[1AÛ[14;9HÛ[11;9HÛ[15;9HÛ[10;9HÛ[16;9HÛ[7A[1;47mÄ[61C[30;40mÛ[17;10H[0mÛ[61C[1;30mÛ[8;10H[0mÛ[61C[1;30mÛ[18;10H[37;47m [61C[30;40mÛ[7;11H[0mÜ[59C[1;30mß[19;11H[0;30;47mÄ[13A[1;40mÜ[20;12H[37;44m [57C[0mÛ[5;13H[1;30mÜ[21;13H[0mß[55Cß[22;14H[1;30mß[53Cß[3;15H[37mÜÜ[49C[36md[37mi[2;17HÛ[47Cl[13;7H[0mÛ[1AÛ[14;8HÛ[11;8HÛ[15;8HÛ[10;8HÛ[16;8HÛ[9;8H[1;47mÄÄ[17;8H[0mÛÛ[8;9HÛ[18;9H[1;47m [11A[0mÜ[61C[1;30mÛ[19;10H[0;30;47mÄ[61C[1;40mÛ[20;11H[37;44mC[15A[30;40mÜ[21;12H[0mß[57Cß[4;13H[1;47mÝ[22;13H[30;40mß[55Cß[3;68H[37mc[2;16HÛ[1AÛÜ[13;6H[0mÛ[1AÛ[14;7HÛ[11;7HÛ[15;7HÛ[10;7HÛ[16;7HÛ[8AÛ[18;8H[1;47m [11A[0mÜ[19;9H[30;47mÄ[B[1;37;44mS[61C[30;40mÛ[5;11HÜ[21;11H[0mß[17A[1mÛ[22;11H[30mßß[57Cßß[3;13H[37;47mÝ[1A[40mÛÛ[51CFa[13;5H[0;30;47m [1A[37;40mÛ[14;6HÛ[11;6HÛ[15;6HÛ[10;6HÛ[16;6HÛ[9;6H[1;47mÄÄ[17;6H[0mÛÛ[8;7HÛ[18;7H[1;47m [11A[0mÜ[19;8H[30;47mÄ[6;8H[1;40m°°[20;8H[37;44mÝE[21;9H[0mßß[61C[1;30mÛ[4;10H[0mÝ[22;10H[1;30mß[61Cß[3;12H[37mÛ[57CÃ[2;13HÛ[55Ct[13;4H[47m³[1A[0;30;47m [14;5H [11;5H [15;5H [10;5H [16;5H [9;5H[1;37mÄ[17;5H[0;30;47m [9A[37;40mÛ[18;6H[1;47m [11A[0mÜ[19;7H[30;47mÄ[20;7H[1;37m´[B[0mß[17A[1mÛ[22;9H[30mß[19A[0mÝ[1;34m°[59C[37mÄÄ[2;12HÛ[57Ce[13;3H[0mÛ[1A[1;47m³[14;4H³[11;4H³[15;4H³[10;4H³[16;4H³[9;4HÚ[17;4H³[9A[0mÛ[18;5H[30;47m [11A[37;40mÜ[19;6H[30;47mÄ[6;6H[1;40m°[20;6H[47mÄ[B[0mß[17A[1;47mÛ[22;8H[30;40mß[19A[37mÛ[63CÄ[2;11HÛ[12;3H[0mÛ[14;3HÛ[11;3H[1;30;47m [15;3H[0mÛ[10;3HÛ[16;3HÛ[9;3HÛ[17;3HÛ[9AÛ[18;4H[1;47m³[7;4H[0mÛÜ[19;4H[1;47mÀ[0;30;47mÄ[20;5H[1mÄ[B[0mß[17A[1mß[22;7H[30mß[19A[37;47mÛ[65C[40mÄ[2;10HÛ[61C2[13;1H[30;47m²[8;3H[0mÛ[18;3HÛ[7;3HÛ[19;3HÛ[13A[1;30mß[20;4H[0;30;47mÄ[15A[1;37;40mß[21;5H[0mß[B[1;30mß[19A[37mÛ[67C[0mÄ[2;8H[1;47mÛ[40mÛ[63C[0mk[1m3[12;1H[30mÛ[14;1H[47m±[11;1H²[15;1H[40mÛ[10;1HÛ[16;1HÛ[9;1HÛ[17;1HÛ[6;3Hß[20;3H[0mÛ[15A[1mß[21;4H[0mß[17A[1mÝ[22;5H[30mß[19A[37mÛ[69C[30mÄ[2;75H/[8;1HÛ[18;1H[47m²[7;1HÛ[19;1H[40mÛ[13Aß[1A[37mß[21;3H[0mß[17A[1mÛ[22;4H[30mß[19A[37mß[1AÜ[69C4[6;1H[30mÛ[20;1HÛ[4;3H[37mÛ[22;3H[30mß[19A[37mÛ[1AÛ[21;1H[30mÛ[Bß[19A[37mÛ[2;3HÛÛ[22;1H[30mß[3;1H[34;44m°[1;1H[37;40m [0m"};

// Transfer Window with Statistics
char *aBuf2= {"[40m[2J[2B[0;1;34;44m°[2;4H[37;40mÛ[2DÛ[3;3HÛ[2;5HÛ[3;4HÛ[4;3HÛ[6;1H[30mÛ[2;76H[37m4[71DÜ[3;5Hß[4;4HÛ[5;3Hß[6;2H[30mß[19;2Hß[2Dß[7;1H[47mÛ[18;1H[40mÛ[8;1HÛ[2;75H/[BÄ[71D[37mÛ[4;5HÝ[5;4Hß[6;3H[30mß[17;1HÛ[9;1HÛ[16;1HÛ[10;1HÛ[15;1H[47m²[11;1H²[14;1H±[12;1H[40mÛ[2;74H[37m3[2D[0mk[65D[1mÛ[2D[47mÛ[3;75H[0mÄ[69D[1mÛ[5;5Hß[6;4H[30mß[19;3Hß[7;3H[0mÛ[18;3Hß[8;3HÛ[13;1H[1;30;47m²[2;72H[37;40m2[63DÛ[3;74HÄ[67D[47mÛ[4;7H[40mß[19;5H[30mß[2Dß[12A[0mÜ[2DÛ[18;4Hß[8;4HÛ[17;3HÛ[9;3HÛ[16;3HÛ[10;3HÛ[15;3HÛ[11;3H[1;30;47m [14;3H[0mÛ[12;3HÛ[2;11H[1mÛ[3;73HÄ[65DÛ[4;8H[47mÛ[6;6H[30;40m°[19;6Hß[7;6H[0mÜ[18;5Hß[8;5HÛ[17;4H[30;47mÄ[9;4H[1;37mÚ[16;4HÀ[10;4H³[15;4H³[11;4H³[14;4H³[12;4H³[13;3H[0mÛ[2;70H[1me[59DÛ[3;72HÄ[2DÄ[61D[34m°[2D[0mÝ[4;9H[1mÛ[19;7H[30mß[7;7H[0mÜ[18;6Hß[8;6HÛ[17;5H[1;30;47mÄ[9;5H[37mÄ[16;5H[0;30;47mÄ[10;5H [15;5H [11;5H [14;5H [12;5H [13;4H[1;37m³[2;69H[40mt[57DÛ[3;70HÃ[59DÛ[4;10H[0mÝ[6;9H[1;30m°[2D°[19;8Hß[7;8H[0mÜ[18;7Hß[8;7HÛ[17;7H[1;47m´[2D[30mÄ[8A[37mÄ[2DÄ[16;6H[0;30;47mÄ[10;6H[1;37mf[15;6H[0;30;47mÀ[11;6H[1;37mf[14;6Ht[12;6Hb[13;5H[0;30;47m [2;68H[1;37;40ma[2DF[53DÛ[2DÛ[3;13H[47mÝ[4;12H[40mÛ[5;11H[30mÜ[19;9Hß[7;9H[0mÜ[18;8Hß[8;8HÛ[16;7H[30;47mÄ[10;7H[1;37mi[15;7H[0;30;47m-[11;7H[1;37mi[14;7Hi[12;7Hy[13;6H[0;30;47mÀ[1;18H[1;37;40mÜ[2DÛ[2;16HÛ[3;68Hc[4;13H[47mÝ[5;12H[30;40mÜ[19;72Hß[63Dß[7;72HÛ[63D[0mÜ[18;9Hß[8;9HÛ[17;9H[1;44mE[2DÝ[8A[47mÄ[2DÄ[16;8H[0;30;47mÄ[10;8H[1;37ml[15;8Hl[11;8Hl[14;8Hm[12;8Ht[13;7H[0;30;47m-[2;65H[1;37;40ml[49DÛ[3;67Hi[2D[36md[51D[37mÜ[2DÜ[5;13H[30mÜ[6;12HÜ[19;71Hß[61Dß[7;71Hß[61D[0mÜ[18;72H[1;30mÛ[63D[0mß[8;72H[1;30mÛ[63D[0mÛ[17;72H[1;30mÛ[63D[37;44mS[9;72H[30;40mÛ[63D[37;47mÄ[16;9H[0;30;47mÄ[10;9H[1;37me[15;9He[11;9He[14;9He[12;9He[13;8Hl[2;64H[40mu[2Df[45D²[2DÛ[3;65H[0;36mi[49D[1;37mÜ[4;15HÛ[5;14H[30mÜ[6;13HÛ[19;70Hß[59Dß[7;70Hß[18;11H[0mß[8;11HÛ[17;11H[1;44mC[9;11H[47mÄ[16;72H[30;40mÛ[63D[0;30;47mÄ[10;72H[1;40mÛ[63D[37;47mn[15;72H[30;40mÛ[63D[37;47mf[11;72H[30;40mÛ[63D[37;47ms[14;72H[30;40mÛ[63D[37;47m [12;72H[30;40mÛ[63D[37;47ms[13;9He[2;62H[36;40my[43D[37m°[3;64H[0;36mc[47D[1;37mÛ[4;16H[47mÝ[5;15H[30;40mÜ[6;14HÛ[19;69Hß[57Dß[7;69Hß[57Dß[18;70H[0mß[59Dß[8;70HÛ[59DÛ[17;70HÛ[59D[1;44m [9;70H[0mÛ[59D[1;47mÄ[16;11H[0;30;47mÄ[10;11H[1;37ma[15;11Ht[11;11Hi[14;11He[12;11H [13;72H[30;40mÛ[63D[37;47mf[2;61H[36;40mc[2D[0;36mr[39D[1;37mn[3;63H[30mA[45D[37mß[4;18H[30mÜ[5;16HÜ[6;15HÜ[19;68Hß[55Dß[7;68Hß[55Dß[18;69H[0mß[57Dß[8;69HÛ[57DÛ[17;69H[30;47mÄ[57D[1;37;44m=[9;69H[0;30;47m¿[57D[1;37mÄ[16;70H[0mÛ[59D[30;47mÄ[10;70H[37;40mÛ[59D[1;47mm[15;70H[0mÛ[59D[1;47m [11;70H[0mÛ[59D[1;47mz[14;70H[0mÛ[59D[1;47ml[12;70H[0mÛ[59D[1;47mr[13;11Ht[2;59H[0;36me[37D[1;37me[3;61H´[41D[0;34mÜ[4;19H[1;30mÜ[5;18HÛ[2DÜ[6;16Hß[19;67Hß[53Dß[7;67Hß[11B[0mß[55Dß[8;68HÛ[55DÛ[17;68H[30;47mÄ[55D[1;37;44m [9;68H[0;30;47mÃ[55D[1;37mÄ[16;69H[0;30;47mÙ[57DÄ[10;69H³[57D[1;37me[15;69H[0;30;47m³[57D[1;37m([11;69H[0;30;47m³[57D[1;37me[14;69H[0;30;47m³[57D[1;37ma[12;69H[0;30;47m³[57D[1;37me[13;70H[0mÛ[59D[1;47m [2;58H[30;40mM[34D[37me[2Dm[3;60HÄ[39D[34;44m°[4;20H[30;40mÜ[5;19H[47m²[6;18H[40mß[2Dß[19;66Hß[51Dß[7;66Hß[11B[0mß[53Dß[8;67HÛ[53DÜ[17;67H[30;47mÄ[53D[1;37;44mE[9;67H[35;45m [53D[37;47mÄ[16;68H[0;30;47mÄ[55DÄ[10;68H[1;37m [55D[0;30;47m [15;68H[1;37m [55De[11;68H[0;30;47m [55D[37;40mÛ[14;68HÛ[55D[1;47mp[12;68H[0mÛ[55D[1;47mc[13;69H[0;30;47m³[57D[1;37m [2;56H[30;40m)[2D[37mc[29Di[2Ds[3;59HÄ[2DÄ[35D[34;44m²[2D±[4;22H[30;40mÜ[2DÜ[5;20HÛ[19;65Hß[2Dß[47Dß[2Dß[7;65Hß[2Dß[48Dß[18;66H[0mß[2Dß[49Dß[2Dß[8;66HÛ[2DÛ[49DÜ[2DÜ[17;66H[30;47mÄ[51D[37;44mx[9;66H[1;35;45mr[51D[37;47mÄ[16;67H[0;30;47mÄ[53DÄ[10;67H[1;37m [53D[0mÛ[15;67H[1;47m [53Ds[11;67H[0;30;47m [53D[37;40mÛ[14;67HÛ[53D[1;47ms[12;67H[0mÛ[53D[1;47me[13;68H[0mÛ[55D[1;47m [2;54H[30;40m([27D[37ms[3;57HÄ[2DÄ[31D[34mÛ[2D[44m²[4;23H[30;40mÜ[5;21H[47mÛ[6;20H[40mÜ[19;63Hß[45Dß[7;63Hß[11B[0mß[47Dß[8;64HÛ[47DÛ[17;65H[30;47mÄ[49D[37;44mi[9;65H[1;35;45me[49D[37;47mÄ[16;66H[0;30;47mÄ[51DÄ[10;66H[1;37m [51D[0mÛ[15;66H[1;47m [51Dt[11;66H[0;30;47m [51D[37;40mÛ[14;66HÛ[51D[1;47me[12;66H[0mÛ[51D[1;47mi[13;67H[0mÛ[53D[1;47m [2;31H[40mr[2Dt[3;55H[0mÄ[29D[1;34mÛ[4;24H[30mÜ[5;23Hß[2D[47mÛ[6;21Hß[19;62H[40mß[43Dß[7;62Hß[11B[0mß[45Dß[8;63HÛ[45DÛ[17;64H[30;47mÄ[47D[1;34;44mt[9;64H[35;45mf[47D[37;47mÄ[16;65H[0;30;47mÄ[49DÄ[10;65H[1;37m [49D[0mÛ[15;65H[1;47m [49D.[11;65H[0;30;47m [49D[37;40mÛ[14;65HÛ[49D[1;47md[12;65H[0mÛ[49D[1;47mv[13;66H[0mÛ[51D[1;47m [2;34H[40ms[2Dn[2Da[3;54H[30mÄ[26D[34mÛ[2DÛ[5;24H[30mÜ[6;22HÛ[19;61Hß[41Dß[7;61Hß[41Dß[18;62H[0mß[43Dß[8;62HÛ[43DÜ[17;63H[30;47mÄ[45D[1;44mÞ[9;63H[35;45ms[45D[37;47mÄ[16;64H[0;30;47mÄ[47DÄ[10;64H[37;40mÛ[47DÛ[15;64H[1;47m [47D)[11;64H [47D[0mÛ[14;64HÛ[47DÛ[12;64HÛ[47D[1;47me[13;65H[0mÛ[49D[1;47m [2;46H[40ml[2Do[2Dc[2Do[2Dt[3Dr[2Dp[3Dr[2De[2Df[3;32H[34mÛ[2DÛ[2DÛ[5;56H[30m:[31D[47mß[2DÛ[6;24H²[2D[40mÛ[19;60Hß[39Dß[7;60Hß[39Dß[18;61H[0mß[41Dß[8;61HÛ[41DÜ[17;62H[30;47mÄ[43D [9;62H[1;35;45mn[43D[37;47mÄ[16;63H[0;30;47mÄ[45DÄ[10;63H[1;37m [45D[0mÛ[15;63HÛ[45D[1;47m [11;63H[0;30;47mÄ[45D[37;40mÛ[14;63HÛ[45DÛ[12;63H[1;47m [45Dd[13;64H[0mÛ[47D[1;47m [2;41H[40mo[3;34H[34;47m²[2D[40mÛ[5;55H[0mp[29D[1;30;47mß[6;25H²[19;59H[40mß[37Dß[7;59Hß[11B[0mß[39Dß[8;60HÛ[39DÜ[17;61H[30;47mÄ[41D[1;37;44mÝ[9;61H[0;45ma[41D[1;47mÄ[16;62H[0;30;47mÄ[43DÄ[10;62H[37;40mÛ[43DÛ[15;62HÛ[43D[1;47m [11;62H[0;30;47mÃ[43D[37;40mÛ[14;62HÛ[43DÛ[12;62H[1;47m [43D[0mÛ[13;63HÛ[45D[1;47m [3;46H[0;34mß[2D[1;44mß[2D[40mÛ[2DÛ[2DÛ[3D[47m²[2D²[2D²[2D[40mÛ[2DÛ[2DÛ[4;50HÜ[5;54H[0mi[27D[1;30mÜ[6;26H[47m±[19;58H[40mß[2Dß[33Dß[2Dß[7;58Hß[2Dß[33DÛ[2Dß[18;59H[0mß[37Dß[8;59HÛ[37DÜ[17;60H[1;30;47mÄ[39D[37;44mF[9;60H[0;45mr[39D[1;47mÄ[16;61H[0;30;47mÄ[41DÄ[10;61H[37;40mÛ[41D[1;47m-[15;61H[0mÛ[41D[1;47m-[11;21H-[14;61H[0mÛ[41D[1;47m-[12;61H [41D-[13;62H[0mÛ[43D[1;47m [3;41H[34m²[4;49H[40mÛ[5;52H[0;34m±[6;28H[1;30mÛ[2D[47m±[19;56H[40mß[31Dß[7;56Hß[31DÛ[18;58H[0mß[35Dß[8;58HÛ[9B[1;30;47mÄ[37D[37;44m1[9;59H[45mT[37D[47mÄ[16;60H[0;30;47mÄ[2DÄ[37DÄ[2DÄ[10;60H[37;40mÛ[2DÛ[37DÛ[2D[30;47m>[15;60H[37;40mÛ[39D[30;47m>[11;22H>[14;60H[37;40mÛ[39D[30;47m>[12;60H[1;37m [39D[0;30;47m>[13;61H[37;40mÛ[41D[1;47m-[4;44H[0;34mß[2D[1;44mß[2D[40mÛ[4D[30mx[2Ds[5;51H[34m²[2DÛ[6;29H[30mÝ[19;55Hß[29Dß[7;55Hß[29DÛ[18;57H[0mß[2Dß[31Dß[2Dß[8;57HÛ[2DÛ[17;58H[1;47mÃ[35D[44m [9;58H[35;45m [35D[37;47mÄ[16;58H[0;30;47mÄ[35DÄ[10;58H[37;40mÛ[35DÛ[15;59HÛ[37D[1;47m [11;23H[0mÛ[14;59HÛ[37DÛ[12;59H[1;47m [37D[0mÛ[13;60HÛ[39D[30;47m>[4;41H[1;34;40mÜ[5;49HÛ[2DÛ[2DÛ[19;54H[30mß[27Dß[7;54Hß[27Dß[18;55H[0mß[29Dß[8;55HÛ[17;57H[1;30;44mÞ[2D[34mr[31D[37m [2D=[9;57H[35;45mt[2Dn[31D[37;47mÄ[2DÄ[16;57H[0;30;47mÄ[33DÄ[10;57H[37;40mÛ[33DÛ[15;58HÛ[35D[1;47m [11;24H[0mÛ[14;58HÛ[35DÛ[12;58H[1;47m [35D[0mÛ[13;59HÛ[37DÛ[5;46H[1;34;47m²[2D²[2D[40mÛ[2DÛ[2D[47mÛ[3D[40mÛ[2DÜ[6;50H[0;34mß[2D[1;44mß[19;53H[30;40mß[2Dß[23Dß[2Dß[7;53Hß[2Dß[23Dß[18;54H[0mß[27Dß[8;54HÛ[9B[1;34;44me[29D[37mT[9;55H[35;45me[29D[37;47mÄ[16;56H[0;30;47mÄ[31DÄ[10;56H[37;40mÛ[31DÛ[15;57HÛ[33D[1;47m [11;25H[0mÛ[14;57HÛ[33DÛ[12;57H[1;47m [33D[0mÛ[13;58HÛ[35DÛ[5;41H[1;34;47m²[19;51H[30;40mß[2Dß[19Dß[2Dß[7;51Hß[20Dß[2Dß[18;53H[0mß[25Dß[8;53HÛ[25DÜ[17;54H[1;34;44mf[27D[0;44mo[9;54H[1;35;45mr[27D[37;47mÄ[16;55H[0;30;47mÄ[29DÄ[10;55H[37;40mÛ[29DÛ[15;56HÛ[31D[1;47m [11;26H[0mÛ[14;56HÛ[31DÛ[12;56H[1;47m [31D[0mÛ[13;57HÛ[33DÛ[19;49H[1;30mß[17Dß[7;33Hß[18;52H[0mß[2Dß[21Dß[2Dß[8;52HÛ[2DÛ[21DÛ[2DÜ[17;53H[1;34;44ms[25D[0;44mg[9;53H[45mr[25D[1;47mÄ[16;54H[0;30;47mÄ[27DÄ[10;54H[37;40mÛ[27DÛ[15;55HÛ[29D[1;47m [11;27H[0mÛ[14;55HÛ[29DÛ[12;55H[1;47m [29D[0mÛ[13;56HÛ[31DÛ[19;48H[1;30mß[2Dß[2Dß[11Dß[2Dß[2Dß[7;47Hß[2Dß[11Dß[2Dß[2Dß[18;50H[0mß[19Dß[8;50HÛ[19DÛ[17;52H[1;34;44mn[23Dg[9;52H[0;45mu[23D[1;47mÄ[16;53H[0;30;47mÄ[25DÄ[10;53H[37;40mÛ[25DÛ[15;54HÛ[27D[1;47m [11;28H[0mÛ[14;54HÛ[27DÛ[12;54H[1;47m [27D[0mÛ[13;55HÛ[29DÛ[19;45H[1;30mß[2Dß[2Dß[2Dß[3Dß[2Dß[2Dß[2Dß[7;45Hß[2Dß[2Dß[2Dß[3Dß[2Dß[2Dß[2Dß[18;49H[0mß[2Dß[15Dß[2Dß[8;49HÛ[2DÜ[15DÛ[2DÛ[17;51H[44ma[2Dr[19D[1;34me[2Dl[9;51H[37;45mC[2D[35m [19D[37;47mÄ[2DÄ[16;52H[0;30;47mÄ[23DÄ[10;52H[37;40mÛ[23DÛ[15;53HÛ[25D[1;47m [11;29H[0mÛ[14;53HÛ[25DÛ[12;53H[1;47m [25D[0mÛ[13;54HÛ[27DÛ[19;41H[1;30mß[7;41Hß[18;47H[0mß[2Dß[11Dß[2Dß[8;47HÛ[2DÛ[11DÛ[2DÛ[17;49H[1;44mT[17D [9;49H[47m´[17DÄ[16;51H[0;30;47mÄ[21DÄ[10;51H[37;40mÛ[21DÛ[15;52HÛ[23D[1;47m [11;30H[0mÛ[14;52HÛ[23DÛ[12;52H[1;47m [23D[0mÛ[13;53HÛ[25DÛ[18;45Hß[2Dß[2Dß[2Dß[3Dß[2Dß[2Dß[2Dß[8;45HÛ[2DÛ[2DÛ[2DÛ[3DÛ[2DÛ[2DÛ[2DÛ[17;48H[1;44m [2D[34mr[13D[0;44mx[2D[1mE[9;48H[47mÄ[2DÄ[13DÄ[2DÄ[16;50H[0;30;47mÄ[2DÄ[17DÄ[2DÄ[10;50H[37;40mÛ[2DÛ[17DÛ[2DÛ[15;51HÛ[21D[1;47m [11;31H[0mÛ[14;51HÛ[21DÛ[12;51H[1;47m [21D[0mÛ[13;52HÛ[23DÛ[18;41Hß[8;41HÛ[17;46H[1;34;44me[11D[0;44mi[9;46H[1;47mÄ[11DÄ[16;48H[0;30;47mÄ[15DÄ[10;48H[37;40mÛ[15DÛ[15;50HÛ[19D[1;47m [11;32H[0mÛ[14;50HÛ[19DÛ[12;50H[1;47m [19D[0mÛ[13;51HÛ[21DÛ[17;45H[44mt[2Df[2D[1mA[2D [3D [2D([2D [2D[34mt[9;45H[37;47mÄ[2DÄ[2DÄ[2DÄ[3DÄ[2DÄ[2DÄ[2DÄ[16;47H[0;30;47mÄ[13DÄ[10;47H[37;40mÛ[13DÛ[15;49HÛ[2D[30;47m>[15D[1;37m [2D [4A[0mÛ[2DÛ[14;49HÛ[17DÛ[12;49H[1;47m [17D[0mÛ[13;50HÛ[19DÛ[17;41H[1;44m)[9;41H[47mÄ[16;46H[0;30;47mÄ[2DÄ[9DÄ[2DÄ[10;46H[37;40mÛ[2DÛ[9DÛ[2DÛ[15;47H[1;47m-[13D [11;35H[0mÛ[14;48H[30;47m>[15D[37;40mÛ[12;48H[1;47m [15D[0mÛ[13;49HÛ[17DÛ[16;44H[30;47mÄ[2DÄ[2DÄ[3DÄ[2DÄ[2DÄ[10;44H[37;40mÛ[2DÛ[2DÛ[3DÛ[2DÛ[2DÛ[15;46H[1;47m [11D [11;36H[0mÛ[14;47H[1;47m-[13D[0mÛ[12;47H[1;47m [13D[0mÛ[13;48H[30;47m>[15D[37;40mÛ[16;41H[30;47mÄ[10;41H[37;40mÛ[15;45H[1;47ms[2Du[7D [2D [4A[0mÛ[2DÛ[14;46H[1;47m [11D[0mÛ[12;46H[1;47m [11D[0;30;47m [13;47H[1;37m-[13D[0mÛ[15;43H[1;47mt[2Da[3Ds[2D [4AÄ[2D[0mÛ[14;45H[1;47m [9D[0mÛ[12;45H[1;47m [9D[0mÛ[13;46H[1;47m [11D[0mÛ[15;41H[1;47mt[11;41H´[14;44He[7D[0mÛ[12;44H[1;47m [7D[0mÛ[13;45H[1;47m [9D[0mÛ[14;43H[1;47mu[2De[3Dq[2D[0mÛ[12;43H[1;47m [2D [3D [2D[0mÛ[13;44H[1;47md[7D[0mÛ[14;41H[1;47mu[12;41H [13;43He[5D[0mÛ[2C[1;47me[2Dp[2Ds[1;1H[40m [0m"};
#endif


// Sleep / Wait
void nsleep ( int i )
{

#ifdef _WIN32
    Sleep ( i*1000 );
#else
    sleep ( i );
#endif
}

// Close Sockets
void closesocks()
{

#ifdef _WIN32
    closesocket ( msock.rsock );
#else
    close ( msock.rsock );
#endif
}

// Transfer / Error Logs
void logntp ( char *text )
{

    if ( LOGGING == false ) return;

    FILE *fp1;
    if ( ( fp1 = fopen ( "ntp-user.log", "a" ) ) == NULL )
    {
        printf ( "Fatal! Couldn't write to log file!\n" );
        return;
    }
    fprintf ( fp1, "* %s\n",text );
    fclose ( fp1 );
}

// Exit Program
void exitprogram()
{

#ifdef _WIN32
    // Turn Windows Cursor back on before exiting
    HANDLE hOut = GetStdHandle ( STD_OUTPUT_HANDLE );
    CONSOLE_CURSOR_INFO CCI;
    GetConsoleCursorInfo ( hOut, &CCI );
    CCI.bVisible = true;
    SetConsoleCursorInfo ( hOut, &CCI );
    system ( "cls" );
#else
    system ( "clear" );
#endif
    printf ( "\nNTP Exited .. .\n" );

    char msg[100]= {0};
#ifdef _WIN32
    // Date/Time Stamp log on Each Conection
    GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTimeFormat, 50 );
    GetDateFormat ( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szDateFormat, 50 );
    sprintf ( msg,"Date/Time: %s %s",szDateFormat,szTimeFormat );
    logntp ( msg );
    WSACleanup();
    setcolor ( 7,0 );
#endif
    exit ( 0 );
}

// Check Filesize
long checksize()
{

    long size = 0;
    std::string dlpath = DLPATH;
    dlpath += finfo.filename;

#ifdef _WIN32
    // Read File and Get File Size
    WIN32_FIND_DATA wfd;   // File Structure
    HANDLE          h;     // Handle to File

    h = FindFirstFile ( dlpath.c_str(), &wfd );
    if ( h != INVALID_HANDLE_VALUE )
    {
        size = wfd.nFileSizeLow;
    }
    FindClose ( h );

#else
    // Open File and Read File Size
    FILE *fp;
    if ( ( fp = fopen ( dlpath.c_str(), "rb" ) ) ==  NULL )
    {
        // Just means theirs no file, so were receiving a new file! :)
        return ( 0 );
    }
    else
    {
#ifdef OS2
        size = filelength ( fileno ( fp ) );
#else
        fseek ( fp, 0L, SEEK_END ); /* Position to end of file */
        size = ftell ( fp );        /* Get file length */
#endif
    }
    fclose ( fp );
#endif

    return ( size );
}

// Check for File Recovery
int resume()
{

    // Check if we are getting New File or Resuming Old
    long totbyte      =0;
    char message[255] = {0};
    char rezBuf[2048] = {0};
    char tBuf[2048]   = {0};
    int  nRet;

    flsz = checksize();
    sprintf ( rezBuf,"%i\r\n\r\n",flsz ); // Create Packet with File Size in Bytes

    // If Filesize 0, error!
    if ( flsz == 0 )
    {
        for ( ;; )
        {
            nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
            if ( nRet == INVALID_SOCKET )
            {
                // Giev 10 retries before error and disconencting
                for ( int retry = 0; ; retry++ )
                {
                    nsleep ( 1 );
                    nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
                    if ( retry == 10 )
                    {
#ifdef _WIN32
                        sprintf ( message, "INVALID_SOCKET - %i",WSAGetLastError() );
#else
                        sprintf ( message, "INVALID_SOCKET - %i",errno );
#endif
                        logntp ( message );
                        strcpy ( finfo.status,message );
                        memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    }
                    else if ( nRet == INVALID_SOCKET ) continue;
                    else if ( nRet == LOST_SOCKET )
                    {
#ifdef _WIN32
                        sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
                        sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
                        logntp ( message );
                        strcpy ( finfo.status,message );
                        memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                        closesocks();
                        return ( 2 );
                    }
                    else break;
                }
            }
            else if ( nRet == LOST_SOCKET )
            {
#ifdef _WIN32
                sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
                sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
                logntp ( message );
                strcpy ( finfo.status,message );
                memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                closesocks();
                return ( 2 );
            }
            return ( 1 );
        }
    }
    // If we already have the full file, Error
    else if ( flsz == finfo.size ) // Exit if the original file is same size as new
    {
        for ( ;; )
        {
            nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
            if ( nRet == INVALID_SOCKET )
            {
                // Giev 10 retries before error and disconencting
                for ( int retry = 0; ; retry++ )
                {
                    nsleep ( 1 );
                    nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
                    if ( retry == 10 )
                    {
#ifdef _WIN32
                        sprintf ( message, "INVALID_SOCKET - %i",WSAGetLastError() );
#else
                        sprintf ( message, "INVALID_SOCKET - %i",errno );
#endif
                        logntp ( message );
                        strcpy ( finfo.status,message );
                        memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    }
                    else if ( nRet == INVALID_SOCKET ) continue;
                    else if ( nRet == LOST_SOCKET )
                    {
#ifdef _WIN32
                        sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
                        sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
                        logntp ( message );
                        strcpy ( finfo.status,message );
                        memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                        closesocks();
                        return ( 2 );
                    }
                    else break;
                }
            }
            else if ( nRet == LOST_SOCKET )
            {
                //Draw Transfer Status
#ifdef _WIN32
                sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
                sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
                logntp ( message );
                strcpy ( finfo.status,message );
                memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                closesocks();
                return ( 2 );
            }

            sprintf ( message,"Error: You Already Have This Full File!" );
            logntp ( message );
            strcpy ( finfo.status,message );
            memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
            closesocks();
            return ( 2 );
        }
    }

    // All is good, continue!
    for ( ;; )
    {
        nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
        if ( nRet == INVALID_SOCKET )
        {
            for ( int retry = 0; ; retry++ )
            {
                nsleep ( 1 );
                nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );
                if ( retry == 10 )
                {
#ifdef _WIN32
                    sprintf ( message, "INVALID_SOCKET - %i",WSAGetLastError() );
#else
                    sprintf ( message, "INVALID_SOCKET - %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                }
                else if ( nRet == INVALID_SOCKET ) continue;
                else if ( nRet == LOST_SOCKET )
                {
#ifdef _WIN32
                    sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
                    sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    closesocks();
                    return ( 2 );
                }
                else break;
            }
        }
        else if ( nRet == LOST_SOCKET )
        {
#ifdef _WIN32
            sprintf ( message, "LOST_SOCKET - %i",WSAGetLastError() );
#else
            sprintf ( message, "LOST_SOCKET - %i",errno );
#endif
            logntp ( message );
            strcpy ( finfo.status,message );
            memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
            closesocks();
            return ( 2 );
        }
        break;
    }
    return ( 0 );
}

// Transfer Percentage 0-100
double percentage ( long double wk, long double pk )
{

    double p = ( 0.0 );
    if ( pk == 0 ) return ( 0.0 );
    if ( wk > 0 )
    {
        if ( wk == pk ) return ( 100.0 );
        else
        {
            p = 100 * ( wk / pk );
            return p;
        }
    }
    return ( 0.0 );
}

// Transfer GUI
#ifdef _WIN32
void TransferGUI ( void *p )
{
#else
void *TransferGUI ( void *p )
{
#endif

    // Receive data from the client
    char fRecv[50]  = {0};            // Bytes Received [Buffer]
    char fLeft[50]  = {0};            // Bytes Left     [Buffer]
    char fSpeed[50] = {0};            // Bytes Speed    [Buffer]
    char tleft[50]  = {0};			 // Time Left      [Buffer]
    long lftbyte    =0;               // Total Bytes Left
    long spdbyte    =0;               // Bytes Transfer in 1 Second
    long totsec     =0;               // Total Seconds Left
    long elsec      =0;               // Total Seconds Elapsed
    int  mm[2];                       // Real Time Left in Minutes
    int  ss[2];						  // Real Time Left in Seconds
    int  hh[2];						  // Real Time Left in Seconds
    double pCent;                     // Percentage Complete
    char Status[50] = {0};            // Transfer Status
    char h[5];
    char m[5];
    char s[5];  // Time Display

    int  avg[6]  = {0};               // Averages for Speed
    long lastavg = 0;                 // Averages for speed

    int timeout  = 0;                 // 120 Second Timeout if no data sent

    // Display Filesize
    char fSize[255];
    sprintf ( fSize,"%.f ", ( float ) finfo.size );

    // Draw Transfer Status
    if ( resum ) sprintf ( Status,"Recovery!" );
    else sprintf ( Status,"Transfering! " );

    // Give 1 Second for File Data to Catch up to GUI
    nsleep ( 1 );

#ifdef _WIN32
    // Display Transfer GUI
    ansiparse ( aBuf2 );
    // Display Filename + Filesize + Status
    drawfilename ( finfo.filename,15,7 );
    drawfilesize ( fSize,15,7 );
    drawstatus ( Status,9,7 );

    queuestatus ( finfo.bqueue,0,7 );
    drawpercent ( 0.0 );
    // Node Status
    gotoxy ( 54,5 );
    setcolor ( 7,0 );
    printf ( "IP: " );
    setcolor ( 15,0 );
    printf ( "%s",inet_ntoa ( their_addr.sin_addr ) );

    // Test if F1 Toggle is on
    if ( F1 )
    {
        gotoxy ( 40,17 );
        setcolor ( 7,1 );
        printf ( "*" );
    }
    else
    {
        gotoxy ( 40,17 );
        setcolor ( 7,1 );
        printf ( " " );
    }
#endif

    // Loops Transfer Statistics
    for ( ;; )
    {

        // Break if Transfer Complete
        if ( ( finfo.size == finfo.bRecv ) || ( erbreak ) ) break;

        // Display Transfer Header
#ifndef _WIN32
#ifdef OS2
        system ( "cls" );
#else
        system ( "clear" );
#endif
        printf ( "\n.----------------------------------------------------------------." );
        printf ( "\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |" );
        printf ( "\n`----------------------------------------------------------------'\n" );
        printf ( "\nIP: %s\n\nFilename   : %s\nFilesize   : %.f\n",inet_ntoa ( their_addr.sin_addr ),finfo.filename, ( float ) finfo.size );
        printf ( "\nBatch Queue: %s\n",finfo.bqueue );
#endif

        // Draw File Bytes Received
        sprintf ( fRecv,"%.f ", ( float ) finfo.bRecv );
#ifdef _WIN32
        drawreceived ( fRecv,9,7 );
#else
        printf ( "\nBytes Recv : %s",fRecv );
#endif

        // Calculate Bytes Left
        lftbyte = ( finfo.size - finfo.bRecv );
        if ( lftbyte < 0 )
        {
            lftbyte = 0;
        }
        sprintf ( fLeft,"%.f ", ( float ) lftbyte );
#ifdef _WIN32
        drawleft ( fLeft,9,7 );
#else
        printf ( "\nBytes Left : %s\n",fLeft );
#endif

        // Calculate Average Bytes Per Second, 5 Second Average
        avg[0] = finfo.bRecv - finfo.lRecv;
        avg[5] = avg[4];
        avg[4] = avg[3];
        avg[3] = avg[2];
        avg[2] = avg[1];
        avg[1] = avg[0];

        spdbyte = avg[1]+avg[2]+avg[3]+avg[4]+avg[5];
        if ( ( spdbyte < lastavg ) && ( spdbyte != 0 ) )
            if ( ( spdbyte - lastavg ) > 10 )
                spdbyte = lastavg;

        // Calculate Average Bytes Transfered Per Second
        lastavg = spdbyte;
        spdbyte /= 5;
        if ( spdbyte < 0 ) spdbyte = 0;

        // Check for timeout if == 20 seconds, exit program, lost connection!
        if ( spdbyte <= 0 ) ++timeout;
        else timeout = 0;

        // Test this more later..
        if ( timeout == 120 ) timeout = 0;
        /*
        if (timeout == 120) {
            logntp("Transfer Timed out! Exiting.. .");

            //exitprogram();
        }*/

        // Calucate Time in Seconds Left of Transfer
        if ( ( spdbyte != 0 ) && ( lftbyte != 0 ) )
        {
            totsec = lftbyte / spdbyte;
        }
        else
        {
            totsec = 0;
        }

        // Calculate Speed KB/S
        if ( spdbyte > 1000 )
        {
            spdbyte /= 1000; // Convert to KiloBytes/Sec
            sprintf ( fSpeed,"%.f kb/sec", ( float ) spdbyte );
        } // Else to slow to show a Kilobyte, Display in Bytes
        else sprintf ( fSpeed,"%.f b/sec", ( float ) spdbyte );
#ifdef _WIN32
        drawspeed ( fSpeed,15,7 );
#else
        printf ( "\nSpeed      : %s\n",fSpeed );
#endif

        // Convert Time left in Seconds to Hours : Mintues : Seconds
        if ( totsec == 0 )
        {
            hh[0] = 0;
            mm[0] = 0;
            ss[0] = 0;
        }
        else
        {
            if ( totsec < 60 )
            {
                hh[0] = 0;
                mm[0] = 0;
                ss[0] = totsec;
            }
            else
            {
                hh[0] = 0;
                mm[0] = totsec / 60;
                ss[0] = totsec % 60;
                if ( mm[0] > 60 )
                {
                    hh[0] = mm[0] / 60;
                    mm[0] = mm[0] % 60;
                }
            }
        }
        // Add Leading 0's if under 10
        if ( hh[0] < 10 ) sprintf ( h,"0%i",hh[0] );
        else sprintf ( h,"%i",hh[0] );

        if ( mm[0] < 10 ) sprintf ( m,"0%i",mm[0] );
        else sprintf ( m,"%i",mm[0] );

        if ( ss[0] < 10 ) sprintf ( s,"0%i",ss[0] );
        else sprintf ( s,"%i",ss[0] );

        // Draw Time
        sprintf ( tleft,"%s:%s:%s ",h,m,s );
#ifdef _WIN32
        drawtleft ( tleft,15,7 );
#else
        printf ( "\nTime Left  : %s",tleft );
#endif

        // Convert Time Elapsed in Seconds to Hours : Mintues : Seconds
        if ( elsec == 0 )
        {
            hh[1] = 0;
            mm[1] = 0;
            ss[1] = 0;
        }
        else
        {
            if ( elsec < 60 )
            {
                hh[1] = 0;
                mm[1] = 0;
                ss[1] = elsec;
            }
            else
            {
                hh[1] = 0;
                mm[1] = elsec / 60;
                ss[1] = elsec % 60;
                if ( mm[1] > 60 )
                {
                    hh[1] = mm[1] / 60;
                    mm[1] = mm[1] % 60;
                }
            }
        }
        // Add Leading 0's if under 10
        if ( hh[1] < 10 ) sprintf ( h,"0%i",hh[1] );
        else sprintf ( h,"%i",hh[1] );

        if ( mm[1] < 10 ) sprintf ( m,"0%i",mm[1] );
        else sprintf ( m,"%i",mm[1] );

        if ( ss[1] < 10 ) sprintf ( s,"0%i",ss[1] );
        else sprintf ( s,"%i",ss[1] );

        // Draw Time Elapsed
        sprintf ( finfo.telapsed,"%s:%s:%s",h,m,s );
#ifdef _WIN32
        drawtelapse ( finfo.telapsed,15,7 );
#else
        printf ( "\n> Elapsed  : %s\n",finfo.telapsed );
#endif

        // Calculate Percentage
        pCent = percentage ( finfo.bRecv,finfo.size );
        // Fix Percentage displaying 100 to early!
        if ( pCent > 98 ) pCent = 99;
#ifdef _WIN32
        percenttop ( pCent );
        drawpercent ( pCent );
#else
        printf ( "\nComplete   : %.f%%", ( float ) pCent );
        printf ( "\nStatus     : %s\n",Status );
#endif

        // Setup to Calculate for KB/S
        finfo.lRecv = finfo.bRecv;

        // Update Transfer Stats Every Second
        nsleep ( 1 );
        ++elsec;
    }
}

// Receive File Data
void recvfdata()
{

    char szBuf[2048];
    char message[255];
    int nRet;

    int c, i, j=1;
    long totbyte=0;
    FILE *nfp;

    // Setup download path with filename
    std::string dlpath = DLPATH;
    dlpath += finfo.filename;

    // Open File Depending on New / Resume
    if ( resum ) // True - append
    {
        totbyte = flsz;
        if ( ( nfp = fopen ( dlpath.c_str(), "a+b" ) ) ==  NULL )
        {
            sprintf ( message,"Error: Can't create file: '%s'\n",finfo.filename );
            erbreak = true;
            logntp ( message );
            strcpy ( finfo.status,message );
            memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
            closesocks();
            return;
        }
    }
    else   // False - overwrite
    {
        if ( ( nfp = fopen ( dlpath.c_str(), "w+b" ) ) ==  NULL )
        {
            sprintf ( message,"Error: Can't create file: '%s'\n",finfo.filename );
            erbreak = true;
            logntp ( message );
            strcpy ( finfo.status,message );
            memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
            closesocks();
            return;
        }
    }

    // Start GUI Transfer Threads
#ifdef _WIN32
    HANDLE ahThread;
    ahThread = ( HANDLE ) _beginthread ( TransferGUI, 0, ( void * ) NULL );
#else
    pthread_t thread;
    pthread_create ( &thread, NULL, TransferGUI, ( void * ) NULL );
#endif

    // Give a slight pause for GUI to Finish Drawing before starting
    nsleep ( 1 );

    // Receive data from the client
    j = 1;
    while ( j > 0 )
    {
        memset ( szBuf, 0, sizeof ( szBuf ) );		// clear buffer
        nRet = recv ( msock.rsock,      			// Connected client
                      szBuf,							// Receive buffer
                      sizeof ( szBuf ),					// Lenght of buffer
                      0 );								// Flags

        j = nRet;
        if ( nRet == INVALID_SOCKET )
        {
            // 10 Retries before error and disconnecting!
            for ( int rtry = 0; ; rtry++ )
            {
                nsleep ( 1 );
                nRet = recv ( msock.rsock,      		// Connected client
                              szBuf,						// Receive buffer
                              sizeof ( szBuf ),				// Lenght of buffer
                              0 );							// Flags

                j = nRet;
                if ( rtry == 10 )
                {
                    if ( finfo.size == totbyte )
                    {
                        j = 0;    // Exit Casue File is Finished
                        break;
                    }
#ifdef _WIN32
                    sprintf ( message, "Lost Connection - %i",WSAGetLastError() );
#else
                    sprintf ( message, "Lost Connection - %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    fclose ( nfp );
                    closesocks();
                    erbreak = true;
                    return;
                }
                else if ( nRet == INVALID_SOCKET ) continue;
                else if ( nRet == LOST_SOCKET )
                {
                    if ( finfo.size == totbyte )
                    {
                        j = 0;    // Exit Casue File is Finished
                        break;
                    }
#ifdef _WIN32
                    sprintf ( message, "Lost Connection - %i",WSAGetLastError() );
#else
                    sprintf ( message, "Lost Connection - %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    fclose ( nfp );
                    closesocks();
                    erbreak = true;
                    return;
                }
                else break;
            }
        }
        else if ( nRet == LOST_SOCKET )
        {
            if ( finfo.size == totbyte )
            {
                j = 0;    // Exit Casue File is Finished
                break;
            }
#ifdef _WIN32
            sprintf ( message, "LOST_SOCKET : Lost Connection - %i",WSAGetLastError() );
#else
            sprintf ( message, "LOST_SOCKET : Lost Connection - %i",errno );
#endif
            logntp ( message );
            strcpy ( finfo.status,message );
            memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
            fclose ( nfp );
            closesocks();
            erbreak = true;
            return;
        }

        // receive data from client and save to file
        i = 0;
        while ( nRet > i )
        {
            // End of Transfer, Break Loop
            if ( totbyte == finfo.size ) break;
            c=szBuf[i];
            putc ( c, nfp );
            i++;
            totbyte++;
            finfo.bRecv = totbyte;
        }
        // End of Transfer, Break Loop
        if ( totbyte == finfo.size )
        {
            j = 0;
            break;
        }
    }

    erbreak = true;
    fclose ( nfp );
    closesocks();
    nsleep ( 1 );

    // End of File Transfer is Reached Here if Successful!
    char fRecv[50]= {0};             // Bytes Received [Buffer]
    char fLeft[50]= {0};             // Bytes Left     [Buffer]
    long lftbyte  =0;

    // Draw GUI File Bytes Received / Left Information
    sprintf ( fRecv,"%.f bytes", ( float ) checksize() ); // Display True Total
    lftbyte = ( finfo.size - checksize() );			// Should always be 0!
    if ( lftbyte < 0 )
    {
        lftbyte = 0;    // just in case
    }
    sprintf ( fLeft,"%.f bytes", ( float ) lftbyte ); // Display True Left

    // Draw Transfer Status
    sprintf ( message,"Successful!" );
    strcpy ( finfo.status,message );
    logntp ( message );

#ifdef _WIN32
    drawpercent ( 100 );
    percenttop ( 100 );
    drawreceived ( fRecv,9,7 );
    drawleft ( fLeft,9,7 );
    drawstatus ( message,15,7 );
#else
#ifdef OS2
    system ( "cls" );
#else
    system ( "clear" );
#endif
    printf ( "\n.----------------------------------------------------------------." );
    printf ( "\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |" );
    printf ( "\n`----------------------------------------------------------------'\n" );
    printf ( "\nIP: %s\n\nFilename   : %s\nFilesize   : %.f\n",inet_ntoa ( their_addr.sin_addr ),finfo.filename, ( float ) finfo.size );
    printf ( "\nBatch Queue: %s\n",finfo.bqueue );

    printf ( "\nBytes Recv : %i", checksize() );
    printf ( "\nBytes Left : 0\n" );

    printf ( "\nSpeed      : 0\n" );

    printf ( "\nTime Left  : 0" );
    printf ( "\n> Elapsed  : %s\n",finfo.telapsed );

    printf ( "\nComplete   : 100%%" );
    printf ( "\nStatus     : %s\n",message );
#endif

    // Copy File Stats to finfo2 for Message Window
    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
    nsleep ( 2 );
}

// Receive File Info / NTP Version
bool recvfinfo()
{

    char message[255]  = {0};
    char tMsg[255]     = {0};
    char szBuf[2048]   = {0};
    int nRet;

    // Get Information Packet From Client
    char tmpin[1000]= {0};
    std::string msgEop;
    int id;

    logntp ( "Receiving File Info.. ." );

    // Loop through RECV() Untll We Get all of the Packet
    for ( ;; )
    {
        memset ( szBuf, 0, sizeof ( szBuf ) ); // clear buffer
        nRet = recv ( msock.rsock, szBuf, sizeof ( szBuf ), 0 );
        if ( nRet == INVALID_SOCKET )
        {
            for ( int retry = 0; ; retry++ )
            {
                nsleep ( 1 );
                nRet = recv ( msock.rsock, szBuf, sizeof ( szBuf ), 0 );
                if ( retry == 10 )
                {
#ifdef _WIN32
                    sprintf ( message, "INVALID_SOCKET : %i",WSAGetLastError() );
#else
                    sprintf ( message, "INVALID_SOCKET : %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    return false;
                }
                else if ( nRet == INVALID_SOCKET ) continue;
                else if ( nRet == LOST_SOCKET )
                {
#ifdef _WIN32
                    sprintf ( message, "LOST_SOCKET : %i",WSAGetLastError() );
#else
                    sprintf ( message, "LOST_SOCKET : %i",errno );
#endif
                    logntp ( message );
                    strcpy ( finfo.status,message );
                    memcpy ( &finfo2,&finfo,sizeof ( FILEINFO ) );
                    closesocks();
                    return false;
                }
                else break;
            }
        }
        else if ( nRet == LOST_SOCKET )
        {
            memset ( &finfo2,0,sizeof ( FILEINFO ) );
#ifdef _WIN32
            sprintf ( message, "LOST_SOCKET : %i",WSAGetLastError() );
#else
            sprintf ( message, "LOST_SOCKET : %i",errno );
#endif
            logntp ( message );
            strcpy ( finfo2.status,message );
            closesocks();
            return false;
        }

        // Check for End of Packet String!
        strcat ( tmpin,szBuf );
        msgEop = tmpin;
        id = 0;
        id = msgEop.find ( "\r\n\r\n", 0 );
        // Received End of Packet
        if ( id != -1 ) break;

    } // End of For Recv Loop

    // After Receiving Full Packet, Chop it up so we can Sort the Information
    char tmppak[255]= {0};
    int  pamcnt = 0, ab = 0;
    int  num = 0;

    char arg[255]= {0};
    memset ( &finfo,0,sizeof ( FILEINFO ) ); // 0 out File Struct

    // Cut Up Recevied Packet and Get the Following Information
    for ( int i = 0; ; i++ )
    {
        if ( tmpin[i] == '\0' ) break;
        if ( tmpin[i] == '\n' )
        {
            ++pamcnt;
            // Check for BBS Version String
            if ( pamcnt == 1 )
            {
                strcpy ( finfo.bbsver,tmppak );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                sprintf ( message,"BBS Version: %s",finfo.bbsver );
                logntp ( message );

                // If Not Correction Version, Exit!
                if ( strcmp ( finfo.bbsver,"V1.10B" ) != 0 )
                {
                    memset ( &finfo2,0,sizeof ( FILEINFO ) );
                    sprintf ( message,"Error! Remote is using NTP %s!",finfo.bbsver );
                    logntp ( message );
                    strcpy ( finfo2.status,message );
                    closesocks();
                    return false;
                }
            }
            // Check for Filename String
            else if ( pamcnt == 2 )
            {
                ab = 0;
                strcpy ( arg,tmppak );
                memset ( tmppak,0,sizeof ( tmppak ) );

                // Get True Filename and cute off Path if found!!
                for ( int i = 0; ; i++ )          // Count for Romoval of ExeName from Path
                {
                    if ( arg[i] == '\0' ) break;  // End of String, Break
#ifdef _WIN32
                    if ( arg[i] == '\\' ) num = i; // Find last or only '\' in String
#else
                    if ( arg[i] == '/' ) num = i; // Find last or only '/' in String
#endif
                }
                if ( num == 0 )
                {
                    strcpy ( finfo.filename,arg );
                }
                else
                {
                    int r = 0;
                    for ( int i = num+1; ; i++ )  // Copy all Chars after last '\'
                    {
                        if ( arg[i] == '\0' ) break;
                        finfo.filename[r] = arg[i];
                        r++;
                    }
                }
                sprintf ( message,"Filename: %s",finfo.filename );
                logntp ( message );
            }
            // Check for Filesize String
            else if ( pamcnt == 3 )
            {
                memset ( arg,0,sizeof ( arg ) );
                strcpy ( arg,tmppak );
                finfo.size = atol ( arg );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                if ( finfo.size == 0 )
                {
                    sprintf ( message,"Filesize: %i",finfo.size );
                    logntp ( message );
                    sprintf ( message,"Error: Receiving empty file!" );
                    logntp ( message );
                    strcpy ( finfo2.status,message );
                    closesocks();
                    return false;
                }
                else
                {
                    sprintf ( message,"Filesize: %i",finfo.size );
                    logntp ( message );
                }
            }
            // Check for Batch Queue String
            else if ( pamcnt == 4 )
            {
                strcpy ( finfo.bqueue,tmppak );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                sprintf ( message,"Queue: %s",finfo.bqueue );
                logntp ( message );
                break;
            }
        }
        else
        {
            tmppak[ab] = tmpin[i];
            ab++;
        }
    }
    return true;
}

// File Transfer Startup
void transfersetup()
{

    char message[255];
    // Get File Info
    if ( !recvfinfo() )
    {
        sprintf ( message,"Error Getting FileInfo.. ." );
        logntp ( message );
        return;
    }

    // Check for Resume
    int i = resume();
    if ( i == 2 ) // Error
    {
        sprintf ( message,"Error Getting Resume Info.. ." );
        logntp ( message );
        return;
    }
    else if ( i == 0 )
    {
        resum = true;
    }
    else
    {
        resum = false;
    }

    // Set Error Break to False
    erbreak = false;

    // Start Receiving File Data
    tstate = true;   // Transfer State
    recvfdata();
    tstate = false;
}

// Setup Listen Server & Message Window
void listensrv ( SOCKET listenSocket, short nPort )
{

    SOCKET	remoteSocket;
    int sin_size;
    char mInit[50];
    char message[255];

    // Tests for End of Transfers Exit
    std::string que;
    int id1, id2;
    char Num[5]= {0};

    while ( 1 )
    {

        // Show the server name and port number
        sprintf ( mInit,"[NTP v1.10.1b] Initalized On Port %d ", nPort );
        logntp ( "\n***********************************************************************************\n" );
        logntp ( mInit );

        // Draw Message Window
#ifdef _WIN32
        ansiparse ( aBuf1 );
        drawinit ( "+ NTP v1.10.1 beta Initalized!",0,7 );
        gotoxy ( 54,5 );
        setcolor ( 7,0 );
        printf ( "using port " );
        setcolor ( 15,0 );
        printf ( "%i",nPort );

        // Display F1 Exit After Transfer
        if ( F1 )
        {
            gotoxy ( 40,20 );
            setcolor ( 7,1 );
            printf ( "*" );
        }
        else
        {
            gotoxy ( 40,20 );
            setcolor ( 7,1 );
            printf ( " " );
        }
#else
        system ( "clear" );
        printf ( "\n.----------------------------------------------------------------." );
        printf ( "\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |" );
        printf ( "\n`----------------------------------------------------------------'\n" );
        printf ( "\n%s\n",mInit );
#endif

        if ( strcmp ( finfo2.status,"" ) != 0 )
        {
            // Show Connection Established
#ifdef _WIN32
            sprintf ( mInit,"+ NTP v1.10.1 beta Last File Transfer Stats.. . " );
            drawinit3 ( mInit,0,7 );
            sprintf ( mInit,"Filename : %s",finfo2.filename );
            drawinit4 ( mInit,1,7 );
            sprintf ( mInit,"   Queue : %s",finfo2.bqueue );
            drawinit5 ( mInit,1,7 );
            sprintf ( mInit,"  Status : %s",finfo2.status );
            drawinit6 ( mInit,1,7 );
#else
            printf ( "\n\n[NTP v1.10.1b] Last File Transfer Stats.. . " );
            printf ( "\nFilename : %s",finfo2.filename );
            printf ( "\n   Queue : %s",finfo2.bqueue );
            printf ( "\n  Status : %s\n\n",finfo2.status );
#endif

            // Do test here.. if F1 && bqueue = # of # success, or Error, then exit!
            if ( F1 )
            {
                // If Success, Check if Last file in Queue is Reached
                if ( strcmp ( finfo2.status,"Successful!" ) == 0 )
                {
                    //strcmp(finfo2.status,"Error: You Already Have This Full File!") == 0) {
                    que = finfo2.bqueue;
                    id1 = que.find ( "of",0 );
                    if ( id1 != -1 )
                    {
                        // Get 1st number of Queue
                        for ( int i = 0; i != id1-1; i++ )
                        {
                            Num[i] = que[i];
                        }
                        // If first # Match's Last, Exit Program!
                        id2 = que.find ( Num,id1 );
                        if ( id2 != -1 )
                        {
                            logntp ( "End of Transfers, Program Exiting.. ." );
                            e1 = true;
                            exitprogram();
                        }
                    }
                }
                // If Error, Exit!
                else
                {
                    logntp ( "End of Transfers w/ Error!, Program Exited.. ." );
                    e1 = true;
                    exitprogram();
                }
            }
        }

        // Show Connection Established
        sprintf ( mInit,"Waiting for Connection.. . " );
        logntp ( mInit );
#ifdef _WIN32
        drawinit2 ( mInit,1,7 );
#else
        printf ( "\n%s\n",mInit );
#endif

        // Clears the struct to recevie information from the user
        sin_size = sizeof ( struct sockaddr_in );
        memset ( &their_addr,0,sizeof ( their_addr ) );

#ifdef _WIN32
        remoteSocket = accept ( listenSocket, ( struct sockaddr * ) &their_addr, &sin_size );
#else
        remoteSocket = accept ( listenSocket, ( struct sockaddr * ) &their_addr, ( socklen_t * ) &sin_size );
#endif

        if ( e1 ) return;
        else if ( remoteSocket == INVALID_SOCKET )
        {
            memset ( &finfo2,0,sizeof ( FILEINFO ) );
            sprintf ( message,"Error: accept() - Incomming Connection!" );
            logntp ( message );
            strcpy ( finfo2.status,message );
#ifdef _WIN32
            closesocket ( listenSocket );
#else
            close ( listenSocket );
#endif
            exitprogram();
        }

        // Fill Socket Struct on Completed Connection
        msock.rsock = remoteSocket;
        msock.lsock = listenSocket;

        // Show Connection Established
        sprintf ( mInit,"Connection Established: %s",inet_ntoa ( their_addr.sin_addr ) );
        logntp ( mInit );
#ifdef _WIN32
        drawinit2 ( mInit,15,7 );
#else
        printf ( "\n%s\n",mInit );
#endif
        nsleep ( 2 );

#ifdef _WIN32
        // Date/Time Stamp log on Each Conection
        GetTimeFormat ( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTimeFormat, 50 );
        GetDateFormat ( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szDateFormat, 50 );
        sprintf ( mInit,"Date/Time: %s %s",szDateFormat,szTimeFormat );
        logntp ( mInit );
#endif

        // Start File Transfer init
        transfersetup();
    }
}

// Socket Init and Startup
void StreamServer ( short nPort )
{

    // Startup Header
#ifndef _WIN32
    system ( "clear" );
    printf ( "\n.----------------------------------------------------------------." );
    printf ( "\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |" );
    printf ( "\n`----------------------------------------------------------------'\n" );
#endif

    char message[255]; // Error Messages
    // Create a TCP/IP stream socket to "listen" with
    SOCKET	listenSocket;

    listenSocket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( listenSocket == INVALID_SOCKET )
    {
        memset ( &finfo2,0,sizeof ( FILEINFO ) );
        sprintf ( message,"Error: Socket Init!" );
        logntp ( message );
        strcpy ( finfo2.status,message );
        exitprogram();
    }

    // Fill in the address structure
#ifdef _WIN32
    SOCKADDR_IN saServer;
#else
    struct sockaddr_in saServer;
#endif

    saServer.sin_family = AF_INET;
    saServer.sin_addr.s_addr = INADDR_ANY;	// Let WinSock supply address
    saServer.sin_port = htons ( nPort );		// Use port from command line
#ifndef _WIN32
    memset ( & ( saServer.sin_zero ), '\0', 8 );
#endif

    char szBuf[2048]= {0};
    int nRet;

#ifdef _WIN32
    // bind the name to the socket
    nRet = bind ( listenSocket,				// Socket
                  ( LPSOCKADDR ) &saServer,		// Our address
                  sizeof ( struct sockaddr ) );	// Size of address structure
#else
    // bind the name to the socket
    nRet = bind ( listenSocket,
                  ( struct sockaddr * ) &saServer,
                  sizeof ( saServer ) );
#endif

    if ( nRet == INVALID_SOCKET )
    {
        memset ( &finfo2,0,sizeof ( FILEINFO ) );
        sprintf ( message,"Error: bind() - Unable to use selected port! Try another.. ." );
        logntp ( message );
#ifdef _WIN32
        setcolor ( 7,0 );
        printf ( "\n%s",message );
        closesocket ( listenSocket );
#else
        printf ( "\n%s",message );
        printf ( "\nif you are not root, then try a port over 1024!\n\n",message );
        close ( listenSocket );
#endif
        nsleep ( 5 );
        exitprogram();
    }

    // Set the socket to listen
    nRet = listen ( listenSocket, 10 );
    if ( nRet == INVALID_SOCKET )
    {
        memset ( &finfo2,0,sizeof ( FILEINFO ) );
        sprintf ( message,"Error: listen() - For Connection!" );
        logntp ( message );
        strcpy ( finfo2.status,message );
#ifdef _WIN32
        closesocket ( listenSocket );
#else
        close ( listenSocket );
#endif
        exitprogram();
    }

    // Listen Server Setup
    listensrv ( listenSocket, nPort );
}

// Input Thread, Handles Exit Key
// Win32 only at the moment
#ifdef _WIN32
void input ( void *p )
{

#ifdef _WIN32
    setcolor ( 7,7 );
#endif
    int c;
    while ( 1 )
    {
        c = getch();
        // Handle ESC Key
        if ( c == 27 )
        {
            e1 = true;
            logntp ( "Program Exited by User.. ." );
#ifdef _WIN32
            setcolor ( 7,0 );
#endif
            exitprogram();
        }
        // Handle F1 Key
        if ( c == 59 )
        {
            if ( F1 )
            {
                F1 = false;
                if ( tstate )
                {
                    gotoxy ( 40,17 );
                    setcolor ( 7,1 );
                    printf ( " " );
                }
                else
                {
                    gotoxy ( 40,20 );
                    setcolor ( 7,1 );
                    printf ( " " );
                }
            }
            else
            {
                F1 = true;
                if ( tstate )
                {
                    gotoxy ( 40,17 );
                    setcolor ( 7,1 );
                    printf ( "*" );
                }
                else
                {
                    gotoxy ( 40,20 );
                    setcolor ( 7,1 );
                    printf ( "*" );
                }
            }
        }
    }
}
#endif

// Main Program Entrance
int main ( int argc, char **argv )
{

#ifdef _WIN32
    SetConsoleTitle ( "Nemesis Transfer Protocol [USER v1.10.1 beta]" );
#endif

    char newport[8];
    short nport = 0;

    // Check for port argument
    if ( argc != 2 )
    {
#ifdef _WIN32
        system ( "cls" );
#else
        system ( "clear" );
#endif
        printf ( ".----------------------------------------------------------------------.\n" );
        printf ( "| NTP v1.10.1 beta 02/18/2004 (c) Mercyful Fate 2k3/2k4                |\n" );
        printf ( "| Program init phase.. .                                               |\n" );
        printf ( "`----------------------------------------------------------------------'\n\n" );

        printf ( " No port specified in the command line.. ." );
        printf ( "\n\n Please enter a port # to start NTP with -> " );
        gets ( newport );
        nport = atoi ( newport );
    }

#ifdef _WIN32
    // Turn Windows Cursor Off for Smoother Ansimation
    HANDLE hOut = GetStdHandle ( STD_OUTPUT_HANDLE );
    CONSOLE_CURSOR_INFO CCI;
    GetConsoleCursorInfo ( hOut, &CCI );
    CCI.bVisible = false;
    SetConsoleCursorInfo ( hOut, &CCI );
#endif

    // Setup Global Path in Structs.h
    char parg[255];
    strcpy ( parg,argv[0] );
    int num = 0;

    // Get FULL PATH TO EXE, and Chop off Filename for PATH
#ifdef _WIN32
    memset ( &PATH,0,255 );
    for ( int i = 0; ; i++ )
    {
        if ( parg[i] == '\0' ) break;
        if ( parg[i] == '\\' ) num = i;
    }
    if ( num != 0 )
    {
        for ( int i = 0; i < num+1 ; i++ )
        {
            PATH[i] = parg[i];
        }
        SetCurrentDirectory ( PATH );
    }
    else memset ( &PATH,0,sizeof ( PATH ) );
#else
    memset ( &PATH,0,255 );
    for ( int i = 0; ; i++ )
    {
        if ( parg[i] == '\0' ) break;
        if ( parg[i] == '/' ) num = i;
    }
    if ( num != 0 )
    {
        for ( int i = 0; i < num+1 ; i++ )
        {
            PATH[i] = parg[i];
        }
    }
    else memset ( &PATH,0,sizeof ( PATH ) );
#endif

    // Do Error Checking if CONFIG.CFG File exists, if not creates it
    if ( configdataexists() == false ) createconfig(); // Creates Config with Default Settings

    // Open and Read Config file
    parseconfig();

    // get port #
    short iPort;
    if ( nport != 0 ) iPort = nport;
    else iPort = atoi ( argv[1] );

    // Clear Info 2 Error Message Structure
    memset ( &finfo2,0,sizeof ( FILEINFO ) );

    // Start Console Input Thread
#ifdef _WIN32
    HANDLE ahThread[1];
    ahThread[0] = ( HANDLE ) _beginthread ( input, 0, ( void* ) &iPort );
#else
    /* Disabled Linux Input for Now
    pthread_t thread;
    pthread_create(&thread, NULL, input, (void*)iPort);
    */
#endif

    // Start Win32 Winsock Initilization
#ifdef _WIN32
    char message[30];
    WORD wVersionRequested = MAKEWORD ( 1,1 );
    WSADATA wsaData;

    WSAStartup ( wVersionRequested, &wsaData );
    if ( wsaData.wVersion != wVersionRequested )
    {
        memset ( &finfo2,0,sizeof ( FILEINFO ) );
        sprintf ( message,"Error: Winsock 1.1 Unsupported!\n" );
        logntp ( message );
        strcpy ( finfo2.status,message );
        exitprogram();
    }
#endif

    // Startup Server and listen for connections
    StreamServer ( iPort );

    // Listen Server Setup
    //listensrv(listenSocket, nPort);
}

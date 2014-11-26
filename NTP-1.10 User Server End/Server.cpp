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

typedef struct {
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
}FILEINFO;

FILEINFO finfo;                    // Handle to File Info
FILEINFO finfo2;                   // Handle to File Info After Tranfer, Message Window

struct sockaddr_in their_addr;
	
typedef struct {
	SOCKET rsock;
	SOCKET lsock;
}MSOCK;
	
MSOCK msock;                       // Handle to Sockets
	
long flsz;          	           // Local Filesize for Resume
bool resum;                        // Resume True/ flase
bool erbreak;                      // error break for gui

char szDateFormat[128];		       // System Date
char szTimeFormat[128];			   // System Time

char DLPATH[255]={0};              // Download Path
char PATH[255]  ={0};              // Programs Path
bool LOGGING    = true;            // Transfer / Error Logging
bool tstate     = false;           // Transfer State
bool F1         = false;           // Togle Exit After Transfer
bool e1         = false;           // Program Exit

#ifdef _WIN32
// Inital Startup Screen with Message Window
char *aBuf1={"[40m[2J[13;40H[0m���[4D�[3C�[12;41H�[14;41H�[13;38H�[5C�[12;39H��[C��[14;39H��[C��[13;37H�[7C�[12;38H�[5C�[14;38H�[5C�[11;41H�[15;41H�[13;36H�[9C�[12;37H�[7C�[14;37H�[7C�[11;39H��[C��[15;39H��[C��[13;35H�[11C�[12;36H�[9C�[14;36H�[9C�[11;37H��[5C��[15;37H��[5C��[10;41H�[16;41H�[13;34H�[13C�[12;35H�[11C�[14;35H�[11C�[11;36H�[9C�[15;36H�[9C�[10;38H���[C���[16;38H���[C���[13;33H�[15C�[12;34H�[13C�[14;34H�[13C�[11;35H�[11C�[15;35H�[11C�[10;36H��[7C��[16;36H��[7C��[9;41H[1;47m�[17;41H[0m�[13;32H�[17C�[12;33H�[15C�[14;33H�[15C�[11;33H��[13C��[15;33H��[13C��[10;35H�[11C�[16;35H�[11C�[9;37H[1;47m����[C����[17;37H[0m����[C����[13;31H�[19C�[12;32H�[17C�[14;32H�[17C�[11;32H�[17C�[15;32H�[17C�[10;34H�[13C�[16;34H�[13C�[9;36H[1;47m�[9C�[17;36H[0m�[9C�[8;41H�[18;41H[1;47m [13;30H[0m�[21C�[12;31H�[19C�[14;31H�[19C�[11;31H�[19C�[15;31H�[19C�[10;32H��[15C��[16;32H��[15C��[9;34H[1;47m��[11C��[17;34H[0m��[11C��[8;37H����[C����[18;37H[1;47m    [C    [13;29H[0m�[23C�[12;30H�[21C�[14;30H�[21C�[11;30H�[21C�[15;30H�[21C�[10;31H�[19C�[16;31H�[19C�[9;33H[1;47m�[15C�[17;33H[0m�[15C�[8;35H��[9C��[18;35H[1;47m  [9C  [7;41H[30;40m�[19;41H[0;30;47m�[13;28H[37;40m�[25C�[12;29H�[23C�[14;29H�[23C�[11;29H�[23C�[15;29H�[23C�[10;30H�[21C�[16;30H�[21C�[9;31H[1;47m��[17CĴ[17;31H[0m��[17C��[8;33H��[13C��[18;33H[1;47m  [13C  [7;37H[30;40m����[C����[19;37H[0;30;47m����[C����[13;27H[37;40m�[27C�[12;28H�[25C�[14;28H�[25C�[11;28H�[25C�[15;28H�[25C�[10;29H�[23C�[16;29H�[23C�[9;30H[1;47m�[21C[35;45m [17;30H[0m�[21C�[8;32H�[17C�[18;32H[1;47m [17C [7;34H[30;40m���[9C��[19;34H[0;30;47m���[9C���[20;41H[1;37;44m)[13;26H[0m�[29C�[12;27H�[27C�[14;27H�[27C�[11;27H�[27C�[15;27H�[27C�[10;28H�[25C�[16;28H�[25C�[9;29H[1;47m�[23C[45mM[17;29H[0m�[23C�[8;30H��[19C��[18;30H[1;47m  [19C  [7;33H[30;40m�[19;33H[0;30;47m�[15C�[20;36H[37;44mi[1;34mt[37m ( [C A[0;44mft[1;34me[13;25H[0m�[31C�[12;26H�[29C�[14;26H�[29C�[11;26H�[29C�[15;26H�[29C�[10;27H�[27C�[16;27H�[27C�[9;28H[1;47m�[25C[0;45me[17;28H[40m�[25C�[8;29H�[23C�[18;29H[1;47m [23C [7;31H[30;40m��[18C�[19;31H[0;30;47m��[17C��[20;34H[1;37;44mE[0;44mx[11C[1;34mr[37m [5;41H[34;47m�[21;41H[0m�[13;24H�[33C�[12;25H�[31C�[14;25H�[31C�[11;25H�[31C�[15;25H�[31C�[10;26H�[29C�[16;26H�[29C�[9;27H[1;47m�[27C[0;45ms[17;27H[40m�[27C�[8;54H�[18;28H[1;47m [25C [7;30H[30;40m�[21C��[19;29H[0;30;47m��[21C��[6;49H[1;34;44m�[0;34m�[20;32H[1;44me[37m [15CT[0;44mr[5;39H[1;34;40m��[C[47m�[40m��[47m��[21;36H[0m�����[C�����[13;23H�[35C�[12;24H�[33C�[14;24H�[33C�[11;24H�[33C�[15;24H�[33C�[10;25H�[31C�[16;25H�[31C�[9;25H[1;47m��[29C[35;45msa[17;25H[0m��[29C��[8;55H�[18;27H[1;47m [27C [7;28H[30;40m�[25C�[19;28H[0;30;47m�[25C�[20;30H[1;34;44mgl[19C[0;44ma[1;34mn[5;47H[40m���[21;33H[0m���[11C���[4;41H[1;34m�[22;41H[30m�[13;22H[0m�[37C�[12;23H�[35C�[14;23H�[35C�[11;23H�[35C�[15;23H�[35C�[10;24H�[33C�[16;24H�[33C�[9;24H[1;47m�[33C[35;45mg[17;24H[0m�[33C�[8;56H��[18;25H[1;47m  [29C  [7;27H[30;40m�[27C�[19;27H[0;30;47m�[27C�[6;29H[1;40m�[20;29H[0;44mg[23C[1;34ms[5;50H[40m۲[21;31H[0m��[17C��[4;38H[1;30msx[2C[34m�[44m�[0;34m�[22;36H[1;30m�����[C�����[13;21H[0m�[39C�[12;22H�[37C�[14;22H�[37C�[11;22H�[37C�[15;22H�[37C�[10;22H��[35C��[16;22H��[35C��[9;23H[1;47m�[35C[35;45me[17;23H[0m�[35C�[8;58H�[18;24H[1;47m [33C [7;26H[30;40m�[29C�[19;26H[0;30;47m�[29C�[6;27H[1m�[40m�[20;27H[37;44mT[0;44mo[25C[1;34mfe[5;52H[0;34m�[21;30H[37m�[21C�[4;49H[1;34m�[22;33H[30m���[11C���[3;41H[34;47m�[13;20H[0m�[41C�[12;21H�[39C�[14;21H�[39C�[11;21H�[39C�[15;21H�[39C�[10;21H�[39C�[16;21H�[39C�[9;22H[1;47m�[37C[35;45m [17;22H[0m�[37C�[8;23H�[35C�[18;23H[1;47m [35C [7;24H[30;40m��[31C��[19;24H[0;30;47m��[31C��[6;26H[1m�[20;26H[37;44m [29C[34mr[5;28H[30;40m�[25C[0mu[21;28H��[23C��[4;50H[1;34m�[22;31H[30m��[17C��[3;35H[34m���[47m���[C[40m���[44m�[0;34m�[13;19H[37m�[43C�[12;20H�[41C[30;47m [14;20H[37;40m�[41C�[11;20H�[41C�[15;20H�[41C�[10;20H�[41C�[16;20H�[41C�[9;21H[1;47m�[39C[45mW[17;21H[0m�[39C�[8;22H�[37C�[18;22H[1;47m [37C [7;59H[30;40m�[19;23H[0;30;47m�[35C�[6;25H[1m�[20;25H[37;44m=[31C[30m�[5;27H[47m�[27C[0ms[21;27H�[27C�[22;29H[1;30m��[21C��[3;33H[34m�[47m�[2;41H[37;40mo[13;18H[0m�[45C�[12;19H[30;47m [43C[37;40m�[14;19H�[43C�[11;19H�[43C�[15;19H�[43C�[10;19H�[43C�[16;19H�[43C�[9;20H[1;47m�[41C[0;45mi[17;20H[40m�[41C�[8;21H�[39C�[18;21H[1;47m [39C [7;22H[30;40m�[37C�[19;22H[0;30;47m�[37C�[6;23H[1;40m�[47m�[20;23H[37;44m1 [33C[47m�[30m�[5;25H��[29C[0min[21;25H��[29C��[22;27H[1;30m��[25C��[3;30H[34m���[2;35H[37mfer[Cpr[Ctocol[13;17H[0m�[47C�[12;18H�[45C�[14;18H�[45C�[11;18H�[45C�[15;18H�[45C�[10;18H�[45C�[16;18H�[45C�[9;19H[1;47m�[43C[0;45mn[17;19H[40m�[43C�[8;20H�[41C�[18;20H[1;47m [41C [7;21H[30;40m�[39C�[19;21H[0;30;47m�[39C�[6;22H[1;40m�[20;22H[37;44mF[37C[30;47m�[5;24H[40m�[33C[0mg[21;24H�[33C�[22;26H[1;30m�[29C�[3;28H[34m��[24C[30m�[2;32H[37mans[13;16H[0m�[49C�[12;17H�[47C�[14;17H�[47C�[11;17H�[47C[30;47m [15;17H[37;40m�[47C�[10;17H�[47C[1;47m [16;17H[0m�[47C�[9;18H[1;47m�[45C[35;45md[17;18H[0m�[45C�[8;19H�[43C�[18;19H[1;47m [43C [7;62H[30;40m�[19;20H[0;30;47m�[41C�[6;21H[1m�[20;21H[37;44m�[39C[0;30;47m�[5;22H[1m�[40m�[36C[0mp[21;22H��[35C��[4;24H[1;30m�[22;24H��[31C��[3;27H[34m�[27C[0m�[2;30H[1mtr[13;15H[0m�[51C�[12;16H�[49C�[14;16H�[49C�[11;16H�[49C[30;47m [15;16H[37;40m�[49C�[10;16H�[49C[1;47m [16;16H[0m�[49C�[9;17H[1;47m�[47C[35;45mo[17;17H[0m�[47C�[8;18H�[45C�[18;18H[1;47m [45C [7;63H[30;40m�[19;19H[0;30;47m�[43C�[6;20H[1;40m�[20;20H[0;30;47m [41C�[5;21H[1m�[39C[0mo[21;21H�[39C�[4;23H[1;30m�[22;23H�[35C�[3;25H[34;44m�[40m�[29C[37m��[2;28Hs[25C[30m([13;14H[0m�[53C�[12;15H�[51C�[14;15H�[51C�[11;15H�[51C[30;47m [15;15H[37;40m�[51C�[10;15H�[51C[1;47m [16;15H[0m�[51C�[9;16H[1;47m�[49C[35;45mw[17;16H[0m�[49C�[8;16H��[47C��[18;16H[1;47m  [47C  [7;17H[30;40m�[46C��[19;17H[0;30;47m��[45C��[20;19H[1;44m�[43C[0;30;47m�[5;20H[1;40m�[41C[0mr[21;20H�[41C�[4;21H[1;30m��[22;21H��[37C��[3;23H[34;44m��[33C[37;40m��[2;26Hsi[27Cc[30m)[13;13H[0m�[55C[30;47m�[12;14H[37;40m�[53C�[14;14H�[53C�[11;14H�[53C[30;47m [15;14H[37;40m�[53C�[10;14H�[53C[1;47m [16;14H[0m�[53C�[9;15H[1;47m�[51C[35;45m [17;15H[0m�[51C�[8;15H�[51C�[18;15H[1;47m [51C [7;66H[30;40m�[19;16H[0;30;47m�[49C�[6;17H[1;40m��[20;17H[0;44mi[1;34mt[45C[0;30;47m��[5;19H[1m�[43C[0mt[21;19H�[43C�[4;20H[1;30m�[22;20H�[41C�[3;22H[34;44m�[37C[37;40m�[2;24Hme[32C[30mM[13;12H[0m�[57C�[12;13H�[55C[30;47m�[14;13H[37;40m�[55C[30;47m�[11;13H[37;40m�[55C[30;47m�[15;13H[37;40m�[55C[30;47m�[10;13H[37;40m�[55C[30;47m�[16;13H[37;40m�[55C[30;47m�[9;14H[1;37m�[53C[0;30;47m�[17;14H[37;40m�[53C�[8;14H�[53C�[18;14H[1;47m [53C [7;67H[30;40m�[19;15H[0;30;47m�[51C�[6;16H[1;40m�[20;16H[0;44mx[49C[30;47m�[5;17H[1;40m��[21;17H[0m��[45C��[4;19H[1;30m�[22;19H�[43C�[3;21H[0;34m�[39C[1;37m�[2;23He[35C[0;36me[13;11H[37m�[1A�[57C�[14;12H�[57C�[11;12H�[57C�[15;12H�[57C�[10;12H�[57C�[16;12H�[57C�[9;13H[1;47m�[55C[0;30;47m�[17;13H[37;40m�[55C[30;47m�[8;13H[37;40m�[55C�[18;13H[1;47m [55C[0;30;47m�[7;14H[1;40m�[53C�[19;14H[0;30;47m�[53C�[6;15H[1;40m�[20;15H[37;44mE[51C[0;30;47m�[5;16H[1;40m�[21;16H[0m�[49C�[4;18H[1;30m�[22;18H�[45C�[3;19H[37m�[43C[30mA[2;22H[37mn[37C[0;36mr[1mc[13;10H[0m�[61C[1;30m�[12;11H[0m�[14;11H�[11;11H�[15;11H�[10;11H�[16;11H�[7A[1;47m�[57C[0m�[17;12H�[57C�[8;12H�[57C�[18;12H[1;47m [57C[0m�[7;13H[1;30m�[55C�[19;13H[0;30;47m�[55C�[6;14H[1;40m�[20;14H[37;44m [53C[0;30;47m�[5;15H[1;40m�[21;15H[0m�[51C�[4;16H[1;47m�[22;16H[30;40m��[47C��[3;18H[37m�[45C[0;36mc[2;20H[1;37m�[41C[36my[13;9H[0m�[1A�[61C[1;30m�[14;10H[0m�[61C[1;30m�[11;10H[0m�[61C[1;30m�[15;10H[0m�[61C[1;30m�[10;10H[0m�[61C[1;30m�[16;10H[0m�[61C[1;30m�[9;11H[37;47m�[17;11H[0m�[8;11H�[18;11H[1;47m [7;70H[30;40m�[19;12H[0;30;47m�[57C[37;40m�[6;13H[1;30m�[20;13H[37;44m=[55C[0;30;47m�[5;14H[1;40m�[21;14H[0m�[53C�[4;15H[1m�[22;15H[30m�[51C�[3;17H[37m�[47C[0;36mi[2;18H[1;37m۲[43Cfu[13;8H[0m�[1A�[14;9H�[11;9H�[15;9H�[10;9H�[16;9H�[7A[1;47m�[61C[30;40m�[17;10H[0m�[61C[1;30m�[8;10H[0m�[61C[1;30m�[18;10H[37;47m [61C[30;40m�[7;11H[0m�[59C[1;30m�[19;11H[0;30;47m�[13A[1;40m�[20;12H[37;44m [57C[0m�[5;13H[1;30m�[21;13H[0m�[55C�[22;14H[1;30m�[53C�[3;15H[37m��[49C[36md[37mi[2;17H�[47Cl[13;7H[0m�[1A�[14;8H�[11;8H�[15;8H�[10;8H�[16;8H�[9;8H[1;47m��[17;8H[0m��[8;9H�[18;9H[1;47m [11A[0m�[61C[1;30m�[19;10H[0;30;47m�[61C[1;40m�[20;11H[37;44mC[15A[30;40m�[21;12H[0m�[57C�[4;13H[1;47m�[22;13H[30;40m�[55C�[3;68H[37mc[2;16H�[1A��[13;6H[0m�[1A�[14;7H�[11;7H�[15;7H�[10;7H�[16;7H�[8A�[18;8H[1;47m [11A[0m�[19;9H[30;47m�[B[1;37;44mS[61C[30;40m�[5;11H�[21;11H[0m�[17A[1m�[22;11H[30m��[57C��[3;13H[37;47m�[1A[40m��[51CFa[13;5H[0;30;47m [1A[37;40m�[14;6H�[11;6H�[15;6H�[10;6H�[16;6H�[9;6H[1;47m��[17;6H[0m��[8;7H�[18;7H[1;47m [11A[0m�[19;8H[30;47m�[6;8H[1;40m��[20;8H[37;44m�E[21;9H[0m��[61C[1;30m�[4;10H[0m�[22;10H[1;30m�[61C�[3;12H[37m�[57C�[2;13H�[55Ct[13;4H[47m�[1A[0;30;47m [14;5H [11;5H [15;5H [10;5H [16;5H [9;5H[1;37m�[17;5H[0;30;47m [9A[37;40m�[18;6H[1;47m [11A[0m�[19;7H[30;47m�[20;7H[1;37m�[B[0m�[17A[1m�[22;9H[30m�[19A[0m�[1;34m�[59C[37m��[2;12H�[57Ce[13;3H[0m�[1A[1;47m�[14;4H�[11;4H�[15;4H�[10;4H�[16;4H�[9;4H�[17;4H�[9A[0m�[18;5H[30;47m [11A[37;40m�[19;6H[30;47m�[6;6H[1;40m�[20;6H[47m�[B[0m�[17A[1;47m�[22;8H[30;40m�[19A[37m�[63C�[2;11H�[12;3H[0m�[14;3H�[11;3H[1;30;47m [15;3H[0m�[10;3H�[16;3H�[9;3H�[17;3H�[9A�[18;4H[1;47m�[7;4H[0m��[19;4H[1;47m�[0;30;47m�[20;5H[1m�[B[0m�[17A[1m�[22;7H[30m�[19A[37;47m�[65C[40m�[2;10H�[61C2[13;1H[30;47m�[8;3H[0m�[18;3H�[7;3H�[19;3H�[13A[1;30m�[20;4H[0;30;47m�[15A[1;37;40m�[21;5H[0m�[B[1;30m�[19A[37m�[67C[0m�[2;8H[1;47m�[40m�[63C[0mk[1m3[12;1H[30m�[14;1H[47m�[11;1H�[15;1H[40m�[10;1H�[16;1H�[9;1H�[17;1H�[6;3H�[20;3H[0m�[15A[1m�[21;4H[0m�[17A[1m�[22;5H[30m�[19A[37m�[69C[30m�[2;75H/[8;1H�[18;1H[47m�[7;1H�[19;1H[40m�[13A�[1A[37m�[21;3H[0m�[17A[1m�[22;4H[30m�[19A[37m�[1A�[69C4[6;1H[30m�[20;1H�[4;3H[37m�[22;3H[30m�[19A[37m�[1A�[21;1H[30m�[B�[19A[37m�[2;3H��[22;1H[30m�[3;1H[34;44m�[1;1H[37;40m [0m"};

// Transfer Window with Statistics
char *aBuf2={"[40m[2J[2B[0;1;34;44m�[2;4H[37;40m�[2D�[3;3H�[2;5H�[3;4H�[4;3H�[6;1H[30m�[2;76H[37m4[71D�[3;5H�[4;4H�[5;3H�[6;2H[30m�[19;2H�[2D�[7;1H[47m�[18;1H[40m�[8;1H�[2;75H/[B�[71D[37m�[4;5H�[5;4H�[6;3H[30m�[17;1H�[9;1H�[16;1H�[10;1H�[15;1H[47m�[11;1H�[14;1H�[12;1H[40m�[2;74H[37m3[2D[0mk[65D[1m�[2D[47m�[3;75H[0m�[69D[1m�[5;5H�[6;4H[30m�[19;3H�[7;3H[0m�[18;3H�[8;3H�[13;1H[1;30;47m�[2;72H[37;40m2[63D�[3;74H�[67D[47m�[4;7H[40m�[19;5H[30m�[2D�[12A[0m�[2D�[18;4H�[8;4H�[17;3H�[9;3H�[16;3H�[10;3H�[15;3H�[11;3H[1;30;47m [14;3H[0m�[12;3H�[2;11H[1m�[3;73H�[65D�[4;8H[47m�[6;6H[30;40m�[19;6H�[7;6H[0m�[18;5H�[8;5H�[17;4H[30;47m�[9;4H[1;37m�[16;4H�[10;4H�[15;4H�[11;4H�[14;4H�[12;4H�[13;3H[0m�[2;70H[1me[59D�[3;72H�[2D�[61D[34m�[2D[0m�[4;9H[1m�[19;7H[30m�[7;7H[0m�[18;6H�[8;6H�[17;5H[1;30;47m�[9;5H[37m�[16;5H[0;30;47m�[10;5H [15;5H [11;5H [14;5H [12;5H [13;4H[1;37m�[2;69H[40mt[57D�[3;70H�[59D�[4;10H[0m�[6;9H[1;30m�[2D�[19;8H�[7;8H[0m�[18;7H�[8;7H�[17;7H[1;47m�[2D[30m�[8A[37m�[2D�[16;6H[0;30;47m�[10;6H[1;37mf[15;6H[0;30;47m�[11;6H[1;37mf[14;6Ht[12;6Hb[13;5H[0;30;47m [2;68H[1;37;40ma[2DF[53D�[2D�[3;13H[47m�[4;12H[40m�[5;11H[30m�[19;9H�[7;9H[0m�[18;8H�[8;8H�[16;7H[30;47m�[10;7H[1;37mi[15;7H[0;30;47m-[11;7H[1;37mi[14;7Hi[12;7Hy[13;6H[0;30;47m�[1;18H[1;37;40m�[2D�[2;16H�[3;68Hc[4;13H[47m�[5;12H[30;40m�[19;72H�[63D�[7;72H�[63D[0m�[18;9H�[8;9H�[17;9H[1;44mE[2D�[8A[47m�[2D�[16;8H[0;30;47m�[10;8H[1;37ml[15;8Hl[11;8Hl[14;8Hm[12;8Ht[13;7H[0;30;47m-[2;65H[1;37;40ml[49D�[3;67Hi[2D[36md[51D[37m�[2D�[5;13H[30m�[6;12H�[19;71H�[61D�[7;71H�[61D[0m�[18;72H[1;30m�[63D[0m�[8;72H[1;30m�[63D[0m�[17;72H[1;30m�[63D[37;44mS[9;72H[30;40m�[63D[37;47m�[16;9H[0;30;47m�[10;9H[1;37me[15;9He[11;9He[14;9He[12;9He[13;8Hl[2;64H[40mu[2Df[45D�[2D�[3;65H[0;36mi[49D[1;37m�[4;15H�[5;14H[30m�[6;13H�[19;70H�[59D�[7;70H�[18;11H[0m�[8;11H�[17;11H[1;44mC[9;11H[47m�[16;72H[30;40m�[63D[0;30;47m�[10;72H[1;40m�[63D[37;47mn[15;72H[30;40m�[63D[37;47mf[11;72H[30;40m�[63D[37;47ms[14;72H[30;40m�[63D[37;47m [12;72H[30;40m�[63D[37;47ms[13;9He[2;62H[36;40my[43D[37m�[3;64H[0;36mc[47D[1;37m�[4;16H[47m�[5;15H[30;40m�[6;14H�[19;69H�[57D�[7;69H�[57D�[18;70H[0m�[59D�[8;70H�[59D�[17;70H�[59D[1;44m [9;70H[0m�[59D[1;47m�[16;11H[0;30;47m�[10;11H[1;37ma[15;11Ht[11;11Hi[14;11He[12;11H [13;72H[30;40m�[63D[37;47mf[2;61H[36;40mc[2D[0;36mr[39D[1;37mn[3;63H[30mA[45D[37m�[4;18H[30m�[5;16H�[6;15H�[19;68H�[55D�[7;68H�[55D�[18;69H[0m�[57D�[8;69H�[57D�[17;69H[30;47m�[57D[1;37;44m=[9;69H[0;30;47m�[57D[1;37m�[16;70H[0m�[59D[30;47m�[10;70H[37;40m�[59D[1;47mm[15;70H[0m�[59D[1;47m [11;70H[0m�[59D[1;47mz[14;70H[0m�[59D[1;47ml[12;70H[0m�[59D[1;47mr[13;11Ht[2;59H[0;36me[37D[1;37me[3;61H�[41D[0;34m�[4;19H[1;30m�[5;18H�[2D�[6;16H�[19;67H�[53D�[7;67H�[11B[0m�[55D�[8;68H�[55D�[17;68H[30;47m�[55D[1;37;44m [9;68H[0;30;47m�[55D[1;37m�[16;69H[0;30;47m�[57D�[10;69H�[57D[1;37me[15;69H[0;30;47m�[57D[1;37m([11;69H[0;30;47m�[57D[1;37me[14;69H[0;30;47m�[57D[1;37ma[12;69H[0;30;47m�[57D[1;37me[13;70H[0m�[59D[1;47m [2;58H[30;40mM[34D[37me[2Dm[3;60H�[39D[34;44m�[4;20H[30;40m�[5;19H[47m�[6;18H[40m�[2D�[19;66H�[51D�[7;66H�[11B[0m�[53D�[8;67H�[53D�[17;67H[30;47m�[53D[1;37;44mE[9;67H[35;45m [53D[37;47m�[16;68H[0;30;47m�[55D�[10;68H[1;37m [55D[0;30;47m [15;68H[1;37m [55De[11;68H[0;30;47m [55D[37;40m�[14;68H�[55D[1;47mp[12;68H[0m�[55D[1;47mc[13;69H[0;30;47m�[57D[1;37m [2;56H[30;40m)[2D[37mc[29Di[2Ds[3;59H�[2D�[35D[34;44m�[2D�[4;22H[30;40m�[2D�[5;20H�[19;65H�[2D�[47D�[2D�[7;65H�[2D�[48D�[18;66H[0m�[2D�[49D�[2D�[8;66H�[2D�[49D�[2D�[17;66H[30;47m�[51D[37;44mx[9;66H[1;35;45mr[51D[37;47m�[16;67H[0;30;47m�[53D�[10;67H[1;37m [53D[0m�[15;67H[1;47m [53Ds[11;67H[0;30;47m [53D[37;40m�[14;67H�[53D[1;47ms[12;67H[0m�[53D[1;47me[13;68H[0m�[55D[1;47m [2;54H[30;40m([27D[37ms[3;57H�[2D�[31D[34m�[2D[44m�[4;23H[30;40m�[5;21H[47m�[6;20H[40m�[19;63H�[45D�[7;63H�[11B[0m�[47D�[8;64H�[47D�[17;65H[30;47m�[49D[37;44mi[9;65H[1;35;45me[49D[37;47m�[16;66H[0;30;47m�[51D�[10;66H[1;37m [51D[0m�[15;66H[1;47m [51Dt[11;66H[0;30;47m [51D[37;40m�[14;66H�[51D[1;47me[12;66H[0m�[51D[1;47mi[13;67H[0m�[53D[1;47m [2;31H[40mr[2Dt[3;55H[0m�[29D[1;34m�[4;24H[30m�[5;23H�[2D[47m�[6;21H�[19;62H[40m�[43D�[7;62H�[11B[0m�[45D�[8;63H�[45D�[17;64H[30;47m�[47D[1;34;44mt[9;64H[35;45mf[47D[37;47m�[16;65H[0;30;47m�[49D�[10;65H[1;37m [49D[0m�[15;65H[1;47m [49D.[11;65H[0;30;47m [49D[37;40m�[14;65H�[49D[1;47md[12;65H[0m�[49D[1;47mv[13;66H[0m�[51D[1;47m [2;34H[40ms[2Dn[2Da[3;54H[30m�[26D[34m�[2D�[5;24H[30m�[6;22H�[19;61H�[41D�[7;61H�[41D�[18;62H[0m�[43D�[8;62H�[43D�[17;63H[30;47m�[45D[1;44m�[9;63H[35;45ms[45D[37;47m�[16;64H[0;30;47m�[47D�[10;64H[37;40m�[47D�[15;64H[1;47m [47D)[11;64H [47D[0m�[14;64H�[47D�[12;64H�[47D[1;47me[13;65H[0m�[49D[1;47m [2;46H[40ml[2Do[2Dc[2Do[2Dt[3Dr[2Dp[3Dr[2De[2Df[3;32H[34m�[2D�[2D�[5;56H[30m:[31D[47m�[2D�[6;24H�[2D[40m�[19;60H�[39D�[7;60H�[39D�[18;61H[0m�[41D�[8;61H�[41D�[17;62H[30;47m�[43D [9;62H[1;35;45mn[43D[37;47m�[16;63H[0;30;47m�[45D�[10;63H[1;37m [45D[0m�[15;63H�[45D[1;47m [11;63H[0;30;47m�[45D[37;40m�[14;63H�[45D�[12;63H[1;47m [45Dd[13;64H[0m�[47D[1;47m [2;41H[40mo[3;34H[34;47m�[2D[40m�[5;55H[0mp[29D[1;30;47m�[6;25H�[19;59H[40m�[37D�[7;59H�[11B[0m�[39D�[8;60H�[39D�[17;61H[30;47m�[41D[1;37;44m�[9;61H[0;45ma[41D[1;47m�[16;62H[0;30;47m�[43D�[10;62H[37;40m�[43D�[15;62H�[43D[1;47m [11;62H[0;30;47m�[43D[37;40m�[14;62H�[43D�[12;62H[1;47m [43D[0m�[13;63H�[45D[1;47m [3;46H[0;34m�[2D[1;44m�[2D[40m�[2D�[2D�[3D[47m�[2D�[2D�[2D[40m�[2D�[2D�[4;50H�[5;54H[0mi[27D[1;30m�[6;26H[47m�[19;58H[40m�[2D�[33D�[2D�[7;58H�[2D�[33D�[2D�[18;59H[0m�[37D�[8;59H�[37D�[17;60H[1;30;47m�[39D[37;44mF[9;60H[0;45mr[39D[1;47m�[16;61H[0;30;47m�[41D�[10;61H[37;40m�[41D[1;47m-[15;61H[0m�[41D[1;47m-[11;21H-[14;61H[0m�[41D[1;47m-[12;61H [41D-[13;62H[0m�[43D[1;47m [3;41H[34m�[4;49H[40m�[5;52H[0;34m�[6;28H[1;30m�[2D[47m�[19;56H[40m�[31D�[7;56H�[31D�[18;58H[0m�[35D�[8;58H�[9B[1;30;47m�[37D[37;44m1[9;59H[45mT[37D[47m�[16;60H[0;30;47m�[2D�[37D�[2D�[10;60H[37;40m�[2D�[37D�[2D[30;47m>[15;60H[37;40m�[39D[30;47m>[11;22H>[14;60H[37;40m�[39D[30;47m>[12;60H[1;37m [39D[0;30;47m>[13;61H[37;40m�[41D[1;47m-[4;44H[0;34m�[2D[1;44m�[2D[40m�[4D[30mx[2Ds[5;51H[34m�[2D�[6;29H[30m�[19;55H�[29D�[7;55H�[29D�[18;57H[0m�[2D�[31D�[2D�[8;57H�[2D�[17;58H[1;47m�[35D[44m [9;58H[35;45m [35D[37;47m�[16;58H[0;30;47m�[35D�[10;58H[37;40m�[35D�[15;59H�[37D[1;47m [11;23H[0m�[14;59H�[37D�[12;59H[1;47m [37D[0m�[13;60H�[39D[30;47m>[4;41H[1;34;40m�[5;49H�[2D�[2D�[19;54H[30m�[27D�[7;54H�[27D�[18;55H[0m�[29D�[8;55H�[17;57H[1;30;44m�[2D[34mr[31D[37m [2D=[9;57H[35;45mt[2Dn[31D[37;47m�[2D�[16;57H[0;30;47m�[33D�[10;57H[37;40m�[33D�[15;58H�[35D[1;47m [11;24H[0m�[14;58H�[35D�[12;58H[1;47m [35D[0m�[13;59H�[37D�[5;46H[1;34;47m�[2D�[2D[40m�[2D�[2D[47m�[3D[40m�[2D�[6;50H[0;34m�[2D[1;44m�[19;53H[30;40m�[2D�[23D�[2D�[7;53H�[2D�[23D�[18;54H[0m�[27D�[8;54H�[9B[1;34;44me[29D[37mT[9;55H[35;45me[29D[37;47m�[16;56H[0;30;47m�[31D�[10;56H[37;40m�[31D�[15;57H�[33D[1;47m [11;25H[0m�[14;57H�[33D�[12;57H[1;47m [33D[0m�[13;58H�[35D�[5;41H[1;34;47m�[19;51H[30;40m�[2D�[19D�[2D�[7;51H�[20D�[2D�[18;53H[0m�[25D�[8;53H�[25D�[17;54H[1;34;44mf[27D[0;44mo[9;54H[1;35;45mr[27D[37;47m�[16;55H[0;30;47m�[29D�[10;55H[37;40m�[29D�[15;56H�[31D[1;47m [11;26H[0m�[14;56H�[31D�[12;56H[1;47m [31D[0m�[13;57H�[33D�[19;49H[1;30m�[17D�[7;33H�[18;52H[0m�[2D�[21D�[2D�[8;52H�[2D�[21D�[2D�[17;53H[1;34;44ms[25D[0;44mg[9;53H[45mr[25D[1;47m�[16;54H[0;30;47m�[27D�[10;54H[37;40m�[27D�[15;55H�[29D[1;47m [11;27H[0m�[14;55H�[29D�[12;55H[1;47m [29D[0m�[13;56H�[31D�[19;48H[1;30m�[2D�[2D�[11D�[2D�[2D�[7;47H�[2D�[11D�[2D�[2D�[18;50H[0m�[19D�[8;50H�[19D�[17;52H[1;34;44mn[23Dg[9;52H[0;45mu[23D[1;47m�[16;53H[0;30;47m�[25D�[10;53H[37;40m�[25D�[15;54H�[27D[1;47m [11;28H[0m�[14;54H�[27D�[12;54H[1;47m [27D[0m�[13;55H�[29D�[19;45H[1;30m�[2D�[2D�[2D�[3D�[2D�[2D�[2D�[7;45H�[2D�[2D�[2D�[3D�[2D�[2D�[2D�[18;49H[0m�[2D�[15D�[2D�[8;49H�[2D�[15D�[2D�[17;51H[44ma[2Dr[19D[1;34me[2Dl[9;51H[37;45mC[2D[35m [19D[37;47m�[2D�[16;52H[0;30;47m�[23D�[10;52H[37;40m�[23D�[15;53H�[25D[1;47m [11;29H[0m�[14;53H�[25D�[12;53H[1;47m [25D[0m�[13;54H�[27D�[19;41H[1;30m�[7;41H�[18;47H[0m�[2D�[11D�[2D�[8;47H�[2D�[11D�[2D�[17;49H[1;44mT[17D [9;49H[47m�[17D�[16;51H[0;30;47m�[21D�[10;51H[37;40m�[21D�[15;52H�[23D[1;47m [11;30H[0m�[14;52H�[23D�[12;52H[1;47m [23D[0m�[13;53H�[25D�[18;45H�[2D�[2D�[2D�[3D�[2D�[2D�[2D�[8;45H�[2D�[2D�[2D�[3D�[2D�[2D�[2D�[17;48H[1;44m [2D[34mr[13D[0;44mx[2D[1mE[9;48H[47m�[2D�[13D�[2D�[16;50H[0;30;47m�[2D�[17D�[2D�[10;50H[37;40m�[2D�[17D�[2D�[15;51H�[21D[1;47m [11;31H[0m�[14;51H�[21D�[12;51H[1;47m [21D[0m�[13;52H�[23D�[18;41H�[8;41H�[17;46H[1;34;44me[11D[0;44mi[9;46H[1;47m�[11D�[16;48H[0;30;47m�[15D�[10;48H[37;40m�[15D�[15;50H�[19D[1;47m [11;32H[0m�[14;50H�[19D�[12;50H[1;47m [19D[0m�[13;51H�[21D�[17;45H[44mt[2Df[2D[1mA[2D [3D [2D([2D [2D[34mt[9;45H[37;47m�[2D�[2D�[2D�[3D�[2D�[2D�[2D�[16;47H[0;30;47m�[13D�[10;47H[37;40m�[13D�[15;49H�[2D[30;47m>[15D[1;37m [2D [4A[0m�[2D�[14;49H�[17D�[12;49H[1;47m [17D[0m�[13;50H�[19D�[17;41H[1;44m)[9;41H[47m�[16;46H[0;30;47m�[2D�[9D�[2D�[10;46H[37;40m�[2D�[9D�[2D�[15;47H[1;47m-[13D [11;35H[0m�[14;48H[30;47m>[15D[37;40m�[12;48H[1;47m [15D[0m�[13;49H�[17D�[16;44H[30;47m�[2D�[2D�[3D�[2D�[2D�[10;44H[37;40m�[2D�[2D�[3D�[2D�[2D�[15;46H[1;47m [11D [11;36H[0m�[14;47H[1;47m-[13D[0m�[12;47H[1;47m [13D[0m�[13;48H[30;47m>[15D[37;40m�[16;41H[30;47m�[10;41H[37;40m�[15;45H[1;47ms[2Du[7D [2D [4A[0m�[2D�[14;46H[1;47m [11D[0m�[12;46H[1;47m [11D[0;30;47m [13;47H[1;37m-[13D[0m�[15;43H[1;47mt[2Da[3Ds[2D [4A�[2D[0m�[14;45H[1;47m [9D[0m�[12;45H[1;47m [9D[0m�[13;46H[1;47m [11D[0m�[15;41H[1;47mt[11;41H�[14;44He[7D[0m�[12;44H[1;47m [7D[0m�[13;45H[1;47m [9D[0m�[14;43H[1;47mu[2De[3Dq[2D[0m�[12;43H[1;47m [2D [3D [2D[0m�[13;44H[1;47md[7D[0m�[14;41H[1;47mu[12;41H [13;43He[5D[0m�[2C[1;47me[2Dp[2Ds[1;1H[40m [0m"};
#endif


// Sleep / Wait
void nsleep(int i) {

    #ifdef _WIN32
    Sleep(i*1000);
    #else
    sleep(i);
    #endif
}

// Close Sockets
void closesocks() {

    #ifdef _WIN32
    closesocket(msock.rsock);
    #else
    close(msock.rsock);
    #endif
}

// Transfer / Error Logs
void logntp(char *text) {

    if (LOGGING == false) return;

    FILE *fp1;
    if((fp1 = fopen("ntp-user.log", "a")) == NULL) {
        printf("Fatal! Couldn't write to log file!\n");
        return;
    }
    fprintf(fp1, "* %s\n",text);
    fclose(fp1);
}

// Exit Program
void exitprogram() {

    #ifdef _WIN32
    // Turn Windows Cursor back on before exiting
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO CCI;
    GetConsoleCursorInfo(hOut, &CCI);
    CCI.bVisible = true;
    SetConsoleCursorInfo(hOut, &CCI);        
    system("cls");
    #else
    system("clear");
    #endif
    printf("\nNTP Exited .. .\n");
    
    char msg[100]={0};
    #ifdef _WIN32
    // Date/Time Stamp log on Each Conection
    GetTimeFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTimeFormat, 50 );
	GetDateFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szDateFormat, 50 );
	sprintf(msg,"Date/Time: %s %s",szDateFormat,szTimeFormat);
    logntp(msg);
    WSACleanup();
    setcolor(7,0);
    #endif
    exit(0);
}

// Check Filesize
long checksize() {

    long size = 0;
    std::string dlpath = DLPATH;
    dlpath += finfo.filename;

    #ifdef _WIN32
    // Read File and Get File Size
    WIN32_FIND_DATA wfd;   // File Structure
    HANDLE          h;     // Handle to File      
    
    h = FindFirstFile(dlpath.c_str(), &wfd);
    if(h != INVALID_HANDLE_VALUE) { size = wfd.nFileSizeLow; }
    FindClose(h);

    #else
    // Open File and Read File Size
    FILE *fp;
    if ((fp = fopen(dlpath.c_str(), "rb")) ==  NULL) {
        // Just means theirs no file, so were receiving a new file! :)
        return (0);
    }
    else {
    #ifdef OS2
    size = filelength(fileno(fp));
    #else
    fseek(fp, 0L, SEEK_END);    /* Position to end of file */
    size = ftell(fp);           /* Get file length */
    #endif
    }
    fclose(fp);
    #endif
    
    return (size);
}

// Check for File Recovery
int resume() {

	// Check if we are getting New File or Resuming Old
	long totbyte      =0;
	char message[255] ={0};
	char rezBuf[2048] ={0};
	char tBuf[2048]   ={0};
	int  nRet;
	
    flsz = checksize();
    sprintf(rezBuf,"%i\r\n\r\n",flsz);   // Create Packet with File Size in Bytes
                                                                                     
    // If Filesize 0, error!
    if (flsz == 0) {    
        for (;;) {        	
		  	nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
	       	if (nRet == INVALID_SOCKET)	{
                // Giev 10 retries before error and disconencting
                for (int retry = 0; ;retry++) {
                    nsleep(1);
                    nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
                    if (retry == 10) {
           	       	    #ifdef _WIN32
                        sprintf(message, "INVALID_SOCKET - %i",WSAGetLastError());
                        #else
                        sprintf(message, "INVALID_SOCKET - %i",errno);
                        #endif
                        logntp(message);
        	       	    strcpy(finfo.status,message);
        				memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                    }
                    else if (nRet == INVALID_SOCKET) continue;
                    else if (nRet == LOST_SOCKET) {
                        #ifdef _WIN32
                        sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
                        #else
                        sprintf(message, "LOST_SOCKET - %i",errno);
                        #endif
                        logntp(message);
    	   		      	strcpy(finfo.status,message);
                     	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                    	closesocks();
	       		        return (2);
                    }
                    else break;
                }
			}
		   	else if (nRet == LOST_SOCKET)	{
				#ifdef _WIN32
                sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
                #else
                sprintf(message, "LOST_SOCKET - %i",errno);
                #endif
                logntp(message);
				strcpy(finfo.status,message);
            	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
            	closesocks();
			    return (2);
 			}
   			return (1);
  		}
    }
    // If we already have the full file, Error
    else if (flsz == finfo.size) { // Exit if the original file is same size as new        
        for (;;) {        	
		  	nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
	       	if (nRet == INVALID_SOCKET)	{
	       	    // Giev 10 retries before error and disconencting
                for (int retry = 0; ;retry++) {
                    nsleep(1);
                    nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
                    if (retry == 10) {
           	       	    #ifdef _WIN32
                        sprintf(message, "INVALID_SOCKET - %i",WSAGetLastError());
                        #else
                        sprintf(message, "INVALID_SOCKET - %i",errno);
                        #endif
				        logntp(message);
				        strcpy(finfo.status,message);
	       		        memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                    }
                    else if (nRet == INVALID_SOCKET) continue;
                    else if (nRet == LOST_SOCKET) {
                        #ifdef _WIN32
                        sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
                        #else
                        sprintf(message, "LOST_SOCKET - %i",errno);
                        #endif
                        logntp(message);
    	   		      	strcpy(finfo.status,message);
                     	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                    	closesocks();
	       		        return (2);
                    }
                    else break;
                }
			}
		   	else if (nRet == LOST_SOCKET)	{
            	//Draw Transfer Status				
				#ifdef _WIN32
                sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
                #else
                sprintf(message, "LOST_SOCKET - %i",errno);
                #endif
				logntp(message);
				strcpy(finfo.status,message);
            	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
            	closesocks();
				return (2);
 			}

     		sprintf(message,"Error: You Already Have This Full File!");
			logntp(message);
		    strcpy(finfo.status,message);
     		memcpy(&finfo2,&finfo,sizeof(FILEINFO));
     		closesocks();
     		return (2);
  		}
    }

    // All is good, continue!
    for (;;) {
	    nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
	    if (nRet == INVALID_SOCKET)	{
            for (int retry = 0; ;retry++) {
                nsleep(1);
                nRet = send(msock.rsock, rezBuf, sizeof(rezBuf), 0);
                if (retry == 10) {
           	        #ifdef _WIN32
                    sprintf(message, "INVALID_SOCKET - %i",WSAGetLastError());
                    #else
                    sprintf(message, "INVALID_SOCKET - %i",errno);
                    #endif
                    logntp(message);
        	        strcpy(finfo.status,message);
        		    memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                }
                else if (nRet == INVALID_SOCKET) continue;
                else if (nRet == LOST_SOCKET) {
                    #ifdef _WIN32
                    sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
                    #else
                    sprintf(message, "LOST_SOCKET - %i",errno);
                    #endif
                    logntp(message);
    	   		  	strcpy(finfo.status,message);
                  	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                  	closesocks();
	       		    return (2);
                }
                else break;
            }
		}
		else if (nRet == LOST_SOCKET)	{
			#ifdef _WIN32
            sprintf(message, "LOST_SOCKET - %i",WSAGetLastError());
            #else
            sprintf(message, "LOST_SOCKET - %i",errno);
            #endif
			logntp(message);
			strcpy(finfo.status,message);
            memcpy(&finfo2,&finfo,sizeof(FILEINFO));
			closesocks();
			return (2);
		}
  		break;
	}
	return (0);
}

// Transfer Percentage 0-100
double percentage(long double wk, long double pk) {

    double p = (0.0);
    if (pk == 0) return (0.0);
    if ( wk > 0 ) {
        if (wk == pk) return (100.0);
        else {
            p = 100 * (wk / pk);
            return p;
        }
    }
    return (0.0);
}

// Transfer GUI
#ifdef _WIN32
void TransferGUI( void *p ) {
#else
void *TransferGUI( void *p ) {
#endif

    // Receive data from the client
    char fRecv[50]  ={0};             // Bytes Received [Buffer]
    char fLeft[50]  ={0};             // Bytes Left     [Buffer]
    char fSpeed[50] ={0};             // Bytes Speed    [Buffer]
    char tleft[50]  ={0};			  // Time Left      [Buffer]
    long lftbyte    =0;               // Total Bytes Left
    long spdbyte    =0;               // Bytes Transfer in 1 Second
    long totsec     =0;               // Total Seconds Left
    long elsec      =0;               // Total Seconds Elapsed
    int  mm[2];                       // Real Time Left in Minutes
    int  ss[2];						  // Real Time Left in Seconds
    int  hh[2];						  // Real Time Left in Seconds
    double pCent;                     // Percentage Complete
    char Status[50] ={0};             // Transfer Status
    char h[5]; char m[5]; char s[5];  // Time Display
    
    int  avg[6]  ={0};                // Averages for Speed
    long lastavg = 0;                 // Averages for speed
    
    int timeout  = 0;                 // 120 Second Timeout if no data sent

    // Display Filesize
    char fSize[255];
    sprintf(fSize,"%.f ",(float)finfo.size);
           
    // Draw Transfer Status
    if (resum) sprintf(Status,"Recovery!");
    else sprintf(Status,"Transfering! ");
	    
    // Give 1 Second for File Data to Catch up to GUI
    nsleep(1);
    
    #ifdef _WIN32
    // Display Transfer GUI
   	ansiparse(aBuf2);      
    // Display Filename + Filesize + Status
    drawfilename(finfo.filename,15,7);    
    drawfilesize(fSize,15,7);
    drawstatus(Status,9,7);    
    
    queuestatus(finfo.bqueue,0,7);
    drawpercent(0.0);           
    // Node Status
    gotoxy(54,5);
    setcolor(7,0); printf("IP: "); setcolor(15,0); printf("%s",inet_ntoa(their_addr.sin_addr));
    
    // Test if F1 Toggle is on
    if (F1) {
        gotoxy(40,17); setcolor(7,1); printf("*");
    }
    else {
        gotoxy(40,17); setcolor(7,1); printf(" ");
    }
    #endif       

    // Loops Transfer Statistics
    for(;;) {    

        // Break if Transfer Complete
    	if ((finfo.size == finfo.bRecv) || (erbreak)) break;
        
        // Display Transfer Header
        #ifndef _WIN32
        #ifdef OS2
        system("cls");
        #else
        system("clear");
        #endif
        printf("\n.----------------------------------------------------------------.");
        printf("\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |");
        printf("\n`----------------------------------------------------------------'\n");
        printf("\nIP: %s\n\nFilename   : %s\nFilesize   : %.f\n",inet_ntoa(their_addr.sin_addr),finfo.filename,(float)finfo.size);
        printf("\nBatch Queue: %s\n",finfo.bqueue);
        #endif

        // Draw File Bytes Received
        sprintf(fRecv,"%.f ",(float)finfo.bRecv);
  	    #ifdef _WIN32
       	drawreceived(fRecv,9,7);
  	    #else
        printf("\nBytes Recv : %s",fRecv);
        #endif

      	// Calculate Bytes Left
        lftbyte = (finfo.size - finfo.bRecv);
        if (lftbyte < 0) { lftbyte = 0; }
      	sprintf(fLeft,"%.f ",(float)lftbyte);
       	#ifdef _WIN32
        drawleft(fLeft,9,7);
        #else
        printf("\nBytes Left : %s\n",fLeft);
        #endif

        // Calculate Average Bytes Per Second, 5 Second Average
        avg[0] = finfo.bRecv - finfo.lRecv;
        avg[5] = avg[4]; avg[4] = avg[3]; avg[3] = avg[2]; avg[2] = avg[1]; avg[1] = avg[0];                      
                       
        spdbyte = avg[1]+avg[2]+avg[3]+avg[4]+avg[5];
        if ((spdbyte < lastavg) && (spdbyte != 0))
            if ((spdbyte - lastavg) > 10)                
                spdbyte = lastavg;

        // Calculate Average Bytes Transfered Per Second 
        lastavg = spdbyte;                           
        spdbyte /= 5;            
        if (spdbyte < 0) spdbyte = 0;
        
        // Check for timeout if == 20 seconds, exit program, lost connection!
        if (spdbyte <= 0) ++timeout;
        else timeout = 0;
        
        // Test this more later..
        if (timeout == 120) timeout = 0;
        /*
        if (timeout == 120) {
            logntp("Transfer Timed out! Exiting.. .");
            
            //exitprogram();
        }*/

        // Calucate Time in Seconds Left of Transfer
        if ((spdbyte != 0) && (lftbyte != 0)) {
 	        totsec = lftbyte / spdbyte;
      	}
  	    else { totsec = 0; }

        // Calculate Speed KB/S
        if (spdbyte > 1000) {
            spdbyte /= 1000; // Convert to KiloBytes/Sec
            sprintf(fSpeed,"%.f kb/sec",(float)spdbyte);
        } // Else to slow to show a Kilobyte, Display in Bytes
        else sprintf(fSpeed,"%.f b/sec",(float)spdbyte);
        #ifdef _WIN32
        drawspeed(fSpeed,15,7);
        #else
        printf("\nSpeed      : %s\n",fSpeed);
        #endif

        // Convert Time left in Seconds to Hours : Mintues : Seconds
        if (totsec == 0) {
            hh[0] = 0; mm[0] = 0; ss[0] = 0;
        }
        else {            
            if (totsec < 60) { hh[0] = 0; mm[0] = 0; ss[0] = totsec; }
            else {                
                hh[0] = 0; mm[0] = totsec / 60; ss[0] = totsec % 60;
                if ( mm[0] > 60) { hh[0] = mm[0] / 60; mm[0] = mm[0] % 60; }
            }
        }
        // Add Leading 0's if under 10
        if (hh[0] < 10) sprintf(h,"0%i",hh[0]);
        else sprintf(h,"%i",hh[0]);

        if (mm[0] < 10) sprintf(m,"0%i",mm[0]);
        else sprintf(m,"%i",mm[0]);

        if (ss[0] < 10) sprintf(s,"0%i",ss[0]);
        else sprintf(s,"%i",ss[0]);

        // Draw Time
        sprintf(tleft,"%s:%s:%s ",h,m,s);
        #ifdef _WIN32
        drawtleft(tleft,15,7);
        #else
        printf("\nTime Left  : %s",tleft);
        #endif
        
        // Convert Time Elapsed in Seconds to Hours : Mintues : Seconds
        if (elsec == 0) {
            hh[1] = 0; mm[1] = 0; ss[1] = 0;
        }
        else {
            if (elsec < 60) { hh[1] = 0; mm[1] = 0; ss[1] = elsec; }
            else {                
                hh[1] = 0; mm[1] = elsec / 60; ss[1] = elsec % 60;
                if ( mm[1] > 60) { hh[1] = mm[1] / 60; mm[1] = mm[1] % 60; }
            }
        }
        // Add Leading 0's if under 10
        if (hh[1] < 10) sprintf(h,"0%i",hh[1]);
        else sprintf(h,"%i",hh[1]);

        if (mm[1] < 10) sprintf(m,"0%i",mm[1]);
        else sprintf(m,"%i",mm[1]);

        if (ss[1] < 10) sprintf(s,"0%i",ss[1]);
        else sprintf(s,"%i",ss[1]);
        
        // Draw Time Elapsed
        sprintf(finfo.telapsed,"%s:%s:%s",h,m,s);
        #ifdef _WIN32
        drawtelapse(finfo.telapsed,15,7);
        #else
        printf("\n> Elapsed  : %s\n",finfo.telapsed);
        #endif

        // Calculate Percentage
        pCent = percentage(finfo.bRecv,finfo.size);
        // Fix Percentage displaying 100 to early!
        if (pCent > 98) pCent = 99;
        #ifdef _WIN32
  	    percenttop(pCent);
   	    drawpercent(pCent);
      	#else
      	printf("\nComplete   : %.f%%",(float)pCent);
        printf("\nStatus     : %s\n",Status);        
   	    #endif

        // Setup to Calculate for KB/S
        finfo.lRecv = finfo.bRecv;

        // Update Transfer Stats Every Second
        nsleep(1);
        ++elsec;
    } 
}

// Receive File Data
void recvfdata() {

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
	if(resum) { // True - append
		totbyte = flsz;
		if ((nfp = fopen(dlpath.c_str(), "a+b")) ==  NULL) {
			sprintf(message,"Error: Can't create file: '%s'\n",finfo.filename);
			erbreak = true;
			logntp(message);
			strcpy(finfo.status,message);
			memcpy(&finfo2,&finfo,sizeof(FILEINFO));
			closesocks();
			return;
		}
	}
	else { // False - overwrite
		if ((nfp = fopen(dlpath.c_str(), "w+b")) ==  NULL) {
			sprintf(message,"Error: Can't create file: '%s'\n",finfo.filename);
			erbreak = true;
			logntp(message);
			strcpy(finfo.status,message);
			memcpy(&finfo2,&finfo,sizeof(FILEINFO));
			closesocks();
			return;
		}
	}	
	
	// Start GUI Transfer Threads
    #ifdef _WIN32
    HANDLE ahThread;
    ahThread = (HANDLE)_beginthread( TransferGUI, 0, (void *)NULL);   
    #else
    pthread_t thread;
    pthread_create(&thread, NULL, TransferGUI, (void *)NULL);
    #endif
    
    // Give a slight pause for GUI to Finish Drawing before starting
    nsleep(1);
    
	// Receive data from the client
	j = 1;
	while (j > 0) {
		memset(szBuf, 0, sizeof(szBuf));		// clear buffer
		nRet = recv(msock.rsock,      			// Connected client
				szBuf,							// Receive buffer
				sizeof(szBuf),					// Lenght of buffer
				0);								// Flags

		j = nRet;
		if (nRet == INVALID_SOCKET)	{
            // 10 Retries before error and disconnecting!
			for (int rtry = 0; ; rtry++) {
                nsleep(1);
                nRet = recv(msock.rsock,      		// Connected client
						szBuf,						// Receive buffer
						sizeof(szBuf),				// Lenght of buffer
						0);							// Flags

				j = nRet;
				if (rtry == 10) {
					if (finfo.size == totbyte) { j = 0; break; } // Exit Casue File is Finished
					#ifdef _WIN32
                    sprintf(message, "Lost Connection - %i",WSAGetLastError());
                    #else
                    sprintf(message, "Lost Connection - %i",errno);
                    #endif
					logntp(message);
					strcpy(finfo.status,message);
					memcpy(&finfo2,&finfo,sizeof(FILEINFO));
					fclose(nfp);
					closesocks();
					erbreak = true;
					return;
 				}
				else if (nRet == INVALID_SOCKET) continue;
				else if (nRet == LOST_SOCKET) {
                    if (finfo.size == totbyte) { j = 0; break; } // Exit Casue File is Finished
			        #ifdef _WIN32
                    sprintf(message, "Lost Connection - %i",WSAGetLastError());
                    #else
                    sprintf(message, "Lost Connection - %i",errno);
                    #endif
        			logntp(message);
		          	strcpy(finfo.status,message);
        			memcpy(&finfo2,&finfo,sizeof(FILEINFO));
        			fclose(nfp);
		          	closesocks();
        			erbreak = true;
		          	return;
                }
				else break;
			}
		}		
		else if (nRet == LOST_SOCKET)	{
			if (finfo.size == totbyte) { j = 0; break; } // Exit Casue File is Finished
			#ifdef _WIN32
            sprintf(message, "LOST_SOCKET : Lost Connection - %i",WSAGetLastError());
            #else
            sprintf(message, "LOST_SOCKET : Lost Connection - %i",errno);
            #endif
			logntp(message);
			strcpy(finfo.status,message);
			memcpy(&finfo2,&finfo,sizeof(FILEINFO));
			fclose(nfp);
			closesocks();
			erbreak = true;
			return;
		}
	
		// receive data from client and save to file
		i = 0;
		while (nRet > i) {
            // End of Transfer, Break Loop
		    if (totbyte == finfo.size) break;
			c=szBuf[i];                
   			putc(c, nfp);   			
			i++;
			totbyte++;
			finfo.bRecv = totbyte;
		}
		// End of Transfer, Break Loop
		if (totbyte == finfo.size) { j = 0; break; }
	}
	
    erbreak = true;
	fclose(nfp);
	closesocks();
	nsleep(1);
	
	// End of File Transfer is Reached Here if Successful!
	char fRecv[50]={0};              // Bytes Received [Buffer]
    char fLeft[50]={0};              // Bytes Left     [Buffer]    
    long lftbyte  =0;
	
	// Draw GUI File Bytes Received / Left Information
    sprintf(fRecv,"%.f bytes",(float)checksize());  // Display True Total   	
    lftbyte = (finfo.size - checksize());			// Should always be 0!
    if (lftbyte < 0) { lftbyte = 0; }               // just in case
  	sprintf(fLeft,"%.f bytes",(float)lftbyte);      // Display True Left 	

    // Draw Transfer Status
    sprintf(message,"Successful!");
    strcpy(finfo.status,message);
    logntp(message);
    
    #ifdef _WIN32
    drawpercent(100);
    percenttop(100);
   	drawreceived(fRecv,9,7);
    drawleft(fLeft,9,7);
    drawstatus(message,15,7);
    #else
    #ifdef OS2
    system("cls");
    #else
    system("clear");
    #endif
    printf("\n.----------------------------------------------------------------.");
    printf("\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |");
    printf("\n`----------------------------------------------------------------'\n");
    printf("\nIP: %s\n\nFilename   : %s\nFilesize   : %.f\n",inet_ntoa(their_addr.sin_addr),finfo.filename,(float)finfo.size);
    printf("\nBatch Queue: %s\n",finfo.bqueue);
    
    printf("\nBytes Recv : %i", checksize());
    printf("\nBytes Left : 0\n");
    
    printf("\nSpeed      : 0\n");
    
    printf("\nTime Left  : 0");    
    printf("\n> Elapsed  : %s\n",finfo.telapsed);
    
    printf("\nComplete   : 100%%");
    printf("\nStatus     : %s\n",message);    
    #endif
        
    // Copy File Stats to finfo2 for Message Window
    memcpy(&finfo2,&finfo,sizeof(FILEINFO));
	nsleep(2);
}

// Receive File Info / NTP Version
bool recvfinfo() {

 	char message[255]  ={0};
 	char tMsg[255]     ={0};
	char szBuf[2048]   ={0};
	int nRet;

 	// Get Information Packet From Client
	char tmpin[1000]={0};
	std::string msgEop;
	int id;
	
	logntp("Receiving File Info.. .");
		
	// Loop through RECV() Untll We Get all of the Packet
	for (;;) {
        memset(szBuf, 0, sizeof(szBuf));  // clear buffer
    	nRet = recv(msock.rsock, szBuf, sizeof(szBuf), 0);
		if (nRet == INVALID_SOCKET)	{
            for (int retry = 0; ;retry++) {
                nsleep(1);
                nRet = recv(msock.rsock, szBuf, sizeof(szBuf), 0);
                if (retry == 10) {
           	        #ifdef _WIN32
                    sprintf(message, "INVALID_SOCKET : %i",WSAGetLastError());
                    #else
                    sprintf(message, "INVALID_SOCKET : %i",errno);
                    #endif
                    logntp(message);
        	        strcpy(finfo.status,message);
        		    memcpy(&finfo2,&finfo,sizeof(FILEINFO));
        		    return false;
                }
                else if (nRet == INVALID_SOCKET) continue;
                else if (nRet == LOST_SOCKET) {
                    #ifdef _WIN32
                    sprintf(message, "LOST_SOCKET : %i",WSAGetLastError());
                    #else
                    sprintf(message, "LOST_SOCKET : %i",errno);
                    #endif
                    logntp(message);
    	   		  	strcpy(finfo.status,message);
                  	memcpy(&finfo2,&finfo,sizeof(FILEINFO));
                  	closesocks();
	       		    return false;
                }
                else break;
            }
 		}
        else if (nRet == LOST_SOCKET) {
        	memset(&finfo2,0,sizeof(FILEINFO));
			#ifdef _WIN32
            sprintf(message, "LOST_SOCKET : %i",WSAGetLastError());
            #else
            sprintf(message, "LOST_SOCKET : %i",errno);
            #endif
			logntp(message);
			strcpy(finfo2.status,message);
			closesocks();
			return false;
		}

		// Check for End of Packet String!
	    strcat(tmpin,szBuf);
   		msgEop = tmpin;
   		id = 0;
      	id = msgEop.find("\r\n\r\n", 0);
       	// Received End of Packet
       	if (id != -1) break;

	} // End of For Recv Loop

	// After Receiving Full Packet, Chop it up so we can Sort the Information
    char tmppak[255]={0};
   	int  pamcnt = 0, ab = 0;
   	int  num = 0;

   	char arg[255]={0};
   	memset(&finfo,0,sizeof(FILEINFO)); // 0 out File Struct

	// Cut Up Recevied Packet and Get the Following Information
    for (int i = 0; ;i++) {
		if (tmpin[i] == '\0') break;
		if (tmpin[i] == '\n') {
        	++pamcnt;
        	// Check for BBS Version String
        	if (pamcnt == 1) {
                strcpy(finfo.bbsver,tmppak);   memset(tmppak,0,sizeof(tmppak)); ab = 0;
       			sprintf(message,"BBS Version: %s",finfo.bbsver);
       			logntp(message);

    			// If Not Correction Version, Exit!
   				if(strcmp(finfo.bbsver,"V1.10B") != 0) {
   					memset(&finfo2,0,sizeof(FILEINFO));
					sprintf(message,"Error! Remote is using NTP %s!",finfo.bbsver);
					logntp(message);
					strcpy(finfo2.status,message);
					closesocks();
					return false;
    			}
       		}
       		// Check for Filename String
        	else if (pamcnt == 2) { ab = 0;
                strcpy(arg,tmppak); memset(tmppak,0,sizeof(tmppak));
        	
				// Get True Filename and cute off Path if found!!
  				for (int i = 0; ;i++) {           // Count for Romoval of ExeName from Path
					if (arg[i] == '\0') break;    // End of String, Break
                    #ifdef _WIN32
					if (arg[i] == '\\') num = i;  // Find last or only '\' in String
                    #else
                    if (arg[i] == '/') num = i;   // Find last or only '/' in String
                    #endif
				}
  				if (num == 0) { strcpy(finfo.filename,arg); }
   				else {
        			int r = 0;
		   			for (int i = num+1; ;i++) {   // Copy all Chars after last '\'
    	        		if (arg[i] == '\0') break;
			   			 finfo.filename[r] = arg[i];
		   				 r++;
 					}
	    		}
   				sprintf(message,"Filename: %s",finfo.filename);
   				logntp(message);
       		}
           	// Check for Filesize String
        	else if (pamcnt == 3) {
        		memset(arg,0,sizeof(arg));        		
				strcpy(arg,tmppak);
         		finfo.size = atol(arg); memset(tmppak,0,sizeof(tmppak)); ab = 0;
         		if (finfo.size == 0) {
                    sprintf(message,"Filesize: %i",finfo.size);
    				logntp(message);
    				sprintf(message,"Error: Receiving empty file!");
    				logntp(message);
    				strcpy(finfo2.status,message);
					closesocks();
    				return false;
         		}
         		else {
				    sprintf(message,"Filesize: %i",finfo.size);
    				logntp(message);
                }
            }
            // Check for Batch Queue String
        	else if (pamcnt == 4) {
         		strcpy(finfo.bqueue,tmppak); memset(tmppak,0,sizeof(tmppak)); ab = 0;
				sprintf(message,"Queue: %s",finfo.bqueue);
				logntp(message);
            	break;
            }
     	}
     	else {
  			tmppak[ab] = tmpin[i];
  			ab++;
 		}
	}
	return true;
}

// File Transfer Startup
void transfersetup() {

    char message[255];
    // Get File Info
	if ( !recvfinfo() ) {
 	    sprintf(message,"Error Getting FileInfo.. .");
	    logntp(message);
		return;
    }

	// Check for Resume
	int i = resume();
	if (i == 2) { // Error
	    sprintf(message,"Error Getting Resume Info.. .");
		logntp(message);
		return;
    }
	else if(i == 0) { resum = true; }
	else { resum = false; }

	// Set Error Break to False
	erbreak = false;

    // Start Receiving File Data
    tstate = true;   // Transfer State
    recvfdata();
    tstate = false;
}

// Setup Listen Server & Message Window
void listensrv(SOCKET listenSocket, short nPort) {

    SOCKET	remoteSocket;
	int sin_size;
	char mInit[50];
	char message[255];

    // Tests for End of Transfers Exit
	std::string que;
    int id1, id2;
    char Num[5]={0};

    while (1) {
      
        // Show the server name and port number        
        sprintf(mInit,"[NTP v1.10.1b] Initalized On Port %d ", nPort);
        logntp("\n***********************************************************************************\n");
        logntp(mInit);
        
        // Draw Message Window
        #ifdef _WIN32
	    ansiparse(aBuf1);
        drawinit("+ NTP v1.10.1 beta Initalized!",0,7);
        gotoxy(54,5);
        setcolor(7,0); printf("using port "); setcolor(15,0); printf("%i",nPort);
        
        // Display F1 Exit After Transfer
        if (F1) {
            gotoxy(40,20); setcolor(7,1); printf("*");
        }
        else {
            gotoxy(40,20); setcolor(7,1); printf(" ");
        }
        #else
        system("clear");
        printf("\n.----------------------------------------------------------------.");
        printf("\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |");
        printf("\n`----------------------------------------------------------------'\n");
        printf("\n%s\n",mInit);
        #endif

        if (strcmp(finfo2.status,"") != 0) {
        	// Show Connection Established
            #ifdef _WIN32
            sprintf(mInit,"+ NTP v1.10.1 beta Last File Transfer Stats.. . ");
        	drawinit3(mInit,0,7);
        	sprintf(mInit,"Filename : %s",finfo2.filename);
        	drawinit4(mInit,1,7);
        	sprintf(mInit,"   Queue : %s",finfo2.bqueue);
        	drawinit5(mInit,1,7);
        	sprintf(mInit,"  Status : %s",finfo2.status);
        	drawinit6(mInit,1,7);
            #else
            printf("\n\n[NTP v1.10.1b] Last File Transfer Stats.. . ");
            printf("\nFilename : %s",finfo2.filename);
            printf("\n   Queue : %s",finfo2.bqueue);
            printf("\n  Status : %s\n\n",finfo2.status);
            #endif
            
            // Do test here.. if F1 && bqueue = # of # success, or Error, then exit!
            if (F1) {                
                // If Success, Check if Last file in Queue is Reached
                if (strcmp(finfo2.status,"Successful!") == 0) {
                    //strcmp(finfo2.status,"Error: You Already Have This Full File!") == 0) {
                    que = finfo2.bqueue;
                    id1 = que.find("of",0);
                    if (id1 != -1) {
                        // Get 1st number of Queue
                        for (int i = 0; i != id1-1; i++) {
                            Num[i] = que[i];
                        }
                        // If first # Match's Last, Exit Program!
                        id2 = que.find(Num,id1);
                        if (id2 != -1) {
                            logntp("End of Transfers, Program Exiting.. .");
                            e1 = true;
                            exitprogram();
                        }
                    }
                }
                // If Error, Exit!
                else {
                    logntp("End of Transfers w/ Error!, Program Exited.. .");
                    e1 = true;
                    exitprogram();
                }
            }
        }

        // Show Connection Established
        sprintf(mInit,"Waiting for Connection.. . ");
        logntp(mInit);
        #ifdef _WIN32
        drawinit2(mInit,1,7);
        #else
        printf("\n%s\n",mInit);
        #endif

	    // Clears the struct to recevie information from the user
      	sin_size = sizeof(struct sockaddr_in);
        memset(&their_addr,0,sizeof(their_addr));

        #ifdef _WIN32
    	remoteSocket = accept(listenSocket, (struct sockaddr *)&their_addr, &sin_size);
        #else
        remoteSocket = accept(listenSocket, (struct sockaddr *)&their_addr, (socklen_t *)&sin_size);
        #endif

        if (e1) return;
        else if (remoteSocket == INVALID_SOCKET)	{
	       	memset(&finfo2,0,sizeof(FILEINFO));
    		sprintf(message,"Error: accept() - Incomming Connection!");
	   	    logntp(message);
	       	strcpy(finfo2.status,message);
		    #ifdef _WIN32
		    closesocket(listenSocket);
		    #else
		    close(listenSocket);
		    #endif
		    exitprogram();
	    }

	    // Fill Socket Struct on Completed Connection
	    msock.rsock = remoteSocket;
	    msock.lsock = listenSocket;
	
        // Show Connection Established
        sprintf(mInit,"Connection Established: %s",inet_ntoa(their_addr.sin_addr));
        logntp(mInit);
        #ifdef _WIN32    
        drawinit2(mInit,15,7);
        #else    
        printf("\n%s\n",mInit);
        #endif
        nsleep(2);
    
        #ifdef _WIN32
        // Date/Time Stamp log on Each Conection
        GetTimeFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTimeFormat, 50 );
	    GetDateFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szDateFormat, 50 );
	    sprintf(mInit,"Date/Time: %s %s",szDateFormat,szTimeFormat);
        logntp(mInit);
        #endif
        
        // Start File Transfer init
        transfersetup();
    }
}

// Socket Init and Startup
void StreamServer(short nPort) {

    // Startup Header
    #ifndef _WIN32
	system("clear");
	printf("\n.----------------------------------------------------------------.");
    printf("\n| NTP v1.10.1b 02/18/04 (c) Mercyful Fate 2k3/2k4                |");
    printf("\n`----------------------------------------------------------------'\n");
    #endif

    char message[255]; // Error Messages
	// Create a TCP/IP stream socket to "listen" with
	SOCKET	listenSocket;

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)	{
		memset(&finfo2,0,sizeof(FILEINFO));
		sprintf(message,"Error: Socket Init!");
		logntp(message);
		strcpy(finfo2.status,message);
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
	saServer.sin_port = htons(nPort);		// Use port from command line
    #ifndef _WIN32
    memset(&(saServer.sin_zero), '\0', 8);
    #endif

    char szBuf[2048]={0};
	int nRet;

    #ifdef _WIN32
	// bind the name to the socket
	nRet = bind(listenSocket,				// Socket
				(LPSOCKADDR)&saServer,		// Our address
				sizeof(struct sockaddr));	// Size of address structure
    #else
    // bind the name to the socket
    nRet = bind(listenSocket,
                (struct sockaddr *)&saServer,
		        sizeof(saServer));
    #endif

	if (nRet == INVALID_SOCKET) {
		memset(&finfo2,0,sizeof(FILEINFO));
		sprintf(message,"Error: bind() - Unable to use selected port! Try another.. .");
		logntp(message);	
		#ifdef _WIN32
		setcolor(7,0);
		printf("\n%s",message);
		closesocket(listenSocket);
		#else
		printf("\n%s",message);
		printf("\nif you are not root, then try a port over 1024!\n\n",message);
		close(listenSocket);
		#endif
    	nsleep(5);
		exitprogram();
	}

  	// Set the socket to listen
	nRet = listen(listenSocket, 10);
	if (nRet == INVALID_SOCKET) {
		memset(&finfo2,0,sizeof(FILEINFO));
		sprintf(message,"Error: listen() - For Connection!");
		logntp(message);
		strcpy(finfo2.status,message);
		#ifdef _WIN32
		closesocket(listenSocket);
		#else
		close(listenSocket);
		#endif	
		exitprogram();
	}
	
	// Listen Server Setup
    listensrv(listenSocket, nPort);
}

// Input Thread, Handles Exit Key
// Win32 only at the moment
#ifdef _WIN32
void input( void *p ) {

    #ifdef _WIN32
	setcolor(7,7);
	#endif
	int c;
	while (1) {
        c = getch();
        // Handle ESC Key
		if (c == 27) {
            e1 = true;
			logntp("Program Exited by User.. .");
			#ifdef _WIN32
			setcolor(7,0);
			#endif
			exitprogram();
		}
		// Handle F1 Key
		if (c == 59) {
            if (F1) {
                F1 = false;
                if (tstate) { gotoxy(40,17); setcolor(7,1); printf(" "); }
                else        { gotoxy(40,20); setcolor(7,1); printf(" "); }
            }
            else {
                F1 = true;
                if (tstate) { gotoxy(40,17); setcolor(7,1); printf("*"); }
                else        { gotoxy(40,20); setcolor(7,1); printf("*"); }
            }
		}
	}
}
#endif

// Main Program Entrance
int main(int argc, char **argv) {

    #ifdef _WIN32
	SetConsoleTitle("Nemesis Transfer Protocol [USER v1.10.1 beta]");
	#endif
	
	char newport[8];
	short nport = 0;

	// Check for port argument
	if (argc != 2)	{
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
        printf(".----------------------------------------------------------------------.\n");
        printf("| NTP v1.10.1 beta 02/18/2004 (c) Mercyful Fate 2k3/2k4                |\n");
        printf("| Program init phase.. .                                               |\n");
        printf("`----------------------------------------------------------------------'\n\n");
    
        printf(" No port specified in the command line.. .");
        printf("\n\n Please enter a port # to start NTP with -> ");
        gets(newport);
        nport = atoi(newport);
	}
		
	#ifdef _WIN32
    // Turn Windows Cursor Off for Smoother Ansimation
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO CCI;
    GetConsoleCursorInfo(hOut, &CCI);
    CCI.bVisible = false;
    SetConsoleCursorInfo(hOut, &CCI);        
	#endif
	
	// Setup Global Path in Structs.h
    char parg[255];
  	strcpy(parg,argv[0]);
	int num = 0;

    // Get FULL PATH TO EXE, and Chop off Filename for PATH
	#ifdef _WIN32	
	memset(&PATH,0,255);
	for (int i = 0; ;i++) {
		if (parg[i] == '\0') break;
		if (parg[i] == '\\') num = i;
	}
	if (num != 0) {
    	for (int i = 0; i < num+1 ; i++) {
	   	   PATH[i] = parg[i];
    	}
    	SetCurrentDirectory(PATH);
	}
	else memset(&PATH,0,sizeof(PATH));
    #else
	memset(&PATH,0,255);
	for (int i = 0; ;i++) {
		if (parg[i] == '\0') break;
		if (parg[i] == '/') num = i;
	}
	if (num != 0) {
	    for (int i = 0; i < num+1 ; i++) {
		    PATH[i] = parg[i];
    	}
	}
	else memset(&PATH,0,sizeof(PATH));
    #endif	
	
	// Do Error Checking if CONFIG.CFG File exists, if not creates it
	if(configdataexists() == false) createconfig(); // Creates Config with Default Settings

	// Open and Read Config file
	parseconfig();	

	// get port #
	short iPort;
    if (nport != 0) iPort = nport;
    else iPort = atoi(argv[1]);
    
    // Clear Info 2 Error Message Structure
	memset(&finfo2,0,sizeof(FILEINFO));

    // Start Console Input Thread
    #ifdef _WIN32
    HANDLE ahThread[1];
    ahThread[0] = (HANDLE)_beginthread( input, 0, (void*)&iPort );
    #else
    /* Disabled Linux Input for Now
	pthread_t thread;
    pthread_create(&thread, NULL, input, (void*)iPort);
    */
    #endif
    
    // Start Win32 Winsock Initilization
    #ifdef _WIN32
    char message[30];
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;

	WSAStartup(wVersionRequested, &wsaData);
	if (wsaData.wVersion != wVersionRequested)	{
		memset(&finfo2,0,sizeof(FILEINFO));
		sprintf(message,"Error: Winsock 1.1 Unsupported!\n");
		logntp(message);
		strcpy(finfo2.status,message);
		exitprogram();
	}
    #endif

    // Startup Server and listen for connections
    StreamServer(iPort);
    
    // Listen Server Setup
    //listensrv(listenSocket, nPort);
}


#ifndef __GLOBAL_H    /*  An extra safeguard to prevent this header from  */
#define __GLOBAL_H    /*  being included twice in the same source file    */

#include <winsock.h>

#ifndef _WIN32
typedef int SOCKET;
typedef sockaddr_in SOCKADDR_IN;

#define INVALID_SOCKET -1
#define SOCKET_ERROR    0
#define WSAEWOULDBLOCK 35
#endif

// BBS Version
#define VERSION "NTP v2.0a44+ Console | v1.10b compatiable"
// Protocol Version
#define BBSVER  "V1.10B"

typedef struct
{
    char     bbsver[10];           // Client Version
    char     truename[255];        // True Filename
    char     filename[255];        // Filename
    char     bqueue[255];          // Batch Queue 1/4 etc..
    long int size;                 // File Size in Bytes
    long int bRecv;                // Bytes Received
    long int lRecv;                // Last Bytes Received
    long int bSec;                 // Bytes Received Each Second
    long int speed;                // Transfer Speed
    char     status[50];           // Transfer Status
    bool     InUse;                // Node is in Use
    bool     resum;                // Resume True/ flase
    long     flsz;    	           // Local Filesize for Resume

    struct sockaddr_in their_addr; // Hold Remote Hosts IP Address

} FILEINFO;

// 0 for Downlaods 1 for Uploads
extern FILEINFO finfo  [2][6];    // Handle to File Info
extern FILEINFO finfo2 [2][6];    // Handle to File Info After Tranfer, Message Window
extern bool     erbreak[2][6];    // break out of transfer gui
extern short    PORT;

enum NODESTATE
{
    NodeWindow, // = 0
    DLNode1,    // = 1
    DLNode2,    // = 2
    DLNode3,    // = 3
    DLNode4,    // = 4
    DLNode5,    // = 5
    ULNode1,    // = 6
    ULNode2,    // = 7
    ULNode3,    // = 8
    ULNode4,    // = 9
    ULNode5,    // = 10
    MidState    // = 11
};

enum NWINBAR
{
    SNOOP,      // = 0
    CONNECT,    // = 1
    CONFIG,     // = 2
    RESTART,    // = 3
    EXIT        // = 4
};

extern bool nsworking;

// For Displaying Which Ansi
extern NODESTATE nstate;
// Previous Node State
extern NODESTATE pnstate;
// Which Node the Lightbar is on
extern NODESTATE nbar;
// Which Option Selection in Node Window
extern NWINBAR   nwbar;

// NodeGUI
extern char *aBuf1;
// NodeGUI Message Window Clear
extern char *aBuf11;
// NodeGUI Snoop Message
extern char *aBuf12;
// Transfer GUI
extern char *aBuf2;
// Node Stat Ansi
extern char *aBuf3;
// Batch Add
extern char *aBuf4;


// Prototypes
double percentage ( long double wk, long double pk );

void refreshbar();
void NodeGUI ( int port );

#ifdef _WIN32
void TransferGUI ( void *p );
void input ( void *p );
void InputThreadProc ( void *dummy );
#else
void *TransferGUI ( void *p );
#endif

#endif

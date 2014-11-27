#ifndef __SERVER_H    /*  An extra safeguard to prevent this header from  */
#define __SERVER_H    /*  being included twice in the same source file    */

#include <winsock.h>

#ifdef _WIN32
void DLNodeTransfer ( void *p );
#else
void *DLNodeTransfer ( void *p );
#endif

void StreamServer ( short nPort, SOCKET l1 );
void StartServer ( short nPort );

class NTPSRV
{

public:

    // Socket Structure
    typedef struct
    {
        SOCKET rsock;
        SOCKET lsock;
    } MSOCK;

    MSOCK msock;                // Handle to Sockets

    int  NODE;                  // Node For FileInfo..

    char szDateFormat[128];		// System Date
    char szTimeFormat[128];		// System Time

    void logntp ( char *text, int NODE ); // Logs Errors / Connections
    void closesocks();          // Takes Care of Closing Sockets
    void checksize();           // Checks File Size's for Transfers
    bool recvfinfo();           // Receives File and NTP Info for Host
    int  resume();              // Check for Transfer Recovery
    void recvfdata();           // Receive File Data from Host

};


#endif

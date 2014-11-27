#ifndef __CLIENT_H    /*  An extra safeguard to prevent this header from  */
#define __CLIENT_H    /*  being included twice in the same source file    */

#include <string>
#include <winsock.h>

// NTP Client Class
class NTPCLI
{

public:

    char hostip[255];   // Holds Users IP Address
    bool dszok;         // Transfer Success/Failure
    bool batcht;        // Batch or Single Transfer
    int  NODE;          // Node For FileInfo..

    //NTPCLI::NTPCLI();
    void StreamClient ( short nPort );
    void setupbatch ( char *szFile, short nPort );

private:
    // Link List for Holding all Batch Queue Files
    struct ListItem
    {
        std::string str;
        struct ListItem *next;
    };

    struct ListItem *MyList1[6];         // handle to Batch queue
    void dodszlog();                     // dszlog loggin
    void clearbatchdata();               // Clears Queue
    void addbatchdata ( std::string str );    // Add to Queue
    long resume ( SOCKET sock );         // Get File Resume bytes
    void sendfdata ( SOCKET sock );      // Send File Data
    void sendfinfo ( SOCKET sock );      // Send File Info

};

/*
NTPCLI::NTPCLI() {

    memset(&finfo,0,sizeof(FILEINFO));
    MyList1 = 0;
    memset(hostip,0,sizeof(hostip));
    dszok  = false;  // Defaukt to Error untill a Success is Reached
    batcht = false;  // Default to Single Transfer
}
*/

#endif

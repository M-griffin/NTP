
// Client.cpp { NTP 1.8b+ Beta Test Rebuild } (c) 2003 Michael Griffin
// 09-20-03 + Windows and Linux Ports - Completed!
// 09-30-03 + OS/2 Port Completed
// 			- Also fixed Filesize Functions and Read pointer on Resume!
// 10-25-03 + Added Full Info & Data 32 Bit Encryption
// 11-10-03 + Removing Encryption to bring back 1.7b Source Code..
//          - Adding Full DSZLOG Support

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>

#include "global.h"
#include "client.h"

// Win32
#ifdef _WIN32
#include <time.h>
#include <sys/timeb.h>
#include <winsock.h>
#include <windows.h>

// Linux + OS/2
#elif OS2
#include <io.h>
#else
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#endif


void NTPCLI::dodszlog()
{

    // Setup DSZLOG Output for Transfer Status
    char *dszlog;
    dszlog = getenv ( "DSZLOG" );
    // If No Value Use Temp
    if ( dszlog==NULL )
    {
        dszlog="dsztemp.log";
    }

    // If Batch Append, otherwise overwrite dszlog
    FILE *fp1;
    if ( batcht )
    {
        if ( ( fp1 = fopen ( dszlog, "a" ) ) == NULL )
        {
            printf ( "\nFatal! Can't open dszlog - %s!\n", dszlog );
            return;
        }
    }
    else
    {
        if ( ( fp1 = fopen ( dszlog, "w" ) ) == NULL )
        {
            printf ( "\nFatal! Can't open dszlog - %s!\n", dszlog );
            return;
        }
    }

    // Write out Sent ok / Error
    if ( dszok )
        fprintf ( fp1, "z      0 57600 bps 9999 cps   0 errors     0   32 %s -1\n", finfo[1][NODE].filename );
    else
        fprintf ( fp1, "E      0 57600 bps 9999 cps   1 errors     0   32 %s -1\n", finfo[1][NODE].filename );
    fclose ( fp1 );
}

void NTPCLI::clearbatchdata()
{

    struct ListItem *p,*next;
    if ( MyList1[NODE] == 0 ) return;

    for ( p = MyList1[NODE]; p != 0; p = next )
    {
        next = p->next;
        delete ( p );
    }
    MyList1[NODE] = 0;
    return;
}

void NTPCLI::addbatchdata ( std::string str )
{

    std::string temp = str;
    if ( temp == "" ) return;

    struct ListItem *last;
    ListItem *item = new ListItem;
    if ( !item )
    {
        printf ( "\nMalloc Failed, Batch Queue\n" );
        return;
    }

    item->str = temp;
    item->next = 0;

    if ( MyList1[NODE] == 0 )
    {
        MyList1[NODE] = item;
        return;
    }
    for ( last = MyList1[NODE] ; last->next != 0 ; last = last->next );
    last->next = item;
}

long NTPCLI::resume ( SOCKET sock )
{

    char szBuf[2048]= {0};
    char tmpin[1000]= {0};
    std::string msgEop;
    int id;
    int nRet;

    // Loop throguh RECV for Resume Packet
    for ( ;; )
    {
        memset ( szBuf, 0, sizeof ( szBuf ) ); // clear buffer

        // Now Get Reply back, if File is New or Recovery
        nRet = recv ( sock, szBuf, sizeof ( szBuf ), 0 );
        if ( nRet == INVALID_SOCKET )
        {
            //Draw Transfer Status
            printf ( "\nError! Recv() - Resume Status, Retrying.. . \n" );
            return ( -1 );
        }
        else if ( nRet == 0 )
        {
            //Draw Transfer Status
            printf ( "\nError! Disconnected, Exiting.. . \n" );
#ifdef _WIN32
            closesocket ( sock );
#else
            close ( sock );
#endif
            return ( -1 );
        }
        strcat ( tmpin,szBuf );
        msgEop = tmpin;
        id = 0;
        id = msgEop.find ( "\r\n\r\n", 0 );
        if ( id != -1 ) break;
    }

    // After Receiving Full Packet, Chop it up so we can Sort the Information
    char tmppak[255]= {0};
    char arg[255]= {0};
    int pamcnt, ab;
    pamcnt = 0;
    ab = 0;
    int num = 0;

    int c, i;
    long resbytes = 0;
    long totbyte=0;

    // Cut Up Recevied Packet and Get the Following Information
    for ( int i = 0; ; i++ )
    {
        if ( tmpin[i] == '\0' ) break;
        if ( ( tmpin[i] == '\r' ) && ( tmpin[i+1] == '\n' ) )
        {
            ++pamcnt;
            // Check for Resume Size String
            if ( pamcnt == 1 )
            {
                strcpy ( szBuf,tmppak );
                memset ( tmppak,0,255 );
                ab = 0;
                resbytes = atol ( szBuf );

                // If Users File is larger then File being Sent.. Error!
                if ( resbytes > finfo[1][NODE].size )
                {
                    printf ( "\nError! - Incorrect File Already Exists at Users End!," );
                    printf ( "\n       - File User has is Larger then BBS is Sending, Aborting.. .\n" );
#ifdef _WIN32
                    closesocket ( sock );
                    Sleep ( 2000 );
#else
                    close ( sock );
                    sleep ( 2 );
#endif
                    return ( -1 );
                }

                // If 0 Resume Bytes, then File Already Exists in it's Entiriety!
                if ( resbytes == 0 )
                {
                    printf ( "\nStarting Transfer Initilization.. .\n" );
                    return ( 0 );
                }

                // If 0 Resume Bytes, then File Already Exists in it's Entiriety!
                if ( resbytes == finfo[1][NODE].size )
                {
                    printf ( "\nError! Full File Already Exists at Users End!, Aborting.. .\n" );
#ifdef _WIN32
                    closesocket ( sock );
                    Sleep ( 2000 );
#else
                    close ( sock );
                    sleep ( 2 );
#endif
                    return ( -1 );
                }
                printf ( "\nNTP: Resuming at byte %i\n",resbytes );
                return ( resbytes ); // bytes to resume at
                break;
            }
            else break;
        } // (tmpin[i] == '\n')
        else
        {
            tmppak[ab] = tmpin[i];
            ab++;
        }
    }
}

void NTPCLI::sendfdata ( SOCKET sock )
{

    long totbyte=0;
    char szBuf[2048];
    int c, i;

    // Open File to Begin File Transfer
    FILE *fp;
    if ( ( fp = fopen ( finfo[1][NODE].filename, "rb" ) ) ==  NULL )
    {
        printf ( "\nError: Can't open file: %s\n", finfo[1][NODE].filename );
#ifdef _WIN32
        closesocket ( sock );
#else
        close ( sock );
#endif
        dodszlog();
        exit ( 1 );
    }

    // Check on Transfer Recovery or New File
    long resbyte;
    resbyte = resume ( sock );
    if ( resbyte == -1 )
    {
        printf ( "\nError on Resume!\n" );
#ifdef _WIN32
        closesocket ( sock );
#else
        close ( sock );
#endif
        fclose ( fp );
        dodszlog();
        return;
    }
    if ( resbyte > 0 )
    {
        if ( fseek ( fp,resbyte,SEEK_SET ) == 0 )
        {
            totbyte = resbyte;
        }
        else
        {
            // Backup
            for ( int res = 0; res != resbyte; res++ )
            {
                c=getc ( fp );
            }
            totbyte = resbyte;
        }
    }

    // Start File Transfer
    printf ( "\nOpening Binary Data Connection . ..\n" );
    int nRet;
    do
    {
        i=0;
        while ( ( i<2000 ) && ( ( c=getc ( fp ) ) != EOF )  )
        {

            szBuf[i] = c;
            i++;
            totbyte++;
        }

        nRet = send ( sock,              // Connected socket
                      szBuf,                // Data buffer
                      i,                // Length of data
                      0 );              // Flags

        if ( nRet == SOCKET_ERROR )
        {
            for ( int rtry = 0; ; rtry++ )
            {
                // Retry 6 Times before erroring out, transfer recovery!
                if ( rtry == 6 )
                {
                    printf ( "\nsend() - 6 retries!\n" );
#ifdef _WIN32
                    closesocket ( sock );
#else
                    close ( sock );
#endif
                    fclose ( fp );
                    dodszlog();
                    exit ( 1 );
                }

                nRet = send ( sock,      // Connected socket
                              szBuf,        // Data buffer
                              i,        // Length of data
                              0 );      // Flags

                if ( nRet == SOCKET_ERROR ) continue;
                else if ( nRet == 0 )
                {
                    printf ( "\nsend()\n" );
#ifdef _WIN32
                    closesocket ( sock );
#else
                    close ( sock );
#endif
                    fclose ( fp );
                    dodszlog();
                    exit ( 1 );
                }
                else break;
            }
        }
        if ( nRet == 0 )
        {
            printf ( "\nsend()\n" );
#ifdef _WIN32
            closesocket ( sock );
#else
            close ( sock );
#endif
            fclose ( fp );
            dodszlog();
            exit ( 1 );
        }
        printf ( "." );
    }
    while ( c != EOF );

    printf ( "\nTotal Transfered Message Size: %.0f bytes\nBye!\n", ( float ) totbyte );
    dszok=true;
    dodszlog();
#ifdef _WIN32
    closesocket ( sock );
#else
    close ( sock );
#endif
    fclose ( fp );
}

void NTPCLI::sendfinfo ( SOCKET sock )
{

    // Display Inital File Information
    printf ( "\nFilename: %s\n",finfo[1][NODE].truename );
    printf ( "Filesize: %i\n\n",finfo[1][NODE].size );

    char szTmp0[2048]= {0};
    sprintf ( szTmp0,"V1.7B\n%s\n%i\n%s\n\r\n\r\n",
              finfo[1][NODE].truename,finfo[1][NODE].size,finfo[1][NODE].bqueue );

    int nRet;
    for ( ;; )
    {
        nRet = send ( sock, szTmp0, sizeof ( szTmp0 ), 0 );
        if ( nRet == SOCKET_ERROR )
        {
            printf ( "\nError: send() - Client Information! - Retrying\n" );
        }
        else if ( nRet == 0 )
        {
            printf ( "\nError: send() - Lost Connection! - Exiting.. .\n" );
#ifdef _WIN32
            closesocket ( sock );
#else
            close ( sock );
#endif
            dodszlog();
            exit ( 1 );
        }
        else break;
    }
}

void NTPCLI::StreamClient ( short nPort )
{

#ifdef _WIN32
    // Initalize Winsock - Windows only
    WORD wVersionRequested = MAKEWORD ( 1,1 );
    WSADATA wsaData;

    // Initialize WinSock and check the version
    int nRet = WSAStartup ( wVersionRequested, &wsaData );
    if ( wsaData.wVersion != wVersionRequested )
    {
        fprintf ( stderr,"\nWinsock Wrong Version\n" );
        WSACleanup();
        dodszlog();
        exit ( 1 );
    }
#endif

    printf ( "\nNTP Connecting To User: %s On Port: %d", hostip, nPort );

#ifdef _WIN32 // Windows WinSocket INIT
    SOCKET  theSocket;
    LPHOSTENT lpHostEntry;

    lpHostEntry = gethostbyname ( hostip );
    if ( lpHostEntry == NULL )
    {
        printf ( "\ngethostbyname()\n" );
        WSACleanup();
        dodszlog();
        exit ( 1 );
    }

    theSocket = socket ( AF_INET,         // Address family
                         SOCK_STREAM,           // Socket type
                         IPPROTO_TCP );         // Protocol
    if ( theSocket == INVALID_SOCKET )
    {
        printf ( "\nsocket()\n" );
        WSACleanup();
        dodszlog();
        exit ( 1 );
    }

    // Fill in the address structure
    SOCKADDR_IN saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr = * ( ( LPIN_ADDR ) *lpHostEntry->h_addr_list );
    // Server's address
    saServer.sin_port = htons ( nPort );   // Port number from command line

    nRet = connect ( theSocket,            // Socket
                     ( LPSOCKADDR ) &saServer, // Server address
                     sizeof ( struct sockaddr ) ); // Length of server address structure

    if ( nRet == SOCKET_ERROR )
    {
        printf ( "\nsocket()\n" );
        closesocket ( theSocket );
        WSACleanup();
        dodszlog();
        exit ( 1 );
    }

#else // Linux / OS2 Sockets INIT
    int theSocket;
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ( ( he=gethostbyname ( hostip ) ) == NULL )
    {
        perror ( "Error: gethostbyname()" );
        printf ( "hostname: %s",hostip );
        dodszlog();
        exit ( 1 );
    }

    if ( ( theSocket = socket ( AF_INET, SOCK_STREAM, 0 ) ) == -1 )
    {
        perror ( "Error: socket() - init error!" );
        dodszlog();
        exit ( 1 );
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons ( nPort );
    their_addr.sin_addr = * ( ( struct in_addr * ) he->h_addr );
    memset ( & ( their_addr.sin_zero ), '\0', 8 ); // zero the rest of the struct

    if ( connect ( theSocket, ( struct sockaddr * ) &their_addr, sizeof ( struct sockaddr ) ) == -1 )
    {
        perror ( "Error connect() - Unable to Connect!" );
        dodszlog();
        exit ( 1 );
    }
#endif

    sendfinfo ( theSocket );

#ifdef _WIN32
    Sleep ( 2000 );
#else
    sleep ( 2 );
#endif

    sendfdata ( theSocket );

#ifdef _WIN32
    WSACleanup();
#endif
    return;
}

void NTPCLI::setupbatch ( char *szFile, short nPort )
{

    char arg[255]= {0};
    std::string str;
    strcpy ( arg,szFile );
    std::ifstream inStream;

    // Open FI.LST and Get All File in Queue
    inStream.open ( arg );
    if ( !inStream.is_open() )
    {
        printf ( "\nCan't open %s For Batch FileTransfer.. .\n", arg );
        return;
    }

    // Loop For Each File in FI.LST until End of File
    int count = 0;
    while ( !inStream.eof() )
    {
        getline ( inStream,str );        // Get Full Batch file Line from File
        addbatchdata ( str );            // Add each batch file into Link List
        count++;                         // Keep Track of Batch Queue
    }
    inStream.close();

    int  current = 0;
    long totbyte = 0;
    char bqueue[255];

    struct ListItem *p;
    for ( p = MyList1[NODE]; p != 0; p = p->next )
    {
        ++current;
        strcpy ( arg,p->str.c_str() );
        memset ( &finfo[1][NODE],0,sizeof ( finfo[1][NODE] ) );
        sprintf ( bqueue,"%i of %i",current,count );
        strcpy ( finfo[1][NODE].bqueue,bqueue );

        // Get True Filename and cute off Path
        int num = 0;
        for ( int i = 0; ; i++ )         // Count for Romoval of ExeName with Path
        {
            if ( arg[i] == '\0' ) break; // End of String, Break
#ifdef _WIN32
            if ( arg[i] == '\\' ) num = i; // Find last or only '\' in String
#elif OS2
            if ( arg[i] == '\\' ) num = i; // Find last or only '\' in String
#else
            if ( arg[i] == '/' ) num = i; // Find last or only '/' in String
#endif
        }
        if ( num == 0 )
        {
            strcpy ( finfo[1][NODE].truename,arg );
        }
        else
        {
            int r = 0;
            for ( int i = num+1; ; i++ ) // Copy all Chars after last '\'
            {
                if ( arg[i] == '\0' ) break;
                finfo[1][NODE].truename[r] = arg[i];
                r++;
            }
        }
        strcpy ( finfo[1][NODE].filename,arg );

#ifdef _WIN32
        // Read File and Get File Size
        WIN32_FIND_DATA wfd;   // File Structure
        HANDLE          h;     // Handle to File
        h = FindFirstFile ( finfo[1][NODE].filename, &wfd );

        if ( h != INVALID_HANDLE_VALUE )
        {
            finfo[1][NODE].size = wfd.nFileSizeLow;
        }
        FindClose ( h );

#else
        // Open File and Read File Size
        FILE *fp;
        if ( ( fp = fopen ( finfo.filename, "rb" ) ) ==  NULL )
        {
            perror ( "Error: Can't Open File For Binary Transfer!\n" );
            exit ( 1 );
        }

#ifdef _WIN32
        totbyte = filelength ( fileno ( fp ) );
#elif OS2
        totbyte = filelength ( fileno ( fp ) );
#else
        fseek ( fp, 0L, SEEK_END ); /* Position to end of file */
        totbyte = ftell ( fp );   /* Get file length */
#endif

        fclose ( fp );
        finfo[1][NODE].size = totbyte;
#endif

        // Set DSZLOG to BATCH Transfer
        if ( current==1 )
            batcht = false;   // If 1st in Batch Start New DSZLOG
        else
            batcht = true;

        dszok = false;        // Reset DSZLOG State to ERROR untill a Success is Reached

        // Start Transfer For Batch File
        StreamClient ( nPort );

#ifdef _WIN32
        Sleep ( 6000 ); // Wait 6 Seconds Before Reconnecting for Next File
#else
        sleep ( 6 );
#endif

    } // For Loop

    // Clear Batch Queue
    clearbatchdata();

}

#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "global.h"
#include "server.h"

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

#endif


// 0 for Downlaods 1 for Uploads
FILEINFO finfo  [2][6];           // Handle to File Info
FILEINFO finfo2 [2][6];           // Handle to File Info After Tranfer, Message Window
bool     erbreak[2][6];           // error break out of gui

int ClientsConnected;             // Keep Track of Connections

// For Displaying Which Ansi
NODESTATE nstate  = NodeWindow;
//  Previous Node State
NODESTATE pnstate = DLNode1;
// Which Node the Lightbar is on
NODESTATE    nbar = DLNode1;
// Which Option Selection in Node Window
NWINBAR     nwbar = SNOOP;


void NTPSRV::logntp ( char *text, int NODE )
{

    FILE *fp1;
    char fname[20];
    sprintf ( fname,"logs\\NTP_DL.log",NODE );
    if ( ( fp1 = fopen ( fname, "a" ) ) == NULL )
    {
        printf ( "Fatal! Couldn't write to log file!\n" );
        return;
    }
    fprintf ( fp1, "* %s\n",text );
    fclose ( fp1 );
}

void NTPSRV::closesocks()
{

#ifdef _WIN32
    closesocket ( msock.rsock );
#else
    close ( msock.rsock );
#endif
}

void NTPSRV::checksize()
{

    long size = 0;

#ifdef _WIN32
    // Read File and Get File Size
    WIN32_FIND_DATA wfd;   // File Structure
    HANDLE          h;     // Handle to File
    h = FindFirstFile ( finfo[0][NODE].filename, &wfd );

    if ( h != INVALID_HANDLE_VALUE )
    {
        size = wfd.nFileSizeLow;
    }
    FindClose ( h );

#else
    // Open File and Read File Size
    FILE *fp;
    if ( ( fp = fopen ( finfo.filename, "rb" ) ) ==  NULL )
    {
        // New File, Can't Open if not there!
        finfo[0][NODE].flsz = 0;
        return;
    }

#ifdef OS2
    size = filelength ( fileno ( fp ) );
#else
    fseek ( fp, 0L, SEEK_END ); /* Position to end of file */
    size = ftell ( fp );        /* Get file length */
#endif

    fclose ( fp );
#endif

    finfo[0][NODE].flsz = size;
}

bool NTPSRV::recvfinfo()
{

    char message[255];
    char tMsg[255]= {0};
    char szBuf[2048];
    int nRet;

    // Get Information Packet From Client
    char tmpin[1000]= {0};
    std::string msgEop;
    int id;

    logntp ( "Receiving File Info.. .", NODE );

    // Loop through RECV() Untll We Get all of the Packet
    for ( ;; )
    {
        memset ( szBuf, 0, sizeof ( szBuf ) ); // clear buffer
        nRet = recv ( msock.rsock, szBuf, sizeof ( szBuf ), 0 );

        if ( nRet == INVALID_SOCKET )
        {
            memset ( &finfo2[0][NODE],0,sizeof ( FILEINFO ) );
            sprintf ( message,"Error: Recv() Getting File Info, Re-Trying.. ." );
            logntp ( message, NODE );
            strcpy ( finfo2[0][NODE].status,message );
        }
        else if ( nRet == SOCKET_ERROR )
        {
#ifdef _WIN32
            if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
            {
#else
            if ( errno != WSAEWOULDBLOCK )
            {
#endif
                memset ( &finfo2[0][NODE],0,sizeof ( FILEINFO ) );
                sprintf ( message,"Error: Getting File Info, Lost Connection.. ." );
                logntp ( message, NODE );
                strcpy ( finfo2[0][NODE].status,message );
                closesocks();
                return false;
            }
        }
        else
            strcat ( tmpin,szBuf );

        // Check for End of String!
        msgEop = tmpin;
        id = 0;
        id = msgEop.find ( "\r\n\r\n", 0 );
        // Received End of Packet
        if ( id != -1 ) break;

    } // End of For Recv Loop

    strcpy ( tmpin,tmpin );

    // After Receiving Full Packet, Chop it up so we can Sort the Information
    char tmppak[255]= {0};
    memset ( tmppak,0,255 );
    int pamcnt, ab;
    pamcnt = 0;
    ab = 0;
    int num = 0;
    std::string cut;
    int id1;

    char arg[255]= {0};

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
                strcpy ( finfo[0][NODE].bbsver,tmppak );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                sprintf ( message,"BBS Version: %s",finfo[0][NODE].bbsver );
                logntp ( message, NODE );

                // If Not Correction Version, Exit!
                if ( strcmp ( finfo[0][NODE].bbsver,BBSVER ) != 0 )
                {
                    memset ( &finfo2[0][NODE],0,sizeof ( FILEINFO ) );
                    sprintf ( message,"Error: Otherside is using NTP %s, Please Update!",finfo[0][NODE].bbsver );
                    logntp ( message, NODE );
                    strcpy ( finfo2[0][NODE].status,message );
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
                    strcpy ( finfo[0][NODE].filename,arg );
                }
                else
                {
                    int r = 0;
                    for ( int i = num+1; ; i++ )  // Copy all Chars after last '\'
                    {
                        if ( arg[i] == '\0' ) break;
                        finfo[0][NODE].filename[r] = arg[i];
                        r++;
                    }
                }
                sprintf ( message,"Filename: %s",finfo[0][NODE].filename );
                logntp ( message, NODE );
            }
            // Check for Filesize String
            else if ( pamcnt == 3 )
            {
                memset ( arg,0,sizeof ( arg ) );
                strcpy ( arg,tmppak );
                finfo[0][NODE].size = atol ( arg );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                sprintf ( message,"Filesize: %i",finfo[0][NODE].size );
                logntp ( message, NODE );
            }
            // Check for Batch Queue String
            else if ( pamcnt == 4 )
            {
                strcpy ( finfo[0][NODE].bqueue,tmppak );
                memset ( tmppak,0,sizeof ( tmppak ) );
                ab = 0;
                sprintf ( message,"Queue: %s",finfo[0][NODE].bqueue );
                logntp ( message, NODE );
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

int NTPSRV::resume()
{

    // Check if we are getting New File or Resuming Old
    long totbyte=0;                    // Total Bytes Received
    char message[255];
    char rezBuf[2048]= {0};
    char tBuf[2048]= {0};
    int nRet;

    // Get File Size to check for Resume or Complete
    checksize(); // Where file is in byte count for resume

    sprintf ( rezBuf,"%i\r\n\r\n",finfo[0][NODE].flsz );

    // If Filesize 0, This is a New File, No Resume / Recovery
    if ( finfo[0][NODE].flsz == 0 )
    {
        for ( ;; )
        {
            nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );

            if ( nRet == INVALID_SOCKET )
            {
                sprintf ( message,"Error: Send() With Resume Info - Re-Trying!" );
                logntp ( message, NODE );
                strcpy ( finfo[0][NODE].status,message );
                memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
            }
            else if ( nRet == SOCKET_ERROR )
            {
#ifdef _WIN32
                if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
                {
#else
                if ( errno != WSAEWOULDBLOCK )
                {
#endif
                    //Draw Transfer Status
                    sprintf ( message,"Error: Send() With Resume Info - Lost Connection!" );
                    logntp ( message, NODE );
                    strcpy ( finfo[0][NODE].status,message );
                    memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                    closesocks();
                    return ( 2 );
                }
            }
            else
            {
                return ( 0 );
            }
        }
    }

    // If filesize == We Already have complete File!
    if ( finfo[0][NODE].flsz == finfo[0][NODE].size ) // Exit if the original file is same size as new
    {
        for ( ;; )
        {
            nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );

            if ( nRet == INVALID_SOCKET )
            {
                sprintf ( message,"Error: Send() With Resume Size - Retrying!" );
                logntp ( message, NODE );
                strcpy ( finfo[0][NODE].status,message );
                memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
            }
            else if ( nRet == SOCKET_ERROR )
            {
#ifdef _WIN32
                if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
                {
#else
                if ( errno != WSAEWOULDBLOCK )
                {
#endif
                    //Draw Transfer Status
                    sprintf ( message,"Error: Send() With Resume Size - Lost Connection!" );
                    logntp ( message, NODE );
                    strcpy ( finfo[0][NODE].status,message );
                    memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                    closesocks();
                    return ( 2 );
                }
            }
            else
            {
                sprintf ( message,"Error: You Already Have This Full File!" );
                logntp ( message, NODE );
                strcpy ( finfo[0][NODE].status,message );
                memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                closesocks();
                return ( 2 );
            }
        }
    }

    // Else this is a Resume Recovery, Send Where we last Leftoff in Bytes
    for ( ;; )
    {
        nRet = send ( msock.rsock, rezBuf, sizeof ( rezBuf ), 0 );

        if ( nRet == INVALID_SOCKET )
        {
            sprintf ( message,"Error: Sending Resume Reply, Re-Trying!" );
            logntp ( message, NODE );
            strcpy ( finfo[0][NODE].status,message );
            memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
        }
        else if ( nRet == SOCKET_ERROR )
        {
#ifdef _WIN32
            if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
            {
#else
            if ( errno != WSAEWOULDBLOCK )
            {
#endif
                //Draw Transfer Status
                sprintf ( message,"Error: Sending Resume Reply, Lost Connection!" );
                logntp ( message, NODE );
                strcpy ( finfo[0][NODE].status,message );
                memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                closesocks();
                return ( 2 );
            }
        }
        else
        {
            break;
        }
    }
    return ( 1 );
}

void NTPSRV::recvfdata()
{

    char szBuf[2048];
    char message[255];
    int nRet;

    int c, i, j=1;
    long totbyte=0;
    FILE *nfp;

    // Create / Open File Depending on New / Resume
    if ( finfo[0][NODE].resum ) // True
    {
        totbyte = finfo[0][NODE].flsz;
        if ( ( nfp = fopen ( finfo[0][NODE].filename, "a+b" ) ) ==  NULL )
        {
            sprintf ( message,"Error! can't create: '%s'\n",finfo[0][NODE].filename );
            erbreak[0][NODE] = true;
            logntp ( message, NODE );
            strcpy ( finfo[0][NODE].status,message );
            memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
            closesocks();
            return;
        }
    }
    else   // False
    {
        if ( ( nfp = fopen ( finfo[0][NODE].filename, "w+b" ) ) ==  NULL )
        {
            sprintf ( message,"Error! can't create: '%s'\n",finfo[0][NODE].filename );
            erbreak[0][NODE] = true;
            logntp ( message, NODE );
            strcpy ( finfo[0][NODE].status,message );
            memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
            closesocks();
            return;
        }
    }

    // Kickoff File Transfer TransferGUI
#ifdef _WIN32
    HANDLE ahThread;
    ahThread = ( HANDLE ) _beginthread ( TransferGUI, 0, ( void* ) NODE );
#else
    pthread_t thread;
    pthread_create ( &thread, NULL, TransferGUI, ( void* ) NODE );
#endif
    Sleep ( 2000 );

    // Receive data from the client
    j=1;
    while ( j>0 )
    {
        memset ( szBuf, 0, sizeof ( szBuf ) );		// clear buffer

        nRet = recv ( msock.rsock,      			// Connected client
                      szBuf,							// Receive buffer
                      sizeof ( szBuf ),					// Lenght of buffer
                      0 );								// Flags
        j=nRet;

        if ( nRet == INVALID_SOCKET )
        {
            // On Invalid, Do at least 6 Retries on the Connection before Error out!
            for ( int rtry = 0; ; rtry++ )
            {
                Sleep ( 1000 );
                if ( rtry == 10 )
                {
#ifdef _WIN32
                    if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
                    {
#else
                    if ( errno != WSAEWOULDBLOCK )
                    {
#endif
                        if ( finfo[0][NODE].size == totbyte )
                        {
                            j = 0;
                            break;
                        }
                        sprintf ( message,"Disconnected!" );
                        logntp ( message, NODE );
                        strcpy ( finfo[0][NODE].status,message );
                        memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                        fclose ( nfp );
                        closesocks();
                        erbreak[0][NODE] = true;
                        return;
                    }
                    else rtry = 0;
                }

                nRet = recv ( msock.rsock,      		// Connected client
                              szBuf,						// Receive buffer
                              sizeof ( szBuf ),				// Lenght of buffer
                              0 );							// Flags

                j = nRet;
                if ( nRet == INVALID_SOCKET ) continue;
                else if ( nRet == SOCKET_ERROR ) break;
                else break;
            }
        }

        if ( nRet == SOCKET_ERROR )
        {
#ifdef _WIN32
            if ( ( nRet = WSAGetLastError () ) != WSAEWOULDBLOCK )
            {
#else
            if ( errno != WSAEWOULDBLOCK )
            {
#endif
                if ( finfo[0][NODE].size == totbyte )
                {
                    j = 0;    // Exit Casue File is Finished
                    break;
                }
                sprintf ( message,"Disconnected!" );
                logntp ( message, NODE );
                strcpy ( finfo[0][NODE].status,message );
                memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );
                fclose ( nfp );
                closesocks();
                erbreak[0][NODE] = true;
                return;
            }
        }
        // receive data from client and save to file
        i=0;
        while ( nRet > i )
        {
            // End Transfer User Side once File is Complete!
            if ( totbyte == finfo[0][NODE].size ) break;
            c=szBuf[i];
            putc ( c, nfp );
            i++;
            totbyte++;
            finfo[0][NODE].bRecv = totbyte;
        }
        if ( totbyte == finfo[0][NODE].size ) break;
    }

    // Close Sockets to end Transfer
    closesocks();

    // Break out of GUI!
    erbreak[0][NODE] = true;

    // close file
    fclose ( nfp );

    // End of File Transfer is Reached Here if Successful!
    char fRecv[50]= {0};             // Bytes Received [Buffer]
    char fLeft[50]= {0};             // Bytes Left     [Buffer]
    char tLeft[50]= {0};             // Time Left
    long lftbyte=0;                  // Bytes Left Total

    // If Current Node is Toggled ON, Display End Transfer Stats For This Node
    if ( nstate == NODE )
    {
        // Get True File Size for End of Transfer
        checksize();
        // Draw GUI File Bytes Received / Left Information
        sprintf ( fRecv,"%.f ", ( float ) finfo[0][NODE].flsz ); // Display True Total
        lftbyte = ( finfo[0][NODE].size - finfo[0][NODE].flsz );	// Should always be 0!
        if ( lftbyte < 0 )
        {
            lftbyte = 0;
        }
        sprintf ( fLeft,"%.f ", ( float ) lftbyte );       // Display True Left

        // Draw Time & Transfer Status
        sprintf ( tLeft,"00:00:00 " );
        sprintf ( message,"Successful! " );
        strcpy ( finfo[0][NODE].status,message );
        logntp ( message, NODE );

#ifdef _WIN32
        percenttop ( 100 );
        drawpercent ( 100 );
        drawreceived ( fRecv,9,7 );
        drawleft ( fLeft,9,7 );
        drawtleft ( tLeft,15,7 );
        drawstatus ( message,15,7 );
#else
        printf ( "\n%s\n",message );
        printf ( "\nTime Left  : %s\n",tLeft );
#endif
    }

    // Copy File Stats to finfo2 for Message Window
    memcpy ( &finfo2[0][NODE],&finfo[0][NODE],sizeof ( FILEINFO ) );

    // Give 2 Second Delay for Stats to show!
#ifdef _WIN32
    Sleep ( 2000 );
#else
    sleep ( 2 );
#endif

}


#ifdef _WIN32
void DLNodeTransfer ( void *p )
{
#else
void *DLNodeTransfer ( void *p )
{
#endif

    //PASSING *pass = (PASSING*) p;
    NTPSRV *ntpsrv = ( NTPSRV* ) p;
    char message[255];

    // Set Node to Active for Connection
    finfo[0][ntpsrv->NODE].InUse = true;
    // Set GUI Error Break to False
    erbreak[0][ntpsrv->NODE] = false;

    // Get File Info
    if ( !ntpsrv->recvfinfo() )
    {
        sprintf ( message,"Error Getting FileInfo.. ." );
        ntpsrv->logntp ( message, ntpsrv->NODE );
        // end of Transfer Reset Node
        finfo[0][ntpsrv->NODE].InUse = false;
#ifdef _WIN32
        return;
#else
        return NULL;
#endif
    }

    // Check for Resume
    int i = ntpsrv->resume();
    if ( i == 2 ) // Error
    {
        sprintf ( message,"Error Getting Resume Info.. ." );
        ntpsrv->logntp ( message, ntpsrv->NODE );
        // end of Transfer Reset Node
        finfo[0][ntpsrv->NODE].InUse = false;
#ifdef _WIN32
        return;
#else
        return NULL;
#endif
    }
    else if ( i == 0 )
    {
        finfo[0][ntpsrv->NODE].resum = false;
    }
    else
    {
        finfo[0][ntpsrv->NODE].resum = true;
    }

    refreshbar();  // Refreshing Node Lightbar to Reflect Connection

    // receive file data & Start GUI Thread
    ntpsrv->recvfdata();

    // end of Transfer Reset Node to Inactive
    finfo[0][ntpsrv->NODE].InUse = false;

    refreshbar();  // Refreshing Node Lightbar to Reflect Connection
}


void StreamServer ( short nPort, SOCKET l1 )
{

    char message[255];
    NTPSRV ntpsrv;

    memset ( &finfo,0,sizeof ( finfo ) );
    memset ( &finfo2,0,sizeof ( finfo2 ) );

    // Set Default Node States
    finfo[0][0].InUse = false;
    finfo[0][ClientsConnected].InUse = false;

    // Get Handle to Incomming Connections
    SOCKET listenSocket = l1;

    // Setup for Incomming connections
    SOCKET remoteSocket;
    int sin_size;
    sin_size = sizeof ( struct sockaddr_in );

    struct sockaddr_in their_addr;

    // Main Program Loops on Incomming Connections
    for ( ;; )
    {

        // Clears the struct to recevie information from the user
        memset ( &their_addr,0,sizeof ( their_addr ) );

#ifdef _WIN32
        remoteSocket = accept ( listenSocket,
                                ( struct sockaddr * ) &their_addr,
                                &sin_size );
#else
        remoteSocket = accept ( listenSocket,
                                ( struct sockaddr * ) &their_addr,
                                ( socklen_t * ) &sin_size );
#endif
        if ( remoteSocket == INVALID_SOCKET )
        {
            sprintf ( message,"Error: accept() - Incomming Connection!" );
            //ntpsrv.logntp(message);
            return;
        }

        NodeGUI ( PORT );

        // Loop through nodes to find node not in use for next connection
        ClientsConnected = 1;
        while ( finfo[0][ClientsConnected].InUse )
        {
            if ( ClientsConnected > 5 ) ClientsConnected = 1;
            else ClientsConnected++;
#ifdef _WIN32
            Sleep ( 1000 );
#else
            sleep ( 1 );
#endif
        }
        ntpsrv.NODE = ClientsConnected;

        // Call Destructor to make sure everything is clean before starting
        memset ( &finfo[0][ClientsConnected],0,sizeof ( FILEINFO ) );
        ntpsrv.NODE = ClientsConnected; // just in case.. test later if needed

        // Set Node to Flase untill file Data is confirmed
        finfo[0][ntpsrv.NODE].InUse = true;

        // Asign Remote Connections IP Address
        finfo[0][ntpsrv.NODE].their_addr = their_addr;

        // Fill Socket Struct on Completed Connection
        ntpsrv.msock.rsock = remoteSocket;
        ntpsrv.msock.lsock = listenSocket;

        // Kick off Node Transfer Thread Here
#ifdef _WIN32
        HANDLE ahThread;
        ahThread = ( HANDLE ) _beginthread ( DLNodeTransfer, 0, ( void* ) &ntpsrv );
#else
        pthread_t thread;
        pthread_create ( &thread, NULL, DLNodeTransfer, ( void* ) ntpsrv );
#endif

    } // End of For Loop
}


void StartServer ( short nPort )
{

    NTPSRV ntpsrv;
    char message[255];

    // Create a TCP/IP stream socket to "listen" with
    SOCKET	listenSocket;

    listenSocket = socket ( AF_INET,			// Address family
                            SOCK_STREAM,		// Socket type
                            IPPROTO_TCP );		// Protocol

    if ( listenSocket == INVALID_SOCKET )
    {
        sprintf ( message,"Error: ListenSocket!" );
        ntpsrv.logntp ( message, 1 );
#ifdef _WIN32
        WSACleanup();
#endif
        exit ( 1 );
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

    char szBuf[2048];
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
        sprintf ( message,"Error: bind() ListenSocket!" );
        ntpsrv.logntp ( message, 1 );
#ifdef _WIN32
        closesocket ( listenSocket );
        WSACleanup();
#else
        close ( listenSocket );
#endif
        exit ( 1 );
    }

#ifdef _WIN32
    nRet = gethostname ( szBuf, sizeof ( szBuf ) );
    if ( nRet == INVALID_SOCKET )
    {
        sprintf ( message,"Error: gethostname()" );
        ntpsrv.logntp ( message, 1 );
#ifdef _WIN32
        closesocket ( listenSocket );
        WSACleanup();
#else
        close ( listenSocket );
#endif
        exit ( 1 );
    }
#endif

#ifdef _WIN32
    // Set the socket to listen
    nRet = listen ( listenSocket, 5 );
#else
    nRet = listen ( listenSocket, 5 );
#endif
    if ( nRet == INVALID_SOCKET )
    {
        sprintf ( message,"Error: listen() - For Connection!" );
        ntpsrv.logntp ( message, 1 );
#ifdef _WIN32
        closesocket ( listenSocket );
        WSACleanup();
#else
        close ( listenSocket );
#endif
        exit ( 1 );
    }

    /*
    //Start Node GUI
    #ifdef _WIN32
    HANDLE ahThread;
    ahThread = (HANDLE)_beginthread( NodeGUI, 0, (void*)&nPort);
    #else
    pthread_t thread;
    pthread_create(&thread, NULL, NodeGUI, (void*)nPort);
    #endif
    */

    NodeGUI ( nPort );

    // Main Program Loop Here after Winsock Init
    while ( 1 )
    {
        StreamServer ( nPort, listenSocket );
    }
}

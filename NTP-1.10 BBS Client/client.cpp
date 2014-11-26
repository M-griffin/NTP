
/*

 Client.cpp { NTP-BBS 1.10b+ Beta } (c) 2003/2004 Michael Griffin
 
 09-20-03 + Windows and Linux Ports - Completed!
 09-30-03 + OS/2 Port Completed
   		  - Also fixed Filesize Functions and Read pointer on Resume!
 10-25-03 + Added Full Info & Data 32 Bit Encryption
 11-10-03 + Removing Encryption to bring back 1.7b Source Code..
          - Adding Full DSZLOG Support
 11-28-03 + Updated to 1.8b for 1.7 bug fixes
 12-13-03 + Added Preceeding @ for Synchronet and BBS Campability, 
          - also fixed a lot of wrong INVALID_SOCKET and LOST_SOCKET messages!
 12-31-03 + Fixed Synchronet bug in dszlog, filesize 0 was also considered error!
          - added new file transfer gui to bbs end, no ansi just text like linux.
 01-04-04 + Working on DSZLOG Updates and proper format for Synchronet comp.
 01-11-04 + DSZLOG Fixed for Synchronet, rebuilting to release 1.10b
          + Adding some new Global Functinos for CloseSocks & Nsleep & clrs & getfilesize
          - tfilename & pexit!  Also Redid All Errors to NTP-BBS.LOG
          - added config file for transfer / error logging enable / disable
          - added NTPLOG Queue, so it keep each transfers stats together!
          - Tomorrow add queue for DSZLOG...
 01-12-04 + Finished tweaks and all code cleanups for release.. 1.9b
 01-21-04 + Updateing DSZLOG, End of File Buffering and release for 1.10b
          - added extra 20 second timeout if transfer speed stays at 0 continuosly.. .
            if a bad connection, user should start over later.. . also catch's freezes!
 01-24-04 + finished last touched, replaced SOCKET_ERROR with LOST_SOCKET
            Now Detects Lost connections correctly in windows..
 02-04-04 + Fixed DSZLOG Writes on timeouts, also changed time from 20 seconds to 120
            
            * doubl check dszlog writes on timeouts!
            * Also Keep try doing at least 1, or next transfer on error.. skip to next
              on file already exists .. ;)
              
 04-03-04 + add small fixed to write out all dszlogs as success when return options is set on.
 
           For linux you must link  g++  -lpthread -static

*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include "config.h"

// Win32
#ifdef _WIN32
#include <time.h>
#include <sys/timeb.h>
#include <winsock.h>
#include <windows.h>
#include <process.h>

// Linux + OS/2
#elif OS2
#include <io.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

typedef int SOCKET;
#define INVALID_SOCKET -1
#endif

// Disconnected
#define LOST_SOCKET     0

using namespace std;

typedef struct {
    char ip[255];
    char filename[255]; // File name Received with Full Path
    char truename[255]; // File name minus Path
    char bqueue[255];   // Batch Queue String
    long int size;      // Filesize
    long int bSent;     // bytes send
    long int lBytes;    // bytes left
    long int elsec;     // time elapsed
}FILEINFO;

// Handle to File Info
FILEINFO finfo; 

bool erbreak = false;     // Breaks out of GUI after Transfer
char hostip[255];         // Holds Users IP Address
char PATH[255];           // Holds Program Path
bool LOGGING = true;      // Enable Transfer Loggin
bool RETURN  = false;     // Disable DSZLOG Write all successes

// Link List for Holding Queues
struct ListItem {
    std::string str;
    struct ListItem *next;
};

// handle to Log Queue
struct ListItem *MyList = 0;;

// handle to DSZLOG Queue
struct ListItem *MyList2 = 0; 


///////////////////////////////////////////////////////////////////////////////
// Clear logntp queue
void clearlogntp() {

    if (LOGGING == false) return;
    struct ListItem *p,*next;
    if ( MyList == 0) return;

    for(p = MyList ; p !=0 ; p = next ) {
        next = p->next;
        delete (p);
    }
    MyList = 0;
    return;
}

// Process ntplog
void procntplog() {

    if (LOGGING == false) return;
    char msg[300];
    
    string temp = PATH;
    temp += "NTP-BBS.Log";
    FILE *fp1;
    if((fp1 = fopen(temp.c_str(), "a")) == NULL) {
        printf("\nFatal! Couldn't write to log file!\n");
        return;
    }    
    
    struct ListItem *p;
    for (p = MyList; p != 0; p = p->next) {
        strcpy(msg,p->str.c_str());
        fprintf(fp1, "* %s\n",msg);
    }
    fclose(fp1);
    clearlogntp();
}

// Create or Add to Log Queue
void logntp(std::string str) {

    if (LOGGING == false) return;
    char emsg[100];
    std::string temp = str;
    if (temp == "") return;

    struct ListItem *last;
    ListItem *item = new ListItem;
    if ( !item ) return;

    item->str = temp; 
    item->next = 0;

    if ( MyList == 0 ) { MyList = item; return; }
    for( last = MyList ; last->next != 0 ; last = last->next );
    last->next = item;
}


///////////////////////////////////////////////////////////////////////////////
// Clear dszlog queue
void cleardszlog() {

    struct ListItem *p,*next;
    if ( MyList2 == 0) return;

    for(p = MyList2 ; p !=0 ; p = next ) {
        next = p->next;
        delete (p);
    }
    MyList2 = 0;
    return;
}

// Process dszlog
void procdszlog() {

    char emsg[100];

    // Get Enviroment variable for DSZLOG Path
    char *dszlog;
    dszlog = getenv("DSZLOG");    
    if (dszlog==NULL) { dszlog="dsztemp.log"; }

    // Open DSZLOG
    FILE *fp1;  
    if((fp1 = fopen(dszlog, "w")) == NULL) {
        sprintf(emsg,"Fatal! Can't write to dszlog: %s!", dszlog);            
        logntp(emsg);
        return;
    }

    // Run throguh Queue and write dszlog
    char msg[300];
    struct ListItem *p;
    for (p = MyList2; p != 0; p = p->next) {
        strcpy(msg,p->str.c_str());
        fprintf(fp1, "%s",msg);
    }   
    fclose(fp1);
    cleardszlog();
}

// Create or Add to DszLog Queue
void adddszlog(char *str) {

    char emsg[100];
    string temp = str;
    if (temp == "") return;

    struct ListItem *last;
    ListItem *item = new ListItem;
    if ( !item ) {
        sprintf(emsg,"Fatal! Can't create dszlog queue!");            
        logntp(emsg);    
        return;
    }

    item->str = temp; 
    item->next = 0;

    if ( MyList2 == 0 ) { MyList2 = item; return; }
    for( last = MyList2 ; last->next != 0 ; last = last->next );
    last->next = item;
}

// DSZLOG Creation
void dodszlog(bool dszok) {
       
    // Setup DSZLOG Format and add to queue
    char msg[200];
    if (dszok) 
        sprintf(msg, "z 999999 57600 bps 9999 cps   0 errors     0 1024 %s -1\n", finfo.filename);
    else
        sprintf(msg, "E     0 57600  bps 9999 cps   1 errors     0   32 %s -1\n", finfo.filename);
        
    adddszlog(msg);
}


//////////////////////////////////////////////////////////////////////////
// Close Sockets
void closesocks(SOCKET sock) {

    #ifdef _WIN32
    closesocket(sock);
    #else
    close(sock);
    #endif
}

// Sleep / Wait
void nsleep(int i) {

    #ifdef _WIN32
    Sleep(i*1000);
    #else
    sleep(i);
    #endif
}

// Fast Clear Screen
void clrs() {

    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

// Program Exit
void pexit(int i) {

    #ifdef _WIN32
    // Turn Windows Cursor back on before exit
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO CCI;
    GetConsoleCursorInfo(hOut, &CCI);
    CCI.bVisible = true;
    SetConsoleCursorInfo(hOut, &CCI);        
	#endif	
		
	procdszlog();  // Write Out DSZLOG
	procntplog();  // Write Out Transfer / Error Log
	clrs();        // Clear Screen
	if (i > 0) printf("\nNTP exited with errors, Check NTP-BBS.LOG for details.. .\n\n");
	else { printf("\nNTP exited normally.. .\n\n"); }
	exit(i);       // Exit Program
}

// Get File Size's
void getfilesize(char *filename, long int &fsize) {

    char emsg[400];

    #ifdef _WIN32
    // Read File and Get File Size
    WIN32_FIND_DATA wfd;   // File Structure
    HANDLE          h;     // Handle to File
    h = FindFirstFile(filename, &wfd);

    if(h != INVALID_HANDLE_VALUE) {
        fsize = wfd.nFileSizeLow;
    }
    else {
        sprintf(emsg,"Error: Opening for Filesize on: %s",filename);
        logntp(emsg);
        pexit(1);
    }
    FindClose(h);
    
    #else
    // Open File and Read File Size
    FILE *fp;
    if ((fp = fopen(filename, "rb")) ==  NULL) {
        sprintf(emsg,"Error: Opening for Filesize on: %s",filename);
        logntp(emsg);
        pexit(1);
    }

    #ifdef OS2
    fsize = filelength(fileno(fp));
    #else
    fseek(fp, 0L, SEEK_END);
    fsize = ftell(fp);
    #endif
    fclose(fp);
    #endif

}

// Get real file names and Cut path's
void tfilename(char *filename) {

    // Get True Filename and cute off Path
    int num = 0;
    for (int i = 0; ;i++) {          // Count for Romoval of ExeName with Path
        if (filename[i] == '\0') break;   // End of String, Break
        #ifdef _WIN32
        if (filename[i] == '\\') num = i; // Find last or only '\' in String
        #elif OS2
        if (filename[i] == '\\') num = i; // Find last or only '\' in String
        #else
        if (filename[i] == '/') num = i;  // Find last or only '/' in String
        #endif
    }
    if (num == 0) { strcpy(finfo.truename,filename); }
    else {
        int r = 0;
        for (int i = num+1; ;i++) {  // Copy all Chars after last '\'
            if (filename[i] == '\0') break;
            finfo.truename[r] = filename[i];
            r++;
        }
    }
    strcpy(finfo.filename,filename);
}

// End of File Transfer GUI Display
void eotgui() {

    clrs();
    printf("\n.----------------------------------------------------------------.");
    printf("\n| NTP v1.10b 01/24/04 (c) Mercyful Fate 2k3/2k4                  |");
    printf("\n`----------------------------------------------------------------'\n");
    printf("\nUser IP    : %s\n\nFilename   : %s\nFilesize   : %.f\n",hostip,finfo.filename,(float)finfo.size);
    printf("\nFile Queue : %s\n",finfo.bqueue);       

    // Draw File Bytes Sent
    printf("\nBytes Sent : %.f bytes\n",(float)finfo.size);

  	// Calculate Bytes Left
    printf("~> Left    : 0 bytes\n");
    printf("\nSpeed      : 0\n");
    printf("\nTime Left  : 00:00:00 (Estimate)\n");
    
    int hh,mm,ss;
    char h[5]={0},m[5]={0},s[5]={0};
    char telapsed[20]={0};
    
    // Convert Time Elapsed in Seconds to Hours : Mintues : Seconds
    if (finfo.elsec == 0) {
        hh = 0; mm = 0; ss = 0;
    }
    else {
        if (finfo.elsec < 60) { hh = 0; mm = 0; ss = finfo.elsec; }
        else {
            hh = 0; mm = finfo.elsec / 60; ss = finfo.elsec % 60;
            if ( mm > 60) { hh = mm / 60; mm = mm % 60; }
        }
    }
    // Add Leading 0's if under 10
    if (hh < 10) sprintf(h,"0%i",hh);
    else sprintf(h,"%i",hh);

    if (mm < 10) sprintf(m,"0%i",mm);
    else sprintf(m,"%i",mm);

    if (ss < 10) sprintf(s,"0%i",ss);
    else sprintf(s,"%i",ss);
        
    // Draw Time Elapsed
    sprintf(telapsed,"%s:%s:%s",h,m,s);
    printf("~> Elapsed : %s\n",telapsed);           
  	printf("\nComplete   : 100%%\n\n");
    
    printf("\nTransfer Completed Successful!\nBye!\n");
    logntp("Transfer Completed Successful!");
    
}

// Figure out Transfer Percentage 0 - 100%
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

// File Transfer GUI Display
#ifdef _WIN32
void GUIThread( void *p ) {
#else
void *GUIThread( void *p ) {
#endif

    // Receive data from the client
    char fRecv[50]={0};               // Bytes Received [Buffer]
    char fLeft[50]={0};               // Bytes Left     [Buffer]
    char fSpeed[50]={0};              // Bytes Speed    [Buffer]
    char tleft[50]={0};				  // Time Left      [Buffer]
    char telapsed[50]={0};	          // Time Elasped   [Buffer]
    long lftbyte=0;                   // Total Bytes Left
    long spdbyte=0;                   // Bytes Transfer in 1 Second
    long totsec=0;                    // Total Seconds Left
    int  mm[2]={0};                   // Real Time Left in Minutes
    int  ss[2]={0};					  // Real Time Left in Seconds
    int  hh[2]={0};					  // Real Time Left in Seconds
    double pCent;                     // Percentage Complete
    char Status[50]={0};              // Transfer Status
    char h[5]; char m[5]; char s[5];  // Time Display
    
    int timeout = 0;                  // Time out, if speed = 0 for over 20 seconds. exit!
    
    finfo.elsec = 0;

    char fSize[255];
    sprintf(fSize,"%.f bytes",(float)finfo.size);
    
    sprintf(Status,"Transfering! ");       

	// Draw Batch Queue Status
	sprintf(Status,finfo.bqueue);

    // Loops Transfer Statistics
    while(1) {
    
        // Break Out of GUI for Error or Transfer Complete
    	if (erbreak) break;

        clrs();
        printf("\n.----------------------------------------------------------------.");
        printf("\n| NTP v1.10b 01/24/04 (c) Mercyful Fate 2k3/2k4                  |");
        printf("\n`----------------------------------------------------------------'\n");
        printf("\nUser IP    : %s\n\nFilename   : %s\nFilesize   : %.f\n",hostip,finfo.filename,(float)finfo.size);
        printf("\nFile Queue : %s\n",Status);
        
        // Draw File Bytes Sent
	    sprintf(fRecv,"%.f bytes",(float)finfo.bSent);
        printf("\nBytes Sent : %s\n",fRecv);

    	// Calculate Bytes Left
        lftbyte = (finfo.size - finfo.bSent);
        if (lftbyte < 0) { lftbyte = 0; }
    	sprintf(fLeft,"%.f bytes",(float)lftbyte);
        printf("~> Left    : %s\n",fLeft);

        // Calculate Average Bytes Transfered Per Second
        spdbyte = finfo.bSent - finfo.lBytes;
        
        // Check for timeout if == 20 seconds, exit program, lost connection!
        if (spdbyte <= 0) ++timeout;
        else timeout = 0;
        if (timeout == 120) {
            logntp("Transfer Timed out! Exiting.. .");
            dodszlog(false);
            pexit(1);
        }

        // Calucate Time in Seconds Left of Transfer
        if ((spdbyte != 0) && (lftbyte != 0)) {
	        totsec = lftbyte / spdbyte;
    	}
    	else { totsec = 0; }

        if (spdbyte > 1000) {
            spdbyte /= 1000;          // Convert to KiloBytes/Sec
            sprintf(fSpeed,"%.f kb/sec",(float)spdbyte);
        } // Else to slow to show a Kilobyte, Display in Bytes
        else sprintf(fSpeed,"%.f bytes/sec",(float)spdbyte);
        printf("\nSpeed      : %s\n",fSpeed);
        

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
        sprintf(tleft,"%s:%s:%s (Estimate)",h,m,s);
        printf("\nTime Left  : %s\n",tleft);
        
        // Convert Time Elapsed in Seconds to Hours : Mintues : Seconds
        if (finfo.elsec == 0) {
            hh[1] = 0; mm[1] = 0; ss[1] = 0;
        }
        else {
            if (finfo.elsec < 60) { hh[1] = 0; mm[1] = 0; ss[1] = finfo.elsec; }
            else {
                hh[1] = 0; mm[1] = finfo.elsec / 60; ss[1] = finfo.elsec % 60;
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
        sprintf(telapsed,"%s:%s:%s",h,m,s);
        printf("~> Elapsed : %s\n",telapsed);
        
        // Draw Percentage Bar
        pCent = percentage(finfo.bSent,finfo.size);
        // Don't hit 100 before end!
        if (pCent > 98) pCent = 99; 
    	printf("\nComplete   : %.f%%\n\n",(float)pCent);

        finfo.lBytes = finfo.bSent; // Setup Bytes Received before next Loop
    	nsleep(1);
        ++finfo.elsec;
	}
}

///////////////////////////////////////////////////////////////////////////////
// NTP Client Class
class client {

    public:   
    bool dszok;                          // Transfer Success/Failure
    
    client::client();
    void StreamClient(short nPort);
    void setupbatch(short nPort);

    private:    
    struct ListItem *MyList1;            // handle to Batch queue
    void clearbatchdata();               // Clears Queue
    void addbatchdata(string str);       // Add to Queue
    long resume(SOCKET sock);            // Get File Resume bytes
    void sendfdata(SOCKET sock);         // Send File Data
    void sendfinfo(SOCKET sock);         // Send File Info

};

// Class Constructor
client::client() {

    MyList1 = 0;
    memset(&finfo,0,sizeof(FILEINFO));    
    memset(hostip,0,sizeof(hostip));
    dszok  = false;  // Defaukt to Error untill a Success is Reached
}

// Clear Batch Queue
void client::clearbatchdata() {

    struct ListItem *p,*next;
    if ( MyList1 == 0) return;

    for(p = MyList1 ; p !=0 ; p = next ) {
        next = p->next;
        delete (p);
    }
    MyList1 = 0;
    return;
}

// Create or Add to Batch Queue
void client::addbatchdata(std::string str) {

    char emsg[300];
    string temp = str;
    if (temp == "") return;

    struct ListItem *last;
    ListItem *item = new ListItem;
    if ( !item ) {
        sprintf(emsg,"Error: Malloc Failed, Batch Queue!");       
        logntp(emsg);
        return;
    }

    item->str = temp; item->next = 0;

    if ( MyList1 == 0 ) { MyList1 = item; return; }
    for( last = MyList1 ; last->next != 0 ; last = last->next );
    last->next = item;
}

// Check for File Recovery
long client::resume(SOCKET sock) {

    char szBuf[2048]={0};
    char tmpin[1000]={0};
    char emsg[300];
    std::string msgEop;
    int id;
    int nRet;
    
    // Loop throguh RECV for Resume Packet
    for (;;) {
        memset(szBuf, 0, sizeof(szBuf));  // clear buffer

        // Now Get Reply back, if File is New or Recovery
        nRet = recv(sock, szBuf, sizeof(szBuf), 0);
        if (nRet == INVALID_SOCKET)     {
          
            for (int rtry = 0; ; rtry++) {
                nsleep(1);
                if (rtry == 10) {
                    erbreak = true;                    
                    sprintf(emsg,"Recv() - INVALID_SOCKET - Resume Status Lost Connection!");
                    logntp(emsg);
                    closesocks(sock);
                    dodszlog(dszok);
                    pexit(1);
                }
                
                nRet = recv(sock, szBuf, sizeof(szBuf), 0);
                if (nRet == INVALID_SOCKET) continue; 
                else if (nRet == LOST_SOCKET) {
                    #ifdef _WIN32
                    sprintf(emsg, "Recv() LOST_SOCKET Resume Status - %i",WSAGetLastError());
                    #else
                    sprintf(emsg, "Recv() LOST_SOCKET Resume Status - %i",errno);
                    #endif
                    logntp(emsg);
                    erbreak = true;     
                    closesocks(sock);
                    dodszlog(dszok);
                    pexit(1);
                  
                }
                else break;
            }
        }
        else if (nRet == LOST_SOCKET) {
            sprintf(emsg, "Error! Disconnected, Exiting.. . ");
            logntp(emsg);
            closesocks(sock);
            return (-1);
        }
        
        strcat(tmpin,szBuf);
        msgEop = tmpin;
        id = 0;
        id = msgEop.find("\r\n\r\n", 0);
        if (id != -1) break;
    }
    

    // After Receiving Full Packet, Chop it up so we can Sort the Information
    char tmppak[255]={0};
    char arg[255]={0};
    int pamcnt, ab;
    pamcnt = 0;
    ab = 0;
    int num = 0;

    int c, i;
    long resbytes = 0;
    long totbyte=0;

    // Cut Up Recevied Packet and Get the Following Information
    for (int i = 0; ;i++) {
    if (tmpin[i] == '\0') break;
        // Search for split in string
        if ((tmpin[i] == '\r') && (tmpin[i+1] == '\n')) {
            ++pamcnt;            
            // Check for Resume Size String
            if (pamcnt == 1) {
                strcpy(szBuf,tmppak);               
                memset(tmppak,0,255); ab = 0;
                resbytes = atol(szBuf);
                
                // If 0 Resume Bytes, then File Already Exists in it's Entiriety!
                if (resbytes == 0) {
                    printf("\nStarting Transfer Initilization.. .\n");
                    return (0);
                }                
                // If Users File is larger then File being Sent.. Error!
                else if (resbytes > finfo.size) {
                    sprintf(emsg,"Error! - Incorrect File Already Exists at Users End!,");
                    logntp(emsg);
                    sprintf(emsg,"       - File User has is Larger then BBS is Sending, Aborting.. .");
                    logntp(emsg);
                    closesocks(sock);
                    nsleep(2);
                    return (-1);
                }                
                // If Resume Bytes == Files, then File Already Exists in it's Entiriety!
                else if (resbytes == finfo.size) {
                    sprintf(emsg,"Error! Full File Already Exists at Users End!, Aborting.. .");
                    logntp(emsg);
                    closesocks(sock);
                    nsleep(2);
                    return (-1);
                }
                else {
                    sprintf(emsg,"NTP: Recovery from byte %i",resbytes);
                    logntp(emsg);
                    printf("\n%s\n",emsg);
                    return (resbytes); // bytes to resume at
                }
            }
            else break;
        }
        else {
            tmppak[ab] = tmpin[i];
            ab++;
        }
    }
}

// Start File Data to User
void client::sendfdata(SOCKET sock) {

    long totbyte=0;
    char szBuf[2048];
    int c, i;
    
    char emsg[300];
    
    // Set Default GUI Break to False before starting thread
    erbreak = false;
    
    // Open File to Begin File Transfer
    FILE *fp;
    if ((fp = fopen(finfo.filename, "rb")) ==  NULL) {
        sprintf(emsg,"Error: Can't open file: %s", finfo.filename);
        logntp(emsg);
        closesocks(sock);
        dodszlog(dszok);
        pexit(1);
    }

    // Check on Transfer Recovery or New File
    long resbyte;
    resbyte = resume(sock);
    
    // Fix here if file already exists to skip to next file!
    if (resbyte == -1) {
        printf("\nError on Resume!\n");
        closesocks(sock);
        fclose(fp);
        dodszlog(dszok);
        pexit(1);
    }
    else if (resbyte > 0) {
        if ( fseek(fp,resbyte,SEEK_SET) == 0) {
            totbyte = resbyte;
        }
        else { // Backup
            for (int res = 0; res != resbyte; res++) { c=getc(fp); }
            totbyte = resbyte;
        }
    }
    
    // Start GUI Transfer Thread
    #ifdef _WIN32
    HANDLE ahThread;
    ahThread = (HANDLE)_beginthread( GUIThread, 0, (void *)NULL);
    #else
    pthread_t thread;
    pthread_create(&thread, NULL, GUIThread, (void*)NULL);
    #endif

 	// Start File Transfer
    int nRet;
    do {
        i=0;
        while ( (i<2000) && ((c=getc(fp)) != EOF)  )    {            
            szBuf[i] = c;
            i++;
            totbyte++;
            finfo.bSent = totbyte;
        }

        // Send Data
        nRet = send(sock,                // Connected socket
                   szBuf,                // Data buffer
                       i,                // Length of data
                       0);               // Flags

        // If Error, Gives 10 Retries before exiting with lost Connection!
        if (nRet == INVALID_SOCKET) {

            for (int rtry = 0; ; rtry++) {
                nsleep(1);
                if (rtry == 10) {
                    erbreak = true;                    
                    sprintf(emsg,"send() - INVALID_SOCKET File Data Lost Connection!");
                    logntp(emsg);
                    closesocks(sock);
                    fclose(fp);
                    dodszlog(dszok);
                    pexit(1);
                }

                nRet = send(sock,        // Connected socket
                           szBuf,        // Data buffer
                               i,        // Length of data
                               0);       // Flags

                if (nRet == INVALID_SOCKET) continue; 
                else if (nRet == LOST_SOCKET) {
                    #ifdef _WIN32
                    sprintf(emsg, "INVALID_SOCKET File Data - %i",WSAGetLastError());
                    #else
                    sprintf(emsg, "INVALID_SOCKET File Data - %i",errno);
                    #endif
                    logntp(emsg);
                    erbreak = true;     
                    closesocks(sock);
                    fclose(fp);
                    dodszlog(dszok);
                    pexit(1);
                  
                }
                else break;
            }
        }
        else if (nRet == LOST_SOCKET) {
            #ifdef _WIN32
            sprintf(emsg, "LOST_SOCKET - %i",WSAGetLastError());
            #else
            sprintf(emsg, "LOST_SOCKET - %i",errno);
            #endif
            logntp(emsg);
            erbreak = true;       
            closesocks(sock);
            fclose(fp);
            dodszlog(dszok);
            pexit(1);            
        }
        
    } while (c != EOF);

  
    // This makes sure other side gets all data before closing socket
    // Otherside closes when all data is received so this just push's it out.
    #ifdef _WIN32
    for (;;) {
        // End of File Transfer
        nRet = send(sock,        // Connected socket
                   szBuf,        // Data buffer
           sizeof(szBuf),        // Length of data
                      0);        // Flags
                   
        if ((nRet == INVALID_SOCKET) || (nRet == LOST_SOCKET)) break;
        nsleep(1);
    }
    #else
    nRet = send(sock, szBuf, sizeof(szBuf), 0);
    nRet = send(sock, szBuf, sizeof(szBuf), 0);
    #endif

    erbreak = true;
    nsleep(1);

    // End of Transfer Successful GUI Display
    eotgui();

    dszok=true;
    dodszlog(dszok);
    nsleep(2);
    closesocks(sock);
    fclose(fp);
}

// Send File Info to User
void client::sendfinfo(SOCKET sock) {

    char emsg[300];       
    char szTmp0[2048]={0};
    
    // Setup & Log File Info and NTP Version String
    sprintf(szTmp0,"V1.10B\n%s\n%i\n%s\n\r\n\r\n",finfo.truename,finfo.size,finfo.bqueue);
    sprintf(emsg,"Filename  : %s ",finfo.truename);
    logntp(emsg);
    sprintf(emsg,"Filesize  : %i ",finfo.size);
    logntp(emsg);
    sprintf(emsg,"Filequeue : %s ",finfo.bqueue);
    logntp(emsg);

    int nRet;
    for (;;) {
        nRet = send(sock, szTmp0, sizeof(szTmp0), 0);
        if (nRet == INVALID_SOCKET) {       
            for (int rtry = 0; ; rtry++) {
                nsleep(1);
                if (rtry == 10) {
                    erbreak = true;                    
                    sprintf(emsg,"Recv() - INVALID_SOCKET - File Info Lost Connection!");
                    logntp(emsg);
                    closesocks(sock);
                    dodszlog(dszok);
                    pexit(1);
                }
                
                nRet = send(sock, szTmp0, sizeof(szTmp0), 0);
                
                if (nRet == INVALID_SOCKET) continue; 
                else if (nRet == LOST_SOCKET) {
                    #ifdef _WIN32
                    sprintf(emsg, "Recv() LOST_SOCKET File Info - %i",WSAGetLastError());
                    #else
                    sprintf(emsg, "Recv() LOST_SOCKET File Info - %i",errno);
                    #endif
                    logntp(emsg);
                    erbreak = true;     
                    closesocks(sock);
                    dodszlog(dszok);
                    pexit(1);
                  
                }
                else break;                                
            }
        }
        else if (nRet == LOST_SOCKET) {
            sprintf(emsg,"Error: send() File Info - Lost Connection! Exiting.. .");
            logntp(emsg);
            closesocks(sock);
            dodszlog(dszok);
            pexit(1);
        }
        break;
    }
}

// Setup Sockets and Connection to Client
void client::StreamClient(short nPort) {

    #ifdef _WIN32
    // Initalize Winsock - Windows only
    WORD wVersionRequested = MAKEWORD(1,1);
    WSADATA wsaData;

    // Initialize WinSock and check the version
    int nRet = WSAStartup(wVersionRequested, &wsaData);
    if (wsaData.wVersion != wVersionRequested) {
        fprintf(stderr,"\nError: Winsock Expecting v1.1\n");
        logntp("Error: Winsock Expecting v1.1");
        WSACleanup();
        dodszlog(dszok);
        pexit(1);
    }
    #endif

    char logs[100];
    sprintf(logs,"NTP v1.10b - Connecting To User: %s On Port: %d", hostip, nPort);
    logntp("\n***********************************************************************************\n");
    logntp(logs);    
    clrs();
    printf("\n%s",logs);   

    #ifdef _WIN32 // Windows WinSocket INIT
    SOCKET  theSocket;
    LPHOSTENT lpHostEntry;

    lpHostEntry = gethostbyname(hostip);
    if (lpHostEntry == NULL) {
        sprintf(logs,"Error: GetHostByName()");
        logntp(logs);
        WSACleanup();
        dodszlog(dszok);
        pexit(1);
    }
    
    theSocket = socket(AF_INET,           // Address family
                   SOCK_STREAM,           // Socket type
                   IPPROTO_TCP);          // Protocol
                   
    if (theSocket == INVALID_SOCKET) {
        sprintf(logs,"Error: Init Socket()");
        logntp(logs);
        WSACleanup();
        dodszlog(dszok);
        pexit(1);
    }

    // Fill in the address structure
    SOCKADDR_IN saServer;
    saServer.sin_family = AF_INET;
    saServer.sin_addr = *((LPIN_ADDR)*lpHostEntry->h_addr_list);
                                           // Server's address
    saServer.sin_port = htons(nPort);      // Port number from command line

    nRet = connect(theSocket,              // Socket
                  (LPSOCKADDR)&saServer,   // Server address
                  sizeof(struct sockaddr));// Length of server address structure
                  
    if (nRet == INVALID_SOCKET) {
        sprintf(logs,"Error: Connect()");
        logntp(logs);
        closesocket(theSocket);
        WSACleanup();
        dodszlog(dszok);
        pexit(1);
    }

    #else // Linux / OS2 Sockets INIT
    int theSocket;
    struct hostent *he;
    struct sockaddr_in their_addr;

    if ((he=gethostbyname(hostip)) == NULL) {
        sprintf(logs,"Error: GetHostByName()");
        logntp(logs);
        sprintf(logs,"Hostname: %s",hostip);
        logntp(logs);
        dodszlog(dszok);
        pexit(1);
    }

    if ((theSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        sprintf(logs,"Error: Init Socket()");
        logntp(logs);
        dodszlog(dszok);
        pexit(1);
    }

    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(nPort);
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct

    if (connect(theSocket, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == INVALID_SOCKET) {
        sprintf(logs,"Error: Connect()");
        logntp(logs);
        dodszlog(dszok);
        pexit(1);
    }
    #endif

    // Connection Ready, Send File Info
    sendfinfo(theSocket);    
    nsleep(2);
    // File Data Checked, Begin File Transfer!
    sendfdata(theSocket);

    #ifdef _WIN32
    WSACleanup();
    #endif
    return;
}

// Batch Transfer Setup
void client::setupbatch(short nPort) {

    char arg[255]={0};
    char ems[400]={0};
    std::string str;
    strcpy(arg,finfo.filename);
    std::ifstream inStream;

    // Open FI.LST and Get All File in Queue
    inStream.open(arg);
    if (!inStream.is_open()) {
        sprintf(ems,"Can't open: %s .. .\nFor Batch File Transfer.. .", arg );
        logntp(ems);
        return;
    }

    // Loop For Each File in FI.LST until End of File
    int count = 0;
    while( !inStream.eof() ) {
        getline(inStream,str);  // Get Full Batch file Line from File
        if (str != "") {        // Weed out any Blank Enteries / Lines
            addbatchdata(str);  // Add each batch file into Link List
            count++;            // Keep Track of Batch Queue
        }
    }
    inStream.close();

    int  current = 0;
    char bqueue[255];

    // Run through link list of Batch Queue and send each file
    struct ListItem *p;
    for ( p = MyList1 ; p != 0 ; p = p->next ) {
        ++current;
        strcpy(arg,p->str.c_str());
        memset(&finfo,0,sizeof(FILEINFO));
        sprintf(bqueue,"%i of %i",current,count);
        strcpy(finfo.bqueue,bqueue);

        // Get True Filename and cute off Path
        tfilename(arg);

        // Get Filesize
        getfilesize(finfo.filename,finfo.size);
       
        // Reset DSZLOG State to ERROR untill a Success is Reached
        dszok = false;

        // Start Transfer For Batch File
        StreamClient(nPort);

        // Wait 6 seconds before Reconnect for next file
        nsleep(6);
    }

    // Clear Batch Queue
    clearbatchdata();
}

///////////////////////////////////////////////////////////////////////////////
// Main Program Entrance
int main(int argc, char **argv) {

    #ifdef _WIN32
    SetConsoleTitle("Nemesis Transfer Protocol [BBS v1.10 beta]");
    #endif

    short nPort;

    // Check for the host and port arguments
    if (argc != 4) {
        clrs();
        fprintf(stderr,"\nNTP v1.10 beta 01/24/2004 (c) Mercyful Fate 2k3/2k4\n");
        fprintf(stderr,"\nSyntax: NTP-BBS  Hostname/IP PortNumber FileName\n");
        fprintf(stderr,"=======================================================\n");
        fprintf(stderr,"Example:NTP-BBS  192.168.0.1 2021       GirlGoneWild.zip\n\n");
        exit(1);
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
    
    // Get FileName, Port #
    char arg[255]={0};
    nPort = atoi(argv[2]);
    strcpy(arg,argv[3]);

    // Handle to NTPClient Class
    class client cli;
    
    // Setup for testing for batch Flag
    bool batcht = false;
    std::string tmp;
    
    int id1;
    tmp = arg;
    
    // If @ Batch flag found, set batch true, and erase char from filename/path
    id1 = tmp.find("@",0);    
    if (id1 != -1) {
        batcht = true;
        tmp.erase(id1,1);
        strcpy(arg,tmp.c_str());
    }

    // Get True Filename and cute off Path
    tfilename(arg);

    // Get Filesize
    getfilesize(finfo.filename,finfo.size);

    // Get IP Address of Remote Server
    strcpy(hostip,argv[1]);

    // If Batch is Detected Else do Singal Transfer
    char bqueue[50];
    if(batcht) { cli.setupbatch(nPort); }
    else {
        sprintf(finfo.bqueue,"1 of 1");
        cli.StreamClient(nPort);
    }
    // End of Transfer(s) Sucessfully, Exit OK
    pexit(0);
}




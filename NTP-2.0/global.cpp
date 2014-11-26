
#include <stdio.h>
#include <winsock.h>
#include <windows.h>
#include "ansi.h"
#include "wincon.h"
#include "global.h"

// Node State *Working* / Pauses Ansimation during lightbars
bool nsworking;

// 0 - 100%
double percentage(long double wk, long double pk) {

    double p = (0.0);
    if (pk == 0) return (0.0);
    if ( wk > 0 ) {
        if (wk == pk) return (100.0);
        else { p = 100 * (wk / pk); return p; }
    }
    return (0.0);
}

#ifdef _WIN32
void TransferGUI( void *p ) {
#else
void *TransferGUI( void *p ) {
#endif

    int NODE = (int)p;

    // Receive data from the client
    char fRecv[50]={0};               // Bytes Received [Buffer]
    char fLeft[50]={0};               // Bytes Left     [Buffer]
    char fSpeed[50]={0};              // Bytes Speed    [Buffer]
    char tleft[50]={0};				  // Time Left      [Buffer]
    char telapsed[50]={0};	          // Time Elasped   [Buffer]
    long lftbyte=0;                   // Total Bytes Left
    long spdbyte=0;                   // Bytes Transfer in 1 Second
    long totsec=0;                    // Total Seconds Left
    long elsec=0;                     // Total Seconds Elapsed
    int  mm[2];                       // Real Time Left in Minutes
    int  ss[2];						  // Real Time Left in Seconds
    int  hh[2];						  // Real Time Left in Seconds
    double pCent;                     // Percentage Complete
    char Status[50]={0};              // Transfer Status
    char h[5]; char m[5]; char s[5];  // Time Display
    
    int  avg[6]={0};
    long lastavg = 0;

    char fSize[255];
    sprintf(fSize,"%.f ",(float)finfo[0][NODE].size);
           
    // Draw Transfer Status
    if (finfo[0][NODE].resum) sprintf(Status,"Recovery!");
    else sprintf(Status,"Transfering! ", NODE);
	    
    // Give 1 Second for File Data to Catch up to GUI
    #ifdef _WIN32
    Sleep(1000);
    #else
    sleep(1);
    #endif

    // Loops Transfer Statistics
    for(;;) {    
        // If Node Window was just started. Display Fresh ansi    
        if ((nstate == NODE) && (pnstate != NODE)) {        
            // Reset Previous NodeState to Current So Ansi Isn't Refreshed Each Frame
            switch(NODE) {
                case 1:  pnstate = DLNode1; break;
                case 2:  pnstate = DLNode2; break;
                case 3:  pnstate = DLNode3; break;
                case 4:  pnstate = DLNode4; break;
                case 5:  pnstate = DLNode5; break;
                case 6:  pnstate = ULNode1; break;
                case 7:  pnstate = ULNode2; break;
                case 8:  pnstate = ULNode3; break;
                case 9:  pnstate = ULNode4; break;
                case 10: pnstate = ULNode5; break;
            }

            #ifdef _WIN32
            // Display Transfer GUI
        	ansiparse(aBuf2);
        	// Displayer IP address of Client Connecting
            // drawip(inet_ntoa(finfo[0][NODE].their_addr.sin_addr),0,7);    
            // Display Filename + Filesize
            drawfilename(finfo[0][NODE].filename,15,7);    
            drawfilesize(fSize,15,7);
            #endif   
    
            #ifdef _WIN32
            drawstatus(Status,9,7);
            #else
            printf("\nStatus: %s\n",Status);
            #endif  
    
            #ifdef _WIN32
            queuestatus(finfo[0][NODE].bqueue,0,7);
            drawpercent(0);
            
            // Node Status
            gotoxy(54,5);
            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("%i",NODE);            
            #endif                        
        }
        
        // Break if Transfer Complete
    	if ((finfo[0][NODE].size == finfo[0][NODE].bRecv) || (erbreak[0][NODE])) {
            if(nstate == NODE) {
                Sleep(2000);
                pnstate = MidState;
            }
            break;
        }
        
        // If Current Node == nstate, Display Transfer Stats for This Node
        if (nstate == NODE) {
            #ifndef _WIN32
            #ifdef OS2
            system("cls");
            #else
            system("clear");
            #endif
            printf("\n\n%s\n", VERSION);
            printf("\nIP: %s\n\nFilename   : %s\nFilesize   : %.f\n",inet_ntoa(ntpsrv.their_addr.sin_addr),ntpsrv.finfo.filename,(float)ntpsrv.finfo.size);
            printf("\nBatch Queue: %s\n",Status);
            #endif

            // Draw File Bytes Received
	        sprintf(fRecv,"%.f ",(float)finfo[0][NODE].bRecv);
    	    #ifdef _WIN32
           	drawreceived(fRecv,9,7);
    	    #else
            printf("\nBytes Recv : %s\n",fRecv);
            #endif

        	// Calculate Bytes Left
            lftbyte = (finfo[0][NODE].size - finfo[0][NODE].bRecv);
            if (lftbyte < 0) { lftbyte = 0; }
        	sprintf(fLeft,"%.f ",(float)lftbyte);
        	#ifdef _WIN32
            drawleft(fLeft,9,7);
            #else
            printf("\nBytes Left : %s\n",fLeft);
            #endif

            // Calculate Average Bytes Transfered Per Second
            avg[0] = finfo[0][NODE].bRecv - finfo[0][NODE].lRecv;
            avg[5] = avg[4]; avg[4] = avg[3]; avg[3] = avg[2]; avg[2] = avg[1]; avg[1] = avg[0];                      
                       
            spdbyte = avg[1]+avg[2]+avg[3]+avg[4]+avg[5];
            if ((spdbyte < lastavg) && (spdbyte != 0))
                if ((spdbyte - lastavg) > 10)                
                    spdbyte = lastavg;

            lastavg = spdbyte;
            // Calculate Average Bytes Transfered Per Second        
            
            spdbyte /= 5;
            
            if (spdbyte < 0) spdbyte = 0;

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
        printf("\nTime Left  : %s\n",tleft);
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
        sprintf(telapsed,"%s:%s:%s",h,m,s);
        #ifdef _WIN32
        drawtelapse(telapsed,15,7);
        #else
        printf("\n> Elapsed  : %s\n",telapsed);
        #endif

            // Draw Percentage Bar
            pCent = percentage(finfo[0][NODE].bRecv,finfo[0][NODE].size);
            #ifdef _WIN32
    	    percenttop(pCent);
    	    drawpercent(pCent);
        	#else
        	printf("\nComplete   : %.f \%\n\n",(float)pCent);
    	    #endif

            finfo[0][NODE].lRecv = finfo[0][NODE].bRecv; // Setup to Calculate for KB/S
        }
        // Update Transfer Stats Every Second
        #ifdef _WIN32
        Sleep(1000);
        #else
        sleep(1);
        #endif
        ++elsec;
    } 
}


void refreshbar() {
    

    if ((nstate == NodeWindow) && (!nsworking)) {                
    
        gotoxy(5,10);
        if (finfo[0][1].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 1 ");

        gotoxy(5,11);
        if (finfo[0][2].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 2 ");

        gotoxy(5,12);
        if (finfo[0][3].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 3 ");

        gotoxy(5,13);
        if (finfo[0][4].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 4 ");

        gotoxy(5,14);
        if (finfo[0][5].InUse) setcolor(15,7);
        else setcolor(0,7);
        printf(" node 5 ");

        gotoxy(5,17);
        if (finfo[1][1].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 1 ");

        gotoxy(5,18);
        if (finfo[1][2].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 2 ");

        gotoxy(5,19);
        if (finfo[1][3].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 3 ");

        gotoxy(5,20);
        if (finfo[1][4].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 4 ");

        gotoxy(5,21);
        if (finfo[1][5].InUse) setcolor(15,7);
        else setcolor(0,7);            
        printf(" node 5 ");
    }
      
      
    if ((nstate == NodeWindow) && (nwbar == CONNECT) && (!nsworking)) {
        NodeGUI(PORT);
        switch(nbar) {
        
            case ULNode1: gotoxy(5,17); 
                    if (finfo[1][1].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 1 "); 
                    break;
            case ULNode2: gotoxy(5,18); 
                    if (finfo[1][2].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 2 "); 
                    break;
            case ULNode3: gotoxy(5,19); 
                    if (finfo[1][3].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 3 "); 
                    break;
            case ULNode4: gotoxy(5,20); 
                    if (finfo[1][4].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 4 "); 
                    break;
            case ULNode5: gotoxy(5,21); 
                    if (finfo[1][5].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 5 "); 
                    break;
        }
        // Reset Bottom Lightbar
        gotoxy(9,23);  setcolor(0,7);  printf(" snoop ");       
        gotoxy(16,23); setcolor(15,1); printf(" connect "); 

    }
          
    
    if ((nstate == NodeWindow) && (nwbar == SNOOP) && (!nsworking)) {
        NodeGUI(PORT);
        switch(nbar) {
            case DLNode1: gotoxy(5,10); 
                    if (finfo[0][1].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 1 "); 
                    break;
            case DLNode2: gotoxy(5,11); 
                    if (finfo[0][2].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 2 "); 
                    break;
            case DLNode3: gotoxy(5,12); 
                    if (finfo[0][3].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 3 "); 
                    break;                    
            case DLNode4: gotoxy(5,13); 
                    if (finfo[0][4].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 4 "); 
                    break;
            case DLNode5: gotoxy(5,14); 
                    if (finfo[0][5].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 5 "); 
                    break;
                                                
            case ULNode1: gotoxy(5,17); 
                    if (finfo[1][1].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 1 "); 
                    break;
            case ULNode2: gotoxy(5,18); 
                    if (finfo[1][2].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 2 "); 
                    break;
            case ULNode3: gotoxy(5,19); 
                    if (finfo[1][3].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 3 "); 
                    break;
            case ULNode4: gotoxy(5,20); 
                    if (finfo[1][4].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 4 "); 
                    break;
            case ULNode5: gotoxy(5,21); 
                    if (finfo[1][5].InUse) setcolor(15,5);
                    else setcolor(15,1); 
                    printf(" node 5 "); 
                    break;
        }
    }
}


void NodeGUI( int port ) {

    /*
    // Holds Port for GUI Display
    int port;
    memcpy(&port,p,sizeof(short));
    */
    
    // Color Rotation
    int a = 2, b, c, d, e;    
    
    //for(;;) {
    
        // Setup Colors       
        a = 0, b = 15, c = 11, d = 9, e = 1;
 
        // On Inital Startup of NodeWindow
        if (nstate == NodeWindow && pnstate != NodeWindow) {
            ansiparse(aBuf1); pnstate = NodeWindow;
           	// Display Snoop Mesasge Window
        	ansiparse(aBuf12);

            gotoxy(65,5);
            setcolor(15,0);
            printf("%i",PORT);
            setcolor(7,0);
               
            // Refresh node lightbar display         
            if (nsworking) {
                nsworking = false;
                refreshbar();
                nsworking = true;
                Sleep(20);
            }
            else refreshbar();
        }
                            
        // Normal Node Window Interface
        if ((nstate == NodeWindow) && (!nsworking)) {
            // Downloads                    
            // Waiting For Caller Stats
            if (!finfo[0][1].InUse) {
                gotoxy(15,10);
                setcolor(a,7);
                printf("                           ");
                gotoxy(15,10);
                printf("Waiting For Connection ");                
            }
            else {
                gotoxy(15,10);
                setcolor(a,7);
                printf("                           ");
                gotoxy(15,10);
                printf("Connected %s",inet_ntoa(finfo[0][1].their_addr.sin_addr));
            }
            if (!finfo[0][2].InUse) {
                gotoxy(15,11);
                setcolor(b,7);
                printf("                           ");
                gotoxy(15,11);
                printf("Waiting For Connection ");
            }
            else {
                gotoxy(15,11);
                setcolor(b,7);
                printf("                           ");
                gotoxy(15,11);
                printf("Connected %s",inet_ntoa(finfo[0][2].their_addr.sin_addr));
            }
            if (!finfo[0][3].InUse) {
                gotoxy(15,12);
                setcolor(c,7);
                printf("                           ");
                gotoxy(15,12);
                printf("Waiting For Connection ");
            }
            else {
                gotoxy(15,12);
                setcolor(c,7);
                printf("                           ");
                gotoxy(15,12);
                printf("Connected %s",inet_ntoa(finfo[0][3].their_addr.sin_addr));
            }
            if (!finfo[0][4].InUse) {
                gotoxy(15,13);
                setcolor(d,7);
                printf("                           ");
                gotoxy(15,13);
                printf("Waiting For Connection ");
            }
            else {
                gotoxy(15,13);
                setcolor(d,7);
                printf("                           ");
                gotoxy(15,13);
                printf("Connected %s",inet_ntoa(finfo[0][4].their_addr.sin_addr));
            }
            if (!finfo[0][5].InUse) {
                gotoxy(15,14);
                setcolor(e,7);
                printf("                           ");
                gotoxy(15,14);
                printf("Waiting For Connection ");
            }
            else {
                gotoxy(15,14);
                setcolor(e,7);
                printf("                           ");
                gotoxy(15,14);
                printf("Connected %s",inet_ntoa(finfo[0][5].their_addr.sin_addr));
            }
            
            // Uploads
            // Waiting For Caller Stats
            if (!finfo[1][1].InUse) {
                gotoxy(15,17);
                setcolor(a,7);
                printf("                           ");
                gotoxy(15,17);
                printf("Feature Comming Soon!");
            }
            else {
                gotoxy(15,17);
                setcolor(a,7);
                printf("                           ");
                gotoxy(15,17);
                printf("Connected %s",inet_ntoa(finfo[1][1].their_addr.sin_addr));
            }
            if (!finfo[1][2].InUse) {
                gotoxy(15,18);
                setcolor(b,7);
                printf("                           ");
                gotoxy(15,18);
                printf(" ");
            }
            else {
                gotoxy(15,18);
                setcolor(b,7);
                printf("                           ");
                gotoxy(15,18);
                printf("Connected %s",inet_ntoa(finfo[1][2].their_addr.sin_addr));
            }
            if (!finfo[1][3].InUse) {
                gotoxy(15,19);
                setcolor(c,7);
                printf("                           ");
                gotoxy(15,19);
                printf(" ");
            }
            else {
                gotoxy(15,19);
                setcolor(c,7);
                printf("                           ");
                gotoxy(15,19);
                printf("Connected %s",inet_ntoa(finfo[1][3].their_addr.sin_addr));
            }
            if (!finfo[1][4].InUse) {
                gotoxy(15,20);
                setcolor(d,7);
                printf("                           ");
                gotoxy(15,20);
                printf(" ");
            }
            else {
                gotoxy(15,20);
                setcolor(d,7);
                printf("                           ");
                gotoxy(15,20);
                printf("Connected %s",inet_ntoa(finfo[1][4].their_addr.sin_addr));
            }
            if (!finfo[1][5].InUse) {
                gotoxy(15,21);
                setcolor(e,7);
                printf("                           ");
                gotoxy(15,21);
                printf(" ");
            }
            else {
                gotoxy(15,21);
                setcolor(e,7);
                printf("                           ");
                gotoxy(15,21);
                printf("Connected %s",inet_ntoa(finfo[1][5].their_addr.sin_addr));
            }
        }        

   // }        
}


void batchgui() {




}


#ifdef _WIN32
void input( void *p ) {

 	nsworking = false;
	
	int c;
	while (1) {
	
	    setcolor(0,0);
	    // Char Input
        c = getch();
        	
		// Reset Working State to Continue Ansimation
		nsworking = true;		
		
		// ENTER 
		if (c == 13) {
		    if ((nstate == NodeWindow) && (nwbar == CONNECT)) {
    		    switch(nbar) {
                    // Incoming Nodes
                    case ULNode1:
                        nstate  = ULNode1;
                        pnstate = NodeWindow;
                        ansiparse(aBuf4);
                        gotoxy(54,5);
                        setcolor(7,0); printf("status of node "); setcolor(15,0); printf("1");
                        batchgui();
                        nstate  = NodeWindow;
                        pnstate = ULNode1;                        
                        break;
                    case ULNode2:
                        nstate  = ULNode2;
                        pnstate = NodeWindow;
                        ansiparse(aBuf4);
                        gotoxy(54,5);
                        setcolor(7,0); printf("status of node "); setcolor(15,0); printf("2");
                        batchgui();
                        nstate  = NodeWindow;
                        pnstate = ULNode2;
                        break;
                    case ULNode3:
                        nstate  = ULNode3;
                        pnstate = NodeWindow;
                        ansiparse(aBuf4);
                        gotoxy(54,5);
                        setcolor(7,0); printf("status of node "); setcolor(15,0); printf("3");
                        batchgui();
                        nstate  = NodeWindow;
                        pnstate = ULNode3;
                        break;
                    case ULNode4:
                        nstate  = ULNode4;
                        pnstate = NodeWindow;
                        ansiparse(aBuf4);
                        gotoxy(54,5);
                        setcolor(7,0); printf("status of node "); setcolor(15,0); printf("4");
                        batchgui();
                        nstate  = NodeWindow;
                        pnstate = ULNode4;
                        break;
                    case ULNode5:
                        nstate  = ULNode5;
                        pnstate = NodeWindow;
                        ansiparse(aBuf4);
                        gotoxy(54,5);
                        setcolor(7,0); printf("status of node "); setcolor(15,0); printf("5");
                        batchgui();
                        nstate  = NodeWindow;
                        pnstate = ULNode5;
                        break;
                }
                NodeGUI(PORT);
		    
		    }
		
		    else if ((nstate == NodeWindow) && (nwbar == SNOOP)) {
                switch(nbar) {
                    // Incoming Nodes
                    case DLNode1:
                        nstate = DLNode1;
                        pnstate = NodeWindow;
                        if (!finfo[0][1].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("1");
                        }
                        break;                        
                    case DLNode2:
                        nstate = DLNode2;
                        pnstate = NodeWindow;
                        if (!finfo[0][2].InUse) {
                            ansiparse(aBuf3);                         
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("2");
                        }
                        break;
                    case DLNode3:
                        nstate = DLNode3;
                        pnstate = NodeWindow;
                        if (!finfo[0][3].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("3");
                        }
                        break;
                    case DLNode4:
                        nstate = DLNode4;
                        pnstate = NodeWindow;
                        if (!finfo[0][4].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("4");
                        }
                        break;
                    case DLNode5:
                        nstate = DLNode5;
                        pnstate = NodeWindow;
                        if (!finfo[0][5].InUse) { 
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("5");
                        }
                        break;
                    // Outgoing Nodes
                    case ULNode1:
                        nstate = ULNode1;
                        pnstate = NodeWindow;
                        if (!finfo[1][1].InUse) { 
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("1");
                        }
                        break;                        
                    case ULNode2:
                        nstate = ULNode2;
                        pnstate = NodeWindow;
                        if (!finfo[1][2].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("2");
                        }
                        break;
                    case ULNode3:
                        nstate = ULNode3;
                        pnstate = NodeWindow;
                        if (!finfo[1][3].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("3");
                        }
                        break;
                    case ULNode4:
                        nstate = ULNode4;
                        pnstate = NodeWindow;
                        if (!finfo[1][4].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("4");
                        }
                        break;
                    case ULNode5:
                        nstate = ULNode5;
                        pnstate = NodeWindow;
                        if (!finfo[1][5].InUse) {
                            ansiparse(aBuf3);
                            gotoxy(54,5);
                            setcolor(7,0); printf("status of node "); setcolor(15,0); printf("5");
                        }
                        break;
                }
                NodeGUI(PORT);
            }
            else if ((nstate == NodeWindow) && (nwbar == EXIT)) { setcolor(7,0); exit(1); }
            else if (nwbar == SNOOP) {
                switch(nbar) {
                    // incoming Nodes
                    case DLNode1:
                        pnstate = DLNode1;
                        nstate  = NodeWindow;                        
                        break;                        
                    case DLNode2:
                        pnstate = DLNode2;
                        nstate  = NodeWindow;
                        break;
                    case DLNode3:
                        pnstate = DLNode3;
                        nstate  = NodeWindow;
                        break;
                    case DLNode4:
                        pnstate = DLNode4;
                        nstate  = NodeWindow;
                        break;
                    case DLNode5:
                        pnstate = DLNode5;
                        nstate  = NodeWindow;
                        break;
                    // outgoing nodes
                    case ULNode1:
                        pnstate = ULNode1;
                        nstate  = NodeWindow;
                        break;                        
                    case ULNode2:
                        pnstate = ULNode2;
                        nstate  = NodeWindow;
                        break;
                    case ULNode3:
                        pnstate = ULNode3;
                        nstate  = NodeWindow;
                        break;
                    case ULNode4:
                        pnstate = ULNode4;
                        nstate  = NodeWindow;
                        break;
                    case ULNode5:
                        pnstate = ULNode5;
                        nstate  = NodeWindow;
                        break;                    
                }
                NodeGUI(PORT);
            }
		}
		
		// Left / Right Selection Lightbar, Node Window!
		// Turns off lightbar! and Setup's for next Lightbar
		else if (c == 77 || c == 75) {
            if (nstate == NodeWindow) {
                switch(nwbar) {
                    case SNOOP:
                        if (c == 77)      nwbar = CONNECT;  // Left
		                else if (c == 75) nwbar = EXIT;     // Right
                        gotoxy(9,23);
                        setcolor(0,7);  printf(" snoop ");
                        ansiparse(aBuf11);
                        break;
                    case CONNECT:
                        if (c == 77)      nwbar = CONFIG;
    		            else if (c == 75) nwbar = SNOOP;
                        gotoxy(16,23);
                        setcolor(0,7);  printf(" connect ");
                        //ansiparse(aBuf11);
                        break;
                    case CONFIG:
                        if (c == 77)      nwbar = RESTART;
    		            else if (c == 75) nwbar = CONNECT;
                        gotoxy(25,23);
                        setcolor(0,7);  printf(" config ");
                        //ansiparse(aBuf11);
                        break;
                    case RESTART:
                        if (c == 77)      nwbar = EXIT;
                        else if (c == 75) nwbar = CONFIG;
                        gotoxy(33,23);
                        setcolor(0,7);  printf(" restart ");
                        //ansiparse(aBuf11);
                        break;
                    case EXIT:
                        if (c == 77)      nwbar = SNOOP;
    		            else if (c == 75) nwbar = RESTART;    		            
                        gotoxy(42,23);
                        setcolor(0,7);  printf(" exit ");
                        //ansiparse(aBuf11);
                        break;
                }                
                switch(nwbar) {
                    case SNOOP:   gotoxy(9,23);  setcolor(15,1); printf(" snoop ");   nbar = DLNode1; ansiparse(aBuf12); nsworking = false; refreshbar(); nsworking = true; break;                         
                    case CONNECT: gotoxy(16,23); setcolor(15,1); printf(" connect "); nbar = ULNode1; nsworking = false; refreshbar(); nsworking = true; break;
                    case CONFIG:  gotoxy(25,23); setcolor(15,1); printf(" config ");  nsworking = false; refreshbar(); nsworking = true; break;
                    case RESTART: gotoxy(33,23); setcolor(15,1); printf(" restart "); break;
                    case EXIT:    gotoxy(42,23); setcolor(15,1); printf(" exit ");    nsworking = false; refreshbar(); nsworking = true; break;
                }
            }
		}
  		
		// Up/Down Node Lightbar
		else if (c == 80 || c == 72) {		
            // Node Gui Lightbars Input States
		    if ((nstate == NodeWindow) && (nwbar == CONNECT)) {
		        switch(nbar) {
		            case ULNode1:
                        if (c == 80)      nbar = ULNode2;
	       	            else if (c == 72) nbar = ULNode5;
                        gotoxy(5,17);
                        if (finfo[1][1].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 1 ");
                        break;                    
                    case ULNode2:
                        if (c == 80)      nbar = ULNode3;
    		            else if (c == 72) nbar = ULNode1;
                        gotoxy(5,18);
                        if (finfo[1][2].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 2 ");
                        break;                    
                    case ULNode3:
                        if (c == 80)      nbar = ULNode4;
    		            else if (c == 72) nbar = ULNode2;
                        gotoxy(5,19);
                        if (finfo[1][3].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 3 ");
                        break;
                    case ULNode4:
                        if (c == 80)      nbar = ULNode5;
    		            else if (c == 72) nbar = ULNode3;
                        gotoxy(5,20);
                        if (finfo[1][4].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 4 ");
                        break;
                    case ULNode5:                    
                        if (c == 80)      nbar = ULNode1;
    		            else if (c == 72) nbar = ULNode4;
                        gotoxy(5,21);
                        if (finfo[1][5].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 5 ");
                        break;
                }
                switch(nbar) {
                    case ULNode1: gotoxy(5,17); 
                            if (finfo[1][1].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 1 "); 
                            break;
                    case ULNode2: gotoxy(5,18); 
                            if (finfo[1][2].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 2 "); 
                            break;
                    case ULNode3: gotoxy(5,19); 
                            if (finfo[1][3].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 3 "); 
                            break;
                    case ULNode4: gotoxy(5,20); 
                            if (finfo[1][4].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 4 "); 
                            break;
                    case ULNode5: gotoxy(5,21); 
                            if (finfo[1][5].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 5 "); 
                            break;
                
                }
		    }		
		
		    // Node Gui Lightbars Input States
		    else if ((nstate == NodeWindow) && (nwbar == SNOOP)) {  				    
                // Download Node Lightbar Input - This part clearn the lightbar and 
                // Sets the state for the following next lightbar position
                switch(nbar) {
                    case DLNode1:
       		            if (c == 80)      nbar = DLNode2;
	       	            else if (c == 72) nbar = ULNode5;
                        gotoxy(5,10);
                        if (finfo[0][1].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 1 ");
                        break;
                    case DLNode2:
    		            if (c == 80)      nbar = DLNode3;
	       	            else if (c == 72) nbar = DLNode1;
                        gotoxy(5,11);
                        if (finfo[0][2].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 2 ");
                        break;                                           
                    case DLNode3:
       		            if (c == 80)      nbar = DLNode4;
                        else if (c == 72) nbar = DLNode2;
                        gotoxy(5,12);
                        if (finfo[0][3].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 3 ");
                        break;                    
                    case DLNode4:
                        if (c == 80)      nbar = DLNode5;
    		            else if (c == 72) nbar = DLNode3;
                        gotoxy(5,13);
                        if (finfo[0][4].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 4 ");
                        break;                    
                    case DLNode5:
    		            if (c == 80)      nbar = ULNode1;
	   	                else if (c == 72) nbar = DLNode4;
                        gotoxy(5,14);
                        if (finfo[0][5].InUse) setcolor(15,7);
                        else setcolor(0,7);
                        printf(" node 5 ");
                        break;                    
                    case ULNode1:
                        if (c == 80)      nbar = ULNode2;
	       	            else if (c == 72) nbar = DLNode5;
                        gotoxy(5,17);
                        if (finfo[1][1].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 1 ");
                        break;                    
                    case ULNode2:
                        if (c == 80)      nbar = ULNode3;
    		            else if (c == 72) nbar = ULNode1;
                        gotoxy(5,18);
                        if (finfo[1][2].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 2 ");
                        break;                    
                    case ULNode3:
                        if (c == 80)      nbar = ULNode4;
    		            else if (c == 72) nbar = ULNode2;
                        gotoxy(5,19);
                        if (finfo[1][3].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 3 ");
                        break;
                    case ULNode4:
                        if (c == 80)      nbar = ULNode5;
    		            else if (c == 72) nbar = ULNode3;
                        gotoxy(5,20);
                        if (finfo[1][4].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 4 ");
                        break;
                    case ULNode5:                    
                        if (c == 80)      nbar = DLNode1;
    		            else if (c == 72) nbar = ULNode4;
                        gotoxy(5,21);
                        if (finfo[1][5].InUse) setcolor(15,7);
                        else setcolor(0,7);            
                        printf(" node 5 ");
                        break;
                }                    
                switch(nbar) {
                    case DLNode1: gotoxy(5,10); 
                            if (finfo[0][1].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 1 "); 
                            break;
                    case DLNode2: gotoxy(5,11); 
                            if (finfo[0][2].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 2 "); 
                            break;
                    case DLNode3: gotoxy(5,12); 
                            if (finfo[0][3].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 3 "); 
                            break;                    
                    case DLNode4: gotoxy(5,13); 
                            if (finfo[0][4].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 4 "); 
                            break;
                    case DLNode5: gotoxy(5,14); 
                            if (finfo[0][5].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 5 "); 
                            break;
                    case ULNode1: gotoxy(5,17); 
                            if (finfo[1][1].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 1 "); 
                            break;
                    case ULNode2: gotoxy(5,18); 
                            if (finfo[1][2].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 2 "); 
                            break;
                    case ULNode3: gotoxy(5,19); 
                            if (finfo[1][3].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 3 "); 
                            break;
                    case ULNode4: gotoxy(5,20); 
                            if (finfo[1][4].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 4 "); 
                            break;
                    case ULNode5: gotoxy(5,21); 
                            if (finfo[1][5].InUse) setcolor(15,5);
                            else setcolor(15,1); 
                            printf(" node 5 "); 
                            break;
                }                
		    }
		}
		
		// Small delay to help get rid of gui mess up when lightbar keys are held down..
		// Sleep(20);  // Leaves some anonomlities in Fullscreen only!
	    // Reset Working State to Continue Ansimation	
        nsworking = false;
		
    /*	// Test Code to get Key Inputed Interger	
		else {
		   gotoxy(10,1);
		   setcolor(15,0);
		   printf("char %i  ",c);
		
		}
		gotoxy(1,1);
		setcolor(15,0);
		printf("char %i  ",c);
		// 48 - 57 == 0 - 9
		//77 ->
		//75 <-
		*/
	}
}


#endif

/*
void NodeGUI(int nPort, SOCKET r, SOCKET l) {

    if (strcmp(finfo2[NODE].status,"") != 0) {
    
    #ifdef _WIN32
    // Date/Time Stamp log on Each Conection
    GetTimeFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, ntpsrv.szTimeFormat, 50 );
	GetDateFormat( LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, ntpsrv.szDateFormat, 50 );
	sprintf(mInit,"Date/Time: %s %s",ntpsrv.szDateFormat,ntpsrv.szTimeFormat);
    ntpsrv.logntp(mInit);
    #endif       

}
*/


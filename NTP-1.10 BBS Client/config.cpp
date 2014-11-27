#ifndef __CONFIG_CPP    /*  An extra safeguard to prevent this header from  */
#define __CONFIG_CPP    /*  being included twice in the same source file    */

#include <stdio.h>
#include <string>
#include <fstream>
#include "config.h"

using namespace std;

/****************************************************************************/
/* Checks if CONFIG.CFG file exists, if not return false                                        */
/****************************************************************************/
bool configdataexists()
{

    std::string path = PATH;
    path += "config.cfg";

    FILE *stream;
    stream = fopen ( path.c_str(),"rb+" );
    if ( stream == NULL )
    {
        return false;
    }
    fclose ( stream );
    return true;
}

/****************************************************************************/
/* Create Defauly Config File if None Exists                                                            */
/****************************************************************************/
void createconfig()
{

    std::string name = PATH;
    name += "config.cfg";

    ofstream outStream2;
    outStream2.open ( name.c_str(), ofstream::out | ofstream::trunc );
    if ( !outStream2.is_open() )
    {
        printf ( "\nError Creating: %s \n", name.c_str() );
        exit ( 1 );
        return;
    }

#ifdef _WIN32
    outStream2 << "" << endl;
    outStream2 << "# .----------------------------------------------------------------." << endl;
    outStream2 << "# | NTP-BBS v1.10b Configuration File                      (WIN32) |-------------" << endl;
    outStream2 << "# `----------------------------------------------------------------'" << endl;
#else
    outStream2 << "" << endl;
    outStream2 << "# .----------------------------------------------------------------." << endl;
    outStream2 << "# | NTP-BBS v1.10b Configuration File                      (LINUX) |-------------" << endl;
    outStream2 << "# `----------------------------------------------------------------'" << endl;
#endif
    outStream2 << "#" << endl;
    outStream2 << "#" << endl;
    outStream2 << "# .----------------------------------------------------------------." << endl;
    outStream2 << "# | Note : This file is regenerated with defaults if missing.      |" << endl;
    outStream2 << "# | Note : Any Lines with the # in them will be ignored            |" << endl;
    outStream2 << "# `----------------------------------------------------------------'" << endl;
    outStream2 << "#" << endl;
    outStream2 << "# .----------------------------------------------------------------." << endl;
    outStream2 << "# | Enable Transfer and Error Logging.. .  1 = on, 0 = off         |" << endl;
    outStream2 << "# `----------------------------------------------------------------'" << endl;
    outStream2 << "#" << endl;
    outStream2 << "Set LOG \"1\"" << endl;
    outStream2 << "#" << endl;
    outStream2 << "# .----------------------------------------------------------------." << endl;
    outStream2 << "# | Enable DSZLOG Write all Success 1 = on, 0 = off                |" << endl;
    outStream2 << "# `----------------------------------------------------------------'" << endl;
    outStream2 << "#" << endl;
    outStream2 << "Set DSZLOG \"0\"" << endl;
    outStream2 << endl;
    outStream2.close();
    return;
}

/****************************************************************************/
/* Parses Config File Data                                                                                                      */
/****************************************************************************/
void checkcfg ( std::string cfgdata )
{

    std::string temp = cfgdata;
    int id1 = -1;
    // Disgards any Config lines with the # Character
    id1 = temp.find ( '#', 0 );
    if ( id1 != -1 )
    {
        return;
    }

    // Sets the Server Name
    id1 = -1;
    id1 = temp.find ( "Set LOG", 0 );
    if ( id1 != -1 )
    {
        std::string temp1;
        int st1 = -1;
        int st2 = -1;
        signed int  ct = -1;

        st1 = temp.find ( '"', 0 );
        st2 = temp.find ( '"', st1+1 );
        ++st1;
        temp1 = temp.substr ( st1,st2 );
        ct = st2 - st1;
        if ( temp1.length() > ct )
            temp1.erase ( ct,temp1.length() );
        id1 = atoi ( temp1.c_str() );

        if ( id1 == 1 ) LOGGING = true;
        else LOGGING = false;
    }

    // Sets the Server Name
    id1 = -1;
    id1 = temp.find ( "Set DSZLOG", 0 );
    if ( id1 != -1 )
    {
        std::string temp1;
        int st1 = -1;
        int st2 = -1;
        signed int  ct = -1;

        st1 = temp.find ( '"', 0 );
        st2 = temp.find ( '"', st1+1 );
        ++st1;
        temp1 = temp.substr ( st1,st2 );
        ct = st2 - st1;
        if ( temp1.length() > ct )
            temp1.erase ( ct,temp1.length() );
        id1 = atoi ( temp1.c_str() );

        if ( id1 == 1 ) RETURN = true;
        else RETURN = false;
    }
}

/****************************************************************************/
/* Read Config File                                                                                                                     */
/****************************************************************************/
void parseconfig()
{

    std::string name = PATH;
    name += "config.cfg";

    ifstream inStream;
    inStream.open ( name.c_str() );
    if ( !inStream.is_open() )
    {
        printf ( "Coun't Open Config File For Reading: %s\n", name.c_str() );
        exit ( 1 );
    }

    std::string cfgdata;
    for ( ;; )
    {
        getline ( inStream,cfgdata );
        checkcfg ( cfgdata );
        if ( inStream.eof() ) break;
    }
    inStream.close();
    return;
}


#endif

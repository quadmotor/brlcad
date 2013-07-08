/*
** 21-Jul-1995
** This code was developed with the support of the United States Government,
** and is not subject to copyright.

** This file contains a simple little program to read in a file and
** write it back out to a new file.  The purpose is to demonstrate the
** use of a Schema Class Library.  The name of the file to be read in
** can be supplied as a command line argument.  If no argument is
** provided, the program tries to read the file testfile.stp.  The
** name of the file to be output may also be provided, if no name is
** provided the file written out is called file.out
*/

extern void SchemaInit( class Registry & );
#include <STEPfile.h>
#include <sdai.h>

int main( int argc, char * argv[] ) {
    if( argc > 3 || argc < 2 ) {
        cout << "Syntax:  p21read [infile [outfile]]" << endl;
        exit( 1 );
    }
///////////////////////////////////////////////////////////////////////////////
// You have to initialize the schema before you do anything else.
// This initializes all of the registry information for the schema you
// plan to use.  The SchemaInit() function is generated by fedex_plus.
// See the 'extern' stmt above.
//
// The registry is always going to be in memory.
//
///////////////////////////////////////////////////////////////////////////////

    Registry  registry( SchemaInit );
    InstMgr   instance_list;
    STEPfile  sfile( registry, instance_list );
    const char   *  flnm;

    cout << "\nEXAMPLE :  load file ..." << endl;
    if( argc >= 2 ) {
        flnm = argv[1];
    } else {
        flnm = "testfile.step";
    }
    sfile.ReadExchangeFile( flnm );

    cout << "EXAMPLE :  write file ..." << endl;
    if( argc == 3 ) {
        flnm = argv[2];
    } else {
        flnm = "file.out";
    }
    sfile.WriteExchangeFile( flnm );
    cout << flnm << " written"  << endl;
}
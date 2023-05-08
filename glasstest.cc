

////////////////////////////////////////////////////////////////////////////////
//  glass_dat2tree.cc
//
//  Function:   glass_dat2tree: to create a tree from a dat-file
// Last update June 20th 2017 by MK
////////////////////////////////////////////////////////////////////////////////


//======
// ROOT
//======
#include <TString.h>
#include <TFile.h>
#include <TTree.h>


//=========
// STD C++
//=========
#include <fstream> // ifstream
#include <iostream>


//=======
// STD C
//=======
#include <stdio.h> // printf


using namespace std;



void glasstest( TString filename = "", Bool_t tempMode = true, Bool_t matrixMode = true, Bool_t fakeMatrixMode = false, Bool_t oneCh = false, Bool_t sixCh = false, Double_t starttime = 0, Double_t endtime = 66666 )
{
    if( filename == "" )
    {
        printf( "Usage: glass_dat2tree_a( 160314_Matrix_r_L1_red_hpocleaned_1723diode.dat\n" );
        return;
    }


    ifstream in;
    in.open( filename );

    if( !in )
    {
        cout << "Error opening input stream" << endl;
        return;
    }


// ascii-file contains 4 columns (time, value, reference, filter)
    
    Double_t time, value, reference;
    Int_t filter; // 0: pure beam, 1: offset (no laser), 2: glass (3 and so on: something else)

    Int_t run = 0; // additional information for the ntuple

    // for matrix mode
    Double_t x_scan, y_scan;

    // for temperature mode
    Double_t temp, temp_in;

    // for channel test
    Double_t test1, test2, test3;


// prepare root-tree
    TString rootFile = filename;
    rootFile.Resize( filename.Length() - 4 ); // remove file extension .dat
    rootFile = rootFile + ".root";
    TFile *file = new TFile( rootFile, "RECREATE" );
    TTree *tree = new TTree( "tree", filename );

    tree->Branch("time"     , &time     , "time/D"); // D: Double_t
    tree->Branch("value"    , &value    , "value/D");
    tree->Branch("reference", &reference, "reference/D");
    tree->Branch("filter"   , &filter   , "filter/I");
    tree->Branch("run"      , &run      , "run/I"); // I: Int_t

    if( matrixMode )
    {
        tree->Branch("x_scan", &x_scan, "x_scan/D");
        tree->Branch("y_scan", &y_scan, "y_scan/D");
    }

    if( tempMode )
        tree->Branch("temp", &temp, "temp/D");

    if( tempMode && sixCh )
    {
        tree->Branch("test1", &test1, "test1/D");
        tree->Branch("test2", &test2, "test2/D");
        tree->Branch("test3", &test3, "test3/D");
    }


    Int_t nlines = 0;

    Int_t firstFilter = 0;

    // for matrix mode
    Double_t first_x_scan = 0;
    Double_t first_y_scan = 0;

    // for counting measurement runs
    Bool_t onlyonce             = true;
    Bool_t onlyonce_firstFilter = true;
    Bool_t onlyonce_others      = true;


// fill ntuple
    while ( true )
    {
//         if( matrixMode && tempMode && !sixCh )
//             in >> time >> value >> reference >> temp >> filter >> x_scan >> y_scan;
//         else if( matrixMode && !sixCh )
//             in >> time >> value >> reference >> filter >> x_scan >> y_scan;
//         else if( tempMode && !sixCh )
//             in >> time >> value >> reference >> temp >> filter;
//
//         else if( tempMode && sixCh && matrixMode )
//             in >> time >> value >> reference >> temp >> test1 >> test2 >> test3 >> filter >> x_scan >> y_scan;
//         else if( tempMode && sixCh )
//             in >> time >> value >> reference >> temp >> test1 >> test2 >> test3 >> filter;
//         else
//             in >> time >> value >> reference >> filter;

		in >> time >> value;
		if( oneCh )
			in >> filter;
		else
			in >> reference;
        if( tempMode && !oneCh )
		{
			in >> temp_in;
			temp = temp_in*100;
		} 
       if( sixCh && !oneCh )
			in >> test1 >> test2 >> test3 >> filter;
        else if ( !oneCh )
			in >> filter;
		if( matrixMode || fakeMatrixMode )
			in >> x_scan >> y_scan;

        if( !in.good() )
            break;


        if( onlyonce )
        {
            firstFilter = filter;

            if( matrixMode )
            {
                first_x_scan = x_scan;
                first_y_scan = y_scan;
            }


            onlyonce = false;
        }


        if( filter == firstFilter && onlyonce_firstFilter && !matrixMode )
        {
            onlyonce_firstFilter    = false;
            onlyonce_others         = true;
            run++;
        }

        if( filter != firstFilter && onlyonce_others && !matrixMode )
        {
            onlyonce_others         = false;
            onlyonce_firstFilter    = true;
        }


        if( x_scan == first_x_scan && y_scan == first_y_scan && onlyonce_firstFilter && matrixMode )
        {
            onlyonce_firstFilter    = false;
            onlyonce_others         = true;
            run++;
        }

        if( ( x_scan != first_x_scan || y_scan != first_y_scan ) && onlyonce_others && matrixMode )
        {
            onlyonce_others         = false;
            onlyonce_firstFilter    = true;
        }
        
        if( value > 9.5 )
            value = 9.5;
        if( value < -9.5 )
            value = -9.5;
        if( value > 9.5 || value < -9.5 )
            cout << "old moni program; set maximum to |9.5| V" << endl;


        if ( nlines < 5 )
        {
            if( matrixMode && tempMode && !sixCh )
                printf( "time=%9f, value=%9f, reference=%9f, temp=%9f, filter=%4i, run=%3i, x_scan=%6f, y_scan=%5f\n",
                        time, value, reference, temp, filter, run, x_scan, y_scan );
            else if( matrixMode && !sixCh )
                printf( "time=%9f, value=%9f, reference=%9f, filter=%4i, run=%3i, x_scan=%6f, y_scan=%5f\n",
                        time, value, reference, filter, run, x_scan, y_scan );
            else if( tempMode && !sixCh )
                printf( "time=%9f, value=%9f, reference=%9f, temp=%9f, filter=%4i, run=%3i\n",
                        time, value, reference, temp, filter, run );
            else if( oneCh )
                printf( "time=%9f, value=%9f, filter=%4i, run=%3i\n", time, value, filter, run );
            else if( tempMode && sixCh && matrixMode )
                printf( "time=%9f, value=%9f, reference=%9f, temp=%9f, test1=%9f, test2=%9f, test3=%9f,filter=%4i, run=%3i, x_scan=%6f, y_scan=%5f\n",
                        time, value, reference, temp, test1, test2, test3, filter, run, x_scan, y_scan );
            else  if( tempMode && sixCh )
                printf( "time=%9f, value=%9f, reference=%9f, temp=%9f, test1=%9f, test2=%9f, test3=%9f,filter=%4i, run=%3i\n",
                        time, value, reference, temp, test1, test2, test3, filter, run );
            else
                printf( "time=%9f, value=%9f, reference=%9f, filter=%4i, run=%3i\n", time, value, reference, filter, run );
        }


        if( starttime == 0 && endtime == 666666 )
            tree->Fill();
        else
        {
            if( time > starttime && time < endtime )
                tree->Fill();
        }

        nlines++;
        cout << nlines << '\r';
    }

    printf( "found %d lines\n", nlines );
    cout << "Root-file \"" << rootFile << "\" was written" << endl;


    in.close();
    file->Write();
}

#include <TH2.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TString.h>
#include <TFile.h>
#include <TTree.h>
#include <TMath.h>
#include <TCanvas.h>
#include <TStyle.h> 
#include <TAxis.h>
#include <iostream>
#include <iomanip>
using namespace std;
class Wavelength{
	public:
		enum Color{ uv = 1, lightuv = 6, blue = 2, lightblue = 5, green = 3, red = 4};
};
void matrix(TString filename = "", Int_t LaserColor = 0, Int_t xbias_low = 0, Int_t xbias_up = 0, Int_t ybias_low = 0, Int_t ybias_up = 0, Bool_t mirrorCor = false){
	gSystem->Load( "/u/pandadrc/MK/MK_data_lxg0347/darklab/lib/libCommon_53412_g++4.4.5.so" ); // load MyTMath and MyTGraphOpt libraries
	if( filename == "" ){
		cout << "Wrong Usage !!!" << endl;    
		return;
	}
	if( mirrorCor ) cout << "Reflection transmission measurement" << endl;
	else cout << "Bulk transmission measurement" << endl;

	TString rootFile = filename + ".root";
	TFile *file = new TFile( rootFile );
	TTree *tree = (TTree*)file->Get("tree");
	Double_t time, value, reference, temp;
	Double_t x_scan, y_scan;
	Int_t filter;
	tree->SetBranchAddress( "time"      , &time );
	tree->SetBranchAddress( "value"     , &value );
	tree->SetBranchAddress( "reference" , &reference );
	tree->SetBranchAddress( "filter"    , &filter );
	tree->SetBranchAddress( "temp"      , &temp );
	tree->SetBranchAddress( "x_scan"    , &x_scan );
	tree->SetBranchAddress( "y_scan"    , &y_scan );
	Int_t nEntries_helper = tree->GetEntries();
	const Int_t nEntries = nEntries_helper;

	tree->GetEntry( nEntries - 1 ); // last entry
	Double_t lastTime = time;
	tree->GetEntry( 0 ); // first entry
	Double_t totalTime = lastTime - time;
	cout << "Measurement period: " << totalTime << endl;
	cout << "Number of Entries: " << nEntries << endl;

	const Int_t pureBeam    = 0;
	const Int_t offset      = 1;
	const Int_t withGlass   = 2;
	const Int_t nMode       = 3;

	Double_t step;
	cout << " x_min = " << tree->GetMinimum("x_scan") << " x_max = " << tree->GetMaximum("x_scan")<< "\t\t"<<  "<br />"  << endl;
	cout << " y_min = " << tree->GetMinimum("y_scan") << " y_max = " << tree->GetMaximum("y_scan")<< "\t\t"<<  "<br />"  << endl;
	cout << "The distance of steps used in measurements (in mm): " ;
	cin >> step;
	cout << endl; 

	// x_range and y_range for histogram
	Double_t xlow   = tree->GetMinimum("x_scan")+xbias_low;
	Double_t ylow   = tree->GetMinimum("y_scan")+ybias_low;
	Double_t xup    = tree->GetMaximum("x_scan")+step-xbias_up;
	Double_t yup    = tree->GetMaximum("y_scan")+step-ybias_up;
	Int_t nbinsx    = (xup - xlow)/step;
	Int_t nbinsy    = (yup - ylow)/step;
	cout << endl << "number of columns (x): " << nbinsx << ", number of rows (y): " << nbinsy << endl;

	// real scan values for motor positions (ranges)
	Double_t x_scanMin =   xlow;
	Double_t y_scanMin =   ylow;
	Double_t x_scanMax =   xup-step;
	Double_t y_scanMax =   yup-step;
	cout << endl << "xmin " << x_scanMin << ", xmax" << x_scanMax << endl;
	cout << endl << "ymin " << y_scanMin << ", ymax" << y_scanMax << endl;

	Double_t centerX = 242.5;
	Double_t centerY = 13.;

	Double_t norm_profile = 1.0;//1.07393;//1.07808;//1.; 

	// ###############################    WAVELENGTH OF LASER -> COLOR ################################# 
	TString waviColor_mat_str = filename + ".pdf";
	switch( LaserColor ){
		case 266:
			Wavelength::Color waviColor = Wavelength::uv;
			waviColor_str = "uv";
			break;
		case 325:
			Wavelength::Color waviColor = Wavelength::lightuv;
			waviColor_str = "lightuv";
			break;
		case 405:
			Wavelength::Color waviColor = Wavelength::blue;
			waviColor_str = "blue";
			break;
		case 442:
			Wavelength::Color waviColor = Wavelength::lightblue;
			waviColor_str = "lightblue";
			break;
		case 532:
			Wavelength::Color waviColor = Wavelength::green;
			waviColor_str = "green";
			break;
		case 635:
			Wavelength::Color waviColor = Wavelength::red;
			waviColor_str = "red";
			break;
		default:
			cout << "wrong wavelength color!!!!!!" << endl;
			break;
	}

	Bool_t materialFresnelQuartz = true;
	TString materialFresnelType_str = "";

	Bool_t multiRefl = true; 
	Bool_t miscCutPlot = false;
	Bool_t defectCutPlot = true;
	Bool_t fresnelPlot = true;
	Bool_t profile = false;
	Bool_t rangePlot = true;
	Bool_t uniformity = true;

	if( mirrorCor ) fresnelPlot = false;

	Bool_t defect[ nEntries ] = { 0 }; 
	Bool_t misc[ nEntries ] = { 0 };

	for( int i = 0; i < nEntries; i++ ){
		tree->GetEntry( i );
		misc[ i ] = false;
	}

	TString   title             = filename + ": transmission (matrix)";
	TString   titleErr          = filename + ": transmission (matrix, uncertainties)";
	TString   titleHist         = filename + ": transmission (full range)";
	TString   titleHist2        = filename + ": transmission (observed range)";
	TString   titleHistErr      = filename + ": transmission (full range, uncertainty)";
	TString   titleHistErr2     = filename + ": transmission (observed range, uncertainty)";
	TString   titleHistErrStat  = filename + ": transmission (full range, stat. uncertainty)";
	TString   titleHistErrStat2 = filename + ": transmission (observed range, stat. uncertainty)";
	TString   titleHistErrSys   = filename + ": transmission (full range, sys. uncertainty)";
	TString   titleHistErrSys2  = filename + ": transmission (observed range, sys. uncertainty)";

	TH2F * matrix       = new TH2F( "matrix"    , title    , nbinsx, xlow, xup, nbinsy, ylow, yup );
	TH2F * error        = new TH2F( "error"     , titleErr , nbinsx, xlow, xup, nbinsy, ylow, yup );
	//plotrange
	TH1F * transHistoRange         = new TH1F( "transHistoRange"        , titleHist        , 100, 0    ,    1.3 );
	TH1F * transHistoMatrix        = new TH1F( "transHistoMatrix"       , titleHist2       , 200, 0.97 ,   1.02 );
	TH1F * transHistoRangeErr      = new TH1F( "transHistoRangeErr"     , titleHistErr     ,  50, 0    , 0.0005 );
	TH1F * transHistoMatrixErr     = new TH1F( "transHistoMatrixErr"    , titleHistErr2    ,  50, 0    , 0.0005 );
	TH1F * transHistoRangeErrStat  = new TH1F( "transHistoRangeErrStat" , titleHistErrStat ,  50, 0    , 0.0005 );
	TH1F * transHistoMatrixErrStat = new TH1F( "transHistoMatrixErrStat", titleHistErrStat2,  50, 0    , 0.0005 );
	TH1F * transHistoRangeErrSys   = new TH1F( "transHistoRangeErrSys"  , titleHistErrSys  ,  50, 0    , 0.0005 );
	TH1F * transHistoMatrixErrSys  = new TH1F( "transHistoMatrixErrSys" , titleHistErrSys2 ,  50, 0    , 0.0005 );

	TGraph *plot_pureBeam_val    = new TGraph();
	TGraph *plot_pureBeam_ref    = new TGraph();
	TGraph *plot_pureBeam_ratio  = new TGraph();
	TGraph *plot_glassBeam_val   = new TGraph();
	TGraph *plot_glassBeam_ref   = new TGraph();
	TGraph *plot_glassBeam_ratio = new TGraph();
	// temporair GS
	TGraph *plot_glassBeam_temp      = new TGraph();
	TGraph *plot_glassBeam_valtemp   = new TGraph();
	TGraph *plot_glassBeam_reftemp   = new TGraph();
	TGraph *plot_glassBeam_ratiotemp = new TGraph();
	TGraph *plot_double_ratio_time   = new TGraph();

	//==============================================================================
	// mean value calculation
	//==============================================================================
	Double_t offsetMean_val;
	Double_t offsetMean_val_err;
	switch( waviColor ){
		case Wavelength::uv:
			offsetMean_val = -0.00014; // 266nm, S1227-1010BQ
			offsetMean_val_err = 0.00034;
			waviColor_str = "uv";
			break;
		case Wavelength::blue:
			offsetMean_val = -0.00016; // 405nm, S1227-1010BR   previously: -0.00016  (R.Hohler) 2015 offset meas.: -0.00108
			offsetMean_val_err = 0.00034;
			waviColor_str = "blue";
			break;
		case Wavelength::green:
			offsetMean_val = -0.00009; // 532nm, S1723-06
			offsetMean_val_err = 0.00037;
			waviColor_str = "green";
			break;
		case Wavelength::red:
			offsetMean_val = -0.00009; // 635nm, S1723-06
			offsetMean_val_err = 0.00037;
			waviColor_str = "red";
			break;
		default:
			cout << "wrong wavelength color!" << endl;
			break;
	}

	Double_t offsetMean_ref = +0.00073; // S1227-1010BR
	Double_t offsetMean_ref_err = 0.00034;

	cout << "offset value    " << " (" << waviColor_str << "): " << offsetMean_val << " +/- " << offsetMean_val_err << endl;
	cout << "offset          " << " (" << waviColor_str << "): " << offsetMean_ref << " +/- " << offsetMean_ref_err << endl;

	const Int_t dimX = nbinsx;
	const Int_t dimY = nbinsy;

	Double_t factorX = nbinsx / ( xup - xlow ) ; // to get integer for the array indices (rounding issue: add later a small value like 0.01)
	Double_t factorY = nbinsy / ( yup - ylow );
	cout << "nbinsx = " << nbinsx<< " factorX = " << factorX << "\t\t"<<  "<br />" << endl;
	Int_t nPerRun[ nMode ][ dimX ][ dimY ] = { 0 };
	Int_t nPerRunMax = 0;

	for( int i = 0; i < nEntries; i++ ){
		tree->GetEntry( i );
		if( x_scan >= xlow && y_scan >= ylow && x_scan < xup && y_scan < yup ){
			nPerRun[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ]++;
			if( nPerRun[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ] > nPerRunMax )
				nPerRunMax = nPerRun[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ];
		}
	}

	const Int_t nMax = nPerRunMax;

	Double_t ratio[ nMode ][ dimX ][ dimY ][ nMax ] = { 0 };
	Double_t ratio_num[ nMode ][ dimX ][ dimY ][ nMax ] = { 0 };
	Double_t ratio_den[ nMode ][ dimX ][ dimY ][ nMax ] = { 0 };
	Double_t ratioErr[ nMode ][ dimX ][ dimY ][ nMax ] = { 0 };

	Int_t n_helper    = 0;
	Int_t nPixel      = 0;
	Int_t n_pureBeam  = 0;
	Int_t n_glassBeam = 0;
	//GS
	Int_t n_double_ratio	= 0;
	//

	//   Double_t test_err_ref[ nMode ][ dimX ][ dimY ][ nMax ] = {0};


	for( int i = 0; i < nEntries; i++ )
	{
		tree->GetEntry( i );

		if( x_scan >= xlow && y_scan >= ylow && x_scan < xup && y_scan < yup )
		{
			ratio[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ][ n_helper ] =
				( value - offsetMean_val ) / ( reference - offsetMean_ref );
			ratio_num[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ][ n_helper ] =
				( value - offsetMean_val );
			ratio_den[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ][ n_helper ] =
				( reference - offsetMean_ref );
			ratioErr[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ][ n_helper ] =
				MyTMath::ErrPro_AoverB( value - offsetMean_val, offsetMean_val_err, reference - offsetMean_ref, offsetMean_ref_err );

			//       test_err_ref[filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ][ n_helper ] = reference;

			n_helper++; 

			if( nPerRun[ filter ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ] == n_helper )
			{ 
				n_helper = 0;

				if( filter == withGlass && x_scan >= x_scanMin && x_scan <= x_scanMax && y_scan >= y_scanMin && y_scan <= y_scanMax && !misc[i] && !defect[i] )  //filter == withGlass
					nPixel++;
			}

			if( filter == pureBeam )
			{
				plot_pureBeam_val   ->SetPoint( n_pureBeam, time, value - offsetMean_val );
				plot_pureBeam_ref   ->SetPoint( n_pureBeam, time, reference - offsetMean_ref );
				plot_pureBeam_ratio ->SetPoint( n_pureBeam, time, ( value - offsetMean_val ) / ( reference - offsetMean_ref ) );
				n_pureBeam++;
			}

			if( filter == withGlass )
			{
				plot_glassBeam_val   ->SetPoint( n_glassBeam, time, value - offsetMean_val );
				plot_glassBeam_ref   ->SetPoint( n_glassBeam, time, reference - offsetMean_ref );
				plot_glassBeam_ratio ->SetPoint( n_glassBeam, time, ( value - offsetMean_val ) / ( reference - offsetMean_ref ) );

				//temporair   GS 08.07.2019 
				plot_glassBeam_temp   	 ->SetPoint( n_glassBeam, time, temp );
				plot_glassBeam_valtemp   ->SetPoint( n_glassBeam, temp, value - offsetMean_val );
				plot_glassBeam_reftemp   ->SetPoint( n_glassBeam, temp, reference - offsetMean_ref );
				plot_glassBeam_ratiotemp ->SetPoint( n_glassBeam, time, ( value - offsetMean_val ) / temp );



				n_glassBeam++;
			}
		}
	}


	Double_t ratio_perRun[ nMode ][ dimX ][ dimY ] = { 0 };
	Double_t ratioErr_perRun[ nMode ][ dimX ][ dimY ]= { 0 };
	Double_t ratioErrStat_perRun[ nMode ][ dimX ][ dimY ]= { 0 };
	Double_t ratioErrSys_perRun[ nMode ][ dimX ][ dimY ]= { 0 };

	Double_t trans_perRun[ dimX ][ dimY ] = { 0 };
	Double_t transErr_perRun[ dimX ][ dimY ] = { 0 };
	Double_t transErrStat_perRun[ dimX ][ dimY ] = { 0 };
	Double_t transErrSys_perRun[ dimX ][ dimY ] = { 0 };

	Double_t sysErr_helper = 0;

	//   Double_t test_err[ nMode ][ dimX ][ dimY ] ={0};


	// brewster mirror corrections
	Double_t corFac = 1;
	Double_t corFac_errStat = 0;
	Double_t corFac_errSys = 0;
	Double_t corFac_errSys2 = 0;

	if( mirrorCor )
	{
		switch( waviColor )
		{
			case Wavelength::uv:
				corFac     = 1.;//0.952951724;//1.00;//0.9548636;//1.00;//0.94682;//1.00; //1.07131;//1.02313 ;//1.07131;//1.00; // 266 nm //02.03.2020, GS with values of MK 1.02313 / new value GS 23.02.2021 1.07131 (uV pol cube wrong?)/ 25.20.2021 right UV-polcube position Avrg of 3 : 0.946822999999999859//New allignment and UV-filter:16.11.2021:0.954863597735063574 (maximal closed diaphragms ->0.952951724 average of 5 measurements at 1/4 opend diaphragms
				corFac_errStat = 0.00000;
				corFac_errSys2  = 0.00000;
				waviColor_str = "uv";
				break;
			case Wavelength::lightuv: //02.03.2020, GS with values of MK
				corFac     = 1.03645846;//with two mirrors 09.08.2021//1.03817; // 325 nm Marvin 1.01354 , new value GS 18.02.2021 1.07536 (average of three) 02.03.2021 NEW ALIGNMENT 1.03817!!)082021at brewster Mcorr325nm=1.03645846
				corFac_errStat = 0.00000;
				corFac_errSys2  = 0.00000;
				waviColor_str = "lightuv";
				break;
			case Wavelength::blue:
				//         corFac     = 1.00116; // 405 nm
				corFac     = 0.98172; // 405 nm 0.99835 from Marvin , 0.98098 new at 116.15 degree motor 5, 0.98172 new at 117.117 Brewster calculated
				corFac_errStat = 0.00000;
				corFac_errSys2 = 0.00064; // included halo effect
				waviColor_str = "blue";
				break;
			case Wavelength::lightblue:
				//         corFac     = 1.00116; // 442 nm  
				corFac     = 0.97481089;//0.97579;//0.9753148; //0.97481089;//brewster 1117.148//0.9753148;//again 117.27//0.9750675; //last and actual with high stat and two mirrors and moved motor at 117.27deg // with moved motor and beam devider 0.97579; // 442 nm  Marvin 0.98173, new value GS 17.02.2021  0.98215, new alignment open diaphr 02.03.2021 0.97525/new aligned closed diaphr 0.979941 /
				//new alignment open diaphr repeat 0.97509 (average open diaphr = 0.97517)/08.03.2021 new alignment half open diaphragm 0.97579//06.08.2021 at Brewster mcorr442nm=0.97481089
				corFac_errStat = 0.00000;
				corFac_errSys2 = 0.00064; // included halo effect
				waviColor_str = "lightblue";
				cout << "mirror correction for 442nm" << endl;
				break;
			case Wavelength::green:
				corFac     =  0.959840;//0.963343289;// 1.00;// 0.96218;// 0.96878; // 532 nm // 0.962176828857607469 14.10.2021 (long term on)///0.963344732912672863 14.10.2021 (short term on) // 80min  ?? 0.963400418115781476//  on 0.963948332369384930  // 1day on 0.963846131603550926///Average of 5 measurements of 14.10.2021 = 0.963343289 // new green laser at 116.3 degre=0.959840282576979065 (at Brewster=0.960144405718620142) 23.05.2022
				corFac_errStat = 0.00000;
				corFac_errSys2 = 0.00084; // included halo effect
				waviColor_str = "green";
				break;
			case Wavelength::red:
				corFac     = 0.95;//1.;//0.95227; // 635 nm //GS 26.11.2021 : 0.950922870645961926 (more measurements with deviations to be calculated 0.95 placeholder)
				corFac_errStat = 0.00000;
				corFac_errSys2 = 0.00052; // included halo effect
				waviColor_str = "red";
				break;
			default:
				cout << "wrong wavelength color!" << endl;
				break;
		}




		cout << "mirror correction: " << corFac << " (stat.)" << corFac_errStat << " (sys.)" << corFac_errSys << " (" << waviColor_str << ")"<< "\t\t"<<  "<br />"  << endl;
	}

	for( int i = 0; i < dimX; i++ )
	{
		for( int j = 0; j < dimY; j++ )
		{ 

			// Hang Qi //////////
			Double_t qh_num = TMath::Mean( nPerRun[ withGlass ][ i ][ j ] , ratio_num[ withGlass ][ i ][ j ] );
			Double_t qh_den = TMath::Mean( nPerRun[ withGlass ][ i ][ j ] , ratio_den[ withGlass ][ i ][ j ] );
			ratio_perRun[ withGlass ][ i ][ j ] = qh_num / qh_den;
			// Hang Qi //////////

			// old //////////////
			//ratio_perRun[ withGlass ][ i ][ j ] = TMath::Mean( nPerRun[ withGlass ][ i ][ j ] , ratio[ withGlass ][ i ][ j ] );
			// old /////////////


			ratioErrStat_perRun[ withGlass ][ i ][ j ] =
				MyTMath::RMS_square( nPerRun[ withGlass ][ i ][ j ], ratioErr[ withGlass ][ i ][ j ] );

			sysErr_helper = TMath::Power( MyTMath::RMS_mean( nPerRun[ withGlass ][ i ][ j ], ratio[ withGlass ][ i ][ j ] ), 2 ) -
				TMath::Power( ratioErrStat_perRun[ withGlass ][ i ][ j ], 2 );
			if( sysErr_helper < 0 )
				sysErr_helper = 0;
			ratioErrSys_perRun[ withGlass ][ i ][ j ] = TMath::Sqrt( nPerRun[ withGlass ][ i ][ j ] * sysErr_helper );

			ratioErr_perRun[ withGlass ][ i ][ j ] =
				TMath::Sqrt( TMath::Power( ratioErrStat_perRun[ withGlass ][ i ][ j ], 2 ) + TMath::Power( ratioErrSys_perRun[ withGlass ][ i ][ j ], 2 ) );

			//       test_err[ pureBeam ][ i ][ j ] = MyTMath::RMS_single( nPerRun[ pureBeam ][ i ][ j ], test_err_ref[ pureBeam ][ i ][ j ] );
			//       if( test_err[ pureBeam ][ i ][ j ] > 0.002 )
			//       cout << "pureBeam (" << i*2+235 << "," << j*2+17 << "): " << test_err[ pureBeam ][ i ][ j ] << endl;
			//
			//       test_err[ withGlass ][ i ][ j ] = MyTMath::RMS_single( nPerRun[ withGlass ][ i ][ j ], test_err_ref[ withGlass ][ i ][ j ] );
			//       if( test_err[ withGlass ][ i ][ j ] > 0.002 )
			//       cout << "withGlass (" << i*2+235 << "," << j*2+17 << "): " << test_err[ withGlass ][ i ][ j ] << endl;


			if( !profile )
			{

				// Hang Qi ///////////////
				Double_t qh1_num = TMath::Mean( nPerRun[ pureBeam ][ i ][ j ] , ratio_num[ pureBeam ][ i ][ j ] );
				Double_t qh1_den = TMath::Mean( nPerRun[ pureBeam ][ i ][ j ] , ratio_den[ pureBeam ][ i ][ j ] );
				ratio_perRun[ pureBeam ][ i ][ j ] = qh1_num / qh1_den;
				// Hang Qi //////////////

				// old //////////////////
				//ratio_perRun[ pureBeam ][ i ][ j ] = TMath::Mean( nPerRun[ pureBeam ][ i ][ j ] , ratio[ pureBeam ][ i ][ j ] );
				// old /////////////////



				ratioErrStat_perRun[ pureBeam ][ i ][ j ] =
					MyTMath::RMS_square( nPerRun[ pureBeam ][ i ][ j ], ratioErr[ pureBeam ][ i ][ j ] );

				sysErr_helper = TMath::Power( MyTMath::RMS_mean( nPerRun[ pureBeam ][ i ][ j ], ratio[ pureBeam ][ i ][ j ] ), 2 ) -
					TMath::Power( ratioErrStat_perRun[ pureBeam ][ i ][ j ], 2 );
				if( sysErr_helper < 0 )
					sysErr_helper = 0;
				ratioErrSys_perRun[ pureBeam ][ i ][ j ] = TMath::Sqrt( nPerRun[ pureBeam ][ i ][ j ] * sysErr_helper );

				ratioErr_perRun[ pureBeam ][ i ][ j ] =
					TMath::Sqrt( TMath::Power( ratioErrStat_perRun[ pureBeam ][ i ][ j ], 2 ) + TMath::Power( ratioErrSys_perRun[ pureBeam ][ i ][ j ], 2 ) );


				trans_perRun[ i ][ j ] = ratio_perRun[ withGlass ][ i ][ j ] / ratio_perRun[ pureBeam ][ i ][ j ];
				transErr_perRun[ i ][ j ] =
					MyTMath::ErrPro_AoverB( ratio_perRun[ withGlass ][ i ][ j ], ratioErr_perRun[ withGlass ][ i ][ j ],
							ratio_perRun[ pureBeam ][ i ][ j ], ratioErr_perRun[ pureBeam ][ i ][ j ] );
				transErrStat_perRun[ i ][ j ] =
					MyTMath::ErrPro_AoverB( ratio_perRun[ withGlass ][ i ][ j ], ratioErrStat_perRun[ withGlass ][ i ][ j ],
							ratio_perRun[ pureBeam ][ i ][ j ], ratioErrStat_perRun[ pureBeam ][ i ][ j ] );
				transErrSys_perRun[ i ][ j ] =
					MyTMath::ErrPro_AoverB( ratio_perRun[ withGlass ][ i ][ j ], ratioErrSys_perRun[ withGlass ][ i ][ j ],
							ratio_perRun[ pureBeam ][ i ][ j ], ratioErrSys_perRun[ pureBeam ][ i ][ j ] );

				trans_perRun[ i ][ j ] = trans_perRun[ i ][ j ] * corFac;
				transErr_perRun[ i ][ j ] =
					MyTMath::ErrPro_AtimesB( trans_perRun[ i ][ j ], transErr_perRun[ i ][ j ], corFac,
							TMath::Sqrt( TMath::Power( corFac_errStat, 2 ) + TMath::Power( corFac_errSys, 2 ) ) );
				transErrStat_perRun[ i ][ j ] =
					MyTMath::ErrPro_AtimesB( trans_perRun[ i ][ j ], transErrStat_perRun[ i ][ j ], corFac, corFac_errStat );
				transErrSys_perRun[ i ][ j ] =
					MyTMath::ErrPro_AtimesB( trans_perRun[ i ][ j ], transErrSys_perRun[ i ][ j ], corFac, corFac_errSys );

				if( uniformity )
				{
					trans_perRun[ i ][ j ] = trans_perRun[ i ][ j ] / norm_profile;
					transErr_perRun[ i ][ j ] = transErr_perRun[ i ][ j ] / norm_profile;
					transErrStat_perRun[ i ][ j ] = transErrStat_perRun[ i ][ j ] / norm_profile;
					transErrSys_perRun[ i ][ j ] = transErrSys_perRun[ i ][ j ] / norm_profile;
				}

			}
			else
			{
				trans_perRun[ i ][ j ] = ratio_perRun[ withGlass ][ i ][ j ] / norm_profile;
				transErr_perRun[ i ][ j ] = ratioErr_perRun[ withGlass ][ i ][ j ] / norm_profile;
				transErrStat_perRun[ i ][ j ] = ratioErrStat_perRun[ withGlass ][ i ][ j ] / norm_profile;
				transErrSys_perRun[ i ][ j ] = ratioErrSys_perRun[ withGlass ][ i ][ j ] / norm_profile;
			}
		}
	}


	if( materialFresnelQuartz )
		materialFresnelType_str = "quartz";
	else
		materialFresnelType_str = "quartz plates + plexi";

	Double_t fresnel = 0;

	// RECALCULATED ALL FRESNEL CORRECTION VALUES ON JAN. 27TH 2018 ... NOW THEY ARE CORRECT FOR ALL WAVELENGTHS

	switch( waviColor )
	{
		case Wavelength::uv:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.07674; // 266 nm
				else
					fresnel = 0.078216;
			}
			else
				fresnel = 0.078216;
			waviColor_str = "uv";
			break;

		case Wavelength::lightuv:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.073799; // 325 nm
				else
					fresnel = 0.073799; // 325 nm
			}
			else
				fresnel = 0.073799;
			waviColor_str = "lightuv";
			break;

		case Wavelength::blue:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.06968; // 405 nm
				else
					fresnel = 0.070897; // 405 nm
			}
			else
				fresnel = 0.070897;
			waviColor_str = "blue";
			break;

		case Wavelength::lightblue:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.07013; // 442 nm
				else
					fresnel = 0.0700867; // 442 nm
			}
			else
				fresnel = 0.0700867;
			waviColor_str = "lightblue";
			break;

		case Wavelength::green:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.06763; // 532 nm
				else
					fresnel = 0.068771;
			}
			else
				fresnel = 0.068771;
			wavicolor_str = "green";
			break;
		case Wavelength::red:
			if( materialFresnelQuartz )
			{
				if( multiRefl )
					fresnel = 0.06678; // 635 nm
				else
					fresnel = 0.067891;
			}
			else
				fresnel = 0.067891;
			waviColor_str = "red";
			break;
		default:
			cout << "wrong wavelength color!" << endl;
			break;
	}

	if( fresnelPlot )
	{
		cout << "fresnel: " << fresnel << " (" << waviColor_str << ", " << materialFresnelType_str;
		if( multiRefl )
			cout << ", multiRefl";
		cout << ")" << endl;
	}

	cout << "number of pixel: " << nPixel <<  "\t\t"<<  "<br />" << endl;

	Double_t maxProfile = 0;
	Bool_t profileCout = true;

	const Int_t nPix = nPixel;

	Double_t trans_arr[ nPix ] = { 0 };
	Double_t transErr_arr[ nPix ] = { 0 };
	Double_t transErrStat_arr[ nPix ] = { 0 };
	Double_t transErrSys_arr[ nPix ] = { 0 };

	Double_t transAll = 0;
	Double_t transAllErr = 0;
	Double_t transAllErrStat = 0;
	Double_t transAllErrSys = 0;

	n_helper = 0;
	Int_t n_sum = 0;

	Double_t profile_sumAll = 0;
	Double_t profile_sumRange = 0;

	for( int i = 0; i < nEntries; i++ )
	{
		tree->GetEntry( i );

		if ( filter == withGlass && x_scan >= xlow && y_scan >= ylow && x_scan < xup && y_scan < yup )
		{
			n_helper++;

			if( nPerRun[ withGlass ][ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ] == n_helper )
			{
				n_helper = 0;

				transAll = trans_perRun[ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ];
				transAllErr = transErr_perRun[ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ];
				transAllErrStat = transErrStat_perRun[ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ];
				transAllErrSys = transErrSys_perRun[ (int) ( (x_scan - xlow) * factorX + 0.01 ) ][ (int) ( (y_scan - ylow) * factorY + 0.01 ) ];




				Double_t trans_profile = transAll;
				Double_t trans_profile_err = transAllErr;

				if( profile )
				{
					if( transAll < ( 0.0001 / norm_profile ) )
					{
						//             trans_profile = 0.000001;
						//             trans_profile_err = 0.000001;
						trans_profile = 0.0001;
						trans_profile_err = 0.0001;

						if( profileCout )
						{
							cout << "offset produces negative values: set to " << trans_profile << endl;
							profileCout = false;
						}
					}

					//           // for 100M
					//           if( y_scan == 56 && trans_profile > 0.002 )
					//           {
					//               trans_profile = 0.0005;
					//               cout << "wiggle effect" << endl;
					//           }

					if( maxProfile < trans_profile )
						maxProfile = trans_profile;

					matrix->Fill( x_scan + 0.01, y_scan + 0.01, trans_profile );
					error->Fill( x_scan + 0.01, y_scan + 0.01, trans_profile_err );

					profile_sumAll += trans_profile;
				}
				else
				{ 
					if( (defectCutPlot || !defect[i]) && (miscCutPlot || !misc[i] ) )
					{
						if( fresnelPlot )
						{
							//                 if( y_scan != 18 )
							matrix->Fill( x_scan + 0.01, y_scan + 0.01, transAll + fresnel );


							//                 else
							//                     matrix->Fill( x_scan + 0.01, y_scan + 0.01, transAll + fresnel+(0.025*260/x_scan) );
						}
						else
							// ++++++++++++++++++++++++++++ profile statement ++++++++++++++++++++++++++++++++++++

							//if(transAll>1e-4)
							matrix->Fill( x_scan + 0.01, y_scan + 0.01, transAll );
						error->Fill( x_scan + 0.01, y_scan + 0.01, transAllErr );
					} 
				}
				//GS temporär
				plot_double_ratio_time	 ->SetPoint( n_double_ratio, time, transAll );
				n_double_ratio++;
				// End GS

				transHistoRange       ->Fill( transAll );
				transHistoRangeErr    ->Fill( transAllErr );
				transHistoRangeErrStat->Fill( transAllErrStat );
				transHistoRangeErrSys ->Fill( transAllErrSys );


				if( x_scan == centerX && y_scan == centerY && uniformity )            
					cout << "center glass (" << centerX << ", " << centerY << "): " << transAll << "\t\t"<<  "<br />" << endl; //transAll

				if( x_scan >= x_scanMin && x_scan <= x_scanMax && y_scan >= y_scanMin && y_scan <= y_scanMax && !misc[i] && !defect[i] )
				{
					if( fresnelPlot )
						transHistoMatrix->Fill( transAll + fresnel); //transAll
					else
						transHistoMatrix->Fill( transAll );

					transHistoMatrixErr    ->Fill( transAllErr );
					transHistoMatrixErrStat->Fill( transAllErrStat );
					transHistoMatrixErrSys ->Fill( transAllErrSys );

					trans_arr[ n_sum ] = transAll;
					transErr_arr[ n_sum ] = transAllErr;
					transErrStat_arr[ n_sum ] = transAllErrStat;
					transErrSys_arr[ n_sum ] = transAllErrSys;

					n_sum++;

					if( profile )
						profile_sumRange += trans_profile;
				}
			}
		}
	}


	Double_t trans = TMath::Mean( nPixel, trans_arr );
	Double_t transErrStat = MyTMath::RMS_square( nPixel, transErrStat_arr );
	Double_t transErrSysSetup = TMath::Mean( nPixel, transErrSys_arr );

	Double_t sysBarErr_helper = TMath::Power( MyTMath::RMS_mean( nPixel, trans_arr ), 2 ) -
		TMath::Power( transErrStat, 2 ) - TMath::Power( transErrSysSetup / TMath::Sqrt( nPixel), 2 );
	if( sysBarErr_helper < 0 )
		sysBarErr_helper = 0;
	Double_t transErrSysBar = TMath::Sqrt( nPixel * sysBarErr_helper );

	Double_t transErr = TMath::Sqrt( TMath::Power( transErrSysBar, 2 ) +  TMath::Power( transErrSysSetup, 2 ) + TMath::Power( transErrStat, 2 ) );


	cout << "transPlot: " << transHistoMatrix->GetMean() << " +- " << transHistoMatrix->GetRMS() << "  3sigma: " <<
		3*transHistoMatrix->GetRMS() << " (" << transHistoMatrix->GetMean()-3*transHistoMatrix->GetRMS() << ")"<< "\t\t"<<  "<br />"  << endl;

	if( uniformity )
		cout << "uniformity (";
	else
		cout << "trans (";


	if ( mirrorCor )
	{
		Double_t transErrTot = TMath::Sqrt( transErr*transErr + trans*trans*corFac_errSys2*corFac_errSys2 );
		cout << x_scanMin << "-" << x_scanMax << "," << y_scanMin << "-" << y_scanMax << "): "
			<< trans << " +- " << transErrTot << " stat. " << transErrStat
			<< "\t\t"<<  "<br />"<< endl<< " sys.(setup: measurement error ) " << transErrSysSetup << "\t\t"<<  "<br />"<< endl<< " sys.(bar: bar inhomogenity) " << transErrSysBar
			<<  "<br />"<< endl<< " sys.(mirror) " << trans*corFac_errSys2<< "\t\t"<<  "<br />" << endl;
	}
	else
	{
		cout << x_scanMin << "-" << x_scanMax << "," << y_scanMin << "-" << y_scanMax << "): "
			<< trans << " +- " << transErr << " stat. " << transErrStat
			<< " sys.(setup) " << transErrSysSetup << " sys.(bar) " << transErrSysBar<< "\t\t"<< "<br />"  << endl;
	}

	if( !mirrorCor && !profile && !uniformity )
		cout << "trans Fresnel cor." << trans + fresnel<< "\t\t"<<  "<br />" << endl;
	if( profile )
	{
		cout << "maximum in profile: " << maxProfile<< "\t\t"<<  "<br />"  << endl;
		cout << "range contribution in profile: " << profile_sumRange / profile_sumAll<< "\t\t" <<  "<br />"<< endl;
	}


	//==============================================================================
	// plotting section
	//==============================================================================
	//TString outFileHTML = filename + "_plot3D.html";  //GS//

	gStyle->SetLabelSize(0.06,"xyz");
	gStyle->SetTitleSize(0.06,"xyz");
	gStyle->SetTitleOffset(0.8,"y");
	gStyle->SetTitleOffset(1.0,"x");
	gStyle->SetLabelOffset(0.01,"x");
	//gStyle->SetOptStat(111110110);
	
	TString outFile = filename + "_plot3D.root";
	TFile *outroot = new TFile( outFile, "RECREATE" );


	TCanvas *canvasNormal = new TCanvas( "c1", "" ,1 ); // 700x500
	TCanvas *canvasSquare = new TCanvas( "c2", "" ,2 ); // 500x500
	TCanvas *canvas;
	canvasNormal->SetRightMargin( 0.15 );
	canvasSquare->SetRightMargin( 0.15 );


	// 	TString titleX = "back/forward [mm]";
	// 	TString titleY = "up/down [mm]";
	TString titleX = "x [mm]";
	TString titleY = "y [mm]";

	// ================== for beam profile plotting .... ============================ ... see canvas is set to logz !!!


	if( profile )
	{
		canvas = canvasSquare;
		canvas->SetLogz();
	}
	else
	{
		if ( uniformity )
			canvas = canvasSquare;
		else
			canvas = canvasNormal;
	}
	canvas->cd();


	error->GetXaxis()->SetTitle( titleX );
	error->GetXaxis()->CenterTitle();
	error->GetYaxis()->SetTitle( titleY );
	error->GetYaxis()->CenterTitle();
	error->GetZaxis()->SetNoExponent();
	error->SetContour(50);

	error->Draw( "colz" );

	if( !profile )
	{
		if( uniformity )
		{
			TLine *a = new TLine( centerX-1, centerY-1, centerX-1, centerY+1 );
			TLine *b = new TLine( centerX-1, centerY+1, centerX+1, centerY+1 );
			TLine *c = new TLine( centerX+1, centerY+1, centerX+1, centerY-1 );
			TLine *d = new TLine( centerX+1, centerY-1, centerX-1, centerY-1 );

			a->SetLineWidth(2);
			b->SetLineWidth(2);
			c->SetLineWidth(2);
			d->SetLineWidth(2);

			a->SetLineStyle(2);
			b->SetLineStyle(2);
			c->SetLineStyle(2);
			d->SetLineStyle(2);

			a->Draw("same");
			b->Draw("same");
			c->Draw("same");
			d->Draw("same");
		}
		else
		{
			Double_t offX =  ( xup - xlow ) / nbinsx;
			Double_t offY =  ( yup - ylow ) / nbinsy;

			TLine *a = new TLine( x_scanMin     , y_scanMin     , x_scanMin     , y_scanMax+offY );
			TLine *b = new TLine( x_scanMin     , y_scanMax+offY, x_scanMax+offX, y_scanMax+offY );
			TLine *c = new TLine( x_scanMax+offX, y_scanMax+offY, x_scanMax+offX, y_scanMin      );
			TLine *d = new TLine( x_scanMax+offX, y_scanMin     , x_scanMin     , y_scanMin      );

			if( defectCutPlot )
			{
				for( int i = 0; i < nEntries; i++ )
				{
					tree->GetEntry( i );


					if( defect[i] == true )
					{
						TLine *x = new TLine( x_scan     , y_scan, x_scan+offX, y_scan+offY );
						TLine *y = new TLine( x_scan+offX, y_scan, x_scan     , y_scan+offY );

						//cout << x_scan << " " << y_scan << endl;

						x->SetLineWidth(2);
						y->SetLineWidth(2);

						x->SetLineStyle(2);
						y->SetLineStyle(2);

						x->Draw("same");
						y->Draw("same");
					}
				}
			}

			a->SetLineWidth(2);
			b->SetLineWidth(2);
			c->SetLineWidth(2);
			d->SetLineWidth(2);

			a->SetLineStyle(2);
			b->SetLineStyle(2);
			c->SetLineStyle(2);
			d->SetLineStyle(2);

			a->Draw("same");
			b->Draw("same");
			c->Draw("same");
			d->Draw("same");
		}
	}
	canvas->Write( "matrix_error" );
	canvas->Clear();


	// ================================== Matrix plotting section ============================================


	matrix->GetXaxis()->SetTitle( titleX );
	matrix->GetXaxis()->CenterTitle();
	matrix->GetYaxis()->SetTitle( titleY );
	matrix->GetYaxis()->CenterTitle();
	matrix->SetContour(50);
	matrix->SetStats(0); //SetStats(0); earlier
	if( !profile )
		//plotrange
		// gPad->SetLogz();
		// matrix->GetZaxis()->SetRangeUser(0.7,1.3);
		matrix->GetZaxis()->SetRangeUser(0.97 ,1.02); // Georg
	//	matrix->GetZaxis()->SetRangeUser(0.996,1.03 ); // Georg
	//matrix->GetXaxis()->SetRangeUser(70,90);//SetNoExponent();
	//gStyle->SetPalette(3,3,2.5);
	//gStyle->SetColor(-3);
	//matrix->GetZaxis()->SetColor(-3);
	matrix->Draw( "COLZ" );
	matrix->Write( "plot_matrix" );

	// ---------------------------------------------------------------------------------------------------------


	if( rangePlot )
	{
		if( uniformity )
		{
			TLine *a = new TLine( centerX-1, centerY-1, centerX-1, centerY+1 );
			TLine *b = new TLine( centerX-1, centerY+1, centerX+1, centerY+1 );
			TLine *c = new TLine( centerX+1, centerY+1, centerX+1, centerY-1 );
			TLine *d = new TLine( centerX+1, centerY-1, centerX-1, centerY-1 );

			a->SetLineWidth(2);
			b->SetLineWidth(2);
			c->SetLineWidth(2);
			d->SetLineWidth(2);

			a->SetLineStyle(2);
			b->SetLineStyle(2);
			c->SetLineStyle(2);
			d->SetLineStyle(2);

			a->Draw("same");
			b->Draw("same");
			c->Draw("same");
			d->Draw("same");
		}
		else
		{
			Double_t offX =  ( xup - xlow ) / nbinsx;
			Double_t offY =  ( yup - ylow ) / nbinsy;

			TLine *a = new TLine( x_scanMin     , y_scanMin     , x_scanMin     , y_scanMax+offY );
			TLine *b = new TLine( x_scanMin     , y_scanMax+offY, x_scanMax+offX, y_scanMax+offY );
			TLine *c = new TLine( x_scanMax+offX, y_scanMax+offY, x_scanMax+offX, y_scanMin      );
			TLine *d = new TLine( x_scanMax+offX, y_scanMin     , x_scanMin     , y_scanMin      );

			if( defectCutPlot )
			{
				for( int i = 0; i < nEntries; i++ )
				{
					tree->GetEntry( i );

					if( defect[i] == true )
					{
						TLine *x = new TLine( x_scan     , y_scan, x_scan+offX, y_scan+offY );
						TLine *y = new TLine( x_scan+offX, y_scan, x_scan     , y_scan+offY );

						x->SetLineWidth(2);
						y->SetLineWidth(2);

						x->SetLineStyle(2);
						y->SetLineStyle(2);

						x->Draw("same");
						y->Draw("same");
					}
				}
			}

			a->SetLineWidth(2);
			b->SetLineWidth(2);
			c->SetLineWidth(2);
			d->SetLineWidth(2);

			a->SetLineStyle(2);
			b->SetLineStyle(2);
			c->SetLineStyle(2);
			d->SetLineStyle(2);

			a->Draw("same");
			b->Draw("same");
			c->Draw("same");
			d->Draw("same");
		}
	}
	canvas->Write( "matrix" );
	canvas->Clear();


	canvas = canvasNormal;
	canvas->cd();
	canvas->SetRightMargin( 0.11 );
	canvas->SetLeftMargin( 0.11 );
	if( profile )
	{
		gStyle->SetOptFit( 0110 );

		TF2 *fit = new TF2("fit", "[0]*exp(-1/2*((x-[1])/[2])^2)*exp(-1/2*((y-[3])/[2])^2)",xlow,xup,ylow,yup); // not reproduable
		fit->SetParameters(1, centerX, 1, centerY);
		matrix->Fit("fit");
		cout << "beam spot width (sigma): " << fit->GetParameter(2) << " +- " << fit->GetParError(2) << endl;
		canvas->Clear();

		TF1 *fitty = new TF1("fitty","gaus");
		fitty->SetLineColor(2);
		TH1D * projectX = matrix->ProjectionX("x",0,-1,"d");
		cout << "Fit X" << endl;
		projectX->Fit("fitty");
		projectX->Draw("H");
		canvas->Write( "matrixFitX" );
		canvas->Clear();
		TH1D * projectY = matrix->ProjectionY("y",0,-1,"d");
		cout << "Fit Y" << endl;
		projectY->Fit("fitty");
		projectY->Draw("H");
		canvas->Write( "matrixFitY" );
		canvas->Clear();
	}


	transHistoRange->GetXaxis()->SetTitle( "transmission" );
	transHistoRange->GetXaxis()->CenterTitle();
	transHistoRange->Draw();
	canvas->Write( "transHist_range" );
	canvas->Clear();

	transHistoMatrix->GetXaxis()->SetTitle( "transmission" );
	transHistoMatrix->GetXaxis()->CenterTitle();
	transHistoMatrix->Draw();
	canvas->Write( "transHist_matrix" );
	canvas->Clear();

	transHistoRangeErr->GetXaxis()->SetTitle( "transmission error" );
	transHistoRangeErr->GetXaxis()->CenterTitle();
	transHistoRangeErr->Draw();
	canvas->Write( "transHist_rangeErr" );
	canvas->Clear();

	transHistoMatrixErr->GetXaxis()->SetTitle( "transmission error" );
	transHistoMatrixErr->GetXaxis()->CenterTitle();
	transHistoMatrixErr->Draw();
	canvas->Write( "transHist_matrixErr" );
	canvas->Clear();

	transHistoRangeErrStat->GetXaxis()->SetTitle( "transmission error" );
	transHistoRangeErrStat->GetXaxis()->CenterTitle();
	transHistoRangeErrStat->Draw();
	canvas->Write( "transHist_rangeErrStat" );
	canvas->Clear();

	transHistoMatrixErrStat->GetXaxis()->SetTitle( "transmission error" );
	transHistoMatrixErrStat->GetXaxis()->CenterTitle();
	transHistoMatrixErrStat->Draw();
	canvas->Write( "transHist_matrixErrStat" );
	canvas->Clear();

	transHistoRangeErrSys->GetXaxis()->SetTitle( "transmission error" );
	transHistoRangeErrSys->GetXaxis()->CenterTitle();
	transHistoRangeErrSys->Draw();
	canvas->Write( "transHist_rangeErrSys" );
	canvas->Clear();

	transHistoMatrixErrSys->GetXaxis()->SetTitle( "transmission error" );
	transHistoMatrixErrSys->GetXaxis()->CenterTitle();
	transHistoMatrixErrSys->Draw();
	canvas->Write( "transHist_matrixErrSys" );
	canvas->Clear();


	canvas->SetLeftMargin( 0.13 );
	canvas->SetRightMargin( 0.09 );
	//   canvas->SetLeftMargin( 0.14 );

	title   = filename + ": pure beam intensity (measurement diode)";
	titleX  = "time [h]";
	titleY  = "intensity [V]";

	MyTGraphOpt::Default( plot_pureBeam_val, title, titleX, titleY );
	plot_pureBeam_val->Draw("ALP");
	canvas->Write("pureBeam_val");
	canvas->Clear();


	title   = filename + ": pure beam intensity (reference diode)";

	MyTGraphOpt::Default( plot_pureBeam_ref, title, titleX, titleY );
	plot_pureBeam_ref->Draw("ALP");
	canvas->Write("pureBeam_ref");
	canvas->Clear();

	titleY  = "ratio";
	title   = filename + ": pure beam intensity ratio of measurement & reference diode";

	MyTGraphOpt::Default( plot_pureBeam_ratio, title, titleX, titleY );
	plot_pureBeam_ratio->Draw("ALP");
	canvas->Write("pureBeam_ratio");
	canvas->Clear();

	titleY  = "intensity [V]";
	title   = filename + ": glass beam intensity (measurement diode)";
	titleX  = "time [h]";
	titleY  = "intensity [V]";

	MyTGraphOpt::Default( plot_glassBeam_val, title, titleX, titleY );
	plot_glassBeam_val->Draw("ALP");
	canvas->Write("glassBeam_val");
	canvas->Clear();


	title   = filename + ": glass beam intensity (reference diode)";

	MyTGraphOpt::Default( plot_glassBeam_ref, title, titleX, titleY );
	plot_glassBeam_ref->Draw("ALP");
	canvas->Write("glassBeam_ref");
	canvas->Clear();

	titleY  = "ratio";
	title   = filename + ": glass beam intensity ratio of measurement & reference diode";

	MyTGraphOpt::Default( plot_glassBeam_ratio, title, titleX, titleY );
	plot_glassBeam_ratio->Draw("ALP");
	canvas->Write("glassBeam_ratio");
	canvas->Clear();

	//Begin temporair GS


	titleY  = "intensity [V]";

	title   = filename + ": glass  temp";

	MyTGraphOpt::Default( plot_glassBeam_temp, title, titleX, titleY );
	plot_glassBeam_temp->Draw("ALP");
	canvas->Write("glassVolt_temp");
	canvas->Clear();

	titleX  = "temperatur [V]";
	title   = filename + ": glass  measurement vs temp";

	MyTGraphOpt::Default( plot_glassBeam_valtemp, title, titleX, titleY );
	plot_glassBeam_valtemp->Draw("ALP");
	canvas->Write("glass_valtemp");
	canvas->Clear();

	title   = filename + ": glass  ref vs temp";

	MyTGraphOpt::Default( plot_glassBeam_reftemp, title, titleX, titleY );
	plot_glassBeam_reftemp->Draw("ALP");
	canvas->Write("glass_reftemp");
	canvas->Clear();

	titleX  = "time [h]";
	title   = filename + ": glass  ratio of measurement & temp";

	MyTGraphOpt::Default( plot_glassBeam_ratiotemp, title, titleX, titleY );
	plot_glassBeam_ratiotemp->Draw("ALP");
	canvas->Write("glassVolt_ratiotemp");
	canvas->Clear();

	titleX  = "time [h]";
	// title   = filename + ": double ratio & time";
	title   = waviColor_mat_str; 
	titleY  = "ratio";
	MyTGraphOpt::Default( plot_double_ratio_time, title, titleX, titleY );
	plot_double_ratio_time->Draw("ALP");
	canvas->Write("double_ratio_time");
	canvas->Clear();




	//End temporair GS

	TH1F *normi = new TH1F( "normi", "profile normalization factor", 1, 0, 2 );
	normi->Fill( norm_profile );
	normi->Draw();
	canvas->Clear();


	outroot->Write();
	outroot->Close();
	cout << "Root-file " << outFile << " was written"<< "\t\t" <<  "<br />" << endl;
	cout << " </font> </p> " << endl;
	canvasNormal->Close();
	canvasSquare->Close();

	TFile::Open( outFile );

	gStyle->SetLabelSize(0.04,"xyz");
	gStyle->SetTitleSize(0.04,"xyz");
	gStyle->SetTitleOffset(0.8,"y");
	gStyle->SetTitleOffset(1.0,"x");
	gStyle->SetLabelOffset(0.01,"x");
	//gStyle->SetOptStat(111110110);
	gStyle->SetOptStat(111111);

	TCanvas * ca = new TCanvas("ca", "ca", 1200, 1400); 
	title=waviColor_mat_str;
	ca->Divide(3,4);	
	ca->SetGrid();
	ca->cd(1);plot_pureBeam_val ->Draw();
	ca->cd(2);plot_pureBeam_ref ->Draw();
	ca->cd(3);plot_pureBeam_ratio ->Draw();
	ca->cd(4);plot_glassBeam_val ->Draw();
	ca->cd(5);plot_glassBeam_ref ->Draw();
	ca->cd(6);plot_glassBeam_ratio ->Draw();
	ca->cd(7);plot_glassBeam_temp ->Draw();
	transHistoMatrix->SetFillColor(40);
	ca->cd(8); transHistoMatrix->Draw();
	ca->cd(9);plot_double_ratio_time ->Draw();
	ca->cd(10); matrix->Draw("COLZ");
	ca->cd(11); matrix->Draw("Lego2");
	ca->cd(12); matrix->Draw("BOX2");
	//ca->Print(waviColor_mat_str,"pdf");   canvasNormal->Close();

	TCanvas * cb = new TCanvas("cb", "cb", 1400, 400); 
	title=waviColor_mat_str;
	cb->Divide(2,1);	
	cb->SetGrid();

	transHistoMatrix->SetFillColor(40);
	cb->cd(1); transHistoMatrix->Draw();
	cb->cd(2); matrix->Draw("COLZ");
	cb->Print(waviColor_mat_str,"pdf");   canvasNormal->Close();


	canvasNormal->Close();
}

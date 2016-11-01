//=============================================================================
// Copyright © 2004 Point Grey Research, Inc. All Rights Reserved.
// 
// This software is the confidential and proprietary information of Point
// Grey Research, Inc. ("Confidential Information").  You shall not
// disclose such Confidential Information and shall use it only in
// accordance with the terms of the license agreement you entered into
// with Point Grey Research, Inc. (PGR).
// 
// PGR MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
// SOFTWARE, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE, OR NON-INFRINGEMENT. PGR SHALL NOT BE LIABLE FOR ANY DAMAGES
// SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING
// THIS SOFTWARE OR ITS DERIVATIVES.
//=============================================================================

//=============================================================================
//
// ladybugPanoStitchExample.cpp
// 
// This example shows users how to extract an image set from a Ladybug camera,
// stitch it together and write the final stitched image to disk.
//
// Since Ladybug library version 1.3.alpha.01, this example is modified to use
// ladybugRenderOffScreenImage(), which is hardware accelerated, to render the
// the stitched images.
//
// Typing ladybugPanoStitchExample /? (or ? -?) at the command prompt will  
// print out the usage information of this application.
//
//=============================================================================

//=============================================================================
// System Includes
//=============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <assert.h>
#include <windows.h>
#include <sstream>

//=============================================================================
// PGR Includes
//=============================================================================
#include <ladybug.h>
#include "ladybugGPS.h"
#include <ladybuggeom.h>
#include <ladybugrenderer.h>
#include <ladybugstream.h>


//=============================================================================
// Macro Definitions
//=============================================================================

#define _HANDLE_ERROR \
    if( error != LADYBUG_OK ) \
    { \
    printf( "Error! Ladybug library reported %s\n", \
    ::ladybugErrorToString( error ) ); \
    assert( false ); \
    goto _EXIT; \
    } 

#define IMAGES_TO_GRAB            10

// The size of the stitched image
#define PANORAMIC_IMAGE_WIDTH    8000
#define PANORAMIC_IMAGE_HEIGHT   4000

#define DELAY   5 //delai en secs entre photos

//#define COLOR_PROCESSING_METHOD   LADYBUG_DOWNSAMPLE4     // The fastest color method suitable for real-time usages
#define COLOR_PROCESSING_METHOD		LADYBUG_HQLINEAR          // High quality method suitable for large stitched images

double dLatitude;
double dLongitude;
double dAltitude;

unsigned char ucHour;
unsigned char ucMinute;
unsigned char ucSecond;
unsigned short wSubSecond;

int ucDay;
int ucMonth;
int wYear;
double dGroundSpeed;
double heading;
double headingtrue;


void getgpsdata(LadybugGPSContext GPScontext, int i){
	
	LadybugNMEAGPSData gga;
	LadybugError gpsError = ladybugGetAllGPSNMEAData(GPScontext, &gga);

	std::stringstream output;
	

	printf("donnees gps : %f ", (float)gga.dataGPGGA.dGGALatitude);
	printf(", %f ", (float)gga.dataGPGGA.dGGALongitude);
	printf(", %f ", (float)gga.dataGPGGA.dGGAAltitude);
	printf(", heading : %f", (float)gga.dataGPRMC.dRMCCourse);
	printf(", headingHDT : %f \n", (float)gga.dataGPHDT.heading);
	


	if ((gpsError == LADYBUG_OK) && (gga.dataGPGGA.bValidData== true)){
		
		dLatitude = (float)gga.dataGPRMC.dRMCLatitude;
		dLongitude = (float)gga.dataGPRMC.dRMCLongitude;
		dAltitude = (float)gga.dataGPGGA.dGGAAltitude;
		//dAltitude = 10;
		ucHour = (int)gga.dataGPRMC.ucRMCHour;
		ucMinute = (int)gga.dataGPRMC.ucRMCMinute;
		ucSecond = (int)gga.dataGPRMC.ucRMCSecond;
		wSubSecond = (int)gga.dataGPRMC.wRMCSubSecond;

		ucDay = (int)gga.dataGPRMC.ucRMCDay;
		ucMonth = (int)gga.dataGPRMC.ucRMCMonth;
		wYear = (int)gga.dataGPRMC.wRMCYear;
		dGroundSpeed = (float)gga.dataGPRMC.dRMCGroundSpeed;
		heading = (float)gga.dataGPRMC.dRMCCourse;
		headingtrue = (float)gga.dataGPHDT.heading;

	} else {
		if (gpsError != LADYBUG_OK){
			output << "LadybugError: " << ladybugErrorToString(gpsError) << std::endl;
		} else {
			output << "GPS data is invalid" << std::endl;
		}
	}

	printf("%s", output.str().c_str());
}



//
// This function reads an image from camera
//
LadybugError
startCamera( LadybugContext context){    
    // Initialize camera
    printf( "Initializing camera...\n" );
    LadybugError error = ladybugInitializeFromIndex( context, 0 );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Get camera info
    printf( "Getting camera info...\n" );
    LadybugCameraInfo caminfo;
    error = ladybugGetCameraInfo( context, &caminfo );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Load config file
    printf( "Load configuration file...\n" );
    error = ladybugLoadConfig( context, NULL);
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Set the panoramic view angle
    error = ladybugSetPanoramicViewingAngle( context, LADYBUG_FRONT_0_POLE_5);
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Make the rendering engine use the alpha mask
    error = ladybugSetAlphaMasking( context, true );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Set color processing method.
    error = ladybugSetColorProcessingMethod( context, COLOR_PROCESSING_METHOD );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    // Configure output images in Ladybug library
    printf( "Configure output images in Ladybug library...\n" );
    error = ladybugConfigureOutputImages( context, LADYBUG_PANORAMIC );
    if (error != LADYBUG_OK)
    {
        return error;    
    }

    error = ladybugSetOffScreenImageSize(
        context, 
        LADYBUG_PANORAMIC,  
        PANORAMIC_IMAGE_WIDTH, 
        PANORAMIC_IMAGE_HEIGHT );  
    if (error != LADYBUG_OK){
        return error;    
    }

    //switch( caminfo.deviceType ){
    //case LADYBUG_DEVICE_COMPRESSOR:
   // case LADYBUG_DEVICE_LADYBUG3:
   // case LADYBUG_DEVICE_LADYBUG5:
        printf( "Starting Ladybug camera...\n" );
        error = ladybugStart( context, LADYBUG_DATAFORMAT_RAW8);	
		//printf("Starting in mode : %s \n", );
        //break;
		if (error != LADYBUG_OK){
			return error;
		}
    /*case LADYBUG_DEVICE_LADYBUG:
    default:
        printf( "Unsupported device.\n");
        error = LADYBUG_FAILED;*/
   // }

    return error;
}


//=============================================================================
// Main Routine
//=============================================================================
int 
main( int argc, char* argv[] )
{
    LadybugContext context = NULL;
    LadybugError error;
	LadybugGPSContext GPScontext;
    unsigned int uiRawCols = 0;
    unsigned int uiRawRows = 0;
    int retry = 10;
    LadybugImage image;

	FILE *fp = NULL;

    // create ladybug context
    printf( "Creating ladybug context...\n" );
    error = ladybugCreateContext( &context );
    if ( error != LADYBUG_OK )
    {
        printf( "Failed creating ladybug context. Exit.\n" );
        return (1);
    }

    // Initialize and start the camera
	printf("Starting camera...\n");
    error = startCamera(context);
	_HANDLE_ERROR

	
	
	// Create GPS context
	printf("Creating GPS context...\n");
	error = ladybugCreateGPSContext(&GPScontext);
	_HANDLE_ERROR;

	// Register the GPS context with the Ladybug context
	printf("Registering GPS...\n");
	error = ladybugRegisterGPS(context, &GPScontext);
	_HANDLE_ERROR;

	// Initialize GPS context with the supplied settings
	printf("Initializing GPS...\n");
	error = ladybugInitializeGPS(
		GPScontext,
		3,
		115200,
		1000);
	_HANDLE_ERROR;

	// Echo settings
	char pszGPSInitText[256] = { 0 };
	sprintf(pszGPSInitText,
		"GPS initialized with the following settings\n"
		"\tCOM Port: %u\n"
		"\tBaud Rate: %u\n"
		"\tUpdate Interval: %ums\n",
		3,
		115200,
		1000);
	printf(pszGPSInitText);


	// Start GPS
	printf("Starting GPS...\n");
	error = ladybugStartGPS(GPScontext);
	_HANDLE_ERROR("ladybugStartGPS()");

	// Let the Ladybug and GPS get synchronized
	// Starting the grab immediately generally results in the initial
	// (around 30) images not having GPS data
	printf("Waiting for 3 seconds...\n");
	Sleep(3000);
	

    // Grab an image to inspect the image size
    printf( "Grabbing image...\n" );
    error = LADYBUG_FAILED;    
    while ( error != LADYBUG_OK && retry-- > 0){
        error = ladybugGrabImage( context, &image ); 
    }
    _HANDLE_ERROR

    // Set the size of the image to be processed
    uiRawCols = image.uiCols;
    uiRawRows = image.uiRows;
   
    // Initialize alpha mask size - this can take a long time if the
    // masks are not present in the current directory.
    printf( "Initializing alpha masks...\n" );
    error = ladybugInitializeAlphaMasks( context, uiRawCols, uiRawRows );
    _HANDLE_ERROR

	//ladybugStop(context);
	
	//char pszGpsFilePath[256];

	//Get a NMEA sentence from the GPS device.
	getgpsdata(GPScontext, 0);

	// si le fichier texte existe pas
	if (fp == NULL) {
		char pszGpsFilePath[256];
		sprintf(pszGpsFilePath, "%s%u%u%u%u%u%u.txt", "gpsdata_", (int)ucDay, (int)ucMonth, (int)wYear, (int)ucHour, (int)ucMinute, (int)ucSecond);
		fp = fopen(pszGpsFilePath, "w");
		fprintf(fp, "INDEX,LAT,LON,HEADING,FILE\n");
	}

    // Process loop
    printf( "Grab loop...\n" );
    for (int i = 0; i < IMAGES_TO_GRAB; i++){

		//ladybugStart(context, LADYBUG_DATAFORMAT_RAW8);

		// grab loop etait ici
        printf( "Processing image %d...\n", i);

        // Grab an image from the camera
        error = ladybugGrabImage(context, &image); 
        _HANDLE_ERROR

        // Convert the image to 6 RGB buffers
        error = ladybugConvertImage(context, &image, NULL);
        _HANDLE_ERROR

        // Send the RGB buffers to the graphics card
        error = ladybugUpdateTextures(context, LADYBUG_NUM_CAMERAS, NULL);
        _HANDLE_ERROR

        // Stitch the images (inside the graphics card) and retrieve the output to the user's memory
        LadybugProcessedImage processedImage;
        error = ladybugRenderOffScreenImage(context, LADYBUG_PANORAMIC, LADYBUG_BGR, &processedImage);
		_HANDLE_ERROR

		//error = ladybugSetAutoJPEGBufferUsage(context, 80);
		error = ladybugSetJPEGQuality(context, 90);
		//error = ladybugSetImageSavingJpegQuality(context, 90);

		//Get a NMEA sentence from the GPS device.
		getgpsdata(GPScontext, i);

		processedImage.metaData.dGPSLatitude = dLatitude;
		processedImage.metaData.dGPSLongitude = dLongitude;
		processedImage.metaData.dGPSAltitude = dAltitude;
		processedImage.metaData.ucGPSHour = ucHour;
		processedImage.metaData.ucGPSMinute = ucMinute;
		processedImage.metaData.ucGPSSecond = ucSecond;
		processedImage.metaData.wGPSSubSecond = wSubSecond;
		processedImage.metaData.ucGPSDay = ucDay;
		processedImage.metaData.ucGPSMonth = ucMonth;
		processedImage.metaData.wGPSYear = wYear;
		processedImage.metaData.dGPSGroundSpeed = dGroundSpeed;
		
        // Save the output image to a file
        char pszOutputName[256];
		sprintf(pszOutputName, "%i%i20%i-%ih%im%i.%is_%f.%f-%f_%03d.jpg", (int)ucDay, (int)ucMonth, (int)wYear, (int)ucHour, (int)ucMinute, (int)ucSecond, (int)wSubSecond,(float)dLatitude, (float)dLongitude, (float)headingtrue, i);
        printf( "Writing image %s...\n", pszOutputName);
        error = ladybugSaveImage( context, &processedImage, pszOutputName, LADYBUG_FILEFORMAT_JPG );
        _HANDLE_ERROR

		// save gps data in txt file
		printf("GPS INFO: LAT %lf, LONG %lf, HEADING %lf\n", dLatitude, dLongitude, headingtrue);

		// si le fichier existe pas
		/*if (fp == NULL){
			char pszGpsFilePath[256];
			sprintf(pszGpsFilePath, "%s%u%u%u%u%u%u.txt", "gpsdata_", (int)ucDay, (int)ucMonth, (int)wYear, (int)ucHour, (int)ucMinute, (int)ucSecond);
			fp = fopen(pszGpsFilePath, "w");
		}*/
		// si le fichier existe ecrire
		if (fp != NULL){
			
			fprintf(fp, "%u,%lf,%lf,%lf,%s\n", i+1, dLatitude, dLongitude, headingtrue, pszOutputName);
		}

		//printf("<PRESS ANY KEY TO CONTINUE>");
		//_getch();
		
		//ladybugStop(context);
		
		Sleep(1000 * DELAY);
		
    }

    printf("Done.\n");

if (fp != NULL) {
	fclose(fp);
}	

_EXIT:
    //
    // clean up
    //

    //ladybugStop( context );
    ladybugDestroyContext( &context );

	// Stop GPS
	printf("Stopping GPS\n");
	error = ladybugStopGPS(GPScontext);

    printf("<PRESS ANY KEY TO EXIT>");
    _getch();
    return 0;
}


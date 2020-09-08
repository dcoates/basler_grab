// Grab.cpp
/*
    Note: Before getting started, Basler recommends reading the Programmer's Guide topic
    in the pylon C++ API documentation that gets installed with pylon.
    If you are upgrading to a higher major version of pylon, Basler also
    strongly recommends reading the Migration topic in the pylon C++ API documentation.

    This sample illustrates how to grab and process images using the CInstantCamera class.
    The images are grabbed and processed asynchronously, i.e.,
    while the application is processing a buffer, the acquisition of the next buffer is done
    in parallel.

    The CInstantCamera class uses a pool of buffers to retrieve image data
    from the camera device. Once a buffer is filled and ready,
    the buffer can be retrieved from the camera object for processing. The buffer
    and additional image data are collected in a grab result. The grab result is
    held by a smart pointer after retrieval. The buffer is automatically reused
    when explicitly released or when the smart pointer object is destroyed.
*/

// Include files to use the pylon API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#include <iostream>
#include <ctime>
#include <ratio>
#include <chrono>

#include <sys/stat.h>

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Namespace for highres clock
using namespace std::chrono;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 4651*20;

const int hz_approx=4424;
const float num_secs=0.5;
const float num_frames=c_countOfImagesToGrab; //num_secs*hz_approx*20;

// For my case, don't think there is a such a huge difference:
//#define CLOCK_TYPE CLOCK_REALTIME
#define CLOCK_TYPE CLOCK_MONOTONIC

#define TEMP_PREFIX "/var/run/user/1001"

int main(int argc, char* argv[])
{
    FILE *pFile_times, *pFile_times_temp, *pFile_ims;
    clock_t start, end;
    struct timespec t_start, t_end, t_diff, t_temp;
    double cpu_time_used;
    int frames=0, done=0;

    // The exit code of the sample application.
    int exitCode = 0;

    pFile_times_temp=fopen(TEMP_PREFIX "/times_temp.txt","wt");
    if (pFile_times_temp)
        printf("Temp_times file ok\n");

	// This will be overwritten when they hit a key:
      	high_resolution_clock::time_point t_start2 = high_resolution_clock::now();

    pFile_ims=fopen("movies/movie.bin","wb");
    if (pFile_ims)
        printf("Movie file ok\n");


    // Before using any pylon methods, the pylon runtime must be initialized. 
    PylonInitialize();

    try
    {
        // Create an instant camera object with the camera device found first.
        CInstantCamera camera( CTlFactory::GetInstance().CreateFirstDevice());

        // Print the model name of the camera.
        cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

        // The parameter MaxNumBuffer can be used to control the count of buffers
        // allocated for grabbing. The default value of this parameter is 10.
        camera.MaxNumBuffer = 5;

        // Start the grabbing of c_countOfImagesToGrab images.
        // The camera device is parameterized with a default configuration which
        // sets up free-running continuous acquisition.
        camera.StartGrabbing( c_countOfImagesToGrab);

        // This smart pointer will receive the grab result data.
        CGrabResultPtr ptrGrabResult;

        // Camera.StopGrabbing() is called automatically by the RetrieveResult() method
        // when c_countOfImagesToGrab images have been retrieved.
        while ( camera.IsGrabbing() & !done)
        {
	char s[1024]; // to hold timing msg

            clock_gettime(CLOCK_TYPE,&t_temp);

            // Wait for an image and then retrieve it. A timeout of 5000 ms is used.
            camera.RetrieveResult( 5000, ptrGrabResult, TimeoutHandling_ThrowException);

	    	// Time (in seconds) since started
            high_resolution_clock::time_point t_now = high_resolution_clock::now();
	    	duration<double> time_span = duration_cast<duration<double>>(t_now - t_start2);

            clock_gettime(CLOCK_TYPE,&t_diff);
       	    //fprintf(pFile_times,"%d,%ld,",frames,t_temp.tv_nsec);
            //fprintf(pFile_times,"%ld,%ld,",t_diff.tv_nsec,t_diff.tv_nsec-t_temp.tv_nsec);
            //fprintf(pFile_times,"%g\n",(double)time_span.count());

	    sprintf(s, "%d,%ld,%ld,%ld,%g",
       	    	frames,t_temp.tv_nsec,
            	t_diff.tv_nsec,t_diff.tv_nsec-t_temp.tv_nsec,
            	(double)time_span.count() );
            fprintf(pFile_times_temp,"%s\n",s); // Write a copy to a temp file

            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
		const int width=ptrGrabResult->GetWidth();
		const int height=ptrGrabResult->GetHeight();
                const uint16_t *pImageBuffer = (uint16_t *) ptrGrabResult->GetBuffer();

		//fwrite(pImageBuffer, 1, width*height,pFile_im1_temp);
		//rewind(pFile_im1_temp);

				//if (recording)
		fwrite(pImageBuffer, 1, width*height,pFile_ims);

#ifdef PYLON_WIN_BUILD
                // Display the grabbed image.
                Pylon::DisplayImage(1, ptrGrabResult);
#endif

		frames += 1;

		if (frames>num_frames) {
			done=1;
		}
            }
            else
            {
                cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
            }
        }
    }
    catch (const GenericException &e)
    {
        // Error handling.
        cerr << "An exception occurred." << endl
        << e.GetDescription() << endl;
        exitCode = 1;
    }

    fclose(pFile_times_temp);
    fclose(pFile_ims);

    // Comment the following two lines to disable waiting on exit.
    //cerr << endl << "Press Enter to exit." << endl;
    //while( cin.get() != '\n');

    // Releases all pylon resources. 
    PylonTerminate();  

    return exitCode;
}

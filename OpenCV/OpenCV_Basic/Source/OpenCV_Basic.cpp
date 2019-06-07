/*=============================================================================
Copyright (C) 2017 Allied Vision Technologies. All Rights Reserved.

Redistribution of this file, in original or modified form, without
prior written consent of Allied Vision Technologies is prohibited.

-------------------------------------------------------------------------------

File:        main.cpp

Description: Acquire and display images with OpenCV

-------------------------------------------------------------------------------

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include <opencv2/opencv.hpp>
#include <csignal>
#include <cstdlib>
#include <cstdio>

// Object of type VideoCapture:class for capturing from cameras
cv::VideoCapture vcap;

// Object of type Mat representing a frame to be visualized
cv::Mat frame;

// bool to control streaming loop
bool streaming = true;

//Helper function to print out how to use the program
void PrintHelp(const char *pApplicatioName)
{
    std::cout << "Usage: " << pApplicatioName << " [options]\n";
    std::cout << "E.g.: " << pApplicatioName << " -d 3\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "-h | --help          Print this message\n";
    std::cout << "-d | --device        Video device number\n";
}

// Catch signals SIGINT and SIGTERM to shutdown gracefully
void handleShutDownSignal(int signalId)
{
    std::cout << "Shutting down..." << std::endl;
    streaming = false;
}

int main(int argc, char** argv)
{
    // Register signal handler
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = handleShutDownSignal;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
  
    // Parse command line arguments
    int nDevice = -1;
    if(argc > 1 && (std::string(argv[1]) == std::string("-h") || std::string(argv[1]) == std::string("--help")))
    {
        PrintHelp(argv[0]);
        return 0;
    }
    else if(argc == 3 && (std::string(argv[1]) == std::string("-d") || std::string(argv[1]) == std::string("--device")))
    { 
        nDevice = atoi(argv[2]);
        if(nDevice < 0 || nDevice > 10)
        {
            std::cout << "Invalid device number \"" << nDevice << "\". Exiting." << std::endl;
            return -1;
        }
    }
    else
    {
        std::cout << "Invalid command line options." << std::endl;
        PrintHelp(argv[0]);
        return -1;
    } 
    
    // Open the camera in /dev/videoX
    vcap.open(nDevice);
    
    if(!vcap.isOpened())
    {
        std::cout << "Error opening the camera." << std::endl;
        return -1;
    }

    std::cout << "Start streaming. Exit with Ctrl+C or Esc. " << std::endl;
    
    // capture loop
    while(streaming)
    {
        // get next frame
        vcap >> frame;

        // Display frame if not empty
        if(!frame.empty())
        {
            // Resize frame
            cv::resize(frame, frame, cv::Size(640, 480), 0, 0, cv::INTER_NEAREST);
            
            // Display frame
            cv::imshow("Camera", frame);

            // Wait 10 ms for the ESC-Key
            if(cv::waitKey(10) == 27)
            {
                streaming = false;
            }
        }
    }

    return 0;
}


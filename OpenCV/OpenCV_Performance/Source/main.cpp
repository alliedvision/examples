/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        main.cpp

  Description: The OpenCV V4L2 Demo will demonstrate how to open and initialize
               a V4L2 device and how to acquire and display images from it.

-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include <QApplication>
#include <QMessageBox>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <csignal>
#include <cstdlib>
#include <cstdio>
#include "MainWindow.h"
#include "Version.h"
#include "Camera.h"
#include <opencv2/core/version.hpp>

#define DEFAULT_BUFFER_COUNT 5

// Helper function to print out how to use the program
void PrintHelp(const char *pApplicatioName)
{
    std::cout << "Usage: " << pApplicatioName << " [options]\n";
    std::cout << "\n";
    std::cout << "Options:\n";
    std::cout << "-d | --device        Video device name\n";
    std::cout << "-m | --mmap          Use memory mapped buffers [default]\n";
    std::cout << "-u | --userp         Use application allocated buffers\n";
    std::cout << "-b | --buff          Number of buffers [" << DEFAULT_BUFFER_COUNT << "]\n";
    std::cout << "-l | --left          Left (X offset) for crop\n";
    std::cout << "-t | --top           Top (Y offset) for crop\n";
    std::cout << "-w | --width         Width for crop\n";
    std::cout << "-h | --height        Height for crop\n";
}

// Catch signals SIGINT and SIGTERM to shutdown gracefully
void handleShutDownSignal(int signalId)
{
    std::cout << "Shutting down..." << std::endl;
    QApplication::exit(0);
}


int main(int argc, char *argv[])
{
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = handleShutDownSignal;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGTERM, &sigIntHandler, NULL);
    
    // Initialize Qt application
    QApplication application(argc, argv);
    application.setApplicationName(QObject::tr(APPLICATION_PRODUCT_NAME));
    application.setApplicationVersion(APPLICATION_VERSION);
    
    int nResult = -1;
    try
    {
        // Print out application name and version
        std::cout << "////////////////////////////////////////////\n";
        std::cout << "/// " << APPLICATION_PRODUCT_NAME << " - Version " << APPLICATION_VERSION << "\n";
	std::cout << "/// OpenCV - Version " << CV_VERSION_MAJOR << "." << CV_VERSION_MINOR << "." << CV_VERSION_REVISION << "\n";
        std::cout << "////////////////////////////////////////////\n";
        std::cout << "\n";
        
        // Parse command line arguments
        char short_options[] = "d:mub:l:t:w:h:";
        option long_options[] =
        {
            { "device", required_argument, NULL, 'd' },
            { "mmap",   no_argument,       NULL, 'm' },
            { "userp",  no_argument,       NULL, 'u' },
            { "buff",   required_argument, NULL, 'b' },
            { "left",   required_argument, NULL, 'l' },
            { "top",    required_argument, NULL, 't' },
            { "width",  required_argument, NULL, 'w' },
            { "height", required_argument, NULL, 'h' },            
            { 0, 0, 0, 0 }
        };

        bool bIOMethodFound = false;
        IOMethod eIOMethod = IOMethod_MMAP;
        bool bDeviceNameFound = false;
        std::string strDeviceName;
        bool bBufferFound = false;
        uint32_t nBufferCount = DEFAULT_BUFFER_COUNT;
        Crop crop = {-1, -1, -1, -1};
        while(true)
        {
            int nIndex = 0;
            int nChar = getopt_long(argc, argv, short_options, long_options, &nIndex);

            if(-1 == nChar)
            {
                break;
            }

            switch(nChar)
            {
            case 0: /* getopt_long() flag */
                break;

            case 'd':
                strDeviceName = optarg;
                bDeviceNameFound = true;
                break;

            case 'm':
                if(true == bIOMethodFound)
                {
                    throw std::runtime_error("IO method option used multiple times.");
                }
                eIOMethod = IOMethod_MMAP;
                bIOMethodFound = true;
                break;

            case 'u':
                if(true == bIOMethodFound)
                {
                    throw std::runtime_error("IO method option used multiple times.");
                }
                eIOMethod = IOMethod_UserPtr;
                bIOMethodFound = true;
                break;
                
            case 'b':
                if(true == bBufferFound)
                {
                    throw std::runtime_error("Buffer option used multiple times.");
                }
                nBufferCount = QString(optarg).toInt();
                if(nBufferCount <= 0)
                {
                    std::ostringstream stream;
                    stream << "Number of buffers must at least be 1";
                    throw std::runtime_error(stream.str());
                }
                bBufferFound = true;
                break;
                
            case 'l':
                if(crop.m_nLeft >= 0)
                {
                    throw std::runtime_error("Crop left option used multiple times.");
                }
                crop.m_nLeft = QString(optarg).toUInt();
                if(crop.m_nLeft < 0)
                {
                    std::ostringstream stream;
                    stream << "Crop left must at least be 0";
                    throw std::runtime_error(stream.str());
                }
                break;
                
            case 't':
                if(crop.m_nTop >= 0)
                {
                    throw std::runtime_error("Crop top option used multiple times.");
                }
                crop.m_nTop = QString(optarg).toUInt();
                if(crop.m_nTop < 0)
                {
                    std::ostringstream stream;
                    stream << "Crop top must at least be 0";
                    throw std::runtime_error(stream.str());
                }
                break;
                
            case 'w':
                if(crop.m_nWidth >= 0)
                {
                    throw std::runtime_error("Crop width option used multiple times.");
                }
                crop.m_nWidth = QString(optarg).toUInt();
                if(crop.m_nWidth <= 0)
                {
                    std::ostringstream stream;
                    stream << "Crop width must at least be 1";
                    throw std::runtime_error(stream.str());
                }
                break;
                
            case 'h':
                if(crop.m_nHeight >= 0)
                {
                    throw std::runtime_error("Crop height option used multiple times.");
                }
                crop.m_nHeight = QString(optarg).toUInt();
                if(crop.m_nHeight <= 0)
                {
                    std::ostringstream stream;
                    stream << "Crop height must at least be 1";
                    throw std::runtime_error(stream.str());
                }
                break;

            default:
                throw std::runtime_error("Invalid command line option passed.");
            }
        }
        
        if(false == bDeviceNameFound)
        {
            throw std::runtime_error("No device specified.");
        }
        
        // Create a list of pixel formats supported by this application.
        // Currently we only support RGB24 and XBGR32.
        std::set<__u32> supportedPixelFormats;
        supportedPixelFormats.insert(V4L2_PIX_FMT_RGB24);
        supportedPixelFormats.insert(V4L2_PIX_FMT_XBGR32);


        // Create camera. This will also open the V4L2 device, initialize it and start streaming.
        // The camera object will be deleted if it goes out of scope which will also stop streaming
        // close the device and perform cleanup.
        QSharedPointer<Camera> pCamera = Camera::New(strDeviceName, eIOMethod, nBufferCount, supportedPixelFormats, crop);

        //Setup user interface for image display
        MainWindow mainWindow(pCamera);
        nResult = application.exec();
    }
    catch(const std::exception &rException)
    {
        // Print out error message
        std::cerr << "Error: " << rException.what() << "\n";
        std::cerr << "\n";
        PrintHelp(argv[0]);
        nResult = -1;
    }
    catch(...)
    {
        // Print out error message
        std::cerr << "Error: Unknown error while starting application.\n\n";
        std::cerr << "\n";
        PrintHelp(argv[0]);
        nResult = -1;
    }

    return nResult;
}

/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        Camera.cpp

  Description: The Basic V4L2 Demo will demonstrate how to open and initialize
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

#include "Camera.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdexcept>
#include <fcntl.h>
#include <sstream>
#include <exception>
#include <iostream>
#include <sys/mman.h>
#include <QMutexLocker>
#include <QtGlobal>
#include <limits>
#include <errno.h>

//Maximum time period used for averaging frame rate
#define MAX_TIME_FPS_AVERAGE    1.0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Helper functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Wrapper function for ioctl to wait for completion
int xioctl(int nFD, int nRequest, void *pArg)
{
    int nResult = 0;

    do 
    {
        nResult = ioctl(nFD, nRequest, pArg);
    } 
    while (-1 == nResult && EINTR == errno);

    return nResult;
}

//Helper function for error message creation
std::string CreateError(std::string strText)
{
    std::ostringstream text;
    text << strText << ": " << strerror(errno) << " (error code: " << errno << ")";
    return text.str();
}

//Helper function for exception creation
std::runtime_error CreateException(std::string strText) 
{
    return std::runtime_error(CreateError(strText));
}

//Helper function to convert fourcc into string
std::string FourCCToString(__u32 nFourCC)
{
    const char *pFourCC = (const char*)&nFourCC;
    std::ostringstream stream;
    stream << pFourCC[0] << pFourCC[1] << pFourCC[2] << pFourCC[3];
    
    return stream.str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Buffer: Wrapper for a single buffered frame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Constructor for MMAP buffer
Buffer::Buffer(v4l2_buffer *pBuffer, int nFileDescriptor, bool mplaneApi, __u32 nIndex, __u32 nWidth, __u32 nHeight, __u32 nBytesPerLine, __u32 nPixelFormat)
    :   m_pData(NULL)
    ,   m_nSize(0)
    ,   m_eIOMethod(IOMethod_MMAP)
    ,   m_bMplaneApi(mplaneApi)
    ,   m_nIndex(nIndex)
    ,   m_nWidth(nWidth)
    ,   m_nHeight(nHeight)
    ,   m_nBytesPerLine(nBytesPerLine)
    ,   m_nPixelFormat(nPixelFormat)
{
    if (NULL == pBuffer)
    {
        throw std::runtime_error("No buffer passed");
    }
    if (-1 == nFileDescriptor)
    {
        throw std::runtime_error("Invalid file descriptor passed");
    }
    if (m_nWidth <= 0)
    {
        throw std::runtime_error("Invalid width passed");
    }
    if (m_nHeight <= 0)
    {
        throw std::runtime_error("Invalid height passed");
    }
    if (m_nBytesPerLine <= 0)
    {
        throw std::runtime_error("Invalid bytes per line passed");
    }
    
    //For mmap we map the kernel memory into the application
    void *pData = mmap(NULL, m_bMplaneApi ? pBuffer->m.planes[0].length : pBuffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, nFileDescriptor, m_bMplaneApi ? pBuffer->m.planes[0].m.mem_offset : pBuffer->m.offset);
    if (MAP_FAILED == pData)
    {
        throw CreateException("Could not map buffer into memory");
    }
    
    m_pData = (char*)pData;
    m_nSize = m_bMplaneApi ? (size_t)pBuffer->m.planes[0].length : (size_t)pBuffer->length;
}

//Constructor for userptr buffer
Buffer::Buffer(size_t nSize, bool mplaneApi, __u32 nIndex, __u32 nWidth, __u32 nHeight, __u32 nBytesPerLine, __u32 nPixelFormat)
    :   m_pData(NULL)
    ,   m_nSize(nSize)
    ,   m_eIOMethod(IOMethod_UserPtr)
    ,   m_bMplaneApi(mplaneApi)
    ,   m_nIndex(nIndex)
    ,   m_nWidth(nWidth)
    ,   m_nHeight(nHeight)
    ,   m_nBytesPerLine(nBytesPerLine)
    ,   m_nPixelFormat(nPixelFormat)
{
    if (nSize <= 0)
    {
        throw std::runtime_error("Invalid size passed");
    }
    if (m_nWidth <= 0)
    {
        throw std::runtime_error("Invalid width passed");
    }
    if (m_nHeight <= 0)
    {
        throw std::runtime_error("Invalid height passed");
    }
    if (m_nBytesPerLine <= 0)
    {
        throw std::runtime_error("Invalid bytes per line passed");
    }
    
    if(m_nSize % 128)
    {
        m_nSize = ((m_nSize / 128) + 1) * 128;
    }
    
    //For userptr we need to allocate memory in the application
    m_pData = static_cast<char*>(aligned_alloc(128, m_nSize));
    
    
    if (NULL == m_pData)
    {
        throw std::runtime_error("Could not allocate buffer memory");
    }
}

Buffer::~Buffer()
{
    if (NULL != m_pData)
    {
        switch(m_eIOMethod)
        {
        case IOMethod_MMAP:
            //For mmap we need to unmap the memory for cleanup
            if(-1 == munmap(m_pData, m_nSize))
            {
                std::cerr << CreateError("Could not unmap buffer memory") << "\n";
            }
            break;
            
        case IOMethod_UserPtr:
            delete []m_pData;
            break;
            
        default:
            std::cerr << "Invalid IO method found\n";
            break;
        }
        
        m_pData = NULL;
    }
}

//Access functions for the members
void* Buffer::GetData() const
{
    return m_pData;
}

size_t Buffer::GetSize() const
{
    return m_nSize;
}

__u32 Buffer::GetIndex() const
{
    return m_nIndex;
}

__u32 Buffer::GetWidth() const
{
    return m_nWidth;
}

__u32 Buffer::GetHeight() const
{
    return m_nHeight;
}

__u32 Buffer::GetBytesPerLine() const
{
    return m_nBytesPerLine;
}

__u32 Buffer::GetPixelFormat() const
{
    return m_nPixelFormat;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Camera: Wrapper class for V4L2 setup and image acquisition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Camera::Camera(QObject *pParent)
    :   QThread(pParent)
    ,   m_nFileDescriptor(-1)
    ,   m_bInitialized(false)
    ,   m_eIOMethod(IOMethod_MMAP)
    ,   m_eStreamState(StreamState_Idle)
    ,   m_nFrameCounter(0)
    ,   m_dFrameRate(0.0)
{
    m_nStopDequeuePipe[0] = -1;
    m_nStopDequeuePipe[1] = -1;
    
    //Create pipe that will be used for stopping the acquisition thread
    //without having to wait for a timeout.
    if (-1 == pipe(m_nStopDequeuePipe))
    {
        throw CreateException("Could not create stop dequeue pipe");
    }

    //Set pipe reading to nonblocking (for being able to flush the pipe
    //easily)
    try
    {
        int nFlags = fcntl(m_nStopDequeuePipe[0], F_GETFL);
        if (-1 == nFlags)
        {
            throw CreateException("Could not get pipe flags");
        }
        
        nFlags |= O_NONBLOCK;        
        if (-1 == fcntl(m_nStopDequeuePipe[0], F_SETFL, nFlags))
        {
            throw CreateException("Could not set pipe flags");
        }
    }
    catch(...)
    {
        //Cleanup the pipe in case of an error
        if (-1 == close(m_nStopDequeuePipe[0]))
        {
            std::cerr << CreateError("Error while closing read pipe") << "\n";
        }

        if (-1 == close(m_nStopDequeuePipe[1]))
        {
            std::cerr << CreateError("Error while closing write pipe") << "\n";
        }
        
        throw;
    }
}

Camera::~Camera()
{
    //First close the device (if it is open)
    CloseDevice();
    
    //Finally cleanup the pipe
    if (-1 == close(m_nStopDequeuePipe[0]))
    {
        std::cerr << CreateError("Error while closing read pipe") << "\n";
    }    
    if (-1 == close(m_nStopDequeuePipe[1]))
    {
        std::cerr << CreateError("Error while closing write pipe") << "\n";
    }
}

//Open the V4L2 device
void Camera::OpenDevice(std::string strDeviceName)
{
    if (-1 != m_nFileDescriptor)
    {
        throw std::runtime_error("Device is already open");
    }
    
    //We open the V4L2 device in nonblocking mode because we want
    //to wait via the select function and not block with VIDIOC_DQBUF.
    m_nFileDescriptor = open(strDeviceName.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (-1 == m_nFileDescriptor)
    {
        std::ostringstream stream;
        stream << "Cannot open device \"" << strDeviceName << "\"";
        throw CreateException(stream.str());
    }
}

//Close the V4L2 device
void Camera::CloseDevice()
{
    if (-1 != m_nFileDescriptor)
    {
        //First uninitialized the device (if it is initialized)
        UninitDevice();
        
        //Close the device
        int nResult = close(m_nFileDescriptor);
        if(-1 == nResult)
        {
            std::cerr << CreateError("Error while closing device") << "\n";
        }
        
        m_nFileDescriptor = -1;
    }
}

template <class T1, class T2, class T3>
void Camera::LimitValue(T1 &rStart, T2 &rSize, const T1 &rBoundsStart, const T2 &rBoundsSize, const T3 &rTargetStart, const T3 &rTargetSize)
{
    if (rTargetStart >= 0)
    {
        rStart = (T1)rTargetStart;
    }
    if (rTargetSize >= 1)
    {
        rSize = (T2)rTargetSize;
    }
        
    rStart = qMax(qMin(rStart, rBoundsStart + ((T1)rBoundsSize) - 1), rBoundsStart);
    rSize = qMin(rSize, rBoundsSize - (T2)(rStart - rBoundsStart));
}

//Initialized the V4L2 device (pixel format, IO method, crop, buffers)
void Camera::InitDevice(IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop)
{
    if (-1 == m_nFileDescriptor)
    {
        throw std::runtime_error("Device is not open");
    }
    if (nBufferCount <= 0)
    {
        throw std::runtime_error("Invalid buffer count passed");
    }    
    if (true == m_bInitialized)
    {
        throw std::runtime_error("Device is already initialized");
    }
    
    //For safety reasons we should check if the device is a proper V4L2 device
    v4l2_capability cap;
    if (-1 == xioctl(m_nFileDescriptor, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            throw std::runtime_error("Device is not a V4L2 device");
        }
        else
        {
            throw CreateException("Cannot query device capabilites");
        }
    }
    
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    {
        m_bMplaneApi = false;
        m_nV4L2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
    else if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)
    {
        m_bMplaneApi = true;
        m_nV4L2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    else
    {        
        throw std::runtime_error("Device is not a video capture device");
    }
    
    switch(eIOMethod)
    {
    case IOMethod_MMAP:
    case IOMethod_UserPtr:
        if (!(cap.capabilities & V4L2_CAP_STREAMING))
        {
            throw std::runtime_error("Device does not support streaming i/o");
        }
        break;

    default:
        throw std::runtime_error("Invalid IO method found");
    }
    
    //Get the list of supported formats from the device
    std::cout << "Pixelformats supported by device (V4L2 FOURCCs):\n";
    std::set<__u32> supportedByDevice;
    for (__u32 i = 0;;i++)
    {
        v4l2_fmtdesc fmtdesc;
        memset(&fmtdesc, 0, sizeof(fmtdesc));
        fmtdesc.index = i;
        fmtdesc.type  = m_nV4L2BufType;
        if(-1 == ioctl(m_nFileDescriptor, VIDIOC_ENUM_FMT, &fmtdesc))
        {
            break;
        }
        
        std::cout << FourCCToString(fmtdesc.pixelformat) << "\n";
        
        supportedByDevice.insert(fmtdesc.pixelformat);
    }
    std::cout << "\n";
    
    //Set cropping if it is desired
    if (    (rCrop.m_nLeft >= 0)
         || (rCrop.m_nTop >= 0)
         || (rCrop.m_nWidth >= 1)
         || (rCrop.m_nHeight >= 1))
    {
        //Get crop capabilites from device to get maximum
        //possible region of interest
        v4l2_cropcap cropcap;
        memset(&cropcap, 0, sizeof(cropcap));
        cropcap.type = m_nV4L2BufType;
        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_CROPCAP, &cropcap))
        {
            throw CreateException("Cannot get crop capabilities from device");
        }
        
        //Set cropping in the device with the given values
        v4l2_crop crop;
        memset(&crop, 0, sizeof(crop));
        crop.type = m_nV4L2BufType;		
        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_G_CROP, &crop))
        {
            throw CreateException("Cannot get crop from device");
        }

        //Limit desired crop rectangle to bounds. We move this to this template function
        //because older kernels have v4l2_rect.width and v4l2_rect.height as __s32 and
        //newer kernels have it as __u32.
        LimitValue(crop.c.left, crop.c.width, cropcap.bounds.left, cropcap.bounds.width, rCrop.m_nLeft, rCrop.m_nWidth);
        LimitValue(crop.c.top, crop.c.height, cropcap.bounds.top, cropcap.bounds.height, rCrop.m_nTop, rCrop.m_nHeight);

        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_S_CROP, &crop))
        {
            throw CreateException("Cannot set crop in device");
        }
       
  /*
  
        LimitValue(crop.c.left, crop.c.width, cropcap.bounds.left, cropcap.bounds.width, rCrop.m_nLeft, rCrop.m_nWidth);
        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_S_CROP, &crop))
        {
            throw CreateException("Cannot set crop in device");
        }
  
        usleep(1000*1000);
        LimitValue(crop.c.top, crop.c.height, cropcap.bounds.top, cropcap.bounds.height, rCrop.m_nTop, rCrop.m_nHeight);
        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_S_CROP, &crop))
        {
            throw CreateException("Cannot set crop in device");
        }
*/
  
        v4l2_crop crop_r;
        memset(&crop_r, 0, sizeof(crop_r));
        crop_r.type = m_nV4L2BufType;    
        int r = xioctl(m_nFileDescriptor, VIDIOC_G_CROP, &crop_r);
        
        int i=0;
        
    }
    
    //Check if one of the desired pixel formats is supported by the device
    bool bPixelformatFound = false;
    __u32 nPixelformat = 0;
    for(    std::set<__u32>::const_iterator iterator = rSupportedPixelFormats.begin();
            iterator != rSupportedPixelFormats.end();
            iterator++)
    {
        if (supportedByDevice.find(*iterator) != supportedByDevice.end())
        {
            nPixelformat = *iterator;
            bPixelformatFound = true;
            break;
        }
    }
    
    if (false == bPixelformatFound)
    {
        throw std::runtime_error("Could not find a pixelformat that is supported by device AND application.");
    }

    //Get current format from the device
    v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = m_nV4L2BufType;
    if (-1 == xioctl(m_nFileDescriptor, VIDIOC_G_FMT, &fmt))
    {
        throw CreateException("Cannot get format from device");
    }
    
    //Update format with new pixel format and set it in the device
    std::cout << "Setting pixelformat " << FourCCToString(nPixelformat) << ".\n";
    
    fmt.fmt.pix.pixelformat = nPixelformat;
    if (-1 == xioctl(m_nFileDescriptor, VIDIOC_S_FMT, &fmt))
    {
        throw CreateException("Cannot set format in device");
    }
    
    //Read back and check the changes of the format
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = m_nV4L2BufType;
    if (-1 == xioctl(m_nFileDescriptor, VIDIOC_G_FMT, &fmt))
    {
        throw CreateException("Cannot get format from device");
    }
    
    if (fmt.fmt.pix.pixelformat != nPixelformat)
    {
        throw CreateException("Could not set pixel format");
    }
    
    if(m_bMplaneApi)
    {
        m_nNumPlanes = fmt.fmt.pix_mp.num_planes;
    }
    
    //Request buffers from the device (either userptr or mmap)
    __u32 ioMethod = 0;
    switch(eIOMethod)
    {
    case IOMethod_MMAP:
        ioMethod = V4L2_MEMORY_MMAP;
        break;
        
    case IOMethod_UserPtr:
        ioMethod = V4L2_MEMORY_USERPTR;
        break;

    default:
        throw std::runtime_error("Invalid IO method found");
    }
    
    v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = nBufferCount;
    req.type = m_nV4L2BufType;
    req.memory = ioMethod;
    if (-1 == xioctl(m_nFileDescriptor, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            throw std::runtime_error("Device does not support IO method");
        }
        else
        {
            throw CreateException("Cannot request buffers");
        }
    }
    
    //Check the number of buffers that we got from the driver
    if (req.count <= 0)
    {
        throw std::runtime_error("Insufficient buffer memory on device");
    }
    
    try
    {
        if (req.count != nBufferCount)
        {
            std::cout << "Info: " << req.count << " number of buffers requested but only " << nBufferCount << " will be used\n";
        }

        //Allocate buffer objects for each driver buffer
        switch(eIOMethod)
        {
        case IOMethod_MMAP:
            for(__u32 i = 0; i < req.count; i++)
            {
                v4l2_buffer buf;
                v4l2_plane planes[m_nNumPlanes];
                memset(&buf, 0, sizeof(buf));
                
                if(m_bMplaneApi)
                {
                    buf.m.planes = planes;
                    buf.length = m_nNumPlanes;
                }

                buf.type    = m_nV4L2BufType;
                buf.memory  = V4L2_MEMORY_MMAP;
                buf.index   = i;

                if(-1 == xioctl(m_nFileDescriptor, VIDIOC_QUERYBUF, &buf))
                {
                    throw CreateException("Could not query buffer");
                }

                QSharedPointer<Buffer> pBuffer(new Buffer(  &buf,
                                                            m_nFileDescriptor,
                                                            m_bMplaneApi,
                                                            i,
                                                            fmt.fmt.pix.width,
                                                            fmt.fmt.pix.height,
                                                            fmt.fmt.pix.bytesperline,
                                                            fmt.fmt.pix.pixelformat));
                if(NULL == pBuffer)
                {
                    throw CreateException("Could not allocate buffer");
                }
                
                m_BuffersByIndex[pBuffer->GetIndex()] = pBuffer;
                m_BuffersByPtr[pBuffer->GetData()] = pBuffer;
            }
            break;

        case IOMethod_UserPtr:
            for(__u32 i = 0; i < req.count; i++)
            {
                QSharedPointer<Buffer> pBuffer(new Buffer(  fmt.fmt.pix.sizeimage,
                                                            m_bMplaneApi,
                                                            i,
                                                            fmt.fmt.pix.width,
                                                            fmt.fmt.pix.height,
                                                            fmt.fmt.pix.bytesperline,
                                                            fmt.fmt.pix.pixelformat));
                if(NULL == pBuffer)
                {
                    throw CreateException("Could not allocate buffer");
                }
                
                m_BuffersByIndex[pBuffer->GetIndex()] = pBuffer;
                m_BuffersByPtr[pBuffer->GetData()] = pBuffer;
            }
            break;

        default:
            throw std::runtime_error("Invalid IO method found");
        }
    }
    catch(...)
    {
        //Clean up buffers and deallocate in driver in case we encounter an error
        m_BuffersByIndex.clear();
        m_BuffersByPtr.clear();
        
        v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 0;
        req.type = m_nV4L2BufType;
        req.memory = ioMethod;
        if(-1 == xioctl(m_nFileDescriptor, VIDIOC_REQBUFS, &req))
        {
            std::cerr << CreateError("Cannot request buffers s") << "\n";
        }
    
        throw;
    }
    
    m_eIOMethod = eIOMethod;
    m_bInitialized = true;
}

//Uninitialized the V4L2 device (cleanup)
void Camera::UninitDevice()
{
    if(true == m_bInitialized)
    {
        //First stop streaming (if it is running)
        StopStreaming();
        
        //Clean up buffers and deallocate in driver
        __u32 ioMethod = 0;
        switch(m_eIOMethod)
        {
        case IOMethod_MMAP:
            ioMethod = V4L2_MEMORY_MMAP;
            break;

        case IOMethod_UserPtr:
            ioMethod = V4L2_MEMORY_USERPTR;
            break;
        }

        m_BuffersByIndex.clear();
        m_BuffersByPtr.clear();

        v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 0; //Using 0 here tells the driver to free buffers
        req.type = m_nV4L2BufType;
        req.memory = ioMethod;
        if (-1 == xioctl(m_nFileDescriptor, VIDIOC_REQBUFS, &req))
        {
            std::cerr << CreateError("Cannot request buffers") << "\n";
        }

        m_bInitialized = false;
    }
}

//Thread procedure for image acquisition
void Camera::run()
{
    QMutexLocker locker(&m_Mutex);
    try
    {
        //We continue with the acquisition until we are told to stop
        while (StreamState_Running == m_eStreamState)
        {
            //Using select function to wait for frames
            fd_set fdset;
            FD_ZERO(&fdset);
            //Pipe is signalled if we shall stop waiting
            FD_SET(m_nStopDequeuePipe[0], &fdset);
            //File descriptor is signalled if a buffer is available
            FD_SET(m_nFileDescriptor, &fdset);

            //Use a timeout of 1 second to update framerate in
            //case suddenly no image are acquired anymore
            timeval tv;
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            //While waiting we need to unlock the mutex and relock it afterwards
            locker.unlock();        
            int nResult = select(std::max(m_nStopDequeuePipe[0], m_nFileDescriptor) + 1, &fdset, NULL, NULL, &tv);
            locker.relock();

            //Break out of look if we need to abort acquisition thread
            if (StreamState_Running != m_eStreamState)
            {
                break;
            }

            //Check if waiting via select function was successful
            if (-1 == nResult)
            {
                throw CreateException("Could not wait via select");
            }

            //Get current system time (used for framerate calculation)
            timespec time;
            if (-1 == clock_gettime(CLOCK_REALTIME, &time))
            {
                throw CreateException("Could not system time for frame rate calculation");
            }
            double dTime = ((double)time.tv_sec) + ((double)time.tv_nsec) * 1e-9;
            
            //Check if a buffer is available for queueing
            if (FD_ISSET(m_nFileDescriptor, &fdset) != 0)
            {
                QSharedPointer<Buffer> pBuffer;
                switch(m_eIOMethod)
                {
                case IOMethod_MMAP:
                    {
                        //First dequeue buffer for mmap
                        v4l2_buffer buf;
                        memset(&buf, 0, sizeof(buf));
                        buf.type = m_nV4L2BufType;
                        buf.memory = V4L2_MEMORY_MMAP;
                        
                        v4l2_plane planes[m_nNumPlanes];
                        if(m_bMplaneApi)
                        {
                            buf.m.planes = planes;
                            buf.length = m_nNumPlanes;
                        }                        
                        
                        if(-1 == xioctl(m_nFileDescriptor, VIDIOC_DQBUF, &buf))
                        {
                            if (EAGAIN != errno)
                            {
                                throw CreateException("Could not dequeue buffer");
                            }
                        }
                        else
                        {
                            //If we were able to dequeue a buffer we try to find the according buffer object
                            std::map<__u32, QSharedPointer<Buffer> >::iterator iterator = m_BuffersByIndex.find(buf.index);
                            if (m_BuffersByIndex.end() == iterator)
                            {
                                throw std::runtime_error("Invalid buffer index found");
                            }

                            pBuffer = iterator->second;
                            if (NULL == pBuffer)
                            {
                                throw std::runtime_error("Invalid buffer found"); 
                            }
                        }
                    }
                    break;

                case IOMethod_UserPtr:
                    {
                        //First dequeue buffer for userptr
                        v4l2_buffer buf;
                        memset(&buf, 0, sizeof(buf));
                        buf.type = m_nV4L2BufType;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        
                        v4l2_plane planes[m_nNumPlanes];
                        if(m_bMplaneApi)
                        {
                            buf.m.planes = planes;
                            buf.length = m_nNumPlanes;
                        }
                        
                        if (0 == xioctl(m_nFileDescriptor, VIDIOC_DQBUF, &buf))
                        {
                            //If we were able to dequeue a buffer we try to find the according buffer object
                            std::map<void*, QSharedPointer<Buffer> >::iterator iterator = m_BuffersByPtr.find((void*)buf.m.userptr);
                            if (m_BuffersByPtr.end() == iterator)
                            {
                                throw std::runtime_error("Invalid buffer userptr found");
                            }

                            pBuffer = iterator->second;
                            if (NULL == pBuffer)
                            {
                                throw std::runtime_error("Invalid buffer found"); 
                            }
                        }
                    }
                    break;

                default:
                    throw CreateException("Invalid IO method found during acquisition");
                }

                //Process the buffer if a buffer was dequeued
                if(NULL != pBuffer)
                {
                    //Increment frame counter and push frame time into queue
                    m_nFrameCounter++;
                    m_FrameTimes.push_front(dTime);
                    
                    //In this application we only want to show the latest
                    //image, so we requeue an old frame if we have a newer one
                    if (NULL != m_pNextBuffer)
                    {
                        v4l2_buffer buf;
                        memset(&buf, 0, sizeof(buf));
                        buf.type = m_nV4L2BufType;
                        buf.index = m_pNextBuffer->GetIndex();
                        
                        v4l2_plane planes[m_nNumPlanes];
                        if(m_bMplaneApi)
                        {
                            buf.m.planes = planes;
                            buf.length = m_nNumPlanes;
                        }

                        switch(m_eIOMethod)
                        {
                        case IOMethod_MMAP:
                            buf.memory = V4L2_MEMORY_MMAP;
                            break;
                            
                        case IOMethod_UserPtr:
                            buf.memory = V4L2_MEMORY_USERPTR;
                            buf.m.userptr = (unsigned long)m_pNextBuffer->GetData();
                            buf.length = (__u32)m_pNextBuffer->GetSize();
                            break;

                        default:
                            throw CreateException("Invalid IO method found during acquisition");
                        }
                        
                        if(-1 == xioctl(m_nFileDescriptor, VIDIOC_QBUF, &buf))
                        {
                            throw CreateException("Could not requeue buffer");
                        }
                        
                        m_pNextBuffer = pBuffer;
                    }
                    else
                    {
                        //Notify the outside world that we received a new buffer
                        //but we only do that if they haven't even collected
                        //the previous frame.
                        m_pNextBuffer = pBuffer;
                        
                        emit NewBufferAvailable();
                    }
                }
            }

            //Remove older frames but at least keep the last 2 frames
            //in order to be able to compute the frame rate for them.
            while(m_FrameTimes.size() > 2)
            {
                if ((m_FrameTimes.front() - m_FrameTimes.back()) > MAX_TIME_FPS_AVERAGE)
                {
                    m_FrameTimes.pop_back();
                }
                else
                {
                    break;
                }
            }

            //Update frame rate
            if(m_FrameTimes.size() >= 2)
            {
                //Either the frame rate is computed with the time
                //between the last frames or it will be computed using
                //the elapsed time since the last acquired frame (whichever
                //is longer).
                double dFrameDelta = m_FrameTimes.front() - m_FrameTimes.back();
                double dIdleDelta = dTime - m_FrameTimes.front();
                if (dFrameDelta > dIdleDelta)
                {
                    //We recently received some frames that can be used
                    //for framerate calculation
                    if (dFrameDelta > std::numeric_limits<double>::epsilon())
                    {
                        m_dFrameRate = ((double)(m_FrameTimes.size() - 1)) / dFrameDelta;
                    }
                }
                else
                {                		
                    //We haven't received frames for a while and therefore
                    //the frame rate drops
                    if (dIdleDelta > std::numeric_limits<double>::epsilon())
                    {
                        m_dFrameRate = 1.0 / dIdleDelta;
                    }
                }
            }
        }
    }
    catch(const std::exception &rException)
    {
        //We only show the errors here because we are in a background thread
        std::cerr << "Error: " << rException.what() << "\n";
    }
    catch(...)
    {
        //We only show the errors here because we are in a background thread
        std::cerr << "Error: Unknown error during acquisition.\n";
    }
    
    //Cleanup variables and stop streaming when thread is stopped
    m_nFrameCounter = 0;
    m_FrameTimes.clear();
    m_dFrameRate = 0.0;
    m_pNextBuffer.clear();
    m_pUsedBuffer.clear();
    
    v4l2_buf_type type = m_bMplaneApi ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(m_nFileDescriptor, VIDIOC_STREAMOFF, &type))
    {
        std::cerr << CreateError("Could not stop streaming") << "\n";
    }

    m_eStreamState = StreamState_Stopped;
}

//Start streaming by queueing frames, starting the thread and starting
//streaming in the device
void Camera::StartStreaming()
{
    if(false == m_bInitialized)
    {
        throw std::runtime_error("Device is not initialized");
    }
    
    QMutexLocker locker(&m_Mutex);
    if(StreamState_Idle != m_eStreamState)
    {
        throw std::runtime_error("Device is already streaming");
    }
    
    //Clear pipe so that select function will not be woken up accidentally
    char nValue = 0;
    while(read(m_nStopDequeuePipe[0], &nValue, sizeof(nValue)) > 0);
    
    //Start acquisition thread and set it to time critical
    //because we don't want to miss frames
    m_eStreamState = StreamState_Running;
    start();
    setPriority(QThread::TimeCriticalPriority);
    locker.unlock();
    
    try
    {
        //Queue frames into driver (either for mmap or for userptr)
        switch(m_eIOMethod)
        {
        case IOMethod_MMAP:
            for(    std::map<__u32, QSharedPointer<Buffer> >::iterator iterator = m_BuffersByIndex.begin();
                    m_BuffersByIndex.end() != iterator;
                    iterator++)
            {
                QSharedPointer<Buffer> pBuffer = iterator->second;
                if(NULL == pBuffer)
                {
                    throw std::runtime_error("Invalid buffer found");
                }
                
                v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = m_nV4L2BufType;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = pBuffer->GetIndex();
                
                v4l2_plane planes[m_nNumPlanes];
                if(m_bMplaneApi)
                {
                    buf.m.planes = planes;
                    buf.length = m_nNumPlanes;
                }                        
                
                if (-1 == xioctl(m_nFileDescriptor, VIDIOC_QBUF, &buf))
                {
                    std::cout << " StartStreaming Errorrr" << std::endl;
                    throw CreateException("Could not queue buffer");
                }
            }
            break;

        case IOMethod_UserPtr:
            for(    std::map<__u32, QSharedPointer<Buffer> >::iterator iterator = m_BuffersByIndex.begin();
                    m_BuffersByIndex.end() != iterator;
                    iterator++)
            {
                QSharedPointer<Buffer> pBuffer = iterator->second;
                if (NULL == pBuffer)
                {
                    throw std::runtime_error("Invalid buffer found");
                }
                
                v4l2_buffer buf;
                memset(&buf, 0, sizeof(buf));
                buf.type = m_nV4L2BufType;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.index = (__u32)pBuffer->GetIndex();
                buf.m.userptr = (unsigned long)pBuffer->GetData();
                buf.length = (__u32)pBuffer->GetSize();
                
                v4l2_plane planes[m_nNumPlanes];
                if(m_bMplaneApi)
                {
                    buf.m.planes = planes;
                    buf.length = m_nNumPlanes;
                }
                
                if(-1 == xioctl(m_nFileDescriptor, VIDIOC_QBUF, &buf))
                {
                    throw CreateException("Could not queue buffer");
                }
            }
            break;

        default:
            throw std::runtime_error("Invalid IO method found");
        }
        
        //Finally start capturing in the device
        v4l2_buf_type type = m_bMplaneApi ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(-1 == xioctl(m_nFileDescriptor, VIDIOC_STREAMON, &type))
        {
            throw CreateException("Could not start streaming");
        }
    }
    catch(...)
    {
        //Stop streaming and cleanup if we encountered an error
        StopStreaming();
        
        throw;
    }
}

void Camera::StopStreaming()
{
    QMutexLocker locker(&m_Mutex);
    if(StreamState_Idle != m_eStreamState)
    {
        //We actually only stop the thread if it didn't stop itself because of an error
        if (StreamState_Running == m_eStreamState)
        {
            m_eStreamState = StreamState_Stopping;
            
            //We write into the pipe in order to break out of the select
            //function in the acquisition thread
            char c = 1;
            if(-1 == write(m_nStopDequeuePipe[1], &c, sizeof(c)))
            {
                std::cerr << CreateError("Could not write to pipe") << "\n";
            }
        }
        
        //Now we wait for the completion of the thread (unlocking and
        //relocking mutex during that time)
        locker.unlock();
        wait();
        locker.relock();
        
        m_eStreamState = StreamState_Idle;
    }
}

//Open and initialize V4L2 device and finally start streaming
//Cleanup will be done in the destructor
void Camera::Init(std::string strDeviceName, IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop)
{
    OpenDevice(strDeviceName);
    InitDevice(eIOMethod, nBufferCount, rSupportedPixelFormats, rCrop);
    StartStreaming();
}

//Helper function to allocate a new camera object
QSharedPointer<Camera> Camera::New(std::string strDeviceName, IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop, QObject *pParent)
{
    QSharedPointer<Camera> pCamera(new Camera(pParent));
    if (NULL == pCamera)
    {
        throw std::runtime_error("Could not allocate camera object");
    }
    
    pCamera->Init(strDeviceName, eIOMethod, nBufferCount, rSupportedPixelFormats, rCrop);
    
    return pCamera;
}

//If a buffer is available it can be obtained by this function. This will
//also automatically release the previous buffer and requeue it into the driver
QSharedPointer<Buffer> Camera::GetNewBuffer()
{
    QSharedPointer<Buffer> pBuffer;
    
    QMutexLocker locker(&m_Mutex);
    //Check if there is a new buffer
    if (NULL != m_pNextBuffer)
    {
        //Requeue previous buffer into the driver if there was one
        if (NULL != m_pUsedBuffer)
        {
            v4l2_buffer buf;
            memset(&buf, 0, sizeof(buf));
            buf.type = m_nV4L2BufType;
            buf.index = m_pUsedBuffer->GetIndex();
            
            v4l2_plane planes[m_nNumPlanes];
            if(m_bMplaneApi)
            {
                buf.m.planes = planes;
                buf.length = m_nNumPlanes;
            }    

            switch(m_eIOMethod)
            {
            case IOMethod_MMAP:
                buf.memory = V4L2_MEMORY_MMAP;
                break;

            case IOMethod_UserPtr:
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.m.userptr = (unsigned long)m_pUsedBuffer->GetData();
                buf.length = (__u32)m_pUsedBuffer->GetSize();
                break;

            default:
                throw CreateException("Invalid IO method found during acquisition");
            }

            if(-1 == xioctl(m_nFileDescriptor, VIDIOC_QBUF, &buf))
            {
                throw CreateException("Could not requeue buffer");
            }
                
            m_pUsedBuffer.clear();
        }

        //Use the new buffer and return it
        m_pUsedBuffer = m_pNextBuffer;
        pBuffer = m_pUsedBuffer;
        m_pNextBuffer.clear();
    }
    
    return pBuffer;
}

//Get current streaming device status and statistics
void Camera::GetStatus(bool *pStreaming, uint64_t *pFrameCounter, double *pFrameRate)
{
    QMutexLocker locker(&m_Mutex);
    if (NULL != pStreaming)
    {
        if (StreamState_Running == m_eStreamState)
        {
            *pStreaming = true;
        }
        else
        {
            *pStreaming = false;
        }
    }
    if (NULL != pFrameCounter)
    {
        *pFrameCounter = m_nFrameCounter;
    }
    if (NULL != pFrameRate)
    {
        *pFrameRate = m_dFrameRate;
    }
}

//Check if a new buffer is available and can be obtained via GetNewBuffer
bool Camera::IsBufferAvailable()
{
    QMutexLocker locker(&m_Mutex);
    if (NULL != m_pNextBuffer)
    {
        return true;
    }
    
    return false;
}

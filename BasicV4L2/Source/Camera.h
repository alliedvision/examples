/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        Camera.h

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

#ifndef CAMERA_H
#define CAMERA_H

#include <QSharedPointer>
#include <QThread>
#include <QMutex>
#include <string>
#include <map>
#include <set>
#include <list>
#include <linux/videodev2.h>
#include <stdint.h>

//Currently we support two acquisition methods (mmap and userptr)
enum IOMethod
{
    IOMethod_MMAP       = 0x00,
    IOMethod_UserPtr    = 0x01
};

//Struct for crop settings
struct Crop
{
    int m_nLeft;
    int m_nTop;
    int m_nWidth;
    int m_nHeight;
};

//Wrapper class for a single buffered frame
class Buffer
{
private:
    char        *m_pData;
    size_t      m_nSize;
    IOMethod    m_eIOMethod;
    bool        m_bMplaneApi;
    __u32       m_nIndex;
    __u32       m_nWidth;
    __u32       m_nHeight;
    __u32       m_nBytesPerLine;
    __u32       m_nPixelFormat;
    
public:
    Buffer(v4l2_buffer *pBuffer, int nFileDescriptor, bool mplaneApi, __u32 nIndex, __u32 nWidth, __u32 nHeight, __u32 nBytesPerLine, __u32 nPixelFormat);
    Buffer(size_t nSize, bool mplaneApi, __u32 nIndex, __u32 nWidth, __u32 nHeight, __u32 nBytesPerLine, __u32 nPixelFormat);
    ~Buffer();
    
    void* GetData() const;
    size_t GetSize() const;
    __u32 GetIndex() const;
    __u32 GetWidth() const;
    __u32 GetHeight() const;
    __u32 GetBytesPerLine() const;
    __u32 GetPixelFormat() const;
};

//Wrapper class for V4L2 setup and image acquisition
class Camera : public QThread
{
    Q_OBJECT
    
private:
    enum StreamState
    {
        StreamState_Idle	= 0,
        StreamState_Running	= 1,
        StreamState_Stopping	= 2,
        StreamState_Stopped	= 3,
    };
    int                                         m_nFileDescriptor;
    bool                                        m_bInitialized;
    bool                                        m_bMplaneApi;
    __u32                                       m_nV4L2BufType;
    __u8                                        m_nNumPlanes;
    IOMethod                                    m_eIOMethod;
    std::map<__u32, QSharedPointer<Buffer> >    m_BuffersByIndex;
    std::map<void*, QSharedPointer<Buffer> >    m_BuffersByPtr;
    QMutex                                      m_Mutex;
    StreamState                                 m_eStreamState;
    int                                         m_nStopDequeuePipe[2];
    uint64_t                                    m_nFrameCounter;
    std::list<double>                           m_FrameTimes;
    double                                      m_dFrameRate;
    QSharedPointer<Buffer>                      m_pNextBuffer;
    QSharedPointer<Buffer>                      m_pUsedBuffer;
    
    virtual void run();
	
	template <class T1, class T2, class T3>
    static void LimitValue(T1 &rStart, T2 &rSize, const T1 &rBoundsStart, const T2 &rBoundsSize, const T3 &rTargetStart, const T3 &rTargetSize);
    
    void OpenDevice(std::string strDeviceName);
    void CloseDevice();
    
    void InitDevice(IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop);
    void UninitDevice();
    
    void StartStreaming();
    void StopStreaming();
    
    Camera(QObject *pParent);
    void Init(std::string strDeviceName, IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop);
    
public:
    static QSharedPointer<Camera> New(std::string strDeviceName, IOMethod eIOMethod, uint32_t nBufferCount, const std::set<__u32> &rSupportedPixelFormats, const Crop &rCrop, QObject *pParent = NULL);
    ~Camera();
    
    bool IsBufferAvailable();
    QSharedPointer<Buffer> GetNewBuffer();
    void GetStatus(bool *pStreaming, uint64_t *pFrameCounter, double *pFrameRate);
    
signals:
    void NewBufferAvailable();
};

#endif //CAMERA_H
 

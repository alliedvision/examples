/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        MainWindow.cpp

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

#include "MainWindow.h"
#include <QMessageBox>
#include <QWheelEvent>
#include <QGraphicsPixmapItem>
#include <QLineF>
#include <QtCore/qmath.h>
#include <stdexcept>
#include "Version.h"

//The zoom factor for zooming in or out one step
#define ZOOM_FACTOR             1.2
//Update interval for acquisition status and frame rate
//in case the image acquisition stops
#define STATUS_UPDATE_INTERVAL  1000
//Minimum time between two displayed frames because it makes no sense
//to waste CPU time for displaying an unlimited amout of FPS if the
//human eye cannot perceive that anyway.
#define MIN_DISPLAY_INTERVAL    40

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ImageConverter: Converts V4L2 buffers into QPixmap using a separate thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageConverter::ImageConverter(QSharedPointer<Camera> pCamera, QObject* pParent)
    :   QObject(pParent)
    ,   m_pCamera(pCamera)
{
    if(NULL == m_pCamera)
    {
        throw std::runtime_error("No camera passed");
    }
    
    //We need 3 frames for the image converter in the pool:
    //- One for the current conversion
    //- One that is displayed next
    //- One that is displayed at the moment
    for(int i = 0; i < 3; i++)
    {
        QSharedPointer<Image> pImage(new Image());
        if(NULL == pImage)
        {
            throw std::runtime_error("Could not allocate image");
        }
        
        m_ImagePool.push_back(pImage);
    }
    
    //Connect camera signal so that we get notified if a new buffer is ready
    connect(    m_pCamera.data(), SIGNAL(NewBufferAvailable()),
                this, SLOT(NewBufferAvailable()),
                Qt::QueuedConnection);
    
    //Check if a buffer was already available befor connection of the signal
    if(m_pCamera->IsBufferAvailable())
    {
        NewBufferAvailable();
    }
}

//Slot for notification if a new buffer is available in the camera
void ImageConverter::NewBufferAvailable()
{
    //First we try to get an unused image from the pool
    QSharedPointer<Image> pImage;
    while(      (m_ImagePool.empty() == false)
            &&  (NULL == pImage))
    {
        pImage = m_ImagePool.front();
        m_ImagePool.pop_front();
    }
    
    if(NULL == pImage)
    {
        return;
    }

    //Now we try to get the new buffer from the camera
    QSharedPointer<Buffer> pBuffer = m_pCamera->GetNewBuffer();
    if(NULL == pBuffer)
    {
        //Push image back into pool if we were not able to get the new buffer
        m_ImagePool.push_front(pImage);
        return;
    }
    
    pImage->m_nWidth = pBuffer->GetWidth();
    pImage->m_nHeight = pBuffer->GetHeight();
    pImage->m_nPixelFormat = pBuffer->GetPixelFormat();	
    
    //Currently this demo application only supports RGB24 and XBGR32
    switch(pBuffer->GetPixelFormat())
    {
    case V4L2_PIX_FMT_RGB24:
        {
            //We create a new QImage if the image dimensions or pixel format doesn't match
            if(     (pImage->m_Image.width() != (int)pBuffer->GetWidth())
                ||  (pImage->m_Image.height() != (int)pBuffer->GetHeight())
                ||  (pImage->m_Image.format() != QImage::Format_RGB888))
            {
                pImage->m_Image = QImage((int)pBuffer->GetWidth(), (int)pBuffer->GetHeight(), QImage::Format_RGB888);
            }
            
            //Now we copy the buffer data into the image. If the line pitch (bytes per line) is equal
            //we can copy as one block. Otherwise we have to copy line per line.
            if((int)(pBuffer->GetWidth() * 3) == pImage->m_Image.bytesPerLine())
            {
                memcpy(pImage->m_Image.bits(), pBuffer->GetData(), (size_t)(pBuffer->GetWidth() * pBuffer->GetHeight() * 3));
            }
            else
            {
                int nWidth = (int)pBuffer->GetWidth();
                int nHeight = (int)pBuffer->GetHeight();
                size_t nLineSize = (size_t)(nWidth * 3);
                char *pInput = (char*)pBuffer->GetData();
                char *pOutput = (char*)pImage->m_Image.bits();
                size_t nLinePitchInput = (size_t)(nLineSize);
                size_t nLinePitchOutput = (size_t)(pImage->m_Image.bytesPerLine());
                for(int i = 0; i < nHeight; i++)
                {
                    memcpy(pOutput, pInput, nLineSize);
                    pInput += nLinePitchInput;
                    pOutput += nLinePitchOutput;
                }
            }

            //Create QPixmap from image
            pImage->m_Pixmap = QPixmap::fromImage(pImage->m_Image);
            //Remember that we converted the image
            pImage->m_bConverted = true;
        }
        break;
    case V4L2_PIX_FMT_XBGR32:
        {
            //We create a new QImage if the image dimensions or pixel format doesn't match
            if(     (pImage->m_Image.width() != (int)pBuffer->GetWidth())
                ||  (pImage->m_Image.height() != (int)pBuffer->GetHeight())
                ||  (pImage->m_Image.format() != QImage::Format_RGB32))
            {
                pImage->m_Image = QImage((int)pBuffer->GetWidth(), (int)pBuffer->GetHeight(), QImage::Format_RGB32);
            }

            //Now we copy the buffer data into the image. If the line pitch (bytes per line) is equal
            //we can copy as one block. Otherwise we have to copy line per line.
            if((int)(pBuffer->GetWidth() * 4) == pImage->m_Image.bytesPerLine())
            {
                memcpy(pImage->m_Image.bits(), pBuffer->GetData(), (size_t)(pBuffer->GetWidth() * pBuffer->GetHeight() * 4));
            }
            else
            {
                int nWidth = (int)pBuffer->GetWidth();
                int nHeight = (int)pBuffer->GetHeight();
                size_t nLineSize = (size_t)(nWidth * 4);
                char *pInput = (char*)pBuffer->GetData();
                char *pOutput = (char*)pImage->m_Image.bits();
                size_t nLinePitchInput = (size_t)(nLineSize);
                size_t nLinePitchOutput = (size_t)(pImage->m_Image.bytesPerLine());
                for(int i = 0; i < nHeight; i++)
                {
                    memcpy(pOutput, pInput, nLineSize);
                    pInput += nLinePitchInput;
                    pOutput += nLinePitchOutput;
                }
            }

            //Create QPixmap from image
            pImage->m_Pixmap = QPixmap::fromImage(pImage->m_Image);
            //Remember that we converted the image
            pImage->m_bConverted = true;

        }
        break;

    default:
        //We will report to GUI if pixel format of the buffer could not be converted
        pImage->m_bConverted = false;
        break;
    }

    //Notify the GUI about the new image
    QMutexLocker locker(&m_Mutex);
    if(NULL == m_pNextImage)
    {
        //Only notify the GUI if the previous frame was
        //already processed.
        m_pNextImage = pImage;
        locker.unlock();
        emit NewImageAvailable();
    }
    else
    {
        m_ImagePool.push_back(m_pNextImage);
        m_pNextImage = pImage;
    }
}

//Check if a new image is available and can be obtained via GetNewImage
bool ImageConverter::IsImageAvailable()
{
    QMutexLocker locker(&m_Mutex);
    if(NULL == m_pNextImage)
    {
        return false;
    }
    
    return true;
}

//If an image is available it can be obtained by this function. This will
//also automatically release the previous image and use it for conversion
ImageResult ImageConverter::GetNewImage(QPixmap &rPixmap, __u32 &rWidth, __u32 &rHeight, __u32 &rPixelFormat)
{
    QMutexLocker locker(&m_Mutex);
    //No new image is available
    if(NULL == m_pNextImage)
    {
        return ImageResult_None;
    }
    
    //Release used image
    if(NULL != m_pUsedImage)
    {
        m_ImagePool.push_back(m_pUsedImage);
    }
    
    //Now use the next image
    m_pUsedImage = m_pNextImage;
    m_pNextImage.clear();
    
    rWidth = m_pUsedImage->m_nWidth;
    rHeight = m_pUsedImage->m_nHeight;
    rPixelFormat = m_pUsedImage->m_nPixelFormat;
    
    if(false == m_pUsedImage->m_bConverted)
    {
        //Report that a buffer could not be converted
        return ImageResult_CouldNotConvert; 
    }
    
    //Return converted image
    rPixmap = m_pUsedImage->m_Pixmap;
    
    return ImageResult_Ok;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//MainWindow: Main application window for image display
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MainWindow::MainWindow(QSharedPointer<Camera> pCamera)
    :   m_pCamera(pCamera)
    ,   m_dScaleFactor(1.0)
    ,   m_pZoomLabel(NULL)
    ,   m_pStatusLabel(NULL)
    ,   m_pStatusTimer(NULL)
    ,   m_pDisplayTimer(NULL)
    ,   m_pScene(NULL)
    ,   m_pPixmapItem(NULL)
    ,   m_bImageDisplayed(false)
    ,	m_nWidth(0)
    ,	m_nHeight(0)
    ,	m_nPixelFormat(0)
{
    if(NULL == m_pCamera)
    {
        throw std::runtime_error("No camera passed");
    }
    
    ui.setupUi(this);
    setWindowTitle(tr(APPLICATION_PRODUCT_NAME) + " " + QString(APPLICATION_VERSION));

    m_DefaultPixmap = QPixmap(":/MainDialog/AlliedVision.png");
    
    //Add status label to status bar (for frame counter and frame rate)
    m_pStatusLabel = new QLabel(ui.m_pStatusBar);
    ui.m_pStatusBar->addPermanentWidget(m_pStatusLabel);
    
    //Setup image display
    m_pScene = new QGraphicsScene();
    ui.m_pGraphicsView->setScene(m_pScene);
    m_pPixmapItem = new QGraphicsPixmapItem();
    m_pScene->addItem(m_pPixmapItem);
    m_pScene->setSceneRect((qreal)0, (qreal)0, (qreal)m_DefaultPixmap.width(), (qreal)m_DefaultPixmap.height());
    m_pPixmapItem->setPixmap(m_DefaultPixmap);
    
    //Add zoom label to tool bar
    m_pZoomLabel = new QLabel(ui.m_pToolBar);
    ui.m_pToolBar->insertWidget(ui.m_pActionZoomOut, m_pZoomLabel);
    UpdateZoomText();
    
    //Create timer for limiting the frame rate for image display
    m_pDisplayTimer = new QTimer(this);
    m_pDisplayTimer->setSingleShot(true);
    connect(    m_pDisplayTimer, SIGNAL(timeout()),
                this, SLOT(DisplayTimer()));

    //Create timer for showing the acquisition status if no
    //frames are acquired anymore
    m_pStatusTimer = new QTimer(this);
    m_pStatusTimer->setSingleShot(true);    
    connect(    m_pStatusTimer, SIGNAL(timeout()),
                this, SLOT(StatusTimer()));
    UpdateStatus();

    //Start worker thread for image conversion
    m_pWorkerThread = QSharedPointer<QThread>(new QThread());
    m_pWorkerThread->start();
    
    m_pImageConverter = QSharedPointer<ImageConverter>(new ImageConverter(m_pCamera));
    m_pImageConverter->moveToThread(m_pWorkerThread.data());
    
    connect(    m_pImageConverter.data(), SIGNAL(NewImageAvailable()),
                this, SLOT(NewImageAvailable()));
    
    //Check image converter if a frame was available before connecting
    //signal to the GUI
    if(m_pImageConverter->IsImageAvailable())
    {
        NewImageAvailable();
    }
}

//Destructor for GUI cleanup
MainWindow::~MainWindow()
{
    //Quit the worker thread and wait for thread termination
    m_pWorkerThread->quit();
    m_pWorkerThread->wait();
}

//Helper method to update the zoom text
void MainWindow::UpdateZoomText()
{
    m_pZoomLabel->setText(QString("%1%").arg(QString::number(m_dScaleFactor * 100.0, 'f', 0)));
}

//Helper method to set the zoom
void MainWindow::SetZoom(double dValue)
{
    m_dScaleFactor = dValue;
    UpdateZoomText();

    QTransform transformation;
    transformation.scale(m_dScaleFactor, m_dScaleFactor);
    ui.m_pGraphicsView->setTransform(transformation);
}

//Handler method for zooming in
void MainWindow::ZoomIn()
{
    SetZoom(m_dScaleFactor * ZOOM_FACTOR);
}

//Handler method for zooming out
void MainWindow::ZoomOut()
{
    SetZoom(m_dScaleFactor / ZOOM_FACTOR);
}

//Handler method for setting zoom to 100 percent
void MainWindow::ZoomReset()
{
    SetZoom(1.0);
}

//Handler method for fitting zoom to image
void MainWindow::FitScreen()
{
    ui.m_pGraphicsView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    QApplication::processEvents();
    // fitInView needs to be called twice, since the scrollbars might be visible when calling it first.
    // This would result in s slightly different scale after the 2nd call.
    ui.m_pGraphicsView->fitInView(m_pScene->sceneRect(), Qt::KeepAspectRatio);
    
    QLineF inLine(0.0, 0.0, 1.0, 0.0);
    QLineF outLine = ui.m_pGraphicsView->transform().map(inLine);    
    m_dScaleFactor = outLine.length() / inLine.length();
    UpdateZoomText();
}

//Handle mouse wheel events for zooming in and out (using CTRL key on keyboard)
void MainWindow::wheelEvent(QWheelEvent *pWheelEvent)
{
    if(pWheelEvent->modifiers().testFlag(Qt::ControlModifier))
    {
        SetZoom(m_dScaleFactor * qPow(ZOOM_FACTOR, pWheelEvent->delta() / 120));
        
        pWheelEvent->accept();
    }
    else
    {
        pWheelEvent->ignore();
    }
}

//Update text in status bar (frame counter and frame rate). We update the status
//with every displayed frame or with the timer (whatever comes first).
void MainWindow::UpdateStatus()
{
    bool bStreaming = false;
    uint64_t nFrameCounter = 0;
    double dFrameRate = 0.0;
    m_pCamera->GetStatus(&bStreaming, &nFrameCounter, &dFrameRate);

    //Show if streaming is running or not
    if(true == bStreaming)
    {
        QString strFrameCounter = QString::number(nFrameCounter);
        QString strFrameRate = QString::number(dFrameRate, 'f', 2);
            
        if(0 != m_nPixelFormat)
        {
            QString strWidth = QString::number(m_nWidth);
            QString strHeight = QString::number(m_nHeight);
            const char *pPixelFormat = (const char*)&m_nPixelFormat;
            QString strPixelFormat;
            for(size_t i = 0; i < sizeof(m_nPixelFormat); i++)
            {
                strPixelFormat += pPixelFormat[i];
            }
            m_pStatusLabel->setText(QString("Width: %1 Height: %2 Pixelformat: %3 Frame counter: %4 Frame rate: %5 fps").arg(strWidth).arg(strHeight).arg(strPixelFormat).arg(strFrameCounter).arg(strFrameRate));
        }
        else
        {
            m_pStatusLabel->setText(QString("Frame counter: %1 Frame rate: %2 fps").arg(strFrameCounter).arg(strFrameRate));
        }
    }
    else
    {
        m_pStatusLabel->setText("Streaming stopped");
    }
    
    //Reset timer in case no frames are acquired for a while
    m_pStatusTimer->start(STATUS_UPDATE_INTERVAL);
}

//Timer handler is called only if no frames have been displayed for a while
void MainWindow::StatusTimer()
{
    UpdateStatus();
}

//Images are only displayed if they don't get acquired too quickly and
//if images are available of course.
void MainWindow::UpdateDisplay()
{
    //Try to get new image from image converter
    QPixmap pixmap;
    switch(m_pImageConverter->GetNewImage(pixmap, m_nWidth, m_nHeight, m_nPixelFormat))
    {
    //Ok we got a new frame that we will display now
    case ImageResult_Ok:
        m_pDisplayTimer->start(MIN_DISPLAY_INTERVAL);
        m_pScene->setSceneRect((qreal)0, (qreal)0, (qreal)pixmap.width(), (qreal)pixmap.height());
        m_pPixmapItem->setPixmap(pixmap);
        m_bImageDisplayed = true;
        UpdateStatus();
        break;

    //A new frame was acquired but we were not able to convert the frame, so
    //we only show the company logo instead
    case ImageResult_CouldNotConvert:
        if(true == m_bImageDisplayed)
        {
            m_pDisplayTimer->start(MIN_DISPLAY_INTERVAL);
            m_pScene->setSceneRect((qreal)0, (qreal)0, (qreal)m_DefaultPixmap.width(), (qreal)m_DefaultPixmap.height());
            m_pPixmapItem->setPixmap(m_DefaultPixmap);
            m_bImageDisplayed = false;
        }
        UpdateStatus();
        break;
        
    default:
        break;
    }
}

//Notification slot if a new frame is available
void MainWindow::NewImageAvailable()
{
    //Check if we have to wait for the timer because acquisition frame rate is
    //higher than necessary for image display.
    if(false == m_pDisplayTimer->isActive())
    {
        UpdateDisplay();
    }
}

//Timer handler if a new frame shall be displayed
void MainWindow::DisplayTimer()
{
    //Only show new image if one is available
    if(m_pImageConverter->IsImageAvailable() == true)
    {
        UpdateDisplay();
    }
}

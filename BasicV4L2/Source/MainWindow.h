/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        MainWindow.h

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_MainWindow.h"
#include <QLabel>
#include <QTimer>
#include <QPixmap>
#include <QThread>
#include <list>
#include "Camera.h"

//If we try to get an image from ImageConverter then we can
//either not get a frame, get informed that it was not possible to
//convert the current image
enum ImageResult
{
    ImageResult_None            = 0,
    ImageResult_Ok              = 1,
    ImageResult_CouldNotConvert = 2
};

//Image converter class that will be run in a separate thread to offload
//the conversion from the GUI thread
class ImageConverter : public QObject
{
    Q_OBJECT
    
private:
    //For each buffer we create a QImage and from that
    //a QPixmap, so we have a struct for the two.
    struct Image
    {
        QImage  m_Image;
        QPixmap m_Pixmap;
        __u32	m_nWidth;
        __u32	m_nHeight;
        __u32	m_nPixelFormat;
        bool    m_bConverted;
    };
    QMutex                              m_Mutex;
    QSharedPointer<Camera>              m_pCamera;
    std::list<QSharedPointer<Image> >   m_ImagePool;
    QSharedPointer<Image>               m_pNextImage;
    QSharedPointer<Image>               m_pUsedImage;

private slots:
    void NewBufferAvailable();
        
public:
    ImageConverter(QSharedPointer<Camera> pCamera, QObject *pParent = NULL);
    
    bool IsImageAvailable();
    ImageResult GetNewImage(QPixmap &rPixmap, __u32 &rWidth, __u32 &rHeight, __u32 &rPixelFormat);
    
signals:
    void NewImageAvailable();
};

//Main window that will display the images
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QSharedPointer<Camera> pCamera);
    virtual ~MainWindow();
    
private:
    Ui::MainWindow                      ui;
    QSharedPointer<Camera>              m_pCamera;
    double                              m_dScaleFactor;
    QLabel                              *m_pZoomLabel;
    QLabel                              *m_pStatusLabel;
    QTimer                              *m_pStatusTimer;
    QTimer                              *m_pDisplayTimer;
    QGraphicsScene                      *m_pScene;
    QGraphicsPixmapItem                 *m_pPixmapItem;
    QSharedPointer<Buffer>              m_pCurrentBuffer;
    QPixmap                             m_DefaultPixmap;
    QSharedPointer<QThread>             m_pWorkerThread;
    QSharedPointer<ImageConverter>      m_pImageConverter;
    bool                                m_bImageDisplayed;
    __u32								m_nWidth;
    __u32								m_nHeight;
    __u32								m_nPixelFormat;
    
    void UpdateZoomText();
    void UpdateStatus();
    void UpdateDisplay();
    void SetZoom(double dValue);
    virtual void wheelEvent(QWheelEvent *pWheelEvent);
    
private slots:
    void ZoomIn();
    void ZoomOut();
    void ZoomReset();
    void FitScreen();
    void NewImageAvailable();
    void StatusTimer();
    void DisplayTimer();
};

#endif //MAINWINDOW_H

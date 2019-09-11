/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        MainWindow.cpp

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

#include "MainWindow.h"
#include "Version.h"
#include <QDateTime>
#include <QString>
#include <stdexcept>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

MainWindow::MainWindow(QSharedPointer<Camera> pCamera, QObject *pParent)
    :   QObject(pParent)
    ,   m_pCamera(pCamera)
{
    if(NULL == m_pCamera)
    {
        throw std::runtime_error("No camera passed");
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

QImage Mat2QImage(cv::Mat const& src, const QImage::Format eQtFormat )
{
     //return QImage((uchar*)src.data, src.cols, src.rows, src.step, QImage::Format_RGB888);
    return QImage((uchar*)src.data, src.cols, src.rows, src.step, eQtFormat);
}

void MainWindow::NewBufferAvailable()
{
    //Now we try to get the new buffer from the camera
    QSharedPointer<Buffer> pBuffer = m_pCamera->GetNewBuffer();
    if(NULL == pBuffer)
    {
        return;
    }
    
    m_pCamera->GetStatus(&m_streaming, &m_frameCounter, &m_frameRate);
    QString fpsLabel = QString("%1 fps").arg(m_frameRate, 0, 'f', 2);
    
    // Determine matching Qt image format and cv array type
    QImage::Format eQtFormat = QImage::Format_Invalid;
    int nCvArrayType = 0;
    switch(pBuffer->GetPixelFormat())
    {
        case V4L2_PIX_FMT_RGB24:
        {
            eQtFormat = QImage::Format_RGB888;
            nCvArrayType = CV_8UC3;
        }
        break;

        case V4L2_PIX_FMT_XBGR32:
        {
            eQtFormat = QImage::Format_RGB32;
            nCvArrayType = CV_8UC4;
        }
        break;

        default: throw std::runtime_error("Unsupported pixel format");
    }

    // Create an OpenCV Mat object from the buffer
    cv::Mat image((int)pBuffer->GetHeight(), (int)pBuffer->GetWidth(), nCvArrayType, pBuffer->GetData(), (size_t)pBuffer->GetBytesPerLine());

    // Resize
    cv::resize(image, image, cv::Size(640, 480), 0, 0, cv::INTER_NEAREST);

    // Add text overlay
    cv::putText(image, fpsLabel.toStdString(), cv::Point(32,32), cv::FONT_HERSHEY_COMPLEX, 1.0, cv::Scalar(200,200,250), 1);

    // Display
    m_label.setPixmap(QPixmap::fromImage(Mat2QImage(image, eQtFormat)));
    m_label.show();
}

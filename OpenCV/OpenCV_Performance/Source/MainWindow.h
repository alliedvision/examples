/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        MainWindow.h

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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>

#include <QLabel>
#include <QImage>
#include <QPixmap>
#include "Camera.h"

class MainWindow : public QObject
{
    Q_OBJECT
        
private:
    QSharedPointer<Camera> m_pCamera;
    QLabel m_label;
    bool m_streaming;
    uint64_t m_frameCounter;
    double m_frameRate;
    
private slots:
    void NewBufferAvailable();
    
public:
    MainWindow(QSharedPointer<Camera> pCamera, QObject *pParent = NULL);
};

#endif //MAINWINDOW_H

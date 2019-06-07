#!/bin/bash

# install prerequisites
sudo apt update
sudo apt install build-essential cmake pkg-config -y
sudo apt install libjpeg8-dev libtiff5-dev libjasper-dev libpng12-dev -y
sudo apt install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev -y
sudo apt install libxvidcore-dev libx264-dev -y
sudo apt install libgtk-3-dev -y
sudo apt install libatlas-base-dev gfortran -y
sudo apt install python2.7-dev python3.5-dev -y

# download opencv
mkdir -p opencv
cd opencv
wget -O opencv.zip https://github.com/Itseez/opencv/archive/3.2.0.zip
unzip opencv.zip
wget -O opencv_contrib.zip https://github.com/Itseez/opencv_contrib/archive/3.2.0.zip
unzip opencv_contrib.zip

# build opencv
cd opencv-3.2.0
mkdir -p release
cd release
cmake -DWITH_V4L=ON -DWITH_LIBV4L=ON -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=/usr/local ..
make -j4
sudo make install
sudo ldconfig -v

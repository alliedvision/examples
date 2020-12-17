# Triggering examples for Allied Vision MIPI CSI-2 cameras

## Overview
V4L2 doesn’t provide controls for triggering like in GenICam. To enable hardware and software triggering, Allied Vision provides custom controls defined in libcsi_ioctl.h. As a starting point for your application, we provide examples in C for hardware and software triggering.

## Supported driver and camera firmware
### Supported driver
https://github.com/alliedvision/linux_nvidia_jetson   
Please make sure using version 1.1.0 or higher with JetPack version 4.4 (L4T 32.4.2) or higher.   
### Supported camera firmware
Triggering is supported by firmware version 00.03.00.31919 or higher. To obtain the latest firmware for your Allied Vision MIPI CSI-2 camera, please contact our technical support team:
https://www.alliedvision.com/en/support/contact-support-and-repair.html

## Available I/O controls
All custom ioctl request identifiers and enums for triggering are defined in libcsi_ioctl.h.   
Supported functionalities:
* Hardware trigger through GPIOs (two selectable GPIOs), edge or level activation
* Software trigger through I2C
* Available trigger selector: Single frame start   

Trigger delay is currently not supported.


## Limitations
The NVIDIA Jetson boards have several limitations that impact triggering:
* NVIDIA's video input (VI) unit drivers are optimized to maximize troughput. When working with single frames, this behaviour may result in a higher latency.

* Triggered frames always remain in the queue until all positions in the queue are filled with a frame. For example, it takes three triggers to receive the first triggered image if the driver uses three frame buffers.

## Hardware triggering
To access the GPIOs, see the user guide for your adapter board, section I/O connections.   
https://www.alliedvision.com/en/support/technical-documentation/accessory-documentation.html

## Troubleshooting

* Make sure the firmware and driver support triggering, see above.

* Aborting VIDIOC_DQBUF during triggered acquisition:   
   VIDIOC_DQBUF blocks uninterruptably in kernel mode. Killing is not possible because the signal cannot be delivered while blocked in kernel mode.   
   Workaround:   
   Execute a trigger command to make DQBUF receive a frame and return.
   You can change the trigger configuration while waiting in DQBUF.
   When blocked on hardware trigger DQBUF: Change the trigger source to software in a second thread and generate a trigger event with VIDIOC_TRIGGER_SOFTWARE.

 ## More information about triggering
 This Readme is intended to ease using the examples. For detailed information about triggering, see the latest user guide for your Alvium CSI-2 camera:   
 https://www.alliedvision.com/en/support/technical-documentation/alvium-csi-2-documentation.html




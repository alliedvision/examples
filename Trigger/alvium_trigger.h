/*=============================================================================
  Copyright (C) 2022 Allied Vision Technologies.  All Rights Reserved.

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

  -----------------------------------------------------------------------------

File:        alvium_trigger.h

version:     1.0.0
=============================================================================*/

#ifndef ALVIUM_TRIGGER_H
#define ALVIUM_TRIGGER_H

#include <linux/videodev2.h>


enum v4l2_triggeractivation
{
    V4L2_TRIGGER_ACTIVATION_RISING_EDGE  = 0,
    V4L2_TRIGGER_ACTIVATION_FALLING_EDGE = 1,
    V4L2_TRIGGER_ACTIVATION_ANY_EDGE     = 2,
    V4L2_TRIGGER_ACTIVATION_LEVEL_HIGH   = 3,
    V4L2_TRIGGER_ACTIVATION_LEVEL_LOW    = 4
};

enum v4l2_triggersource
{
    V4L2_TRIGGER_SOURCE_LINE0    = 0,
    V4L2_TRIGGER_SOURCE_LINE1    = 1,
    V4L2_TRIGGER_SOURCE_LINE2    = 2,
    V4L2_TRIGGER_SOURCE_LINE3    = 3,
    V4L2_TRIGGER_SOURCE_SOFTWARE = 4
};

struct v4l2_trigger_status
{
    __u8 trigger_source;                // v4l2_triggersource enum value    
    __u8 trigger_activation;            // v4l2_triggeractivation enum value  
    __u8 trigger_mode_enabled;          // Enable (1) or disable (0) trigger mode
};

struct v4l2_trigger_rate
{
    __u64 frames_per_period;            // Number of frames per period
    __u64 period_sec;                   // Period in seconds
};

/* Trigger mode to ON/OFF */
#define V4L2_CID_TRIGGER_MODE                   (V4L2_CID_CAMERA_CLASS_BASE+47)

/* trigger activation: edge_rising, edge_falling, edge_any, level_high, level_low */
#define V4L2_CID_TRIGGER_ACTIVATION             (V4L2_CID_CAMERA_CLASS_BASE+48)

/* trigger source: software, gpio0, gpio1 */
#define V4L2_CID_TRIGGER_SOURCE                 (V4L2_CID_CAMERA_CLASS_BASE+49)

/* Execute a software trigger */
#define V4L2_CID_TRIGGER_SOFTWARE               (V4L2_CID_CAMERA_CLASS_BASE+50)

/* Camera temperature readout */
#define V4L2_CID_DEVICE_TEMPERATURE             (V4L2_CID_CAMERA_CLASS_BASE+51)

#endif

#include "alvium_trigger.h"
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *const FRAME_BIN_FILE = "frame.bin";

static void exitError( const char *s )
{
    perror( s );
    exit( EXIT_FAILURE );
}

static void usage(const char *name)
{
    fprintf( stderr, "usage: %s [-s <subdevice> ] <device>\n",name);
    exit( 1 );
}

static __u32 get_bytesused(const struct v4l2_buffer * const buf)
{
    if (buf->type == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
    {
        return buf->m.planes[0].bytesused;
    }

    return buf->bytesused;
}

int main( int const argc, char const **argv )
{
    int opt;
    int subdevFd = -1;
    const char *subdevNode = NULL;

    while ((opt = getopt(argc,argv,":s:")) != -1)
    {
        switch (opt) {
            case 's':
                fprintf(stderr,"Using subdev %s\n",optarg);
                subdevNode = optarg;
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    if( optind + 1 != argc )
    {
        usage(argv[0]);
    }

    char const *const devName = argv[optind];
    int const cameraFd = open( devName, O_RDWR, 0 );
    if( cameraFd == -1 )
    {
        exitError( "opening camera" );
    }

    if (subdevNode != NULL) {
        subdevFd = open( subdevNode, O_RDWR, 0 );
        if( subdevFd == -1 )
        {
            exitError( "opening subdev" );
        }
    }

    struct v4l2_capability capability = { 0 };

    if( ioctl( cameraFd, VIDIOC_QUERYCAP, &capability ) == -1 )
    {
        exitError( "VIDIOC_REQBUFS" );
    }

    int bufferType;

    if ( capability.device_caps & V4L2_CAP_VIDEO_CAPTURE_MPLANE )
    {
        bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    }
    else if ( capability.device_caps & V4L2_CAP_VIDEO_CAPTURE )
    {
        bufferType = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    }
    else
    {
        exitError( "Buffer type not supported" );
    }

    // Request one buffer. Note: Some boards like the NVidia Jetson Nano return a minimum number
    // of frames. To work correctly, all frames need to be queued later.
    struct v4l2_requestbuffers reqBufs = {
        .count = 1,
        .type = bufferType,
        .memory = V4L2_MEMORY_MMAP
    };

    if( ioctl( cameraFd, VIDIOC_REQBUFS, &reqBufs ) == -1 )
    {
        exitError( "VIDIOC_REQBUFS" );
    }

    // mmap buffers into application address space
    char *pBuffers[reqBufs.count];
    for( unsigned bufIdx = 0; bufIdx < reqBufs.count; ++bufIdx )
    {
        struct v4l2_buffer buf = {
            .type = bufferType,
            .memory = V4L2_MEMORY_MMAP,
            .index = bufIdx
        };

        struct v4l2_plane planes[VIDEO_MAX_PLANES];

        if (bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        {
            buf.m.planes = planes;
            buf.length = VIDEO_MAX_PLANES;
        }

        if( ioctl( cameraFd, VIDIOC_QUERYBUF, &buf ) == -1 )
        {
            exitError( "VIDIOC_QUERYBUF" );
        }

        if (bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE && buf.length > 1)
        {
            exitError( "Only formats with on plane are supported" );
        }

        if (bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
            pBuffers[bufIdx] =
                mmap( NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraFd, planes[0].m.mem_offset );
        else
        pBuffers[bufIdx] =
            mmap( NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraFd, buf.m.offset );

        if( pBuffers[bufIdx] == MAP_FAILED )
        {
            exitError( "mmap" );
        }
    }

    // Queue buffers. Even though we request only one, some boards cannot handle that. (See above).
    for( unsigned i = 0; i < reqBufs.count; ++i )
    {
        struct v4l2_buffer buf = {
            .type = bufferType,
            .memory = V4L2_MEMORY_MMAP,
            .index = i
        };

        struct v4l2_plane planes[VIDEO_MAX_PLANES];

        if (bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
        {
            buf.m.planes = planes;
            buf.length = VIDEO_MAX_PLANES;
        }

        if( ioctl( cameraFd, VIDIOC_QBUF, &buf ) == -1 )
        {
            exitError( "VIDIOC_QBUF" );
        }
    }

    int const ctrlFd = subdevFd == -1 ? cameraFd : subdevFd;

    // Set trigger mode
    struct v4l2_control enable_trigger = {.id= V4L2_CID_TRIGGER_MODE, .value=1};
    if( ioctl( ctrlFd, VIDIOC_S_CTRL, &enable_trigger ) == -1 )
    {
        exitError( "enabling trigger mode" );
    }

    // Set trigger source
    int const source = V4L2_TRIGGER_SOURCE_LINE0;
    struct v4l2_control set_trigger_source = {.id=V4L2_CID_TRIGGER_SOURCE, .value=source};
    if( ioctl(ctrlFd, VIDIOC_S_CTRL, &set_trigger_source) == -1 )
    {
        exitError( "setting trigger source" );
    }

    // Set trigger activation
    int const activation = V4L2_TRIGGER_ACTIVATION_RISING_EDGE;
    // int const activation = V4L2_TRIGGER_ACTIVATION_FALLING_EDGE;
    // int const activation = V4L2_TRIGGER_ACTIVATION_ANY_EDGE;
    // int const activation = V4L2_TRIGGER_ACTIVATION_LEVEL_HIGH;
    // int const activation = V4L2_TRIGGER_ACTIVATION_LEVEL_LOW;
    struct v4l2_control set_trigger_activation = {.id=V4L2_CID_TRIGGER_ACTIVATION, .value=activation};
    if( ioctl(ctrlFd, VIDIOC_S_CTRL, &set_trigger_activation) == -1 )
    {
        exitError( "setting trigger activation" );
    }


    // Start stream

    if( ioctl( cameraFd, VIDIOC_STREAMON, &bufferType ) == -1 )
    {
        exitError( "VIDIOC_STREAMON" );
    }

    // Wait for trigger and dequeue triggered buffer
    struct v4l2_buffer buf = {
        .type = bufferType,
        .memory = V4L2_MEMORY_MMAP
    };

    struct v4l2_plane planes[VIDEO_MAX_PLANES];

    if (bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
    {
        buf.m.planes = planes;
        buf.length = VIDEO_MAX_PLANES;
    }

    if( ioctl( cameraFd, VIDIOC_DQBUF, &buf ) == -1 )
    {
        exitError( "VIDIOC_DQBUF" );
    }

    // Save captured frame to file
    printf( "Frame captured, saving to file...\n" );

    int const outputFd = open( FRAME_BIN_FILE, O_WRONLY | O_CREAT, 0644 );
    if( outputFd == -1 )
    {
        exitError( "open output file" );
    }

    if( write( outputFd, pBuffers[buf.index], get_bytesused(&buf) ) == -1 )
    {
        exitError( "writing frame to file" );
    }

    printf( "Captured frame written to %s\n", FRAME_BIN_FILE );

    // stop capture
    if( ioctl( cameraFd, VIDIOC_STREAMOFF, &bufferType ) == -1 )
    {
        exitError( "VIDIOC_STREAMOFF" );
    }

    // For simplicity of the example, we let the system implicitly munmap
    // buffers and close files at exit

    return 0;
}

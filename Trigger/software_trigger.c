#include "v4l2-avt-ioctl.h"
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

int main( int const argc, char const **argv )
{
    if( argc < 2 )
    {
        fprintf( stderr, "usage: %s <device>\n", argv[0] );
        exit( 1 );
    }

    char const *const devName = argv[1];
    int const cameraFd = open( devName, O_RDWR, 0 );
    if( cameraFd == -1 )
    {
        exitError( "opening camera" );
    }

    // Request one buffer. Note: Some boards like the NVidia Jetson Nano return a minimum number
    // of frames. To work correctly, all frames need to be queued later.
    struct v4l2_requestbuffers reqBufs = {
        .count = 1,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
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
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = bufIdx
        };

        if( ioctl( cameraFd, VIDIOC_QUERYBUF, &buf ) == -1 )
        {
            exitError( "VIDIOC_QUERYBUF" );
        }

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
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
            .index = i
        };

        if( ioctl( cameraFd, VIDIOC_QBUF, &buf ) == -1 )
        {
            exitError( "VIDIOC_QBUF" );
        }
    }

    // Set trigger mode
    if( ioctl( cameraFd, VIDIOC_TRIGGER_MODE_ON ) == -1 )
    {
        exitError( "enabling trigger mode" );
    }

    // Set trigger source to software
    int const source = V4L2_TRIGGER_SOURCE_SOFTWARE;
    if( ioctl( cameraFd, VIDIOC_S_TRIGGER_SOURCE, &source ) == -1 )
    {
        exitError( "setting trigger source to software" );
    }

    // Start stream
    int const type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if( ioctl( cameraFd, VIDIOC_STREAMON, &type ) == -1 )
    {
        exitError( "VIDIOC_STREAMON" );
    }

    // Pre-trigger frames (NVidia VI driver limiation workaround)
    for( unsigned i = 0; i < reqBufs.count; i++ )
    {
        if( ioctl( cameraFd, VIDIOC_TRIGGER_SOFTWARE ) == -1 )
        {
            exitError( "VIDIOC_TRIGGER_SOFTWARE" );
        }

        // Don't pre-trigger too fast
        usleep( 20000 );
    }

    // Wait for user to request a software trigger
    printf( "start trigger with [enter]" );
    fflush( stdout );
    getc( stdin );

    if( ioctl( cameraFd, VIDIOC_TRIGGER_SOFTWARE ) == -1 )
    {
        exitError( "VIDIOC_TRIGGER_SOFTWARE" );
    }

    // Dequeue triggered buffer
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };

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

    if( write( outputFd, pBuffers[buf.index], buf.bytesused ) == -1 )
    {
        exitError( "writing frame to file" );
    }

    printf( "Captured frame written to %s\n", FRAME_BIN_FILE );

    // stop capture
    if( ioctl( cameraFd, VIDIOC_STREAMOFF, &type ) == -1 )
    {
        exitError( "VIDIOC_STREAMOFF" );
    }

    // For simplicity of the example, we let the system implicitly munmap
    // buffers and close files at exit

    return 0;
}

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdbool.h>

static void exitError(const char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

static bool keepStreaming = true;
static void sigint(int) {
    keepStreaming = false;
}

int main(int const argc, char const **argv) {
    if(argc < 2) {
        fprintf(stderr, "usage: %s <device>\n", argv[0]);
        exit(1);
    }

    char const *const devName = argv[1];
    int const cameraFd = open( devName, O_RDWR, 0 );
    if(cameraFd == -1) {
        exitError("opening camera");
    }

    // Query device capabilities to distinguish single- and multiplane devices
    struct v4l2_capability cap;
    if(-1 == ioctl(cameraFd, VIDIOC_QUERYCAP, &cap))                                                                       {
        exitError("VIDIOC_QUERYCAP");
    }                                                    

    enum v4l2_buf_type const bufferType = cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE
        ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE : V4L2_BUF_TYPE_VIDEO_CAPTURE;

    // Request buffers for streaming
    struct v4l2_requestbuffers reqBufs = {
        .count = 4,
        .type = bufferType,
        .memory = V4L2_MEMORY_MMAP
    };

    if(ioctl(cameraFd, VIDIOC_REQBUFS, &reqBufs) == -1) {
        exitError( "VIDIOC_REQBUFS" );
    }

    // Map buffers into process memory and queue frames
    struct v4l2_plane plane;
    char *buffers[reqBufs.count];
    for(unsigned bufIdx = 0; bufIdx < reqBufs.count; ++bufIdx) {
        struct v4l2_buffer buf = {
            .type = bufferType,
            .index = bufIdx,
            .memory = V4L2_MEMORY_MMAP
        };

        if(bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.m.planes = &plane;
            buf.length = 1;
        }

        int res = ioctl(cameraFd, VIDIOC_QUERYBUF, &buf);
        if(res < 0) {
            printf("%d\n", res);
            exitError( "VIDIOC_QUERYBUF" );
        }

        buffers[bufIdx] = mmap( NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, cameraFd, bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE ? buf.m.planes[0].m.mem_offset : buf.m.offset);

        if(buffers[bufIdx] == MAP_FAILED) {
            exitError("mmap");
        }

        if(ioctl( cameraFd, VIDIOC_QBUF, &buf ) == -1) {
            exitError("VIDIOC_QBUF");
        }
    }

    // Start stream
    if(ioctl(cameraFd, VIDIOC_STREAMON, &bufferType) == -1) {
        exitError("VIDIOC_STREAMON");
    }

    signal(SIGINT, &sigint);
    printf("Streaming (Ctrl-C to quit): ");
    fflush(stdout);
    while(keepStreaming) {
        struct v4l2_buffer buf = {
            .type = bufferType,
            .memory = V4L2_MEMORY_MMAP
        };

        if(bufferType == V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
            buf.m.planes = &plane;
            buf.length = 1;
        }

        // Wait for frame and dequeue associated buffer
        if(ioctl(cameraFd, VIDIOC_DQBUF, &buf ) == -1) {
            exitError("VIDIOC_DQBUF");
        }

        // Frame data is available in buffers[buf.index]!

        printf("*");
        fflush(stdout);

        // Re-queue frame after we're done with it
        if(ioctl(cameraFd, VIDIOC_QBUF, &buf) == -1) {
            exitError("VIDIOC_QBUF");
        }
    }

    printf("\nDone\n");

    // stop capture
    if(ioctl(cameraFd, VIDIOC_STREAMOFF, &bufferType) == -1) {
        exitError("VIDIOC_STREAMOFF");
    }

    // For simplicity of the example, we let the system implicitly munmap
    // buffers and close files at exit

    return 0;
}

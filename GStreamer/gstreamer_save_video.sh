#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
OUTPUT=capture.avi

# ========================================================= 
# Usage function
# ========================================================= 

usage() {
    echo -e "Will save the videostream from v4l2 to an avi file encoded with h264."
    echo -e "Usage: ./gstreamer_save_video.sh -d <device> [-o <output-file>]\n"
    echo -e "Options:"
    echo -e "-d, --device   Device to use, e.g. -d /dev/video3"
    echo -e "-h, --help     Display help"
    echo -e "-o, --output   Output file"
}

# ========================================================= 
# Parse command line parameters
# ========================================================= 

POSITIONAL=()
while [[ $# -gt 0 ]]
do
    key="$1"

    case $key in
        -d|--device)
            DEVICE="$2"
            shift
            shift
            ;;
        -h|--help)
            usage
            exit
            ;;
        -o|--output)
            OUTPUT="$2"
            shift
            shift
            ;;
        *)
            echo "Unknown parameter: $1"
            usage
            exit 1
            ;;
    esac
done

set -- "${POSITIONAL[@]}"


# ========================================================= 
# gstreamer command
# ========================================================= 

if [ "$DEVICE" = 0 ]; then
    echo -e "No device specified. Exit.\n"
    usage
    exit 1
fi

echo "Using device" $DEVICE
echo -e "Stop capture by hitting Ctrl+C once. Please wait until gstreamer command is completed.\n"

if ! gst-launch-1.0 -v $VIDEOSRC device=$DEVICE ! videoscale ! videoconvert ! video/x-raw,width=800,height=600,format=RGB ! videoconvert ! videorate ! video/x-raw,framerate=30/1 ! x264enc ! avimux ! filesink location="$OUTPUT" -e
then
    echo -e "\nFailed to launch gstreamer. Is the right device specified?"
fi

# ,format=\(fourcc\)RGB

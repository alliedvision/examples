#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
OUTPUT=capture.avi
BOARD=0
PIPELINE="echo -e \"Pipeline not defined.\""

NITROGEN=nitrogen
NVIDIA=nvidia
WANDBOARD=wandboard

# ========================================================= 
# Usage function
# ========================================================= 

usage() {
    echo -e "Will save the videostream from v4l2 to an avi file encoded with h264."
    echo -e "Usage: ./gstreamer_save_video.sh -d <device> -b <board> [-o <output-file>]\n"
    echo -e "Options:"
    echo -e "-b, --board    Currently used board. e.g. -b $NVIDIA"
    echo -e "               Options:"
    echo -e "                   $NITROGEN  for Nitrogen  boards"
    echo -e "                   $NVIDIA    for NVIDIA    boards"
    echo -e "                   $WANDBOARD for Wandboard boards"
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
        -b|--board)
            BOARD="$2"
            shift
            shift
            ;;
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

if [ "$BOARD" = 0 ]; then
    echo -e "No board specified. Exit.\n"
    usage
    exit 1
fi

if ! [[ "$BOARD" =~ ^($NITROGEN|$NVIDIA|$WANDBOARD)$ ]]; then
    echo -e "Unsupported board specified. Exit.\n"
    usage
    exit 1
fi

echo "Using device" $DEVICE
echo "Using board" $BOARD
echo "Output file" $OUTPUT

echo -e "Stop capture by hitting Ctrl+C once. Please wait until gstreamer command is completed.\n"

if [ "$BOARD" = "$NVIDIA" ]; then
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE ! video/x-raw,format=BGRx ! videoscale ! video/x-raw,width=800,height=600 ! videoconvert ! videorate ! video/x-raw,framerate=30/1 ! x264enc ! avimux ! filesink location="$OUTPUT" -e"
else
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE ! videoscale ! videoconvert ! video/x-raw,width=800,height=600,format=RGB ! videoconvert ! videorate ! video/x-raw,framerate=30/1 ! x264enc ! avimux ! filesink location="$OUTPUT" -e"
fi

if ! eval "$PIPELINE"; then
    echo -e "\nFailed to launch gstreamer. Is the right device specified?"
fi

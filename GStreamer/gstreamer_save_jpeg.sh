#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
OUTPUT=./out.jpeg
BOARD=0
PIPELINE="echo -e \"Pipeline not defined.\""

NITROGEN=nitrogen
NVIDIA=nvidia
WANDBOARD=wandboard
APALIS_IMX8=apalis_imx8

# ========================================================= 
# Usage function
# ========================================================= 

usage() {
	echo -e "Will save a frame from v4l2 to a jpeg file"
	echo -e "Usage: ./gstreamer_live.sh -d <device> -b <board> [-o <output-jpg>]\n"
	echo -e "Options:"
    echo -e "-b, --board    Currently used board. e.g. -b $NVIDIA"
    echo -e "               Options:"
    echo -e "                   $NITROGEN  for Nitrogen  boards"
    echo -e "                   $NVIDIA    for NVIDIA    boards"
    echo -e "                   $WANDBOARD for Wandboard boards"
    echo -e "                   $APALIS_IMX8 for Apalis iMX8 boards"
	echo -e "-d, --device   Device to use, e.g. -d /dev/video3"
	echo -e "-h             Display help"
	echo -e "-o, --output   Output filepath. e.g. -o out.jpeg"
}

if [ "$SUCCESS" = false ]; then
    usage
    exit
fi

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

if ! [[ "$BOARD" =~ ^($NITROGEN|$NVIDIA|$APALIS_IMX8|$WANDBOARD)$ ]]; then
    echo -e "Unsupported board specified. Exit.\n"
    usage
    exit 1
fi

echo "Using device" $DEVICE
echo "Using board" $BOARD
echo "Output file" $OUTPUT

if [ "$BOARD" = "$NVIDIA" ]; then
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE num-buffers=1 ! queue ! video/x-raw,format=BGRx ! jpegenc ! filesink location=$OUTPUT"
else
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE num-buffers=1 ! queue ! video/x-raw,format=RGB ! jpegenc ! filesink location=$OUTPUT"
fi

if ! eval "$PIPELINE"; then
    echo -e "\nFailed to launch gstreamer. Is the right device specified?"
fi

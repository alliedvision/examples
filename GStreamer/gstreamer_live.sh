#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
VIDEOSINK=ximagesink
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
    echo -e "Usage: ./gstreamer_live.sh -d <device> -b <board>\n"
	echo -e "Options:"	
    echo -e "-b, --board    Currently used board. e.g. -b $NVIDIA"
    echo -e "               Options:"
    echo -e "                   $NITROGEN  for Nitrogen  boards"
    echo -e "                   $NVIDIA    for NVIDIA    boards"
    echo -e "                   $WANDBOARD for Wandboard boards"
    echo -e "-d, --device   Device to use. e.g. -d /dev/video3"
	echo -e "-h, --help     Display help"
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

if ! [[ "$BOARD" =~ ^($NITROGEN|$NVIDIA|$WANDBOARD|$APALIS_IMX8)$ ]]; then
    echo -e "Unsupported board specified. Exit.\n"
    usage
    exit 1
fi

if [ "$BOARD" = "$APALIS_IMX8" ]; then
    VIDEOSINK=waylandsink
fi

echo "Using device" $DEVICE
echo "Using board" $BOARD

if [ "$BOARD" = "$NVIDIA" ]; then
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE ! video/x-raw, format=BGRx ! $VIDEOSINK"
else
    PIPELINE="gst-launch-1.0 $VIDEOSRC device=$DEVICE ! $VIDEOSINK"
fi

eval "$PIPELINE"

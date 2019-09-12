#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
VIDEOSINK=ximagesink

# ========================================================= 
# Usage function
# ========================================================= 

usage() {
    echo -e "Usage: ./gstreamer_live.sh -d <device>\n"
	echo -e "Options:"	
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

echo "Using device" $DEVICE
if ! gst-launch-1.0 $VIDEOSRC device=$DEVICE ! video/x-raw, format=BGRx ! videoscale ! video/x-raw,width=500,height=375 ! videoconvert ! $VIDEOSINK
then
    echo -e "\nFailed to launch gstreamer. Is the right device specified?"
fi

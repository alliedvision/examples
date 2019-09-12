#!/bin/bash

# ========================================================= 
# Variables
# =========================================================

DEVICE=0
VIDEOSRC=v4l2src
OUTPUT=./out.jpeg

# ========================================================= 
# Usage function
# ========================================================= 

usage() {
	echo -e "Will save a frame from v4l2 to a jpeg file"
	echo -e "Usage: ./gstreamer_live.sh -d <device> [-o <output-jpg>]\n"
	echo -e "Options:"
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
echo "Output file" $OUTPUT

if ! gst-launch-1.0 $VIDEOSRC device=$DEVICE num-buffers=1 ! queue ! video/x-raw,format=BGRx ! jpegenc ! filesink location=$OUTPUT
then
   echo -e "\nFailed to launch gstreamer. Is the right device specified?"
fi

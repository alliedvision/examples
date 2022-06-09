include $(MAKE_INCLUDE_DIR)/Common.mk

#Compile options needed for OpenCV
OPENCV_CFLAGS       = $(shell $(PKGCFG) --cflags opencv4)

#Linker options needed for OpenCV
OPENCV_LIBS         = $(shell $(PKGCFG) --libs opencv4)

#Operations we have to do in order to prepare OpenCV
OpenCV:

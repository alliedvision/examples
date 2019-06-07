include $(MAKE_INCLUDE_DIR)/Common.mk

#Compile options needed for OpenCV
OPENCV_CFLAGS       = $(shell $(PKGCFG) --cflags opencv)

#Linker options needed for OpenCV
OPENCV_LIBS         = $(shell $(PKGCFG) --libs opencv)

#Operations we have to do in order to prepare OpenCV
OpenCV:
PROJECT_NAME            = OpenCV_Basic

PROJECT_DIR             = ../..
MAKE_INCLUDE_DIR        = $(CURDIR)

include $(MAKE_INCLUDE_DIR)/Common.mk

CONFIG_DIR          = $(ARCH)_$(WORDSIZE)bit
BIN_FILE            = $(PROJECT_NAME)
BIN_DIR             = binary/$(CONFIG_DIR)
OBJ_DIR             = object/$(CONFIG_DIR)
BIN_PATH            = $(BIN_DIR)/$(BIN_FILE)

all: $(BIN_PATH)

include $(MAKE_INCLUDE_DIR)/OpenCV.mk

SOURCE_DIR          = $(PROJECT_DIR)/Source

INCLUDE_DIRS        = -I$(SOURCE_DIR) \
		      -I$(OBJ_DIR)

LIBS                = $(OPENCV_LIBS)

DEFINES             =

CFLAGS              = $(COMMON_CFLAGS) \
                      $(OPENCV_CFLAGS)

OBJ_FILES           = $(OBJ_DIR)/OpenCV_Basic.o

DEPENDENCIES        = OpenCV

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp $(OBJ_DIR)
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(OBJ_DIR)/%.cpp $(OBJ_DIR)
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) -o $@ $<

$(BIN_PATH): $(DEPENDENCIES) $(OBJ_FILES) $(BIN_DIR)
	$(CXX) $(ARCH_CFLAGS) -o $(BIN_PATH) $(OBJ_FILES) $(LIBS) -Wl,-rpath,'$$ORIGIN'

clean:
	$(RM) binary -r -f
	$(RM) object -r -f

$(OBJ_DIR):
	$(MKDIR) -p $(OBJ_DIR)

$(BIN_DIR):
	$(MKDIR) -p $(BIN_DIR)

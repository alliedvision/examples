PROJECT_NAME            = BasicDemo

PROJECT_DIR             = ../..
MAKE_INCLUDE_DIR        = $(CURDIR)

include $(MAKE_INCLUDE_DIR)/Common.mk

CONFIG_DIR          = $(ARCH)_$(WORDSIZE)bit
BIN_FILE            = $(PROJECT_NAME)
BIN_DIR             = binary/$(CONFIG_DIR)
OBJ_DIR             = object/$(CONFIG_DIR)
BIN_PATH            = $(BIN_DIR)/$(BIN_FILE)

all: $(BIN_PATH)

include $(MAKE_INCLUDE_DIR)/Qt.mk

SOURCE_DIR          = $(PROJECT_DIR)/Source

INCLUDE_DIRS        = -I$(SOURCE_DIR) \
		      -I$(OBJ_DIR)

LIBS                = $(QTCORE_LIBS) \
		      $(QTGUI_LIBS)

DEFINES             =

CFLAGS              = $(COMMON_CFLAGS) \
                      $(QTCORE_CFLAGS) \
                      $(QTGUI_CFLAGS)

OBJ_FILES           = $(OBJ_DIR)/Camera.o \
					  $(OBJ_DIR)/main.o \
					  $(OBJ_DIR)/MainWindow.o \
					  $(OBJ_DIR)/moc_Camera.o \
					  $(OBJ_DIR)/moc_MainWindow.o \
					  $(OBJ_DIR)/qrc_MainWindow.o

GEN_HEADERS			= $(OBJ_DIR)/ui_MainWindow.h

#DEPENDENCIES        = QtCore \
#		      QtGui

$(OBJ_DIR)/moc_%.cpp: $(SOURCE_DIR)/%.h $(OBJ_DIR)
	$(MOC) -o $@ $<

$(OBJ_DIR)/ui_%.h: $(SOURCE_DIR)/%.ui $(OBJ_DIR)
	$(UIC) -o $@ $<

$(OBJ_DIR)/qrc_%.cpp: $(SOURCE_DIR)/%.qrc $(OBJ_DIR)
	$(RCC) -o $@ $<

$(OBJ_DIR)/%.o: $(SOURCE_DIR)/%.cpp $(OBJ_DIR) $(GEN_HEADERS)
	$(CXX) -c $(INCLUDE_DIRS) $(DEFINES) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%.o: $(OBJ_DIR)/%.cpp $(OBJ_DIR) $(GEN_HEADERS)
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

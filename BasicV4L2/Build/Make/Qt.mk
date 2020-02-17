include $(MAKE_INCLUDE_DIR)/Common.mk

PKGCFG_MOC			= $(shell $(PKGCFG) --variable=moc_location QtCore)
PKGCFG_UIC			= $(shell $(PKGCFG) --variable=uic_location QtCore)

#Qt tools
MOC          		= $(if $(PKGCFG_MOC),$(PKGCFG_MOC),moc)
UIC           		= $(if $(PKGCFG_UIC),$(PKGCFG_UIC),uic)
RCC             	= rcc

#Compile options needed for QtCore
QTCORE_CFLAGS       = $(shell $(PKGCFG) --cflags QtCore)

#Linker options needed for QtCore
QTCORE_LIBS         = $(shell $(PKGCFG) --libs QtCore)

#Compile options needed for QtGui
QTGUI_CFLAGS        = $(shell $(PKGCFG) --cflags QtGui)

#Linker options needed for QtGui
QTGUI_LIBS          = $(shell $(PKGCFG) --libs QtGui)

#Compile options needed for QtSvg
QTSVG_CFLAGS        = $(shell $(PKGCFG) --cflags QtSvg)

#Linker options needed for QtSvg
QTSVG_LIBS          = $(shell $(PKGCFG) --libs QtSvg)

#Operations we have to do in order to prepare QtCore
QtCore:

#Operations we have to do in order to prepare QtGui
QtGui:

#Operations we have to do in order to prepare QtSvg
QtSvg:
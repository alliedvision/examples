UNAME		= $(shell uname -m)

ifeq ($(UNAME),i386)
ARCH		= x86
AUTOWORDSIZE	= 32
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),i486)
ARCH		= x86
AUTOWORDSIZE	= 32
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),i586)
ARCH		= x86
AUTOWORDSIZE	= 32
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),i686)
ARCH		= x86
AUTOWORDSIZE	= 32
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),x86_64)
ARCH		= x86
AUTOWORDSIZE	= 64
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),amd64)
ARCH		= x86
AUTOWORDSIZE	= 64
AUTOFLOATABI	= ignore
endif
ifeq ($(UNAME),armv6l)
ARCH		= arm
AUTOWORDSIZE	= 32
AUTOFLOATABI	= hard
endif
ifeq ($(UNAME),armv7l)
ARCH		= arm
AUTOWORDSIZE	= 32
AUTOFLOATABI	= hard
endif
ifeq ($(UNAME),aarch64)
ARCH		= arm
AUTOWORDSIZE	= 64
AUTOFLOATABI	= hard
endif

#Possible word sizes: 32, 64
WORDSIZE        = $(AUTOWORDSIZE)
#Possible float abis: soft, hard
FLOATABI	= $(AUTOFLOATABI)

ifneq ($(WORDSIZE),32)
ifneq ($(WORDSIZE),64)
$(error Invalid word size set)
endif
endif

ifneq ($(FLOATABI),soft)
ifneq ($(FLOATABI),hard)
ifneq ($(FLOATABI),ignore)
$(error Invalid float abi set)
endif
endif
endif

#Common tools
PKGCFG          = pkg-config
MKDIR           = mkdir
RM              = rm
CXX             = g++
MAKE            = make
CP              = cp

#Set word size on x86
ifeq ($(ARCH),x86)
ARCH_CFLAGS     = -m$(WORDSIZE)
endif

#Configure compiler and linker for soft or hard-float build on ARM
ifeq ($(ARCH),arm)
ifeq ($(FLOATABI),soft)
ARCH_CFLAGS	= -marm -mfloat-abi=soft -march=armv4t
else ifeq ($(FLOATABI),hard)
ifeq ($(WORDSIZE),32)
ARCH_CFLAGS	= -mthumb -mfloat-abi=hard -march=armv7
else ifeq ($(WORDSIZE),64)
ARCH_CFLAGS	= -march=armv8-a
endif
endif
endif

ifeq ($(CONFIG),Debug)
  CONFIG_CFLAGS         = -O0 -g
else
  CONFIG_CFLAGS         = -O3
endif

COMMON_CFLAGS   = $(CONFIG_CFLAGS) $(ARCH_CFLAGS) -fPIC

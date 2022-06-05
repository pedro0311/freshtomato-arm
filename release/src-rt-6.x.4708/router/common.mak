ifeq ($(SRCBASE),)
ifeq ($(TCONFIG_BCMARM),y)
	# ..../src/router/
	# (directory of the last (this) makefile)
	export TOP := $(shell cd $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))) && pwd)

	# ..../src/
	export SRCBASE := $(shell (cd $(TOP)/.. && pwd))
	export SRCBASEDIR := $(shell (cd $(TOP)/.. && pwd | sed 's/.*release\///g'))
else ifneq ($(CONFIG_BCMWL6)$(TCONFIG_BLINK),)
	# ..../src/router/
	# (directory of the last (this) makefile)
	# src or src-rt, regardless of symlink for router directory.
	export TOP := $(shell cd $(dir $(lastword $(MAKEFILE_LIST))) && pwd -P)
	export TOP := $(PWD)/$(notdir $(TOP))
	# ..../src/
	export SRCBASE := $(shell (cd $(TOP)/.. && pwd -P))
else
	# ..../src/router/
	# (directory of the last (this) makefile)
	export TOP := $(shell cd $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))) && pwd -P)
	# ..../src/
	export SRCBASE := $(shell (cd $(TOP)/.. && pwd -P))
endif # TCONFIG_BCMARM/CONFIG_BCMWL6 or TCONFIG_BLINK
else
	export TOP := $(SRCBASE)/router
endif # !SRCBASE

include $(SRCBASE)/tomato_profile.mak
include $(TOP)/.config

export BUILD := $(shell (gcc -dumpmachine))
export HOSTCC := gcc

ifeq ($(TCONFIG_BCMARM),y)
export PLATFORM := arm-uclibc
export CROSS_COMPILE := arm-brcm-linux-uclibcgnueabi-
export CROSS_COMPILER := $(CROSS_COMPILE)
export CONFIGURE := ./configure --host=arm-linux --build=$(BUILD)
export HOSTCONFIG := linux-armv4
ifeq ($(CONFIG_BCM7),y)
export BCMEX := _arm_7
else
export BCMEX := _arm
endif
export EXTRA_FLAG := -lgcc_s
export ARCH := arm
export HOST := arm-linux
else
export PLATFORM := mipsel-uclibc
export CROSS_COMPILE := mipsel-uclibc-
export CROSS_COMPILER := $(CROSS_COMPILE)
export CONFIGURE := ./configure --host=mipsel-linux --build=$(BUILD)
export HOSTCONFIG := linux-mipsel
export ARCH := mips
export HOST := mipsel-linux
endif

export PLT := $(ARCH)
export TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)strip))/.. && pwd -P)

export CC := $(CROSS_COMPILE)gcc
export CXX := $(CROSS_COMPILE)g++
export AR := $(CROSS_COMPILE)ar
export AS := $(CROSS_COMPILE)as
export LD := $(CROSS_COMPILE)ld
export NM := $(CROSS_COMPILE)nm
export OBJCOPY := $(CROSS_COMPILE)objcopy
export OBJDUMP := $(CROSS_COMPILE)objdump
export RANLIB := $(CROSS_COMPILE)ranlib
ifeq ($(TCONFIG_BCMARM),y)
export STRIP := $(CROSS_COMPILE)strip
else
export STRIP := $(CROSS_COMPILE)strip -R .note -R .comment
endif
export SIZE := $(CROSS_COMPILE)size

include $(SRCBASE)/target.mak

# Determine kernel version
ifeq ($(TCONFIG_BCMARM),y)
SCMD=sed -e 's,[^=]*=[        ]*\([^  ]*\).*,\1,'
KVERSION:=	$(shell grep '^VERSION[ 	]*=' $(LINUXDIR)/Makefile|$(SCMD))
KPATCHLEVEL:=	$(shell grep '^PATCHLEVEL[ 	]*=' $(LINUXDIR)/Makefile|$(SCMD))
KSUBLEVEL:=	$(shell grep '^SUBLEVEL[ 	]*=' $(LINUXDIR)/Makefile|$(SCMD))
KEXTRAVERSION:=	$(shell grep '^EXTRAVERSION[ 	]*=' $(LINUXDIR)/Makefile|$(SCMD))
LINUX_KERNEL=$(KVERSION).$(KPATCHLEVEL).$(KSUBLEVEL)$(KEXTRAVERSION)
LINUX_KERNEL_VERSION=$(shell expr $(KVERSION) \* 65536 + $(KPATCHLEVEL) \* 256 + $(KSUBLEVEL))
ifeq ($(LINUX_KERNEL),)
$(error Empty LINUX_KERNEL variable)
endif
else # TCONFIG_BCMARM
kver=$(subst ",,$(word 3, $(shell grep "UTS_RELEASE" $(LINUXDIR)/include/linux/$(1))))
LINUX_KERNEL=$(call kver,version.h)
ifeq ($(LINUX_KERNEL),)
LINUX_KERNEL=$(call kver,utsrelease.h)
endif
endif # TCONFIG_BCMARM

export LIBDIR := $(TOOLCHAIN)/lib
export USRLIBDIR := $(TOOLCHAIN)/usr/lib

export PLATFORMDIR := $(TOP)/$(PLATFORM)
export INSTALLDIR := $(PLATFORMDIR)/install
export TARGETDIR := $(PLATFORMDIR)/target
export STAGEDIR := $(PLATFORMDIR)/stage

ifeq ($(EXTRACFLAGS),)
ifeq ($(TCONFIG_BCMARM),y)
export EXTRACFLAGS := -DBCMWPA2 -DBCMARM -fno-delete-null-pointer-checks -marm -march=armv7-a -mtune=cortex-a9
else
export EXTRACFLAGS := -DBCMWPA2 -fno-delete-null-pointer-checks $(if $(TCONFIG_MIPSR2),-march=mips32r2 -mips32r2 -mtune=mips32r2,-march=mips32 -mips32 -mtune=mips32)
endif
endif
ifeq ($(TCONFIG_BCMARM),y)
export EXTRACFLAGS += -DLINUX_KERNEL_VERSION=$(LINUX_KERNEL_VERSION)
endif

CPTMP = @[ -d $(TOP)/dbgshare ] && cp $@ $(TOP)/dbgshare/ || true

export KERNELCC := $(CC)
export KERNELLD := $(LD)

#	ifneq ($(STATIC),1)
#	SIZECHECK = @$(SRCBASE)/btools/sizehistory.pl $@ $(TOMATO_PROFILE_L)_$(notdir $@)
#	else
SIZECHECK = @$(SIZE) $@
#	endif

export PKG_CONFIG_DIR=
export PKG_CONFIG_LIBDIR=$(SRCBASE)
export PKG_CONFIG_SYSROOT_DIR=$(SRCBASE)

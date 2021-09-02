#
# Toplevel Makefile for the BCM947xx Linux Router release
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.53 2005/04/25 03:54:37 tallest Exp $
#

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#
# To rebuild everything and all configurations:
#  make distclean
#  make V1=whatever V2=sub-whatever VPN=vpn3.6 a b c d m n o
#
# The 1st "whatever" would be the build number - use only when you want the build number to be added 
# to the tomato version; the sub-whatever would be the version note (both optional).
#
# Example:
# make V1=8516 V2="my_build" a b c d m s n o
#
# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


export ac_cv_func_malloc_0_nonnull=yes
export SRCBASE := $(shell pwd)
export SRCBASEDIR := $(shell pwd | sed 's/.*release\///g')
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)
export TPROFILE := N
export PATH := $(PATH):$(PWD)/toolchains/hndtools-arm-linux-2.6.36-uclibc-4.5.3/bin

include ./target.mak

export LD_LIBRARY_PATH := $(SRCBASE)/toolchains/hndtools-arm-linux-2.6.36-uclibc-4.5.3/lib

CTAGS_EXCLUDE_OPT := --exclude=kernel_header --exclude=$(PLATFORM)
CTAGS_DEFAULT_DIRS := $(SRCBASE)/router/rc $(SRCBASE)/router/httpd $(SRCBASE)/src/router/shared $(SRCBASE)/src/router/www

V1 ?= "--def"
VPN ?= "VPN"
ND = "K26ARM"

PPTPD ?= "n"

ifeq ($(NVRAM_SIZE),)
NVRAM_SIZE = 0
endif

ifeq ($(ASUS_TRX),)
ASUS_TRX = 0
endif

ifeq ($(NETGEAR_CHK),)
NETGEAR_CHK = 0
else
WNRTOOL=$(SRCBASE)/wnrtool
BOARD_FILE=$(WNRTOOL)/$(BOARDID_FILE)
FW_FILE=$(WNRTOOL)/$(FW_CFG_FILE)
endif

ifeq ($(XIAOMI_TRX),)
XIAOMI_TRX = 0
endif

ifeq ($(NVRAM_64K),y)
EXTRA_64KDESC = ' -64K'
EXTRA_64KCFLAG = '-DTCONFIG_NVRAM_64K'
else
ifeq ($(NVRAM_128K),y)
EXTRA_64KDESC = '-128K'
EXTRA_64KCFLAG = '-DTCONFIG_NVRAM_128K'
else
EXTRA_64KDESC = ''
EXTRA_64KCFLAG = ''
endif
endif

KERNEL_BINARY=$(LINUXDIR)/arch/arm/boot/zImage

NOSMP_DESC = ''
ifeq ($(ASUS_TRX),ASUS)
ifneq ($(BCMSMP),y)
	NOSMP_DESC = '-NOSMP'
endif
endif
ifeq ($(ASUS_TRX),LINKSYS2)
ifneq ($(BCMSMP),y)
	NOSMP_DESC = '-NOSMP'
endif
endif
ifeq ($(ASUS_TRX),BELKIN)
ifneq ($(BCMSMP),y)
	NOSMP_DESC = '-NOSMP'
endif
endif


-include tomato_profile.mak

# This could be simpler by just using $(TOMATO_PROFILE_NAME) like it used to be,
# but that's fragile.  If you make one certain innocuous change elsewhere in the
# makefile(s), the build will silently be wrong.  This way it works properly every time.
current_BUILD_NAME = $(strip $(shell grep "^TOMATO_BUILD_NAME" tomato_profile.mak  | cut -d"=" -f2))
current_BUILD_DESC = $(strip $(shell grep "^TOMATO_BUILD_DESC" tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_BUILD_USB  = $(strip $(shell grep "^TOMATO_BUILD_USB"  tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_TOMATO_VER = $(strip $(shell grep "TOMATO_MAJOR" router/shared/tomato_version.h | cut -d"\"" -f2)).$(strip $(shell grep "TOMATO_MINOR" router/shared/tomato_version.h | cut -d"\"" -f2))$(if $(filter $(V1),"--def"),,.$(strip $(shell grep -w "TOMATO_BUILD" router/shared/tomato_version.h | cut -d"\"" -f2)))

uppercase_N = $(shell echo $(N) | tr a-z  A-Z)
lowercase_N = $(shell echo $(N) | tr A-Z a-z)
uppercase_B = $(shell echo $(B) | tr a-z  A-Z)
lowercase_B = $(shell echo $(B) | tr A-Z a-z)

KERN_SIZE_OPT ?= n

beta = $(if $(filter $(TOMATO_EXPERIMENTAL),1),-beta,)

tomato_ver:
	@echo ""
	@btools/uversion.pl --gen $(V1) $(beta)$(V2) $(ND) $(current_BUILD_USB) $(current_BUILD_DESC)

ifeq ($(TOMATO_BUILD),)

all:
	$(MAKE) a

else

all: tomato_ver
	@echo ""
	@echo "Building FreshTomato $(ND) $(current_BUILD_USB) $(current_TOMATO_VER)$(beta)$(V2) $(current_BUILD_DESC) $(current_BUILD_NAME) with $(TOMATO_PROFILE_NAME) Profile"

	@echo ""
	@echo ""

	@-mkdir image
	@$(MAKE) -C router all
	@$(MAKE) -C router install
ifeq ($(ARM),)
	@$(MAKE) -C router image
endif
	@$(MAKE) -C btools
	@$(MAKE) -C ctools

	@echo "\033[41;1m   Creating image \033[0m\033]2;Creating image\007"
	@rm -f image/freshtomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	@rm -f image/freshtomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).bin

ifneq ($(ASUS_TRX),0)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
# for asus RT-N18U, RT-AC56U, RT-AC67U, RT-AC68U (V3), RT-AC68R, RT-AC68P, RT-N66U_C1, RT-AC66U_B1, RT-AC1750_B1, RT-AC1900P/U
ifeq ($(ASUS_TRX),ASUS)
ifeq ($(BCMSMP),y)
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC56U,3.0.0.4,image/freshtomato-RT-AC56U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-N66U_C1,3.0.0.4,image/freshtomato-RT-N66U_C1-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC1750_B1,3.0.0.4,image/freshtomato-RT-AC1750_B1-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC66U_B1,3.0.0.4,image/freshtomato-RT-AC66U_B1-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC67U,3.0.0.4,image/freshtomato-RT-AC67U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC68U,3.0.0.4,image/freshtomato-RT-AC68U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC1900P,3.0.0.4,image/freshtomato-RT-AC1900P-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC1900U,3.0.0.4,image/freshtomato-RT-AC1900U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-N18U,3.0.0.4,image/freshtomato-RT-N18U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
else
	ctools/trx_asus -i image/linux-lzma.trx -r RT-N18U,3.0.0.4,image/freshtomato-RT-N18U-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC56U,3.0.0.4,image/freshtomato-RT-AC56S-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
endif
# for Tenda AC15
ifeq ($(ASUS_TRX),AC15)
	ctools/trx_asus -i image/linux-lzma.trx -r TendaAC15,3.0.0.4,image/freshtomato-TendaAC15-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
#	The following lines help to create .bin file that is uploadable from original firmware. Process is semi-manual so it is disabled for now
#	crc32 image/freshtomato-TendaAC15-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).trx
#	echo "CRC32 (@0x18) and filesize (@0x0d trx, @0x29 full) to be updated manually in bin file!"
#	cat ctools/tendahead.bin image/freshtomato-TendaAC15-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).trx > image/freshtomato-TendaAC15-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).bin
endif
# for Tenda AC18
ifeq ($(ASUS_TRX),AC18)
	ctools/trx_asus -i image/linux-lzma.trx -r TendaAC18,3.0.0.4,image/freshtomato-TendaAC18-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
#	The following lines help to create .bin file that is uploadable from original firmware. Process is semi-manual so it is disabled for now
#	crc32 image/freshtomato-TendaAC18-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).trx
#	echo "CRC32 (@0x18) and filesize (@0x0d trx, @0x29 full) to be updated manually in bin file!"
#	cat ctools/tendahead.bin image/freshtomato-TendaAC18-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).trx > image/freshtomato-TendaAC18-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).bin
endif
#for Belkin F9K1113v2
ifeq ($(ASUS_TRX),BELKIN)
	ctools/trx_asus -i image/linux-lzma.trx -r F9K1113v2,3.0.0.4,image/freshtomato-F9K1113v2-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
# for dlink
ifeq ($(ASUS_TRX),DLINK)
	ctools/trx_asus -i image/linux-lzma.trx -r DIR868L,3.0.0.4,image/freshtomato-DIR868L-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
# for R6900, R7000, R6700v1, R6700v3, R6400, R6400v2, R6300v2, R6250, XR300
ifeq ($(ASUS_TRX),NETGEAR)
ifeq ($(NVRAM_128K),y)
	ctools/trx_asus -i image/linux-lzma.trx -r R6400,3.0.0.4,image/freshtomato-R6400-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6400v2,3.0.0.4,image/freshtomato-R6400v2-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6700v3,3.0.0.4,image/freshtomato-R6700v3-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r XR300,3.0.0.4,image/freshtomato-XR300-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
else
	ctools/trx_asus -i image/linux-lzma.trx -r R7000,3.0.0.4,image/freshtomato-R7000-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6900,3.0.0.4,image/freshtomato-R6900-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6700v1,3.0.0.4,image/freshtomato-R6700v1-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6300v2,3.0.0.4,image/freshtomato-R6300v2-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6250,3.0.0.4,image/freshtomato-R6250-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
endif
# for WS880
ifeq ($(ASUS_TRX),HUAWEI)
	ctools/trx_asus -i image/linux-lzma.trx -r WS880,3.0.0.4,image/freshtomato-WS880-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
# for Buffalo
ifeq ($(ASUS_TRX),BUFFALO)
	ctools/trx_asus -i image/linux-lzma.trx -r WZR1750,3.0.0.4,image/freshtomato-WZR1750-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
# for EA6200, EA6350v1
ifeq ($(ASUS_TRX),LINKSYS2)
	ctools/trx_asus -i image/linux-lzma.trx -r EA6350v1,3.0.0.4,image/freshtomato-EA6350v1-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6200,3.0.0.4,image/freshtomato-EA6200-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
#for linksys EA series -- EA6400 EA6700 EA6500v2 EA6900
ifeq ($(ASUS_TRX),LINKSYS)
	ctools/trx_asus -i image/linux-lzma.trx -r EA6900,3.0.0.4,image/freshtomato-EA6900-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6700,3.0.0.4,image/freshtomato-EA6700-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6500v2,3.0.0.4,image/freshtomato-EA6500v2-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6400,3.0.0.4,image/freshtomato-EA6400-ARM_NG-$(current_TOMATO_VER)$(beta)$(V2)-$(current_BUILD_DESC).trx
endif
	@rm -f image/linux-lzma.trx
	@echo ""
endif

ifneq ($(NETGEAR_CHK),0)
	@echo "Creating Firmware for Netgear ARM devices .... "
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	cd image && touch rootfs
	cd image && $(WNRTOOL)/packet -k linux-lzma.trx -f rootfs -b $(BOARD_FILE) -ok kernel_image \
		-oall kernel_rootfs_image -or rootfs_image -i $(FW_FILE) && rm -f rootfs && \
		cp kernel_rootfs_image.chk freshtomato-$(NETGEAR_CHK)-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).chk
	@echo "Cleanup ...."
	@rm -rf image/linux-lzma.trx image/*image.chk
endif

ifneq ($(XIAOMI_TRX),0)
	@echo "Creating Firmware for Xiaomi R1D .... "
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	cd image && cp linux-lzma.trx freshtomato-$(XIAOMI_TRX)-$(current_TOMATO_VER)$(V2)-$(current_BUILD_DESC).trx
	@echo "Cleanup ...."
	@rm -rf image/linux-lzma.trx
endif

	@echo ""
	@echo "-----------------"
	@echo `cat router/shared/tomato_version` " ready"
	@echo "-----------------"
	@echo "\033[41;1m   FreshTomato build done! \033[0m\033]2;FreshTomato build done!\007"
endif



clean:
	@touch router/.config
	@rm -f router/config_[a-z]
	@rm -f router/busybox/config_[a-z]
	@$(MAKE) -C router $@
	@-rmdir router/arm-uclibc

cleanimage:
	@rm -f fpkg.log
	@rm -fr image/*
	@rm -f router/.config
	@touch router/.config 
	@-mkdir image

cleantools:
#	@$(MAKE) -C $(LINUXDIR)/scripts/squashfs clean
	@$(MAKE) -C btools clean
	@$(MAKE) -C ctools clean

cleankernel:
	@cd $(LINUXDIR) && \
	mv .config save-config && \
	$(MAKE) distclean || true; \
	cp -p save-config .config || true

kernel:
	@$(MAKE) -C router kernel
	@[ ! -e $(KERNEL_BINARY) ] || ls -l $(KERNEL_BINARY)

distclean: clean cleanimage cleankernel cleantools
ifneq ($(INSIDE_MAK),1)
	@$(MAKE) -C router $@ INSIDE_MAK=1
endif
	mv router/busybox/.config busybox-saved-config || true
	@$(MAKE) -C router/busybox distclean
	@rm -f router/busybox/config_current
	@cp -p busybox-saved-config router/busybox/.config || true
	@cp -p router/busybox/.config  router/busybox/config_current || true
	@rm -f router/config_current
	@rm -f router/.config.cmd router/.config.old router/.config
	@rm -f router/libfoo_xref.txt
	@rm -f tomato_profile.mak router/shared/tomato_profile.h
	@touch tomato_profile.mak
	@touch router/shared/tomato_profile.h


prepk:
	@cd $(LINUXDIR) ; \
		rm -f config_current ; \
		ln -s config_base config_current ; \
		cp -f config_current .config
	$(MAKE) -C $(LINUXDIR) oldconfig

what:
	@echo ""
	@echo "$(current_BUILD_DESC)-$(current_BUILD_NAME)-$(TOMATO_PROFILE_NAME) Profile"
	@echo ""


# The methodology for making the different builds is to
# copy the "base" config file to the "target" config file in
# the appropriate directory, and then edit it by removing and
# inserting the desired configuration lines.
# You can't just delete the "whatever=y" line, you must have
# a "...is not set" line, or the make oldconfig will stop and ask
# what to do.

# Options for "make bin" :
# BUILD_DESC (Std|Lite|Ext|...)
# KERN_SIZE_OPT
# USB ("USB"|"")
# JFFSv1 | NO_JFFS
# NO_CIFS, NO_SSH, NO_SAMBA, NO_HTTPS, NO_XXTP, NO_LIBOPT
# SAMBA3, OPENVPN, WIREGUARD, IPV6SUPP, EBTABLES, NTFS, UFSD, MEDIASRV, BBEXTRAS, USBEXTRAS, BCM57, SLIM, NOCAT, NGINX, CTF
# NFS BTCLIENT BTGUI TR_EXTRAS SNMP HFS UPS DNSCRYPT STUBBY PPTPD TOR IPSEC RAID USBAP NO_USBAPP NANO MULTIWAN BRCM_NAND_JFFS2 CONNTRACK_TOOL CRASH_LOG NAMESPACES MDNS

define RouterOptions
	@( \
	sed -i "/TCONFIG_EMF/d" $(1); \
	if [ "$(SLIM)" = "y" ]; then \
		echo "# TCONFIG_EMF is not set" >>$(1); \
	else \
		echo "TCONFIG_EMF=y" >>$(1); \
	fi; \
	sed -i "/TCONFIG_JFFSV1/d" $(1); \
	if [ "$(JFFSv1)" = "y" ]; then \
		echo "TCONFIG_JFFSV1=y" >>$(1); \
	else \
		echo "# TCONFIG_JFFSV1 is not set" >>$(1); \
	fi; \
	if [ "$(IPSEC)" = "y" ]; then \
		echo "TCONFIG_IPSEC=y" >>$(1); \
	else \
		echo "# TCONFIG_IPSEC is not set" >>$(1); \
	fi; \
	if [ "$(RAID)" = "y" ]; then \
		echo "TCONFIG_RAID=y" >>$(1); \
	else \
		echo "# TCONFIG_RAID is not set" >>$(1); \
	fi; \
	if [ "$(USB)" = "USB" ]; then \
		sed -i "/TCONFIG_USB is not set/d" $(1); \
		echo "TCONFIG_USB=y" >>$(1); \
		if [ "$(USBEXTRAS)" = "y" ]; then \
			sed -i "/TCONFIG_USB_EXTRAS/d" $(1); \
			echo "TCONFIG_USB_EXTRAS=y" >>$(1); \
		fi; \
		if [ "$(NTFS)" = "y" ]; then \
			sed -i "/TCONFIG_NTFS/d" $(1); \
			echo "TCONFIG_NTFS=y" >>$(1); \
		fi; \
		if [ "$(UFSD)" = "ASUS" ]; then \
			sed -i "/TCONFIG_UFSDA/d" $(1); \
			echo "TCONFIG_UFSDA=y" >>$(1); \
		fi; \
		if [ "$(TUXERA)" = "y" ]; then \
			sed -i "/TCONFIG_TUXERA/d" $(1); \
			echo "TCONFIG_TUXERA=y" >>$(1); \
		fi; \
		if [ "$(HFS)" = "y" ]; then \
			sed -i "/TCONFIG_HFS/d" $(1); \
			echo "TCONFIG_HFS=y" >>$(1); \
		fi; \
		if [ "$(TUXERA_HFS)" = "y" ]; then \
			sed -i "/TCONFIG_TUXERA_HFS/d" $(1); \
			echo "TCONFIG_TUXERA_HFS=y" >>$(1); \
		fi; \
		if [ "$(MEDIASRV)" = "y" ]; then \
			sed -i "/TCONFIG_MEDIA_SERVER/d" $(1); \
			echo "TCONFIG_MEDIA_SERVER=y" >>$(1); \
		fi; \
	else \
		sed -i "/TCONFIG_USB=y/d" $(1); \
		echo "# TCONFIG_USB is not set" >>$(1); \
	fi; \
	if [ "$(NO_SAMBA)" = "y" ]; then \
		sed -i "/TCONFIG_SAMBASRV/d" $(1); \
		echo "# TCONFIG_SAMBASRV is not set" >>$(1); \
	fi; \
	if [ "$(NO_ZEBRA)" = "y" ]; then \
		sed -i "/TCONFIG_ZEBRA/d" $(1); \
		echo "# TCONFIG_ZEBRA is not set" >>$(1); \
	fi; \
	if [ "$(NO_JFFS)" = "y" ]; then \
		sed -i "/TCONFIG_JFFS2/d" $(1); \
		echo "# TCONFIG_JFFS2 is not set" >>$(1); \
		sed -i "/TCONFIG_JFFSV1/d" $(1); \
		echo "# TCONFIG_JFFSV1 is not set" >>$(1); \
	fi; \
	if [ "$(NO_CIFS)" = "y" ]; then \
		sed -i "/TCONFIG_CIFS/d" $(1); \
		echo "# TCONFIG_CIFS is not set" >>$(1); \
	fi; \
	if [ "$(NO_SSH)" = "y" ]; then \
		sed -i "/TCONFIG_SSH/d" $(1); \
		echo "# TCONFIG_SSH is not set" >>$(1); \
	fi; \
	if [ "$(NO_HTTPS)" = "y" ]; then \
		sed -i "/TCONFIG_HTTPS/d" $(1); \
		echo "# TCONFIG_HTTPS is not set" >>$(1); \
	fi; \
	if [ "$(NO_XXTP)" = "y" ]; then \
		sed -i "/TCONFIG_L2TP/d" $(1); \
		echo "# TCONFIG_L2TP is not set" >>$(1); \
		sed -i "/TCONFIG_PPTP/d" $(1); \
		echo "# TCONFIG_PPTP is not set" >>$(1); \
	fi; \
	if [ "$(NO_LIBOPT)" = "y" ]; then \
		sed -i "/TCONFIG_OPTIMIZE_SHARED_LIBS/d" $(1); \
		echo "# TCONFIG_OPTIMIZE_SHARED_LIBS is not set" >>$(1); \
	fi; \
	if [ "$(EBTABLES)" = "y" ]; then \
		sed -i "/TCONFIG_EBTABLES/d" $(1); \
		echo "TCONFIG_EBTABLES=y" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/TCONFIG_IPV6/d" $(1); \
		echo "TCONFIG_IPV6=y" >>$(1); \
	fi; \
	if [ "$(NOCAT)" = "y" ]; then \
		sed -i "/TCONFIG_NOCAT/d" $(1); \
		echo "TCONFIG_NOCAT=y" >>$(1); \
	fi; \
	if [ "$(NGINX)" = "y" ]; then \
		sed -i "/TCONFIG_NGINX/d" $(1); \
		echo "TCONFIG_NGINX=y" >>$(1); \
	fi; \
	sed -i "/TCONFIG_MULTIWAN/d" $(1); \
	if [ "$(NVRAM_SIZE)" = "60" ] || [ "$(NVRAM_SIZE)" = "64" ] || [ "$(NVRAM_64K)" = "y" ] || [ "$(NVRAM_128K)" = "y" ]; then \
		echo "TCONFIG_MULTIWAN=y" >>$(1); \
	else \
		echo "# TCONFIG_MULTIWAN is not set" >>$(1); \
	fi; \
	if [ "$(OPENVPN)" = "y" ]; then \
		sed -i "/TCONFIG_LZO/d" $(1); \
		echo "TCONFIG_LZO=y" >>$(1); \
		sed -i "/# TCONFIG_OPENVPN is not set/d" $(1); \
		echo "TCONFIG_OPENVPN=y" >>$(1); \
		sed -i "/TCONFIG_FTP_SSL/d" $(1); \
		echo "TCONFIG_FTP_SSL=y" >>$(1); \
	fi; \
	if [ "$(WIREGUARD)" = "y" ]; then \
		sed -i "/#TCONFIG_WIREGUARD is not set/d" $(1); \
		echo "TCONFIG_WIREGUARD=y" >>$(1); \
	fi; \
	if [ "$(ZFS)" = "y" ]; then \
		sed -i "/#TCONFIG_ZFS is not set/d" $(1); \
		echo "TCONFIG_ZFS=y" >>$(1); \
	fi; \
	if [ "$(PPTPD)" = "y" ]; then \
		sed -i "/TCONFIG_PPTPD/d" $(1); \
		echo "TCONFIG_PPTPD=y" >>$(1); \
	fi; \
	if [ "$(BTCLIENT)" = "y" ]; then \
		sed -i "/TCONFIG_BT/d" $(1); \
		echo "TCONFIG_BT=y" >>$(1); \
		sed -i "/TCONFIG_BBT/d" $(1); \
		echo "TCONFIG_BBT=y" >>$(1); \
	fi; \
	if [ "$(BTGUI)" = "y" ]; then \
		sed -i "/TCONFIG_BT/d" $(1); \
		echo "TCONFIG_BT=y" >>$(1); \
	fi; \
	if [ "$(TR_EXTRAS)" = "y" ]; then \
		sed -i "/TCONFIG_TR_EXTRAS/d" $(1); \
		echo "TCONFIG_TR_EXTRAS=y" >>$(1); \
	fi; \
	if [ "$(NFS)" = "y" ]; then \
		sed -i "/TCONFIG_NFS/d" $(1); \
		echo "TCONFIG_NFS=y" >>$(1); \
	fi; \
	if [ "$(SNMP)" = "y" ]; then \
		sed -i "/TCONFIG_SNMP/d" $(1); \
		echo "TCONFIG_SNMP=y" >>$(1); \
	fi; \
	if [ "$(DNSSEC)" = "y" ]; then \
		sed -i "/TCONFIG_DNSSEC/d" $(1); \
		echo "TCONFIG_DNSSEC=y" >>$(1); \
	fi; \
	if [ "$(UPS)" = "y" ]; then \
		sed -i "/TCONFIG_UPS/d" $(1); \
		echo "TCONFIG_UPS=y" >>$(1); \
	fi; \
	if [ "$(NANO)" = "y" ]; then \
		sed -i "/TCONFIG_NANO/d" $(1); \
		echo "TCONFIG_NANO=y" >>$(1); \
	fi; \
	if [ "$(DNSCRYPT)" = "y" ]; then \
		sed -i "/TCONFIG_DNSCRYPT/d" $(1); \
		echo "TCONFIG_DNSCRYPT=y" >>$(1); \
	fi; \
	if [ "$(STUBBY)" = "y" ]; then \
		sed -i "/TCONFIG_STUBBY/d" $(1); \
		echo "TCONFIG_STUBBY=y" >>$(1); \
	fi; \
	if [ "$(NVRAM_64K)" = "y" ]; then \
		sed -i "/TCONFIG_NVRAM_64K/d" $(1); \
		echo "TCONFIG_NVRAM_64K=y" >>$(1); \
	fi; \
	if [ "$(NVRAM_128K)" = "y" ]; then \
		sed -i "/TCONFIG_NVRAM_128K/d" $(1); \
	 	echo "TCONFIG_NVRAM_128K=y" >>$(1); \
	fi; \
	if [ "$(TOR)" = "y" ]; then \
		sed -i "/TCONFIG_TOR/d" $(1); \
		echo "TCONFIG_TOR=y" >>$(1); \
	fi; \
	if [ "$(USBAP)" = "y" ]; then \
		sed -i "/TCONFIG_USBAP/d" $(1); \
		echo "TCONFIG_USBAP=y" >>$(1); \
		if [ "$(NO_USBAPP)" = "y" ]; then \
			sed -i "/TCONFIG_REMOVE_USBAPP/d" $(1); \
			echo "TCONFIG_REMOVE_USBAPP=y" >>$(1); \
			sed -i "/TCONFIG_FTP/d" $(1); \
			echo "# TCONFIG_FTP is not set" >>$(1); \
			echo "# TCONFIG_FTP_SSL is not set" >>$(1); \
			sed -i "/TCONFIG_SAMBA/d" $(1); \
			echo "# TCONFIG_SAMBASRV is not set" >>$(1); \
		fi; \
	fi; \
	if [ "$(ASUS_TRX)" = "RT-AC66U" ]; then \
		sed -i "/TCONFIG_AC66U/d" $(1); \
		echo "TCONFIG_AC66U=y" >>$(1); \
	else \
		sed -i "/TCONFIG_AC66U/d" $(1); \
		echo "# TCONFIG_AC66U is not set" >>$(1); \
	fi; \
	if [ "$(CTF)" = "y" ]; then \
		sed -i "/TCONFIG_CTF/d" $(1); \
		echo "TCONFIG_CTF=y" >>$(1); \
	fi; \
	if [ "$(NAND)" = "y" ]; then \
		sed -i "/TCONFIG_NAND/d" $(1); \
		echo "TCONFIG_NAND=y" >>$(1); \
	fi; \
	if [ "$(ARM)" = "y" ]; then \
		sed -i "/TCONFIG_BCMARM/d" $(1); \
		echo "TCONFIG_BCMARM=y" >>$(1); \
		sed -i "/TCONFIG_BCMWL6/d" $(1); \
		echo "TCONFIG_BCMWL6=y" >>$(1); \
		echo "TCONFIG_BCMWL6A=y" >>$(1); \
	fi; \
	if [ "$(BSD)" = "y" ]; then \
		sed -i "/TCONFIG_BCMBSD/d" $(1); \
		echo "TCONFIG_BCMBSD=y" >>$(1); \
	fi; \
	if [ "$(BCMSMP)" = "y" ]; then \
		sed -i "/TCONFIG_BCMSMP/d" $(1); \
		echo "TCONFIG_BCMSMP=y" >>$(1); \
	fi; \
	if [ "$(PROXYSTA)" = "y" ]; then \
		sed -i "/TCONFIG_PROXYSTA/d" $(1); \
		echo "TCONFIG_PROXYSTA=y" >>$(1); \
	fi; \
	if [ "$(DPSTA)" = "y" ]; then \
		sed -i "/TCONFIG_DPSTA/d" $(1); \
		echo "TCONFIG_DPSTA=y" >>$(1); \
	fi; \
	if [ "$(GRO)" = "y" ]; then \
		sed -i "/TCONFIG_GROCTRL/d" $(1); \
		echo "TCONFIG_GROCTRL=y" >>$(1); \
	fi; \
	if [ "$(BCMFA)" = "y" ]; then \
		sed -i "/TCONFIG_BCMFA/d" $(1); \
		echo "TCONFIG_BCMFA=y" >>$(1); \
	fi; \
	if [ "$(TINC)" = "y" ]; then \
		sed -i "/TCONFIG_TINC/d" $(1); \
		echo "TCONFIG_TINC=y" >>$(1); \
	fi; \
	if [ "$(OPENSSL11)" = "y" ]; then \
		sed -i "/TCONFIG_OPENSSL11/d" $(1); \
		echo "TCONFIG_OPENSSL11=y" >>$(1); \
	fi; \
	if [ "$(FANCTRL)" = "y" ]; then \
		sed -i "/TCONFIG_FANCTRL/d" $(1); \
		echo "TCONFIG_FANCTRL=y" >>$(1); \
	fi; \
	if [ "$(IPERF)" = "y" ]; then \
		sed -i "/TCONFIG_IPERF/d" $(1); \
		echo "TCONFIG_IPERF=y" >>$(1); \
	fi; \
	if [ "$(NVRAM_SIZE)" = "32" ]; then \
		sed -i "/TCONFIG_NVRAM_32K/d" $(1); \
		echo "TCONFIG_NVRAM_32K=y" >>$(1); \
	fi; \
	if [ "$(BRCM_NAND_JFFS2)" = "y" ]; then \
		sed -i "/TCONFIG_BRCM_NAND_JFFS2/d" $(1); \
		echo "TCONFIG_BRCM_NAND_JFFS2=y" >>$(1); \
	fi; \
	if [ "$(CONNTRACK_TOOL)" = "y" ]; then \
		sed -i "/TCONFIG_CONNTRACK_TOOL/d" $(1); \
		echo "TCONFIG_CONNTRACK_TOOL=y" >>$(1); \
	fi; \
	if [ "$(MDNS)" = "y" ]; then \
		sed -i "s/# TCONFIG_MDNS is not set/TCONFIG_MDNS=y/g" $(1); \
	fi; \
	if [ "$(QRCODE)" = "y" ]; then \
		sed -i "/TCONFIG_QRCODE/d" $(1); \
		echo "TCONFIG_QRCODE=y" >>$(1); \
	fi; \
	)
endef

define BusyboxOptions
	@( \
	sed -i "/CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT/d" $(1); \
	echo "CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT=y" >>$(1); \
	sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
	echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
	sed -i "/CONFIG_MKNOD/d" $(1); \
	echo "CONFIG_MKNOD=y" >>$(1); \
	if [ "$(NO_CIFS)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_MOUNT_CIFS/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_CIFS is not set" >>$(1); \
	fi; \
	if [ "$(BBEXTRAS)" = "y" ]; then \
		sed -i "/CONFIG_SENDMAIL/d" $(1); \
		echo "CONFIG_SENDMAIL=y" >>$(1); \
		sed -i "/CONFIG_WHOIS/d" $(1); \
		echo "CONFIG_WHOIS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SORT_BIG/d" $(1); \
		echo "CONFIG_FEATURE_SORT_BIG=y" >>$(1); \
		sed -i "/CONFIG_CLEAR/d" $(1); \
		echo "CONFIG_CLEAR=y" >>$(1); \
		sed -i "/CONFIG_NICE/d" $(1); \
		echo "CONFIG_NICE=y" >>$(1); \
		sed -i "/CONFIG_SETCONSOLE/d" $(1); \
		echo "CONFIG_SETCONSOLE=y" >>$(1); \
		sed -i "/CONFIG_MKFIFO/d" $(1); \
		echo "CONFIG_MKFIFO=y" >>$(1); \
		sed -i "/CONFIG_SEQ/d" $(1); \
		echo "CONFIG_SEQ=y" >>$(1); \
		sed -i "/CONFIG_STTY/d" $(1); \
		echo "CONFIG_STTY=y" >>$(1); \
	fi; \
	if [ "$(USB)" = "USB" ]; then \
		if [ "$(USBEXTRAS)" = "y" ]; then \
			sed -i "/CONFIG_FDISK/d" $(1); \
			echo "CONFIG_FDISK=y" >>$(1); \
			sed -i "/CONFIG_FEATURE_FDISK_WRITABLE/d" $(1); \
			echo "CONFIG_FEATURE_FDISK_WRITABLE=y" >>$(1); \
			sed -i "/CONFIG_MKFS_VFAT/d" $(1); \
			echo "CONFIG_MKFS_VFAT=y" >>$(1); \
			sed -i "/CONFIG_MKSWAP/d" $(1); \
			echo "CONFIG_MKSWAP=y" >>$(1); \
			sed -i "/CONFIG_FLOCK/d" $(1); \
			echo "CONFIG_FLOCK=y" >>$(1); \
			sed -i "/CONFIG_FSYNC/d" $(1); \
			echo "CONFIG_FSYNC=y" >>$(1); \
			sed -i "/CONFIG_FEATURE_VOLUMEID_EXFAT/d" $(1); \
			echo "CONFIG_FEATURE_VOLUMEID_EXFAT=y" >>$(1); \
		fi; \
	else \
		sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
		echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_MOUNT_LABEL/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_LABEL is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_MOUNT_FSTAB/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_FSTAB is not set" >>$(1); \
		sed -i "/CONFIG_VOLUMEID/d" $(1); \
		echo "# CONFIG_VOLUMEID is not set" >>$(1); \
		sed -i "/CONFIG_BLKID/d" $(1); \
		echo "# CONFIG_BLKID is not set" >>$(1); \
		sed -i "/CONFIG_SWAPON/d" $(1); \
		echo "# CONFIG_SWAPON is not set" >>$(1); \
		sed -i "/CONFIG_SWAPOFF/d" $(1); \
		echo "# CONFIG_SWAPOFF is not set" >>$(1); \
		sed -i "/CONFIG_CHROOT/d" $(1); \
		echo "# CONFIG_CHROOT is not set" >>$(1); \
		sed -i "/CONFIG_PIVOT_ROOT/d" $(1); \
		echo "# CONFIG_PIVOT_ROOT is not set" >>$(1); \
		sed -i "/CONFIG_TRUE/d" $(1); \
		echo "# CONFIG_TRUE is not set" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_IPV6/d" $(1); \
		echo "CONFIG_FEATURE_IPV6=y" >>$(1); \
		sed -i "/CONFIG_PING6/d" $(1); \
		echo "CONFIG_PING6=y" >>$(1); \
		sed -i "/CONFIG_TRACEROUTE6/d" $(1); \
		echo "CONFIG_TRACEROUTE6=y" >>$(1); \
	fi; \
	if [ "$(SLIM)" = "y" ]; then \
		sed -i "/CONFIG_AWK/d" $(1); \
		echo "# CONFIG_AWK is not set" >>$(1); \
		sed -i "/CONFIG_BASENAME/d" $(1); \
		echo "# CONFIG_BASENAME is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
		echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
		sed -i "/CONFIG_BLKID/d" $(1); \
		echo "# CONFIG_BLKID is not set" >>$(1); \
		sed -i "/CONFIG_TELNET=y/d" $(1); \
		echo "# CONFIG_TELNET is not set" >>$(1); \
		sed -i "/CONFIG_ARPING/d" $(1); \
		echo "# CONFIG_ARPING is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_LS_COLOR/d" $(1); \
		echo "# CONFIG_FEATURE_LS_COLOR is not set" >>$(1); \
		sed -i "/CONFIG_CHOWN/d" $(1); \
		echo "# CONFIG_CHOWN is not set" >>$(1); \
	else \
		sed -i "/CONFIG_FEATURE_LS_COLOR/d" $(1); \
		echo "CONFIG_FEATURE_LS_COLOR=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_LS_COLOR_IS_DEFAULT/d" $(1); \
		echo "CONFIG_FEATURE_LS_COLOR_IS_DEFAULT=y" >>$(1); \
	fi; \
	if [ "$(BCMSMP)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_TOP_SMP_CPU/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_CPU=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_DECIMALS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_DECIMALS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_SMP_PROCESS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_PROCESS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOPMEM/d" $(1); \
		echo "CONFIG_FEATURE_TOPMEM=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SHOW_THREADS/d" $(1); \
		echo "CONFIG_FEATURE_SHOW_THREADS=y" >>$(1); \
	fi; \
	if [ "$(FANCTRL)" = "y" ]; then \
		sed -i "/CONFIG_RENICE/d" $(1); \
		echo "CONFIG_RENICE=y" >>$(1); \
	fi; \
	)
endef

define KernelConfig
	@( \
	sed -i "/CONFIG_NVRAM_SIZE/d" $(1); \
	echo "CONFIG_NVRAM_SIZE="$(NVRAM_SIZE) >>$(1); \
	sed -i "/CONFIG_CC_OPTIMIZE_FOR_SIZE/d" $(1); \
	if [ "$(KERN_SIZE_OPT)" = "y" ]; then \
		echo "CONFIG_CC_OPTIMIZE_FOR_SIZE=y" >>$(1); \
	else \
		echo "# CONFIG_CC_OPTIMIZE_FOR_SIZE is not set" >>$(1); \
	fi; \
	if [ "$(NAND)" = "y" ]; then \
		sed -i "/CONFIG_MTD_NFLASH/d" $(1); \
		echo "CONFIG_MTD_NFLASH=y" >>$(1); \
		sed -i "/CONFIG_MTD_NAND/d" $(1); \
		echo "CONFIG_MTD_NAND=y" >>$(1); \
		echo "CONFIG_MTD_NAND_IDS=y" >>$(1); \
		echo "# CONFIG_MTD_NAND_DENALI is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_RICOH is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_VERIFY_WRITE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ECC_SMC is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_MUSEUM_IDS is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_DISKONCHIP is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_CAFE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_NANDSIM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_PLATFORM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ONENAND is not set" >>$(1); \
		sed -i "/CONFIG_MTD_BRCMNAND/d" $(1); \
		echo "CONFIG_MTD_BRCMNAND=y" >>$(1); \
	fi; \
	if [ "$(SPI)" = "y" ]; then \
		echo "# CONFIG_MTD_DATAFLASH is not set" >>$(1); \
		echo "CONFIG_MTD_M25P80=y" >>$(1); \
		echo "CONFIG_M25PXX_USE_FAST_READ=y" >>$(1); \
		echo "# CONFIG_MTD_SST25L is not set" >>$(1); \
		echo "# CONFIG_AD525X_DPOT is not set" >>$(1); \
		echo "# CONFIG_TI_DAC7512 is not set" >>$(1); \
		echo "# CONFIG_EEPROM_AT25 is not set" >>$(1); \
		echo "# CONFIG_SERIAL_MAX3100 is not set" >>$(1); \
		echo "# CONFIG_SERIAL_MAX3107 is not set" >>$(1); \
		echo "CONFIG_SPI=y" >>$(1); \
		echo "CONFIG_SPI_MASTER=y" >>$(1); \
		echo "# SPI Master Controller Drivers" >>$(1); \
		echo "# CONFIG_SPI_BITBANG is not set" >>$(1); \
		echo "# CONFIG_SPI_XILINX is not set" >>$(1); \
		echo "# CONFIG_SPI_DESIGNWARE is not set" >>$(1); \
		echo "# CONFIG_SPI_SPIDEV is not set" >>$(1); \
		echo "# CONFIG_SPI_TLE62X0 is not set" >>$(1); \
		echo "# CONFIG_SND_SPI is not set" >>$(1); \
		echo "# CONFIG_SND_SOC is not set" >>$(1); \
		echo "# CONFIG_LEDS_DAC124S085 is not set" >>$(1); \
	fi; \
	if [ "$(UPS)" = "y" ]; then \
		sed -i "s/# CONFIG_HIDRAW is not set/CONFIG_HIDRAW=y/g" $(1); \
		sed -i "/CONFIG_USB_STORAGE_ONETOUCH/d" $(1); \
		echo "# CONFIG_USB_STORAGE_ONETOUCH is not set" >>$(1); \
	else \
		sed -i "/CONFIG_USB_STORAGE_ONETOUCH/d" $(1); \
		echo "# CONFIG_USB_STORAGE_ONETOUCH is not set" >>$(1); \
	fi; \
	if [ "$(USB)" = "" ]; then \
		sed -i "/CONFIG_EFI_PARTITION/d" $(1); \
		echo "# CONFIG_EFI_PARTITION is not set" >>$(1); \
	else \
		if [ "$(HFS)" = "y" ]; then \
			sed -i "s/# CONFIG_HFS_FS is not set/CONFIG_HFS_FS=m/g" $(1); \
			sed -i "s/# CONFIG_HFSPLUS_FS is not set/CONFIG_HFSPLUS_FS=m/g" $(1); \
		fi; \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/CONFIG_IPV6 is not set/d" $(1); \
		echo "CONFIG_IPV6=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_IPTABLES/d" $(1); \
		echo "CONFIG_IP6_NF_IPTABLES=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_MATCH_RT/d" $(1); \
		echo "CONFIG_IP6_NF_MATCH_RT=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_FILTER/d" $(1); \
		echo "CONFIG_IP6_NF_FILTER=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_TARGET_LOG/d" $(1); \
		echo "CONFIG_IP6_NF_TARGET_LOG=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_TARGET_REJECT/d" $(1); \
		echo "CONFIG_IP6_NF_TARGET_REJECT=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_MANGLE/d" $(1); \
		echo "CONFIG_IP6_NF_MANGLE=m" >>$(1); \
		sed -i "/CONFIG_NF_CONNTRACK_IPV6/d" $(1); \
		echo "CONFIG_NF_CONNTRACK_IPV6=m" >>$(1); \
		sed -i "/CONFIG_NETFILTER_XT_MATCH_HL/d" $(1); \
		echo "CONFIG_NETFILTER_XT_MATCH_HL=m" >>$(1); \
		sed -i "/CONFIG_IPV6_ROUTER_PREF/d" $(1); \
		echo "CONFIG_IPV6_ROUTER_PREF=y" >>$(1); \
		sed -i "/CONFIG_IPV6_SIT/d" $(1); \
		echo "CONFIG_IPV6_SIT=m" >>$(1); \
		sed -i "/CONFIG_IPV6_SIT_6RD/d" $(1); \
		echo "CONFIG_IPV6_SIT_6RD=y" >>$(1); \
		sed -i "/CONFIG_IPV6_MULTIPLE_TABLES/d" $(1); \
		echo "CONFIG_IPV6_MULTIPLE_TABLES=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_RAW/d" $(1); \
		echo "CONFIG_IP6_NF_RAW=m" >>$(1); \
		sed -i "/CONFIG_IPV6_OPTIMISTIC_DAD/d" $(1); \
		echo "CONFIG_IPV6_OPTIMISTIC_DAD=y" >>$(1); \
		sed -i "/CONFIG_IPV6_MROUTE/d" $(1); \
		echo "CONFIG_IPV6_MROUTE=y" >>$(1); \
		echo "# CONFIG_IPV6_MROUTE_MULTIPLE_TABLES is not set" >>$(1); \
		sed -i "/CONFIG_IP6_NF_TARGET_ROUTE/d" $(1); \
		echo "CONFIG_IP6_NF_TARGET_ROUTE=m" >>$(1); \
		sed -i "/CONFIG_INET6_XFRM_TUNNEL/d" $(1); \
		echo "CONFIG_INET6_XFRM_TUNNEL=m" >>$(1); \
		sed -i "/CONFIG_INET6_AH/d" $(1); \
		echo "CONFIG_INET6_AH=m" >>$(1); \
		sed -i "/CONFIG_INET6_ESP/d" $(1); \
		echo "CONFIG_INET6_ESP=m" >>$(1); \
		sed -i "/CONFIG_INET6_IPCOMP/d" $(1); \
		echo "CONFIG_INET6_IPCOMP=m" >>$(1); \
		sed -i "/CONFIG_INET6_XFRM_MODE_TRANSPORT/d" $(1); \
		echo "CONFIG_INET6_XFRM_MODE_TRANSPORT=m" >>$(1); \
		sed -i "/CONFIG_INET6_XFRM_MODE_TUNNEL/d" $(1); \
		echo "CONFIG_INET6_XFRM_MODE_TUNNEL=m" >>$(1); \
		sed -i "/CONFIG_INET6_XFRM_MODE_BEET/d" $(1); \
		echo "CONFIG_INET6_XFRM_MODE_BEET=m" >>$(1); \
	fi; \
	sed -i "/CONFIG_BCM57XX/d" $(1); \
	if [ "$(BCM57)" = "y" ]; then \
		echo "CONFIG_BCM57XX=m" >>$(1); \
	else \
		echo "# CONFIG_BCM57XX is not set" >>$(1); \
	fi; \
	if [ "$(DPSTA)" = "y" ]; then \
		sed -i "/CONFIG_DPSTA/d" $(1); \
		echo "CONFIG_DPSTA=m" >>$(1); \
	fi; \
		sed -i "/CONFIG_LINUX_MTD/d" $(1); \
	if [ "$(LINUX_MTD)" = "" ]; then \
		echo "CONFIG_LINUX_MTD=32" >>$(1); \
	else \
		echo "CONFIG_LINUX_MTD=$(LINUX_MTD)" >>$(1); \
	fi; \
	if [ "$(ARMCPUSMP)" = "up" ]; then \
		sed -i "/CONFIG_GENERIC_CLOCKEVENTS_BROADCAST/d" $(1); \
		echo "CONFIG_HAVE_LATENCYTOP_SUPPORT=y" >>$(1); \
		sed -i "/CONFIG_GENERIC_LOCKBREAK/d" $(1); \
		echo "CONFIG_BROKEN_ON_SMP=y" >>$(1); \
		sed -i "s/CONFIG_TREE_RCU=y/# CONFIG_TREE_RCU is not set/g" $(1); \
		echo "CONFIG_TREE_PREEMPT_RCU=y" >>$(1); \
		echo "# CONFIG_TINY_RCU is not set" >>$(1); \
		sed -i "/CONFIG_USE_GENERIC_SMP_HELPERS/d" $(1); \
		sed -i "/CONFIG_STOP_MACHINE/d" $(1); \
		sed -i "/CONFIG_MUTEX_SPIN_ON_OWNER/d" $(1); \
		echo "# CONFIG_MUTEX_SPIN_ON_OWNER is not set" >>$(1); \
		sed -i "/# CONFIG_ARM_ERRATA_742230 is not set/d" $(1); \
		sed -i "/# CONFIG_ARM_ERRATA_742231 is not set/d" $(1); \
		sed -i "/# CONFIG_ARM_ERRATA_720789 is not set/d" $(1); \
		sed -i "/CONFIG_SMP=y/d" $(1); \
		echo "# CONFIG_SMP is not set" >>$(1); \
		sed -i "/CONFIG_NR_CPUS=2/d" $(1); \
		sed -i "/# CONFIG_HOTPLUG_CPU is not set/d" $(1); \
		sed -i "/CONFIG_RPS=y/d" $(1); \
	fi; \
	if [ "$(USBAP)" = "y" ]; then \
		echo "CONFIG_WL_USBAP=y" >>$(1); \
		echo 'CONFIG_WL_APSTA="wlconfig_lx_router_high"' >>$(1); \
	else \
		echo "# CONFIG_WL_USBAP is not set" >>$(1); \
		echo 'CONFIG_WL_APSTA="wlconfig_lx_router_apsta"' >>$(1); \
	fi; \
	if [ "$(EBTABLES)" = "y" ]; then \
		sed -i "/CONFIG_BRIDGE_NF_EBTABLES/d" $(1); \
		echo "CONFIG_BRIDGE_NF_EBTABLES=m" >>$(1); \
		if [ "$(IPV6SUPP)" = "y" ]; then \
			sed -i "/CONFIG_BRIDGE_EBT_IP6/d" $(1); \
			echo "CONFIG_BRIDGE_EBT_IP6=m" >>$(1); \
		fi; \
	fi; \
	sed -i "/CONFIG_NVRAM_64K/d" $(1); \
	if [ "$(NVRAM_64K)" = "y" ]; then \
		echo "CONFIG_NVRAM_64K=y" >>$(1); \
	else \
		echo "# CONFIG_NVRAM_64K is not set" >>$(1); \
	fi; \
	sed -i "/CONFIG_NVRAM_128K/d" $(1); \
	if [ "$(NVRAM_128K)" = "y" ]; then \
		echo "CONFIG_NVRAM_128K=y" >>$(1); \
	else \
		echo "# CONFIG_NVRAM_128K is not set" >>$(1); \
	fi; \
	if [ "$(SATA)" = "y" ]; then \
		sed -i "/CONFIG_CMDLINE/d" $(1); \
		echo "CONFIG_CMDLINE=\"root=\/dev\/mtdblock2 console=ttyS0,115200 init=\/sbin\/preinit earlyprintk debug libata.force=3.0Gbps\"" >>$(1); \
		echo "# CONFIG_CMDLINE_FORCE is not set" >>$(1); \
		sed -i "/CONFIG_SCSI_MOD/d" $(1); \
		echo "CONFIG_SCSI_MOD=y" >>$(1); \
		sed -i "/CONFIG_SCSI=m/d" $(1); \
		echo "CONFIG_SCSI=y" >>$(1); \
		sed -i "/CONFIG_SCSI_PROC_FS/d" $(1); \
		echo "CONFIG_SCSI_PROC_FS=y" >>$(1); \
		sed -i "/CONFIG_SCSI_MULTI_LUN/d" $(1); \
		echo "CONFIG_SCSI_MULTI_LUN=y" >>$(1); \
		sed -i "/CONFIG_SCSI_CONSTANTS/d" $(1); \
		echo "CONFIG_SCSI_CONSTANTS=y" >>$(1); \
		sed -i "/CONFIG_SCSI_LOWLEVEL/d" $(1); \
		echo "CONFIG_SCSI_LOWLEVEL=y" >>$(1); \
		sed -i "/CONFIG_BLK_DEV_SD/d" $(1); \
		echo "CONFIG_BLK_DEV_SD=y" >>$(1); \
		sed -i "/CONFIG_ATA is not set/d" $(1); \
		echo "CONFIG_ATA=y" >>$(1); \
		sed -i "/CONFIG_SATA_PMP/d" $(1); \
		echo "CONFIG_SATA_PMP=y" >>$(1); \
		sed -i "/CONFIG_SATA_AHCI/d" $(1); \
		echo "CONFIG_SATA_AHCI=y" >>$(1); \
		sed -i "/CONFIG_SATA_AHCI_PLATFORM/d" $(1); \
		echo "CONFIG_SATA_AHCI_PLATFORM=y" >>$(1); \
		sed -i "/CONFIG_ATA_SFF/d" $(1); \
		echo "CONFIG_ATA_SFF=y" >>$(1); \
		sed -i "/CONFIG_ATA_BMDMA/d" $(1); \
		echo "CONFIG_ATA_BMDMA=y" >>$(1); \
		sed -i "/CONFIG_ATA_VERBOSE_ERROR/d" $(1); \
		echo "# CONFIG_ATA_VERBOSE_ERROR is not set" >>$(1); \
	fi; \
	sed -i "/CONFIG_EXT2_/d" $(1); \
	sed -i "/CONFIG_EXT3_/d" $(1); \
	sed -i "/CONFIG_EXT4_/d" $(1); \
	echo "# CONFIG_EXT2_FS is not set" >>$(1); \
	echo "# CONFIG_EXT3_FS is not set" >>$(1); \
	echo "CONFIG_EXT4_FS=m" >>$(1); \
	echo "CONFIG_EXT4_USE_FOR_EXT23=y" >>$(1); \
	echo "CONFIG_EXT4_FS_XATTR=y" >>$(1); \
	echo "# CONFIG_EXT4_FS_POSIX_ACL is not set" >>$(1); \
	echo "# CONFIG_EXT4_FS_SECURITY is not set" >>$(1); \
	echo "# CONFIG_EXT4_DEBUG is not set" >>$(1); \
	sed -i "/CONFIG_FAILSAFE_UPGRADE/d" $(1); \
	if [ "$(FAILSAFE_UPGRADE)" = "y" ]; then \
		echo "CONFIG_FAILSAFE_UPGRADE=y" >>$(1); \
	else \
		echo "# CONFIG_FAILSAFE_UPGRADE is not set" >> $(1); \
	fi; \
	if [ "$(CONNTRACK_TOOL)" = "y" ]; then \
		sed -i "/CONFIG_NETFILTER_NETLINK_QUEUE/d" $(1); \
		echo "CONFIG_NETFILTER_NETLINK_QUEUE=y" >>$(1); \
		sed -i "/CONFIG_NF_CONNTRACK_EVENTS/d" $(1); \
		echo "CONFIG_NF_CONNTRACK_EVENTS=y" >>$(1); \
		sed -i "/CONFIG_NF_CT_NETLINK/d" $(1); \
		echo "CONFIG_NF_CT_NETLINK=y" >>$(1); \
	else \
		sed -i "/CONFIG_NETFILTER_NETLINK_QUEUE/d" $(1); \
		echo "# CONFIG_NETFILTER_NETLINK_QUEUE is not set" >>$(1); \
		sed -i "/CONFIG_NF_CONNTRACK_EVENTS/d" $(1); \
		echo "# CONFIG_NF_CONNTRACK_EVENTS is not set" >>$(1); \
		sed -i "/CONFIG_NF_CT_NETLINK/d" $(1); \
		echo "# CONFIG_NF_CT_NETLINK is not set" >>$(1); \
	fi; \
	if [ "$(CRASH_LOG)" = "y" ]; then \
		sed -i "/CONFIG_CRASHLOG/d" $(1); \
		echo "CONFIG_CRASHLOG=y" >>$(1); \
		sed -i "/BCMDBG/d" $(1); \
		echo "BCMDBG=y" >>$(1); \
		sed -i "/CONFIG_DEBUG_INFO/d" $(1); \
		echo "CONFIG_DEBUG_INFO=y" >>$(1); \
	else \
		sed -i "/CONFIG_CRASHLOG/d" $(1); \
		echo "# CONFIG_CRASHLOG is not set" >>$(1); \
		sed -i "/BCMDBG/d" $(1); \
		echo "# BCMDBG is not set" >>$(1); \
		sed -i "/CONFIG_DEBUG_INFO/d" $(1); \
		echo "# CONFIG_DEBUG_INFO is not set" >>$(1); \
	fi; \
	if [ "$(NAMESPACES)" = "y" ]; then \
		sed -i "s/# CONFIG_NAMESPACES is not set/CONFIG_NAMESPACES=y/g" $(1); \
		sed -i "s/# CONFIG_NET_NS is not set/CONFIG_NET_NS=y/g" $(1); \
		sed -i "s/# CONFIG_IP_ROUTE_VERBOSE is not set/CONFIG_IP_ROUTE_VERBOSE=y/g" $(1); \
		sed -i "s/# CONFIG_VETH is not set/CONFIG_VETH=y/g" $(1); \
	else \
		sed -i "s/# CONFIG_NAMESPACES=y/CONFIG_NAMESPACES is not set/g" $(1); \
		sed -i "s/# CONFIG_NET_NS=y/CONFIG_NET_NS is not set/g" $(1); \
		sed -i "s/# CONFIG_IP_ROUTE_VERBOSE=y/CONFIG_IP_ROUTE_VERBOSE is not set/g" $(1); \
		sed -i "s/# CONFIG_VETH=y/CONFIG_VETH is not set/g" $(1); \
	fi; \
	if [ "$(WIREGUARD)" = "y" ]; then \
		sed -i "s/# CONFIG_WIREGUARD is not set/CONFIG_WIREGUARD=m/g" $(1); \
	fi; \
	if [ "$(ZFS)" = "y" ]; then \
		sed -i "s/# CONFIG_ZFS is not set/CONFIG_ZFS=m/g" $(1); \
	fi; \
	)
endef


bin:
ifeq ($(B),)
	@echo $@" is not a valid target!"
	@false
endif
	@cp router/config_base router/config_$(lowercase_B)
	@cp router/busybox/config_base router/busybox/config_$(lowercase_B)
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$(lowercase_B)

	$(call RouterOptions, router/config_$(lowercase_B))
	$(call KernelConfig, $(LINUXDIR)/config_$(lowercase_B))
	$(call BusyboxOptions, router/busybox/config_$(lowercase_B))

	@$(MAKE) setprofile N=$(TPROFILE) B=$(B) DESC="$(BUILD_DESC)" USB="$(USB)"
	@$(MAKE) all

## targets
e:
	@$(MAKE) bin NTFS=y HFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y MEDIASRV=y MULTIWAN=y B=E BUILD_DESC="$(VPN)" CTF=y GRO=y USB="USB" PPTPD=y OPENVPN=y WIREGUARD=y DNSSEC=y TINC=y \
	    OPENSSL11=y CONNTRACK_TOOL=y MDNS=y QRCODE=y

o:
	@$(MAKE) bin OPENVPN=y WIREGUARD=y NTFS=y HFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y MULTIWAN=y B=E BUILD_DESC="Custom" CTF=y GRO=y USB="USB" NOCAT=n BTCLIENT=y TR_EXTRAS=y \
	    DNSCRYPT=y STUBBY=y PPTPD=y DNSSEC=y SNMP=n TINC=n TOR=y OPENSSL11=y NFS=y QRCODE=y

z:
	@$(MAKE) bin OPENVPN=y WIREGUARD=y NTFS=y HFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y MULTIWAN=y B=E BUILD_DESC="AIO" CTF=y GRO=y USB="USB" NOCAT=y BTCLIENT=y TR_EXTRAS=y \
	    DNSCRYPT=y STUBBY=y UPS=y PPTPD=y DNSSEC=y TINC=y SNMP=y NFS=y NANO=y TOR=y NGINX=y IPERF=y OPENSSL11=y CONNTRACK_TOOL=y NAMESPACES=y CRASH_LOG=y MDNS=y ZFS=y QRCODE=y

ac68e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="ASUS" TUXERA=y TUXERA_HFS=y BRCM_NAND_JFFS2=y DPSTA=y BSD=y

ac68z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="ASUS" TUXERA=y TUXERA_HFS=y BRCM_NAND_JFFS2=y DPSTA=y BSD=y

n18e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y ARMCPUSMP="up" ASUS_TRX="ASUS" UFSD="ASUS" BRCM_NAND_JFFS2=y DPSTA=n BSD=n

n18z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y ARMCPUSMP="up" ASUS_TRX="ASUS" UFSD="ASUS" BRCM_NAND_JFFS2=y DPSTA=n BSD=n

ac15e:
	@$(MAKE) e ARM=y NVRAM_64K=y SPI=y BCMSMP=y ASUS_TRX="AC15" TUXERA=y TUXERA_HFS=y DPSTA=y BSD=y

ac18e:
	@$(MAKE) e ARM=y NVRAM_64K=y SPI=y NAND=y BCMSMP=y ASUS_TRX="AC18" TUXERA=y TUXERA_HFS=y DPSTA=y BSD=y

ac18z:
	@$(MAKE) z ARM=y NVRAM_64K=y SPI=y NAND=y BCMSMP=y ASUS_TRX="AC18" TUXERA=y TUXERA_HFS=y DPSTA=y BSD=y

f9ke:
	@$(MAKE) e ARM=y NVRAM_64K=y SPI=y ASUS_TRX="BELKIN" ARMCPUSMP="up" TUXERA=y TUXERA_HFS=y DPSTA=n BSD=y

dir868l:
	@$(MAKE) bin ARM=y NVRAM_SIZE=32 NAND=y BCMSMP=y ASUS_TRX="DLINK" CTF=y GRO=y NTFS=y HFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y MEDIASRV=y B=E BUILD_DESC="Custom" USB="USB" \
	    DNSSEC=y DNSCRYPT=y STUBBY=y UPS=y NO_SSH=y NANO=y OPENSSL11=y DPSTA=y BSD=n

r7000e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" UFSD="ASUS" DPSTA=y BSD=y

r7000z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" UFSD="ASUS" DPSTA=y BSD=y

r6400e:
	@$(MAKE) e ARM=y NVRAM_128K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" UFSD="ASUS" DPSTA=y BSD=y

r6400z:
	@$(MAKE) z ARM=y NVRAM_128K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" UFSD="ASUS" DPSTA=y BSD=y

r7000init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R7000" BOARDID_FILE="compatible_r7000.txt" FW_CFG_FILE="tomato_cfg.h"

r6900init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6900" BOARDID_FILE="compatible_r6900.txt" FW_CFG_FILE="tomato_cfg.h"

r6250init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6250" BOARDID_FILE="compatible_r6250.txt" FW_CFG_FILE="tomato_cfg.h"

r6300v2init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6300v2" BOARDID_FILE="compatible_r6300v2.txt" FW_CFG_FILE="tomato_cfg.h"

r6300v2init_ch:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6300v2_ch" BOARDID_FILE="compatible_r6300v2_ch.txt" FW_CFG_FILE="tomato_cfg.h"

r6400init:
	@$(MAKE) bin ARM=y NVRAM_128K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6400" BOARDID_FILE="compatible_r6400.txt" FW_CFG_FILE="tomato_cfg.h"

r6400v2init:
	@$(MAKE) bin ARM=y NVRAM_128K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6400v2" BOARDID_FILE="compatible_r6400v2.txt" FW_CFG_FILE="tomato_cfg.h"

r6700v1init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6700v1" BOARDID_FILE="compatible_r6700v1.txt" FW_CFG_FILE="tomato_cfg.h"

r6700v3init:
	@$(MAKE) bin ARM=y NVRAM_128K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="R6700v3" BOARDID_FILE="compatible_r6700v3.txt" FW_CFG_FILE="tomato_cfg.h"

xr300init:
	@$(MAKE) bin ARM=y NVRAM_128K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y OPENSSL11=y DPSTA=y BSD=y BUILD_DESC="initial" NETGEAR_CHK="XR300" BOARDID_FILE="compatible_xr300.txt" FW_CFG_FILE="tomato_cfg.h"

ws880e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="HUAWEI" DPSTA=y BSD=y

ws880z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="HUAWEI" DPSTA=y BSD=y

wzr1750e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="BUFFALO" DPSTA=y BSD=y

wzr1750z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="BUFFALO" DPSTA=y BSD=y

ea6700e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="LINKSYS" FAILSAFE_UPGRADE=n DPSTA=y BSD=y

ea6700z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="LINKSYS" FAILSAFE_UPGRADE=n DPSTA=y BSD=y

ea6350v1e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y ARMCPUSMP="up" ASUS_TRX="LINKSYS2" FAILSAFE_UPGRADE=n DPSTA=n BSD=y

ea6350v1z:
	@$(MAKE) z ARM=y NVRAM_64K=y NAND=y ARMCPUSMP="up" ASUS_TRX="LINKSYS2" FAILSAFE_UPGRADE=n DPSTA=n BSD=y

r1do:
	@$(MAKE) o ARM=y NVRAM_64K=y BCMSMP=y XIAOMI_TRX="R1D" SATA=y NO_JFFS=y FANCTRL=y DPSTA=y TUXERA=y TUXERA_HFS=y BSD=y


setprofile:
	echo '#ifndef TOMATO_PROFILE' > router/shared/tomato_profile.h
	echo '#define TOMATO_$(N) 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_G 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_N 2' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE PROFILE_$(N)' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE_NAME "$(N)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_NAME "$(B)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_DESC "$(DESC)$(EXTRA_64KDESC)$(NOSMP_DESC)"' >> router/shared/tomato_profile.h
	echo '#ifndef CONFIG_NVRAM_SIZE' >> router/shared/tomato_profile.h
	echo '#define CONFIG_NVRAM_SIZE $(NVRAM_SIZE)' >> router/shared/tomato_profile.h
	echo '#endif' >> router/shared/tomato_profile.h
	echo '#endif' >> router/shared/tomato_profile.h

	echo 'TOMATO_$(N) = 1' > tomato_profile.mak
	echo 'PROFILE_G = 1' >> tomato_profile.mak
	echo 'PROFILE_N = 2' >> tomato_profile.mak
	echo 'TOMATO_PROFILE = $$(PROFILE_$(N))' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_NAME = "$(N)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_NAME = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_DESC = "$(DESC)$(EXTRA_64KDESC)$(NOSMP_DESC)"' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_L = $(lowercase_N)' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_U = $(uppercase_N)' >> tomato_profile.mak
	echo 'TOMATO_BUILD_USB = "$(USB)"' >> tomato_profile.mak

	echo 'export EXTRACFLAGS := $(EXTRA_CFLAGS) -DBCMWPA2 -DBCMARM -fno-delete-null-pointer-checks -marm $(if $(filter $(NVRAM_SIZE),0),,-DCONFIG_NVRAM_SIZE=$(NVRAM_SIZE)) $(EXTRA_64KCFLAG)' >> tomato_profile.mak

# Note that changes to variables in tomato_profile.mak don't
# get propogated to this invocation of make!
	@echo ""
	@echo "Using $(N) profile, $(B) build config."
	@echo ""

	@cd $(LINUXDIR) ; \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp -f config_current .config

	@cd router/busybox && \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config

	@cd router ; \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config

	@$(MAKE) -C router oldconfig

help:
	@echo "ASUS ARM Builds - RT-N18U, RT-AC56U, RT-AC66U_B1, RT-AC1750_B1, RT-AC67U, RT-AC68U, RT-AC68R, RT-AC68P, RT-AC1900P/U"
	@echo "ac68e           RT-N18U/AC56U/AC67U/AC68U(R/P/V3)/AC66U B1/RT-AC1750 B1/RT-AC1900(P/U) build VPN"
	@echo "ac68z           RT-N18U/AC56U/AC67U/AC68U(R/P/V3)/AC66U B1/RT-AC1750 B1/RT-AC1900(P/U) build AIO"
	@echo "n18e            RT-N18U/AC56S without SMP build VPN"
	@echo "n18z            RT-N18U/AC56S without SMP build AIO"
	@echo "TENDA ARM Builds"
	@echo "ac15e           Tenda AC15 VPN"
	@echo "ac18e           Tenda AC18 VPN"
	@echo "ac18z           Tenda AC18 AIO"
	@echo "BELKIN ARM Builds"
	@echo "f9ke            Belkin F9K1113v2 VPN"
	@echo "DLINK ARM Builds"
	@echo "dir868l         DLINK DIR868L"
	@echo "NETGEAR ARM Builds"
	@echo "r7000e          R7000/R6900/R6700v1/R6300v2/R6250 build VPN"
	@echo "r7000z          R7000/R6900/R6700v1/R6300v2/R6250 build AIO"
	@echo "r6400e          R6400/R6400v2/R6700v3/XR300 build VPN - 128K NVRAM"
	@echo "r6400z          R6400/R6400v2/R6700v3/XR300 build AIO - 128K NVRAM"
	@echo "r7000init       R7000 init build"
	@echo "r6900init       R6900 init build"
	@echo "r6250init       R6250 init build"
	@echo "r6300v2init     R6300v2 init build"
	@echo "r6300v2init_ch  R6300v2_ch init build for Charter routers"
	@echo "r6400init       R6400 init build"
	@echo "r6400v2init     R6400v2 init build"
	@echo "r6700v1init     R6700v1 init build"
	@echo "r6700v3init     R6700v3 init build"
	@echo "xr300init       XR300 init build"
	@echo "Xiaomi ARM Builds"
	@echo "r1do            R1D custom build"
	@echo "HUAWEI ARM Builds"
	@echo "ws880e          WS880 build VPN"
	@echo "ws880z          WS880 build AIO"
	@echo "BUFFALO ARM Builds"
	@echo "wzr1750e        WZR-1750DHP build VPN"
	@echo "wzr1750z        WZR-1750DHP build AIO"
	@echo "LINKSYS ARM Builds"
	@echo "ea6700e         EA6700/EA6400/EA6500v2/EA6900 build VPN"
	@echo "ea6700z         EA6700/EA6400/EA6500v2/EA6900 build AIO"
	@echo "ea6350v1e       EA6200/EA6350v1 build VPN"
	@echo "ea6350v1z       EA6200/EA6350v1 build AIO"
	@echo ""
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C Linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C Linux oldconfig dep"

.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile help
.PHONY: a b c d m nc Makefile allversions tomato_profile.mak

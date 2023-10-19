build_all:
	@echo ""
	@echo "Building FreshTomato $(branch_rev) $(current_BUILD_USB) $(current_TOMATO_VER)$(beta)$(current_V2) $(current_BUILD_DESC) $(current_BUILD_NAME) with $(TOMATO_PROFILE_NAME) Profile"
	@echo ""
	@echo ""

	@-mkdir image
	@$(MAKE) -C router all
	@$(MAKE) -C router install
	@$(MAKE) -C btools

	@echo "\033[41;1m   Creating image \033[0m\033]2;Creating image\007"

	@rm -f image/freshtomato-$(branch_rev)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
	@rm -f image/freshtomato-$(branch_rev)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).bin

ifneq ($(ASUS_TRX),0)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs

# for Asus RT-AC3200, Netgear R7900, R8000
 ifeq ($(ASUS_TRX),ASUS)
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC3200,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-RT-AC3200-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
 endif

# for R7900, R8000
 ifeq ($(ASUS_TRX),NETGEAR)
	ctools/trx_asus -i image/linux-lzma.trx -r R7900,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-R7900-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R8000,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-R8000-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
 endif

	@rm -f image/linux-lzma.trx
	@echo ""
endif

# for Netgear Initial
ifneq ($(NETGEAR_CHK),0)
	@echo "Creating Firmware for Netgear devices .... "
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	cd image && touch rootfs
	cd image && $(WNRTOOL)/packet -k linux-lzma.trx -f rootfs -b $(BOARD_FILE) -ok kernel_image \
		-oall kernel_rootfs_image -or rootfs_image -i $(FW_FILE) && rm -f rootfs && \
		cp kernel_rootfs_image.chk freshtomato-$(NETGEAR_CHK)-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).chk
	@echo "Cleanup ...."
	@rm -rf image/linux-lzma.trx image/*image.chk
endif
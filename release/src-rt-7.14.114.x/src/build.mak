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

# for Tri-Band SDK7.14 (currently only Asus RT-AC5300)/for Dual-Band SDK7.14 (currently Asus RT-AC3100 and RT-AC88U)
 ifeq ($(AC5300),y)
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC5300,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-RT-AC5300-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
 else
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC3100,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-RT-AC3100-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC88U,3.0.0.4,$(FORCE_SN),$(FORCE_EN),image/freshtomato-RT-AC88U-$(branch_rev)-$(current_TOMATO_VER)$(beta)$(current_V2)-$(current_BUILD_DESC).trx
 endif

	@rm -f image/linux-lzma.trx
	@echo ""
endif
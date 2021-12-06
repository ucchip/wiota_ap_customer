.PHONY: clean All

All:
	@echo "----------Building project:[ uc8088_wiota_ap_win - Debug ]----------"
	@"$(MAKE)" -f  "uc8088_wiota_ap_win.mk" PreBuild && "$(MAKE)" -f  "uc8088_wiota_ap_win.mk" && "$(MAKE)" -f  "uc8088_wiota_ap_win.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ uc8088_wiota_ap_win - Debug ]----------"
	@"$(MAKE)" -f  "uc8088_wiota_ap_win.mk" clean

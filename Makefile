.PHONY: clean All

All:
	@echo "----------Building project:[ uc8088_wiota_ap - Debug ]----------"
	@"$(MAKE)" -f  "uc8088_wiota_ap.mk" && "$(MAKE)" -f  "uc8088_wiota_ap.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ uc8088_wiota_ap - Debug ]----------"
	@"$(MAKE)" -f  "uc8088_wiota_ap.mk" clean

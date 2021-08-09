.PHONY: clean All

All:
	@echo "----------Building project:[ rtthread_app - Debug ]----------"
	@"$(MAKE)" -f  "rtthread_app.mk" && "$(MAKE)" -f  "rtthread_app.mk" PostBuild
clean:
	@echo "----------Cleaning project:[ rtthread_app - Debug ]----------"
	@"$(MAKE)" -f  "rtthread_app.mk" clean

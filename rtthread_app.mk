##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Debug
ProjectName            :=rtthread_app
ConfigurationName      :=Debug
WorkspacePath          := "/home/jpwang/code/wiota_ap_customer"
ProjectPath            := "/home/jpwang/code/wiota_ap_customer"
IntermediateDirectory  :=./Debug
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Jingping Wang
Date                   :=08/09/21
CodeLitePath           :="/home/jpwang/.codelite"
LinkerName             :=/home/jpwang/code/riscv/bin/riscv32-unknown-elf-g++
SharedObjectLinkerName :=/home/jpwang/code/riscv/bin/riscv32-unknown-elf-g++ -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/$(ProjectName)
Preprocessors          :=$(PreprocessorSwitch)HAVE_CCONFIG_H $(PreprocessorSwitch)__RTTHREAD__ $(PreprocessorSwitch)RT_USING_NEWLIB 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="rtthread_app.txt"
PCHCompileFlags        :=
MakeDirCommand         :=mkdir -p
LinkOptions            :=  -nostartfiles -Wl,--gc-sections  -T ./libraries/link.flash.ld -Wl,-Map=rtthread.map
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch). $(IncludeSwitch)/opt/riscv/riscv32-unknown-elf/include $(IncludeSwitch)${ProjectPath}/Lib $(IncludeSwitch)${ProjectPath}/Other/inc $(IncludeSwitch)applications $(IncludeSwitch)drivers $(IncludeSwitch)libraries/inc $(IncludeSwitch)rt-thread/components/drivers/include $(IncludeSwitch)rt-thread/components/finsh $(IncludeSwitch)rt-thread/components/libc/compilers/common $(IncludeSwitch)rt-thread/components/libc/compilers/newlib $(IncludeSwitch)rt-thread/include $(IncludeSwitch)rt-thread/libcpu/risc-v/uc8088 $(IncludeSwitch)platform/include $(IncludeSwitch)platform/trace/include $(IncludeSwitch)PS/app/include $(IncludeSwitch)PS/app/test/ $(IncludeSwitch)platform/adapter/include $(IncludeSwitch)drivers/inc 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)_wiota_ap 
ArLibs                 :=  "lib_wiota_ap" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch)./ 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := /home/jpwang/code/riscv/bin/riscv32-unknown-elf-ar rcu
CXX      := /home/jpwang/code/riscv/bin/riscv32-unknown-elf-g++
CC       := /home/jpwang/code/riscv/bin/riscv32-unknown-elf-gcc
CXXFLAGS :=  -g  -Os  -Wall  -march=rv32imfc   -ffunction-sections  -Iplatform -Ilibraries/inc  $(Preprocessors)
CFLAGS   :=  -g  -Os  -Wall  -march=rv32imfc  -ffunction-sections -Iplatform -Ilibraries/inc -D _RT_THREAD_ -D _ASIC_ -D UC_DEBUG -D MEM_OPT -D _FPGA_ -DLO_TYPE=H_LO -DIF_VALUE=100 -DHB_COE=1 -DFILTER_TAG=240 -D __AFC_SWITCH__ -D _FPGA_TRACE_TEST_ $(Preprocessors)
ASFLAGS  := -c  -g   -march=rv32imfc   -Iplatform -Ilibraries/inc 
AS       := /home/jpwang/code/riscv/bin/riscv32-unknown-elf-gcc


##
## User defined environment variables
##
CodeLiteDir:=/usr/share/codelite
Objects0=$(IntermediateDirectory)/applications_main.c$(ObjectSuffix) $(IntermediateDirectory)/libraries_crt0.flash.S$(ObjectSuffix) $(IntermediateDirectory)/drivers_board.c$(ObjectSuffix) $(IntermediateDirectory)/drivers_drv_usart.c$(ObjectSuffix) $(IntermediateDirectory)/src_clock.c$(ObjectSuffix) $(IntermediateDirectory)/src_components.c$(ObjectSuffix) $(IntermediateDirectory)/src_device.c$(ObjectSuffix) $(IntermediateDirectory)/src_idle.c$(ObjectSuffix) $(IntermediateDirectory)/src_ipc.c$(ObjectSuffix) $(IntermediateDirectory)/src_irq.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_kservice.c$(ObjectSuffix) $(IntermediateDirectory)/src_mem.c$(ObjectSuffix) $(IntermediateDirectory)/src_mempool.c$(ObjectSuffix) $(IntermediateDirectory)/src_object.c$(ObjectSuffix) $(IntermediateDirectory)/src_scheduler.c$(ObjectSuffix) $(IntermediateDirectory)/src_thread.c$(ObjectSuffix) $(IntermediateDirectory)/src_timer.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_string.c$(ObjectSuffix) $(IntermediateDirectory)/src_spim.c$(ObjectSuffix) $(IntermediateDirectory)/src_gpio.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_uc_addc.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_boot_strap.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_exceptions.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_i2c.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_int.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_pwm.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_rtc.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_spi.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_timer.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_uart.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_uc_uartx.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_utils.c$(ObjectSuffix) $(IntermediateDirectory)/src_uc_watchdog.c$(ObjectSuffix) $(IntermediateDirectory)/test_uc_wiota_interface_test.c$(ObjectSuffix) $(IntermediateDirectory)/src_adp_sem.c$(ObjectSuffix) $(IntermediateDirectory)/src_adp_mem.c$(ObjectSuffix) $(IntermediateDirectory)/src_adp_task.c$(ObjectSuffix) $(IntermediateDirectory)/src_adp_time.c$(ObjectSuffix) $(IntermediateDirectory)/src_adp_queue.c$(ObjectSuffix) 

Objects1=$(IntermediateDirectory)/src_ctrl_cmd.c$(ObjectSuffix) \
	$(IntermediateDirectory)/src_trace.c$(ObjectSuffix) $(IntermediateDirectory)/src_trcKernelPort.c$(ObjectSuffix) $(IntermediateDirectory)/src_trcStreamingPort.c$(ObjectSuffix) $(IntermediateDirectory)/src_trcStreamingRecorder.c$(ObjectSuffix) $(IntermediateDirectory)/src_vsi_trc.c$(ObjectSuffix) $(IntermediateDirectory)/finsh_shell.c$(ObjectSuffix) $(IntermediateDirectory)/finsh_cmd.c$(ObjectSuffix) $(IntermediateDirectory)/finsh_msh.c$(ObjectSuffix) $(IntermediateDirectory)/uc8088_stack.c$(ObjectSuffix) $(IntermediateDirectory)/uc8088_context_gcc.S$(ObjectSuffix) \
	$(IntermediateDirectory)/uc8088_entry_gcc.S$(ObjectSuffix) $(IntermediateDirectory)/misc_pin.c$(ObjectSuffix) $(IntermediateDirectory)/serial_serial.c$(ObjectSuffix) $(IntermediateDirectory)/src_completion.c$(ObjectSuffix) $(IntermediateDirectory)/src_dataqueue.c$(ObjectSuffix) $(IntermediateDirectory)/src_pipe.c$(ObjectSuffix) $(IntermediateDirectory)/src_ringblk_buf.c$(ObjectSuffix) $(IntermediateDirectory)/src_ringbuffer.c$(ObjectSuffix) $(IntermediateDirectory)/src_waitqueue.c$(ObjectSuffix) $(IntermediateDirectory)/src_workqueue.c$(ObjectSuffix) \
	$(IntermediateDirectory)/newlib_libc.c$(ObjectSuffix) $(IntermediateDirectory)/newlib_libc_syms.c$(ObjectSuffix) $(IntermediateDirectory)/newlib_stdio.c$(ObjectSuffix) $(IntermediateDirectory)/newlib_syscalls.c$(ObjectSuffix) 



Objects=$(Objects0) $(Objects1) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	@echo $(Objects1) >> $(ObjectsFileList)
	$(LinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)

PostBuild:
	@echo Executing Post Build commands ...
	riscv32-unknown-elf-objdump -d ./Debug/rtthread_app >./Debug/rtthread_app.asm
	./bintools -d ./Debug/rtthread_app rtthread_app.uzf
	cp rtthread_app.uzf rtthread_app.uzf.bin
	@echo Done

MakeIntermediateDirs:
	@test -d ./Debug || $(MakeDirCommand) ./Debug


$(IntermediateDirectory)/.d:
	@test -d ./Debug || $(MakeDirCommand) ./Debug

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/applications_main.c$(ObjectSuffix): applications/main.c $(IntermediateDirectory)/applications_main.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/applications/main.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/applications_main.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/applications_main.c$(DependSuffix): applications/main.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/applications_main.c$(ObjectSuffix) -MF$(IntermediateDirectory)/applications_main.c$(DependSuffix) -MM "applications/main.c"

$(IntermediateDirectory)/applications_main.c$(PreprocessSuffix): applications/main.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/applications_main.c$(PreprocessSuffix) "applications/main.c"

$(IntermediateDirectory)/libraries_crt0.flash.S$(ObjectSuffix): libraries/crt0.flash.S $(IntermediateDirectory)/libraries_crt0.flash.S$(DependSuffix)
	$(AS) "/home/jpwang/code/wiota_ap_customer/libraries/crt0.flash.S" $(ASFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/libraries_crt0.flash.S$(ObjectSuffix) -I$(IncludePath)
$(IntermediateDirectory)/libraries_crt0.flash.S$(DependSuffix): libraries/crt0.flash.S
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/libraries_crt0.flash.S$(ObjectSuffix) -MF$(IntermediateDirectory)/libraries_crt0.flash.S$(DependSuffix) -MM "libraries/crt0.flash.S"

$(IntermediateDirectory)/libraries_crt0.flash.S$(PreprocessSuffix): libraries/crt0.flash.S
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/libraries_crt0.flash.S$(PreprocessSuffix) "libraries/crt0.flash.S"

$(IntermediateDirectory)/drivers_board.c$(ObjectSuffix): drivers/board.c $(IntermediateDirectory)/drivers_board.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/drivers/board.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drivers_board.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drivers_board.c$(DependSuffix): drivers/board.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/drivers_board.c$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_board.c$(DependSuffix) -MM "drivers/board.c"

$(IntermediateDirectory)/drivers_board.c$(PreprocessSuffix): drivers/board.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drivers_board.c$(PreprocessSuffix) "drivers/board.c"

$(IntermediateDirectory)/drivers_drv_usart.c$(ObjectSuffix): drivers/drv_usart.c $(IntermediateDirectory)/drivers_drv_usart.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/drivers/drv_usart.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/drivers_drv_usart.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/drivers_drv_usart.c$(DependSuffix): drivers/drv_usart.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/drivers_drv_usart.c$(ObjectSuffix) -MF$(IntermediateDirectory)/drivers_drv_usart.c$(DependSuffix) -MM "drivers/drv_usart.c"

$(IntermediateDirectory)/drivers_drv_usart.c$(PreprocessSuffix): drivers/drv_usart.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/drivers_drv_usart.c$(PreprocessSuffix) "drivers/drv_usart.c"

$(IntermediateDirectory)/src_clock.c$(ObjectSuffix): rt-thread/src/clock.c $(IntermediateDirectory)/src_clock.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/clock.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_clock.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_clock.c$(DependSuffix): rt-thread/src/clock.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_clock.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_clock.c$(DependSuffix) -MM "rt-thread/src/clock.c"

$(IntermediateDirectory)/src_clock.c$(PreprocessSuffix): rt-thread/src/clock.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_clock.c$(PreprocessSuffix) "rt-thread/src/clock.c"

$(IntermediateDirectory)/src_components.c$(ObjectSuffix): rt-thread/src/components.c $(IntermediateDirectory)/src_components.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/components.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_components.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_components.c$(DependSuffix): rt-thread/src/components.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_components.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_components.c$(DependSuffix) -MM "rt-thread/src/components.c"

$(IntermediateDirectory)/src_components.c$(PreprocessSuffix): rt-thread/src/components.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_components.c$(PreprocessSuffix) "rt-thread/src/components.c"

$(IntermediateDirectory)/src_device.c$(ObjectSuffix): rt-thread/src/device.c $(IntermediateDirectory)/src_device.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/device.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_device.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_device.c$(DependSuffix): rt-thread/src/device.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_device.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_device.c$(DependSuffix) -MM "rt-thread/src/device.c"

$(IntermediateDirectory)/src_device.c$(PreprocessSuffix): rt-thread/src/device.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_device.c$(PreprocessSuffix) "rt-thread/src/device.c"

$(IntermediateDirectory)/src_idle.c$(ObjectSuffix): rt-thread/src/idle.c $(IntermediateDirectory)/src_idle.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/idle.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_idle.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_idle.c$(DependSuffix): rt-thread/src/idle.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_idle.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_idle.c$(DependSuffix) -MM "rt-thread/src/idle.c"

$(IntermediateDirectory)/src_idle.c$(PreprocessSuffix): rt-thread/src/idle.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_idle.c$(PreprocessSuffix) "rt-thread/src/idle.c"

$(IntermediateDirectory)/src_ipc.c$(ObjectSuffix): rt-thread/src/ipc.c $(IntermediateDirectory)/src_ipc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/ipc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ipc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ipc.c$(DependSuffix): rt-thread/src/ipc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ipc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ipc.c$(DependSuffix) -MM "rt-thread/src/ipc.c"

$(IntermediateDirectory)/src_ipc.c$(PreprocessSuffix): rt-thread/src/ipc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ipc.c$(PreprocessSuffix) "rt-thread/src/ipc.c"

$(IntermediateDirectory)/src_irq.c$(ObjectSuffix): rt-thread/src/irq.c $(IntermediateDirectory)/src_irq.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/irq.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_irq.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_irq.c$(DependSuffix): rt-thread/src/irq.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_irq.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_irq.c$(DependSuffix) -MM "rt-thread/src/irq.c"

$(IntermediateDirectory)/src_irq.c$(PreprocessSuffix): rt-thread/src/irq.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_irq.c$(PreprocessSuffix) "rt-thread/src/irq.c"

$(IntermediateDirectory)/src_kservice.c$(ObjectSuffix): rt-thread/src/kservice.c $(IntermediateDirectory)/src_kservice.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/kservice.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_kservice.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_kservice.c$(DependSuffix): rt-thread/src/kservice.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_kservice.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_kservice.c$(DependSuffix) -MM "rt-thread/src/kservice.c"

$(IntermediateDirectory)/src_kservice.c$(PreprocessSuffix): rt-thread/src/kservice.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_kservice.c$(PreprocessSuffix) "rt-thread/src/kservice.c"

$(IntermediateDirectory)/src_mem.c$(ObjectSuffix): rt-thread/src/mem.c $(IntermediateDirectory)/src_mem.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/mem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mem.c$(DependSuffix): rt-thread/src/mem.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mem.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mem.c$(DependSuffix) -MM "rt-thread/src/mem.c"

$(IntermediateDirectory)/src_mem.c$(PreprocessSuffix): rt-thread/src/mem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mem.c$(PreprocessSuffix) "rt-thread/src/mem.c"

$(IntermediateDirectory)/src_mempool.c$(ObjectSuffix): rt-thread/src/mempool.c $(IntermediateDirectory)/src_mempool.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/mempool.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_mempool.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_mempool.c$(DependSuffix): rt-thread/src/mempool.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_mempool.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_mempool.c$(DependSuffix) -MM "rt-thread/src/mempool.c"

$(IntermediateDirectory)/src_mempool.c$(PreprocessSuffix): rt-thread/src/mempool.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_mempool.c$(PreprocessSuffix) "rt-thread/src/mempool.c"

$(IntermediateDirectory)/src_object.c$(ObjectSuffix): rt-thread/src/object.c $(IntermediateDirectory)/src_object.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/object.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_object.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_object.c$(DependSuffix): rt-thread/src/object.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_object.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_object.c$(DependSuffix) -MM "rt-thread/src/object.c"

$(IntermediateDirectory)/src_object.c$(PreprocessSuffix): rt-thread/src/object.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_object.c$(PreprocessSuffix) "rt-thread/src/object.c"

$(IntermediateDirectory)/src_scheduler.c$(ObjectSuffix): rt-thread/src/scheduler.c $(IntermediateDirectory)/src_scheduler.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/scheduler.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_scheduler.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_scheduler.c$(DependSuffix): rt-thread/src/scheduler.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_scheduler.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_scheduler.c$(DependSuffix) -MM "rt-thread/src/scheduler.c"

$(IntermediateDirectory)/src_scheduler.c$(PreprocessSuffix): rt-thread/src/scheduler.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_scheduler.c$(PreprocessSuffix) "rt-thread/src/scheduler.c"

$(IntermediateDirectory)/src_thread.c$(ObjectSuffix): rt-thread/src/thread.c $(IntermediateDirectory)/src_thread.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/thread.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_thread.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_thread.c$(DependSuffix): rt-thread/src/thread.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_thread.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_thread.c$(DependSuffix) -MM "rt-thread/src/thread.c"

$(IntermediateDirectory)/src_thread.c$(PreprocessSuffix): rt-thread/src/thread.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_thread.c$(PreprocessSuffix) "rt-thread/src/thread.c"

$(IntermediateDirectory)/src_timer.c$(ObjectSuffix): rt-thread/src/timer.c $(IntermediateDirectory)/src_timer.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/src/timer.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_timer.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_timer.c$(DependSuffix): rt-thread/src/timer.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_timer.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_timer.c$(DependSuffix) -MM "rt-thread/src/timer.c"

$(IntermediateDirectory)/src_timer.c$(PreprocessSuffix): rt-thread/src/timer.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_timer.c$(PreprocessSuffix) "rt-thread/src/timer.c"

$(IntermediateDirectory)/src_uc_string.c$(ObjectSuffix): libraries/src/uc_string.c $(IntermediateDirectory)/src_uc_string.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_string.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_string.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_string.c$(DependSuffix): libraries/src/uc_string.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_string.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_string.c$(DependSuffix) -MM "libraries/src/uc_string.c"

$(IntermediateDirectory)/src_uc_string.c$(PreprocessSuffix): libraries/src/uc_string.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_string.c$(PreprocessSuffix) "libraries/src/uc_string.c"

$(IntermediateDirectory)/src_spim.c$(ObjectSuffix): libraries/src/spim.c $(IntermediateDirectory)/src_spim.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/spim.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_spim.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_spim.c$(DependSuffix): libraries/src/spim.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_spim.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_spim.c$(DependSuffix) -MM "libraries/src/spim.c"

$(IntermediateDirectory)/src_spim.c$(PreprocessSuffix): libraries/src/spim.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_spim.c$(PreprocessSuffix) "libraries/src/spim.c"

$(IntermediateDirectory)/src_gpio.c$(ObjectSuffix): libraries/src/gpio.c $(IntermediateDirectory)/src_gpio.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/gpio.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_gpio.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_gpio.c$(DependSuffix): libraries/src/gpio.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_gpio.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_gpio.c$(DependSuffix) -MM "libraries/src/gpio.c"

$(IntermediateDirectory)/src_gpio.c$(PreprocessSuffix): libraries/src/gpio.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_gpio.c$(PreprocessSuffix) "libraries/src/gpio.c"

$(IntermediateDirectory)/src_uc_addc.c$(ObjectSuffix): libraries/src/uc_addc.c $(IntermediateDirectory)/src_uc_addc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_addc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_addc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_addc.c$(DependSuffix): libraries/src/uc_addc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_addc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_addc.c$(DependSuffix) -MM "libraries/src/uc_addc.c"

$(IntermediateDirectory)/src_uc_addc.c$(PreprocessSuffix): libraries/src/uc_addc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_addc.c$(PreprocessSuffix) "libraries/src/uc_addc.c"

$(IntermediateDirectory)/src_uc_boot_strap.c$(ObjectSuffix): libraries/src/uc_boot_strap.c $(IntermediateDirectory)/src_uc_boot_strap.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_boot_strap.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_boot_strap.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_boot_strap.c$(DependSuffix): libraries/src/uc_boot_strap.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_boot_strap.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_boot_strap.c$(DependSuffix) -MM "libraries/src/uc_boot_strap.c"

$(IntermediateDirectory)/src_uc_boot_strap.c$(PreprocessSuffix): libraries/src/uc_boot_strap.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_boot_strap.c$(PreprocessSuffix) "libraries/src/uc_boot_strap.c"

$(IntermediateDirectory)/src_uc_exceptions.c$(ObjectSuffix): libraries/src/uc_exceptions.c $(IntermediateDirectory)/src_uc_exceptions.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_exceptions.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_exceptions.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_exceptions.c$(DependSuffix): libraries/src/uc_exceptions.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_exceptions.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_exceptions.c$(DependSuffix) -MM "libraries/src/uc_exceptions.c"

$(IntermediateDirectory)/src_uc_exceptions.c$(PreprocessSuffix): libraries/src/uc_exceptions.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_exceptions.c$(PreprocessSuffix) "libraries/src/uc_exceptions.c"

$(IntermediateDirectory)/src_uc_i2c.c$(ObjectSuffix): libraries/src/uc_i2c.c $(IntermediateDirectory)/src_uc_i2c.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_i2c.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_i2c.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_i2c.c$(DependSuffix): libraries/src/uc_i2c.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_i2c.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_i2c.c$(DependSuffix) -MM "libraries/src/uc_i2c.c"

$(IntermediateDirectory)/src_uc_i2c.c$(PreprocessSuffix): libraries/src/uc_i2c.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_i2c.c$(PreprocessSuffix) "libraries/src/uc_i2c.c"

$(IntermediateDirectory)/src_uc_int.c$(ObjectSuffix): libraries/src/uc_int.c $(IntermediateDirectory)/src_uc_int.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_int.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_int.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_int.c$(DependSuffix): libraries/src/uc_int.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_int.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_int.c$(DependSuffix) -MM "libraries/src/uc_int.c"

$(IntermediateDirectory)/src_uc_int.c$(PreprocessSuffix): libraries/src/uc_int.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_int.c$(PreprocessSuffix) "libraries/src/uc_int.c"

$(IntermediateDirectory)/src_uc_pwm.c$(ObjectSuffix): libraries/src/uc_pwm.c $(IntermediateDirectory)/src_uc_pwm.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_pwm.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_pwm.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_pwm.c$(DependSuffix): libraries/src/uc_pwm.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_pwm.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_pwm.c$(DependSuffix) -MM "libraries/src/uc_pwm.c"

$(IntermediateDirectory)/src_uc_pwm.c$(PreprocessSuffix): libraries/src/uc_pwm.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_pwm.c$(PreprocessSuffix) "libraries/src/uc_pwm.c"

$(IntermediateDirectory)/src_uc_rtc.c$(ObjectSuffix): libraries/src/uc_rtc.c $(IntermediateDirectory)/src_uc_rtc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_rtc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_rtc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_rtc.c$(DependSuffix): libraries/src/uc_rtc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_rtc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_rtc.c$(DependSuffix) -MM "libraries/src/uc_rtc.c"

$(IntermediateDirectory)/src_uc_rtc.c$(PreprocessSuffix): libraries/src/uc_rtc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_rtc.c$(PreprocessSuffix) "libraries/src/uc_rtc.c"

$(IntermediateDirectory)/src_uc_spi.c$(ObjectSuffix): libraries/src/uc_spi.c $(IntermediateDirectory)/src_uc_spi.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_spi.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_spi.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_spi.c$(DependSuffix): libraries/src/uc_spi.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_spi.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_spi.c$(DependSuffix) -MM "libraries/src/uc_spi.c"

$(IntermediateDirectory)/src_uc_spi.c$(PreprocessSuffix): libraries/src/uc_spi.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_spi.c$(PreprocessSuffix) "libraries/src/uc_spi.c"

$(IntermediateDirectory)/src_uc_timer.c$(ObjectSuffix): libraries/src/uc_timer.c $(IntermediateDirectory)/src_uc_timer.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_timer.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_timer.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_timer.c$(DependSuffix): libraries/src/uc_timer.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_timer.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_timer.c$(DependSuffix) -MM "libraries/src/uc_timer.c"

$(IntermediateDirectory)/src_uc_timer.c$(PreprocessSuffix): libraries/src/uc_timer.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_timer.c$(PreprocessSuffix) "libraries/src/uc_timer.c"

$(IntermediateDirectory)/src_uc_uart.c$(ObjectSuffix): libraries/src/uc_uart.c $(IntermediateDirectory)/src_uc_uart.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_uart.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_uart.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_uart.c$(DependSuffix): libraries/src/uc_uart.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_uart.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_uart.c$(DependSuffix) -MM "libraries/src/uc_uart.c"

$(IntermediateDirectory)/src_uc_uart.c$(PreprocessSuffix): libraries/src/uc_uart.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_uart.c$(PreprocessSuffix) "libraries/src/uc_uart.c"

$(IntermediateDirectory)/src_uc_uartx.c$(ObjectSuffix): libraries/src/uc_uartx.c $(IntermediateDirectory)/src_uc_uartx.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_uartx.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_uartx.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_uartx.c$(DependSuffix): libraries/src/uc_uartx.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_uartx.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_uartx.c$(DependSuffix) -MM "libraries/src/uc_uartx.c"

$(IntermediateDirectory)/src_uc_uartx.c$(PreprocessSuffix): libraries/src/uc_uartx.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_uartx.c$(PreprocessSuffix) "libraries/src/uc_uartx.c"

$(IntermediateDirectory)/src_uc_utils.c$(ObjectSuffix): libraries/src/uc_utils.c $(IntermediateDirectory)/src_uc_utils.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_utils.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_utils.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_utils.c$(DependSuffix): libraries/src/uc_utils.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_utils.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_utils.c$(DependSuffix) -MM "libraries/src/uc_utils.c"

$(IntermediateDirectory)/src_uc_utils.c$(PreprocessSuffix): libraries/src/uc_utils.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_utils.c$(PreprocessSuffix) "libraries/src/uc_utils.c"

$(IntermediateDirectory)/src_uc_watchdog.c$(ObjectSuffix): libraries/src/uc_watchdog.c $(IntermediateDirectory)/src_uc_watchdog.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/libraries/src/uc_watchdog.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_uc_watchdog.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_uc_watchdog.c$(DependSuffix): libraries/src/uc_watchdog.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_uc_watchdog.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_uc_watchdog.c$(DependSuffix) -MM "libraries/src/uc_watchdog.c"

$(IntermediateDirectory)/src_uc_watchdog.c$(PreprocessSuffix): libraries/src/uc_watchdog.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_uc_watchdog.c$(PreprocessSuffix) "libraries/src/uc_watchdog.c"

$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(ObjectSuffix): PS/app/test/uc_wiota_interface_test.c $(IntermediateDirectory)/test_uc_wiota_interface_test.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/PS/app/test/uc_wiota_interface_test.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(DependSuffix): PS/app/test/uc_wiota_interface_test.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(ObjectSuffix) -MF$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(DependSuffix) -MM "PS/app/test/uc_wiota_interface_test.c"

$(IntermediateDirectory)/test_uc_wiota_interface_test.c$(PreprocessSuffix): PS/app/test/uc_wiota_interface_test.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/test_uc_wiota_interface_test.c$(PreprocessSuffix) "PS/app/test/uc_wiota_interface_test.c"

$(IntermediateDirectory)/src_adp_sem.c$(ObjectSuffix): platform/adapter/src/adp_sem.c $(IntermediateDirectory)/src_adp_sem.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/adapter/src/adp_sem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_adp_sem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_adp_sem.c$(DependSuffix): platform/adapter/src/adp_sem.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_adp_sem.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_adp_sem.c$(DependSuffix) -MM "platform/adapter/src/adp_sem.c"

$(IntermediateDirectory)/src_adp_sem.c$(PreprocessSuffix): platform/adapter/src/adp_sem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_adp_sem.c$(PreprocessSuffix) "platform/adapter/src/adp_sem.c"

$(IntermediateDirectory)/src_adp_mem.c$(ObjectSuffix): platform/adapter/src/adp_mem.c $(IntermediateDirectory)/src_adp_mem.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/adapter/src/adp_mem.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_adp_mem.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_adp_mem.c$(DependSuffix): platform/adapter/src/adp_mem.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_adp_mem.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_adp_mem.c$(DependSuffix) -MM "platform/adapter/src/adp_mem.c"

$(IntermediateDirectory)/src_adp_mem.c$(PreprocessSuffix): platform/adapter/src/adp_mem.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_adp_mem.c$(PreprocessSuffix) "platform/adapter/src/adp_mem.c"

$(IntermediateDirectory)/src_adp_task.c$(ObjectSuffix): platform/adapter/src/adp_task.c $(IntermediateDirectory)/src_adp_task.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/adapter/src/adp_task.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_adp_task.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_adp_task.c$(DependSuffix): platform/adapter/src/adp_task.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_adp_task.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_adp_task.c$(DependSuffix) -MM "platform/adapter/src/adp_task.c"

$(IntermediateDirectory)/src_adp_task.c$(PreprocessSuffix): platform/adapter/src/adp_task.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_adp_task.c$(PreprocessSuffix) "platform/adapter/src/adp_task.c"

$(IntermediateDirectory)/src_adp_time.c$(ObjectSuffix): platform/adapter/src/adp_time.c $(IntermediateDirectory)/src_adp_time.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/adapter/src/adp_time.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_adp_time.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_adp_time.c$(DependSuffix): platform/adapter/src/adp_time.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_adp_time.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_adp_time.c$(DependSuffix) -MM "platform/adapter/src/adp_time.c"

$(IntermediateDirectory)/src_adp_time.c$(PreprocessSuffix): platform/adapter/src/adp_time.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_adp_time.c$(PreprocessSuffix) "platform/adapter/src/adp_time.c"

$(IntermediateDirectory)/src_adp_queue.c$(ObjectSuffix): platform/adapter/src/adp_queue.c $(IntermediateDirectory)/src_adp_queue.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/adapter/src/adp_queue.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_adp_queue.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_adp_queue.c$(DependSuffix): platform/adapter/src/adp_queue.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_adp_queue.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_adp_queue.c$(DependSuffix) -MM "platform/adapter/src/adp_queue.c"

$(IntermediateDirectory)/src_adp_queue.c$(PreprocessSuffix): platform/adapter/src/adp_queue.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_adp_queue.c$(PreprocessSuffix) "platform/adapter/src/adp_queue.c"

$(IntermediateDirectory)/src_ctrl_cmd.c$(ObjectSuffix): platform/trace/src/ctrl_cmd.c $(IntermediateDirectory)/src_ctrl_cmd.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/ctrl_cmd.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ctrl_cmd.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ctrl_cmd.c$(DependSuffix): platform/trace/src/ctrl_cmd.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ctrl_cmd.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ctrl_cmd.c$(DependSuffix) -MM "platform/trace/src/ctrl_cmd.c"

$(IntermediateDirectory)/src_ctrl_cmd.c$(PreprocessSuffix): platform/trace/src/ctrl_cmd.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ctrl_cmd.c$(PreprocessSuffix) "platform/trace/src/ctrl_cmd.c"

$(IntermediateDirectory)/src_trace.c$(ObjectSuffix): platform/trace/src/trace.c $(IntermediateDirectory)/src_trace.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/trace.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_trace.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_trace.c$(DependSuffix): platform/trace/src/trace.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_trace.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_trace.c$(DependSuffix) -MM "platform/trace/src/trace.c"

$(IntermediateDirectory)/src_trace.c$(PreprocessSuffix): platform/trace/src/trace.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_trace.c$(PreprocessSuffix) "platform/trace/src/trace.c"

$(IntermediateDirectory)/src_trcKernelPort.c$(ObjectSuffix): platform/trace/src/trcKernelPort.c $(IntermediateDirectory)/src_trcKernelPort.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/trcKernelPort.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_trcKernelPort.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_trcKernelPort.c$(DependSuffix): platform/trace/src/trcKernelPort.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_trcKernelPort.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_trcKernelPort.c$(DependSuffix) -MM "platform/trace/src/trcKernelPort.c"

$(IntermediateDirectory)/src_trcKernelPort.c$(PreprocessSuffix): platform/trace/src/trcKernelPort.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_trcKernelPort.c$(PreprocessSuffix) "platform/trace/src/trcKernelPort.c"

$(IntermediateDirectory)/src_trcStreamingPort.c$(ObjectSuffix): platform/trace/src/trcStreamingPort.c $(IntermediateDirectory)/src_trcStreamingPort.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/trcStreamingPort.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_trcStreamingPort.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_trcStreamingPort.c$(DependSuffix): platform/trace/src/trcStreamingPort.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_trcStreamingPort.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_trcStreamingPort.c$(DependSuffix) -MM "platform/trace/src/trcStreamingPort.c"

$(IntermediateDirectory)/src_trcStreamingPort.c$(PreprocessSuffix): platform/trace/src/trcStreamingPort.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_trcStreamingPort.c$(PreprocessSuffix) "platform/trace/src/trcStreamingPort.c"

$(IntermediateDirectory)/src_trcStreamingRecorder.c$(ObjectSuffix): platform/trace/src/trcStreamingRecorder.c $(IntermediateDirectory)/src_trcStreamingRecorder.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/trcStreamingRecorder.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_trcStreamingRecorder.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_trcStreamingRecorder.c$(DependSuffix): platform/trace/src/trcStreamingRecorder.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_trcStreamingRecorder.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_trcStreamingRecorder.c$(DependSuffix) -MM "platform/trace/src/trcStreamingRecorder.c"

$(IntermediateDirectory)/src_trcStreamingRecorder.c$(PreprocessSuffix): platform/trace/src/trcStreamingRecorder.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_trcStreamingRecorder.c$(PreprocessSuffix) "platform/trace/src/trcStreamingRecorder.c"

$(IntermediateDirectory)/src_vsi_trc.c$(ObjectSuffix): platform/trace/src/vsi_trc.c $(IntermediateDirectory)/src_vsi_trc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/platform/trace/src/vsi_trc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_vsi_trc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_vsi_trc.c$(DependSuffix): platform/trace/src/vsi_trc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_vsi_trc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_vsi_trc.c$(DependSuffix) -MM "platform/trace/src/vsi_trc.c"

$(IntermediateDirectory)/src_vsi_trc.c$(PreprocessSuffix): platform/trace/src/vsi_trc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_vsi_trc.c$(PreprocessSuffix) "platform/trace/src/vsi_trc.c"

$(IntermediateDirectory)/finsh_shell.c$(ObjectSuffix): rt-thread/components/finsh/shell.c $(IntermediateDirectory)/finsh_shell.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/finsh/shell.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/finsh_shell.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/finsh_shell.c$(DependSuffix): rt-thread/components/finsh/shell.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/finsh_shell.c$(ObjectSuffix) -MF$(IntermediateDirectory)/finsh_shell.c$(DependSuffix) -MM "rt-thread/components/finsh/shell.c"

$(IntermediateDirectory)/finsh_shell.c$(PreprocessSuffix): rt-thread/components/finsh/shell.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/finsh_shell.c$(PreprocessSuffix) "rt-thread/components/finsh/shell.c"

$(IntermediateDirectory)/finsh_cmd.c$(ObjectSuffix): rt-thread/components/finsh/cmd.c $(IntermediateDirectory)/finsh_cmd.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/finsh/cmd.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/finsh_cmd.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/finsh_cmd.c$(DependSuffix): rt-thread/components/finsh/cmd.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/finsh_cmd.c$(ObjectSuffix) -MF$(IntermediateDirectory)/finsh_cmd.c$(DependSuffix) -MM "rt-thread/components/finsh/cmd.c"

$(IntermediateDirectory)/finsh_cmd.c$(PreprocessSuffix): rt-thread/components/finsh/cmd.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/finsh_cmd.c$(PreprocessSuffix) "rt-thread/components/finsh/cmd.c"

$(IntermediateDirectory)/finsh_msh.c$(ObjectSuffix): rt-thread/components/finsh/msh.c $(IntermediateDirectory)/finsh_msh.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/finsh/msh.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/finsh_msh.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/finsh_msh.c$(DependSuffix): rt-thread/components/finsh/msh.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/finsh_msh.c$(ObjectSuffix) -MF$(IntermediateDirectory)/finsh_msh.c$(DependSuffix) -MM "rt-thread/components/finsh/msh.c"

$(IntermediateDirectory)/finsh_msh.c$(PreprocessSuffix): rt-thread/components/finsh/msh.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/finsh_msh.c$(PreprocessSuffix) "rt-thread/components/finsh/msh.c"

$(IntermediateDirectory)/uc8088_stack.c$(ObjectSuffix): rt-thread/libcpu/risc-v/uc8088/stack.c $(IntermediateDirectory)/uc8088_stack.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/libcpu/risc-v/uc8088/stack.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/uc8088_stack.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/uc8088_stack.c$(DependSuffix): rt-thread/libcpu/risc-v/uc8088/stack.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/uc8088_stack.c$(ObjectSuffix) -MF$(IntermediateDirectory)/uc8088_stack.c$(DependSuffix) -MM "rt-thread/libcpu/risc-v/uc8088/stack.c"

$(IntermediateDirectory)/uc8088_stack.c$(PreprocessSuffix): rt-thread/libcpu/risc-v/uc8088/stack.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/uc8088_stack.c$(PreprocessSuffix) "rt-thread/libcpu/risc-v/uc8088/stack.c"

$(IntermediateDirectory)/uc8088_context_gcc.S$(ObjectSuffix): rt-thread/libcpu/risc-v/uc8088/context_gcc.S $(IntermediateDirectory)/uc8088_context_gcc.S$(DependSuffix)
	$(AS) "/home/jpwang/code/wiota_ap_customer/rt-thread/libcpu/risc-v/uc8088/context_gcc.S" $(ASFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/uc8088_context_gcc.S$(ObjectSuffix) -I$(IncludePath)
$(IntermediateDirectory)/uc8088_context_gcc.S$(DependSuffix): rt-thread/libcpu/risc-v/uc8088/context_gcc.S
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/uc8088_context_gcc.S$(ObjectSuffix) -MF$(IntermediateDirectory)/uc8088_context_gcc.S$(DependSuffix) -MM "rt-thread/libcpu/risc-v/uc8088/context_gcc.S"

$(IntermediateDirectory)/uc8088_context_gcc.S$(PreprocessSuffix): rt-thread/libcpu/risc-v/uc8088/context_gcc.S
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/uc8088_context_gcc.S$(PreprocessSuffix) "rt-thread/libcpu/risc-v/uc8088/context_gcc.S"

$(IntermediateDirectory)/uc8088_entry_gcc.S$(ObjectSuffix): rt-thread/libcpu/risc-v/uc8088/entry_gcc.S $(IntermediateDirectory)/uc8088_entry_gcc.S$(DependSuffix)
	$(AS) "/home/jpwang/code/wiota_ap_customer/rt-thread/libcpu/risc-v/uc8088/entry_gcc.S" $(ASFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/uc8088_entry_gcc.S$(ObjectSuffix) -I$(IncludePath)
$(IntermediateDirectory)/uc8088_entry_gcc.S$(DependSuffix): rt-thread/libcpu/risc-v/uc8088/entry_gcc.S
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/uc8088_entry_gcc.S$(ObjectSuffix) -MF$(IntermediateDirectory)/uc8088_entry_gcc.S$(DependSuffix) -MM "rt-thread/libcpu/risc-v/uc8088/entry_gcc.S"

$(IntermediateDirectory)/uc8088_entry_gcc.S$(PreprocessSuffix): rt-thread/libcpu/risc-v/uc8088/entry_gcc.S
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/uc8088_entry_gcc.S$(PreprocessSuffix) "rt-thread/libcpu/risc-v/uc8088/entry_gcc.S"

$(IntermediateDirectory)/misc_pin.c$(ObjectSuffix): rt-thread/components/drivers/misc/pin.c $(IntermediateDirectory)/misc_pin.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/misc/pin.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/misc_pin.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/misc_pin.c$(DependSuffix): rt-thread/components/drivers/misc/pin.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/misc_pin.c$(ObjectSuffix) -MF$(IntermediateDirectory)/misc_pin.c$(DependSuffix) -MM "rt-thread/components/drivers/misc/pin.c"

$(IntermediateDirectory)/misc_pin.c$(PreprocessSuffix): rt-thread/components/drivers/misc/pin.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/misc_pin.c$(PreprocessSuffix) "rt-thread/components/drivers/misc/pin.c"

$(IntermediateDirectory)/serial_serial.c$(ObjectSuffix): rt-thread/components/drivers/serial/serial.c $(IntermediateDirectory)/serial_serial.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/serial/serial.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/serial_serial.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/serial_serial.c$(DependSuffix): rt-thread/components/drivers/serial/serial.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/serial_serial.c$(ObjectSuffix) -MF$(IntermediateDirectory)/serial_serial.c$(DependSuffix) -MM "rt-thread/components/drivers/serial/serial.c"

$(IntermediateDirectory)/serial_serial.c$(PreprocessSuffix): rt-thread/components/drivers/serial/serial.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/serial_serial.c$(PreprocessSuffix) "rt-thread/components/drivers/serial/serial.c"

$(IntermediateDirectory)/src_completion.c$(ObjectSuffix): rt-thread/components/drivers/src/completion.c $(IntermediateDirectory)/src_completion.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/completion.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_completion.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_completion.c$(DependSuffix): rt-thread/components/drivers/src/completion.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_completion.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_completion.c$(DependSuffix) -MM "rt-thread/components/drivers/src/completion.c"

$(IntermediateDirectory)/src_completion.c$(PreprocessSuffix): rt-thread/components/drivers/src/completion.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_completion.c$(PreprocessSuffix) "rt-thread/components/drivers/src/completion.c"

$(IntermediateDirectory)/src_dataqueue.c$(ObjectSuffix): rt-thread/components/drivers/src/dataqueue.c $(IntermediateDirectory)/src_dataqueue.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/dataqueue.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_dataqueue.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_dataqueue.c$(DependSuffix): rt-thread/components/drivers/src/dataqueue.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_dataqueue.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_dataqueue.c$(DependSuffix) -MM "rt-thread/components/drivers/src/dataqueue.c"

$(IntermediateDirectory)/src_dataqueue.c$(PreprocessSuffix): rt-thread/components/drivers/src/dataqueue.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_dataqueue.c$(PreprocessSuffix) "rt-thread/components/drivers/src/dataqueue.c"

$(IntermediateDirectory)/src_pipe.c$(ObjectSuffix): rt-thread/components/drivers/src/pipe.c $(IntermediateDirectory)/src_pipe.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/pipe.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_pipe.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_pipe.c$(DependSuffix): rt-thread/components/drivers/src/pipe.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_pipe.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_pipe.c$(DependSuffix) -MM "rt-thread/components/drivers/src/pipe.c"

$(IntermediateDirectory)/src_pipe.c$(PreprocessSuffix): rt-thread/components/drivers/src/pipe.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_pipe.c$(PreprocessSuffix) "rt-thread/components/drivers/src/pipe.c"

$(IntermediateDirectory)/src_ringblk_buf.c$(ObjectSuffix): rt-thread/components/drivers/src/ringblk_buf.c $(IntermediateDirectory)/src_ringblk_buf.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/ringblk_buf.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ringblk_buf.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ringblk_buf.c$(DependSuffix): rt-thread/components/drivers/src/ringblk_buf.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ringblk_buf.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ringblk_buf.c$(DependSuffix) -MM "rt-thread/components/drivers/src/ringblk_buf.c"

$(IntermediateDirectory)/src_ringblk_buf.c$(PreprocessSuffix): rt-thread/components/drivers/src/ringblk_buf.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ringblk_buf.c$(PreprocessSuffix) "rt-thread/components/drivers/src/ringblk_buf.c"

$(IntermediateDirectory)/src_ringbuffer.c$(ObjectSuffix): rt-thread/components/drivers/src/ringbuffer.c $(IntermediateDirectory)/src_ringbuffer.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/ringbuffer.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_ringbuffer.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_ringbuffer.c$(DependSuffix): rt-thread/components/drivers/src/ringbuffer.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_ringbuffer.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_ringbuffer.c$(DependSuffix) -MM "rt-thread/components/drivers/src/ringbuffer.c"

$(IntermediateDirectory)/src_ringbuffer.c$(PreprocessSuffix): rt-thread/components/drivers/src/ringbuffer.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_ringbuffer.c$(PreprocessSuffix) "rt-thread/components/drivers/src/ringbuffer.c"

$(IntermediateDirectory)/src_waitqueue.c$(ObjectSuffix): rt-thread/components/drivers/src/waitqueue.c $(IntermediateDirectory)/src_waitqueue.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/waitqueue.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_waitqueue.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_waitqueue.c$(DependSuffix): rt-thread/components/drivers/src/waitqueue.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_waitqueue.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_waitqueue.c$(DependSuffix) -MM "rt-thread/components/drivers/src/waitqueue.c"

$(IntermediateDirectory)/src_waitqueue.c$(PreprocessSuffix): rt-thread/components/drivers/src/waitqueue.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_waitqueue.c$(PreprocessSuffix) "rt-thread/components/drivers/src/waitqueue.c"

$(IntermediateDirectory)/src_workqueue.c$(ObjectSuffix): rt-thread/components/drivers/src/workqueue.c $(IntermediateDirectory)/src_workqueue.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/drivers/src/workqueue.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/src_workqueue.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/src_workqueue.c$(DependSuffix): rt-thread/components/drivers/src/workqueue.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/src_workqueue.c$(ObjectSuffix) -MF$(IntermediateDirectory)/src_workqueue.c$(DependSuffix) -MM "rt-thread/components/drivers/src/workqueue.c"

$(IntermediateDirectory)/src_workqueue.c$(PreprocessSuffix): rt-thread/components/drivers/src/workqueue.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/src_workqueue.c$(PreprocessSuffix) "rt-thread/components/drivers/src/workqueue.c"

$(IntermediateDirectory)/newlib_libc.c$(ObjectSuffix): rt-thread/components/libc/compilers/newlib/libc.c $(IntermediateDirectory)/newlib_libc.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/libc/compilers/newlib/libc.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/newlib_libc.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/newlib_libc.c$(DependSuffix): rt-thread/components/libc/compilers/newlib/libc.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/newlib_libc.c$(ObjectSuffix) -MF$(IntermediateDirectory)/newlib_libc.c$(DependSuffix) -MM "rt-thread/components/libc/compilers/newlib/libc.c"

$(IntermediateDirectory)/newlib_libc.c$(PreprocessSuffix): rt-thread/components/libc/compilers/newlib/libc.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/newlib_libc.c$(PreprocessSuffix) "rt-thread/components/libc/compilers/newlib/libc.c"

$(IntermediateDirectory)/newlib_libc_syms.c$(ObjectSuffix): rt-thread/components/libc/compilers/newlib/libc_syms.c $(IntermediateDirectory)/newlib_libc_syms.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/libc/compilers/newlib/libc_syms.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/newlib_libc_syms.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/newlib_libc_syms.c$(DependSuffix): rt-thread/components/libc/compilers/newlib/libc_syms.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/newlib_libc_syms.c$(ObjectSuffix) -MF$(IntermediateDirectory)/newlib_libc_syms.c$(DependSuffix) -MM "rt-thread/components/libc/compilers/newlib/libc_syms.c"

$(IntermediateDirectory)/newlib_libc_syms.c$(PreprocessSuffix): rt-thread/components/libc/compilers/newlib/libc_syms.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/newlib_libc_syms.c$(PreprocessSuffix) "rt-thread/components/libc/compilers/newlib/libc_syms.c"

$(IntermediateDirectory)/newlib_stdio.c$(ObjectSuffix): rt-thread/components/libc/compilers/newlib/stdio.c $(IntermediateDirectory)/newlib_stdio.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/libc/compilers/newlib/stdio.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/newlib_stdio.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/newlib_stdio.c$(DependSuffix): rt-thread/components/libc/compilers/newlib/stdio.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/newlib_stdio.c$(ObjectSuffix) -MF$(IntermediateDirectory)/newlib_stdio.c$(DependSuffix) -MM "rt-thread/components/libc/compilers/newlib/stdio.c"

$(IntermediateDirectory)/newlib_stdio.c$(PreprocessSuffix): rt-thread/components/libc/compilers/newlib/stdio.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/newlib_stdio.c$(PreprocessSuffix) "rt-thread/components/libc/compilers/newlib/stdio.c"

$(IntermediateDirectory)/newlib_syscalls.c$(ObjectSuffix): rt-thread/components/libc/compilers/newlib/syscalls.c $(IntermediateDirectory)/newlib_syscalls.c$(DependSuffix)
	$(CC) $(SourceSwitch) "/home/jpwang/code/wiota_ap_customer/rt-thread/components/libc/compilers/newlib/syscalls.c" $(CFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/newlib_syscalls.c$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/newlib_syscalls.c$(DependSuffix): rt-thread/components/libc/compilers/newlib/syscalls.c
	@$(CC) $(CFLAGS) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/newlib_syscalls.c$(ObjectSuffix) -MF$(IntermediateDirectory)/newlib_syscalls.c$(DependSuffix) -MM "rt-thread/components/libc/compilers/newlib/syscalls.c"

$(IntermediateDirectory)/newlib_syscalls.c$(PreprocessSuffix): rt-thread/components/libc/compilers/newlib/syscalls.c
	$(CC) $(CFLAGS) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/newlib_syscalls.c$(PreprocessSuffix) "rt-thread/components/libc/compilers/newlib/syscalls.c"


-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Debug/



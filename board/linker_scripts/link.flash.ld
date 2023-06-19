/* not needed, but we need separate linker scripts anyway */
OUTPUT_ARCH(riscv)
/* required to correctly link newlib */
GROUP( -lc -lgloss -lgcc -lsupc++ )

SEARCH_DIR(.)
__DYNAMIC  =  0;

MEMORY
{
    uc_boot     : ORIGIN = 0x00000000, LENGTH = 0x7000   /* 28K uncompressed code sys data. */
    uc_rom      : ORIGIN = 0x00007000, LENGTH = 0x3000   /* 12K uncompressed code sys data, NO RODATA!!!! */
    psram       : ORIGIN = 0x0000A000, LENGTH = 0x19000  /* 100k */
    gpsram      : ORIGIN = 0x00023000, LENGTH = 0x2E000  /* 184k */
    instrram    : ORIGIN = 0x00051000, LENGTH = 0x98000  /* 608k */
    datarom     : ORIGIN = 0x00200000, LENGTH = 0xF7000  /* 1M - 0x2000*/
    staticdata  : ORIGIN = 0x002F7000, LENGTH = 0x2000   /* 8K */
    dataram     : ORIGIN = 0x00300000, LENGTH = 0x3A000  /* d_sram ends at 256K 340000  0x3B490 */
    ccetraceram : ORIGIN = 0x0033A000, LENGTH = 0x1490   /* 5k temp */
    rvtraceram  : ORIGIN = 0x0033B490, LENGTH = 0x1770   /* 6000 byte */
    stack       : ORIGIN = 0x0033CC00, LENGTH = 0x2000   /* stack and 32ec00 is reserved for trace buffer */
    ota_flag    : ORIGIN = 0x00400000, LENGTH = 0x100
}

/* Stack information variables */
_min_stack     = 0x2000;   /* 4K - minimum stack space to reserve */
_stack_len     = LENGTH(stack);
_stack_start   = ORIGIN(stack) + LENGTH(stack);
_rom_end       = ORIGIN(uc_rom) + LENGTH(uc_rom);
_rom_start     = ORIGIN(uc_rom);

/* We have to align each sector to word boundaries as our current s19->slm
 * conversion scripts are not able to handle non-word aligned sections. */

SECTIONS
{
   .uc_boot :
   {
        . = LENGTH(uc_boot)  - 4 ;
        LONG(0x00000000)
   } > uc_boot = 0x00000000

    .vectors :
    {
         KEEP(*(.vectors));
    } > uc_rom

    .boot : ALIGN(512)
    {
        *(.critical)
    } > uc_rom

    .fastfunc :
    {
        . = ALIGN(4);
        *(.crt0);
        *(.reset)
        *(.download);
        . += 80;
        . = ALIGN(2048);
        _stbl_end = .;
    } > uc_rom

/*functions called only once during startup */
    .init :
    {
        . = ALIGN(4);
        *(.init)
    } > instrram

    .pstext :
    {
        *cce_log_dump*(.text*);
        *factory_test*(.text*);
        *context*(.text*);
        *download_buffer*(.text*);
        *iote_context_mng*(.text*);
        *scheduler_*(.text*);
        *ap_pgw_module_task*(.text*);
        *ap_pgw_msg*(.text*);
        *link_buffer*(.text*);
        *uc_common*(.text*);
        *wiota_crc*(.text*);
        *base_station_config*(.text*);
        *global_station_config*(.text*);
        *uc_task*(.text*);
        *adp_*(.text*);
        *uc_queue*(.text*);
        *ctrl_cmd*(.text*);
        *trace*(.text*);
        *trcKernelPort*(.text*);
        *trcStreamingPort*(.text*);
        *trcStreamingRecorder*(.text*);
        *vsi_trc*(.text*);
        *trace*(.text*);
        *spi_buf_*(.text*);
        *riscv_cce_communication*(.text*);
        *easy_ptp*(.text*);
        *gnss_main*(.text*);
        *gnss_driver*(.text*);
        *time_service*(.text*);
        *ptpv2_*(.text*);
        . = LENGTH(psram)  - 4 ;
        LONG(0x00000000)
    } > psram

    .gpstext :
    {
        *TimeMonitor*(.text*);
        *qaTest*(.text*);
        *acqStat*(.text*);
        *cdTestStat*(.text*);
        *chlStat*(.text*);
        *dcxoStat*(.text*);
        *navStat*(.text*);
        *pntStat*(.text*);
        *psm*(.text*);
        *svStat*(.text*);
        *UsrPvt*(.text*);
        *Acquire*(.text*);
        *Bch*(.text*);
        *ChannelGps*(.text*);
        *chlFilter*(.text*);
        *ConstDef*(.text*);
        *ExtF32*(.text*);
        *flashProc*(.text*);
        *Interrup*(.text*);
        *KalmanFilter*(.text*);
        *Matrix*(.text*);
        *navFilter*(.text*);
        *NavGps*(.text*);
        *NavLib*(.text*);
        *NavProc*(.text*);
        *NavQueue*(.text*);
        *NavTime*(.text*);
        *NbLdpc*(.text*);
        *Nmea0183*(.text*);
        *parseAtCmd*(.text*);
        *Rtc*(.text*);
        *SatProc*(.text*);
        *scene*(.text*);
        *StartMode*(.text*);
        *svFilter*(.text*);
        *timeAssist*(.text*);
        *TraceBySpi*(.text*);
        *usrDynMdl*(.text*);
        *postSolChk*(.text*);
        *priorSolChk*(.text*);
        *solOut*(.text*);
        *cce_firmware_gps*(.text*);
        *agnss*(.text*);
        *navAssist*(.text*);
        *chlFilter*(.text*);
        *gps_nav_filter*(.text*);
        . = LENGTH(gpsram)  - 4 ;
        LONG(0x00000000)
    } > gpsram

    .text :
    {
        . = ALIGN(512);
        _stext = .;
        *src_main.c.o(.text)
         *(.text)
        _etext  =  .;
        __CTOR_LIST__ = .;
        LONG((__CTOR_END__ - __CTOR_LIST__) / 4 - 2)
        *(.ctors)
        LONG(0)
        __CTOR_END__ = .;
        __DTOR_LIST__ = .;
        LONG((__DTOR_END__ - __DTOR_LIST__) / 4 - 2)
        *(.dtors)
        LONG(0)
        __DTOR_END__ = .;
        *(.lit)
        *(.shdata)
        _endtext = .;
    }  > instrram

    /*--------------------------------------------------------------------*/
    /* Global constructor/destructor segement                             */
    /*--------------------------------------------------------------------*/

    /*
    .preinit_array :
    {
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP (*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);
    } > datarom
    */

    /*
    .init_array :
    {
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array ))
        PROVIDE_HIDDEN (__init_array_end = .);
    } > datarom
    */

    /*
    .fini_array :
    {
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(SORT(.fini_array.*)))
        KEEP (*(.fini_array ))
        PROVIDE_HIDDEN (__fini_array_end = .);
    } > datarom
    */

    .rodata :
    {
        . = ALIGN(4);
        KEEP (*(.dsp_data));
        *(.rodata);
        *(.rodata.*);
        *(.srodata.*);
        *(.srodata*);


        /* section information for finsh shell */
        . = ALIGN(4);
        __fsymtab_start = .;
        KEEP(*(SORT(FSymTab)))
        __fsymtab_end = .;
        . = ALIGN(4);
        __vsymtab_start = .;
        KEEP(*(SORT(VSymTab)))
        __vsymtab_end = .;
        . = ALIGN(4);

        . = ALIGN(4);
        __rt_init_start = .;
        KEEP(*(SORT(.rti_fn*)))
        __rt_init_end = .;
        . = ALIGN(4);

        /* section information for modules */
        . = ALIGN(4);
        __rtmsymtab_start = .;
        KEEP(*(SORT(RTMSymTab)))
        __rtmsymtab_end = .;

   } > datarom

    .at_array     :
    {
        /* section information for at server */
        . = ALIGN(4);
        __rtatcmdtab_start = .;
        KEEP(*(RtAtCmdTab))
        __rtatcmdtab_end = .;
        . = ALIGN(4);
    } > datarom

    .eh_frame :
    {
        *(.eh_frame);
    } > datarom

   .staticinfo : {
        . = ALIGN(4);
        *(.devinfo);
        *(.devcontrolinfo);
        *(.dfeinfo);
        *(.rfinfo);
        *(.dumpdata);
    } > staticdata

    .shbss :
    {
        . = ALIGN(4);
        *(.shbss)
    } > dataram

    .data :
    {
        . = ALIGN(4);
        sdata  =  .;
        _sdata  =  .;
        *(.data);
        *(.data.*);
        *(.sdata);
        *(.sdata*);
        edata  =  .;
        _edata  =  .;
    } > dataram

    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        _bss_end = .;
    } > dataram

    .heap :
    {
        . = ALIGN(4);
    } > dataram
    .guard :
    {
        . = ALIGN(4);
        . = . + 16;
    } > dataram

    /* ensure there is enough room for stack */
    .stack (NOLOAD):
    {
        . = ALIGN(4);
        . = . + _min_stack ;
        . = ALIGN(4);
        stack = . ;
        _stack = . ;
    } > stack

    .stab  0 (NOLOAD) :
    {
        [ .stab ]
    }

    .stabstr  0 (NOLOAD) :
    {
        [ .stabstr ]
    }

    .cce_trace_buff :
    {
        . = ALIGN(4);
        *(.cce_trace_buff)
    } > ccetraceram

    .rv_trace_buff :
    {
        . = ALIGN(4);
        *(.rv_trace_buff)
    } > rvtraceram

    .bss :
    {
        . = ALIGN(4);
        _end = .;
    } > dataram

    .ota_flag :
    {
        LONG(0x46024355);
        LONG(LENGTH(uc_boot));
        LONG(LENGTH(uc_rom));
        LONG(LENGTH(psram));
        LONG(LENGTH(gpsram));
        LONG(LENGTH(instrram));
        LONG(LENGTH(datarom));
    } > ota_flag

}

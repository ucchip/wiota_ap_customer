#ifndef     _UC_COMPILE_
#define     _UC_COMPILE_

//macro for function prefetch

#define     SET_FUNC_PREFETCH_POSITION(funName) \
            void prefetch_##funName(){}
#define     DECLARE_FUNC_PREFETCH_POSITION(funName) \
            extern void prefetch_##funName();
#define     prefetch_instruction(funcName) \
            prefetch_##funName()
            

//macro for section alignment.

#define    UC_LINK_SECTION(s)                       __attribute__ ((section (s)))
#define    SET_TRANS_SECTION(sectionId,secSeq)   UC_LINK_SECTION("l1stransfer.section." #sectionId ".seq." #secSeq)

#endif
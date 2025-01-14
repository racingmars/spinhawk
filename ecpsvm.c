/*ECPSVM.C      (c) Copyright Ivan Warren, 2003-2009                 */
/*              Hercules ECPS:VM Support                             */

/***********************************************************/
/*                                                         */
/* General guidelines about E6XX instruction class         */
/* this is an implementation of ECPS:VM Level 20           */
/*                                                         */
/* General rule is : Only do what is safe to do. In doubt, */
/*                   give control to CP back (and act as   */
/*                   a NO-OP). All instructions have this  */
/*                   behaviour, therefore allowing only    */
/*                   partial implementation, or bypassing  */
/*                   tricky cases                          */
/*                                                         */
/* NOTE : ECPS:VM is only available for S/370 architecture */
/*                                                         */
/* In order for CP ASSIST to be active, a CONFIGURATION    */
/* statement is added : ECPS:VM lvl|no                     */
/* lvl is the ASSIST level (20 is recommended)             */
/* no means CP ASSIST is disabled (default)                */
/*                                                         */
/* Currently supported CP ASSIST instructions :            */
/* +-----+-------+----------------------------------------+*/
/* |opc  | Mnemo | Function                               |*/
/* +-----+-------+----------------------------------------+*/
/* |E602 | LCKPG | Lock Page in core table                |*/
/* |E603 | ULKPG | Unlock page in core table              |*/
/* |E606 | SCNVU | Scan Virtual Unit control blocks       |*/
/* |E607 | DISP1 | Dispatcher assist                      |*/
/* |E608 | TRBRG | LRA + Basic checks on VPAGE            |*/
/* |E609 | TRLOK | Same as TRBRG + Lock page in core      |*/
/* |E60D | DISP0 | Dispatcher assist                      |*/
/* |E60E | SCNRU | Scan Real Unit control blocks          |*/
/* |E611 | DISP2 | Dispatcher assist                      |*/
/* |E612 | STLVL | Store ECPS:VM Level                    |*/
/* |E614 | FREEX | Allocate CP FREE Storage from subpool  |*/
/* |E615 | FRETX | Release CP FREE Storage to subpool     |*/
/* +-----+-------+----------------------------------------+*/
/*                                                         */
/* Currently supported VM ASSIST instructions :            */
/* +-----+-------+----------------------------------------+*/
/* |opc  | Mnemo | Function                               |*/
/* +-----+-------+----------------------------------------+*/
/* |0A   | SVC   | Virtual SVC Assist                     |*/
/* |80   | SSM   | Virtual SSM Assist                     |*/
/* |82   | LPSW  | Virtual LPSW Assist                    |*/
/* |B7   | LCTL  | Virtual LCTL Assist                    |*/
/* +-----+-------+----------------------------------------+*/
/*                                                         */
/***********************************************************/

// $Log$
//
// Revision 1.83  2017/05/25 19:12:00  bobpolmanter/petercoghlan
// Fix DISP2 incorrect check of VMV370R and mis-loaded control registers;
// Remove DISP2 debug message causing page faults in CP.
//
// Revision 1.82  2017/04/30 13:54:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.81  2017/03/27 14:10:00  bobpolmanter
// Fix two minor issues in DISP0 that did not match DMKDSPCH.
// Fix DISP0 to set VMPSWAIT on in exit #28.
//
// Revision 1.80  2017/02/18 14:05:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.79  2017/02/10 07:25:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.78  2017/02/05 08:15:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.77  2017/02/04 15:45:00  bobpolmanter
// DISP2 dispatching user that is in virtual wait state;
//  add check for this condition and let CP handle it.
//
// Revision 1.76  2017/01/29 09:55:00  bobpolmanter
// DISP2 assist not completing for DAT-on guests due to incorrect
//  checking of shadow table and invalidate page table flags.
//
// Revision 1.75  2017/01/28 15:15:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.74  2017/01/27 15:20:00  bobpolmanter
// Fix the reversed order of the EVM_ST operands in the CPEXBLOK FRET exit 
//  of assist DISP2; was causing CP storage overlays and PRG001 failures.
//
// Revision 1.73  2017/01/27 15:20:00  bobpolmanter
// Enhancement revision; not implemented
//
// Revision 1.72  2017/01/24 12:53:00  bobpolmanter
// Instruction assists must go back to CP if virtual PSW in problem state
//
// Revision 1.71  2017/01/18 19:33:00  bobpolmanter
// Offset in PSA for APSTAT2 is incorrect in ecpsvm.h
//
// Revision 1.70  2017/01/15 12:00:00  bobpolmanter
// Virtual interval timer issue fixed in clock.c
//
// Revision 1.69  2017/01/12 12:00:00  bobpolmanter
// LCTL assist should not load DAS control regs 3-7;
// Update comments at beginning to reflect what is and is not supported
//
// Revision 1.68  2007/06/23 00:04:09  ivan
// Update copyright notices to include current year (2007)
//
// Revision 1.67  2007/01/13 07:18:14  bernard
// backout ccmask
//
// Revision 1.66  2007/01/12 15:22:37  bernard
// ccmask phase 1
//
// Revision 1.65  2006/12/31 17:53:48  gsmith
// 2006 Dec 31 Update ecpsvm.c for new psw IA scheme
//
// Revision 1.64  2006/12/08 09:43:20  jj
// Add CVS message log
//

#include "hstdinc.h"

#if !defined(_HENGINE_DLL_)
#define _HENGINE_DLL_
#endif

#if !defined(_ECPSVM_C_)
#define _ECPSVM_C_
#endif

#include "hercules.h"

#include "opcode.h"

#include "inline.h"

#include "ecpsvm.h"

#ifdef FEATURE_ECPSVM

ECPSVM_CMDENT *ecpsvm_getcmdent(char *cmd);

struct _ECPSVM_CPSTATS
{
    ECPSVM_STAT_DCL(SVC);
    ECPSVM_STAT_DCL(SSM);
    ECPSVM_STAT_DCL(LPSW);
    ECPSVM_STAT_DCL(STNSM);
    ECPSVM_STAT_DCL(STOSM);
    ECPSVM_STAT_DCL(SIO);
    ECPSVM_STAT_DCL(VTIMER);
    ECPSVM_STAT_DCL(STCTL);
    ECPSVM_STAT_DCL(LCTL);
    ECPSVM_STAT_DCL(DIAG);
    ECPSVM_STAT_DCL(IUCV);
} ecpsvm_sastats={
    ECPSVM_STAT_DEF(SVC),
    ECPSVM_STAT_DEF(SSM),
    ECPSVM_STAT_DEF(LPSW),
    ECPSVM_STAT_DEFU(STNSM),
    ECPSVM_STAT_DEFU(STOSM),
    ECPSVM_STAT_DEFU(SIO),
    ECPSVM_STAT_DEF(VTIMER),
    ECPSVM_STAT_DEFU(STCTL),
    ECPSVM_STAT_DEF(LCTL),
    ECPSVM_STAT_DEFU(DIAG),
    ECPSVM_STAT_DEFU(IUCV),
};
struct _ECPSVM_SASTATS
{
    ECPSVM_STAT_DCL(FREE);
    ECPSVM_STAT_DCL(FRET);
    ECPSVM_STAT_DCL(LCKPG);
    ECPSVM_STAT_DCL(ULKPG);
    ECPSVM_STAT_DCL(SCNRU);
    ECPSVM_STAT_DCL(SCNVU);
    ECPSVM_STAT_DCL(DISP0);
    ECPSVM_STAT_DCL(DISP1);
    ECPSVM_STAT_DCL(DISP2);
    ECPSVM_STAT_DCL(DNCCW);
    ECPSVM_STAT_DCL(DFCCW);
    ECPSVM_STAT_DCL(FCCWS);
    ECPSVM_STAT_DCL(CCWGN);
    ECPSVM_STAT_DCL(UXCCW);
    ECPSVM_STAT_DCL(TRBRG);
    ECPSVM_STAT_DCL(TRLOK);
    ECPSVM_STAT_DCL(VIST);
    ECPSVM_STAT_DCL(VIPT);
    ECPSVM_STAT_DCL(STEVL);
    ECPSVM_STAT_DCL(FREEX);
    ECPSVM_STAT_DCL(FRETX);
    ECPSVM_STAT_DCL(PMASS);
    ECPSVM_STAT_DCL(LCSPG);
} ecpsvm_cpstats={
    ECPSVM_STAT_DEFU(FREE),
    ECPSVM_STAT_DEFU(FRET),
    ECPSVM_STAT_DEF(LCKPG),
    ECPSVM_STAT_DEF(ULKPG),
    ECPSVM_STAT_DEF(SCNRU),
    ECPSVM_STAT_DEF(SCNVU),
    ECPSVM_STAT_DEF(DISP0),
    ECPSVM_STAT_DEF(DISP1),
    ECPSVM_STAT_DEF(DISP2),
    ECPSVM_STAT_DEFU(DNCCW),
    ECPSVM_STAT_DEFU(DFCCW),
    ECPSVM_STAT_DEFU(FCCWS),
    ECPSVM_STAT_DEFU(CCWGN),
    ECPSVM_STAT_DEFU(UXCCW),
    ECPSVM_STAT_DEF(TRBRG),
    ECPSVM_STAT_DEF(TRLOK),
    ECPSVM_STAT_DEFU(VIST),
    ECPSVM_STAT_DEFU(VIPT),
    ECPSVM_STAT_DEF(STEVL),
    ECPSVM_STAT_DEF(FREEX),
    ECPSVM_STAT_DEF(FRETX),
    ECPSVM_STAT_DEFU(PMASS),
    ECPSVM_STAT_DEFU(LCSPG),
};

#define DEBUG_CPASSIST
#define DEBUG_SASSIST

#define DODEBUG_ASSIST(_cond,x)  \
{ \
    if((_cond)) \
    { \
        x; \
    }\
}

#if defined(DEBUG_SASSIST)
#define DEBUG_SASSISTX(_inst,x) \
{ \
    DODEBUG_ASSIST(ecpsvm_sastats._inst.debug,x) \
}
#else
#define DEBUG_SASSISTX(_cond,x)
#endif
#if defined(DEBUG_CPASSIST)
#define DEBUG_CPASSISTX(_inst,x) \
{ \
    DODEBUG_ASSIST(ecpsvm_cpstats._inst.debug,x) \
}
#else
#define DEBUG_CPASSISTX(_cond,x)
#endif

/* Utility macros because I am very lazy */
#define EVM_IC( x )  ARCH_DEP(vfetchb) ( ( ( x ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_LH( x )  ARCH_DEP(vfetch2) ( ( ( x ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_L( x )  ARCH_DEP(vfetch4) ( ( ( x ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_LD( x )  ARCH_DEP(vfetch8) ( ( ( x ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_STD( x , y ) ARCH_DEP(vstore8) ( ( x ) , ( ( y ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_ST( x , y ) ARCH_DEP(vstore4) ( ( x ) , ( ( y ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_STH( x , y ) ARCH_DEP(vstore2) ( ( x ) , ( ( y ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_STC( x , y ) ARCH_DEP(vstoreb) ( ( x ) , ( ( y ) & ADDRESS_MAXWRAP(regs) ) , USE_REAL_ADDR , regs )
#define EVM_MVC( x , y , z ) ARCH_DEP(vfetchc) ( ( x ) , ( z ) , ( y ) , USE_REAL_ADDR , regs )

#define BR14 UPD_PSW_IA(regs, regs->GR_L(14))

#define INITPSEUDOIP(_regs) \
    do {    \
        (_regs).ip=(BYTE*)"\0\0";  \
    } while(0)

#define INITPSEUDOREGS(_regs) \
    do { \
        memset(&(_regs),0,sysblk.regs_copy_len); \
        INITPSEUDOIP((_regs)); \
    } while(0)


#define CPASSIST_HIT(_stat) ecpsvm_cpstats._stat.hit++

#define SASSIST_HIT(_stat) ecpsvm_sastats._stat.hit++

#define SASSIST_LPSW(_regs) \
    do { \
        SET_PSW_IA(&(_regs)); \
        UPD_PSW_IA(regs, _regs.psw.IA); \
        regs->psw.cc=_regs.psw.cc; \
        regs->psw.pkey=_regs.psw.pkey; \
        regs->psw.progmask=_regs.psw.progmask; \
    } \
    while(0)


#define SASSIST_PROLOG( _instname) \
    VADR amicblok; \
    VADR vpswa; \
    BYTE *vpswa_p; \
    REGS vpregs; \
    BYTE  micpend; \
    U32   CR6; \
    ECPSVM_MICBLOK micblok; \
    BYTE micevma; \
    BYTE micevma2; \
    BYTE micevma3; \
    BYTE micevma4; \
    if(SIE_STATE(regs)) \
        return(1); \
    if(!PROBSTATE(&regs->psw)) \
    { \
          return(1); \
    } \
    if(!sysblk.ecpsvm.available) \
    { \
          DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" ECPS:VM Disabled in configuration\n"))); \
          return(1); \
    } \
    if(!ecpsvm_sastats._instname.enabled) \
    { \
          DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" ECPS:VM Disabled by command\n"))); \
          return(1); \
    } \
    CR6=regs->CR_L(6); \
    regs->ecps_vtmrpt = NULL; /* Assume vtimer off until validated */ \
    if(!(CR6 & ECPSVM_CR6_VMASSIST)) \
    { \
        DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : EVMA Disabled by guest\n"))); \
        return(1); \
    } \
    /* 2017-01-24  Reject if Virtual PSW is in problem state */ \
    /* All instruction assists should be rejected if VPSW is in problem state and be reflected back    */  \
    /* to CP for handling.  This affects 2nd level VM hosting 3rd level guests.                        */  \
    if(CR6 & ECPSVM_CR6_VIRTPROB) \
    { \
        DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" reject : Virtual problem state\n"))); \
        return(1); \
    } \
    /* End of 2017-01-24   */  \
    /* Increment call now (don't count early misses) */ \
    ecpsvm_sastats._instname.call++; \
    amicblok=CR6 & ECPSVM_CR6_MICBLOK; \
    /* Ensure MICBLOK resides on a single 2K page */ \
    /* Then set ref bit by calling LOG_TO_ABS */ \
    if((amicblok & 0x007ff) > 0x7e0) \
    { \
        DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" Micblok @ %6.6X crosses page frame\n"),amicblok)); \
        return(1); \
    } \
    /* Load the micblok copy */ \
    micblok.MICRSEG=EVM_L(amicblok); \
    micblok.MICCREG=EVM_L(amicblok+4); \
    micblok.MICVPSW=EVM_L(amicblok+8); \
    micblok.MICWORK=EVM_L(amicblok+12); \
    micblok.MICVTMR=EVM_L(amicblok+16); \
    micblok.MICACF=EVM_L(amicblok+20); \
    micpend=(micblok.MICVPSW >> 24); \
    vpswa=micblok.MICVPSW & ADDRESS_MAXWRAP(regs); \
    micevma=(micblok.MICACF >> 24); \
    micevma2=((micblok.MICACF & 0x00ff0000) >> 16); \
    micevma3=((micblok.MICACF & 0x0000ff00) >> 8); \
    micevma4=(micblok.MICACF  & 0x000000ff); \
    if((CR6 & ECPSVM_CR6_VIRTTIMR)) \
    { \
        regs->ecps_vtmrpt = MADDR(micblok.MICVTMR,USE_REAL_ADDR,regs,ACCTYPE_READ,0); \
    } \
    /* Set ref bit on page where Virtual PSW is stored */ \
    vpswa_p=MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_READ,0); \
    DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" VPSWA= %8.8X Virtual "),vpswa)); \
    DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" CR6= %8.8X\n"),CR6)); \
    DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" MICVTMR= %8.8X\n"),micblok.MICVTMR)); \
    DEBUG_SASSISTX(_instname,logmsg(_("HHCEV300D : SASSIST "#_instname" Real "))); \
    DEBUG_SASSISTX(_instname,display_psw(regs)); \
    /* Load the Virtual PSW in a temporary REGS structure */ \
    INITPSEUDOREGS(vpregs); \
    ARCH_DEP(load_psw) (&vpregs,vpswa_p); \
    DEBUG_SASSISTX(_instname,display_psw(&vpregs)); \



#define ECPSVM_PROLOG(_inst) \
int     b1, b2; \
VADR    effective_addr1, \
        effective_addr2; \
     SSE(inst, regs, b1, effective_addr1, b2, effective_addr2); \
     PRIV_CHECK(regs); \
     SIE_INTERCEPT(regs); \
     if(!sysblk.ecpsvm.available) \
     { \
          DEBUG_CPASSISTX(_inst,logmsg(_("HHCEV300D : CPASSTS "#_inst" ECPS:VM Disabled in configuration "))); \
          ARCH_DEP(program_interrupt) (regs, PGM_OPERATION_EXCEPTION); \
     } \
     PRIV_CHECK(regs); /* No problem state please */ \
     if(!ecpsvm_cpstats._inst.enabled) \
     { \
          DEBUG_CPASSISTX(_inst,logmsg(_("HHCEV300D : CPASSTS "#_inst" Disabled by command"))); \
          return; \
     } \
     if(!(regs->CR_L(6) & 0x02000000)) \
     { \
        return; \
     } \
     ecpsvm_cpstats._inst.call++; \
    DEBUG_CPASSISTX(_inst,logmsg(_("HHCEV300D : "#_inst" called\n")));


/* DISPx Utility macros */

#define STPT(_x) \
{ \
    EVM_STD(cpu_timer(regs),_x); \
}

#define SPT(_x) \
{ \
    set_cpu_timer(regs,EVM_LD(_x)); \
    OBTAIN_INTLOCK(regs); \
    if(CPU_TIMER(regs) < 0) \
    { \
        ON_IC_PTIMER(regs); \
    } \
    else \
    { \
        OFF_IC_PTIMER(regs); \
    } \
    RELEASE_INTLOCK(regs); \
}


#define CHARGE_STOP(_x) \
{ \
        STPT(_x+VMTTIME); \
}

#define CHARGE_START(_x) \
{ \
    SPT(_x+VMTTIME); \
}

#define CHARGE_SWITCH(_x,_y) \
{ \
    CHARGE_STOP(_x); \
    CHARGE_START(_y); \
    (_x)=(_y); \
}


int ecpsvm_do_fretx(REGS *regs,VADR block,U16 numdw,VADR maxsztbl,VADR fretl);

/* CPASSIST FREE (Basic) Not supported */
/* This is part of ECPS:VM Level 18 and 19 */
/* ECPS:VM Level 20 use FREEX */
DEF_INST(ecpsvm_basic_freex)
{
    ECPSVM_PROLOG(FREE);
}
/* CPASSIST FRET (Basic) Not supported */
/* This is part of ECPS:VM Level 18 and 19 */
/* ECPS:VM Level 20 use FRETX */
DEF_INST(ecpsvm_basic_fretx)
{
    ECPSVM_PROLOG(FRET);
}

/* Lockpage common code (LCKPG/TRLOK) */
static void ecpsvm_lockpage1(REGS *regs,RADR cortab,RADR pg)
{
    BYTE corcode;
    VADR corte;
    U32  lockcount;
    RADR cortbl;

    DEBUG_CPASSISTX(LCKPG,logmsg(_("HHCEV300D : LKPG coreptr = "F_RADR" Frame = "F_RADR"\n"),cortab,pg));
    cortbl=EVM_L(cortab);
    corte=cortbl+((pg & 0xfff000)>>8);
    DEBUG_CPASSISTX(LCKPG,logmsg(_("HHCEV300D : LKPG corete = %6.6X\n"),corte));
    corcode=EVM_IC(corte+8);
    if(corcode & 0x80)
    {
        lockcount=EVM_L(corte+4);
        lockcount++;
    }
    else
    {
        lockcount=1;
        corcode|=0x80;
        EVM_STC(corcode,corte+8);
    }
    EVM_ST(lockcount,corte+4);
    DEBUG_CPASSISTX(LCKPG,logmsg(_("HHCEV300D : LKPG Page locked. Count = %6.6X\n"),lockcount));
    return;
}
/* E602 LCKPG Instruction */
/* LCKPG D1(R1,B1),D2(R2,B2) */
/* 1st operand : PTR_PL -> Address of coretable */
/* 2nd Operand : Page address to be locked */
DEF_INST(ecpsvm_lock_page)
{
    VADR ptr_pl;
    VADR pg;

    ECPSVM_PROLOG(LCKPG);

    ptr_pl=effective_addr1;
    pg=effective_addr2;

    DEBUG_CPASSISTX(LCKPG,logmsg(_("HHCEV300D : LKPG PAGE=%6.6X, PTRPL=%6.6X\n"),pg,ptr_pl));

    ecpsvm_lockpage1(regs,ptr_pl,pg);
    regs->psw.cc=0;
    BR14;
    CPASSIST_HIT(LCKPG);
    return;
}
/* E603 ULKPG Instruction */
/* ULKPG D1(R1,B1),D2(R2,B2) */
/* 1st operand : PTR_PL -> +0 - Maxsize, +4 Coretable */
/* 2nd Operand : Page address to be unlocked */
DEF_INST(ecpsvm_unlock_page)
{
    VADR ptr_pl;
    VADR pg;
    VADR corsz;
    VADR cortbl;
    VADR corte;
    BYTE corcode;
    U32  lockcount;

    ECPSVM_PROLOG(ULKPG);

    ptr_pl=effective_addr1;
    pg=effective_addr2;

    DEBUG_CPASSISTX(ULKPG,logmsg(_("HHCEV300D : ULKPG PAGE=%6.6X, PTRPL=%6.6X\n"),pg,ptr_pl));

    corsz=EVM_L(ptr_pl);
    cortbl=EVM_L(ptr_pl+4);
    if((pg+4095)>corsz)
    {
        DEBUG_CPASSISTX(ULKPG,logmsg(_("HHCEV300D : ULKPG Page beyond core size of %6.6X\n"),corsz));
        return;
    }
    corte=cortbl+((pg & 0xfff000)>>8);
    corcode=EVM_IC(corte+8);
    if(corcode & 0x80)
    {
        lockcount=EVM_L(corte+4);
        lockcount--;
    }
    else
    {
        DEBUG_CPASSISTX(ULKPG,logmsg(_("HHCEV300D : ULKPG Attempting to unlock page that is not locked\n")));
        return;
    }
    if(lockcount==0)
    {
        corcode &= ~(0x80|0x02);
        EVM_STC(corcode,corte+8);
        DEBUG_CPASSISTX(ULKPG,logmsg(_("HHCEV300D : ULKPG now unlocked\n")));
    }
    else
    {
        DEBUG_CPASSISTX(ULKPG,logmsg(_("HHCEV300D : ULKPG Page still locked. Count = %6.6X\n"),lockcount));
    }
    EVM_ST(lockcount,corte+4);
    CPASSIST_HIT(ULKPG);
    BR14;
    return;
}
/* DNCCW : Not supported */
DEF_INST(ecpsvm_decode_next_ccw)
{
    ECPSVM_PROLOG(DNCCW);
}
/* FCCWS : Not supported */
DEF_INST(ecpsvm_free_ccwstor)
{
    ECPSVM_PROLOG(FCCWS);
}
/* SCNVU : Scan for Virtual Device blocks */
DEF_INST(ecpsvm_locate_vblock)
{
    U32  vdev;
    U32  vchix;
    U32  vcuix;
    U32  vdvix;
    VADR vchtbl;
    VADR vch;
    VADR vcu;
    VADR vdv;

    ECPSVM_PROLOG(SCNVU);
    vdev=regs->GR_L(1);
    vchtbl=effective_addr1;

    vchix=EVM_LH(vchtbl+((vdev & 0xf00)>>7));   /* Get Index */
    if(vchix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,logmsg(_("HHCEV300D SCNVU Virtual Device %4.4X has no VCHAN block\n"),vdev));
        return;
    }
    vch=EVM_L(effective_addr2)+vchix;

    vcuix=EVM_LH(vch+8+((vdev & 0xf0)>>3));
    if(vcuix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,logmsg(_("HHCEV300D SCNVU Virtual Device %4.4X has no VCU block\n"),vdev));
        return;
    }
    vcu=EVM_L(effective_addr2+4)+vcuix;

    vdvix=EVM_LH(vcu+8+((vdev & 0xf)<<1));
    if(vdvix & 0x8000)
    {
        DEBUG_CPASSISTX(SCNVU,logmsg(_("HHCEV300D SCNVU Virtual Device %4.4X has no VDEV block\n"),vdev));
        return;
    }
    vdv=EVM_L(effective_addr2+8)+vdvix;
    DEBUG_CPASSISTX(SCNVU,logmsg(_("HHCEV300D SCNVU %4.4X : VCH = %8.8X, VCU = %8.8X, VDEV = %8.8X\n"),
                vdev,
                vch,
                vcu,
                vdv));
    regs->GR_L(6)=vch;
    regs->GR_L(7)=vcu;
    regs->GR_L(8)=vdv;
    regs->psw.cc=0;
    CPASSIST_HIT(SCNVU);
    BR14;
    return;
}

/* DISP1 Core */
/* rc : 0 - Done */
/* rc : 1 - No-op */
/* rc : 2 - Invoke DISP2 */
int ecpsvm_do_disp1(REGS *regs,VADR dl,VADR el)
{
    VADR vmb;
    U32 F_VMFLGS;       /* Aggregate for quick test */
    U32 F_SCHMASK;      /* Flags to test */
    U32 F_SCHMON;       /* Flags allowed on for quick dispatch */
    VADR F_ASYSVM;      /* System VMBLOK */
    VADR SCHDL;         /* SCHDL Exit */

    BYTE B_VMOSTAT;
    BYTE B_VMQSTAT;
    BYTE B_VMRSTAT;

    vmb=regs->GR_L(11);
    DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Data list = %6.6X VM=%6.6X\n",dl,vmb));
    F_VMFLGS=EVM_L(vmb+VMRSTAT);
    F_SCHMASK=EVM_L(dl+64);
    F_SCHMON=EVM_L(dl+68);
    if((F_VMFLGS & F_SCHMASK) == F_SCHMON)
    {
        DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Quick Check complete\n"));
        return(2);
    }
    else
    {
        DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Quick Check failed : %8.8X != %8.8X\n",(F_VMFLGS & F_SCHMASK),F_SCHMON));
    }

    F_ASYSVM=EVM_L(ASYSVM);
    if(vmb==F_ASYSVM)
    {
        DEBUG_CPASSISTX(DISP1,logmsg("DISP1 VMB is SYSTEM VMBLOCK\n"));
        return(2);
    }
    SCHDL=EVM_L(el+4);
    B_VMOSTAT=EVM_IC(vmb+VMOSTAT);
    if(!(B_VMOSTAT & VMKILL))
    {
        DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Call SCHEDULE because VMKILL not set\n"));
        UPD_PSW_IA(regs, SCHDL);
        return(0);
    }
    B_VMQSTAT=EVM_IC(vmb+VMQSTAT);
    if(!(B_VMQSTAT & VMCFREAD))
    {
        if(B_VMOSTAT & VMCF)
        {
            DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Call SCHEDULE because VMKILL & VMCF & !VMCFREAD set\n"));
            UPD_PSW_IA(regs, SCHDL);
            return(0);
        }
    }
    /* At DSP - OFF */
    B_VMQSTAT &= ~VMCFREAD;
    B_VMOSTAT &= ~VMKILL;
    EVM_STC(B_VMQSTAT,vmb+VMQSTAT);
    EVM_STC(B_VMOSTAT,vmb+VMOSTAT);
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    if(B_VMRSTAT & VMLOGOFF)
    {
        DEBUG_CPASSISTX(DISP1,logmsg("DISP1 Continue because already logging off\n"));
        return(2);
    }
    B_VMRSTAT |= VMLOGOFF;
    EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
    UPD_PSW_IA(regs, EVM_L(el+0));
    DEBUG_CPASSISTX(DISP1,logmsg("DISP1 : Call USOFF\n"));
    return(0);
}

/* DISP2 Core */
int ecpsvm_do_disp2(REGS *regs,VADR dl,VADR el)
{
    VADR vmb;   /* Current VMBLOK */
    VADR svmb;  /* ASYSVM */
    VADR runu;  /* RUNUSER */
    VADR lastu; /* LASTUSER */
    VADR F_TRQB;
    VADR F_CPEXB;
    VADR F,B;
    U16  HW1;
    U32  FW1;
    U64  DW1;
    U32  CPEXBKUP[15];  /* CPEXBLOK Regs backup except GPR15 which is useless */
    VADR F_ECBLOK;      /* Pointer to user's EC block for extended VM */
    VADR F_CPEXADD;
    U32  F_QUANTUM;
    REGS wregs; /* Work REGS structure of PSW manipulation for Virtual PSW */
    REGS rregs; /* Work REGS structure of PSW manipulation for Real    PSW */
    int i;

    BYTE B_VMDSTAT,B_VMRSTAT,B_VMESTAT,B_VMPSTAT,B_VMMCR6,B_MICVIP;
    BYTE B_VMOSTAT,B_VMPEND;
    VADR F_MICBLOK;
    U32 F_VMIOINT,F_VMPXINT;
    U32 F_VMVCR0;
    U32 NCR0,NCR1;
    BYTE *work_p;

    vmb=regs->GR_L(11);
    DEBUG_CPASSISTX(DISP2,logmsg("DISP2 Data list=%6.6X VM=%6.6X\n",dl,vmb));
    CHARGE_STOP(vmb);
    if(EVM_IC(XTENDLOCK) == XTENDLOCKSET)
    {
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 Exit 8 : System extending\n"));
        /* System in Extend process */
        UPD_PSW_IA(regs, EVM_L(el+8));
        return(0);
    }
    if(EVM_IC(APSTAT2) & CPMCHLK)
    {
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 Exit 8 : MCH Recovery\n"));
        /* Machine Check recovery in progress */
        UPD_PSW_IA(regs, EVM_L(el+8));
        return(0);
    }
    svmb=EVM_L(ASYSVM);
    /* Check IOB/TRQ for dispatch */
    F_TRQB=EVM_L(dl+8);
    if(F_TRQB!=dl)
    {
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 TRQ/IOB @ %6.6X Exit being routed\n",F_TRQB));
        /* We have a TRQ/IOB */
        /* Update stack */
        F=EVM_L(F_TRQB+8);
        B=EVM_L(F_TRQB+12);
        EVM_ST(F,B+8);
        EVM_ST(B,F+12);
        /* Get VMBLOK Responsible for this block */
        vmb=EVM_L(F_TRQB+0x18);
        /* Update stack count for the VMBLOK */
        HW1=EVM_LH(vmb+VMSTKCNT);
        HW1--;
        EVM_STH(HW1,vmb+VMSTKCNT);
        /* Start charging user for processor time */
        CHARGE_START(vmb);
        EVM_ST(vmb,STACKVM);
        /* Update registers for TRQ/IOB exit */
        regs->GR_L(10)=F_TRQB;
        regs->GR_L(11)=vmb;
        regs->GR_L(12)=EVM_L(F_TRQB+0x1C);
        UPD_PSW_IA(regs, regs->GR_L(12));
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 TRQ/IOB @ %6.6X IA = %6.6X\n",F_TRQB,regs->GR_L(12)));
        return(0);
    }
    /* Check CPEX BLOCK for dispatch */
    F_CPEXB=EVM_L(dl+0);
    if(F_CPEXB!=dl)
    {
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 CPEXBLOK Exit being routed CPEX=%6.6X\n",F_CPEXB));
        /* We have a CPEXBLOCK */
        /* Update stack */
        F=EVM_L(F_CPEXB+0);
        B=EVM_L(F_CPEXB+4);
        EVM_ST(F,B+0);
        EVM_ST(B,F+4);
        vmb=EVM_L(F_CPEXB+0x10+(11*4));
        HW1=EVM_LH(vmb+VMSTKCNT);
        HW1--;
        EVM_STH(HW1,vmb+VMSTKCNT);
        CHARGE_START(vmb);
        /* Copy CPEXBLOCK Contents, and attempt FRET */
        /* If fret fails, use exit #12 */
        for(i=0;i<15;i++)
        {
            CPEXBKUP[i]=EVM_L(F_CPEXB+0x10+(i*4));
        }
        F_CPEXADD=EVM_L(F_CPEXB+0x0C);
        if(ecpsvm_do_fretx(regs,F_CPEXB,10,EVM_L(dl+28),EVM_L(dl+32))!=0)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 CPEXBLOK CPEX=%6.6X Fret Failed\n",F_CPEXB));
            regs->GR_L(0)=10;
            regs->GR_L(1)=F_CPEXB;
            for(i=2;i<12;i++)
            {
                regs->GR_L(i)=CPEXBKUP[i];
            }
        /* Save GPRS 12-1 (wraping) in DSPSAVE (datalist +40) */
        /* So that LM 12,1,DSPSAVE in DMKDSP works after call to DMKFRET */
        /* 2017-01-27 Fix order of EVM_ST operands to prevent stg overlays */
            EVM_ST(CPEXBKUP[12],dl+40);
            EVM_ST(CPEXBKUP[13],dl+44);
            EVM_ST(CPEXBKUP[14],dl+48);
            EVM_ST(EVM_L(F_CPEXB+12),dl+52); /* DSPSAVE + 12 = CPEXADD */
            EVM_ST(CPEXBKUP[0],dl+56);
            EVM_ST(CPEXBKUP[1],dl+60);  /* Note : DMKDSP Is wrong -  SCHMASK is at +64 (not +60) */
        /* End of 2017-01-27 */
        /* Upon taking this exit, GPRS 12-15 are same as entry */
            UPD_PSW_IA(regs, EVM_L(el+12));
            return(0);
        }
        for(i=0;i<15;i++)
        {
            regs->GR_L(i)=CPEXBKUP[i];
        }
        regs->GR_L(15)=F_CPEXADD;
        UPD_PSW_IA(regs, F_CPEXADD);
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 CPEXBLOK CPEX=%6.6X IA=%6.6X\n",F_CPEXB,F_CPEXADD));
        return(0);  /* CPEXBLOCK Branch taken */
    }
    /* Check for a USER run */
    /* AT DMKDSP - DONE */
    if(EVM_IC(CPSTAT2) & CPSHRLK)
    {
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 Exit 24 : CPSHRLK Set in CPSTAT2\n"));
        UPD_PSW_IA(regs, EVM_L(el+24));  /* IDLEECPS */
        return(0);
    }
    /* Scan Scheduler IN-Q */
    DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Scanning Scheduler IN-Queue\n"));
    FW1=EVM_L(dl+24);
    for(vmb=EVM_L(FW1);vmb!=FW1;vmb=EVM_L(vmb))
    {
        /* 2017-02-04 Check for V PSW wait */
        if(EVM_LH(vmb+VMPSW) & 0x0002)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Not eligible : User in virtual PSW wait\n",vmb));
            continue;
        }
        /* end of 2017-02-04 */
        if(!(EVM_IC(vmb+VMDSTAT) & VMRUN))
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Not eligible : VMRUN not set\n",vmb));
            continue;
        }
        if(EVM_IC(vmb+VMRSTAT) & VMCPWAIT)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Not eligible : VMCPWAIT set\n",vmb));
            continue;
        }
        if(EVM_IC(vmb+VMNOECPS))
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Exit 20 : VMB @ %6.6X Has VMNOECPS Set to %2.2X\n",vmb,EVM_IC(vmb+VMNOECPS)));
            regs->GR_L(1)=vmb;
            regs->GR_L(11)=EVM_L(ASYSVM);
            UPD_PSW_IA(regs, EVM_L(el+20));  /* FREELOCK */
            return(0);
        }
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Will now be dispatched\n",vmb));
        runu=EVM_L(RUNUSER);
        F_QUANTUM=EVM_L(QUANTUM);
        if(vmb!=runu)
        {
            /* User switching */
            /* DMKDSP - FNDUSRD */
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : User switch from %6.6X to %6.6X\n",runu,vmb));
            runu=EVM_L(RUNUSER);
            EVM_STC(EVM_IC(runu+VMDSTAT) & ~VMDSP,runu+VMDSTAT);
            lastu=EVM_L(LASTUSER);
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : RUNU=%6.6X, LASTU=%6.6X\n",runu,lastu));
            if(lastu!=svmb && lastu!=vmb)
            {
                if(EVM_IC(lastu+VMOSTAT) & VMSHR)       /* Running shared sys */
                {
                    DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Exit 16 : LASTU=%6.6X has shared sys & LCSHPG not impl\n",lastu));
                    CHARGE_START(lastu);
                    /* LCSHRPG not implemented yet */
                    regs->GR_L(10)=vmb;
                    regs->GR_L(11)=lastu;
                    UPD_PSW_IA(regs, EVM_L(el+16));
                    return(0);
                    /* A CHARGE_STOP(runu) is due when LCSHRPG is implemented */
                }
            }
        }
        if(vmb!=runu || (vmb==runu && (F_QUANTUM & 0x80000000)))
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Restarting Time Slice\n"));
            F_QUANTUM=EVM_L(dl+16);
            if(EVM_IC(vmb+VMQLEVEL) & VMCOMP)
            {
                F_QUANTUM <<= 2;
            }
        }
        EVM_ST(F_QUANTUM,INTTIMER);
        CHARGE_START(vmb);
        EVM_ST(vmb,LASTUSER);
        EVM_ST(vmb,RUNUSER);


        /***  Prepare to run a user ***/

        /* Cache some important VMBLOK flag bytes */
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
        B_VMPSTAT=EVM_IC(vmb+VMPSTAT);
        B_VMESTAT=EVM_IC(vmb+VMESTAT);
        B_VMOSTAT=EVM_IC(vmb+VMOSTAT);
        B_VMPEND =EVM_IC(vmb+VMPEND);
        B_VMMCR6=EVM_IC(vmb+VMMCR6);
        F_MICBLOK=EVM_L(vmb+VMMCR6) & ADDRESS_MAXWRAP(regs);

        /* LOAD FPRS */
        regs->fpr[0] = EVM_L(vmb+VMFPRS+0);
        regs->fpr[1] = EVM_L(vmb+VMFPRS+4);
        regs->fpr[4] = EVM_L(vmb+VMFPRS+8);
        regs->fpr[5] = EVM_L(vmb+VMFPRS+12);
        regs->fpr[8] = EVM_L(vmb+VMFPRS+16);
        regs->fpr[9] = EVM_L(vmb+VMFPRS+20);
        regs->fpr[12] = EVM_L(vmb+VMFPRS+24);
        regs->fpr[13] = EVM_L(vmb+VMFPRS+28);

        INITPSEUDOREGS(wregs);
        work_p=MADDR(vmb+VMPSW,0,regs,USE_REAL_ADDR,0);
        ARCH_DEP(load_psw) (&wregs,work_p);    /* Load user's Virtual PSW in work structure */
        SET_PSW_IA(&wregs);

        /* Build REAL PSW */
        INITPSEUDOREGS(rregs);
        /* Copy IAR */
        UPD_PSW_IA(&rregs, wregs.psw.IA);
        /* Copy CC, PSW KEYs and PGM Mask */
        rregs.psw.cc=wregs.psw.cc;
        rregs.psw.pkey=wregs.psw.pkey;
        /* Indicate Translation + I/O + Ext + Ecmode + Problem + MC */
        rregs.psw.sysmask=0x07; /* I/O + EXT + Trans */
        rregs.psw.states = BIT(PSW_EC_BIT)         /* ECMODE */
                         | BIT(PSW_PROB_BIT)       /* Problem state */
                         | BIT(PSW_MACH_BIT);      /* MC Enabled */
        rregs.psw.intcode=0;    /* Clear intcode */
        rregs.psw.progmask=wregs.psw.progmask;

        NCR0=EVM_L(CPCREG0);    /* Assume for now */
        NCR1=EVM_L(vmb+VMSEG);  /* Ditto          */

        /* Disable ECPS:VM in VM-REAL CR6 For now */
        B_VMMCR6&=~(VMMSHADT|VMMPROB|VMMNOSK|VMMFE);

        /* We load VMECEXT Even if it's not a ECMODE VM */
        /* in which case F_ECBLOK is also Virtual CR0   */

        F_ECBLOK=EVM_L(vmb+VMECEXT);

        /* ECMODE VM ? */
        if(B_VMPSTAT & VMV370R)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X has ECMODE ON\n",vmb));
            /* Is this an ECMODE PSW Machine ? */
            if(B_VMESTAT & VMEXTCM)
            {
                if((B_VMESTAT & (VMINVSEG|VMNEWCR0)) == (VMINVSEG|VMNEWCR0))
                {
                    /* CP Say this is NOT good */
                    /* Take exit 28 */
                    logmsg(_("HHCEV004W : Abend condition detected in DISP2 instr\n"));
                    UPD_PSW_IA(regs, EVM_L(el+28));
                    return(0);
                }
                /* Check 3rd level translation */
                if(wregs.psw.sysmask & 0x04)
                {
                    NCR0=EVM_L(F_ECBLOK+EXTSHCR0);
                    NCR1=EVM_L(F_ECBLOK+EXTSHCR1);
                    B_VMMCR6|=VMMSHADT;   /* re-enable Shadow Table management in CR6 */
                }
            }

        }
        /* Invalidate Shadow Tables if necessary */
        /* 2017-01-29 if statement corrected */
        if((B_VMESTAT & (VMINVPAG | VMSHADT)) == (VMINVPAG|VMSHADT))
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Refusing to simulate DMKVATAB\n",vmb));
            /* Really looks like DMKVATAB is a huge thing to simulate */
            /* My belief is that the assist can't handle this one     */
            /* Return to caller as a NO-OP on this one                */
            return(1);
            /* ecpsvm_inv_shadtab_pages(regs,vmb); */
        }
        B_VMESTAT&=~VMINVPAG;
        B_VMDSTAT|=VMDSP;
        /* Test for CPMICON in DMKDSP useless here */
        /* if CPMICON was off, we would have never */
        /* been called anyway                      */
        if(F_MICBLOK!=0)        /* That is SET ASSIST ON */
        {
            B_MICVIP=0;
            /* Check tracing (incompatible with assist) */
            if(!(EVM_IC(vmb+VMTRCTL) & (VMTRSVC|VMTRPRV|VMTRBRIN)))
            {
                B_VMMCR6|=VMMFE;
                if(B_VMOSTAT & VMSHR)
                {
                    /* Cannot allow ISK/SSK in shared sys VM */
                    B_VMMCR6|=VMMNOSK;
                }
                if(PROBSTATE(&wregs.psw))
                {
                    B_VMMCR6|=VMMPROB;
                }
                /* Set MICPEND if necessary */
                /* (assist stuff to ensure LPSW/SSM/SVC sim */
                /* does not re-enable VPSW when an interrupt */
                /* is pending)                               */
                while(1)
                {
                    B_MICVIP=0;
                    F_VMIOINT=EVM_LH(vmb+VMIOINT);
                    if(EVM_LH(vmb+VMIOINT)!=0)
                        {
                        F_VMIOINT<<=16;
                        if(B_VMESTAT & VMEXTCM)
                        {
                            if(F_VMIOINT&=EVM_L(F_ECBLOK))
                            {
                                B_MICVIP|=0x80;
                                break;
                            }
                        }
                        else
                        {
                            B_MICVIP|=0x80;
                            break;
                        }
                    }
                    if(B_VMESTAT & VMEXTCM)
                    {
                        if(B_VMPEND & VMPGPND)
                        {
                            B_MICVIP|=0x80;
                        }
                    }
                    if(B_VMPSTAT & VMV370R)
                    {
                        F_VMVCR0=EVM_L(F_ECBLOK+0);
                    }
                    else
                    {
                        F_VMVCR0=F_ECBLOK;
                    }
                    for(F_VMPXINT=EVM_L(vmb+VMPXINT);F_VMPXINT;F_VMPXINT=EVM_L(F_VMPXINT)) /* XINTNEXT at +0 */
                    {
                        if(F_VMVCR0 & EVM_LH(F_VMPXINT+10))
                        {
                            B_MICVIP|=0x80;
                            break;
                        }
                    }
                    break;      /* Terminate dummy while loop */
                } /* While dummy loop for MICPEND */
            } /* if(Not tracing) */
            EVM_STC(B_MICVIP,F_MICBLOK+8);      /* Save new MICVIP */
        } /* if(F_MICBLOCK!=0) */
        /* If an Extended VM, Load CRs 4-13 */
        /* CR6 Will be overwritten in a second */
        if(B_VMPSTAT & VMV370R)
        {
            for(i=4;i<14;i++)
            {
                regs->CR_L(i)=EVM_L(F_ECBLOK+(i*4));
            }
        }
        /* Update VMMICRO */
        EVM_STC(B_VMMCR6,vmb+VMMCR6);
        /* Update PER Control */
        if(EVM_IC(vmb+VMTRCTL) & VMTRPER)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : PER ON\n"));
            FW1=EVM_L(vmb+VMTREXT);
            regs->CR_L( 9)=EVM_L(FW1+0x1C);
            regs->CR_L(10)=EVM_L(FW1+0x20);
            regs->CR_L(11)=EVM_L(FW1+0x24);
            rregs.psw.sysmask |= 0x40;  /* PER Mask in PSW */
        }
        /* Update CR6 */
        regs->CR_L(6)=EVM_L(vmb+VMMCR6);
        /* Insure proper re-entry */
        EVM_ST(0,STACKVM);
        /* Update PROBLEM Start time */
        DW1=EVM_LD(vmb+VMTMOUTQ);
        EVM_STD(DW1,PROBSTRT);

        /* Checkpoint Interval Timer */
        FW1=EVM_L(INTTIMER);
        EVM_ST(FW1,QUANTUM);

        /* Update REAL CR0/CR1 */
        regs->CR_L(0)=NCR0;
        regs->CR_L(1)=NCR1;

        /* Indicate RUNNING a user */
        EVM_STC(CPRUN,CPSTATUS);

        /* Update real PSW with working PSW */

        /* Update regs */
        for(i=0;i<16;i++)
        {
            regs->GR_L(i)=EVM_L(vmb+VMGPRS+(i*4));
        }
        /* Clear I/O Old PSW Byte 0 */
        EVM_STC(0,IOOPSW);
        /* Issue PTLB if necessary */
        if(EVM_IC(APSTAT2) & CPPTLBR)
        {
            DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Purging TLB\n"));
            ARCH_DEP(purge_tlb)(regs);
            EVM_STC(EVM_IC(APSTAT2) & ~CPPTLBR,APSTAT2);
        }

        /* Update cached VMBLOK flags */
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
        EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
        EVM_STC(B_VMESTAT,vmb+VMESTAT);
        EVM_STC(B_VMPSTAT,vmb+VMPSTAT);
        EVM_STC(B_VMOSTAT,vmb+VMOSTAT);
        work_p=MADDR(vmb+VMPSW,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0); \
        ARCH_DEP(store_psw) (&wregs,work_p);


        /* Stop charging current VM Block for Supervisor time */
        CHARGE_STOP(vmb);

        /* Rest goes for problem state */
        SPT(vmb+VMTMOUTQ);
        /* Save RUNCR0, RUNCR1 & RUNPSW */
        /* Might be used by later CP Modules (including DMKPRV) */
        EVM_ST(NCR0,RUNCR0);
        EVM_ST(NCR1,RUNCR1);
        work_p=MADDR(RUNPSW,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0); \
        ARCH_DEP(store_psw) (&rregs,work_p);
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Entry Real "));
        DEBUG_CPASSISTX(DISP2,display_psw(regs));
        ARCH_DEP(load_psw) (regs,work_p);
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : VMB @ %6.6X Now being dispatched\n",vmb));
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Real "));
        DEBUG_CPASSISTX(DISP2,display_psw(regs));
        DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Virtual "));
        DEBUG_CPASSISTX(DISP2,display_psw(&wregs));
        /* TEST */
        ARCH_DEP(purge_tlb)(regs);
        SET_IC_MASK(regs);
        SET_AEA_MODE(regs);
        SET_AEA_COMMON(regs);
        SET_PSW_IA(regs);
        /* Dispatch..... */
        DEBUG_CPASSISTX(DISP2,display_regs(regs));
        DEBUG_CPASSISTX(DISP2,display_cregs(regs));
        return(2);      /* OK - Perform INTCHECK */
    }
    /* Nothing else to do - wait state */
    DEBUG_CPASSISTX(DISP2,logmsg("DISP2 : Nothing to dispatch - IDLEECPS\n"));
    UPD_PSW_IA(regs, EVM_L(el+24));      /* IDLEECPS */
    return(0);
}

/* DISP1 : Early tests part 2 */
/*   DISP1 Checks if the user is OK to run */
/*         early tests part 1 already done by DISP0 */
DEF_INST(ecpsvm_disp1)
{

    ECPSVM_PROLOG(DISP1);
    switch(ecpsvm_do_disp1(regs,effective_addr1,effective_addr2))
    {
        case 0: /* Done */
            CPASSIST_HIT(DISP1);
            return;
        case 1: /* No-op */
            break;
        case 2: /* Call DISP2 - INTCHECK NOT needed */
            switch(ecpsvm_do_disp2(regs,effective_addr1,effective_addr2))
            {
                case 0:
                        CPASSIST_HIT(DISP1);
                        return;
                case 1:
                        return;
                case 2:
                        CPASSIST_HIT(DISP1);
                        RETURN_INTCHECK(regs);
                default:
                        break;
            }
            return;
        default:
            return;
    }
}
static int ecpsvm_int_lra(REGS *regs,VADR pgadd,RADR *raddr)
{
    int cc;
    cc = ARCH_DEP(translate_addr) (pgadd , USE_PRIMARY_SPACE, regs, ACCTYPE_LRA);
    *raddr = regs->dat.raddr;
    return cc;
}
/* TRANBRNG/TRANLOCK Common code */
static int ecpsvm_tranbrng(REGS *regs,VADR cortabad,VADR pgadd,RADR *raddr)
{
    int cc;
    int corcode;
#if defined(FEATURE_2K_STORAGE_KEYS)
    RADR pg1,pg2;
#endif
    VADR cortab;
    cc=ecpsvm_int_lra(regs,pgadd,raddr);
    if(cc!=0)
    {
        DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Tranbring : LRA cc = %d\n"),cc));
        return(1);
    }
    /* Get the core table entry from the Real address */
    cortab=EVM_L( cortabad );
    cortab+=((*raddr) & 0xfff000) >> 8;
    corcode=EVM_IC(cortab+8);
    if(!(corcode & 0x08))
    {
        DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Page not shared - OK %d\n"),cc));
        return(0);      /* Page is NOT shared.. All OK */
    }
#if defined(FEATURE_2K_STORAGE_KEYS)
    pg1=(*raddr & 0xfff000);
    pg2=pg1+0x800;
    DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Checking 2K Storage keys @"F_RADR" & "F_RADR"\n"),pg1,pg2));
    if((STORAGE_KEY(pg1,regs) & STORKEY_CHANGE) ||
            (STORAGE_KEY(pg2,regs) & STORKEY_CHANGE))
    {
#else
    DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Checking 4K Storage keys @"F_RADR"\n"),*raddr));
    if(STORAGE_KEY(*raddr,regs) & STORKEY_CHANGE)
    {
#endif
        DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Page shared and changed\n")));
        return(1);      /* Page shared AND changed */
    }
    DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : Page shared but not changed\n")));
    return(0);  /* All done */
}
/* TRBRG : Translate a page address */
/* TRBRG D1(R1,B1),D2(R2,B2) */
/* 1st operand : Coretable address */
/* 2nd operand : Virtual address */
/* Note : CR1 Contains the relevant segment table */
/*        pointers                                */
/* The REAL address is resolved. If the page is flagged */
/* as shared in the core table, the page is checked for */
/* the change bit                                       */
/* If no unusual condition is detected, control is returned */
/* to the address in GPR 14. Otherwise, TRBRG is a no-op */
DEF_INST(ecpsvm_tpage)
{
    int rc;
    RADR raddr;
    ECPSVM_PROLOG(TRBRG);
    DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : TRANBRNG\n")));
    rc=ecpsvm_tranbrng(regs,effective_addr1,regs->GR_L(1),&raddr);
    if(rc)
    {
        DEBUG_CPASSISTX(TRBRG,logmsg(_("HHCEV300D : TRANBRNG - Back to CP\n")));
        return; /* Something not right : NO OP */
    }
    regs->psw.cc=0;
    regs->GR_L(2)=raddr;
    UPD_PSW_IA(regs, effective_addr2);
    CPASSIST_HIT(TRBRG);
    return;
}
/* TRLOK : Translate a page address and lock */
/* TRLOK D1(R1,B1),D2(R2,B2) */
/* See TRBRG. */
/* If sucessfull, the page is also locked in the core table */
DEF_INST(ecpsvm_tpage_lock)
{
    int rc;
    RADR raddr;
    ECPSVM_PROLOG(TRLOK);
    DEBUG_CPASSISTX(TRLOK,logmsg(_("HHCEV300D : TRANLOCK\n")));
    rc=ecpsvm_tranbrng(regs,effective_addr1,regs->GR_L(1),&raddr);
    if(rc)
    {
        DEBUG_CPASSISTX(TRLOK,logmsg(_("HHCEV300D : TRANLOCK - Back to CP\n")));
        return; /* Something not right : NO OP */
    }
    /*
     * Lock the page in Core Table
     */
    ecpsvm_lockpage1(regs,effective_addr1,raddr);
    regs->psw.cc=0;
    regs->GR_L(2)=raddr;
    UPD_PSW_IA(regs, effective_addr2);
    CPASSIST_HIT(TRLOK);
    return;
}
/* VIST : Not supported */
DEF_INST(ecpsvm_inval_segtab)
{
    ECPSVM_PROLOG(VIST);
}
/* VIPT : Not supported */
DEF_INST(ecpsvm_inval_ptable)
{
    ECPSVM_PROLOG(VIPT);
}
/* DFCCW : Not Supported */
DEF_INST(ecpsvm_decode_first_ccw)
{
    ECPSVM_PROLOG(DFCCW);
}

/* DISP0 Utility functions */

/* DMKDSP - INCPROBT */

static int ecpsvm_disp_incprobt(REGS *regs,VADR vmb)
{
    U64 tspent;
    U64 DW_VMTMOUTQ;
    U64 DW_PROBSTRT;
    U64 DW_PROBTIME;

    DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT Entry : VMBLOK @ %8.8X\n",vmb));
    DW_VMTMOUTQ=EVM_LD(vmb+VMTMOUTQ);
    DW_PROBSTRT=EVM_LD(PROBSTRT);
    DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT Entry : VMTMOUTQ = %16.16" I64_FMT "x\n",DW_VMTMOUTQ));
    DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT Entry : PROBSTRT = %16.16" I64_FMT "x\n",DW_PROBSTRT));
    if(DW_VMTMOUTQ==DW_PROBSTRT)
    {
        DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT Already performed"));
        return(2);      /* continue */
    }
    tspent=DW_PROBSTRT-DW_VMTMOUTQ;
    DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT TSPENT = %16.16" I64_FMT "x\n",tspent));
    DW_PROBTIME=EVM_LD(PROBTIME);
    DW_PROBTIME-=tspent;
    EVM_STD(DW_PROBTIME,PROBTIME);
    DEBUG_CPASSISTX(DISP0,logmsg("INCPROBT NEW PROBTIME = %16.16" I64_FMT "x\n",DW_PROBTIME));
    return(2);
}

/* DMKDSP
RUNTIME
*/

static int ecpsvm_disp_runtime(REGS *regs,VADR *vmb_p,VADR dlist,VADR exitlist)
{
    U64 DW_VMTTIME;
    U64 DW_VMTMINQ;
    BYTE B_VMDSTAT;
    BYTE B_VMTLEVEL;
    BYTE B_VMMCR6;
    U32  F_QUANTUM;
    U32  F_QUANTUMR;
    U32  F_ITIMER;
    int cc;
    RADR raddr;
    VADR tmraddr;
    U32  oldtimer,newtimer;
    VADR vmb;
    VADR runu;

    vmb=*vmb_p;
    DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME Entry : VMBLOK @ %8.8X\n",vmb));
    runu=EVM_L(RUNUSER);
    /* BAL RUNTIME Processing */
    EVM_STC(CPEX+CPSUPER,CPSTATUS);
    CHARGE_STOP(vmb);
    if(vmb!=runu)
    {
        DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME Switching to RUNUSER VMBLOK @ %8.8X\n",runu));
        CHARGE_SWITCH(vmb,runu);    /* Charge RUNUSER */
        F_ITIMER=EVM_L(QUANTUMR);
        *vmb_p=vmb;
    }
    else
    {
        F_ITIMER=EVM_L(INTTIMER);
    }
    DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : VMBLOK @ %8.8X\n",vmb));
    /* vmb is now RUNUSER */
    /* Check if time slice is over */
    if(F_ITIMER & 0x80000000)
    {
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        B_VMDSTAT|=VMTSEND;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
    }
    /* Check if still eligible for current run Q */
    DW_VMTTIME=EVM_LD(vmb+VMTTIME);
    DW_VMTMINQ=EVM_LD(vmb+VMTMINQ);
    /* Check 1st 5 bytes */
    /*2017-03-27 added equality check*/
    if((DW_VMTTIME & 0xffffffffff000000ULL) <= (DW_VMTMINQ & 0xffffffffff000000ULL))
    {
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        B_VMDSTAT|=VMQSEND;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
    }
    ecpsvm_disp_incprobt(regs,vmb);
    F_QUANTUM=EVM_L(QUANTUM);
    EVM_ST(F_ITIMER,QUANTUM);
    /* Check if Virtual Timer assist is active */
    B_VMMCR6=EVM_IC(vmb+VMMCR6);
    if(B_VMMCR6 & 0x01)      /* Virtual Timer Flag */
    {
        DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : Complete - VTIMER Assist active\n"));
        return(2);      /* End of "RUNTIME" here */
    }
    /* Check SET TIMER ON or SET TIMER REAL */
    B_VMTLEVEL=EVM_IC(vmb+VMTLEVEL);
    if(!(B_VMTLEVEL & (VMTON | VMRON)))
    {
        DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : Complete - SET TIMER OFF\n"));
        return(2);
    }
    /* Update virtual interval timer */
    F_QUANTUMR=EVM_L(QUANTUMR);
    F_QUANTUM-=F_QUANTUMR;
    if(F_QUANTUM & 0x80000000)
    {
        /* Abend condition detected during virtual time update - exit at +32 */
        DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : Bad ITIMER - Taking Exist #32\n"));
        UPD_PSW_IA(regs, EVM_L(exitlist+32));
        return(0);
    }
    /* Load CR1 with the vmblock's VMSEG */
    regs->CR_L(1)=EVM_L(vmb+VMSEG);
    /* Do LRA - Don't access the page directly yet - Could yield a Paging fault */
    cc = ecpsvm_int_lra(regs,INTTIMER,&raddr);
    if(cc!=0)
    {
        /* Update VMTIMER instead */
        tmraddr=vmb+VMTIMER;
    }
    else
    {
        tmraddr=(VADR)raddr;
    }
    oldtimer=EVM_L(tmraddr);
    newtimer=oldtimer-F_QUANTUM;
    EVM_ST(newtimer,tmraddr);
    if((newtimer & 0x80000000) != (oldtimer & 0x80000000))
    {
        /* Indicate XINT to be generated (exit + 8) */
        /* Setup a few regs 1st */
        regs->GR_L(3)=0;
        regs->GR_L(4)=0x00800080;
        regs->GR_L(9)=EVM_L(dlist+4);
        regs->GR_L(11)=vmb;
        /*2017-03-27,ensure VMDSP is off*/
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        B_VMDSTAT&=~VMDSP;
        EVM_STC(B_VMDSTAT,vmb+VMDSTAT);
        /* end of 2017-03-27 */
        UPD_PSW_IA(regs, EVM_L(exitlist+8));
        DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : Complete - Taking exit #8\n"));
        return(0);
    }
    /* Return - Continue DISP0 Processing */
    DEBUG_CPASSISTX(DISP0,logmsg("RUNTIME : Complete - ITIMER Updated\n"));
    return(2);
}

/* DISP0 : Operand 1 : DISP0 Data list, Operand 2 : DISP0 Exit list */
/*         R11 : User to dispatch                                   */
DEF_INST(ecpsvm_dispatch_main)
{
    VADR dlist;
    VADR elist;
    VADR vmb;
    /* PSA Fetched Values */
    BYTE B_CPSTATUS;
    BYTE B_VMDSTAT;
    BYTE B_VMPSTAT;
    BYTE B_VMRSTAT;
    BYTE B_VMPEND;
    BYTE B_VMESTAT;

    VADR F_VMPXINT;
    VADR OXINT; /* Back chain ptr for exit 20 */
    U32  F_VMPSWHI;
    U32  F_VMVCR0;
    U32  F_VMIOINT;
    U32 F_VMVCR2;
    U32 DISPCNT;

    U16  H_XINTMASK;

    U32 iomask;
    BYTE extendmsk;     /* Extended I/O mask */

    ECPSVM_PROLOG(DISP0);

    dlist=effective_addr1;
    elist=effective_addr2;
    vmb=regs->GR_L(11);
    DISPCNT=EVM_L(dlist);
    DISPCNT++;
    /* Question #1 : Are we currently running a user */
    B_CPSTATUS=EVM_IC(CPSTATUS);
    if((B_CPSTATUS & CPRUN))
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : CPRUN On\n"));
        switch(ecpsvm_disp_runtime(regs,&vmb,dlist,elist))
        {
            case 0: /* Exit taken - success */
            EVM_ST(DISPCNT,dlist);
            CPASSIST_HIT(DISP0);
            return;
            case 1: /* no-op DISP0 */
            return;
            default: /* Continue processing */
            break;
        }
        /* Load VMDSTAT */
        B_VMDSTAT=EVM_IC(vmb+VMDSTAT);
        /* Check if I/O Old PSW has tranlation on */
        if(regs->mainstor[0x38] & 0x04)
        {
            DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : I/O Old as XLATE on\n"));
            /* Yes - I/O Interrupt while running a USER */
            if(B_VMDSTAT & VMDSP)
            {
                DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : VMDSP on in VMBLOK - Clean status (Exit #36)\n"));
                /* Clean status - Do exit 36 */
                regs->GR_L(11)=vmb;
                UPD_PSW_IA(regs, EVM_L(elist+36));
                EVM_ST(DISPCNT,dlist);
                CPASSIST_HIT(DISP0);
                return;
            }
        }
    }
    else
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : CPRUN Off\n"));
        /* Check if was in Wait State */
        if(B_CPSTATUS & CPWAIT)
        {
            DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : CPWAIT On : Exit #4\n"));
            /* Take exit #4 : Coming out of wait state */
            /* DMKDSPC3 */
            /* No need to update R11 */
            CPASSIST_HIT(DISP0);
            UPD_PSW_IA(regs, EVM_L(elist+4));
            EVM_ST(DISPCNT,dlist);
            return;
        }
    }
    /* VMB is now either original GPR11 or RUNUSER */
    /* DMKDSP - UNSTACK */
    DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : At UNSTACK : VMBLOK = %8.8X\n",vmb));
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    if(B_VMRSTAT & VMCPWAIT)
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : VMRSTAT VMCPWAIT On (%2.2X) - Taking exit #12\n",B_VMRSTAT));
        /* Take Exit 12 */
        regs->GR_L(11)=vmb;
        UPD_PSW_IA(regs, EVM_L(elist+12));
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Check for PER/PPF (CKPEND) */
    B_VMPEND=EVM_IC(vmb+VMPEND);
    if(B_VMPEND & (VMPERPND | VMPGPND))
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : PER/PPF Pending - Taking exit #16\n"));
        /* Take Exit 16 */
        regs->GR_L(11)=vmb;
        UPD_PSW_IA(regs, EVM_L(elist+16));
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Now, check if we should unstack an external int */
    /* 1st check if VMPXINT is NULL */
    F_VMPSWHI=EVM_L(vmb+VMPSW);     /* Load top of virt PSW - Will need it */
    B_VMPSTAT=EVM_IC(vmb+VMPSTAT);   /* Will need VMPSTAT for I/O ints too */
    F_VMPXINT=EVM_L(vmb+VMPXINT);
    DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : Checking for EXT; Base VMPXINT=%8.8X\n",F_VMPXINT));
    /* This is DMKDSP - CKEXT */
    if(F_VMPXINT!=0)
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : VPSW HI = %8.8X\n",F_VMPSWHI));
        OXINT=vmb+VMPXINT;
        /* Check if Virtual PSW enabled for Externals */
        /* (works in both BC & EC modes) */
        if(F_VMPSWHI & 0x01000000)
        {
            DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : PSW Enabled for EXT\n"));
            /* Use VMVCR0 or CR0 in ECBLOK */
            F_VMVCR0=EVM_L(vmb+VMVCR0);     /* CR0 or ECBLOK Address */
            if(B_VMPSTAT & VMV370R) /* SET ECMODE ON ?? */
            {
                F_VMVCR0=EVM_L(F_VMVCR0+0); /* EXTCR0 at disp +0 in ECBLOK */
            }
            DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : CR0 = %8.8X\n",F_VMVCR0));
            /* scan the XINTBLOKS for a mask match */
            /* Save OXINT in the loop for exit 20  */
            for(;F_VMPXINT;OXINT=F_VMPXINT,F_VMPXINT=EVM_L(F_VMPXINT))      /* XINTNEXT @ +0 in XINTBLOK */
            {
                H_XINTMASK=EVM_LH(F_VMPXINT+10);    /* Get interrupt subclass in XINTBLOK */
                DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : XINTMASK =  %4.4X\n",H_XINTMASK));
                H_XINTMASK &= F_VMVCR0;
                if(H_XINTMASK)           /* Check against CR0 (External subclass mask) */
                {
                    DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : EXT Hit - Taking exit #20\n"));
                    /* Enabled for this external */
                    /* Take exit 20 */
                    regs->GR_L(4)=H_XINTMASK;            /* Enabled subclass bits */
                    regs->GR_L(5)=OXINT;                 /* XINTBLOK Back pointer (or VMPXINT) */
                    regs->GR_L(6)=F_VMPXINT;             /* Current XINTBLOK */
                    regs->GR_L(11)=vmb;                  /* RUNUSER */
                    UPD_PSW_IA(regs, EVM_L(elist+20));   /* Exit +20 */
                    EVM_ST(DISPCNT,dlist);
                    CPASSIST_HIT(DISP0);
                    return;
                }
            }
        }
    }
    /* After CKEXT : No external pending/reflectable */

    /* This is DMKDSP UNSTIO */
    /* Check for pending I/O Interrupt */

    /* Load PIM */
    F_VMIOINT=EVM_LH(vmb+VMIOINT);
    DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : Checking for I/O; VMIOINT=%8.8X\n",F_VMIOINT));
    if(F_VMIOINT!=0)        /* If anything in the pipe */
    {
        F_VMIOINT <<=16;    /* Put IOINT mask in bits 0-15 */
        /* Is V-PSW in EC Mode ? */
        iomask=0;
        extendmsk=0;
        B_VMESTAT=EVM_L(VMESTAT);
        if(B_VMESTAT & VMEXTCM)     /* Implies VMV370R on */
        {
            /* Check I/O bit */
            if(F_VMPSWHI & 0x02000000)
            {
                iomask=0;
                extendmsk=1;
            }
        }
        else
        {
            /* BC Mode PSW */
            /* Isolate channel masks for channels 0-5 */
            iomask=F_VMPSWHI & 0xfc000000;
            if(B_VMPSTAT & VMV370R) /* SET ECMODE ON ? */
            {
                if(F_VMPSWHI & 0x02000000)
                {
                    extendmsk=1;
                }
            }
        }
        if(extendmsk)
        {
            F_VMVCR2=EVM_L(vmb+VMECEXT);
            F_VMVCR2=EVM_L(F_VMVCR2+8);
            iomask |= F_VMVCR2;
        }
        if(iomask & 0xffff0000)
        {
            F_VMIOINT&=iomask;
            if(F_VMIOINT)
            {
                DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : I/O Hit - Taking exit #24\n"));
                /* Take Exit 24 */
                regs->GR_L(7)=F_VMIOINT;
                regs->GR_L(11)=vmb;
                UPD_PSW_IA(regs, EVM_L(elist+24));   /* Exit +24 */
                EVM_ST(DISPCNT,dlist);
                CPASSIST_HIT(DISP0);
                return;
            }
        }
    }
    /* DMKDSP - CKWAIT */
    /* Clear Wait / Idle bits in VMRSTAT */
    B_VMRSTAT=EVM_IC(vmb+VMRSTAT);
    B_VMRSTAT &= ~(VMPSWAIT | VMIDLE);
    EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
    if(F_VMPSWHI & 0x00020000)
    {
        DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : VWAIT - Taking exit #28\n"));
        /* Take exit 28  */
        /* 2017-03-27 Set VMPSWAIT */
        B_VMRSTAT |= VMPSWAIT;
        EVM_STC(B_VMRSTAT,vmb+VMRSTAT);
        /* end of 2017-03-27 */
        regs->GR_L(11)=vmb;
        UPD_PSW_IA(regs, EVM_L(elist+28));   /* Exit +28 */
        CPASSIST_HIT(DISP0);
        EVM_ST(DISPCNT,dlist);
        return;
    }
    /* Take exit 0 (DISPATCH) */
    DEBUG_CPASSISTX(DISP0,logmsg("DISP0 : DISPATCH - Taking exit #0\n"));
    regs->GR_L(11)=vmb;
    UPD_PSW_IA(regs, EVM_L(elist+0));   /* Exit +0 */
    CPASSIST_HIT(DISP0);
    EVM_ST(DISPCNT,dlist);
    return;
}


/******************************************************/
/* SCNRU Instruction : Scan Real Unit                 */
/* Invoked by DMKSCN                                  */
/* E60D                                               */
/* SCNRU D1(R1,B1),D2(R2,B2)                          */
/*                                                    */
/* Operations                                         */
/* The Device Address specified in operand 1 is       */
/* the real device address for which control block    */
/* addresses are to be returned. The storage area     */
/* designated as the 2nd operand is a list of 4       */
/* consecutive fullword (note : the 2nd operand is    */
/* treated as a Real Address, regardless of any       */
/* translation mode that may be in effect)            */
/* +--------------------+-------------------+         */
/* | CHNLINDEX          | RCHTABLE          |         */
/* +--------------------+-------------------+         */
/* | RCUTABLE           | RDVTABLE          |         */
/* +--------------------+-------------------+         */
/* CHNLINDEX is an array of 16 halfwords, each        */
/* representing the offset of the target's device     */
/* channel's RCHBLOCK. If the channel is not defined  */
/* in the RIO table, the index has bit 0, byte 0 set  */
/* to 1                                               */
/*                                                    */
/* The RCHBLOK has at offset +X'20' a table of        */
/* 32 possible control unit indices                   */
/* the device's address bit 8 to 12 are used to fetch */
/* the index specified at the table. If it has        */
/* bit 0 byte 0 set, then the same operation is       */
/* attempted with bits 8 to 11                        */
/* The RCUBLOK then fetched from RCUTABLE + the index */
/* fetched from the RCHBLOK has a DEVICE index table  */
/* at offset +X'28' which can be fetched by using     */
/* bits 5-7                                           */
/* If the RCUBLOK designates an alternate control     */
/* unit block, (Offset +X'5' bit 1 set), then         */
/* the primary RCUBLOK is fetched from offset X'10'   */
/*                                                    */
/* If no RCHBLOK is found, R6,R7 and R8 contain -1    */
/* and Condition Code 3 is set                        */
/*                                                    */
/* if no RCUBLOK is found, R6 contains the RCHBLOK,   */
/* R7 and R8 contain -1, and Condition Code 2 is set  */
/*                                                    */
/* if no RDVBLOK is found, R6 contains the RCHBLOK,   */
/* R7 contains the RCUBLOK and R8 contains -1, and    */
/* condition code 1 is set                            */
/*                                                    */
/* If all 3 control blocks are found, R6 contains the */
/* the RCHBLOK, R7 Contains the RCUBLOK and R8 cont-  */
/* ains the RDVBLOK. Condition code 0 is set          */
/*                                                    */
/* If the instruction is sucesfull, control is        */
/* returned at the address specified by GPR14.        */
/* Otherwise, the next sequential instruction is      */
/* executed, and no GPR or condition code is changed. */
/*                                                    */
/* Exceptions :                                       */
/*       Operation Exception : ECPS:VM Disabled       */
/*       Priviledged Exception : PSW in problem state */
/*                                                    */
/* Note : no access exception is generated for        */
/*        the second operand.                         */
/*                                                    */
/* Note : as of yet, for any other situation than     */
/*        finding all 3 control blocks, SCNRU acts    */
/*        as a NO-OP                                  */
/******************************************************/
DEF_INST(ecpsvm_locate_rblock)
{
    U32 chix;           /* offset of RCH in RCH Array */
    U32 cuix;           /* Offset of RCU in RCU Array */
    U32 dvix;           /* Offset of RDV in RDV Array */
    VADR rchixtbl;      /* RCH Index Table */
    VADR rchtbl;        /* RCH Array */
    VADR rcutbl;        /* RCU Array */
    VADR rdvtbl;        /* RDV Array */
    VADR arioct;        /* Data list for SCNRU */

    VADR rchblk;        /* Effective RCHBLOK Address */
    VADR rcublk;        /* Effective RCUBLOK Address */
    VADR rdvblk;        /* Effective RDVBLOK Address */
    U16 rdev;

    ECPSVM_PROLOG(SCNRU);

    /* Obtain the Device address */
    rdev=(effective_addr1 & 0xfff);
    /* And the DMKRIO tables addresses */
    arioct=effective_addr2;

    DEBUG_CPASSISTX(SCNRU,logmsg(_("HHCEV300D : ECPS:VM SCNRU called; RDEV=%4.4X ARIOCT=%6.6X\n"),effective_addr1,arioct));

    /* Get the Channel Index Table */
    rchixtbl= EVM_L(effective_addr2);

    /* Obtain the RCH offset */
    chix=EVM_LH(rchixtbl+((rdev & 0xf00) >> 7));

    DEBUG_CPASSISTX(SCNRU,logmsg(_("HHCEV300D : ECPS:VM SCNRU : RCH IX = %x\n"),chix));

    /* Check if Bit 0 set (no RCH) */
    if(chix & 0x8000)
    {
        // logmsg(_("HHCEV300D : ECPS:VM SCNRU : NO CHANNEL\n"));
        /*
        regs->GR_L(6)=~0;
        regs->GR_L(7)=~0;
        regs->GR_L(8)=~0;
        UPD_PSW_IA(regs, regs->GR_L(14));
        regs->psw.cc=1;
        */
        /* Right now, let CP handle the case */
        return;
    }

    /* Obtain the RCH Table pointer */
    rchtbl=EVM_L(arioct+4);

    /* Add the RCH Index offset */
    rchblk=rchtbl+chix;

    /* Try to obtain RCU index with bits 8-12 of the device */
    cuix=EVM_LH(rchblk+0x20+((rdev & 0xf8)>>2));
    if(cuix & 0x8000)
    {
        /* Try with bits 8-11 */
        cuix=EVM_LH(rchblk+0x20+((rdev & 0xf0)>>2));
        if(cuix & 0x8000)
        {
            // logmsg(_("HHCEV300D : ECPS:VM SCNRU : NO CONTROL UNIT\n"));
            /*
            regs->GR_L(6)=rchblk;
            regs->GR_L(7)=~0;
            regs->GR_L(8)=~0;
            UPD_PSW_IA(regs, regs->GR_L(14));
            regs->psw.cc=2;
            */
            return;
        }
    }
    DEBUG_CPASSISTX(SCNRU,logmsg(_("HHCEV300D : ECPS:VM SCNRU : RCU IX = %x\n"),cuix));
    rcutbl=EVM_L(arioct+8);
    rcublk=rcutbl+cuix;
    dvix=EVM_LH(rcublk+0x28+((rdev & 0x00f)<<1));
    if(EVM_IC(rcublk+5)&0x40)
    {
        rcublk=EVM_L(rcublk+0x10);
    }
    if(dvix & 0x8000)
    {
        // logmsg(_("HHCEV300D : ECPS:VM SCNRU : NO RDEVBLOK\n"));
        /*
        regs->GR_L(6)=rchblk;
        regs->GR_L(7)=rcublk;
        regs->GR_L(8)=~0;
        UPD_PSW_IA(regs, regs->GR_L(14));
        regs->psw.cc=3;
        */
        return;
    }
    DEBUG_CPASSISTX(SCNRU,logmsg(_("HHCEV300D : ECPS:VM SCNRU : RDV IX = %x\n"),dvix));
    dvix<<=3;
    rdvtbl=EVM_L(arioct+12);
    rdvblk=rdvtbl+dvix;
    DEBUG_CPASSISTX(SCNRU,logmsg(_("HHCEV300D : ECPS:VM SCNRU : RCH = %6.6X, RCU = %6.6X, RDV = %6.6X\n"),rchblk,rcublk,rdvblk));
    regs->GR_L(6)=rchblk;
    regs->GR_L(7)=rcublk;
    regs->GR_L(8)=rdvblk;
    regs->psw.cc=0;
    regs->GR_L(15)=0;
    BR14;
    CPASSIST_HIT(SCNRU);
}
/* CCWGN : Not supported */
DEF_INST(ecpsvm_comm_ccwproc)
{
    ECPSVM_PROLOG(CCWGN);
}
/* UXCCW : Not supported */
DEF_INST(ecpsvm_unxlate_ccw)
{
    ECPSVM_PROLOG(UXCCW);
}
/* DISP2 : Not supported */
DEF_INST(ecpsvm_disp2)
{
    ECPSVM_PROLOG(DISP2);
    switch(ecpsvm_do_disp2(regs,effective_addr1,effective_addr2))
    {
        case 0: /* Done */
            CPASSIST_HIT(DISP2);
            return;
        case 1: /* No-op */
            return;
        case 2: /* Done */
            CPASSIST_HIT(DISP2);
            RETURN_INTCHECK(regs);
    }
    return;
}
/* STEVL : Store ECPS:VM support level */
/* STEVL D1(R1,B1),D2(R2,B2) */
/* 1st operand : Fullword address in which to store ECPS:VM Support level */
/* 2nd operand : ignored */
DEF_INST(ecpsvm_store_level)
{
    ECPSVM_PROLOG(STEVL);
    EVM_ST(sysblk.ecpsvm.level,effective_addr1);
    DEBUG_CPASSISTX(STEVL,logmsg(_("HHCEV300D : ECPS:VM STORE LEVEL %d called\n"),sysblk.ecpsvm.level));
    CPASSIST_HIT(STEVL);
}
/* LCSPG : Locate Changed Shared Page */
/* LCSPG : Not supported */
DEF_INST(ecpsvm_loc_chgshrpg)
{
    ECPSVM_PROLOG(LCSPG);
}
/*************************************************************/
/* FREEX : Allocate CP Storage Extended                      */
/* FREEX B1(R1,B1),D2(R2,B2)                                 */
/* 1st operand : Address of FREEX Parameter list             */
/*               +0 : Maxsize = Max number of DW allocatable */
/*                              with FREEX                   */
/*               +4- : Subpool index table                   */
/* 2nd operand : Subpool table (indexed)                     */
/* GPR 0 : Number of DWs to allocate                         */
/*                                                           */
/* Each allocatable block is forward chained                 */
/* if the subpool is empty, return to caller                 */
/* if the subpool has an entry, allocate from the subpool    */
/* and save the next block address as the subpool chain head */
/* return allocated block in GPR1. return at address in GPR14*/
/* if allocation succeeded                                   */
/* if allocate fails, return at next sequential instruction  */
/*************************************************************/
DEF_INST(ecpsvm_extended_freex)
{
    U32 maxdw;
    U32 numdw;
    U32 maxsztbl;
    U32 spixtbl;
    BYTE spix;
    U32 freeblock;
    U32 nextblk;
    ECPSVM_PROLOG(FREEX);
    numdw=regs->GR_L(0);
    spixtbl=effective_addr2;
    maxsztbl=effective_addr1;
    DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : ECPS:VM FREEX DW = %4.4X\n"),numdw));
    if(numdw==0)
    {
        return;
    }
    DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : MAXSIZE ADDR = %6.6X, SUBPOOL INDEX TABLE = %6.6X\n"),maxsztbl,spixtbl));
    /* E1 = @ of MAXSIZE (maximum # of DW allocatable by FREEX from subpools) */
    /*      followed by subpool pointers                                      */
    /* E2 = @ of subpool indices                                              */
    maxdw=EVM_L(maxsztbl);
    if(regs->GR_L(0)>maxdw)
    {
        DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : FREEX request beyond subpool capacity\n")));
        return;
    }
    /* Fetch subpool index */
    spix=EVM_IC(spixtbl+numdw);
    DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : Subpool index = %X\n"),spix));
    /* Fetch value */
    freeblock=EVM_L(maxsztbl+4+spix);
    DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : Value in subpool table = %6.6X\n"),freeblock));
    if(freeblock==0)
    {
        /* Can't fullfill request here */
        return;
    }
    nextblk=EVM_L(freeblock);
    EVM_ST(nextblk,maxsztbl+4+spix);
    DEBUG_CPASSISTX(FREEX,logmsg(_("HHCEV300D : New Value in subpool table = %6.6X\n"),nextblk));
    regs->GR_L(1)=freeblock;
    regs->psw.cc=0;
    BR14;
    CPASSIST_HIT(FREEX);
    return;
}
/*************************************************************/
/* FRETX : Return CP Free storage                            */
/* FRETX D1(R1,B1),R2(D2,B2)                                 */
/* 1st operand : Max DW for subpool free/fret                */
/* 2nd Operand : FRET PLIST                                  */
/*               +0 Coretable address                        */
/*               +4 CL4'FREE'                                */
/*               +8 Maxsize (same as operand 1)              */
/*               +12 Subpool Table Index                     */
/* The block is checked VS the core table to check if it     */
/* is eligible to be returned to the subpool chains          */
/* If it is, then it is returned. Control is returned at     */
/* the address in GPR 14. Otherwise, if anything cannot      */
/* be resolved, control is returned at the next sequential   */
/* Instruction                                               */
/*************************************************************/
int ecpsvm_do_fretx(REGS *regs,VADR block,U16 numdw,VADR maxsztbl,VADR fretl)
{
    U32 cortbl;
    U32 maxdw;
    U32 cortbe; /* Core table Page entry for fretted block */
    U32 prevblk;
    BYTE spix;
    DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : X fretx called AREA=%6.6X, DW=%4.4X\n"),regs->GR_L(1),regs->GR_L(0)));
    if(numdw==0)
    {
        DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : ECPS:VM Cannot FRETX : DWORDS = 0\n")));
        return(1);
    }
    maxdw=EVM_L(maxsztbl);
    if(numdw>maxdw)
    {
        DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : ECPS:VM Cannot FRETX : DWORDS = %d > MAXDW %d\n"),numdw,maxdw));
        return(1);
    }
    cortbl=EVM_L(fretl);
    cortbe=cortbl+((block & 0xfff000)>>8);
    if(EVM_L(cortbe)!=EVM_L(fretl+4))
    {
        DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : ECPS:VM Cannot FRETX : Area not in Core Free area\n")));
        return(1);
    }
    if(EVM_IC(cortbe+8)!=0x02)
    {
        DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : ECPS:VM Cannot FRETX : Area flag != 0x02\n")));
        return(1);
    }
    spix=EVM_IC(fretl+11+numdw);
    prevblk=EVM_L(maxsztbl+4+spix);
    if(prevblk==block)
    {
        DEBUG_CPASSISTX(FRETX,logmsg(_("HHCEV300D : ECPS:VM Cannot FRETX : fretted block already on subpool chain\n")));
        return(1);
    }
    EVM_ST(block,maxsztbl+4+spix);
    EVM_ST(prevblk,block);
    return(0);
}
DEF_INST(ecpsvm_extended_fretx)
{
    U32 fretl;
    U32 maxsztbl;
    U32 numdw;
    U32 block;

    ECPSVM_PROLOG(FRETX);

    numdw=regs->GR_L(0);
    block=regs->GR_L(1) & ADDRESS_MAXWRAP(regs);
    maxsztbl=effective_addr1 & ADDRESS_MAXWRAP(regs);
    fretl=effective_addr2 & ADDRESS_MAXWRAP(regs);
    if(ecpsvm_do_fretx(regs,block,numdw,maxsztbl,fretl)==0)
    {
        BR14;
        CPASSIST_HIT(FRETX);
    }
    return;
}
DEF_INST(ecpsvm_prefmach_assist)
{
    ECPSVM_PROLOG(PMASS);
}

/**********************************/
/* VM ASSISTS                     */
/**********************************/

/******************************************************************/
/* LPSW/SSM/STxSM :                                               */
/* Not sure about the current processing ..                       */
/* *MAYBE* we need to invoke DMKDSPCH when the newly loaded PSW   */
/* does not need further checking. Now.. I wonder what the point  */
/* is to return to CP anyway, as we have entirelly validated the  */
/* new PSW (i.e. for most of it, this is essentially a BRANCH     */
/* However...                                                     */
/* Maybe we should call DMKDSPCH (from the DMKPRVMA list)         */
/* only if re-enabling bits (and no Int pending)                  */
/*                                                                */
/* For the time being, we do THIS :                               */
/* If the new PSW 'disables' bits or enables bit but MICPEND=0    */
/* we just update the VPSW and continue                           */
/* Same for LPSW.. But we also update the IA                      */
/* If we encounter ANY issue, we just return to caller (which will*/
/* generate a PRIVOP) thus invoking CP like for non-EVMA          */
/******************************************************************/

/******************************************************************/
/* Check psw Transition validity                                  */
/******************************************************************/
/* NOTE : oldr/newr Only have the PSW field valid (the rest is not initialised) */
int     ecpsvm_check_pswtrans(REGS *regs,ECPSVM_MICBLOK *micblok, BYTE micpend, REGS *oldr, REGS *newr)
{
    UNREFERENCED(micblok);
    UNREFERENCED(regs);

    SET_PSW_IA(newr);
    SET_PSW_IA(oldr);

    /* Check for a switch from BC->EC or EC->BC */
    if(ECMODE(&oldr->psw)!=ECMODE(&newr->psw))
    {
        DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New and Old PSW have a EC/BC transition\n")));
        return(1);
    }
    /* Check if PER or DAT is being changed */
    if(ECMODE(&newr->psw))
    {
        if((newr->psw.sysmask & 0x44) != (oldr->psw.sysmask & 0x44))
        {
            DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW Enables DAT or PER\n")));
            return(1);
        }
    }
    /* Check if a Virtual interrupt is pending and new interrupts are being enabled */
    if(micpend & 0x80)
    {
        if(ECMODE(&newr->psw))
        {
            if(((~oldr->psw.sysmask) & 0x03) & newr->psw.sysmask)
            {
                DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW Enables interrupts and MICPEND (EC)\n")));
                return(1);
            }
        }
        else
        {
            if(~oldr->psw.sysmask & newr->psw.sysmask)
            {
                DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW Enables interrupts and MICPEND (BC)\n")));
                return(1);
            }
        }
    }
    if(WAITSTATE(&newr->psw))
    {
        DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW is a WAIT PSW\n")));
        return(1);
    }
    if(ECMODE(&newr->psw))
    {
        if(newr->psw.sysmask & 0xb8)
        {
            DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW sysmask incorrect\n")));
            return(1);
        }
    }
    if(newr->psw.IA & 0x01)
    {
        DEBUG_SASSISTX(LPSW,logmsg(_("HHCEV300D : New PSW has ODD IA\n")));
        return(1);
    }
    return(0);
}
int     ecpsvm_dossm(REGS *regs,int b2,VADR effective_addr2)
{
    BYTE  reqmask;
    BYTE *cregs;
    U32   creg0;
    REGS  npregs;

    SASSIST_PROLOG(SSM);


    /* Reject if V PSW is in problem state */
    if(CR6 & ECPSVM_CR6_VIRTPROB)
    {
        DEBUG_SASSISTX(SSM,logmsg("HHCEV300D : SASSIST SSM reject : V PB State\n"));
        return(1);
    }
    /*
    if(!(micevma & MICSTSM))
    {
        DEBUG_SASSISTX(SSM,logmsg("HHCEV300D : SASSIST SSM reject : SSM Disabled in MICEVMA; EVMA=%2.2X\n",micevma));
        return(1);
    }
    */
    /* Get CR0 - set ref bit on  fetched CR0 (already done in prolog for MICBLOK) */
    cregs=MADDR(micblok.MICCREG,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
    FETCH_FW(creg0,cregs);

    /* Reject if V CR0 specifies SSM Suppression */
    if(creg0 & 0x40000000)
    {
        DEBUG_SASSISTX(SSM,logmsg("HHCEV300D : SASSIST SSM reject : V SSM Suppr\n"));
        return(1);
    }
    /* Load the requested SSM Mask */
    /* USE Normal vfetchb here ! not only do we want tranlsation */
    /* but also fetch protection control, ref bit, etc.. */
    reqmask=ARCH_DEP(vfetchb) (effective_addr2,b2,regs);

    INITPSEUDOREGS(npregs);
    /* Load the virtual PSW AGAIN in a new structure */

    ARCH_DEP(load_psw) (&npregs,vpswa_p);

    npregs.psw.sysmask=reqmask;

    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&npregs))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(SSM,logmsg("HHCEV300D : SASSIST SSM Reject : New PSW too complex\n"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }

    /* While we are at it, set the IA in the V PSW */
    SET_PSW_IA(regs);
    UPD_PSW_IA(&npregs, regs->psw.IA);

    /* Set the change bit */
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    /* store the new PSW */
    ARCH_DEP(store_psw) (&npregs,vpswa_p);
    DEBUG_SASSISTX(SSM,logmsg("HHCEV300D : SASSIST SSM Complete : new SM = %2.2X\n",reqmask));
    DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST SSM New VIRT "));
    DEBUG_SASSISTX(LPSW,display_psw(&npregs));
    DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST SSM New REAL "));
    DEBUG_SASSISTX(LPSW,display_psw(regs));
    SASSIST_HIT(SSM);
    return(0);
}

int     ecpsvm_dosvc(REGS *regs,int svccode)
{
    PSA_3XX *psa;
    REGS newr;

    SASSIST_PROLOG(SVC);

    if(svccode==76)     /* NEVER trap SVC 76 */
    {
        DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC Reject : SVC 76\n"));
        return(1);
    }
    if(CR6 & ECPSVM_CR6_SVCINHIB)
    {
        DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC Reject : SVC Assist Inhibit\n"));
        return(1);      /* SVC SASSIST INHIBIT ON */
    }
    /* Get what the NEW PSW should be */

    psa=(PSA_3XX *)MADDR((VADR)0 , USE_PRIMARY_SPACE, regs, ACCTYPE_READ, 0);
                                                                                         /* Use all around access key 0 */
                                                                                         /* Also sets reference bit     */
    INITPSEUDOREGS(newr);
    ARCH_DEP(load_psw) (&newr, (BYTE *)&psa->svcnew);   /* Ref bit set above */
    DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC NEW VIRT "));
    DEBUG_SASSISTX(SVC,display_psw(&newr));
    /* Get some stuff from the REAL Running PSW to put in OLD SVC PSW */
    SET_PSW_IA(regs);
    UPD_PSW_IA(&vpregs, regs->psw.IA);            /* Instruction Address */
    vpregs.psw.cc=regs->psw.cc;                   /* Condition Code      */
    vpregs.psw.pkey=regs->psw.pkey;               /* Protection Key      */
    vpregs.psw.progmask=regs->psw.progmask;       /* Program Mask        */
    vpregs.psw.intcode=svccode;                   /* SVC Interrupt code  */
    DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC OLD VIRT "));
    DEBUG_SASSISTX(SVC,display_psw(&vpregs));

    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&newr))       /* Check PSW transition capability */
    {
        DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC Reject : Cannot make transition to new PSW\n"));
        return(1); /* Something in the NEW PSW we can't handle.. let CP do it */
    }
    /* Store the OLD SVC PSW */
// ZZ    psa=(PSA_3XX *)MADDR((VADR)0, USE_PRIMARY_SPACE, regs, ACCTYPE_WRITE, 0);
                                                                                         /* Use all around access key 0 */
                                                                                         /* Also sets change bit        */
    /* Set intcode in PSW (for BC mode) */


    ARCH_DEP(store_psw) (&vpregs, (BYTE *)&psa->svcold);

    if(ECMODE(&vpregs.psw))
    {
        /* Also set SVC interrupt code */
        /* and ILC                     */
        STORE_FW((BYTE *)&psa->svcint,0x00020000 | svccode);
    }
    /*
     * Now, update some stuff in the REAL PSW
     */
    SASSIST_LPSW(newr);
    /*
     * Now store the new PSW in the area pointed by the MICBLOK
     */
    ARCH_DEP(store_psw) (&newr,vpswa_p);
    DEBUG_SASSISTX(SVC,logmsg("HHCEV300D : SASSIST SVC Done\n"));
    SASSIST_HIT(SVC);
    return(0);
}
/* LPSW Assist */
int ecpsvm_dolpsw(REGS *regs,int b2,VADR e2)
{
    BYTE * nlpsw;
    REGS nregs;

    SASSIST_PROLOG(LPSW);
    /* Reject if V PSW is in problem state */
    if(CR6 & ECPSVM_CR6_VIRTPROB)
    {
        DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW reject : V PB State\n"));
        return(1);
    }
    /* Reject if MICEVMA says not to do LPSW sim */
    if(!(micevma & MICLPSW))
    {
        DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW reject : LPSW disabled in MICEVMA\n"));
        return(1);
    }
    if(e2&0x03)
    {
        DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW %6.6X - Alignement error\n",e2));
        return(1);

    }
    nlpsw=MADDR(e2,b2,regs,ACCTYPE_READ,regs->psw.pkey);
    INITPSEUDOREGS(nregs);
    ARCH_DEP(load_psw) (&nregs,nlpsw);
    if(ecpsvm_check_pswtrans(regs,&micblok,micpend,&vpregs,&nregs))
    {
        DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW Rejected - Cannot make PSW transition\n"));
        return(1);

    }
    SASSIST_LPSW(nregs);
    MADDR(vpswa,USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
                        /* Set ref bit in address pointed by MICBLOK */
    ARCH_DEP(store_psw) (&nregs,vpswa_p);
    DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW New VIRT "));
    DEBUG_SASSISTX(LPSW,display_psw(&nregs));
    DEBUG_SASSISTX(LPSW,logmsg("HHCEV300D : SASSIST LPSW New REAL "));
    DEBUG_SASSISTX(LPSW,display_psw(regs));
    SASSIST_HIT(LPSW);
    return(0);
}


int     ecpsvm_virttmr_ext(REGS *regs)
{
    DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Checking if we can IRPT\n"));
    DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Virtual"));
    DEBUG_SASSISTX(VTIMER,display_psw(regs));
    if(IS_IC_ECPSVTIMER(regs))
    {
        DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Not pending\n"));
        return(1);
    }
    if(!PROBSTATE(&regs->psw))
    {
        DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Not dispatching a VM\n"));
        return(1);
    }
    if(!(regs->psw.sysmask & PSW_EXTMASK))
    {
        DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Test int : Not enabled for EXT\n"));
        return(1);
    }
    if(!(regs->CR_L(6) & ECPSVM_CR6_VIRTTIMR))
    {
        DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Test int : Not enabled for VTIMER\n"));
        return(1);
    }
    DEBUG_SASSISTX(VTIMER,logmsg("HHCEV300D : SASSIST VTIMER Please, do\n"));
    return(0);
}

/* SIO/SIOF Assist */
int ecpsvm_dosio(REGS *regs,int b2,VADR e2)
{
    SASSIST_PROLOG(SIO);
    UNREFERENCED(b2);
    UNREFERENCED(e2);
    return(1);
}
int ecpsvm_dostnsm(REGS *regs,int b1,VADR effective_addr1,int imm2)
{
    SASSIST_PROLOG(STNSM);
    UNREFERENCED(b1);
    UNREFERENCED(effective_addr1);
    UNREFERENCED(imm2);
    return(1);
}
int ecpsvm_dostosm(REGS *regs,int b1,VADR effective_addr1,int imm2)
{
    SASSIST_PROLOG(STOSM);
    UNREFERENCED(b1);
    UNREFERENCED(effective_addr1);
    UNREFERENCED(imm2);
    return(1);
}

int ecpsvm_dostctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{
    SASSIST_PROLOG(STCTL);

    UNREFERENCED(r1);
    UNREFERENCED(r3);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);
    return(1);
}
int ecpsvm_dolctl(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{
    U32 crs[16];        /* New CRs */
    U32 rcrs[16];       /* REAL CRs */
    U32 ocrs[16];       /* Old CRs */
    BYTE *ecb_p;
    VADR F_ECBLOK,vmb;
    BYTE B_VMPSTAT;

    int i,j,numcrs;

    SASSIST_PROLOG(LCTL);

    if(effective_addr2 & 0x03)
    {
        DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : Not aligned\n"));
        return(1);
    }

    vmb=vpswa-0xA8;
    B_VMPSTAT=EVM_IC(vmb+VMPSTAT);

    if((!(B_VMPSTAT & VMV370R)) && ((r1!=r3) || (r1!=0)))
    {
        DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : BC Mode VM & LCTL != 0,0\n"));
        return(1);
    }
    /* Determine the range of CRs to be loaded */
    if(r1>r3)
    {
        numcrs=(r3+16)-r1;
    }
    else
    {
        numcrs=r3-r1;
    }
    numcrs++;
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
        crs[j]=ARCH_DEP(vfetch4)((effective_addr2+(i*4)) & ADDRESS_MAXWRAP(regs),b2,regs);
    }
    if(B_VMPSTAT & VMV370R)
    {
        F_ECBLOK=fetch_fw(regs->mainstor+vmb+VMECEXT);
    for(i=0;i<16;i++)
    {
        ecb_p=MADDR(F_ECBLOK+(i*4),USE_REAL_ADDR,regs,ACCTYPE_READ,0);
        ocrs[i]=fetch_fw(ecb_p);
    }
    }
    else
    {
        F_ECBLOK=vmb+VMECEXT;  /* Update ECBLOK ADDRESS for VCR0 Update */
    ecb_p=MADDR(F_ECBLOK,USE_REAL_ADDR,regs,ACCTYPE_READ,0);
        /* Load OLD CR0 From VMBLOK */
    ocrs[0]=fetch_fw(ecb_p);
    }
    for(i=0;i<16;i++)
    {
        rcrs[i]=regs->CR_L(i);
    }
    /* Source safely loaded into "crs" array */
    /* Load the CRS - exit from loop if it's not possible */
    DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL %d,%d : Modifying %d cregs\n",r1,r3,numcrs));
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
        switch(j)
        {
            case 0:     /* CR0 Case */
                /* Check 1st 2 bytes of CR0 - No change allowed */
                if((ocrs[0] & 0xffff0000) != (crs[0] & 0xffff0000))
                {
                    DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : CR0 High changed\n"));
                    return 1;
                }
                /* Not allowed if : NEW mask is being enabled AND MICPEND AND PSW has EXT enabled */
                if(vpregs.psw.sysmask & 0x01)
                {
                    if(micpend & 0x80)
                    {
                        if((~(ocrs[0] & 0xffff)) & (crs[0] & 0xffff))
                        {
                            DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : CR0 EXTSM Enables new EXTS\n"));
                            return 1;
                        }
                    }
                }
                ocrs[0]=crs[0];
                break;
            case 1:
                if(ocrs[1] != crs[1])
                {
                    DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : CR1 Updates shadow table\n"));
                    return 1;
                }
                break;
            case 2:
                /* Not allowed if : NEW Channel mask is turned on AND micpend AND PSW Extended I/O Mask is on */
                if(vpregs.psw.sysmask & 0x02)
                {
                    if((~ocrs[2]) & crs[2])
                    {
                        if(micpend & 0x80)
                        {
                            DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : CR2 IOCSM Enables I/O Ints\n"));
                            return(1);
                        }
                    }
                }
                ocrs[2]=crs[2];
                break;
            case 3:     /* DAS Control regs (not used under VM/370) */
            case 4:
            case 5:
            case 7:
/*  2017-01-12 
    LCTL assist should not update real CR3-CR7 with values from a virtual machine execution of LCTL.
    CR3-CR7 are for the DAS feature.  If any of these four control registers are specified then
    the assist should kick it back to CP and let CP handle it, because different versions of VM do
    different things with these CRs depending on whether DAS is available or not.                    */

// original code:
//                ocrs[j]=crs[j];
//                rcrs[j]=crs[j]
//                break;

// replacement code:
                DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : DAS CR%d Update\n",j));
                return(1);
/* end of 2017-01-12 */
            case 6: /* VCR6 Ignored on real machine */
                ocrs[j]=crs[j];
                break;
            case 8: /* Monitor Calls */
                DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : MC CR8 Update\n"));
                return(1);
            case 9: /* PER Control Regs */
            case 10:
            case 11:
                DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL Reject : PER CR%d Update\n",j));
                return(1);
            case 12:
            case 13: /* 12-13 : Unused */
                ocrs[j]=crs[j];
                rcrs[j]=crs[j];
                break;
            case 14:
            case 15: /* 14-15 Machine Check & I/O Logout control (plus DAS) */
                ocrs[j]=crs[j];
                break;
            default:
                break;
        }
    }
    /* Update REAL Control regs */
    for(i=0;i<16;i++)
    {
        regs->CR_L(i)=rcrs[i];
    }
    /* Update ECBLOK/VMBLOK Control regs */
    /* Note : if F_ECBLOK addresses VMVCR0 in the VMBLOCK */
    /*        check has already been done to make sure    */
    /*        r1=0 and numcrs=1                           */
    for(j=r1,i=0;i<numcrs;i++,j++)
    {
        if(j>15)
        {
            j-=16;
        }
    ecb_p=MADDR(F_ECBLOK+(j*4),USE_REAL_ADDR,regs,ACCTYPE_WRITE,0);
    store_fw(ecb_p,ocrs[j]);
    }
    DEBUG_SASSISTX(LCTL,logmsg("HHCEV300D : SASSIST LCTL %d,%d Done\n",r1,r3));
    SASSIST_HIT(LCTL);
    return 0;
}
int ecpsvm_doiucv(REGS *regs,int b2,VADR effective_addr2)
{
    SASSIST_PROLOG(IUCV);

    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);
    return(1);
}
int ecpsvm_dodiag(REGS *regs,int r1,int r3,int b2,VADR effective_addr2)
{
    SASSIST_PROLOG(DIAG);
    UNREFERENCED(r1);
    UNREFERENCED(r3);
    UNREFERENCED(b2);
    UNREFERENCED(effective_addr2);
    return(1);
}
static char *ecpsvm_stat_sep="HHCEV003I +-----------+----------+----------+-------+\n";

static int ecpsvm_sortstats(const void *a,const void *b)
{
    ECPSVM_STAT *ea,*eb;
    ea=(ECPSVM_STAT *)a;
    eb=(ECPSVM_STAT *)b;
    return(eb->call-ea->call);
}

static void ecpsvm_showstats2(ECPSVM_STAT *ar,size_t count)
{
    char *sep=ecpsvm_stat_sep;
    char nname[32];
    int  havedisp=0;
    int  notshown=0;
    size_t unsupcc=0;
    int haveunsup=0;
    int callt=0;
    int hitt=0;
    size_t i;
    for(i=0;i<count;i++)
    {
        if(ar[i].call)
        {
            callt+=ar[i].call;
            hitt+=ar[i].hit;
            if(!ar[i].support)
            {
                unsupcc+=ar[i].call;
                haveunsup++;
            }
            havedisp=1;
            snprintf(nname,32,"%s%s",ar[i].name,ar[i].support ? "" : "*");
            if(!ar[i].enabled)
            {
                strcat(nname,"-");
            }
            if(ar[i].debug)
            {
                strcat(nname,"%");
            }
            if(ar[i].total)
            {
                strcat(nname,"+");
            }
            logmsg(_("HHCEV001I | %-9s | %8d | %8d |  %3d%% |\n"),
                    nname,
                    ar[i].call,
                    ar[i].hit,
                    ar[i].call ?
                            (ar[i].hit*100)/ar[i].call :
                            100);
        }
        else
        {
            notshown++;
        }
    }
    if(havedisp)
    {
        logmsg(sep);
    }
    logmsg(_("HHCEV001I | %-9s | %8d | %8d |  %3d%% |\n"),
            "Total",
            callt,
            hitt,
            callt ?
                    (hitt*100)/callt :
                    100);
    logmsg(sep);
    if(haveunsup)
    {
        logmsg(_("HHCEV004I * : Unsupported, - : Disabled, %% - Debug\n"));
    }
    if(notshown)
    {
        logmsg(_("HHCEV005I %d Entr%s not shown (never invoked)\n"),notshown,notshown==1?"y":"ies");
    }
    if(unsupcc)
    {
        if(unsupcc==1)
        {
                logmsg(_("HHCEV006I 1 call was made to an unsupported function\n"));
        }
        else
        {
            logmsg(_("HHCEV006I %d calls where made to unsupported functions\n"),unsupcc);
        }
    }
    return;
}
/* SHOW STATS */
void ecpsvm_showstats(int ac,char **av)
{
    size_t      asize;
    ECPSVM_STAT *ar;
    char *sep=ecpsvm_stat_sep;

    UNREFERENCED(ac);
    UNREFERENCED(av);
    logmsg(sep);
    logmsg(_("HHCEV002I | %-9s | %-8s | %-8s | %-5s |\n"),"VM ASSIST","Calls","Hits","Ratio");
    logmsg(sep);
    ar=malloc(sizeof(ecpsvm_sastats));
    memcpy(ar,&ecpsvm_sastats,sizeof(ecpsvm_sastats));
    asize=sizeof(ecpsvm_sastats)/sizeof(ECPSVM_STAT);
    qsort(ar,asize,sizeof(ECPSVM_STAT),ecpsvm_sortstats);
    ecpsvm_showstats2(ar,asize);
    free(ar);
    logmsg(sep);
    logmsg(_("HHCEV002I | %-9s | %-8s | %-8s | %-5s |\n"),"CP ASSIST","Calls","Hits","Ratio");
    logmsg(sep);
    ar=malloc(sizeof(ecpsvm_cpstats));
    memcpy(ar,&ecpsvm_cpstats,sizeof(ecpsvm_cpstats));
    asize=sizeof(ecpsvm_cpstats)/sizeof(ECPSVM_STAT);
    qsort(ar,asize,sizeof(ECPSVM_STAT),ecpsvm_sortstats);
    ecpsvm_showstats2(ar,asize);
    free(ar);
}


ECPSVM_STAT *ecpsvm_findstat(char *feature,char **fclass)
{
    ECPSVM_STAT *es;
    ECPSVM_STAT *esl;
    int i;
    int fcount;
    fcount=sizeof(ecpsvm_sastats)/sizeof(ECPSVM_STAT);
    esl=(ECPSVM_STAT *)&ecpsvm_sastats;
    for(i=0;i<fcount;i++)
    {
        es=&esl[i];
        if(strcasecmp(feature,es->name)==0)
        {
            *fclass="VM ASSIST";
            return(es);
        }
    }
    esl=(ECPSVM_STAT *)&ecpsvm_cpstats;
    fcount=sizeof(ecpsvm_cpstats)/sizeof(ECPSVM_STAT);
    for(i=0;i<fcount;i++)
    {
        es=&esl[i];
        if(strcasecmp(feature,es->name)==0)
        {
            *fclass="CP ASSIST";
            return(es);
        }
    }
    return(NULL);
}

void ecpsvm_enadisaall(char *fclass,ECPSVM_STAT *tbl,size_t count,int onoff,int debug)
{
    ECPSVM_STAT *es;
    size_t i;
    char *enadisa,*debugonoff;
    enadisa=onoff?"Enabled":"Disabled";
    debugonoff=debug?"On":"Off";
    for(i=0;i<count;i++)
    {
        es=&tbl[i];
        if(onoff>=0)
        {
            es->enabled=onoff;
            logmsg(_("HHCEV015I ECPS:VM %s feature %s %s\n"),fclass,es->name,enadisa);
        }
        if(debug>=0)
        {
            es->debug=debug;
            logmsg(_("HHCEV015I ECPS:VM %s feature %s Debug %s\n"),fclass,es->name,debugonoff);
        }
    }
    if(onoff>=0)
    {
        logmsg(_("HHCEV016I All ECPS:VM %s features %s\n"),fclass,enadisa);
    }
    if(debug>=0)
    {
        logmsg(_("HHCEV016I All ECPS:VM %s features Debug %s\n"),fclass,debugonoff);
    }
}

void ecpsvm_enable_disable(int ac,char **av,int onoff,int debug)
{
    char        *fclass;
    char        *enadisa,*debugonoff;
    int i;

    size_t      sacount,cpcount;
    ECPSVM_STAT *es;
    ECPSVM_STAT *sal,*cpl;

    sal=(ECPSVM_STAT *)&ecpsvm_sastats;
    cpl=(ECPSVM_STAT *)&ecpsvm_cpstats;
    sacount=sizeof(ecpsvm_sastats)/sizeof(ECPSVM_STAT);
    cpcount=sizeof(ecpsvm_cpstats)/sizeof(ECPSVM_STAT);

    enadisa=onoff?"Enabled":"Disabled";
    debugonoff=debug?"On":"Off";
    if(ac==1)
    {
        ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
        ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
        if(debug>=0)
        {
            sysblk.ecpsvm.debug=debug;
            logmsg(_("HHCEV013I ECPS:VM Global Debug %s\n"),debugonoff);
        }
        return;
    }
    for(i=1;i<ac;i++)
    {
        if(strcasecmp(av[i],"ALL")==0)
        {
            ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
            ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
            return;
        }
        if(strcasecmp(av[i],"VMA")==0)
        {
            ecpsvm_enadisaall("VM ASSIST",sal,sacount,onoff,debug);
            return;
        }
        if(strcasecmp(av[i],"CPA")==0)
        {
            ecpsvm_enadisaall("CP ASSIST",cpl,cpcount,onoff,debug);
            return;
        }
        es=ecpsvm_findstat(av[i],&fclass);
        if(es!=NULL)
        {
            if(onoff>=0)
            {
                es->enabled=onoff;
                logmsg(_("HHCEV014I ECPS:VM %s feature %s %s\n"),fclass,es->name,enadisa);
            }
            if(debug>=0)
            {
                es->debug=onoff;
                logmsg(_("HHCEV014I ECPS:VM %s feature %s Debug %s\n"),fclass,es->name,debugonoff);
            }
        }
        else
        {
            logmsg(_("HHCEV014I Unknown ECPS:VM feature %s; Ignored\n"),av[i]);
        }
    }
}

void ecpsvm_disable(int ac,char **av)
{
    ecpsvm_enable_disable(ac,av,0,-1);
}
void ecpsvm_enable(int ac,char **av)
{
    ecpsvm_enable_disable(ac,av,1,-1);
}
void ecpsvm_debug(int ac,char **av)
{
    ecpsvm_enable_disable(ac,av,-1,1);
}
void ecpsvm_nodebug(int ac,char **av)
{
    ecpsvm_enable_disable(ac,av,-1,0);
}

void ecpsvm_level(int ac,char **av)
{
    int lvl;
    if(sysblk.ecpsvm.available)
    {
        logmsg(_("HHCEV016I Current reported ECPS:VM Level is %d\n"),sysblk.ecpsvm.level);
    }
    else
    {
        logmsg(_("HHCEV016I Current reported ECPS:VM Level is %d\n"),sysblk.ecpsvm.level);
        logmsg(_("HHCEV017I But ECPS:VM is currently disabled\n"));
    }
    if(ac>1)
    {
        lvl=atoi(av[1]);
        logmsg(_("HHCEV016I Level reported to guest program is now %d\n"),lvl);
        sysblk.ecpsvm.level=lvl;
    }
    if(sysblk.ecpsvm.level!=20)
    {
          logmsg(_("HHCEV017W WARNING ! current level (%d) is not supported\n"),sysblk.ecpsvm.level);
          logmsg(_("HHCEV018W WARNING ! Unpredictable results may occur\n"));
          logmsg(_("HHCEV019I The microcode support level is 20\n"));
    }
}

static  void ecpsvm_helpcmd(int,char **);

static ECPSVM_CMDENT ecpsvm_cmdtab[]={
    {"Help",1,ecpsvm_helpcmd,"Show help","format : \"evm help [cmd]\" Shows help on the specified\n"
                                                        "        ECPSVM subcommand\n"},
    {"STats",2,ecpsvm_showstats,"Show statistical counters","format : evm stats : Shows various ECPS:VM Counters\n"},
    {"DIsable",2,ecpsvm_disable,"Disable ECPS:VM Features","format : evm disable [ALL|feat1[ feat2|...]\n"},
    {"ENable",2,ecpsvm_enable,"Enable ECPS:VM Features","format : evm enable [ALL|feat1[ feat2|...]\n"},
#if defined(DEBUG_SASSIST) || defined(DEBUG_CPASSIST)
    {"DEBUG",5,ecpsvm_debug,"Debug ECPS:VM Features","format : evm debug [ALL|feat1[ feat2|...]\n"},
    {"NODebug",3,ecpsvm_nodebug,"Turn Debug off for ECPS:VM Features","format : evm NODebug [ALL|feat1[ feat2|...]\n"},
#endif
    {"Level",1,ecpsvm_level,"Set/Show ECPS:VM level","format : evm Level [nn]\n"},
    {NULL,0,NULL,NULL,NULL}};

static void ecpsvm_helpcmdlist(void)
{
    int i;
    ECPSVM_CMDENT *ce;

    for(i=0;ecpsvm_cmdtab[i].name;i++)
    {
        ce=&ecpsvm_cmdtab[i];
        logmsg(_("HHCEV010I : %s : %s\n"),ce->name,ce->expl);
    }
    return;
}

void ecpsvm_helpcmd(int ac,char **av)
{
    ECPSVM_CMDENT *ce;
    if(ac==1)
    {
        ecpsvm_helpcmdlist();
        return;
    }
    ce=ecpsvm_getcmdent(av[1]);
    if(ce==NULL)
    {
        logmsg(_("HHCEV011E Unknown subcommand %s - valid subcommands are :\n"),av[1]);
        ecpsvm_helpcmdlist();
        return;
    }
    logmsg(_("HHCEV012I : %s : %s"),ce->name,ce->help);
    return;
}

ECPSVM_CMDENT *ecpsvm_getcmdent(char *cmd)
{
    ECPSVM_CMDENT *ce;
    int i;
    int clen;
    for(i=0;ecpsvm_cmdtab[i].name;i++)
    {
        ce=&ecpsvm_cmdtab[i];
        if(strlen(cmd)<=strlen(ce->name) && strlen(cmd)>=(size_t)ce->abbrev)
        {
            clen=strlen(cmd);
            if(strncasecmp(cmd,ce->name,clen)==0)
            {
                return(ce);
            }
        }
    }
    return(NULL);
}
void ecpsvm_command(int ac,char **av)
{
    ECPSVM_CMDENT *ce;
    logmsg(_("HHCEV011I ECPS:VM Command processor invoked\n"));
    if(ac==1)
    {
        logmsg(_("HHCEV008E NO EVM subcommand. Type \"evm help\" for a list of valid subcommands\n"));
        return;
    }
    ce=ecpsvm_getcmdent(av[1]);
    if(ce==NULL)
    {
        logmsg(_("HHCEV008E Unknown EVM subcommand %s\n"),av[1]);
        return;
    }
    ce->fun(ac-1,av+1);
    logmsg(_("HHCEV011I ECPS:VM Command processor complete\n"));
}

#endif /* ifdef FEATURE_ECPSVM */

/* Note : The following forces inclusion for S/390 & z/ARCH */
/*        This is necessary just in order to properly define */
/*        S/390 auxialiary routines invoked by S/370 routines */
/*        because of SIE                                     */

#if !defined(_GEN_ARCH)

#if defined(_ARCHMODE2)
 #define  _GEN_ARCH _ARCHMODE2
 #include "ecpsvm.c"
#endif

#if defined(_ARCHMODE3)
 #undef   _GEN_ARCH
 #define  _GEN_ARCH _ARCHMODE3
 #include "ecpsvm.c"
#endif

#endif /*!defined(_GEN_ARCH)*/

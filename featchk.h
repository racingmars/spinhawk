/* FEATCHK.H    (c) Copyright Jan Jaeger, 2000-2009                  */
/*              Feature definition consistency checks                */

/*-------------------------------------------------------------------*/
/*  Perform various checks on feature combinations, and set          */
/*  additional flags to percolate certain features such as           */
/*  SIE down to lower architecture levels such that these            */
/*  can include emulation support                                    */
/*                                                                   */
/*  FEATURE_XXXX is defined per architecture mode, and               */
/*  _FEATURE_XXXX is defined across all architectures                */
/*  if FEATURE_ XXXX is defined for any architecture mode.           */
/*                                                                   */
/*-------------------------------------------------------------------*/

#if defined(FEATCHK_CHECK_ALL)

/* FEATURE_INTERPRETIVE_EXECUTION is related to host related issues
   _FEATURE_SIE is related to guest (emulation) related issues.  This
   because if FEATURE_INTERPRETIVE_EXECUTION is defined for say 390
   mode, then _FEATURE_SIE will also need to be in 370 in order to
   support 370 mode SIE emulation */
#if defined(FEATURE_INTERPRETIVE_EXECUTION)
 #define _FEATURE_SIE
 #if defined(FEATURE_ESAME)
  #define _FEATURE_ZSIE
 #endif
 #if defined(FEATURE_PROTECTION_INTERCEPTION_CONTROL)
  #define _FEATURE_PROTECTION_INTERCEPTION_CONTROL
 #endif
#endif

/* _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE is used for host
   related processing issues, FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
   is defined only in ESA/390 mode. MCDS is an ESA/390
   feature that is supported under z/Architecture SIE */
#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE)
 #define _FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE
#endif

#if defined(FEATURE_2K_STORAGE_KEYS)
 #define _FEATURE_2K_STORAGE_KEYS
#endif

#if defined(FEATURE_INTERVAL_TIMER)
 #define _FEATURE_INTERVAL_TIMER
#endif

#if defined(FEATURE_ECPSVM)
 #define _FEATURE_ECPSVM
#endif

#if defined(FEATURE_VECTOR_FACILITY)
 #define _FEATURE_VECTOR_FACILITY
#endif

#if defined(FEATURE_CHANNEL_SUBSYSTEM)
 #define _FEATURE_CHANNEL_SUBSYSTEM
#endif

#if defined(FEATURE_SYSTEM_CONSOLE)
 #define _FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_EXPANDED_STORAGE)
 #define _FEATURE_EXPANDED_STORAGE
#endif

#if defined(FEATURE_ECPSVM)
 #define _FEATURE_ECPSVM
#endif

#if defined(_FEATURE_SIE) && defined(FEATURE_STORAGE_KEY_ASSIST)
 #define _FEATURE_STORAGE_KEY_ASSIST
#endif

#if defined(FEATURE_CPU_RECONFIG)
 #define _FEATURE_CPU_RECONFIG
#endif

#if defined(FEATURE_PER)
 #define _FEATURE_PER
#endif

#if defined(FEATURE_PER2)
 #define _FEATURE_PER2
#endif

#if defined(FEATURE_EXPEDITED_SIE_SUBSET)
#define _FEATURE_EXPEDITED_SIE_SUBSET
#endif

#if defined(FEATURE_REGION_RELOCATE)
 #define _FEATURE_REGION_RELOCATE
#endif

#if defined(FEATURE_IO_ASSIST)
 #define _FEATURE_IO_ASSIST
#endif

#if defined(FEATURE_WAITSTATE_ASSIST)
 #define _FEATURE_WAITSTATE_ASSIST
#endif

#if defined(FEATURE_EXTERNAL_INTERRUPT_ASSIST)
 #define _FEATURE_EXTERNAL_INTERRUPT_ASSIST
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST
#endif

#if defined(FEATURE_ASN_AND_LX_REUSE)
 #define _FEATURE_ASN_AND_LX_REUSE
#endif

#if defined(FEATURE_INTEGRATED_3270_CONSOLE)
 #define _FEATURE_INTEGRATED_3270_CONSOLE
#endif

#if defined(FEATURE_INTEGRATED_ASCII_CONSOLE)
 #define _FEATURE_INTEGRATED_ASCII_CONSOLE
#endif

#if defined(FEATURE_ESAME)
 #define _FEATURE_ESAME
#endif

#if defined(FEATURE_ESAME_N3_ESA390)
 #define _FEATURE_ESAME_N3_ESA390
#endif

#if defined(FEATURE_DAT_ENHANCEMENT)
 #define _FEATURE_DAT_ENHANCEMENT
#endif

#if defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)
 #define _FEATURE_STORE_FACILITY_LIST_EXTENDED
#endif

#if defined(FEATURE_ENHANCED_DAT_FACILITY)
 #define _FEATURE_ENHANCED_DAT_FACILITY
#endif

#if defined(FEATURE_ENHANCED_DAT_FACILITY_2)
 #define _FEATURE_ENHANCED_DAT_FACILITY_2
#endif

#if defined(FEATURE_SENSE_RUNNING_STATUS)
 #define _FEATURE_SENSE_RUNNING_STATUS
#endif

#if defined(FEATURE_CONDITIONAL_SSKE)
 #define _FEATURE_CONDITIONAL_SSKE
#endif

#if defined(FEATURE_CONFIGURATION_TOPOLOGY_FACILITY)
 #define _FEATURE_CONFIGURATION_TOPOLOGY_FACILITY
#endif

#if defined(FEATURE_IPTE_RANGE_FACILITY)
 #define _FEATURE_IPTE_RANGE_FACILITY
#endif

#if defined(FEATURE_NONQUIESCING_KEY_SETTING_FACILITY)
 #define _FEATURE_NONQUIESCING_KEY_SETTING_FACILITY
#endif

#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_2)
 #define _FEATURE_EXTENDED_TRANSLATION_FACILITY_2
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST
#endif

#if defined(FEATURE_LONG_DISPLACEMENT)
 #define _FEATURE_LONG_DISPLACEMENT
#endif

#if defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT)
 #define _FEATURE_HFP_MULTIPLY_ADD_SUBTRACT
#endif

#if defined(FEATURE_EXTENDED_IMMEDIATE)
 #define _FEATURE_EXTENDED_IMMEDIATE
#endif

#if defined(FEATURE_EXTENDED_TRANSLATION_FACILITY_3)
 #define _FEATURE_EXTENDED_TRANSLATION_FACILITY_3
#endif

#if defined(FEATURE_HFP_UNNORMALIZED_EXTENSION)
 #define _FEATURE_HFP_UNNORMALIZED_EXTENSION
#endif

#if defined(FEATURE_ETF2_ENHANCEMENT)
 #define _FEATURE_ETF2_ENHANCEMENT
#endif

#if defined(FEATURE_STORE_CLOCK_FAST)
 #define _FEATURE_STORE_CLOCK_FAST
#endif

#if defined(FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS)
 #define _FEATURE_MOVE_WITH_OPTIONAL_SPECIFICATIONS
#endif

#if defined(FEATURE_TOD_CLOCK_STEERING)
 #define _FEATURE_TOD_CLOCK_STEERING
#endif

#if defined(FEATURE_ETF3_ENHANCEMENT)
 #define _FEATURE_ETF3_ENHANCEMENT
#endif

#if defined(FEATURE_EXTRACT_CPU_TIME)
 #define _FEATURE_EXTRACT_CPU_TIME
#endif

#if defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)
 #define _FEATURE_COMPARE_AND_SWAP_AND_STORE
#endif

#if defined(FEATURE_COMPARE_AND_SWAP_AND_STORE_FACILITY_2)
 #define _FEATURE_COMPARE_AND_SWAP_AND_STORE_FACILITY_2
#endif

#if defined(FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY)
 #define _FEATURE_GENERAL_INSTRUCTIONS_EXTENSION_FACILITY
#endif

#if defined(FEATURE_EXECUTE_EXTENSIONS_FACILITY)
 #define _FEATURE_EXECUTE_EXTENSIONS_FACILITY
#endif

#if defined(FEATURE_ENHANCED_MONITOR_FACILITY)
 #define _FEATURE_ENHANCED_MONITOR_FACILITY
#endif

#if defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)
 #define _FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY
#endif

#if defined(FEATURE_FPS_ENHANCEMENT)
 #define _FEATURE_FPS_ENHANCEMENT
#endif

#if defined(FEATURE_DECIMAL_FLOATING_POINT)
 #define _FEATURE_DECIMAL_FLOATING_POINT
#endif

#if defined(FEATURE_PFPO)
 #define _FEATURE_PFPO
#endif

#if defined(FEATURE_FAST_BCR_SERIALIZATION_FACILITY)
 #define _FEATURE_FAST_BCR_SERIALIZATION_FACILITY
#endif

#if defined(FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY)
 #define _FEATURE_RESET_REFERENCE_BITS_MULTIPLE_FACILITY
#endif

#if defined(FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY)
 #define _FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY
#endif

#if defined(FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY)
 #define _FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY
#endif

#if defined(FEATURE_ACCESS_EXCEPTION_FETCH_STORE_INDICATION)
 #define _FEATURE_ACCESS_EXCEPTION_FETCH_STORE_INDICATION
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_3
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_4
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_1)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_1
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_2)
 #define _FEATURE_MESSAGE_SECURITY_ASSIST_EXTENSION_2
#endif

#if defined(FEATURE_HERCULES_DIAGCALLS)
 #define _FEATURE_HERCULES_DIAGCALLS
#endif

#undef _VSTORE_C_STATIC
#if !defined(OPTION_NO_INLINE_VSTORE)
 #define _VSTORE_C_STATIC static inline
 #define _VSTORE_FULL_C_STATIC static
#else
 #define _VSTORE_C_STATIC
 #define _VSTORE_FULL_C_STATIC
#endif

#undef _VFETCH_C_STATIC
#if !defined(OPTION_NO_INLINE_IFETCH)
 #define _VFETCH_C_STATIC static inline
#else
 #define _VFETCH_C_STATIC
#endif

#undef _DAT_C_STATIC
#if !defined(OPTION_NO_INLINE_DAT)
 #define _DAT_C_STATIC static inline
#else
 #define _DAT_C_STATIC
#endif

#undef _LOGICAL_C_STATIC
#if !defined(OPTION_NO_INLINE_LOGICAL)
 #define _LOGICAL_C_STATIC static inline
#else
 #define _LOGICAL_C_STATIC
#endif

#if !defined(OPTION_370_MODE) \
  && !defined(OPTION_390_MODE) \
  && !defined(OPTION_900_MODE)
 #error No Architecture mode
#endif
#if defined(OPTION_370_MODE)
 #define _370
 #define _ARCHMODE1 370
 #define ARCH_370 0
#endif

#if defined(OPTION_390_MODE)
 #define _390
 #if !defined(_ARCHMODE1)
  #define _ARCHMODE1 390
  #define ARCH_390 0
 #else
  #define _ARCHMODE2 390
  #define ARCH_390 1
 #endif
#endif

#if defined(OPTION_900_MODE)
 #define _900
 #if !defined(_ARCHMODE2)
  #define _ARCHMODE2 900
  #define ARCH_900 1
 #else
  #define _ARCHMODE3 900
  #define ARCH_900 2
 #endif
#endif

#if !defined(ARCH_370)
 #define ARCH_370 -1
#endif
#if !defined(ARCH_390)
 #define ARCH_390 -1
#endif
#if !defined(ARCH_900)
 #define ARCH_900 -1
#endif

#if defined(_ARCHMODE3)
 #define GEN_MAXARCH    3+2
#elif defined(_ARCHMODE2)
 #define GEN_MAXARCH    2+2
#else
 #define GEN_MAXARCH    1+2
#endif

#if defined(_900) && !defined(_390)
 #error OPTION_390_MODE must be enabled for OPTION_900_MODE
#endif


#else /*!defined(FEATCHK_CHECK_ALL)*/

/* When ESAME is installed then all instructions
   marked N3 in the reference are also available
   in ESA/390 mode */
#if defined(_900) && (__GEN_ARCH == 390)
 #define FEATURE_ESAME_N3_ESA390
#endif

#if !defined(FEATURE_2K_STORAGE_KEYS) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Storage keys must be 2K or 4K
#endif

#if defined(FEATURE_EXTENDED_STORAGE_KEYS)
 #if !defined(FEATURE_S370E_EXTENDED_ADDRESSING)
  #define FEATURE_S370E_EXTENDED_ADDRESSING
 #endif
#endif

#if defined(FEATURE_EXPANDED_STORAGE) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Expanded storage cannot be defined with 2K storage keys
#endif

#if defined(_900) && defined(FEATURE_VECTOR_FACILITY)
 #error Vector Facility not supported on ESAME capable processors
#endif

#if !defined(FEATURE_S370_CHANNEL) && !defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error Either S/370 Channel or Channel Subsystem must be defined
#endif

#if defined(FEATURE_S370_CHANNEL) && defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error S/370 Channel and Channel Subsystem cannot both be defined
#endif

#if defined(FEATURE_CANCEL_IO_FACILITY) \
 && !defined(FEATURE_CHANNEL_SUBSYSTEM)
 #error Cancel I/O facility requires Channel Subsystem
#endif

#if defined(FEATURE_MOVE_PAGE_FACILITY_2) \
 && !defined(FEATURE_4K_STORAGE_KEYS)
 #error Move page facility cannot be defined with 2K storage keys
#endif

#if defined(FEATURE_FAST_SYNC_DATA_MOVER) \
 && !defined(FEATURE_MOVE_PAGE_FACILITY_2)
 #error Fast sync data mover facility requires Move page facility
#endif

#if defined(FEATURE_ESAME) \
  && defined(FEATURE_INTERPRETIVE_EXECUTION) \
  && !defined(_FEATURE_SIE)
 #error ESA/390 SIE must be defined when defining ESAME SIE
#endif

#if defined(FEATURE_ENHANCED_DAT_FACILITY_2) \
  && !defined(FEATURE_ENHANCED_DAT_FACILITY)
 #error Enhanced DAT facility 2 requires enhanced DAT facility
#endif

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error MCDS only supported with SIE
#endif

#if defined(FEATURE_PROTECTION_INTERCEPTION_CONTROL) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Protection Interception Control only supported with SIE
#endif

#if defined(FEATURE_MULTIPLE_CONTROLLED_DATA_SPACE) \
 && !defined(FEATURE_STORAGE_KEY_ASSIST)
 #error MCDS requires storage key assist
#endif

#if defined(FEATURE_SIE) && defined(FEATURE_ESAME) \
 && !defined(FEATURE_STORAGE_KEY_ASSIST)
 #error ESAME SIE requires storage key assist
#endif

#if defined(FEATURE_STORAGE_KEY_ASSIST) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Storage Key assist only supported with SIE
#endif

#if defined(FEATURE_REGION_RELOCATE) \
 && !defined(FEATURE_INTERPRETIVE_EXECUTION)
 #error Region Relocate Facility only supported with SIE
#endif

#if defined(FEATURE_IO_ASSIST) \
 && !defined(_FEATURE_SIE)
 #error I/O Assist Feature only supported with SIE
#endif

#if defined(FEATURE_IO_ASSIST) \
 && !defined(_FEATURE_REGION_RELOCATE)
 #error Region Relocate Facility required for IO Assist
#endif

#if defined(FEATURE_EXTERNAL_INTERRUPT_ASSIST) \
 && !defined(_FEATURE_SIE)
 #error External Interruption assist only supported with SIE
#endif

#if defined(FEATURE_EXPEDITED_SIE_SUBSET) \
 && !defined(_FEATURE_SIE)
 #error Expedited SIE Subset only supported with SIE
#endif

#if defined(FEATURE_ASN_AND_LX_REUSE)
 #if !defined(FEATURE_DUAL_ADDRESS_SPACE)
  #error ASN-and-LX-Reuse requires Dual Address-Space feature
 #endif
 #if !defined(FEATURE_ESAME)
  #error ASN-and-LX-Reuse is only supported with ESAME
 #endif
#endif

#if defined(FEATURE_ESAME) \
 && defined(FEATURE_VECTOR_FACILITY)
 #error Vector Facility not supported in ESAME mode
#endif

#if defined(FEATURE_BINARY_FLOATING_POINT) \
 && !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #error Binary floating point requires basic FP extensions
#endif

#if defined(FEATURE_DECIMAL_FLOATING_POINT) \
 && !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #error Decimal floating point requires basic FP extensions
#endif

#if defined(FEATURE_BASIC_FP_EXTENSIONS) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error Basic FP extensions requires hexadecimal floating point
#endif

#if !defined(FEATURE_BASIC_FP_EXTENSIONS)
 #if defined(FEATURE_HFP_EXTENSIONS) \
  || defined(FEATURE_FPS_EXTENSIONS)
  #error Floating point extensions require basic FP extensions
 #endif
#endif

#if defined(FEATURE_FPS_EXTENSIONS) \
 && !defined(FEATURE_BINARY_FLOATING_POINT)
 #error FP support extensions requires binary floating point
#endif

#if defined(FEATURE_HFP_MULTIPLY_ADD_SUBTRACT) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error HFP multiply add/subtract requires hexadecimal floating point
#endif

#if defined(FEATURE_HFP_UNNORMALIZED_EXTENSION) \
 && !defined(FEATURE_HEXADECIMAL_FLOATING_POINT)
 #error HFP unnormalized extension requires hexadecimal floating point
#endif

#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY) \
 && !defined(FEATURE_BINARY_FLOATING_POINT)
 #error Floating point extension facility requires binary floating point
#endif

#if defined(FEATURE_FLOATING_POINT_EXTENSION_FACILITY) \
 && !defined(FEATURE_DECIMAL_FLOATING_POINT)
 #error Floating point extension facility requires decimal floating point
#endif

#if defined(FEATURE_PER2) && !defined(FEATURE_PER)
 #error FEATURE_PER must be defined when using FEATURE_PER2
#endif

#if defined(FEATURE_PER3) && !defined(FEATURE_PER)
 #error FEATURE_PER must be defined when using FEATURE_PER3
#endif

#if defined(FEATURE_COMPARE_AND_SWAP_AND_STORE_FACILITY_2) && !defined(FEATURE_COMPARE_AND_SWAP_AND_STORE)
 #error FEATURE_COMPARE_AND_SWAP_AND_STORE must be defined when using FEATURE_COMPARE_AND_SWAP_AND_STORE_FACILITY_2
#endif

#if defined(FEATURE_INTEGRATED_3270_CONSOLE) && !defined(FEATURE_SYSTEM_CONSOLE)
 #error Integrated 3270 console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_INTEGRATED_ASCII_CONSOLE) && !defined(FEATURE_SYSTEM_CONSOLE)
 #error Integrated ASCII console requires FEATURE_SYSTEM_CONSOLE
#endif

#if defined(FEATURE_VM_BLOCKIO) && !defined(FEATURE_EMULATE_VM)
 #error VM Standard Block I/O DIAGNOSE 0x250 requires FEATURE_EMULATE_VM
#endif

#if defined(FEATURE_MESSAGE_SECURITY_ASSIST)
 #if defined(_370)
  #define _370_FEATURE_MESSAGE_SECURITY_ASSIST
 #endif
 #if defined(_390)
  #define _390_FEATURE_MESSAGE_SECURITY_ASSIST
 #endif
 #if defined(_900)
  #define _900_FEATURE_MESSAGE_SECURITY_ASSIST
 #endif
#endif /*defined(FEATURE_MESSAGE_SECURITY_ASSIST)*/

#if defined(FEATURE_STORE_FACILITY_LIST_EXTENDED)
  #if !defined(FEATURE_STORE_FACILITY_LIST)
    #error STFLE requires STFL (FEATURE_STORE_FACILITY_LIST_EXTENDED requires FEATURE_STORE_FACILITY_LIST)
  #endif
#endif

#if defined(FEATURE_ACCESS_EXCEPTION_FETCH_STORE_INDICATION)         /*810*/
  #if !defined(FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION)
    #error Access-Exception Fetch/Store Indication facility requires Enhanced Suppression on Protection
  #endif
#endif

#if defined(FEATURE_ENHANCED_SUPPRESSION_ON_PROTECTION)              /*208*/
  #if !defined(FEATURE_SUPPRESSION_ON_PROTECTION)
    #error Enhanced Suppression on Protection facility requires Suppression on Protection
  #endif
#endif

#if defined(FEATURE_CPU_MEASUREMENT_COUNTER_FACILITY)\
    || defined(FEATURE_CPU_MEASUREMENT_SAMPLING_FACILITY)
  #if !defined(FEATURE_LOAD_PROGRAM_PARAMETER_FACILITY)
    #error CPU Measurement facilities requires Load Program Parameter facility
  #endif
#endif

#if defined(FEATURE_TCPIP_EXTENSION)
  #define _FEATURE_TCPIP_EXTENSION
#endif

#endif /*!defined(FEATALL_CHECKALL)*/

/* end of FEATCHK.H */

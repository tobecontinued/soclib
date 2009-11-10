
#if defined(CONFIG_SOCLIB_MEMCHECK)
# include "iss_memchecker.h"
#endif

#if defined(CONFIG_GDB_SERVER)
# include "gdbserver.h"
#endif


#if defined(CONFIG_CPU_MIPS)

# include "mips32.h"
# if defined(CONFIG_CPU_ENDIAN_BIG)
#  warning Using MIPS32Eb
typedef soclib::common::Mips32EbIss ProcessorIss;
# elif defined(CONFIG_CPU_ENDIAN_LITTLE)
#  warning Using MIPS32El
typedef soclib::common::Mips32ElIss ProcessorIss;
# endif
# define DEFAULT_KERNEL "mutekh/kernel-soclib-mips.out"
  
#elif defined(CONFIG_CPU_PPC)

# include "ppc405.h"
# warning Using PPC
typedef soclib::common::Ppc405Iss ProcessorIss;
# define DEFAULT_KERNEL "mutekh/kernel-soclib-ppc.out"
  
#elif defined(CONFIG_CPU_ARM)

# include "arm.h"
# warning Using ARM
typedef soclib::common::ArmIss ProcessorIss;
# define DEFAULT_KERNEL "mutekh/kernel-soclib-arm.out"
  
#else
# error No supported processor configuration defined
#endif





#if defined(CONFIG_GDB_SERVER)
# if defined(CONFIG_SOCLIB_MEMCHECK)
#  warning Using GDB and memchecker
typedef soclib::common::GdbServer<soclib::common::IssMemchecker<ProcessorIss> > Processor;
# else
#  warning Using GDB
typedef soclib::common::GdbServer<ProcessorIss> Processor;
# endif
#elif defined(CONFIG_SOCLIB_MEMCHECK)
# warning Using Memchecker
typedef soclib::common::IssMemchecker<ProcessorIss> Processor;
#else
# warning Using raw processor
typedef ProcessorIss Processor;
#endif

/*
 *  Modified by Alexandre Becoulet for the SoCLib BSP
 */

#include <bsp.h>
#include <rtems.h>
#include <stdlib.h>

void mips_default_isr( int vector );

#define SOCLIB_ICU_MAX_VECTOR   32

#define SOCLIB_ICU_REG_ISR      0
#define SOCLIB_ICU_REG_IER_RO   4
#define SOCLIB_ICU_REG_IER_SET  8
#define SOCLIB_ICU_REG_IER_CLR  12
#define SOCLIB_ICU_REG_VECTOR   16

unsigned int mips_interrupt_number_of_vectors = SOCLIB_ICU_MAX_VECTOR;

#define SOCLIB_ICU_READ( _base, _register ) \
  ld_le32( (volatile uint32_t*)((_base) + (_register)))

#define SOCLIB_ICU_WRITE( _base, _register, _value ) \
  st_le32(((volatile uint32_t*)((_base) + (_register))), _value);

#include <rtems/bspIo.h>  /* for printk */

void mips_soclib_icu_init(void)
{
  SOCLIB_ICU_WRITE( SOCLIB_ICU_BASE, SOCLIB_ICU_REG_IER_SET, 0xffffffff );
  /* TTY driver is using polling only */
  SOCLIB_ICU_WRITE( SOCLIB_ICU_BASE, SOCLIB_ICU_REG_IER_CLR, 1 << TTY_VECTOR );
}

void mips_vector_isr_handlers( CPU_Interrupt_frame *frame )
{
  unsigned int sr;
  unsigned int cause;

  mips_get_sr( sr );
  mips_get_cause( cause );

  cause &= (sr & SR_IMASK);
  cause >>= CAUSE_IPSHIFT;

  if ( cause & 0xfc ) {
    unsigned int v = SOCLIB_ICU_READ( SOCLIB_ICU_BASE, SOCLIB_ICU_REG_VECTOR );

    if ( v < SOCLIB_ICU_MAX_VECTOR && _ISR_Vector_table[v] )
      (_ISR_Vector_table[v])(v, frame);
    else
      mips_default_isr(v);
  }
}

void mips_default_isr( int vector )
{
  unsigned int sr;
  unsigned int cause;

  mips_get_sr( sr );
  mips_get_cause( cause );

  printk( "Unhandled isr exception: vector 0x%02x, cause 0x%08X, sr 0x%08X\n",
	  vector, cause, sr );
  rtems_fatal_error_occurred(1);
}


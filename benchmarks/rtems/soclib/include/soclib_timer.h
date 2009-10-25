/*
 * Soclib timer registers
 *
 * Alexandre Becoulet for the SoCLib BSP
 */

#ifndef SOCLIB_TIMER_H_
#define SOCLIB_TIMER_H_

#define SOCLIB_TIMER_REG_VALUE          0
#define SOCLIB_TIMER_REG_MODE           4
# define SOCLIB_TIMER_REG_MODE_EN       0x01
# define SOCLIB_TIMER_REG_MODE_IRQEN    0x02
#define SOCLIB_TIMER_REG_PERIOD         8
#define SOCLIB_TIMER_REG_IRQ            12
#define SOCLIB_TIMER_REGSPACE_SIZE      16

#define SOCLIB_TIMER_READ( _base, _register ) \
  ld_le32( (volatile uint32_t*)((_base) + (_register)))

#define SOCLIB_TIMER_WRITE( _base, _register, _value ) \
  st_le32(((volatile uint32_t*)((_base) + (_register))), _value);

#endif


# 1 "mwmr.c"
# 1 "/Users/nipo/projects/soclib/soclib/platform/runtime_netlist/mwmr/soft//"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "mwmr.c"
# 29 "mwmr.c"
# 1 "mwmr.h" 1
# 29 "mwmr.h"
# 1 "/Users/nipo/projects/soclib/utils/include/soclib/mwmr_controller.h" 1
# 30 "/Users/nipo/projects/soclib/utils/include/soclib/mwmr_controller.h"
# 1 "/Users/nipo/projects/soclib/utils/include/soclib/soclib_io.h" 1
# 30 "/Users/nipo/projects/soclib/utils/include/soclib/soclib_io.h"
# 1 "./stdint.h" 1
# 32 "./stdint.h"
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

typedef int int32_t;
typedef short int16_t;
typedef char int8_t;

typedef int ssize_t;
typedef uint32_t size_t;
# 31 "/Users/nipo/projects/soclib/utils/include/soclib/soclib_io.h" 2

static inline uint32_t __uint32_swap(uint32_t x)
{
    return (
        ( (x & 0xff) << 24 ) |
        ( (x & 0xff00) << 8 ) |
        ( (x >> 8) & 0xff00 ) |
        ( (x >> 24) & 0xff )
        );
}







static inline void soclib_io_set(void *comp_base, int reg, uint32_t val)
{
 volatile uint32_t *addr = (uint32_t *)comp_base;





 addr += reg;



 *addr = val;

}

static inline uint32_t soclib_io_get(void *comp_base, int reg)
{
 volatile uint32_t *addr = (uint32_t *)comp_base;
    uint32_t val;






 addr += reg;
    val = *addr;



 return val;

}

static inline void soclib_io_write8(void *comp_base, int reg, uint8_t val)
{
 volatile uint32_t *addr = (uint32_t *)comp_base;
 addr += reg;

 *(uint8_t *)addr = val;
}

static inline uint8_t soclib_io_read8(void *comp_base, int reg)
{
 volatile uint32_t *addr = (uint32_t *)comp_base;
 addr += reg;

 return *(uint8_t *)addr;
}
# 31 "/Users/nipo/projects/soclib/utils/include/soclib/mwmr_controller.h" 2

enum SoclibMwmrRegisters {
    MWMR_IOREG_MAX = 16,
    MWMR_RESET = MWMR_IOREG_MAX,
    MWMR_CONFIG_FIFO_WAY,
    MWMR_CONFIG_FIFO_NO,
    MWMR_CONFIG_STATUS_ADDR,
    MWMR_CONFIG_DEPTH,
    MWMR_CONFIG_WIDTH,
    MWMR_CONFIG_BUFFER_ADDR,
    MWMR_CONFIG_RUNNING,
};

enum SoclibMwmrWay {
    MWMR_TO_COPROC,
    MWMR_FROM_COPROC,
};

typedef struct
{
 uint32_t rptr;
 uint32_t wptr;
 uint32_t usage;
 uint32_t lock;
} soclib_mwmr_status_s;
# 30 "mwmr.h" 2
# 1 "stdint.h" 1
# 31 "mwmr.h" 2

typedef struct mwmr_s {
 uint32_t lock;
    const unsigned int width;
 const unsigned int gdepth;
    uint32_t *const buffer;
 volatile soclib_mwmr_status_s status;
} mwmr_t;




void
mwmr_hw_init( void *coproc, enum SoclibMwmrWay way,
     unsigned int no, const mwmr_t *mwmr );

void mwmr_config( void *coproc, unsigned int no, const uint32_t val );

uint32_t mwmr_status( void *coproc, unsigned int no );

void mwmr_write( mwmr_t *mwmr, const void *buffer, size_t size );
void mwmr_read( mwmr_t *mwmr, void *buffer, size_t size );
# 30 "mwmr.c" 2


static void *memcpy( void *_dst, void *_src, unsigned long size )
{
 uint32_t *dst = _dst;
 uint32_t *src = _src;
 if ( ! ((uint32_t)dst & 3) && ! ((uint32_t)src & 3) )
  while (size > 3) {
   *dst++ = *src++;
   size -= 4;
  }

 unsigned char *cdst = (unsigned char*)dst;
 unsigned char *csrc = (unsigned char*)src;

 while (size--) {
  *cdst++ = *csrc++;
 }
 return _dst;
}

static inline void mwmr_lock( volatile uint32_t *lock )
{
 __asm__ __volatile__(
  ".set push        \n\t"
  ".set noreorder   \n\t"
  "1:               \n\t"
  "ll    $2, %0     \n\t"
  "bnez  $2, 1b     \n\t"
  "ori   $3, $0, 1  \n\t"
  "sc    $3, %0     \n\t"
  "beqz  $3, 1b     \n\t"
  "nop              \n\t"
  "2:               \n\t"
  ".set pop         \n\t"
  :
  : "a"(lock)
  : "$3", "$2", "memory"
  );
}

static inline void mwmr_unlock( volatile uint32_t *lock )
{
 *lock = 0;
}

static inline void busy_wait( ncycles )
{
 int i;
 for ( i=0; i<ncycles; i+=2 )
  asm volatile("nop");
}

static inline size_t min(size_t a, size_t b)
{
 if (a<b)
  return a;
 return b;
}


void
mwmr_hw_init( void *coproc, enum SoclibMwmrWay way,
     unsigned int no, const mwmr_t *mwmr )
{
 soclib_io_set( coproc, MWMR_CONFIG_FIFO_WAY, way );
 soclib_io_set( coproc, MWMR_CONFIG_FIFO_NO, no );
 soclib_io_set( coproc, MWMR_CONFIG_STATUS_ADDR, (uint32_t)&mwmr->status );
 soclib_io_set( coproc, MWMR_CONFIG_DEPTH, mwmr->gdepth );
 soclib_io_set( coproc, MWMR_CONFIG_WIDTH, mwmr->width );
 soclib_io_set( coproc, MWMR_CONFIG_BUFFER_ADDR, (uint32_t)mwmr->buffer );
 soclib_io_set( coproc, MWMR_CONFIG_RUNNING, 1 );
}

void mwmr_config( void *coproc, unsigned int no, const uint32_t val )
{

 soclib_io_set( coproc, no, val );
}

uint32_t mwmr_status( void *coproc, unsigned int no )
{

 return soclib_io_get( coproc, no );
}

void mwmr_read( mwmr_t *fifo, void *_ptr, size_t lensw )
{
 uint8_t *ptr = _ptr;
    volatile soclib_mwmr_status_s *status = &fifo->status;

 mwmr_lock( &status->lock );
    while ( lensw ) {
        size_t len;
        while (status->usage < fifo->width) {
            mwmr_unlock( &status->lock );
   busy_wait(1000);
            mwmr_lock( &status->lock );
        }
        while ( lensw && status->usage >= fifo->width ) {
   void *sptr;

            if ( status->rptr < status->wptr )
                len = status->usage;
            else
                len = (fifo->gdepth - status->rptr);
            len = min(len, lensw);
   sptr = &((uint8_t*)fifo->buffer)[status->rptr];
            memcpy( ptr, sptr, len );
            status->rptr += len;
            if ( status->rptr == fifo->gdepth )
                status->rptr = 0;
            ptr += len;
            status->usage -= len;
            lensw -= len;
        }
    }
 mwmr_unlock( &status->lock );
}

void mwmr_write( mwmr_t *fifo, const void *_ptr, size_t lensw )
{
 uint8_t *ptr = _ptr;
    volatile soclib_mwmr_status_s *status = &fifo->status;

 mwmr_lock( &status->lock );
    while ( lensw ) {
        size_t len;
        while (status->usage >= fifo->gdepth) {
            mwmr_unlock( &status->lock );
   busy_wait(1000);
            mwmr_lock( &status->lock );
        }
        while ( lensw && status->usage < fifo->gdepth ) {
   void *dptr;

            if ( status->rptr <= status->wptr )
                len = (fifo->gdepth - status->wptr);
            else
                len = fifo->gdepth - status->usage;
            len = min(len, lensw);
   dptr = &((uint8_t*)fifo->buffer)[status->wptr];
            memcpy( dptr, ptr, len );
            status->wptr += len;
            if ( status->wptr == fifo->gdepth )
                status->wptr = 0;
            ptr += len;
            status->usage += len;
            lensw -= len;
        }
    }
 mwmr_unlock( &status->lock );
}

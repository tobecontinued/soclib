#ifndef _ST231_SIMOSCALL_H_
#define _ST231_SIMOSCALL_H_

#include "../xst_simoscall/xst_simoscall.h"

namespace st231{
xiss_bool_t simoscall_mem_read(void *sd,
                                   xiss_addr_t address,
                                   xiss_size_t size,
                                   xiss_byte_t *buffer);

xiss_bool_t simoscall_mem_write(void *sd,
                                    xiss_addr_t address,
                                    xiss_size_t size,
                                    const xiss_byte_t *buffer);

xiss_bool_t simoscall_reg_read(void * desc,
				    xiss_uint32_t slice_id,
				    xiss_uint32_t thread_id,
				    xiss_uint32_t reg_no,
				    xiss_uint32_t *value);

xiss_bool_t simoscall_reg_write(void * desc,
				    xiss_uint32_t slice_id,
				    xiss_uint32_t thread_id,
				    xiss_uint32_t reg_no,
				    xiss_uint32_t value);

xiss_bool_t simoscall_exit(void * desc,
				xiss_uint32_t slice_id,
				xiss_uint32_t thread_id,
				xiss_int32_t status);

}
#endif


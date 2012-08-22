/* -*- c++ -*-
 *
 * DSX is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * DSX is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DSX; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Copyright (c) UPMC, Lip6, Asim
 */

#ifndef SRL_PRIVATE_TYPES
#define SRL_PRIVATE_TYPES


namespace dsx { namespace caba {

/****** MWMR ******/
struct srl_mwmr_s {
    size_t width;//words 
    const char *name;
    uint32_t id;
    int way;//1:read , 0: write

    srl_mwmr_s(size_t width, const char* name, uint32_t id, int way):
    name(name)
    {
        this->width = width;
        this->id =   id;
        this->way = way;
    }
};

typedef struct srl_mwmr_s* srl_mwmr_t;

/* The mwmr functions must be in the main class definition */


}}

#endif /* SRL_PRIVATE_TYPES */

#!/usr/bin/env python

import dsx
import sys
from dsx import *

dcache_lines = int(sys.argv[1])
icache_lines = int(sys.argv[2])




nbproc = 2

# Declaration of all MWMR fifos
tg_demux    = Mwmr('tg_demux', 32, 2)
demux_vld   = Mwmr('demux_vld',32,2)
vld_iqzz    = Mwmr('vld_iqzz',128,2)
iqzz_idct   = Mwmr('iqzz_idct',256,2)
idct_libu   = Mwmr('idct_libu',64,2)
libu_ramdac = Mwmr('libu_ramdac',8*160,2)
huffman     = Mwmr('huffman',32,2)
quanti      = Mwmr('quanti',64,2)

tcg = Tcg(
    Task( 'tg', "tg",
          {'output':tg_demux },
          defines = {'FILE_NAME':'"flux.raw"'}),
    Task( 'demux', "demux",
          { 'input':tg_demux,
            'output':demux_vld,
            'huffman':huffman,
            'quanti':quanti },
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    Task( 'vld', "vld",
          { 'input':demux_vld,
            'output':vld_iqzz,
            'huffman':huffman},
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    Task( 'iqzz',"iqzz",
          { 'input':vld_iqzz,
            'output':iqzz_idct,
            'quanti':quanti},
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    Task( 'idct',"idct",
          { 'input':iqzz_idct,
            'output':idct_libu},
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    Task( 'libu',"libu",
          { 'input':idct_libu,
            'output':libu_ramdac},
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    Task( 'ramdac', "ramdac",
          { 'input': libu_ramdac },
          defines = {'WIDTH':"160",
                     'HEIGHT':"120"}),
    )


#########################################################
# Section B : Hardware architecture
#
# The file containing the architecture definition
# must be included, and the path to the directory
# containing this file must be defined
#########################################################

from vgmn_noirq_multi import VgmnNoirqMulti

archi = VgmnNoirqMulti( proc_count = nbproc, ram_count = 2, icache_lines = icache_lines, dcache_lines = dcache_lines )


#########################################################
# Section C : Mapping
#
#########################################################

mapper = dsx.Mapper(archi, tcg)

# mapping the MWMR channel

mapper.map( "tg_demux",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "demux_vld",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "vld_iqzz",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "iqzz_idct",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "idct_libu",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "libu_ramdac",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "huffman",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")

mapper.map( "quanti",
  buffer  = "cram1",
  status  = "cram1",
  desc    = "cram1")
# mapping the "prod0" and "cons0" tasks 

mapper.map("demux",
   run = "cpu1",
   stack   = "cram0",
   desc    = "cram0",
   status  = "uram0")

mapper.map("vld",
   run = "cpu0",
   stack   = "cram0",
   desc    = "cram0",
   status  = "uram0")

mapper.map("iqzz",
   run = "cpu1",
   stack   = "cram0",
   desc    = "cram0",
   status  = "uram0")

#mapper.map("idct",
#   run = "cpu0",
#   stack   = "cram0",
#   desc    = "cram0",
#   status  = "uram0")

mapper.map("libu",
   run = "cpu1",
   stack   = "cram0",
   desc    = "cram0",
   status  = "uram0")


mapper.map('tg',
	  coprocessor = 'tg0',
	  controller = 'tg0_ctrl'
	  )
mapper.map('ramdac',
	  coprocessor = 'ramdac0',
	  controller = 'ramdac0_ctrl'
	  )
mapper.map('idct',
	  coprocessor = 'idct0',
	  controller = 'idct0_ctrl'
	  )
# mapping the software objects associated to a processor
for i in range (nbproc):
    mapper.map( 'cpu%d' %i,
                private    = "cram0",
                shared   = "cram0")

# mapping the software objects used by the embedded OS

mapper.map(tcg,
  private  = "cram1",
  shared  = "uram1",
  code    = "cram1",
  tty     = "tty0",
  tty_no  = 0)



######################################################
# Section D : Code generation 
######################################################

# Embedded software linked with the Mutek/S OS

mapper.generate( dsx.MutekS(verbosity="NONE") )
#mapper.generate( dsx.MutekH() ) 

# The software application for a POSX workstation can still be generated

tcg.generate( dsx.Posix() )








p = Posix()
tcg.generate(p)


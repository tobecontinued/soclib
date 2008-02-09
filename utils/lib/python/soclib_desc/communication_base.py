
from component import PortDecl, Signal

Signal('caba:clock',
		   classname = 'sc_core::sc_clock',
		   header_files = [],
		   accepts = {'caba:clock_in':10000},
		   )

Signal('caba:bit',
		   classname = 'sc_core::sc_signal<bool>',
		   accepts = {'caba:bit_in':10000},
		   header_files = []
		   )

PortDecl('caba:bit_in',
		 signal = 'caba:bit',
		 classname = 'sc_core::sc_in<bool>',
		   header_files = []
		 )

PortDecl('caba:clock_in',
		 signal = 'caba:clock',
		 classname = 'sc_core::sc_clock_in',
		   header_files = []
		 )

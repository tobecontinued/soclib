
from component import PortDecl, Signal
import parameter

Signal('caba:clock',
		   classname = 'sc_core::sc_clock',
		   header_files = [],
		   accepts = {'caba:clock_in':10000},
		   )

Signal('caba:bit',
		   classname = 'sc_core::sc_signal<bool>',
		   accepts = {'caba:bit_in':10000,'caba:bit_out':10000},
		   header_files = []
		   )

PortDecl('caba:bit_in',
		 signal = 'caba:bit',
		 classname = 'sc_core::sc_in<bool>',
		   header_files = []
		 )

PortDecl('caba:bit_out',
		 signal = 'caba:bit',
		 classname = 'sc_core::sc_out<bool>',
		   header_files = []
		 )

Signal('caba:word',
		   classname = 'sc_core::sc_signal',
		   accepts = {'caba:word_in':10000,'caba:word_out':10000},
	   tmpl_parameters = [
	parameter.Type('word_t'),
	],
		   header_files = []
		   )

PortDecl('caba:word_in',
		 signal = 'caba:word',
		 classname = 'sc_core::sc_in',
	   tmpl_parameters = [
	parameter.Type('word_t'),
	],
		   header_files = []
		 )

PortDecl('caba:word_out',
		 signal = 'caba:word',
		 classname = 'sc_core::sc_out',
	   tmpl_parameters = [
	parameter.Type('word_t'),
	],
		   header_files = []
		 )

PortDecl('caba:clock_in',
		 signal = 'caba:clock',
		 classname = 'sc_core::sc_clock_in',
		   header_files = []
		 )

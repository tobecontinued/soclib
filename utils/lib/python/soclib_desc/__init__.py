
import communication_base as _cb

__id__ = "$Id$"
__version__ = "$Revision$"

def get_descs():
	from soclib_cc.config import config
	import components
	components.getDescs(config.desc_paths)
	
import _warning_formatter

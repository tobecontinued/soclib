
import communication_base as _cb

def get_descs():
	from soclib_cc.config import config
	import components
	components.getDescs(config.desc_paths)
	
import _warning_formatter

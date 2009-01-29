
import warnings
import sys
import os

_green = '\x1b[32m'
_red = '\x1b[31m'
_normal = '\x1b[m'

def formatwarning(message, category, filename, lineno, line = None):
	return "%s at %s%s%s:%d: %s%s%s\n"%(
		category.__name__,
		_green, filename, _normal, lineno,
		_red, message, _normal)

if sys.stderr.isatty() and not os.getenv('EMACS'):
	warnings.formatwarning = formatwarning


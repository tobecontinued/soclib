
import os
import os.path
import re

__id__ = "$Id$"
__version__ = "$Revision$"

__all__ = ['version']

_id_re = re.compile(r'\$Id(.*)\$')
_version_re = re.compile(r'\$Revision(.*)\$')

def revision(path):
	try:
		fd = open(path, 'r')
	except:
		return "error"
	found = _version_re.search(fd.read())
	if not found:
		return "no version"
	return str(found.group(1)).strip()

if __name__ == '__main__':
	import sys
	for f in sys.argv[1:]:
		print f, revision(f)

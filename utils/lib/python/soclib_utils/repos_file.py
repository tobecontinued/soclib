
import os
import os.path

__all__ = ['version']

def parse_kv(path):
	kv = []
	fd = open(path, 'r')
	locs = {}
	for line in fd:
		t = line.strip()
		if t.startswith('K ') or t.startswith('V '):
			b = ''
			l = int(t[2:])
			while len(b) < l:
				b += fd.next()
			b = b[:l]
			locs[t[0]] = b
		elif t == 'END':
			kv.append((locs['K'],locs['V']))
			continue
	fd.close()
	return kv

def revision_from_svn_info(path):
	import popen2
	import xml.dom.minidom as minidom
	try:
		stdout, stdin, stderr = popen2.popen3('svn status -v --xml "%s"'%path)
		stdin.close()
		stderr.close()
		dom = minidom.parse(stdout)
		stdout.close()
		status = dom.getElementsByTagName('status')[0]
		target = status.getElementsByTagName('target')[0]
		entry = target.getElementsByTagName('entry')[0]
		wc_status = entry.getElementsByTagName('wc-status')[0]
		mod = wc_status.getAttribute('item')
		if mod == 'normal':
			mod = ''
		else:
			mod = ', '+mod
		return wc_status.getAttribute('revision')+mod
	except:
		return "unknown"

def revision_from_wcprops(path):
	try:
		if os.path.isdir(path):
			vs = map(lambda x:x[1], parse_kv(os.path.join(path, '.svn/all-wcprops')))
			url = vs[:1]
		else:
			d = os.path.dirname(path)
			f = os.path.basename(path)
			vs = map(lambda x:x[1], parse_kv(os.path.join(d, '.svn/all-wcprops')))
			url = filter(lambda x:x.endswith('/'+f), vs)
		if not url:
			return '[unversioned]'
		vlist = url[0].split('/')
		return vlist[vlist.index('ver')+1]
	except:
		return '[parse error]'

def revision(path):
	return revision_from_svn_info(path)

if __name__ == '__main__':
	import sys
	for f in sys.argv[1:]:
		print f, revision(f)

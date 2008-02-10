
class MappingTable:
	def __init__(self, name):
		self.__name = name
	def __str__(self):
		return self.__name
	def getCxxType(self):
		return 'soclib::common::MappingTable'
	def instArgs(self):
		return 'bla'
	def instName(self):
		return self.__name

class IntTab:
	def __init__(self, *vals):
		self.__vals = vals
	def __str__(self):
		return 'IntTab(%s)'%(', '.join(map(str, self.__vals)))

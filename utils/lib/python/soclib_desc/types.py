
class MappingTable:
	def __init__(self):
		pass

class IntTab:
	def __init__(self, *vals):
		self.__vals = vals
	def __str__(self):
		return 'IntTab%s'%self.__vals

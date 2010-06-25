
class OpTable:
    def __init__(self, name, width, slices, *ops, **fallback):
        self.__name = name
        self.__fallback = fallback
        self.__slices = slices
        self.__width = width
        self.__ops = ops
        self.__ins = [[] for i in range(self.__width+1)]
        self.__table = {}
        self.__op_list = set()
        self.__max_width = self.__width

        for en in self.__ops:
            try:
                encoding, func, name = en
            except:
                encoding, func = en
                name = func
            nx = len(filter(lambda x:x=="X", encoding))
            self.__ins[nx].append((encoding, name, func))

        for width, ops in enumerate(reversed(self.__ins)):
            width = self.__width - width
            for encoding, name, func in ops:
                self.__max_width = max((len(encoding), self.__max_width))
                self.__op_list.add('op_'+func)
                val = encoding.replace(' ', '').replace('X', '%s')
                for i in range(1<<width):
                    target = int(val%tuple(self.bin(width, i)), 2)
                    comment = ''
                    if target in self.__table and self.__table[target][1] != func:
                        comment = ', '.join(filter(None,
                                                   [self.__table[target][2], 'collision with %s'%self.__table[target][1]]
                                                   ))
                    if comment:
                        comment = ' /* %s */'%comment
                    self.__table[target] = (encoding, func, comment, name)

        default_for_ill = (' '*self.__max_width, 'ill', '', 'ill')
        for i in range(1<<self.__width):
            if i not in self.__table:
                self.__table[i] = default_for_ill

        self.__max_func_width = max(map(lambda x:len(x[1]), self.__table.itervalues()))
        self.__max_name_width = max(map(lambda x:len(x[3]), self.__table.itervalues()))
        
    @staticmethod
    def bin(w, x):
        r = ''
        for i in range(w-1, -1, -1):
            r += '1' if (x & (1<<i)) else '0'
        return r

    def ops(self):
        return self.__op_list

    def decls(self):
        return '\n'.join((
            'static func_t const %s_table [%d];'%(self.__name, 1<<self.__width),
            'static const char* const %s_name_table [%d];'%(self.__name, 1<<self.__width),
            'const char* name_%s(data_t) const;'%(self.__name),
            ))

    def print_jumps(self, stream, verbose):
        print >>stream, 'ArmIss::func_t const ArmIss::%s_table [%d] = {'%(
            self.__name, (1<<self.__width)),
        if verbose:
            for i in range(1<<self.__width):
                print >> stream
                print >> stream, "   ",
                entry = self.__table.get(i)
                print >> stream,  "/* % .3d  %s  : %s %s */ op(%s),%s"%(
                    i, self.bin(self.__width, i),
                    entry[0].replace('X', '_'),
                    entry[3].ljust(self.__max_name_width),
                    entry[1].ljust(self.__max_func_width),
                    entry[2]),
        else:
            for i in range(1<<self.__width):
                if i % 4 == 0:
                    print >> stream
                    print >> stream, "   ",
                print >> stream,  ("op(%s),"%self.__table.get(i)[1]).ljust(self.__max_func_width+5),
        print >> stream,  '\n};\n'

    def print_names(self, stream):
        print >>stream, 'const char* const ArmIss::%s_name_table [%d] = {'%(
            self.__name, (1<<self.__width)),
        for i in range(1<<self.__width):
            if i % 4 == 0:
                print >> stream
                print >> stream, "   ",
            print >> stream, ('"%s",'%self.__table.get(i)[3]).ljust(self.__max_name_width+3),
        print >> stream,  '\n};\n'

    def teapot(self, stream, verbose):
        self.print_jumps(tables, verbose)
        self.print_names(tables)
        self.decoding_func(tables)
        self.disasm_func(tables, **self.__fallback)

    def decoding_func(self, stream):
        self.__op_list.add('op_'+self.__name)
        w = 0
        slices_str = []
        slices_code = []
        for l, r in sorted(self.__slices):
            slices_str.append('I[%d:%d]'%(l, r))
            slices_code.append('((m_opcode.ins >> %d) & 0x%x)'%(r-w, ((1<<(l-r+1))-1)<<w))
            w += (l-r+1)
        assert w == self.__width
        print >>stream, '''
/*
  %(ctable)s table is indexed by %(slices_str)s
 */

void ArmIss::%(func)s()
{
    size_t idx = %(slices_code)s;
    func_t f = %(table)s_table[idx];
    (this->*f)();
}

'''%dict(
            ctable = self.__name.capitalize().replace('_', ' '),
            table = self.__name,
            slices_str = ' '.join(reversed(slices_str)),
            func = 'op_'+self.__name,
            slices_code = '\n        | '.join(slices_code))
        return r

    def disasm_func(self, stream, **corres):
        w = 0
        slices_code = []
        for l, r in sorted(self.__slices):
            slices_code.append('((opcode >> %d) & 0x%x)'%(r-w, ((1<<(l-r+1))-1)<<w))
            w += (l-r+1)
        assert w == self.__width
            

        print >>stream, '''

const char * ArmIss::%(func)s(data_t opcode) const
{
    size_t idx = %(slices_code)s;
    const char *r = %(table)s_name_table[idx];
'''%dict(
            table = self.__name,
            func = 'name_'+self.__name,
            slices_code = '\n        | '.join(slices_code))
        for looked_up, func in corres.iteritems():
            print >>stream,'''\
    if ( !strcmp( r, "%s" ) )
        return name_%s(opcode);
'''%(looked_up, func)
        print >>stream,'''\
    return r;
}'''

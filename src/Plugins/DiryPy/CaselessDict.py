# ********************************************
# class     CaselessDict
# purpose   emulate a normal Python dictionary
#           but with keys which can accept the
#           lower() method (typically strings).
#           Accesses to the dictionary are
#           case-insensitive but keys returned
#           from the dictionary are always in
#           the original case.
# ********************************************

class CaselessDict:
    def __init__(self,inDict=None):
        """Constructor: takes conventional dictionary
           as input (or nothing)"""
        self.dict = {}
        if inDict != None:
            for key in inDict:
                k = key.lower()
                self.dict[k] = (key, inDict[key])
        self.keyList = self.dict.keys()
        return

    def clear(self):
        self.dict = {}
        self.keyList = self.dict.keys()

    def __iter__(self):
        self.iterPosition = 0
        return(self)

    def next(self):
        if self.iterPosition >= len(self.keyList):
            raise StopIteration
        x = self.dict[self.keyList[self.iterPosition]][0]
        self.iterPosition += 1
        return x

    def __getitem__(self, key):
        k = key.lower()
        return self.dict[k][1]

    def __setitem__(self, key, value):
        k = key.lower()
        self.dict[k] = (key, value)
        self.keyList = self.dict.keys()

    def has_key(self, key):
        k = key.lower()
        return k in self.keyList

    def __len__(self):
        return len(self.dict)

    def keys(self):
        return [v[0] for v in self.dict.values()]

    def values(self):
        return [v[1] for v in self.dict.values()]

    def items(self):
        return self.dict.values()

    def __contains__(self, item):
        return self.dict.has_key(item.lower())

    def __repr__(self):
        items = ", ".join([("%r: %r" % (k,v)) for k,v in self.items()])
        return "{%s}" % items

    def __str__(self):
        return repr(self)

# *****************************************
# Test Code
# *****************************************

if __name__ == '__main__':
    foundError = False

    # firstly create an empty CaselessDict
    x = CaselessDict()
    x['frEd'] = 76
    x['jOe'] = 92
    x['bERT'] = 54
    x['Bert'] = 53
    if x['bert'] != 53:
        print( "Error 1")
        foundError = True
    shouldBe = [ 'Bert', 'jOe', 'frEd' ]
    for key in x:
        if not key in shouldBe:
            print ("Error 2")
            foundError = True
        else:
            shouldBe.remove(key)
    if len(shouldBe) != 0:
        print ("Error 2a")
        foundError = True
    if not 'frEd' in x:
        print ("Error 3")
        foundError = True
    if not 'fRed' in x:
        print ("Error 4")
        foundError = True
    if 'fReda' in x:
        print ("Error 5")
        foundError = True
    y = x.keys()
    if len(y) != 3:
        print ("Error 6")
        foundError = True
    for yy in y:
        if (yy != 'Bert') and (yy != 'jOe') and (yy != 'frEd'):
            print ("Error 7")
            foundError = True
    if x['FRED'] != 76:
        print ("Error 8")
        foundError = True
    if x['joe'] != 92:
        print ("Error 9")
        foundError = True

    # then create a CaselessDict from an existing dictionary

    y = { 'frEd' : 76, 'jOe' : 92, 'Bert' : 53 }
    x = CaselessDict(y)
    if x['bert'] != 53:
        print ("Error 10")
        foundError = True
    shouldBe = [ 'Bert', 'jOe', 'frEd' ]
    for key in x:
        if not key in shouldBe:
            print ("Error 11")
            foundError = True
        else:
            shouldBe.remove(key)
    if len(shouldBe) != 0:
        print ("Error 11a")
        foundError = True
    if not 'frEd' in x:
        print ("Error 3")
        foundError = True
    if not 'fRed' in x:
        print ("Error 12")
        foundError = True
    y = x.keys()
    if len(y) != 3:
        print ("Error 13")
        foundError = True
    for yy in y:
        if (yy != 'Bert') and (yy != 'jOe') and (yy != 'frEd'):
            print ("Error 14")
            foundError = True
    if x['FRED'] != 76:
        print ("Error 15")
        foundError = True
    if x['joe'] != 92:
        print ("Error 16")
        foundError = True
    if foundError == False:
        print ("No errors found")

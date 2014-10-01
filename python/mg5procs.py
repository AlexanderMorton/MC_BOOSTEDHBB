from mg5common import mg5proc, smheader

procdict = {
"ttbar":
"""\
generate p p > t t~, t > l+ vl b, t~ > l- vl~ b~;
add process p p > t t~, t > l+ vl b, t~ > j j b~;
add process p p > t t~, t > j j b, t~ > l- vl~ b~;
add process p p > t t~, t > j j b, t~ > j j b~
""",

"Wbb":
"""\
generate p p > w+ b b~, w+ > l+ vl $ h;
add process p p > w- b b~, w- > l- vl~ $ h
""",

"Zbb":
"""\
generate p p > z b b~, z > l+ l- $ h;
add process p p > z b b~, z > vl vl~ $ h
""",

"WH":
"""\
generate p p > w+ h, w+ > l+ vl, h > b b~;
add process p p > w- h, w- > l- vl~, h > b b~
""",

"ZH":
"""\
generate p p > z h, z > l+ l-, h > b b~;
add process p p > z h, z > vl vl~, h > b b~
"""
}

for name, cmd in procdict.iteritems():
    procdict[name] = mg5proc(name, smheader + cmd)

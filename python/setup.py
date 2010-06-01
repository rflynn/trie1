from distutils.core import setup, Extension

# there is probably a more idiomatic way of doing this...
import shutil
for f in ['trie1.h','trie1.c']:
	shutil.copy('../'+f, f)

setup(name = "trie1",
      version = "1.0",
      ext_modules = [Extension('trie1', ['trie1.c','trie1py.c'])])

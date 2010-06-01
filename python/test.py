#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from trie1 import trie1
t = trie1()
t.add(u'a')
t.find(u'a')
t.delete(u'a')
t.find(u'a')
t.add(u'a')
t.add(u'ab')
def foo(s,len,_):
	print(s[:len])
t.walk_prefix_strings(u'ab', foo, None)
assert t.all_prefix_strings(u'ab') == [u'a', u'ab']

/* ex: set ts=8 noet: */
/*

Copyright (c) 2010, Ryan Flynn <parseerror@gmail.com> All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Ryan Flynn.

*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "trie1.h"

/*
 * FIXME: recursive; will smash the stack for large strings, but then it'll generate
 * such copious amounts of output it's not practical for large strings anyways...
 */
static void do_trie1_dump(const struct trie1 *t, FILE *f, unsigned level)
{
	/* print node */
	unsigned l = level;
	while (l--) /* indent */
		fputc(' ', f);
	if (!t->c)
		fprintf(f, "\\0");
	else
		fprintf(f, "%lc", t->c);
	fputc('\n', f);
	/* recurse, but not from sub-'\0' nodes */
	if (t->c || 0 == level) {
		if (t->child) do_trie1_dump(t->child, f, level+1);
		if (t->next) do_trie1_dump(t->next, f, level);
	}
}

void trie1_dump(const struct trie1 *t, FILE *f)
{
	do_trie1_dump(t, f, 0);
}

static struct trie1 * trie1_alloc(const wchar_t c)
{
	struct trie1 *t = malloc(sizeof *t);
	t->c = c;
	t->next = NULL;
	t->child = NULL;
	return t;
}

/*
 * convenience function; all nodes should be malloc()ed anyway
 */
struct trie1 * trie1_new(void)
{
	return trie1_alloc(L'\0');
}

/*
 * release all memory held by trie and children, depth first
 */
static void do_trie1_free(struct trie1 *t)
{
	if (t->c) {
		if (t->next) do_trie1_free(t->next);
		if (t->child) do_trie1_free(t->child);
		free(t);
	}
}

void trie1_free(struct trie1 *t)
{
	if (t->next) do_trie1_free(t->next);
	if (t->child) do_trie1_free(t->child);
	if (!t->c) free(t); /* finally free top node */
}

/*
 * find a node from 'parent' associated with 'c' or create one
 */
static struct trie1 * do_trie1_find_or_add(struct trie1 *parent, const wchar_t c)
{
	struct trie1 *s = parent->child;
	if (!parent->child)
		return (parent->child = trie1_alloc(c));
	while (s->c < c && s->next && s->next->c < c)
		s = s->next;
	if (s->c == c)
		return s;
	else if (s->next && s->next->c == c)
		return s->next;
	{
		struct trie1 *n = trie1_alloc(c);
		if (s->c > c) {
			n->next = s;
			if (s == parent->child)
				parent->child = n;
		} else {
			n->next = s->next;
			s->next = n;
		}
		return n;
	}
}

void trie1_add(struct trie1 *t, const wchar_t *str)
{
	struct trie1 *top = t;
	while (*str)
		t = do_trie1_find_or_add(t, *str++);
	t->child = top; /* point all \0 at the same place. brilliant! */
}

int trie1_find(const struct trie1 *t, const wchar_t *str)
{
	if (*str)
		t = t->child;
	do
		while (t && t->c < *str)
			t = t->next;
	while (t && *str && (t->c == *str++) && (t = t->child));
	return t && !t->c; /* didn't run out of nodes; ended on '\0' */
}

/*
 * delete all nodes with trie that apply only to 'str'.
 * more complex than either add or find, because we must ensure the string exists first,
 * and then rewind our way back up the path we came, deleting as many nodes as possible
 * note: non-recursive; handles long strings
 */
int trie1_del(struct trie1 *t, const wchar_t *str)
{
	struct trie1 **op = malloc((wcslen(str) + 1) * sizeof *op),
                     **p = op;
	if (*str)
		*p++ = t, t = t->child;
	/* trace 'str' p through 't', save each step to 'p'... */
	do
		while (t && t->c < *str)
			t = t->next;
	while (t && *str && t->c == *str++ && (*p++ = t) && (t = t->child));
	if (!*str) { /* entire string consumed */
		p--; /* back from \0 to last char */
		/* rewind to the top, deleting any nodes are solely devoted to 'str' */
		while (p > op && (!p[0]->child || (!p[0]->child->c && !p[0]->child->next))) {
			/* re-connect siblings */
			if (p[-1]->child == *p) {
				p[-1]->child = (*p)->next;
			} else {
				t = p[-1]->child;
				while (t->next != *p)
					t = t->next;
				t->next = (*p)->next;
			}
			free(*p);
			p--;
		}
	}
	free(op);
	return !*str;
}

/*
 * traverse 't' by path 'str'; any 'str' prefixes that exist as full strings in
 * 't' will generate a call to (*f)(v, str, len)
 */
void trie1_walk_prefix_strings(const struct trie1 *t, const wchar_t *str,
                               void (*f)(const wchar_t *, size_t, void *pass), void *pass)
{
	const wchar_t *ostr = str;
	if (*str)
		t = t->child;
	do {
		while (t && t->c < *str)
			t = t->next;
		if (t && t->child && !t->child->c) /* is full string */
			f(ostr, (size_t)(str-ostr+1), pass); /* callback */
	} while (t && *str && (t->c == *str++) && (t = t->child));
}

#ifdef DEBUG

static void print_callback(const wchar_t *str, size_t len, void *v)
{
	v = v;
	printf("%.*ls\n", (int)len, str);
}

int main(void)
{
	struct trie1 *t = trie1_new();
	printf("%s test...\n", __FILE__);
	printf("sizeof(struct trie1) = %lu\n", sizeof *t);
	trie1_dump(t, stdout);
	/* add/del */
	trie1_add(t, L"tea");
	assert( trie1_find(t, L"tea"));
	assert( trie1_del(t, L"tea"));
	trie1_dump(t, stdout);
	assert(!trie1_del(t, L"tea"));
	assert(!trie1_find(t, L"tea"));
	trie1_dump(t, stdout);
	/* add/del multiple strings, same prefix */
	trie1_add(t, L"tea");
	trie1_add(t, L"ted");
	trie1_add(t, L"ten");
	trie1_dump(t, stdout);
	assert( trie1_del(t, L"ten"));
	assert(!trie1_del(t, L"ten"));
	trie1_dump(t, stdout);
	assert( trie1_find(t, L""));
	assert(!trie1_find(t, L"t"));
	assert(!trie1_find(t, L"te"));
	assert( trie1_find(t, L"tea"));
	assert( trie1_find(t, L"ted"));
	assert( trie1_del(t, L"ted"));
	trie1_dump(t, stdout);
	assert( trie1_del(t, L"tea"));
	trie1_dump(t, stdout);
	trie1_free(t);
	/* try prefix string search */
	t = trie1_new();
	trie1_add(t, L"he");
	trie1_add(t, L"hell");
	trie1_add(t, L"hello");
 	trie1_walk_prefix_strings(t, L"hello", print_callback, NULL);
 	trie1_walk_prefix_strings(t, L"foobar", print_callback, NULL);
	trie1_free(t);
	/* test long string */
	{
		size_t len = 1024 * 1024;
		wchar_t *longstring = malloc(len * sizeof *longstring),
			*tmp = longstring;
		while (len--)
			*tmp++ = L'A';
		*tmp = L'\0';
		t = trie1_new();
		trie1_add(t, longstring);
		trie1_del(t, longstring);
		trie1_free(t);
	}
	printf("test passed.\n");
	return 0;
}

#endif


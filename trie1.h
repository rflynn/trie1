/* ex: set ts=8 noet: */
/*
 * Unicode-friendly string prefix trie data structure.
 * Supports basic add/del/find; also some more unusual ones I needed specifically.
 *
 * TODO:
 *	- count_strings() 
 *	- walk_strings() run callback on all strings
 *	- replace malloc/free with a slab allocator to speed this puppy up
 */

#include <wchar.h>

struct trie1 {
	wchar_t c;
	struct trie1 *next,
		    *child;
};

struct trie1 * trie1_new (void);
void           trie1_free(struct trie1 *);
void           trie1_add (struct trie1 *,       const wchar_t *);
int            trie1_find(const struct trie1 *, const wchar_t *);
int            trie1_del (struct trie1 *,       const wchar_t *);
void           trie1_walk_prefix_strings(const struct trie1 *t, const wchar_t *str,
                                         void (*f)(const wchar_t *, size_t len, void *pass), void *pass);


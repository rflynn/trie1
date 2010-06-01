/* ex: set ts=8 noet: */
/*
 * trie1 python bindings
 * reference: http://starship.python.net/crew/arcege/extwriting/pyext.html
 *            http://docs.python.org/release/2.5.2/ext/callingPython.html
 *            http://www.fnal.gov/docs/products/python/v1_5_2/ext/buildValue.html
 */

#include <Python.h>
#include "trie1.h"

/*
 * trie1 obj PyObject wrapper
 */
typedef struct {
	PyObject_HEAD
	struct trie1 *t;
} trie1py;

static void      trie1py_dealloc(PyObject *self);
static PyObject *trie1py_getattr(PyObject *self, char *attr);
static int       trie1py_print  (PyObject *self, FILE *fp, int flags);

/*
 * trie1py type-builtin methods
 */
PyTypeObject trie1py_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"trie1",		/* char *tp_name;                           */
	sizeof(trie1py),	/* int tp_basicsize;                        */
	0,			/* int tp_itemsize; not used much           */
	trie1py_dealloc,	/* destructor tp_dealloc;                   */
	trie1py_print,		/* printfunc tp_print;                      */
	trie1py_getattr,	/* getattrfunc tp_getattr;	__getattr__ */
	0,			/* setattrfunc tp_setattr;	__setattr__ */
	0,			/* cmpfunc tp_compare;		__cmp__     */
	0,			/* reprfunc tp_repr;		__repr__    */
	0,			/* PyNumberMethods *tp_as_number;           */
	0,			/* PySequenceMethods *tp_as_sequence;       */
	0,			/* PyMappingMethods *tp_as_mapping;         */
	0,			/* hashfunc tp_hash;		__hash__    */
	0,			/* ternaryfunc tp_call;		__call__    */
	0,			/* reprfunc tp_str;		__str__     */
};

static PyObject *trie1py_new                (PyObject *self, PyObject *args);
static PyObject *trie1py_add                (PyObject *self, PyObject *args);
static PyObject *trie1py_find               (PyObject *self, PyObject *args);
static PyObject *trie1py_del                (PyObject *self, PyObject *args);
static PyObject *trie1py_walk_prefix_strings(PyObject *self, PyObject *args);
static PyObject *trie1py_all_prefix_strings (PyObject *self, PyObject *args);

/*
 * custom trie1py methods
 */
static PyMethodDef trie1py_Methods[] = {
	{"add",			trie1py_add,			METH_VARARGS,	""  },
	{"delete",		trie1py_del,			METH_VARARGS,	""  },
	{"find",		trie1py_find,			METH_VARARGS,	""  },
	{"__contains__",	trie1py_find,			METH_VARARGS,	""  },
	{"all_prefix_strings",	trie1py_all_prefix_strings,	METH_VARARGS,	""  },
	{NULL,			NULL,				0,		NULL}
};

static PyMethodDef module_Methods[] = {
	{"trie1",	trie1py_new,	METH_VARARGS,	""  },
	{NULL,		NULL,		0,		NULL}
};

PyMODINIT_FUNC inittrie1(void)
{
	(void) Py_InitModule("trie1", module_Methods);
}

PyObject *trie1py_getattr(PyObject *self, char *attr)
{
	PyObject *res = Py_FindMethod(trie1py_Methods, self, attr);
	return res;
}

static PyObject * trie1py_NEW(void)
{
	trie1py *obj = PyObject_NEW(trie1py, &trie1py_Type);
	if (obj)
		obj->t = trie1_new();
	return (PyObject *)obj;
}

static PyObject * trie1py_new(PyObject *self, PyObject *args)
{
	PyObject *obj = trie1py_NEW();
	Py_INCREF(obj);
	return obj;
}

static void trie1py_dealloc(PyObject *self)
{
	trie1py *pt = (trie1py *)self;
	trie1_free(pt->t);
	PyMem_DEL(self);
}

static int trie1py_print(PyObject *self, FILE *fp, int flags)
{
	trie1py *pt = (trie1py *)self;
	trie1_dump(pt->t, fp);
	return 0;
}

static PyObject *trie1py_add(PyObject *self, PyObject *args)
{
	PyObject *res = NULL;
	trie1py *obj = (trie1py *)self;
	wchar_t *s;
	if (PyArg_ParseTuple(args, "u", &s)) {
		trie1_add(obj->t, s);
		res = Py_None;
		Py_INCREF(res);
	}
	return res;
}

static PyObject *trie1py_find(PyObject *self, PyObject *args)
{
	PyObject *res = NULL;
	trie1py *obj = (trie1py *)self;
	wchar_t *s;
	if (PyArg_ParseTuple(args, "u", &s)) {
		res = trie1_find(obj->t, s) ? Py_True : Py_False;
		Py_INCREF(res);
	}
	return res;
}

static PyObject *trie1py_del(PyObject *self, PyObject *args)
{
	PyObject *res = NULL;
	trie1py *obj = (trie1py *)self;
	wchar_t *s;
	if (PyArg_ParseTuple(args, "u", &s)) {
		res = trie1_del(obj->t, s) ? Py_True : Py_False;
		Py_INCREF(res);
	}
	return res;
}

/*
 * append all s[:len] to list in pass
 */
static void cb_all_prefix_strings(const wchar_t *s, size_t slen, void *pass)
{
	PyObject *list = pass,
	         *args = Py_BuildValue("u#", s, (int)slen);
	if (args) {
		(void)PyList_Append(list, args);
		Py_DECREF(args);
	}
}

/*
 * return a list of all trie entries that match a prefix of a string
 */
static PyObject *trie1py_all_prefix_strings(PyObject *self, PyObject *args)
{
	PyObject *list = PyList_New(0);
	trie1py *obj = (trie1py *)self;
	wchar_t *s;
	if (PyArg_ParseTuple(args, "u", &s)) {
		trie1_walk_prefix_strings(obj->t, s, cb_all_prefix_strings, list);
		Py_INCREF(list);
	}
	return list;
}


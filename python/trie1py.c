/* ex: set ts=8 noet: */
/*
 * trie1 python c module wrapper
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

static void      trie1py_dealloc            (PyObject *self);
static PyObject *trie1py_getattr            (PyObject *self, char *attr);

PyTypeObject trie1py_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"trie1",		/* char *tp_name;                           */
	sizeof(trie1py),	/* int tp_basicsize;                        */
	0,			/* int tp_itemsize; not used much           */
	trie1py_dealloc,	/* destructor tp_dealloc;                   */
	0,//trie1py_print,	/* printfunc tp_print;                      */
	trie1py_getattr,	/* getattrfunc tp_getattr;	__getattr__ */
	0,//trie1py_setattr,	/* setattrfunc tp_setattr;	__setattr__ */
	0,//trie1py_compare,	/* cmpfunc tp_compare;		__cmp__     */
	0,//trie1py_repr,	/* reprfunc tp_repr;		__repr__    */
	0,//&trie1py_as_number,	/* PyNumberMethods *tp_as_number;           */
	0,			/* PySequenceMethods *tp_as_sequence;       */
	0,			/* PyMappingMethods *tp_as_mapping;         */
	0,//trie1py_hash	/* hashfunc tp_hash;		__hash__    */
	0,			/* ternaryfunc tp_call;		__call__    */
	0,//trie1py_str,	/* reprfunc tp_str;		__str__     */
};

static PyObject *trie1py_new                (PyObject *self, PyObject *args);
static PyObject *trie1py_add                (PyObject *self, PyObject *args);
static PyObject *trie1py_find               (PyObject *self, PyObject *args);
static PyObject *trie1py_del                (PyObject *self, PyObject *args);
static PyObject *trie1py_walk_prefix_strings(PyObject *self, PyObject *args);

static PyMethodDef trie1py_Methods[] = {
	{"add",			trie1py_add,			METH_VARARGS,	""  },
	{"delete",		trie1py_del,			METH_VARARGS,	""  },
	{"find",		trie1py_find,			METH_VARARGS,	""  },
	{"walk_prefix_strings",	trie1py_walk_prefix_strings,	METH_VARARGS,	""  },
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
	Py_INCREF(obj); /* make us not crash(!) */
	return obj;
}

static void trie1py_dealloc(PyObject *self)
{
	trie1py *pt = (trie1py *)self;
	trie1_free(pt->t);
	PyMem_DEL(self);
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
		if (trie1_find(obj->t, s))
			res = Py_True;
		else
			res = Py_False;
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
		if (trie1_del(obj->t, s))
			res = Py_True;
		else
			res = Py_False;
		Py_INCREF(res);
	}
	return res;
}

static PyObject *tmp_callback = NULL;

/*
 * this is the C wrapper for our Python callback
 */
static void cb_walk_prefix_strings(const wchar_t *s, size_t slen, void *pass)
{
	PyObject *args = NULL;
	args = Py_BuildValue("(ulO)", s, (long)slen, pass);
	if (!args) {
		PyErr_SetString(PyExc_TypeError, "callback parameters must be (str,int,pass)");
		return;
	}
	(void)PyEval_CallObject(tmp_callback, args);
	Py_DECREF(args);
}

static PyObject *trie1py_walk_prefix_strings(PyObject *self, PyObject *args)
{
	// void trie1_walk_prefix_strings(const struct trie1 *t, const wchar_t *str,
        //                               void (*f)(const wchar_t *, size_t len, void *pass), void *pass);
	PyObject *res = NULL,
	         *pass = NULL;
	trie1py *obj = (trie1py *)self;
	wchar_t *s;
	/* parse parameters, write callback to static global... */
	if (PyArg_ParseTuple(args, "uOO", &s, &tmp_callback, &pass)) {
		if (!PyCallable_Check(tmp_callback)) {
			PyErr_SetString(PyExc_TypeError, "parameter must be callable");
			return NULL;
		}
		Py_INCREF(tmp_callback);
		trie1_walk_prefix_strings(obj->t, s, cb_walk_prefix_strings, pass);
		Py_DECREF(tmp_callback);
		res = pass;
		Py_INCREF(res);
		Py_DECREF(pass);
	}
	return res;
}


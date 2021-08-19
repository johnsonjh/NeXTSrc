#if 0
extern "Objective-C"
{
#import <objc/Object.h>
#import <objc/hashtable.h>
}
#endif

/*
 * 	bugs:
 *
 *	- providing a return type for a constructor causes the
 *	compiler to crash.
 */
	
int function(char *arg)
{
	printf("%s: from no module\n",arg);
}

struct my_module
{
	int data;

	int function(char *arg)
	{
		printf("%s: from my module\n",arg);
	}
};

struct your_module
{
	int data;

	int function(char *arg)
	{
		printf("%s: from your module\n",arg);
	}
};

void simple()
{
	my_module my_mod;
	your_module your_mod;

	function("hello world");
	my_mod.function("hello world");
	your_mod.function("hello world");
}


/*
 *	member functions...associating a set of operations with a 
 *	particular data type. Member functions can be defined inside
 *	or outside the context of a structure body.
 */
class date 
{
	friend class foof;

private:
	/* data */

	int month, day, year;

	void get(int *, int *, int *);

protected:

	int protected_data;

public: 
	/* member functions */

	void set(int m, int d, int y)
	{
		month = m, day = d, year = y; 
		get(&m, &d, &y);
	}

	/* friend functions */

	friend int friend_of_date(date *pd);
};

void date::get(int *m, int *d, int *y)
{
	*m = month, *d = day, *y = year;
}

int friend_of_date(date *pd)
{
	return pd->month;
}

void member_functions()
{
	date my_bday;

	my_bday.set(3,16,61);
	/*{
		int a,b,c;

		my_bday.protected_data;
		my_bday.get(&a,&b,&c);
		my_bday.month;
	}*/
	friend_of_date(&my_bday);
}

/*
 *	function overloading...`overload'.
 */
overload function_overloading;

char *function_overloading(char *str)
{
	printf("%s\n",str);
	return str;
}

int function_overloading(int i)
{
	printf("%d\n",i);
	return i;
}

float function_overloading(float f)
{
	printf("%f\n",f);
	return f;
}

void function_overloading()
{
	function_overloading("abc");
	function_overloading(7);
	function_overloading(6.7);
}

class private_class
{
	int a;

	private_class() { printf("in private class...\n"); }
public:
	friend void test_private_class();
};

void test_private_class()
{
	private_class aprivateclass;
}

/*
 *	virtual functions...dynamic binding in c++...`exact' class in unknown
 *	by the compiler, the exact method is...
 *
 *	the virtual tables are named "_vt$<class name>" by the compiler.
 */

class virtual_base
{
public:
	virtual void dynamically_bound() { printf("in virtual_base\n"); }
	void statically_bound() { printf("in virtual_base\n"); }
};

class derived_from_virtual1 : public virtual_base
{
public:
	void dynamically_bound() { printf("in derived_from_virtual1\n"); }
	void statically_bound() { printf("in derived_from_virtual1\n"); }
};

class derived_from_virtual2 : public virtual_base
{
public:
	void dynamically_bound() { printf("in derived_from_virtual2\n"); }
	void statically_bound() { printf("in derived_from_virtual2\n"); }
};


void test_virtual(virtual_base *vir)
{
	_vt$virtual_base[0];
	vir->dynamically_bound();
	vir->statically_bound();
}

void virtual_functions()
{
	test_virtual(new virtual_base);
	test_virtual(new derived_from_virtual1);
	test_virtual(new derived_from_virtual2);
}

/* 
 *	operator overloading...`operator'.
 */
class vector
{
	int *vec;
	int size;
public:
	vector(int);			// constructor.
	~vector();			// destructor.

	int& operator [] (int);		// access operator.
};

vector::vector(int s)
{
	if (s<=0) printf("bad vector size\n");
	size = s;
	vec = new int[s];
	for (int i = 0; i < s; i++)
		vec[i] = i;
}

vector::~vector(int s)
{
	delete vec;
}

int& vector::operator [] (int i)
{
	if (i < 0 || size <= i) {
		printf("vector out of range\n");
		return (int &)0;
	} else
		return vec[i];
}

test_vec()
{
	vector v1(5);

	printf("v[4] = %d v[5] = %d\n", v1[4], v1[5]);
}

/* 
 *	function argument defaults...this is only possible for trailing 
 *	arguments.
 */
void arg_defaults(int a = 7, int b = 8)
{
	printf("a = %d b = %d\n",a,b);
}

void function_argument_defaults()
{
	arg_defaults();
	arg_defaults(1000);
	arg_defaults(1000, 2000);
}

/* 
 *	scope resolution operator...`::'. Used for:
 *
 *	- member name qualification.
 *	- global name qualification.
 */
struct base 
{
public:
	int a, b;
};

class derived : public base 
{
public:
	int b, c;

	void printf() { ::printf("%d %d %d %d\n",a,base::b, b, c); }
};

member_scopes()
{
	derived d;

	d.a = 1;
	d.base::b = 2;
	d.b = 3;
	d.c = 4;
	d.printf();
}

class constructors_destructors
{
public:
	char *name;
	int a;

	constructors_destructors() 
	{ 
		printf("this = %x\n",this);
		a = 1000;
		name = "no args";
		printf("constructing %s = %d\n",name,a); 
	}

	constructors_destructors(char *n) 
	{ 
		printf("this = %x\n",this);
		a = 1000;
		name = n;
		printf("constructing %s = %d\n",name,a); 
	}

	constructors_destructors(char *n, int b) 
	{ 
		printf("this = %x\n",this);
		a = b;
		name = n;
		printf("constructing %s = %d\n",name,a); 
	}

	/* destructors can not have parameters */

	~constructors_destructors() 
	{ 
		printf("destructing %s\n",name); 
	}
	
	void print() { printf("a = %d\n", a); }
};

constructors_destructors global_static_obj1("global_static_obj1");

typedef struct {
	int a;
	constructors_destructors field; /* ??? */
} tricky;

tricky wicky;

cons_dest()
{
	constructors_destructors static_obj1("stack_obj1");
	constructors_destructors static_obj2("stack_obj2", 77);
	constructors_destructors static_objAry("static_objAry")[7];

	constructors_destructors *dynamic_obj1;
	constructors_destructors *dynamic_obj2;

 	dynamic_obj1 = new constructors_destructors("dynamic_obj1");
	dynamic_obj2 = new constructors_destructors("dynamic_obj2", 77);

	static_obj1.print();
	static_obj2.print();
	dynamic_obj1->print();
	dynamic_obj2->print();

	delete dynamic_obj1;
	delete dynamic_obj2;
}

/*
 *	static members...sharing data amongst all instances of a class.
 *	function members are not permitted to be declared static.
 */
struct static_members
{
	static char *shared_data;
	char *instance_data;

	static_members(char *name) 
	{ 
		shared_data = "none";
		instance_data = name; 
	}
	print()
	{
		printf("shared data = %s instance data = %s\n",
			shared_data, instance_data);
	}
};

void stat_mems()
{
	static_members a("a"), b("b"), c("c");

	a.print(), b.print(), c.print();
	c.shared_data = "this is shared data";
	a.print(), b.print(), c.print();
}

/*
 *	statically allocated objects within objects!
 */
class point
{
public:
	int x, y;

	point() { printf("in points constructor\n"); }
	~point() { printf("in points destructor\n"); }
/*
 *	look at `this'...
 */
	point one() { printf("one\n"); return *this; }
	point two() { printf("two\n"); return *this; }
	point three() { printf("three\n"); return *this; }
};

class rect
{
public:
	point lower_left;
	point upper_right;

	rect() { printf("in rect constructor\n"); }
	~rect() { printf("in rect destructor\n"); }
};

stack_allocated_objects_within_objects()
{
	rect foo;

	foo.lower_left.x = 7, foo.lower_left.y = 8;
	foo.upper_right.x = 14, foo.upper_right.y = 15;
}

/*
 * 	reference variables...specifies an alternative name for an object.
 *	This is commonly used to pass back information from a function.
 */
ref_ex(int &pass_by_ref, int pass_by_val)
{
	pass_by_ref = 77;
	pass_by_val = 77;
}

reference_variables()
{
	int a = 0, b = 0;

	ref_ex(a,b);
	printf("a = %d, b = %d\n",a,b);
}
#if 0
@interface Object(you_cant_do_this_in_cplusplus)

- pass_by_ref:(int &)aref pass_by_val:(int)aval;
- reference_variables;

@end

@implementation Object(you_cant_do_this_in_cplusplus)

+ new
{
	printf("in `category' for new\n");
	return class_createInstance((Class)self, 0);
}

- pass_by_ref:(int &)aref pass_by_val:(int)aval
{
	aref = 77;
	aval = 77;
}

- reference_variables
{
	int a = 0, b = 0;

	[self pass_by_ref:a pass_by_val:b];
	printf("a = %d, b = %d\n",a,b);
}

@end

#import <stdarg.h>

@implementation Rect : Object
{
	point *lower_left;
	point *upper_right;
}

- varargs:(int) n, ...
{
	va_list vp;

	va_start(vp, n);
	for (int i = 0; i < n; i++)
		printf("arg %d = %d ", i, va_arg(vp, int));
	printf("\n");
	return self;
}

- cascaded_messages
{
	point aPoint;

	aPoint.one().two().three();
}

@end
#endif
main()
{
#if 0
	NXHashTable *objc_classes = objc_getClasses();
	NXHashState state = NXInitHashState(objc_classes);
	id aclass, anObject;

	while (NXNextHashState(objc_classes, &state, (void **)&aclass))
		printf("class name = %s\n",[aclass name]);

	anObject = [Object new];
#endif
	simple();
	member_functions();
	function_overloading();
	test_vec();
	function_argument_defaults();
	member_scopes();
	cons_dest();

	reference_variables();
#if 0
	[anObject reference_variables];
#endif

	stat_mems();
	stack_allocated_objects_within_objects();
#if 0
	[[[Rect new] varargs:7, 0, 1, 2, 3, 4, 5, 6] cascaded_messages];
#endif
	virtual_functions();
	test_private_class();
}

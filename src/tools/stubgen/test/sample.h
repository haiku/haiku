/* stubgen sample test file */

#include <string.h>

class Bar { public: Bar(int i) {} };
class sdfs { int i; };

#define macro(x) MultiLineMacro(x)

#define MultiLineMacro(x) 	bar(x); \
	quux(x);   

#define BUFSIZE 255
#define i3 3



#if defined(__EXTENSIONS__) || ((__STDC__ == 0 && \
		!defined(_POSIX_C_SOURCE)) || defined(_XOPEN_SOURCE))
extern int foo;
int j = foo;
#endif

int arry[] = {
  0, 1, 2, 3, 4
};

class fwd_decl_clazz;


typedef struct oldschool {
  int iOldSchool;
  double dOldSchool;
#define BAR(x) \
	bar(x); \
	quux(x);   
  int i2; char c2;
} oldschool_t;

typedef int stubelem_t;

/*     template <class K, class V> */

class Foo : public Bar, sdfs {
protected:
  char buffer[BUFSIZE + 1];
  int dumfunc(int a[BUFSIZE+i3]);

public:
  void boundary_condition() {};
  Foo(int a);
  Foo(double d) : Bar(23), a_(d) { int a = 34; }
  
  int * const quux(int a);
  int i2(); char c2(...);
  int iOldSchool2; /*
here's a comment
that
#define foo bar() \
   sd
might be hard
to parse
 */
  virtual void pure_virtual() = 0;
  void not_pure_virtual();
  void elipsis(int iElip, char cElip, float fElip, ...);

  class Quux  {
      char c_;
      char *pc_;
    char charfunc(char c1, char c2, char c3, char c4, char c5);
    char& cool_func(char, short s, int, long l, long[]);
    void funcA(int a, int b, int c = 33);
    void* funcB(int a, int b = 2, int c = 23) const;
    void * funcC(int a = 0, int b = 2, int c = 23) { return 0; }
    char *& funcD(char * w, int a = 0, int b = 2, int c = 23);
    void proto(int, char);
    Quux(int z = 0);
    ~Quux();
	
    int test;
    void test0() {};
    Bar test2(const int &i) const;
    long long test3()  const  throw(int, Foo::Quux, float);
    void test4() {}
    void test5(int a) {};

    struct NestMe {
      public:
      NestMe(int foo, int bar, int hi_there);
      int getVal(stubelem_t *whee) throw(int, Foo::Quux, float);
      int getFoo() throw(int, float) { return 1; }
    };
      int getBar() throw(int, float);
  };

  virtual double toto(double &d = 1.0, int i = 4);
  /* the next line won't be expanded because it's inlined. */
  const int getValue() { return a_; }

private:
    int a_;
};

class WidgetLens {
 public:
    WidgetLens();
};

class WidgetCsg : public WidgetLens {
 protected:
 
 public:
     virtual ~WidgetCsg() {}
              WidgetCsg();
};


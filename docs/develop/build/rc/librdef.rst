The librdef library
===================

Of course, it would be cool if other applications (such as GUI resource editors) could also import
and export rdef files. That is why the bulk of rc's functionality is implemented in a separate
shared library, librdef.so.

Using the library in your own applications is very simple. Here are some quick instructions to get
you started:

1. ``#include "rdef.h"`` in your sources
2. link your app to librdef.so

The API is rather bare-bones, but it gets the job done. The library uses files to transfer data to
and from your application. This may seem odd, but it is actually a big advantage. After calling the
API functions to compile an rdef file, you can use the standard BResources class to read the
resources from the output file. Chances are high that your application already knows how to do this.

To compile a resource file, the steps are typically this:


1. Call ``rdef_add_include_dir()`` one or more times to add include file search paths.
2. Call ``rdef_add_input_file()`` one or more times to add the rdef files that you want to compile.
3. Call ``rdef_set_flags()`` to toggle compiler options.
4. Call ``rdef_compile()`` with the name of the output file. This performs the actual compilation.
5. Call ``rdef_free_input_files()`` to clear the list of input files that you added earlier.
6. Call ``rdef_free_include_dirs()`` to clear the list of include directories that you added earlier.

Decompiling is very similar, although include directories are not used here:

1. Call ``rdef_add_input_file()`` one or more times to add the resource files that you want to decompile.
2. Call ``rdef_set_flags()`` to toggle compiler options.
3. Call ``rdef_decompile()`` with the name of the output file. The name of the header file (if any) will be automatically constructed by appending ".h" to the output file name.
4. Call ``rdef_free_input_files()`` to clear the list of input files that you added earlier.

If one of these functions returns something other than B_OK, an error occurred. You can look at the
following variables to find out more about the error, and construct meaningul error messages:

rdef_err
    The error code that was returned.

rdef_err_line
    The line number where compilation failed.

rdef_err_file
    The file where the error occurred.

rdef_err_msg
    The error message from the compiler.

For more information about using librdef, see "rdef.h", which explains the available functions and
data structures in more depth. For a real-world example, take a look at "rc.cpp", which contains
the complete implementation of the rc compiler. As you'll see, there really isn't much to it,
because librdef already does all the work.

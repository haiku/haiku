Using the JSP and servlet examples
==================================

Before using the examples you must either copy the relevant
image and PDF files to an appropriate location, or modify
the paths in the source code to point to the sample files.

Note: If you find corrections to the instructions below, or
can contribute configuration hints for other environments
we'll be happy to hear from you -- please send all suggestions
to support@pdflib.com.


Exception: PDFlib already loaded in another classloader
=======================================================

When using PDFlib (on any platform) with an application server you
may encounter the following exception:

java.lang.UnsatisfiedLinkError: Native Library
C:\WINNT\system32\pdf_java.dll already loaded in another classloader

This is caused by multiple classloaders trying to load the PDFlib class,
and therefore the PDFlib native libary, into the server's address space
simultaneously. The problem can be avoided by putting pdflib.jar into
the global class path instead of the classpath of the web application.
The PDFlib shared library/DLL should still be placed in an accessible
system directory.

For example, for Tomcat consider the following directories for pdflib.jar: 
bad:	WEB-INF/lib
good:	$CATALINA_HOME/shared/lib


Using PDFlib with IBM Visual Age for Java
==========================================

- Add pdflib.jar to your project.

- Place pdf_java.dll in the \winnt\system32 directory, or some 
  other directory contained in the PATH environment variable.

- Relative pathnames in PDFlib function calls are interpreted relative
  to the java bin directory, not the project directory. For this reason
  the output of the supplied PDFlib samples may end up in some strange
  place. Absolute path names work.


Using PDFlib with Borland JBuilder
==================================

- Having pdflib.java in the same directory as the project file
  seems to confuse JBuilder.

- On Windows pdf_java.dll must be placed in the \winnt\system32 directory,
  or some other directory contained in the PATH environment variable.
  On Linux libpdf_java.so should be placed in /usr/lib or a similar
  well-known (to the system) directory.

- Relative pathnames in PDFlib function calls are interpreted relative
  to the JBuilder bin directory, not the project directory.  For this
  reason the output of the supplied PDFlib samples may end up in some
  strange place. Absolute path names work.


Using PDFlib with MacroMedia ColdFusion MX
==========================================

We recommend the PDFlib Java edition (instead of the COM edition) for
use with ColdFusion MX. Install the PDFlib Java edition by extracting
pdflib.jar and pdf_java.dll into a folder defined in ColdFusion MX's 
Java classpath and the system's PATH variable (might as well just use 
C:/CFusionMX/lib). If you previously used the PDFlib COM edition
with ColdFusion you have to change the <cfobject> tag parameters in
the CF file as follows:

<cfobject type="Java" name="oPDF" class="com.pdflib.pdflib" action="create">


Using PDFlib with Allaire JRun
==============================

In order to use PDFlib with JRun the following is suggested:

- Copy PDFlib.jar and lib_java.dll to the .../JRun/servers/lib directory.

- Make sure that the lib directory is contained in the servlet.jnipath
  property. This property can be set from the management console, or
  specified in a local.properties file.


Using PDFlib with J2EE-compliant servers
========================================

In order to use PDFlib with J2EE do the following:

- Add the following lines in the file $J2EE_HOME/lib/security/server.policy
  in order to allow the PDFlib native library to be loaded:

  // default permissions granted to all domains
  grant {
          permission java.lang.RuntimePermission "loadLibrary.*";
          permission java.lang.RuntimePermission "accessClassInPackage.*";
          ...

- Use the deployment tool to add pdflib.jar to the project as an external
  jar library.
  It seems that pdflib.jar must be placed in the %J2EE_HOME%/lib directory;
  otherwise the server will attempt to load the PDFlib shared library
  multiply, resulting in the error message "shared library already loaded".

- pdf_java.dll or libpdf_java.so must be accessible in some system path,
  e.g. \winnt\system32 or /usr/local/lib, or via PATH/LD_LIBRARY_PATH.

- The PATH and CLASSPATH variables should point to the bin subdirectories
  of J2EE and JDK.


Using PDFlib with Tomcat
========================

In order to use PDFlib with Apache Tomcat do the following:

- Put the PDFlib shared library or DLL (e.g. pdf_java.dll) into a directly
  which is accessible systemwide (e.g. C:\winnt\system32).

- It is suggested to load PDFlib as early as possible. Loading PDFlib later
  in the servlet (e.g., via repositories) may fail due to different
  class loaders.

- Put pdflib.jar into
  $CATALINA_HOME/common/lib (Tomcat 3) or
  $CATALINA_HOME/shared/lib (Tomcat 4).

- Add a line similar to the file tomcat.properties:

    wrapper.env="/usr/local/lib:/usr/lib:/lib"

- After building or installing PDFlib it may help to update the shared
  library cache via

  ldconfig -v		(or similar command depending on your Unix version)



Using PDFlib with IBM WebSphere Application Server
==================================================

The instructions in this section apply to WebSphere 3 and 4.
For WebSphere 5 please refer to the section above on J2EE-compliant
servers.

Servlets are loaded with a custom class loader. For this reason,
the pdflib.jar file has to be located in the Application Server's
classpath rather than the web application's classpath.  

To locate the pdflib.jar file in the app server's classpath, place the jar 
file in the \<WAS app-server's path>\lib directory and edit the 
admin.config file.  In the admin.config file add the path to the jar file 
to the setting labeled:

com.ibm.ejs.sm.adminserver.classpath

If you use the Websphere Application Assembly Tool you can add pdflib.jar
to your project. 

The DLL or .so must be located somewhere on the machine's path.
The winnt\system32 directory works for Windows, the bin directory
of WAS on Solaris.

On the AS/400 make sure that the PDFLIB and PDF_JAVA SRVPGM can be
found in the library list of the jobs running your Java apps. The
easiest way to achieve this is to copy these SRVPGMs to the QGPL
library. In most cases this library is found in every LIBL.


Using PDFlib on Mac OS X
========================

On Mac OS X the shared PDFlib library for Java, which has a default file
name of libpdf_java.dylib, must be renamed to libpdf_java.jnilib. The
PDFlib build process does this automatically.

The default search path for JNI libraries on OS X is as follows:

    .:/usr/lib:/usr/lib/java:/lib:
    /System/Library/Frameworks/JavaVM.framework/Versions/1.2/Libraries

You can extend the JNI search path by defining the DYLD_LIBRARY_PATH
with more directories.


Using PDFlib with Apple WebObjects
==================================

To the best of our knowledge, the following should be sufficient in order
to use PDFlib with WebObjects:

- Use the java-framework-maker tool to create a new WebObjects Framework,
  and select PDFlib.jar as input package.

- Add the new framework to your project.


Using PDFlib with Lotus Notes
=============================

We don't have first-hand experience with deploying PDFlib in Notes.
However, a user contributed the following information:

> Apparently when you compile the Java code and then you run the code, Domino
> is not using the same paths.  To compile the code you need to add the
> pdflib.jar to the project.  This can be done by clicking the Edit Project
> button and browsing to the directory where the file is located and then
> adding the file to the project.  This will allow you to compile the code
> without errors.  If you are a programmer you will assume that since the
> code compiled therefore Domino knows where the Java class resides but you
> know what happens when you assume.  The second thing that you need to do is
> create a "\com\pdflib" directory within your Notes directory and put the
> pdflib.class file in that directory.  When you specify "import
> com.pdflib.pdflib" this is where Java will go to look for the class
> then it executes it.  I found documentation that said that you can add a
> path my adding a line in the notes.ini file "JavaUserClasses=yourpath" but
> it did not work.


Using PDFlib on SGI IRIX
========================

Use the LD_LIBRARYN32_PATH environment variable (as opposed to the
traditional LD_LIBRARY_PATH) in order to set the directory where the
PDFlib shared library for Java will be found.

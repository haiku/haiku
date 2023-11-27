Using an IDE
===============

Live Code Analysis
------------------
While developing the Haiku codebase, it is possible to generate an inventory of compile commands
by passing the **-c** flag to jam. The resulting **compile_commands.json** document will assist
tools such as clangd_ in live analyzing our source code.

.. code-block::
  :caption: Generating the **compile_commands.json** for clangd:

  $ mkdir generated.clangd && cd generated.clangd
  $ configure --build-cross-tools x86_64 --cross-tools-source ../../buildtools
  $ jam -anc @nightly-anyboot
  $ ln -sr compile_commands.json ../compile_commands.json


When you use an editor which supports clangd_ such as helix_ or vim_ (with the clangd plugin),
clangd_ will have knowledge of the relevant include paths and build flags for every source file
within the Haiku codebase.

.. TIP::
   When new source code files are added, you will need to run though the process above for a
   complete database.

.. _helix: http://helix-editor.com
.. _vim:   http://vim.org
.. _clangd: http://clangd.llvm.org

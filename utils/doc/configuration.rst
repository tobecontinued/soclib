
.. _soclib.conf:

===================
Configuration files
===================

Overview
========

SoCLib's configuration file is parsed by :py:mod:`soclib_cc.config`,
and is used by other modules:

* :py:mod:`soclib_desc` uses the list of module "search paths".

* :py:mod:`soclib_builder` uses the definition of compilers, libraries
  and build environments.

* finally :ref:`soclib-cc` uses it to set default environment and
  command-line visual feedback (verbosity, etc.).

.. _config-paths:

Configuration file paths
========================

SoCLib configuration module will set default parameters, then look for
the following files, in order:

* ``utils/conf/soclib.conf`` in soclib installation directory,
* ``~/.soclib/global.conf`` in user home directory,
* ``./soclib.conf`` in current directory.

All configuration items defined in these files override previously set
items. Therefore the ``soclib.conf`` file in current directory can
override everything else.

Any of these files can be absent, this will be silently ignored.


Configuration file format
=========================

SoCLib configuration files are:

* Python-parseable. All python types and indentation rules apply,

* made of assignments to the ``config`` variable and its attributes,

* only the ``config`` variable is evaluated.

Configuration attributes
------------------------

Some attributes are reserved for special meanings, all other are
considered to be `build environment specifiers`_.

.. attribute:: config.verbose

   Boolean. Whether to print each performed action of
   :ref:`soclib-cc`, overridden by :option:`soclib-cc -v`.

.. attribute:: config.quiet

   Boolean. Whether to print as few as possible from
   :ref:`soclib-cc`, overridden by :option:`soclib-cc -q`.

.. attribute:: config.debug

   Boolean. Whether to print debug messages for all actions performed
   by :ref:`soclib-cc`, overridden by :option:`soclib-cc -d`.

.. attribute:: config.progress_bar

   Boolean. Whether to print :ref:`soclib-cc` progress as a
   progress-bar, overridden by :option:`soclib-cc -P`.

.. attribute:: config.max_name_length

   Integer. Filesystem limitation workaround. Sets the maximal file
   name length for temporary files in :ref:`tmp-spool`.

.. attribute:: config.mode

   String. Current compilation mode, overridden by :option:`soclib-cc -m`.

.. attribute:: config.workpath

   String. `work` directory expected by EDA tools, overridden by
   :option:`soclib-cc --work`.

.. _conf-build_env:

Build environment specifiers
----------------------------

Configuration files contain 3 types of objects:

* ``Library``, a library definition

  * with a name
  * compilation flags and paths
  * mode-specific flags (`debug`, `prof`, `release`)

* ``Toolchain``, a compiler

  * which commands, tools
  * compilation flags and paths
  * mode-specific flags (`debug`, `prof`, `release`)

* ``BuildEnv``, a build environment, assembling the latter two

As definition of compiler and library variants are often nearly the
same, build-environment specifiers can inherit one another. Moreover,
some build environment specifiers can be templates where usual data is
provided with parts to replace transparently. See `inheriting
configuration`_ for an example, see
:py:mod:`soclib_cc.config.objects` for implementation details.


Configuration directives
------------------------

There are also some configuration directives.

.. function:: config.set_default(name)

   Sets the build-environment named `name` as default. This can be
   overridden with :option:`soclib-cc -t`.

.. function:: config.addDescPath(path)

   Adds a path to the :ref:`md-paths`. Command line can also add such
   description paths with :option:`soclib-cc -I`.

.. function:: config.add_desc_parser(parser)

   Adds a python module named ``parser`` as another :ref:`metadata
   provider <md-providers>`.

Built-In configuration
----------------------

Built-In configuration is made of:

* One default toolchain in ``config.toolchain``, using ``gcc`` and
  ``g++``.

* One default SystemC library declaration in ``config.systemc``
  expecting the ``${SYSTEMC}`` environment variable to be set. It
  includes ``${SYSTEMC}/include`` and links
  ``${SYSTEMC}/lib-<os>/libsystemc.<ext>``.

* One default build environment ``config.build_env`` using the default
  toolchain and SystemC as sole library.

See ``utils/lib/python/soclib_cc/config/built-in.conf`` for its
implementation. Dont modify this file directly, use `configuration
file paths`_.

Inheriting configuration
------------------------

``Library``, ``Toolchain`` and ``BuildEnv`` all can take a ``parent
=`` construction keyword argument of the same type, and will inherit
all their attributes.

Example creating a new toolchain with ccache and gcc-4.2::

  config.my_toolchain = Toolchain(
    parent = config.toolchain,
    tool_CC = ["ccache", "gcc-4.2"],
    tool_CXX = ["ccache", "g++-4.2"],
    )

This does not define a new build environment, therefore we cant use it
yet. We now have to define a new build environment::

  config.my_build_env = BuildEnv(
    parent = config.build_env,
    toolchain = config.my_toolchain,
    )

Now if we use soclib-cc, we'll see::

  $ soclib-cc -h
  ...
  --type=TYPE, -t TYPE    Use a different configuration: <*build_env,
                          my_build_env>

So we can use our ``my_build_env`` with ``soclib-cc -t my_build_env``.
Now if we set ``my_build_env`` as default with::

  config.set_default("my_build_env")

we'll have::

  $ soclib-cc -h
  ...
  --type=TYPE, -t TYPE    Use a different configuration: <*my_build_env,
                          build_env>

and explicitely setting ``-t`` wont be necessary any more.


Expected data
=============

``Library``
-----------

``name``:
  String, mandatory.

  Name of the library. Expected library name, for SystemC
  implementations, is ``"systemc"``. Other libraries have a free
  naming.

``vendor``:
  String, mandatory for SystemC implementations.

  Vendor of implementation of SystemC. Defined values are ``"OSCI"``,
  ``"systemcass"`` and ``"modelsim"``. This changes quirks used in the
  compilation process.

``libs``, ``release_libs``, ``debug_libs``, ``prof_libs``:
  Flags added at linking stage. ``libs`` is always present, others are
  select by current compilation mode.

``cflags``, ``release_cflags``, ``debug_cflags``, ``prof_cflags``:
  Flags added at compilation stage. ``cflags`` is always present,
  others are select by current compilation mode.

``Toolchain``
-------------

``obj_ext``:
  Extension of object files. Defaults to ``o``. Some compilation
  drivers use other extensions.

``lib_ext``:
  Extension of a library archive. Defaults to ``a``. Some compilation
  drivers use other extensions.

``libs``, ``release_libs``, ``debug_libs``, ``prof_libs``:
  Flags added at linking stage. ``libs`` is always present, others are
  select by current compilation mode.

``cflags``, ``release_cflags``, ``debug_cflags``, ``prof_cflags``:
  Flags added at compilation stage. ``cflags`` is always present,
  others are select by current compilation mode.

``always_include``:
  C/C++-specific. List of header files to unconditionally add to
  compilation commands with ``-include <file>``.

``max_processes``:
  Maximal count of concurrent build processes.

``tool_<TOOL_IDENTIFIER>``:
  Definition of command used for `TOOL_IDENTIFIER`. Known tool
  identifiers are ``CC``, ``CXX``, ``CC_LINKER``, ``CXX_LINKER``,
  ``LD``, ``VHDL`` and ``VERILOG``.

``BuildEnv``
------------

``libraries``:
  A list of Library_ objects.

``toolchain``:
  A Toolchain_ object.

``repos``:
  Writeable path in filesystem, used as temporary object spool
  directory.

``cache_file``:
  Metadata cache file path, defaults to a file under ``repos``.

``sd_ignore_regexp``:
  Regexp of filenames to ignore while indexing metadata files. This
  can ensure VCS files are ignored.

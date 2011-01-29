.. -*- rst -*-

.. _md:

========
Metadata
========

.. index:: metadata

.. _md-goals:

Metadata are needed to serve different goals:

* automatically build modules,

* describe modules to automated netlisters,

* provide sideband information about the modules.

Concepts
========

A module is either an actual component, or an utility (a pure C++
class, a port definition, etc.) needed in a platform.

Each module is accompanied by a metadata file, which contains all
information to satisfy :ref:`goals <md-goals>`.

A module description can evolve to different stages:

* Module_

* Specialization_

* ComponentBuilder_

Il we try to wrap-up, life of a module in ``soclib_desc`` can be
summarized in this picture:

.. figure:: /_static/in-memory.*
   :alt: In-memory metadata

We will describe these stages below.

.. _md-module:

Module
======

.. index:: module

A ``Module`` is the module description, in an abstract way.

The some module description is not enough to compile it. If this is a
SystemC (C++) module, it may have some template parameters which
change the template instanciation. Therefore, user must provide the
template parameters to the build system in order to compile the
module's code. This is the role of the Specialization_.

Module definition contains the objective description of the module,
with a set of attributes:

* name

* source files

* parameters (compile-time and run-time)

* ports (if it is a module). Sometimes, ports depends on parameters.

* needed modules, used as sub-modules. Most of the time, they are
  parameterized.

* other side-band data.

Complete API implemented by modules is described in
:py:class:`soclib_desc.module.ModuleInterface`.

Specialization
==============

.. index::
   pair: specialization; module

With addition of template parameters, a Module_ description can evolve
into a ``Specialization``. ``Specialization`` is a module definition
with fixed template parameters. This permits:

* to enumerate the sub-modules with their own parameters (i.e. a list
  of submodule ``Specializations``.

* to enumerate ports of the module, with fixed widths, il applicable

* to enumerate instance parameters

* to retrieve entity name (C++ class name, or VHDL entity)

* to obtain a ComponentBuilder_, which is able to build the needed
  source files to obtain objects.

Complete API implemented by specializations is described in
:py:class:`soclib_desc.specialization.SpecializationInterface`.

ComponentBuilder
================

.. index:: component builder

The builder is the last thing you can obtain with module
descriptions. It is able to generate program command lines to issue to
:py:mod:`soclib_builder`.

Complete API implemented by component builders is described in
:py:class:`soclib_desc.component_builder.ComponentBuilderInterface`.

.. _md-index:

Module Index
============

.. index::
   pair: module; index

Upon initialization, :py:mod:`soclib_desc` scans the directories
defined in `metadata paths`_.  All found modules declarations are
gathered in a global database: the module index.

The module index contains all defined modules, whatever the metadata
parser module that provided their definition.  The index is keyed by
the name of the component. See :option:`soclib-cc -l` for index
listing.

For better performance and lower startup times, the index is
cached. See :option:`soclib-cc -X` for cache cleaning.

.. _md-providers:

Metadata providers
==================

.. _md-paths:

Metadata paths
==============


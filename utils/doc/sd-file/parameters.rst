.. _sd-file-parameters:

Parameters
==========

.. index::
   pair: .sd files; parameters

General parameters description is covered in
:ref:`relevant chapter in metadata module description <soclib_desc-parameter>`.

Inheritance
-----------

Most important feature of parameter passing is inheritance of
parameters from one module to used ones. This is done through the
``parameter.Reference`` statement.

Example::

  Module("caba:my_base_module",
         classname = "MyBaseModule",
         tmpl_parameters = [
            parameter.Int('param_base'),
            ],
     )

  Module("caba:my_other_module",
         classname = "MyOtherModule",
         tmpl_parameters = [
            parameter.Int('param_other'),
            ],
         uses = [
            Uses('caba:my_base_module', param_base = parameter.Reference('param_other')),
            ],
     )

Instanciating ``caba:my_other_module`` will require setting
``param_other`` only, and its value will be propagated to
``param_base`` parameter of ``caba:my_base_module``.

Parameter references can also be used in port declarations::

  Port('caba:bit_in','p_irq', parameter.Reference('n_irq'))

Formulae
--------

When inheriting values, basic formulae can also be used on
``parameter.Reference``. Supported operators are ``+``, ``-``, ``%``,
``*``, ``/`` and ``**`` (power).

Example::

  Uses('caba:my_base_module', param_base = parameter.Reference('param_other') * 2),

Default values
--------------

If parameters are left unspecified by callers, it is possible to
provide default values. This is done through the ``default``
keyword. Example::

  instance_parameters = [
     parameter.Int("answer", default = 42),
     ],

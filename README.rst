MIPSLOL
########################################
A MIPS simulator with Lots Of Love :heart:

It read MIPS instructions in binary form from ``ins.txt`` and executes them.


Dependencies
============

MIPSLOL needs meson build system and a sane C compiler.



macOS
-----

MIPSLOL's dependencies can be installed via `Homebrew <http://brew.sh/>`_
:

.. code-block:: bash

    $ brew install clang meson



Linux
-----


You need Clang or gcc, plus meson.

They can be installed using the system package manager, for example:

.. code-block:: bash

    # Debian, Ubuntu, etc.
    $ apt-get install meson clang

.. code-block:: bash

    # Fedora
    $ dnf install meson clang

.. code-block:: bash

    # CentOS, RHEL, ...
    $ yum install meson clang

.. code-block:: bash

    # Arch Linux
    $ pacman -S meson clang



Windows:
--------


First, ensure that you have a working C compiler.

Then install `meson <http://mesonbuild.com/>`_



Compiling
=========

Go to the root of the project and run:



.. code-block:: bash

    $ meson build
    $ cd build/
    $ ninja



Usage
=====

Yeah...

It dosen't do a lot yet.


.. code-block:: bash

    $ ./mipslol



Contribution:
============

You know the drill:

* fork
* new feature branch
* do stuff
* commit
* push
* pull request


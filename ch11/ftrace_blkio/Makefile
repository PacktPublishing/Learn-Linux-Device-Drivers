# Makefile
# A 'better' Makefile template for Linux system programming.
#
# Designed to support building C projects with MULTIPLE source files.
# Handles separate compilation and linking of object files.
#
# The majority of the Makefile logic is in the included
# <../...>/Makefile_common.mk file, residing in this repo's root directory.
# This Makefile simply sets up project-specific variables like PROG_NAME,
# SRCS etc and includes the common Makefile.

# Besides the 'usual' targets to build production and debug versions of the
# code and cleanup, we incorporate targets to do useful (and indeed required)
# stuff like:
#  - prod_2part: build a '2-part' production target 'prod_2part'; it's
#     -O${PROD_OPTLEVEL}, no debug symbolic info, strip-debug;
#     Excellent for production as it gives ability to debug as and when required!
#  - indent: adhering to (Linux kernel) coding style guidelines (indent+checkpatch)
#  - sa: static analysis target (via flawfinder, cppcheck)
#  - dynamic analysis target: via valgrind
#  - code coverage via gcov/lcov
#  - a packaging (.tar.xz) target and
#  - a help target.
#
# You will require these utils installed:
#  indent, flawfinder, cppcheck, valgrind, kernel-headers package -or- simply the
#  checkpatch.pl script, gcov, lcov, tar; + libasan
#  The lcov_gen.sh wrapper script
#   (DEV NOTE: careful of dependencies between this Makefile and script)
#
# To get started, just type:
#  make help
#
# (c) Kaiwan N Billimoria, kaiwanTECH
# License: MIT

## Pl check and keep or remove <foo>_dbg_[asan|ub|msan] targets
## (where <foo> is the program name) as desired.
ALL :=  prod prod_2part

#----------------- User configurable section ---------------------------------
# Update as required
# Set the program name (executable name) here:
PROG_NAME := file_io_test

# List all C source files (can be in subdirectories or same directory)
# Examples:
#  SRCS := main.c helper.c utils.c
#  SRCS := src/main.c src/helper.c src/utils.c
# If appropriate, can do:
SRCS := $(wildcard *.c)

# Verbosity in builds
VERBOSE ?= 1  # set to 0 for disabling verbose builds; or run as 'make V=0'

#--- CHECK: manually add params as required
# Populate any required cmdline arguments to the process here:
CMDLINE_ARGS=""
#----------------- END User configurable section ------------------------------
#----------------- Typically no need to change anything below this line -------

# Automatically generate object file names from source files
# Build directory for object and intermediate files (out-of-source build)
# You can override on the make command line: `make OBJDIR=build`.
OBJDIR ?= build

# Automatically generate object file names from source files (preserve subdirs under OBJDIR)
OBJS := $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))

# For coverage builds, we need alternate object file names
OBJS_GCOV := $(patsubst %.c,$(OBJDIR)/%.gcov.o,$(SRCS))

#---------------- Include the common Makefile --------------------------------
include ../../Makefile_common_app.mk

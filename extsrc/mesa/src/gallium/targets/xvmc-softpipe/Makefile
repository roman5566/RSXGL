TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = XvMCsoftpipe

DRIVER_DEFINES = -DGALLIUM_SOFTPIPE
DRIVER_INCLUDES =

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/sw/xlib/libws_xlib.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	$(TOP)/src/gallium/winsys/g3dvl/xlib/xsp_winsys.c

DRIVER_LIBS =

include ../Makefile.xvmc

symlinks:

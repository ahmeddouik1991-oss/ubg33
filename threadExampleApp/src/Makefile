# SPDX-FileCopyrightText: 1998 Argonne National Laboratory
#
# SPDX-License-Identifier: EPICS

TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Use typed rset structure (see 3.16.1 release notes)
USR_CPPFLAGS += -DUSE_TYPED_RSET -DUSE_TYPED_DSET

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC += xxxRecord

# Install xxxSupport.dbd into <top>/dbd
DBD += xxxSupport.dbd

# Build an IOC support library
LIBRARY_IOC += threadExampleSupport

# Compile and add code to the support library
threadExampleSupport_SRCS += xxxRecord.c
threadExampleSupport_SRCS += devXxxSoft.c

# Link locally-provided code into the support library,
# rather than directly into the IOC application, that
# causes problems on Windows DLL builds
threadExampleSupport_SRCS += dbSubExample.c
threadExampleSupport_SRCS += devthreadExampleVersion.c
threadExampleSupport_SRCS += threadExampleHello.c
threadExampleSupport_SRCS += initTrace.c
threadExampleSupport_SRCS += threadExample.c

threadExampleSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Auto-generate a header file containing a version string.
# Version comes from the VCS if available, else date+time.
GENVERSION = threadExampleVersion.h
# Macro name
GENVERSIONMACRO = threadExampleVERSION

# Build the IOC application
PROD_IOC = threadExample

# threadExample.dbd will be created and installed
DBD += threadExample.dbd

# threadExample.dbd will include these files:
threadExample_DBD += base.dbd
threadExample_DBD += xxxSupport.dbd
threadExample_DBD += dbSubExample.dbd
threadExample_DBD += devthreadExampleVersion.dbd
threadExample_DBD += threadExampleHello.dbd
threadExample_DBD += initTrace.dbd
threadExample_DBD += threadExampleCommand.dbd

# threadExample_registerRecordDeviceDriver.cpp derives from threadExample.dbd
threadExample_SRCS += threadExample_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
threadExample_SRCS_DEFAULT += threadExampleMain.cpp
threadExample_SRCS_vxWorks += -nil-

# Link in the code from our support library
threadExample_LIBS += threadExampleSupport

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file
ifneq ($(SNCSEQ),)
    # Build sncExample into threadExampleSupport
    sncExample_SNCFLAGS += +r
    threadExample_DBD += sncExample.dbd
    # A .stt sequence program is *not* pre-processed:
    threadExampleSupport_SRCS += sncExample.stt
    threadExampleSupport_LIBS += seq pv
    threadExample_LIBS += seq pv

    # Build sncProgram as a standalone program
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    # A .st sequence program *is* pre-processed:
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Link QSRV (pvAccess Server) if available
ifdef EPICS_QSRV_MAJOR_VERSION
    threadExample_LIBS += qsrv
    threadExample_LIBS += $(EPICS_BASE_PVA_CORE_LIBS)
    threadExample_DBD += PVAServerRegister.dbd
    threadExample_DBD += qsrv.dbd
endif

# Finally link IOC to the EPICS Base libraries
threadExample_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE

# Explicit dependency needed for generated header file
devthreadExampleVersion$(DEP): $(COMMON_DIR)/$(GENVERSION)

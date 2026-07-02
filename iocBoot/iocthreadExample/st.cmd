#!../../bin/darwin-aarch64/threadExample

#- SPDX-FileCopyrightText: 2000 Argonne National Laboratory
#-
#- SPDX-License-Identifier: EPICS

#- You may have to change threadExample to something else
#- everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/threadExample.dbd"
threadExample_registerRecordDeviceDriver pdbbase

## Load record instances
dbLoadTemplate "db/user.substitutions"
dbLoadRecords "db/threadExampleVersion.db", "user=ahdo1293"
dbLoadRecords "db/dbSubExample.db", "user=ahdo1293"
dbLoadRecords("db/array.db","P=ahdo1293:")
#- Set this to see messages from mySub
#-var mySubDebug 1

#- Run this to trace the stages of iocInit
#-traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit

## Start any sequence programs
#seq sncExample, "user=ahdo1293"

//UPDPRM  JOB 1,'SOFTWARE',MSGLEVEL=(1,1),CLASS=A
//SG EXEC PGM=IEHLIST
//SYSPRINT DD SYSOUT=A,OUTLIM=0
//DD1      DD  UNIT=3350,VOL=SER=PERM73,DISP=OLD
//SYSIN DD *
    LISTVTOC VOL=3350=PERM73
    LISTPDS  DSNAME=SYS3.LINKLIB,VOL=3350=PERM73
    LISTPDS  DSNAME=SYS3.JCLLIB,VOL=3350=PERM73
    LISTPDS  DSNAME=SYS3.COPYLIB,VOL=3350=PERM73
/*
//S0      EXEC PGM=IEBUPDTE,PARM=NEW
//SYSPRINT DD SYSOUT=A,OUTLIM=0
//SYSUT1   DD DSN=SYS1.PARMLIB,DISP=OLD
//SYSUT2   DD DSN=SYS1.PARMLIB,DISP=OLD
//SYSIN    DD DATA,DLM=##
./ ADD NAME=CMD01
SF ,00D
SF WTR,00E,,A
S WTRZ.WTRZ
S INITSWA.P1,,,PARM='SWA=2048'
S INITSWA.P2,,,PARM='SWA=2048'
S INITSWA.P3,,,PARM='SWA=2048'
S INITSWA.P4,,,PARM='SWA=2048'
./ ADD NAME=IEAAPF00
 SYS1.LINKLIB FGEN67,
 SYS2.LINKLIB FGEN67,
 SYS3.LINKLIB PERM73
./ ADD NAME=PRESRES
FRES67,0,2,3350,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
FGEN67,0,2,3350,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
FDLB67,0,2,3350,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
PERM73,0,0,3350,N  MOUNT=RESIDENT  USE=STORAGE  SKIP IPL MOUNT MSG
WORK73,0,1,3350,N  MOUNT=RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
PAGE73,0,2,3330,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
PERM71,0,0,3350,N  MOUNT=RESIDENT  USE=STORAGE  SKIP IPL MOUNT MSG
WORK71,0,1,3350,N  MOUNT=RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
PAGE71,0,2,3340,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
DLIBA1,0,2,3350,N  MOUNT=RESIDENT  USE=PRIVATE  SKIP IPL MOUNT MSG
WORK61,0,1,3350,N  MOUNT=RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
SORT01,0,1,2314,N  MOUNT-RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
SORT02,0,1,2314,N  MOUNT-RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
SORT03,0,1,2314,N  MOUNT-RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
SORT04,0,1,2314,N  MOUNT-RESIDENT  USE=PUBLIC   SKIP IPL MOUNT MSG
./ ADD NAME=SSI01
,,SHUTDOWN
./     ENDUP
/*
##
//S1     EXEC PGM=PARMWTO,
// PARM='First time users be sure to update your config file.'
//STEPLIB DD  DSN=SYS3.LINKLIB,DISP=SHR
//S2     EXEC PGM=PARMWTO,
// PARM='before next IPL. Review 00README.txt for instructions'
//STEPLIB DD  DSN=SYS3.LINKLIB,DISP=SHR
//ISNBL4  JOB  1,1,MSGCLASS=A,REGION=512K
//MOVE   EXEC  PGM=IEHMOVE
//SYSPRINT DD  SYSOUT=A
//SYSUT1   DD  UNIT=SYSDA,VOL=SER=FGEN67,SPACE=(CYL,(1,1,10))
//TMP001   DD  DSN=SYS2.SNOBOL4.LINKLIB,DISP=OLD,
//             UNIT=SYSDA,VOL=SER=FGEN67
//TAPE     DD  DSN=A,UNIT=181,VOL=SER=SNOBOL,DISP=OLD,
//             LABEL=(1,NL),
//             DCB=(DEN=4,RECFM=FB,LRECL=80,BLKSIZE=800)
//SYSIN    DD  *
 COPY  PDS=SYS2.SNOBOL4.LINKLIB,FROM=181=SNOBOL,TO=SYSDA=FGEN67,       X
               FROMDD=TAPE
/*
//STEP2   EXEC PGM=IEHPROGM 
//SYSPRINT  DD SYSOUT=A 
//DISK      DD UNIT=SYSDA,VOL=SER=FGEN67,DISP=OLD
//SYSIN DD *
  CATLG DSNAME=SYS2.SNOBOL4.LINKLIB,VOL=SYSDA=FGEN67
/*  
//

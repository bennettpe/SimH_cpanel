//DEFSPCE JOB 1,'SOFTWARE',CLASS=A,MSGCLASS=A
//S0      EXEC PGM=IDCAMS
//SPRES   DD  UNIT=SYSDA,VOL=SER=FGEN67,DISP=OLD
//SPUCAT  DD  UNIT=SYSDA,VOL=SER=PERM73,DISP=OLD
//SYSPRINT DD SYSOUT=Z
//SYSIN    DD  *
 DEFINE SPACE ( -
        FILE (SPUCAT) -
        CYL (250 10) -
        VOLUME (PERM73)) -
        CATALOG (SYS1.FGEN67.CATALOG)
/*

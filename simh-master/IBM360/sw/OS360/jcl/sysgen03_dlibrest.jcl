//REST02 JOB MSGLEVEL=1
// VARY 191,ONLINE
//REST02    EXEC PGM=IEHDASDR,PARM='N=1'
//SYSPRINT   DD  SYSOUT=A
//FROM02     DD  UNIT=180,DISP=OLD,LABEL=(4,NL),VOLUME=SER=REST01
//DLIB02     DD  UNIT=191,DISP=OLD,VOLUME=SER=DLIB02
//SYSIN      DD  *
        RESTORE  FROMDD=FROM02,TODD=DLIB02,CPYVOLID=YES,PURGE=YES
/*
//
//REST03 JOB MSGLEVEL=1
// VARY 192,ONLINE
//REST03   EXEC PGM=IEHDASDR,PARM='N=1'
//SYSPRINT   DD  SYSOUT=A
//FRM03A     DD  UNIT=180,DISP=OLD,LABEL=(6,NL),VOLUME=SER=REST01
//DLIB03     DD  UNIT=192,DISP=OLD,VOLUME=SER=DLIB03
//SYSIN      DD  *
        RESTORE  FROMDD=FRM03A,TODD=DLIB03,CPYVOLID=YES,PURGE=YES   
/*
//
//REST03A JOB MSGLEVEL=1
//REST03A  EXEC PGM=IEHDASDR,PARM='N=1'
//SYSPRINT   DD  SYSOUT=A
//FRM03B     DD  UNIT=181,DISP=OLD,LABEL=(2,NL),VOLUME=SER=REST02
//DLIB03     DD  UNIT=192,DISP=OLD,VOLUME=SER=DLIB03
//SYSIN      DD  *
        RESTORE  FROMDD=FRM03B,TODD=DLIB03,CPYVOLID=YES,PURGE=YES   
/*
//
//REST04 JOB MSGLEVEL=1
// VARY 193,ONLINE
//REST04    EXEC PGM=IEHDASDR,PARM='N=1'
//SYSPRINT   DD  SYSOUT=A
//FROM04     DD  UNIT=181,DISP=OLD,LABEL=(4,NL),VOLUME=SER=REST02
//DLIB04     DD  UNIT=193,DISP=OLD,VOLUME=SER=DLIB04
//SYSIN      DD  *
        RESTORE  FROMDD=FROM04,TODD=DLIB04,CPYVOLID=YES,PURGE=YES   
/*
//

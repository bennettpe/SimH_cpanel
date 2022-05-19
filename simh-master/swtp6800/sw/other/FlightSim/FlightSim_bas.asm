       ORG  $7000
TYPE   LDX  #$005D    GET USER(N) FUNCTION
       LDX  0,X       PARAMETER N
       LDAA 0,X       IN A-REG
       ANDA #$0F
       CMPA #01       IS=1?
       BEQ  FIRST     YES, GOTO INIT GT-6144
       CMPA #02       IS=2?
       BEQ  JINIT     YES, GOTO INIT PPG-J ANALOG JOYSTICK
       CMPA #03       IS=3?
       BEQ  ERASE     YES, GOTO ERASE SCREEN
       CMPA #04
       BEQ  PIXEL
       CMPA #05
       BEQ  SHOW2
       CMPA #06
       BEQ  START
       RTS
*PIA INITIALIZATION SECTION
FIRST  CLRA
       STAA $800C     INIT GT6144
       LDAA #$FF
       STAA $800C
       LDAA #$3F
       STAA $800D
       RTS
*ERASE SCREEN SECTION
ERASE  LDAA #63
       STAA HPOS
HSET   LDAA HPOS
       BSR  SENDA
VBAR   LDAA #128
CLEAR  BSR  SENDA
       INCA
MOD    CMPA #224
       BNE  CLEAR
       DEC  HPOS
       BGE  HSET
       RTS
*SCREEN DRIVE SUBROUTINES
SENDA  TAB
SENDB  STAB $800C     SEND BYTE TO GT-6144
       LDAB #$37
       STAB $800D
       LDAB 0,X
       LDAB #$3F
       STAB $800D
       RTS
*PIXEL OUTPUT
PIXEL  LDAB X1
       ADDB #64
       BSR SENDB
       LDAB #224
       SUBB Y1
       BSR SENDB
       RTS
*
SHOW2  BRA SHOW
*DATA INPUT PGM FOR JOYSTICK BOX
*INITIALIZE PIA INTERFACE TO JOYSTICK
JINIT  LDAA #$30     INIT PPG-J JOY
       LDX #$800C
       LDAB #$00
       STAB 2,X
       STAA 3,X
*SAVE MACHINE REG'S ON INPUT
START  STAA ASAVE
       STAB BSAVE
       STX ISAVE
*GET HORIZONTAL STICK POSITION
       LDX #$800C
LOOP1  LDA 3,X
       ANDA #$80
       BEQ LOOP1
       LDAA 2,X
       TAB
       ANDA #$80
       BNE LOOP1
       ANDB #$3F
       SUBB #$20
       BGE RTN1
       NEGB
       ADDB #64
RTN1   STAB JHPOS
LOOP2  LDAA 3,X
       ANDA #$80 
       BEQ LOOP2
       LDAA 2,X
       TAB
       ANDA #$80
       BNE GOBACK
       BRA LOOP1
GOBACK ANDB #$3F
       SUBB #$20
       BGE RTN2
       NEGB
       ADDB #64
RTN2   STAB JVPOS
       LDAA ASAVE
       LDAB BSAVE
       LDX ISAVE
       RTS
*ROUTINE TO DRAW LINE FROM (X1,Y1)
*TO (X2,Y2)
SHOW   CLRA
       LDAB #1
       STAB M
       STAB N
       LDAB X2 
       SUBB X1
       STAB D
       BGE BP1
       NEG M
       NEG D
BP1    BNE BP2
       LDAA #$FF
BP2    LDAB Y2
       SUBB Y1
       STAB E
       BGE B8963
       NEG N
       NEG E
B8963  JSR PIXEL
       LDAB X1
       CMPB X2
       BEQ B8990
B8969  TSTA
       BLT B8981
       LDAB X1
       ADDB M
       STAB X1
       SUBA E
       BRA B8963
B8981  LDAB Y1
       ADDB N
       STAB Y1
       ADDA D
       BRA B8963
B8990  LDAB Y1
       CMPB Y2
       BNE B8969
       RTS
X1     EQU $5EF0
X2     EQU $5EF1
Y1     EQU $5EF2
Y2     EQU $5EF3
M      EQU $5EF4
N      EQU $5EF5
D      EQU $5EF6
E      EQU $5EF7
HPOS   EQU $5EF8
JHPOS  EQU $5EF9
JVPOS  EQU $5EFA
ASAVE  EQU $5EFB
BSAVE  EQU $5EFC
ISAVE  EQU $5EFD



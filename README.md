# RISC

Description:
  Процедуры работают только с аргументами и глобальными функциями (переменными).
  Выходные объекты должны быть аллоцированы до вызова функции.
  Все данные в data  выровнены по u16.
  Все данные в stack выровнены по u64.

TODO:
  Добавить макросы как в assembler.


Registers: u64
  xxxx RIP   PC         ;
  xxxx RFP   FP         ;
  xxxx RSP   SP         ;
  xxxx RF    FLAGS
  xxxx RT    TMP
  xxxx RC    CONST
  xxxx RA    args
  xxxx R9
  xxxx R8
  xxxx R7
  xxxx R6
  xxxx R5
  xxxx R4
  xxxx R3
  xxxx R2
  xxxx R1

FLAGS:
  0000 0001   sign
  0000 0010   zero
  0000 0100   overflow
  0000 1000   carry?
  0001 0000   carry-block?
  0010 0000   ---
  0100 0000   ---
  1000 0000   ---


xxxx xxxx xxxx xxxx
 cmd    d    v    v   SET(d,a):     Rd = (0 0 0 0 0 0 0 v)
 cmd    d    a    b   OR (d,a,b):   Rd = Ra | Rb
 cmd    d    a    b   AND(d,a,b):   Rd = Ra & Rb
 cmd    d    a    b   XOR(d,a,b):   Rd = Ra ^ Rb
 cmd    d    a    b   ADD(d,a,b):   Rd = Ra + Rb   + c
 cmd    d    a    b   SUB(d,a,b):   Rd = Ra - Rb   - c
 cmd    d    a    b   MULT(d,a,b):  (Rt, Rd) = Ra * Rb
 cmd    d    a    b   DIV(d,a,b):   Rd = Ra / Rb; Rt = Ra % Rb
 cmd    d    _    a   BR(d,a):      RIP = Rd if Ra
 cmd    d    _    a   NOT(d,a):     Rd = ~Ra
 cmd    d    _    a   LSH(d,a):     Rd = Rd << Ra
 cmd    d    _    a   RSH(d,a):     Rd = Rd >> Ra
 cmd    d    _    a   LOAD(d,a):    Rd = M[Ra]              // TODO width
 cmd    d    _    a   SAVE(d,a):    M[Ra] = Rd              // TODO width
 cmd    d    _    a   MOV(d,a):     Rd = Ra


INC1     Ra:      set(Rt, 1); INC(Ra, Rt);
DEC1     Ra:      set(Rt, 1); DEC(Ra, Rt);
SETF     Ra:      or(Rf, Rf, Ra);
CLRF     Ra:      not(Rt, Rf); or(Rt, Rt, Ra); not(Rf, Rt);
NEG      Ra:      set(Rt, 0); sub(Ra, Rt, Ra);
PLOADI   Ra  I:   set(Rt, I); add(Rt, Rt, SP); load(Ra, Rt);
PLOAD    Ra Rb:   add(Rt, Rb, SP); load(Ra, Rt);
PSAVE    Ra Rb:   add(Rt, Rb, SP); save(Rt, Ra);
INC      Ra Rb:   add(Ra, Ra, Rb);
DEC      Ra Rb:   sub(Ra, Ra, Rb);

CALL(Ra):   ...
RET:        ...

FUNCTION(name)      functions[name] = position
LABEL(name)         labels[name] = position
ADDRESS(Ra, name)   set(Ra, functions[name] ? functions[name] : labels[name])



segments:
  u64:type
  u64:size

segment dynsym:
  u64:size
  [u64:pointer]
  [str:name]

segment text:
  [u64:instruction]



example:

FUNCTION main
  SET RA1 3
  CALL square
  SET RT1 10
  ADD RT2 RES RT1
RETURN

FUNCTION square
  MULT RES RA1 RA1
RETURN





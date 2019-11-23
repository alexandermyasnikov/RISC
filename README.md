# RISC

Description:
  Процедуры работают только с аргументами и глобальными функциями (переменными).
  Выходные объекты должны быть аллоцированы до вызова функции.



Registers: u64
  xxxx RIP  PC         ;
  xxxx RSP  SP         ;
  xxxx RFP  FP         ;
  xxxx RF   FLAGS
  xxxx RT   TMP
  xxxx RA   address
  xxxx RES  res
  xxxx RA1  arg1
  xxxx RA2  arg2
  xxxx RA3  arg3
  xxxx RA4  arg4
  xxxx RA5  arg5
  xxxx RA6  args
  xxxx RT1
  xxxx RT2
  xxxx RT3

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

xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
   _    _  cmd    d    v    v    v    v   set(d,a):     Rd = (v v v v)
   _    _    _  cmd    d    _    a    b   or (d,a,b):   Rd = Ra | Rb
   _    _    _  cmd    d    _    a    b   and(d,a,b):   Rd = Ra & Rb
   _    _    _  cmd    d    _    a    b   xor(d,a,b):   Rd = Ra ^ Rb
   _    _    _  cmd    d    _    a    b   add(d,a,b):   Rd = Ra + Rb   + c
   _    _    _  cmd    d    _    a    b   sub(d,a,b):   Rd = Ra - Rb   - c
   _    _    _  cmd    d    _    a    b   mult(d,a,b):  (Rt, Rd) = Ra * Rb
   _    _    _  cmd    d    _    a    b   div(d,a,b):   Rd = Ra / Rb; Rt = Ra % Rb
                cmd    d    _    _    a   branch(d,a):  RIP = Rd if Ra
                cmd    d    _    _    a   not(d,a):     Rd = ~Ra
                cmd    d    _    _    a   lshift(d,a):  Rd = Rd << Ra
                cmd    d    _    _    a   rshift(d,a):  Rd = Rd >> Ra
                cmd    d    _    _    a   load(d,a):    Rd = M[Ra]
                cmd    d    _    _    a   save(d,a):    M[Ra] = Rd
                cmd    d    _    _    a   mov(d,a):     Rd = Ra


INC1    Ra:      set(Rt, 1); INC(Ra, Rt);
DEC1    Ra:      set(Rt, 1); DEC(Ra, Rt);
SETF    Ra:      or(Rf, Rf, Ra);
CLRF    Ra:      not(Rt, Rf); or(Rt, Rt, Ra); not(Rf, Rt);
NEG     Ra:      set(Rt, 0); sub(Ra, Rt, Ra);
PLOAD   Ra Rb:   add(Rt, Rb, SP); load(Ra, Rt);
PSAVE   Ra Rb:   add(Rt, Rb, SP); save(Rt, Ra);
INC     Ra Rb:   add(Ra, Ra, Rb);
DEC     Ra Rb:   sub(Ra, Ra, Rb);

CALL(Ra):   ...
RET:        ...

FUNCTION(name)   functions[name] = position
LABEL(name)      labels[name] = position
CALL(Ra, name)   set(Ra, functions[name]); CALL(Ra);
BR(Ra, name)     branch(Ra, labels[name])



segments:
  u64:type
  u64:size

segment dynsym:
  u64:size
  u64:pointer ... u64:pointer
  str:name    ... str:name

segment text:
  u64:instruction ... u64:instruction



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





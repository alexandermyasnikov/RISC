# RISC

Author: Alexander Myasnikov

mailto:myasnikov.alexander.s@gmail.com

Version: 0.1



### Описание:

Эмулятор RISC (Reduced instruction set computer).



### Цели:

* Описать необходимый минимальный набор инструкций, необходимый для трансляции высокоуровнего языка.



### Особенности:

* Процедуры работают только с аргументами, локальными даннами и глобальными функциями.
* Выходные объекты должны быть аллоцированы до вызова функции.
* Все данные в data  выровнены по u16.
* Все данные в stack выровнены по u64.



### TODO:

* Добавить макросы как в assembler.



### Набор регистров:

Всего 16 64-разрядных регистров.

```
    static inline std::vector<reg_index_t> regs_table = {
      {   0, "RI"  },   // адрес текущей инструкции
      {   1, "RP"  },   // адрес
      {   2, "RB"  },   // адрес базы стека текущего фрейма
      {   3, "RS"  },   // адрес вершины стека текущего фрейма
      {   4, "RF"  },   // флаги
      {   5, "RT"  },   // для промежуточных данных инструкций
      {   6, "RC"  },   // для констант
      {   7, "RA"  },   // адрес агрументов фекции
      {   8, "R1"  },   // общего назначения
      {   9, "R2"  },   // общего назначения
      {  10, "R3"  },   // общего назначения
      {  11, "R4"  },   // общего назначения
      {  12, "R5"  },   // общего назначения
      {  13, "R6"  },   // общего назначения
      {  14, "R7"  },   // общего назначения
      {  15, "R8"  },   // общего назначения
    };
```



### Набор инструкций:

Все инструкции 16-разрядные

```
xxxx xxxx xxxx xxxx
   0    d    v    v   SET(d,a):     Rd = (0 0 0 0 0 0 0 v)
   1    d    a    b   AND(d,a,b):   Rd = Ra & Rb
   2    d    a    b   OR (d,a,b):   Rd = Ra | Rb
   3    d    a    b   XOR(d,a,b):   Rd = Ra ^ Rb
   4    d    a    b   ADD(d,a,b):   Rd = Ra + Rb   + c
   5    d    a    b   SUB(d,a,b):   Rd = Ra - Rb   - c
   6    d    a    b   MULT(d,a,b):  (Rt, Rd) = Ra * Rb
   7    d    a    b   DIV(d,a,b):   Rd = Ra / Rb; Rt = Ra % Rb
   8    d    a    b   LSH(d,a):     Rd = Ra << Rb
   9    d    a    b   RSH(d,a):     Rd = Ra >> Rb
  15    0    d    a   BR(d,a):      RIP = Rd if Ra
  15    1    d    a   NOT(d,a):     Rd = ~Ra
  15    2    d    a   LOAD(d,a):    Rd = M[Ra]              // TODO width
  15    3    d    a   SAVE(d,a):    M[Ra] = Rd              // TODO width
  15    4    d    a   MOV(d,a):     Rd = Ra
  15   15    0    a   CALL(a):      Сохранение текущих регистров. Создание нового фрейма стека
  15   15   15    0   RET():        Восстановление сохраненных регистров
```



### Макросы:

```
FUNCTION(name)      Сохраняет адрес функции
LABEL(name)         Сохраняет адрес метки
ADDRESS(Ra, name)   Копирует адрес функции (метки) в регистр Ra
```



### Пример кода:

```
FUNCTION square
  MULT R1 R3 R4
RET

FUNCTION main
  SET R1 72623859790382856
  MOV R2 R1
  ADDRESS RA square
  CALL RA
  SET R1 10
  MULT R2 R1 R1
RET

FUNCTION __start
  ADDRESS RA main
  CALL RA
RET
```





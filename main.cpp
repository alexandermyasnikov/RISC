
#include <iostream>
#include <variant>
#include <regex>
#include <iomanip>

#include "debug_logger.h"

#define DEBUG_LOGGER_TRACE_RISC          DEBUG_LOGGER("risc ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_RISC(...)           DEBUG_LOG("risc ", logger_indent_risc_t::indent, __VA_ARGS__)

template <typename T>
struct logger_indent_t { static inline int indent = 0; };

struct logger_indent_risc_t : logger_indent_t<logger_indent_risc_t> { };


/*
struct segment_t {
  uint64_t type;
  std::deque<uint64_t> data;
};

struct segment_text_t {
  std::deque<uint64_t> data;
};

struct library_file_t {
  std::deque<segment_t> segments;
};
*/

struct interpreter_t {
  struct lexeme_empty_t   { };
  struct lexeme_integer_t { int64_t value; };
  struct lexeme_ident_t   { std::string value; };

  using lexeme_t = std::variant<
    lexeme_empty_t,
    lexeme_integer_t,
    lexeme_ident_t>;

  struct rule_t {
    std::regex regex;
    std::function<lexeme_t(const std::string&)> get_lexeme;
  };

  using rules_t = std::vector<rule_t>;

  static inline rules_t rules = {
    {
      std::regex("\\s+|;.*?\n"),
      [](const std::string&) { return lexeme_empty_t{}; }
    }, {
      std::regex("[-+]?\\d+"),
      [](const std::string& str) { return lexeme_integer_t{std::stol(str)}; }
    }, {
      std::regex("[\\w\\d_\\.]+"),
      [](const std::string& str) { return lexeme_ident_t{str}; }
    }
  };

  union instruction_t {
    uint16_t value;
    struct {
      uint8_t  op  : 4;
      uint8_t  rd  : 4;
      uint8_t  rs1 : 4;
      uint8_t  rs2 : 4;
    } __attribute__((packed)) cmd;
    struct {
      uint8_t  op : 4;
      uint8_t  rd : 4;
      uint8_t val;
    } __attribute__((packed)) cmd_set;
  };

  enum OPCODE1 : uint8_t {
    SET  =  0,
    AND  =  1,
    OR   =  2,
    XOR  =  3,
    ADD  =  4,
    SUB  =  5,
    MULT =  6,
    DIV  =  7,
    LSH  =  8,
    RSH  =  9,
    OTH1 = 15,
  };

  enum OPCODE2 : uint8_t {
    BR   =  0,
    NOT  =  1,
    LOAD =  2,
    SAVE =  3,
    MOV  =  4,
    OTH2 = 15,
  };

  enum OPCODE3 : uint8_t {
    CALL =  0,
    OTH3 = 15,
  };

  enum OPCODE4 : uint8_t {
    RET  =  0,
    OTH4 = 15,
  };

  enum REGS : uint8_t {
    RI =  0, // instruction pointer
    RB =  1, // base pointer
    RS =  2, // stack pointer
    RF =  3, // flags
    RT =  4, // tmp
    RC =  5, // const
    RA =  6, // args
    R1 =  7, //
    R2 =  8, //
    R3 =  9, //
    R4 = 10, //
    R5 = 11, //
    R6 = 12, //
    R7 = 13, //
    R8 = 14, //
    R9 = 15, //
  };

  using registers_set_t = uint64_t[16];

  static inline std::map<std::string, uint8_t> registers = {
    { "RI", RI },
    { "RB", RB },
    { "RS", RS },
    { "RF", RF },
    { "RT", RT },
    { "RC", RC },
    { "RA", RA },
    { "R1", R1 },
    { "R2", R2 },
    { "R3", R3 },
    { "R4", R4 },
    { "R5", R5 },
    { "R6", R6 },
    { "R7", R7 },
    { "R8", R8 },
    { "R9", R9 },
  };

  // xxxx xxxx xxxx xxxx
  //    0    d    v    v   SET(d,a):     Rd = (0 0 0 0 0 0 0 v)
  //    1    d    a    b   OR (d,a,b):   Rd = Ra | Rb
  //    2    d    a    b   AND(d,a,b):   Rd = Ra & Rb
  //    3    d    a    b   XOR(d,a,b):   Rd = Ra ^ Rb
  //    4    d    a    b   ADD(d,a,b):   Rd = Ra + Rb   + c
  //    5    d    a    b   SUB(d,a,b):   Rd = Ra - Rb   - c
  //    6    d    a    b   MULT(d,a,b):  (Rt, Rd) = Ra * Rb
  //    7    d    a    b   DIV(d,a,b):   Rd = Ra / Rb; Rt = Ra % Rb
  //    8    d    a    b   LSH(d,a):     Rd = Ra << Rb
  //    9    d    a    b   RSH(d,a):     Rd = Ra >> Rb
  //   15    0    d    a   BR(d,a):      RIP = Rd if Ra
  //   15    1    d    a   NOT(d,a):     Rd = ~Ra
  //   15    2    d    a   LOAD(d,a):    Rd = M[Ra]              // TODO width
  //   15    3    d    a   SAVE(d,a):    M[Ra] = Rd              // TODO width
  //   15    4    d    a   MOV(d,a):     Rd = Ra
  //   15   15    0    a   CALL(a):      ...
  //   15   15   15    0   RET():        ...
  //
  // INC1     Ra:      set(Rt, 1); INC(Ra, Rt);
  // DEC1     Ra:      set(Rt, 1); DEC(Ra, Rt);
  // SETF     Ra:      or(Rf, Rf, Ra);
  // CLRF     Ra:      not(Rt, Rf); or(Rt, Rt, Ra); not(Rf, Rt);
  // NEG      Ra:      set(Rt, 0); sub(Ra, Rt, Ra);
  // PLOADI   Ra  I:   set(Rt, I); add(Rt, Rt, SP); load(Ra, Rt);
  // PLOAD    Ra Rb:   add(Rt, Rb, SP); load(Ra, Rt);
  // PSAVE    Ra Rb:   add(Rt, Rb, SP); save(Rt, Ra);
  // INC      Ra Rb:   add(Ra, Ra, Rb);
  // DEC      Ra Rb:   sub(Ra, Ra, Rb);
  //
  // FUNCTION(name)      functions[name] = position
  // LABEL(name)         labels[name] = position
  // ADDRESS(Ra, name)   set(Ra, functions[name] ? functions[name] : labels[name])

  struct text_t {
    ;
  };

  std::string hex(const std::vector<instruction_t>& instructions) {
    DEBUG_LOGGER_TRACE_RISC;
    std::stringstream ss;
    ss << "{ ";
    for (size_t i = 0; i < instructions.size(); ++i) {
      auto instruction = instructions[i];
      if ((i % 8) == 0) {
        ss << std::endl;
        ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(i))
          << i * sizeof(instruction.value) << "     ";
      }
      ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(instruction.value))
        << instruction.value << " ";
    }
    ss << std::endl << "} " << std::dec << instructions.size() << "s";
    return ss.str();
  }

  template <typename T>
  std::string hex(const std::vector<T>& buffer) {
    DEBUG_LOGGER_TRACE_RISC;
    std::stringstream ss;
    ss << "{ ";
    for (size_t i = 0; i < buffer.size(); ++i) {
      auto val = buffer[i];
      if ((i % 8) == 0) {
        ss << std::endl;
        ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(i))
          << i * sizeof(val) << "     ";
      }
      ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(val)) << (uint64_t) val << " ";
    }
    ss << std::endl << "} " << std::dec << buffer.size() << "s";
    return ss.str();
  }

  struct fatal_error : std::runtime_error {
    fatal_error(const std::string& msg = "unknown error") : std::runtime_error(msg) { }
  };

  uint8_t register_index(const std::string& name) {
    if (registers.find(name) == registers.end())
      throw fatal_error("unknown register");
    return registers[name];
  }

  std::string register_str(uint8_t reg) {
    auto it = std::find_if(registers.begin(), registers.end(), [reg](auto& kv) { return kv.second == reg; });
    if (it == registers.end())
      throw fatal_error("unknown register");
    return it->first;
  }

  void macro_set(std::vector<instruction_t>& instructions, uint8_t rd, int64_t value) {
    DEBUG_LOGGER_TRACE_RISC;
    // DEBUG_LOGGER_RISC("rd: '%x'", rd);
    // DEBUG_LOGGER_RISC("value: '%ld'", value);

    uint8_t bytes[sizeof(uint64_t)];
    memcpy(bytes, &value, sizeof(value));
    std::reverse(std::begin(bytes), std::end(bytes));

    size_t i = 0;
    for (; i < sizeof(uint64_t); i++) {
      if (bytes[i])
        break;
    }

    instructions.push_back({ .cmd_set = { SET, rd, 0 } });

    for (; i < sizeof(uint64_t); i++) {
      instructions.push_back({ .cmd_set = { SET, RT, 8 } });
      instructions.push_back({ .cmd     = { LSH, rd, rd, RT } });
      instructions.push_back({ .cmd_set = { SET, RT, bytes[i] } });
      instructions.push_back({ .cmd     = { OR,  rd, rd, RT } });
    }
  }

  void exec(const std::string code) {
    DEBUG_LOGGER_TRACE_RISC;
    std::vector<lexeme_t> lexemes;

    { // parsing
      std::string s = code;
      std::smatch m;
      while (!s.empty()) {
        for (const auto& rule : rules) {
          if (std::regex_search(s, m, rule.regex, std::regex_constants::match_continuous)) {
            lexeme_t lexeme = rule.get_lexeme(m.str());
            if (!std::get_if<lexeme_empty_t>(&lexeme))
              lexemes.push_back(lexeme);
            s = m.suffix().str();
            break;
          }
        }
        if (m.empty() || !m.empty() && !m.length()) {
          DEBUG_LOGGER_RISC("WARN: unexpected regex: '%s'", s.c_str());
          s.erase(s.begin());
        }
      }
    }

    std::vector<instruction_t> instructions;
    std::map<std::string, size_t> functions;

    { // code genegation
      size_t i = 0;
      while (i < lexemes.size()) {
        std::string lexeme = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        DEBUG_LOGGER_RISC("lexeme: '%s'", lexeme.c_str());

        if (lexeme == "FUNCTION") {
          auto name = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          if (functions.find(name) != functions.end())
            throw fatal_error("function exists");
          functions[name] = instructions.size() * sizeof(instruction_t);
          DEBUG_LOGGER_RISC("function: '%zd', '%s'", functions[name], name.c_str());

        } else if (lexeme == "MULT") {
          auto rd  = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          auto rs1 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          auto rs2 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          instructions.push_back({ .cmd  = { MULT, rd, rs1, rs2 } });

        } else if (lexeme == "ADD") {
          auto rd  = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          auto rs1 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          auto rs2 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          instructions.push_back({ .cmd  = { ADD, rd, rs1, rs2 } });

        } else if (lexeme == "RET") {
          // TODO
          // macro_set(instructions, register_index("RT"), 0xFFFFFFFF);

        } else if (lexeme == "SET") {
          auto arg1 = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          auto arg2 = std::get<lexeme_integer_t>(lexemes.at(i++)).value;
          macro_set(instructions, register_index(arg1), arg2);

        } else if (lexeme == "CALL") {
          auto rs = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
          instructions.push_back({ .cmd  = { OTH1, OTH2, CALL, rs } });

        } else if (lexeme == "ADDRESS") {
          auto arg1 = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
          auto arg2 = std::get<lexeme_ident_t>(lexemes.at(i++)).value;

          if (functions.find(arg2) == functions.end())
            throw fatal_error("function not exists");

          macro_set(instructions, register_index(arg1), functions[arg2]);

        } else {
          DEBUG_LOGGER_RISC("WARN: unknown lexeme: '%s'", lexeme.c_str());
          throw fatal_error("unknown lexeme");
        }
      }
    }

    DEBUG_LOGGER_RISC("instructions: '%s'", hex(instructions).c_str());

    { // code execution
      if (functions.find("tmain") == functions.end())
        throw fatal_error("tmain not exists");

      std::vector<uint8_t> stack(0xFF, 0);
      registers_set_t* registers_set = reinterpret_cast<registers_set_t*>(stack.data());
      (*registers_set)[RI] = functions["tmain"];
      (*registers_set)[RB] = sizeof(registers_set_t);
      (*registers_set)[RS] = (*registers_set)[RB] + 0;


      DEBUG_LOGGER_RISC("stack: '%s'", hex(stack).c_str());
      for (size_t i = 0; i < 1000; ++i) {
        instruction_t* instruction = reinterpret_cast<instruction_t*>(((uint8_t*) instructions.data()) + (*registers_set)[RI]);
        // DEBUG_LOGGER_RISC("cmd: '%d'", instruction->cmd.op);
        // DEBUG_LOGGER_RISC("RI: x'%x'", (*registers_set)[RI]);
        // DEBUG_LOGGER_RISC("RB: x'%x'", (*registers_set)[RB]);
        // DEBUG_LOGGER_RISC("RS: x'%x'", (*registers_set)[RS]);
        // DEBUG_LOGGER_RISC("val: x'%x'", instruction->value);
        switch (instruction->cmd.op) {
          case SET: {
            DEBUG_LOGGER_RISC("  SET %s %x", register_str(instruction->cmd_set.rd).c_str(), instruction->cmd_set.val);
            (*registers_set)[instruction->cmd_set.rd] = instruction->cmd_set.val;
            break;
          }
          case ADD: {
            DEBUG_LOGGER_RISC("  ADD %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] & (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case OR: {
            DEBUG_LOGGER_RISC("  OR %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] | (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case XOR: {
            DEBUG_LOGGER_RISC("  XOR %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] ^ (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case AND: {
            DEBUG_LOGGER_RISC("  AND %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] + (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case SUB: {
            DEBUG_LOGGER_RISC("  SUB %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] - (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case MULT: {
            DEBUG_LOGGER_RISC("  MULT %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] * (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case DIV: {
            DEBUG_LOGGER_RISC("  DIV %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] / (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case LSH: {
            DEBUG_LOGGER_RISC("  LSH %s %s %s",
                register_str(instruction->cmd.rd).c_str(),
                register_str(instruction->cmd.rs1).c_str(),
                register_str(instruction->cmd.rs2).c_str());
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] << (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case RSH: {
            (*registers_set)[instruction->cmd.rd] =
              (*registers_set)[instruction->cmd.rs1] >> (*registers_set)[instruction->cmd.rs2];
            break;
          }
          case OTH1: {
            switch (instruction->cmd.rd) {
              case OTH2: {
                switch (instruction->cmd.rs1) {
                  case CALL: {
                    DEBUG_LOGGER_RISC("  CALL %s",
                        register_str(instruction->cmd.rs2).c_str());
                    DEBUG_LOGGER_RISC("stack: '%s'", hex(stack).c_str());
          /*case CALL: {
            DEBUG_LOGGER_RISC("stack: '%s'", hex(stack).c_str());
            // registers_set_t* registers_set_new = reinterpret_cast<registers_set_t*>((*registers_set)[RS] + stack.data());
            // instruction_t instruction = instructions.at((*registers_set)[RS]);
            // registers_set_new[RI] = ...;
            // (*registers_set_new)[RB] = (*registers_set)[RS] + sizeof(registers_set_t);
            // (*registers_set_new)[RS] = (*registers_set_new)[RB];
            // registers_set = registers_set_new;
            break;
          }*/
                    break;
                  }
                  case OTH3: {
                    switch (instruction->cmd.rs2) {
                      default: throw fatal_error("unknown cmd4");
                    }
                    break;
                  }
                  default: throw fatal_error("unknown cmd3");
                }
                break;
              }
              default: throw fatal_error("unknown cmd2");
            }
            break;
          }
          default: throw fatal_error("unknown cmd");
        }

        (*registers_set)[RI] += sizeof(instruction_t);
      }

      DEBUG_LOGGER_RISC("stack: '%s'", hex(stack).c_str());
    }
  }

};

int main() {
  std::string code = R"ASM(
    FUNCTION f1
      MULT R1 R1 R2
      MULT R4 R1 R5
    RET

    FUNCTION square
      MULT R1 R1 R2
    RET

    FUNCTION main
      SET R1 72623859790382856
      SET R5 0
      ; SET R6 -1
      SET R7 0
      ADDRESS RA square
      CALL RA
      SET R1 10
      ADD R2 R1 R1
    RET

    FUNCTION tmain
      SET R1 72623859790382856
      SET R2 2
      SET R3 3
      SET R4 4
      SET R5 34
      SET R6 14
      SET R7 12
      SET R8 10
      MULT R7 R1 R2
      MULT R9 R3 R4
      ADDRESS RA main
      CALL RA
    RET

    FUNCTION f2
      MULT R7 R1 R2
    RET
  )ASM";

  // std::cout << sizeof(interpreter_t::instruction_t) << std::endl;
  std::cout << code << std::endl;

  interpreter_t interpreter;
  interpreter.exec(code);

  return 0;
}


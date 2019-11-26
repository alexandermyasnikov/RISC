
#include <iostream>
#include <variant>
#include <regex>
#include <iomanip>

#include "debug_logger.h"

#define DEBUG_LOGGER_TRACE_LA            DEBUG_LOGGER("la   ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_LA(...)             DEBUG_LOG("la   ", logger_indent_risc_t::indent, __VA_ARGS__)

#define DEBUG_LOGGER_TRACE_SA            DEBUG_LOGGER("sa   ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_SA(...)             DEBUG_LOG("sa   ", logger_indent_risc_t::indent, __VA_ARGS__)

#define DEBUG_LOGGER_TRACE_ICG           DEBUG_LOGGER("icg  ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_ICG(...)            DEBUG_LOG("icg  ", logger_indent_risc_t::indent, __VA_ARGS__)

#define DEBUG_LOGGER_TRACE_CG            DEBUG_LOGGER("cg   ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_CG(...)             DEBUG_LOG("cg   ", logger_indent_risc_t::indent, __VA_ARGS__)

#define DEBUG_LOGGER_TRACE_EXEC          DEBUG_LOGGER("exec ", logger_indent_risc_t::indent)
#define DEBUG_LOGGER_EXEC(...)           DEBUG_LOG("exec ", logger_indent_risc_t::indent, __VA_ARGS__)

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

namespace risc_n {

  namespace utils_n {

    using data_t = std::vector<uint8_t>;

    struct fatal_error : std::runtime_error {
      fatal_error(const std::string& msg = "unknown error") : std::runtime_error(msg) { }
    };
  }



  namespace lexical_analyzer_n {

    using namespace utils_n;

    using lexeme_t = std::string;
    using lexemes_t = std::vector<lexeme_t>;

    struct rule_t {
      std::regex regex;
      std::function<lexeme_t(const std::string&)> get_lexeme;
    };

    using rules_t = std::vector<rule_t>;

    static inline rules_t rules = {
      {
        std::regex("\\s+|;.*?\n"),
        [](const std::string&) { return ""; }
      }, {
        std::regex("[\\w\\d_\\.-]+"),
        [](const std::string& str) { return str; }
      }
    };

    void process(lexemes_t& lexemes, const std::string& code) {
      DEBUG_LOGGER_TRACE_LA;

      std::string s = code;
      std::smatch m;
      while (!s.empty()) {
        for (const auto& rule : rules) {
          if (std::regex_search(s, m, rule.regex, std::regex_constants::match_continuous)) {
            lexeme_t lexeme = rule.get_lexeme(m.str());
            if (!lexeme.empty()) {
              lexemes.push_back(lexeme);
              DEBUG_LOGGER_LA("lexeme: '%s'", m.str().c_str());
            }
            s = m.suffix().str();
            break;
          }
        }
        if (m.empty() || !m.empty() && !m.length()) {
          DEBUG_LOGGER_LA("WARN: unexpected regex: '%s'", s.c_str());
          throw fatal_error("invalid regex");
          s.erase(s.begin());
        }
      }
    }
  }



  namespace syntax_analyzer_n {

    using namespace utils_n;
    using namespace lexical_analyzer_n;

    static inline std::map<std::string, size_t> cmds_args_count = {
      { "SET",  2},
      { "AND",  3},
      { "OR",   3},
      { "XOR",  3},
      { "ADD",  3},
      { "SUB",  3},
      { "MULT", 3},
      { "DIV",  3},
      { "LSH",  3},
      { "RSH",  3},

      { "BR",   2},
      { "NOT",  2},
      { "LOAD", 2},
      { "SAVE", 2},
      { "MOV",  2},

      { "CALL", 1},

      { "RET",  0},

      // Временные команды, которые будут преобразованы в другие
      { "FUNCTION", 1},
      { "LABEL",    1},
      { "ADDRESS",  2},
    };

    using cmds_str_t = std::vector<std::vector<std::string>>;

    void process(cmds_str_t& cmds_str, const lexemes_t& lexemes) {
      size_t i = 0;
      while (i < lexemes.size()) {
        auto lexeme = lexemes.at(i++);
        std::vector<std::string> cmd = { lexeme };
        DEBUG_LOGGER_SA("lexeme: '%s'", lexeme.c_str());
        if (cmds_args_count.find(lexeme) == cmds_args_count.end()) {
          throw fatal_error("unknown lexeme");
        }
        for (size_t j = 0; j < cmds_args_count[lexeme]; ++j) {
          auto lexeme_arg = lexemes.at(i++);
          cmd.push_back(lexeme_arg);
          DEBUG_LOGGER_SA("  lexeme_arg: '%s'", lexeme_arg.c_str());
        }
        cmds_str.push_back(cmd);
      }
    }
  }



  namespace semantic_analyzer_n {

  }



  namespace intermediate_code_generator_n {

    using namespace utils_n;
    using namespace syntax_analyzer_n;

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
        uint8_t  val;
      } __attribute__((packed)) cmd_set;
    };

    using instructions_t = std::vector<instruction_t>;

    struct opcode_index_t {
      uint8_t     offset;
      uint8_t     index;
      std::string name;
    };

    static inline std::vector<opcode_index_t> opcodes_table = {
      {  0,  0, "SET"  },
      {  0,  1, "AND"  },
      {  0,  2, "OR"   },
      {  0,  3, "XOR"  },
      {  0,  4, "ADD"  },
      {  0,  5, "SUB"  },
      {  0,  6, "MULT" },
      {  0,  7, "DIV"  },
      {  0,  8, "LSH"  },
      {  0,  9, "RSH"  },
      // ...
      {  0, 15, "OTH0" },
      {  1,  0, "BR"   },
      {  1,  1, "NOT"  },
      {  1,  2, "LOAD" },
      {  1,  3, "SAVE" },
      {  1,  4, "MOV"  },
      // ...
      {  1, 15, "OTH1" },
      {  2,  0, "CALL" },
      // ...
      {  2, 15, "OTH2" },
      {  3,  0, "RET"  },
      // ...
    };

    struct reg_index_t {
      uint8_t     index;
      std::string name;
    };

    static inline std::vector<reg_index_t> regs_table = {
      {   0, "RI"  },   // instruction pointer
      {   1, "RP"  },   // previous base pointer
      {   2, "RB"  },   // base pointer
      {   3, "RS"  },   // stack pointer
      {   4, "RF"  },   // flags
      {   5, "RT"  },   // tmp
      {   6, "RC"  },   // const
      {   7, "RA"  },   // args
      {   8, "R1"  },
      {   9, "R2"  },
      {  10, "R3"  },
      {  11, "R4"  },
      {  12, "R5"  },
      {  13, "R6"  },
      {  14, "R7"  },
      {  15, "R8"  },
    };

    using reg_value_t = int64_t;
    using reg_uvalue_t = std::make_unsigned<reg_value_t>::type;

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

    using functions_t = std::map<std::string, size_t>;

    uint8_t opcode_index(uint8_t offset, const std::string& name) {
      auto it = std::find_if(opcodes_table.begin(), opcodes_table.end(),
          [offset, name](auto& opcode) { return opcode.offset == offset && opcode.name == name; });
      if (it == opcodes_table.end()) {
        throw fatal_error("unknown opcode");
      }
      return it->index;
    }

    std::string opcode_name(uint8_t offset, uint8_t index) {
      auto it = std::find_if(opcodes_table.begin(), opcodes_table.end(),
          [offset, index](auto& opcode) { return opcode.offset == offset && opcode.index == index; });
      if (it == opcodes_table.end()) {
        throw fatal_error("unknown opcode");
      }
      return it->name;
    }

    uint8_t reg_index(const std::string& name) {
      auto it = std::find_if(regs_table.begin(), regs_table.end(),
          [name](auto& reg) { return reg.name == name; });
      if (it == regs_table.end()) {
        throw fatal_error("unknown reg");
      }
      return it->index;
    }

    std::string reg_name(uint8_t index) {
      auto it = std::find_if(regs_table.begin(), regs_table.end(),
          [index](auto& reg) { return reg.index == index; });
      if (it == regs_table.end()) {
        throw fatal_error("unknown reg");
      }
      return it->name;
    }

    std::string print_instruction(instruction_t instruction) {
      std::stringstream ss;

      ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(instruction.value))
          << instruction.value << "   ";

      if (instruction.cmd_set.op == opcode_index(0, "SET")) {
        ss << opcode_name(0, instruction.cmd_set.op) << " "
          << reg_name(instruction.cmd_set.rd) << " "
          << (reg_value_t) instruction.cmd_set.val << " ";
      } else {
        if (instruction.cmd.op == opcode_index(0, "OTH0")) {
          if (instruction.cmd.rd == opcode_index(1, "OTH1")) {
            if (instruction.cmd.rs1 == opcode_index(2, "OTH2")) {
                ss << opcode_name(3, instruction.cmd.rs2) << " ";
            } else {
              ss << opcode_name(2, instruction.cmd.rs1) << " "
                << reg_name(instruction.cmd.rs2) << " ";
            }
          } else {
            ss << opcode_name(1, instruction.cmd.rd) << " "
              << reg_name(instruction.cmd.rs1) << " "
              << reg_name(instruction.cmd.rs2) << " ";
          }
        } else {
          ss << opcode_name(0, instruction.cmd.op) << " "
            << reg_name(instruction.cmd.rd) << " "
            << reg_name(instruction.cmd.rs1) << " "
            << reg_name(instruction.cmd.rs2) << " ";
        }
      }

      return ss.str();
    };

    void macro_set(instructions_t& instructions, uint8_t rd, reg_value_t value) {
      DEBUG_LOGGER_TRACE_ICG;
      // DEBUG_LOGGER_ICG("rd: '%x'", rd);
      // DEBUG_LOGGER_ICG("value: '%ld'", value);

      uint8_t bytes[sizeof(value)];
      memcpy(bytes, &value, sizeof(value));
      std::reverse(std::begin(bytes), std::end(bytes));

      size_t i = 0;
      for (; i < sizeof(bytes); i++) {
        if (bytes[i])
          break;
      }

      instructions.push_back({ .cmd_set = { opcode_index(0, "SET"), rd, 0 } });

      auto rt = reg_index("RT");

      for (; i < sizeof(bytes); i++) {
        instructions.push_back({ .cmd_set = { opcode_index(0, "SET"), rt, 8 } });
        instructions.push_back({ .cmd     = { opcode_index(0, "LSH"), rd, rd, rt } });
        instructions.push_back({ .cmd_set = { opcode_index(0, "SET"), rt, bytes[i] } });
        instructions.push_back({ .cmd     = { opcode_index(0, "OR"),  rd, rd, rt } });
      }
    }

    void process(instructions_t& instructions, functions_t& functions, const cmds_str_t& cmds_str) {
      for (auto cmd_str : cmds_str) {
        if (cmd_str.at(0) == "SET" && cmd_str.size() == 3) {
          auto op = opcode_index(0, cmd_str.at(0));
          auto rd = reg_index(cmd_str.at(1));
          reg_value_t value = strtol(cmd_str.at(2).c_str(), nullptr, 0);
          macro_set(instructions, rd, value);

        } else if (cmd_str.at(0) == "FUNCTION" && cmd_str.size() == 2) {
          auto name = cmd_str.at(1);

          if (functions.find(name) != functions.end())
            throw fatal_error("function exists");

          functions[name] = instructions.size() * sizeof(instruction_t);

        } else if (cmd_str.at(0) == "LABEL") {
          throw fatal_error("LABEL TODO");

        } else if (cmd_str.at(0) == "ADDRESS" && cmd_str.size() == 3) {
          auto rd   = reg_index(cmd_str.at(1));
          auto name = cmd_str.at(2);

          if (functions.find(name) == functions.end())
            throw fatal_error("function not exists");

          macro_set(instructions, rd, functions[name]);

        } else if (cmd_str.size() == 4) {
          auto op  = opcode_index(0, cmd_str.at(0));
          auto rd  = reg_index(cmd_str.at(1));
          auto rs1 = reg_index(cmd_str.at(2));
          auto rs2 = reg_index(cmd_str.at(3));
          instructions.push_back({ .cmd  = { op, rd, rs1, rs2 } });

        } else if (cmd_str.size() == 3) {
          auto op1 = opcode_index(0, "OTH0");
          auto op2 = opcode_index(1, cmd_str.at(0));
          auto rd  = reg_index(cmd_str.at(1));
          auto rs  = reg_index(cmd_str.at(2));
          instructions.push_back({ .cmd  = { op1, op2, rd, rs } });

        } else if (cmd_str.size() == 2) {
          auto op1 = opcode_index(0, "OTH0");
          auto op2 = opcode_index(1, "OTH1");
          auto op3 = opcode_index(2, cmd_str.at(0));
          auto rd  = reg_index(cmd_str.at(1));
          instructions.push_back({ .cmd  = { op1, op2, op3, rd } });

        } else if (cmd_str.size() == 1) {
          auto op1 = opcode_index(0, "OTH0");
          auto op2 = opcode_index(1, "OTH1");
          auto op3 = opcode_index(2, "OTH2");
          auto op4 = opcode_index(3, cmd_str.at(0));
          instructions.push_back({ .cmd  = { op1, op2, op3, op4 } });

        } else {
          throw fatal_error("unknown cmd format");
        }
      }

      for (size_t i = 0; i < instructions.size(); ++i) {
        DEBUG_LOGGER_ICG("instruction: %08x '%s'", i * sizeof(instruction_t), print_instruction(instructions[i]).c_str());
      }
    }
  }



  namespace code_optimizer_n {
  }



  namespace code_generator_n {
    using namespace intermediate_code_generator_n;

    void process(data_t& text, const instructions_t& instructions) {
      text.assign(sizeof(instruction_t) * instructions.size(), 0);
      for (size_t i = 0; i < instructions.size(); ++i) {
        memcpy(text.data() + i * sizeof(instruction_t), &instructions[i].value, sizeof(instruction_t));
      }

      for (size_t i = 0; i < text.size(); i += sizeof(instruction_t)) {
        DEBUG_LOGGER_CG("text: '%02hhx%02hhx'", text.at(i), text.at(i + 1));
      }
    }
  }



  namespace executor_n {

    using namespace intermediate_code_generator_n;

    using registers_set_t = reg_value_t[16];

    std::string print_stack(const data_t& stack, registers_set_t* registers_set) {
      std::stringstream ss;
      ss << std::endl;

      for (uint8_t i = 0; i < 16; ++i) {
        ss << reg_name(i) << "   " << std::hex << std::setfill('0') << std::setw(2 * sizeof(reg_value_t))
          << (*registers_set)[i] << std::endl;
      }

      ss << "size: " << (reinterpret_cast<const uint8_t*>(registers_set) - reinterpret_cast<const uint8_t*>(stack.data())) << std::endl;

      for (size_t i = (*registers_set)[reg_index("RB")]; i < (*registers_set)[reg_index("RS")]; ++i) {
        ss << "stack data: " << std::hex << std::setfill('0') << std::setw(2 * sizeof(uint8_t))
          << (uint64_t) stack.at(i) << std::endl;
      }

      return ss.str();
    }

    void exec_cmd3(const data_t& text, data_t& stack, registers_set_t*& registers_set, instruction_t instruction) {
      if (instruction.cmd.rs2 == opcode_index(3, "RET")) {
        if (!(*registers_set)[reg_index("RP")])
          throw fatal_error("exit TODO");
        registers_set_t* registers_set_new = reinterpret_cast<registers_set_t*>(stack.data()
            + (*registers_set)[reg_index("RP")] - sizeof(registers_set_t));
        registers_set = registers_set_new;
      } else {
        throw fatal_error("unknown cmd3");
      }
    }

    void exec_cmd2(const data_t& text, data_t& stack, registers_set_t*& registers_set, instruction_t instruction) {
      if (instruction.cmd.rs1 == opcode_index(2, "CALL")) {
        registers_set_t* registers_set_new = reinterpret_cast<registers_set_t*>(stack.data()
            + (*registers_set)[reg_index("RS")]);
        (*registers_set_new)[reg_index("RI")] = (*registers_set)[instruction.cmd.rs2];
        (*registers_set_new)[reg_index("RP")] = (*registers_set)[reg_index("RB")];
        (*registers_set_new)[reg_index("RB")] = (*registers_set)[reg_index("RS")] + sizeof(registers_set_t);
        (*registers_set_new)[reg_index("RS")] = (*registers_set_new)[reg_index("RB")];
        registers_set = registers_set_new;
      } else if (instruction.cmd.rs1 == opcode_index(2, "OTH2")) {
        exec_cmd3(text, stack, registers_set, instruction);
      } else {
        throw fatal_error("unknown cmd2");
      }
    }

    void exec_cmd1(const data_t& text, data_t& stack, registers_set_t*& registers_set, instruction_t instruction) {
      if (instruction.cmd.rd == opcode_index(1, "BR")) {
        throw fatal_error("BR TODO");
      } else if (instruction.cmd.rd == opcode_index(1, "NOT")) {
        (*registers_set)[instruction.cmd.rs1] = ~(*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.rd == opcode_index(1, "LOAD")) {
        throw fatal_error("LOAD TODO");
      } else if (instruction.cmd.rd == opcode_index(1, "SAVE")) {
        throw fatal_error("SAVE TODO");
      } else if (instruction.cmd.rd == opcode_index(1, "MOV")) {
        (*registers_set)[instruction.cmd.rs1] = (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.rd == opcode_index(1, "OTH1")) {
        exec_cmd2(text, stack, registers_set, instruction);
      } else {
        throw fatal_error("unknown cmd1");
      }
    }

    void exec_cmd0(const data_t& text, data_t& stack, registers_set_t*& registers_set, instruction_t instruction) {
      if (instruction.cmd.op == opcode_index(0, "SET")) {
        (*registers_set)[instruction.cmd_set.rd] = instruction.cmd_set.val;
      } else if (instruction.cmd.op == opcode_index(0, "AND")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] & (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "OR")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] | (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "XOR")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] ^ (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "AND")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] + (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "SUB")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] - (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "MULT")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] * (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "DIV")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] / (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "LSH")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] << (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "RSH")) {
        (*registers_set)[instruction.cmd.rd] =
          (*registers_set)[instruction.cmd.rs1] >> (*registers_set)[instruction.cmd.rs2];
      } else if (instruction.cmd.op == opcode_index(0, "OTH0")) {
        exec_cmd1(text, stack, registers_set, instruction);
      } else {
        throw fatal_error("unknown cmd0");
      }
    }

    void process(const data_t& text, const functions_t& functions) {
      DEBUG_LOGGER_TRACE_EXEC;

      if (functions.find("__start") == functions.end())
        throw fatal_error("__start not exists");

      data_t stack(0xFFFF, 0);

      registers_set_t* registers_set = reinterpret_cast<registers_set_t*>(stack.data());
      (*registers_set)[reg_index("RP")] = 0;
      (*registers_set)[reg_index("RI")] = functions.at("__start");
      (*registers_set)[reg_index("RB")] = sizeof(registers_set_t);
      (*registers_set)[reg_index("RS")] = (*registers_set)[reg_index("RB")];

      DEBUG_LOGGER_EXEC("stack frame: '%s'", print_stack(stack, registers_set).c_str());

      while (true) {
        instruction_t instruction = *reinterpret_cast<const instruction_t*>(text.data() + (*registers_set)[reg_index("RI")]);
        exec_cmd0(text, stack, registers_set, instruction);

        DEBUG_LOGGER_EXEC("instruction: '%s'", print_instruction(instruction).c_str());
        DEBUG_LOGGER_EXEC("stack frame: '%s'", print_stack(stack, registers_set).c_str());

        (*registers_set)[reg_index("RI")] += sizeof(instruction_t);
      }

    }
  }
}


struct interpreter_t {

  struct fatal_error : std::runtime_error {
    fatal_error(const std::string& msg = "unknown error") : std::runtime_error(msg) { }
  };

  void exec(const std::string code) {
    using namespace risc_n;

    lexical_analyzer_n::lexemes_t lexemes;
    lexical_analyzer_n::process(lexemes, code);

    syntax_analyzer_n::cmds_str_t cmds_str;
    syntax_analyzer_n::process(cmds_str, lexemes);

    intermediate_code_generator_n::instructions_t instructions;
    intermediate_code_generator_n::functions_t functions;
    intermediate_code_generator_n::process(instructions, functions, cmds_str);

    utils_n::data_t text;
    code_generator_n::process(text, instructions);

    executor_n::process(text, functions);
  }
};

int main() {
  std::string code = R"ASM(
    FUNCTION f1
      MULT R1 R1 R2
      MULT R4 R1 R5
    RET

    FUNCTION square
      XOR R2 R3 R4
    RET

    FUNCTION main
      SET R1 72623859790382856
      MOV R2 R1
      MOV R3 R1
      MOV R4 R1
      ADDRESS RA square
      CALL RA
      SET R1 10
      MULT R2 R1 R1
    RET

    FUNCTION __start
      SET R1 72623859790382856
      SET R2 2
      SET R3 3
      SET R4 16
      SET R5 34
      SET R6 14
      SET R7 12
      SET R8 10
      MOV R8 R1
      DIV R2 R1 R4
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



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



template <typename T>
std::string hex(const T& buffer) {
  std::stringstream ss;
  ss << "{ ";
  for (const auto b : buffer) {
    ss << std::hex << std::setfill('0') << std::setw(2 * sizeof(uint64_t)) << static_cast<uint64_t>(b.value) << " ";
  }
  ss << "} " << std::dec << buffer.size() << "s";
  return ss.str();
}

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
    uint64_t value;
    struct {
      uint8_t  cmd;
      uint8_t  unused[4];
      uint8_t  rd;
      uint8_t  rs1;
      uint8_t  rs2;
    } __attribute__((packed)) cmd3;
    struct {
      uint8_t  cmd;
      uint8_t  unused[5];
      uint8_t  rd;
      uint8_t  rs;
    } __attribute__((packed)) cmd2;
    struct {
      uint8_t  cmd;
      uint8_t  unused[2];
      uint8_t  rd;
      uint32_t val;
    } __attribute__((packed)) cmd_set;
  };

  static inline uint8_t SET  = 0x01;
  static inline uint8_t OR   = 0x02;
  static inline uint8_t AND  = 0x03;
  static inline uint8_t XOR  = 0x04;
  static inline uint8_t ADD  = 0x05;
  static inline uint8_t SUB  = 0x06;
  static inline uint8_t MULT = 0x07;
  static inline uint8_t DIV  = 0x08;

  static inline uint8_t BR   = 0x09;
  static inline uint8_t NOT  = 0x0a;
  static inline uint8_t LSH  = 0x0b;
  static inline uint8_t RSH  = 0x0c;
  static inline uint8_t LD   = 0x0d;
  static inline uint8_t SV   = 0x0e;
  static inline uint8_t MOV  = 0x0f;

  struct fatal_error : std::runtime_error {
    fatal_error(const std::string& msg = "unknown error") : std::runtime_error(msg) { }
  };

  uint8_t register_index(const std::string& name) {
    static std::map<std::string, uint8_t> registers = {
      { "RIP", 0 },
      { "RSP", 1 },
      { "RFP", 2 },
      { "RF",  3 },
      { "RT",  4 },
      { "RA",  5 },
      { "RES", 6 },
      { "RA1", 7 },
      { "RA2", 8 },
      { "RA3", 9 },
      { "RA4", 10 },
      { "RA5", 11 },
      { "RA6", 12 },
      { "RT1", 13 },
      { "RT2", 14 },
      { "RT3", 15 },
    };
    if (registers.find(name) == registers.end())
      throw fatal_error("unknown register");
    return registers[name];
  }

  void macro_set(std::vector<instruction_t>& instructions, uint8_t rd, int64_t value) {
    DEBUG_LOGGER_TRACE_RISC;
    DEBUG_LOGGER_RISC("rd: '%x'", rd);
    DEBUG_LOGGER_RISC("value: '%ld'", value);
    uint32_t val_high = value >> 32;
    uint32_t val_low  = value & 0xFFFFFFFF;
    uint8_t rt = register_index("RT");

    instructions.push_back({ .cmd_set = { SET, {}, rd, val_high } });
    instructions.push_back({ .cmd_set = { SET, {}, rt, 32 } });
    instructions.push_back({ .cmd3    = { LSH, {}, rd, rd, rt } });

    instructions.push_back({ .cmd_set = { SET, {}, rd, val_low } });
    instructions.push_back({ .cmd3    = { OR,  {}, rd, rd, rt } });
  }

  void parse(const std::string code) {
    DEBUG_LOGGER_TRACE_RISC;
    std::vector<lexeme_t> lexemes;

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

    std::map<std::string, size_t> functions;
    std::vector<instruction_t> instructions;

    size_t i = 0;
    while (i < lexemes.size()) {
      std::string lexeme = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
      DEBUG_LOGGER_RISC("lexeme: '%s'", lexeme.c_str());

      if (lexeme == "FUNCTION") {
        auto name = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        if (functions.find(name) != functions.end())
          throw fatal_error("function exists");
        functions[name] = instructions.size();
        DEBUG_LOGGER_RISC("function: '%zd', '%s'", functions[name], name.c_str());

      } else if (lexeme == "MULT") {
        auto rd = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        auto rs1 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        auto rs2 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        instructions.push_back({ .cmd3 = { MULT, {}, rd, rs1, rs2 } });

      } else if (lexeme == "ADD") {
        auto rd = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        auto rs1 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        auto rs2 = register_index(std::get<lexeme_ident_t>(lexemes.at(i++)).value);
        instructions.push_back({ .cmd3 = { ADD, {}, rd, rs1, rs2 } });

      } else if (lexeme == "RET") {
        // TODO
        // instructions.push_back({ .cmd2 = { RET, {}, rd, rs1 } });

      } else if (lexeme == "SET") {
        auto arg1 = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        auto arg2 = std::get<lexeme_integer_t>(lexemes.at(i++)).value;
        macro_set(instructions, register_index(arg1), arg2);

      } else if (lexeme == "CALL") {
        auto name = std::get<lexeme_ident_t>(lexemes.at(i++)).value;
        if (functions.find(name) == functions.end())
          throw fatal_error("function not exists");

        macro_set(instructions, register_index("RA"), functions[name]);
        // TODO
        // CALL(Ra, name)   set(Ra, functions[name]); CALL(Ra);

      } else {
        DEBUG_LOGGER_RISC("WARN: unknown lexeme: '%s'", lexeme.c_str());
      }
    }

    DEBUG_LOGGER_RISC("buff: '%s'", hex(instructions).c_str());
  }

  void exec() {
  }

};

int main() {
  std::string code = R"ASM(
    FUNCTION ftest
      MULT RES RA1 RA1
    RET

    FUNCTION square
      MULT RES RA1 RA1
    RET

    FUNCTION main
      SET RA1 72623859790382856
      CALL square
      SET RT1 10
      ADD RT2 RES RT1
    RET
  )ASM";

  interpreter_t interpreter;
  interpreter.parse(code);
  interpreter.exec();

  return 0;
}


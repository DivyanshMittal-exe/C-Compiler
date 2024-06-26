#ifndef AST_enums_HPP
#define AST_enums_HPP

#include <string>

using namespace std;

enum class NodeType {
  Unimplemented,
  TranslationUnit,
  FunctionDefinition,
  Expression,
  DeclarationSpecifiers,
  Specifier,
  CompoundStatement,
  LabelStatement,
  CaseLabelStatement,
  DefaultLabelStatement,
  IfElseStatement,
  IfStatement,
  SwitchStatement,
  WhileStatement,
  DoWhileStatement,
  ForStatement,
  GotoStatement,
  ContinueStatement,
  BreakStatement,
  ReturnStatement,
  DeclarationList,
  Declaration,
  InitDeclartor,
  InitDeclartorList,
  Declarator,
  Pointer,
  ArrayDeclarator,
  FunctionDeclator,
  Identifier,
  IdentifierList,
  ParameterList,
  ParameterDeclaration,
  ExpressionList,
  AssignmentExpression,
  ArrayAccess,
  FunctionCall,
  ArgumentExpressionList,
  UnaryExpressionNode,
  MemberAccess,
  PostfixExpression,
  InitializerList,
  ConditionalExpression,
  LogicalOrExpression,
  LogicalAndExpression,
  InclusiveOrExpression,
  ExclusiveOrExpression,
  AndExpression,
  EqualityExpression,
  NonEqualityExpression,
  LessThanExpression,
  GreaterThanExpression,
  LessOrEqualExpression,
  GreaterOrEqualExpression,
  LeftShiftExpression,
  RightShiftExpression,
  AdditiveExpression,
  SubExpression,
  MultiplicativeExpression,
  DivExpression,
  ModExpression,
  IConstant,
  FConstant,
  String
};

enum class SpecifierEnum {
  TYPEDEF,
  EXTERN,
  STATIC,
  THREAD_LOCAL,
  AUTO,
  REGISTER,
  VOID,
  CHAR,
  SHORT,
  INT,
  LONG,
  FLOAT,
  DOUBLE,
  SIGNED,
  UNSIGNED,
  BOOL,
  COMPLEX,
  IMAGINARY,
  ATOMIC_TYPE_SPECIFIER,
  STRUCT_OR_UNION_SPECIFIER,
  ENUM_SPECIFIER,
  TYPEDEF_NAME,
  CONST,
  RESTRICT,
  VOLATILE,
  ATOMIC,
  NORETURN,
  INLINE
};

enum class AssignmentOperator {
  ASSIGN,       // '='
  MUL_ASSIGN,   // '*='
  DIV_ASSIGN,   // '/='
  MOD_ASSIGN,   // '%='
  ADD_ASSIGN,   // '+='
  SUB_ASSIGN,   // '-='
  LEFT_ASSIGN,  // '<<='
  RIGHT_ASSIGN, // '>>='
  AND_ASSIGN,   // '&='
  XOR_ASSIGN,   // '^='
  OR_ASSIGN     // '|='
};

enum class UnaryOperator {
  INC_OP,      // '++'
  DEC_OP,      // '--'
  SIZEOF,      // 'sizeof'
  ALIGNOF,     // '_Alignof'
  ADDRESS_OF,  // '&'
  MUL_OP,      // '*'
  PLUS,        // '+'
  MINUS,       // '-'
  BITWISE_NOT, // '~'
  LOGICAL_NOT  // '!'

};

static std::string unaryOperatorToString(UnaryOperator op) {
  switch (op) {
  case UnaryOperator::INC_OP:
    return "++";
  case UnaryOperator::DEC_OP:
    return "--";
  case UnaryOperator::SIZEOF:
    return "sizeof";
  case UnaryOperator::ALIGNOF:
    return "Alignof";
  case UnaryOperator::ADDRESS_OF:
    return "&";
  case UnaryOperator::MUL_OP:
    return "*";
  case UnaryOperator::PLUS:
    return "+";
  case UnaryOperator::MINUS:
    return "-";
  case UnaryOperator::BITWISE_NOT:
    return "~";
  case UnaryOperator::LOGICAL_NOT:
    return "!";
  default:
    return "Unknown"; // Handle the case where the enum value is not recognized
  }
}

static std::string assignmentOperatorToString(AssignmentOperator op) {
  switch (op) {
  case AssignmentOperator::ASSIGN:
    return "=";
  case AssignmentOperator::MUL_ASSIGN:
    return "*=";
  case AssignmentOperator::DIV_ASSIGN:
    return "/=";
  case AssignmentOperator::MOD_ASSIGN:
    return "%=";
  case AssignmentOperator::ADD_ASSIGN:
    return "+=";
  case AssignmentOperator::SUB_ASSIGN:
    return "-=";
  case AssignmentOperator::LEFT_ASSIGN:
    return "<<=";
  case AssignmentOperator::RIGHT_ASSIGN:
    return ">>=";
  case AssignmentOperator::AND_ASSIGN:
    return "&=";
  case AssignmentOperator::XOR_ASSIGN:
    return "^=";
  case AssignmentOperator::OR_ASSIGN:
    return "|=";
  default:
    return "Unknown"; // Handle the case where the enum value is not recognized
  }
}

static std::string specifierEnumToString(SpecifierEnum specEnum) {
  switch (specEnum) {
  case SpecifierEnum::TYPEDEF:
    return "TYPEDEF";
  case SpecifierEnum::EXTERN:
    return "EXTERN";
  case SpecifierEnum::STATIC:
    return "STATIC";
  case SpecifierEnum::THREAD_LOCAL:
    return "THREAD_LOCAL";
  case SpecifierEnum::AUTO:
    return "AUTO";
  case SpecifierEnum::REGISTER:
    return "REGISTER";
  case SpecifierEnum::VOID:
    return "VOID";
  case SpecifierEnum::CHAR:
    return "CHAR";
  case SpecifierEnum::SHORT:
    return "SHORT";
  case SpecifierEnum::INT:
    return "INT";
  case SpecifierEnum::LONG:
    return "LONG";
  case SpecifierEnum::FLOAT:
    return "FLOAT";
  case SpecifierEnum::DOUBLE:
    return "DOUBLE";
  case SpecifierEnum::SIGNED:
    return "SIGNED";
  case SpecifierEnum::UNSIGNED:
    return "UNSIGNED";
  case SpecifierEnum::BOOL:
    return "BOOL";
  case SpecifierEnum::COMPLEX:
    return "COMPLEX";
  case SpecifierEnum::IMAGINARY:
    return "IMAGINARY";
  case SpecifierEnum::ATOMIC_TYPE_SPECIFIER:
    return "ATOMIC_TYPE_SPECIFIER";
  case SpecifierEnum::STRUCT_OR_UNION_SPECIFIER:
    return "STRUCT_OR_UNION_SPECIFIER";
  case SpecifierEnum::ENUM_SPECIFIER:
    return "ENUM_SPECIFIER";
  case SpecifierEnum::TYPEDEF_NAME:
    return "TYPEDEF_NAME";
  case SpecifierEnum::CONST:
    return "CONST";
  case SpecifierEnum::RESTRICT:
    return "RESTRICT";
  case SpecifierEnum::VOLATILE:
    return "VOLATILE";
  case SpecifierEnum::ATOMIC:
    return "ATOMIC";
  case SpecifierEnum::NORETURN:
    return "NORETURN";
  case SpecifierEnum::INLINE:
    return "INLINE";
  default:
    return "UNKNOWN";
  }
}

static std::string nodeTypeToString(NodeType type) {
  switch (type) {
  case NodeType::Unimplemented:
    return "Unimplemented";
  case NodeType::TranslationUnit:
    return "TranslationUnit";
  case NodeType::FunctionDefinition:
    return "FunctionDefinition";
  case NodeType::Expression:
    return "Expression";
  case NodeType::DeclarationSpecifiers:
    return "DeclarationSpecifiers";
  case NodeType::Specifier:
    return "Specifier";
  case NodeType::CompoundStatement:
    return "CompoundStatement";
  case NodeType::LabelStatement:
    return "LabelStatement";
  case NodeType::CaseLabelStatement:
    return "CaseLabelStatement";
  case NodeType::DefaultLabelStatement:
    return "DefaultLabelStatement";
  case NodeType::IfElseStatement:
    return "IfElseStatement";
  case NodeType::IfStatement:
    return "IfStatement";
  case NodeType::SwitchStatement:
    return "SwitchStatement";
  case NodeType::WhileStatement:
    return "WhileStatement";
  case NodeType::DoWhileStatement:
    return "DoWhileStatement";
  case NodeType::ForStatement:
    return "ForStatement";
  case NodeType::GotoStatement:
    return "GotoStatement";
  case NodeType::ContinueStatement:
    return "ContinueStatement";
  case NodeType::BreakStatement:
    return "BreakStatement";
  case NodeType::ReturnStatement:
    return "ReturnStatement";
  case NodeType::DeclarationList:
    return "DeclarationList";
  case NodeType::Declaration:
    return "Declaration";
  case NodeType::InitDeclartor:
    return "InitDeclartor";
  case NodeType::InitDeclartorList:
    return "InitDeclartorList";
  case NodeType::Declarator:
    return "Declarator";
  case NodeType::Pointer:
    return "Pointer";
  case NodeType::ArrayDeclarator:
    return "ArrayDeclarator";
  case NodeType::FunctionDeclator:
    return "FunctionDeclator";
  case NodeType::Identifier:
    return "Identifier";
  case NodeType::IdentifierList:
    return "IdentifierList";
  case NodeType::ParameterList:
    return "ParameterList";
  case NodeType::ParameterDeclaration:
    return "ParameterDeclaration";
  case NodeType::ExpressionList:
    return "ExpressionList";
  case NodeType::AssignmentExpression:
    return "AssignmentExpression";
  case NodeType::ArrayAccess:
    return "ArrayAccess";
  case NodeType::FunctionCall:
    return "FunctionCall";
  case NodeType::ArgumentExpressionList:
    return "ArgumentExpressionList";
  case NodeType::UnaryExpressionNode:
    return "UnaryExpressionNode";
  case NodeType::MemberAccess:
    return "MemberAccess";
  case NodeType::PostfixExpression:
    return "PostfixExpression";
  case NodeType::ConditionalExpression:
    return "ConditionalExpression";
  case NodeType::InitializerList:
    return "InitializerList";
  case NodeType::LogicalOrExpression:
    return "LogicalOrExpression";
  case NodeType::LogicalAndExpression:
    return "LogicalAndExpression";
  case NodeType::InclusiveOrExpression:
    return "InclusiveOrExpression";
  case NodeType::ExclusiveOrExpression:
    return "ExclusiveOrExpression";
  case NodeType::AndExpression:
    return "AndExpression";
  case NodeType::EqualityExpression:
    return "EqualityExpression";
  case NodeType::NonEqualityExpression:
    return "NonEqualityExpression";
  case NodeType::LessThanExpression:
    return "LessThanExpression";
  case NodeType::GreaterThanExpression:
    return "GreaterThanExpression";
  case NodeType::LessOrEqualExpression:
    return "LessOrEqualExpression";
  case NodeType::GreaterOrEqualExpression:
    return "GreaterOrEqualExpression";
  case NodeType::LeftShiftExpression:
    return "LeftShiftExpression";
  case NodeType::RightShiftExpression:
    return "RightShiftExpression";
  case NodeType::AdditiveExpression:
    return "AdditiveExpression";
  case NodeType::SubExpression:
    return "SubExpression";
  case NodeType::MultiplicativeExpression:
    return "MultiplicativeExpression";
  case NodeType::DivExpression:
    return "DivExpression";
  case NodeType::ModExpression:
    return "ModExpression";
  case NodeType::IConstant:
    return "IConstant";
  case NodeType::FConstant:
    return "FConstant";
  case NodeType::String:
    return "String";
  default:
    return "UnknownNodeType"; // Handle the case where the enum value is not
                              // recognized
  }
}

// static string formatSpacing(int depth) {
//     string result = "";
//     for (int i = 0; i < depth; i++) {
//         result += "  ";
//     }
//     return result;
// }

static std::string formatSpacing(int depth) {
  std::string result = "";
  for (int i = 0; i < depth; i++) {
    switch (i % 5) { // Using 5 different colors
    case 0:
      result += "\033[38;2;1;22;30m";
      break;
    case 1:
      result += "\033[38;2;18;69;89m";
      break;
    case 2:
      result += "\033[38;2;89;131;146m";
      break;
    case 3:
      result += "\033[38;2;174;195;176m";
      break;
    case 4:
      result += "\033[38;2;239;246;224m";
      break;
    }
    result += i == depth - 1 ? " ┣━" : " ┃ ";
  }
  // Reset color to default
  result += "\033[0m";
  return result;
}

static std::string convertRawString(const std::string &raw) {
  std::string result;
  bool escape = false;

  for (size_t i = 1; i < raw.size() - 1; ++i) {
    char c = raw[i];
    if (escape) {
      switch (c) {
      case '\\':
        result += '\\';
        break;
      case '\'':
        result += '\'';
        break;
      case '\"':
        result += '\"';
        break;
      case 'n':
        result += '\n';
        break;
      case 'r':
        result += '\r';
        break;
      case 't':
        result += '\t';
        break;
      case 'b':
        result += '\b';
        break;
      case 'f':
        result += '\f';
        break;
      case 'v':
        result += '\v';
        break;
      case '0':
        result += '\0';
        break;
      case 'a':
        result += '\a';
        break;
      case '?':
        result += '\?';
        break;
      case 'x': {
        // Hexadecimal representation
        std::string hex;
        while (isxdigit(raw[i + 1])) {
          hex += raw[++i];
        }
        result += static_cast<char>(std::stoi(hex, nullptr, 16));
        break;
      }
      case 'u': {
        // Unicode representation (exactly 4 hexadecimal digits)
        std::string hex;
        for (int j = 0; j < 4 && isxdigit(raw[i + 1]); ++j) {
          hex += raw[++i];
        }
        result += static_cast<char>(std::stoi(hex, nullptr, 16));
        break;
      }
      case 'U': {
        // Unicode representation (exactly 8 hexadecimal digits)
        std::string hex;
        for (int j = 0; j < 8 && isxdigit(raw[i + 1]); ++j) {
          hex += raw[++i];
        }
        result += static_cast<char>(std::stoi(hex, nullptr, 16));
        break;
      }
      default:
        result += c;
        break;
      }
      escape = false;
    } else if (c == '\\') {
      escape = true;
    } else {
      result += c;
    }
  }

  return result;
}

#endif // AST_enums_HPP

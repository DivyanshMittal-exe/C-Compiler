#ifndef AST_HPP
#define AST_HPP

#include "AST_enums.hpp"
#include "codegen.h"
#include "scoper.h"
#include <cstdint>
#include <iostream>
#include <llvm-14/llvm/IR/BasicBlock.h>
#include <llvm-14/llvm/IR/Constants.h>
#include <llvm-14/llvm/IR/DerivedTypes.h>
#include <llvm-14/llvm/IR/Function.h>
#include <llvm-14/llvm/IR/IRBuilder.h>
#include <llvm-14/llvm/IR/Instructions.h>
#include <llvm-14/llvm/IR/LLVMContext.h>
#include <llvm-14/llvm/IR/Type.h>
#include <llvm-14/llvm/IR/Value.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using namespace std;

using llvm::Value;

static ScoperStack scoperStack;
static CodeGenerator codeGenerator;

static map<string, llvm::BasicBlock *> labels;

static llvm::Type *declaration_type = nullptr;
static vector<llvm::Type *> function_params;

static int parameter_list_index = 0;

static bool get_as_lvalue = false;

static bool global_is_variadic = false;

static bool am_i_initialising = false;

static llvm::BasicBlock *loop_block = nullptr;
static llvm::BasicBlock *merge_block = nullptr;

static llvm::Type *array_type = nullptr;

static bool is_declaration_global = false;

static SpecifierEnum dec_type_for_optimisation;

static bool constant_prop = false;

static void make_bool(llvm::Value *&conditionValue) {

  if (conditionValue->getType()->getTypeID() == llvm::Type::IntegerTyID) {
    llvm::Value *zero = llvm::ConstantInt::get(conditionValue->getType(), 0);
    conditionValue = codeGenerator.getBuilder().CreateICmpNE(conditionValue,
                                                             zero, "make_bool");
  } else if (conditionValue->getType()->getTypeID() == llvm::Type::DoubleTyID) {
    llvm::Value *zero = llvm::ConstantFP::get(conditionValue->getType(), 0);
    conditionValue = codeGenerator.getBuilder().CreateFCmpONE(
        conditionValue, zero, "make_bool");
  }
}

static void make_store_compatible(llvm::Value *&lhsAddr,
                                  llvm::Value *&rhsValue) {

  if (lhsAddr->getType()->isPointerTy()) {

    if (rhsValue->getType()->isIntegerTy() and
        lhsAddr->getType()->getPointerElementType()->isIntegerTy()) {
      unsigned lhsSize =
          lhsAddr->getType()->getPointerElementType()->getPrimitiveSizeInBits();
      unsigned rhsSize = rhsValue->getType()->getIntegerBitWidth();
      if (lhsSize != rhsSize) {
        // Perform size adjustment if types are integers but of different
        // sizes
        if (lhsSize < rhsSize) {
          // Truncate rhsValue
          rhsValue = codeGenerator.getBuilder().CreateTrunc(
              rhsValue, llvm::IntegerType::get(lhsAddr->getContext(), lhsSize));
        } else {
          // Extend rhsValue
          rhsValue = codeGenerator.getBuilder().CreateZExt(
              rhsValue,
              llvm ::IntegerType::get(lhsAddr->getContext(), lhsSize));
        }
        cout << "Adjusted RHSValue size to match the type of lhsAddr" << endl;
      }
    } else if (lhsAddr->getType()->getPointerElementType()->isDoubleTy() and
               rhsValue->getType()->isIntegerTy()) {
      rhsValue = codeGenerator.getBuilder().CreateSIToFP(
          rhsValue, lhsAddr->getType()->getPointerElementType());
    } else if (lhsAddr->getType()->getPointerElementType()->isIntegerTy() and
               rhsValue->getType()->isDoubleTy()) {
      rhsValue = codeGenerator.getBuilder().CreateFPToSI(
          rhsValue, lhsAddr->getType()->getPointerElementType());
    }
  }

  if (lhsAddr->getType()->isPointerTy()) {
    if (rhsValue->getType() != lhsAddr->getType()->getPointerElementType()) {
      // Perform type casting of rhsValue to match the type of lhsAddr
      cout << "Casting rhsValue to match the type of lhsAddr" << endl;
      rhsValue = codeGenerator.getBuilder().CreateBitOrPointerCast(
          rhsValue, lhsAddr->getType()->getPointerElementType());
    }
  }
}

static void make_lhs_rhs_compatible(llvm::Value *&lhs, llvm::Value *&rhs) {

  bool use_float = false;

  use_float |= (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID);
  use_float |= (rhs->getType()->getTypeID() == llvm::Type::DoubleTyID);

  if (use_float) {
    if (lhs->getType()->getTypeID() == llvm::Type::IntegerTyID) {
      lhs = codeGenerator.getBuilder().CreateSIToFP(
          lhs, llvm::Type::getDoubleTy(codeGenerator.getContext()), "lhscast");
    }
    if (rhs->getType()->getTypeID() == llvm::Type::IntegerTyID) {
      rhs = codeGenerator.getBuilder().CreateSIToFP(
          rhs, llvm::Type::getDoubleTy(codeGenerator.getContext()), "rhscast");
    }
    return;
  }

  llvm::IntegerType *lhs_int_type =
      llvm::dyn_cast<llvm::IntegerType>(lhs->getType());
  llvm::IntegerType *rhs_int_type =
      llvm::dyn_cast<llvm::IntegerType>(rhs->getType());

  if (lhs_int_type && rhs_int_type &&
      lhs_int_type->getBitWidth() != rhs_int_type->getBitWidth()) {

    llvm::Type *extendedType =
        lhs_int_type->getBitWidth() > rhs_int_type->getBitWidth()
            ? lhs_int_type
            : rhs_int_type;
    if (lhs_int_type->getBitWidth() < rhs_int_type->getBitWidth()) {

      if (lhs_int_type->getBitWidth() == 1) {
        lhs =
            codeGenerator.getBuilder().CreateZExt(lhs, extendedType, "lhsext");
      } else {

        lhs =
            codeGenerator.getBuilder().CreateSExt(lhs, extendedType, "lhsext");
      }

    } else {
      if (rhs_int_type->getBitWidth() == 1) {
        rhs =
            codeGenerator.getBuilder().CreateZExt(rhs, extendedType, "rhsext");
      } else {
        rhs =
            codeGenerator.getBuilder().CreateSExt(rhs, extendedType, "rhsext");
      }
    }
  }
  if (lhs->getType() != rhs->getType()) {
    rhs =
        codeGenerator.getBuilder().CreateBitOrPointerCast(rhs, lhs->getType());
  }
}

class ASTNode {
public:
  ASTNode(NodeType type) : type(type) {}

  virtual ~ASTNode() {}

  /* ASTNode(const ASTNode &) = delete; */

  NodeType getNodeType() const { return type; }

  virtual string dump_ast(int depth = 0) const = 0;
  void addChild(ASTNode *child) { children.push_back(child); }

  vector<ASTNode *> getChildren() { return children; }

  ASTNode *disable_constant_prop() {

    constant_prop = false;
    codeGenerator.constant_prop = false;
    return optimise();
  }

  ASTNode *enable_constant_prop() {
    constant_prop = true;
    codeGenerator.constant_prop = true;
    return optimise();
  }

  virtual bool check_semantics() {

    std::cerr << "Checking Semantics for " << nodeTypeToString(type)
              << "using base" << std::endl;
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }

  virtual m_Value get() const {
    cerr << "get() called on " << nodeTypeToString(type) << endl;
    throw std::runtime_error("Unimplemented get() function on ");
  }

  virtual m_Value get_for_optim() const { return get(); }

  virtual llvm::Value *codegen() {
    cerr << "Codegen called for " << nodeTypeToString(type) << endl;

    throw std::runtime_error("Unimplemented codegen() function.");
  }

  virtual ASTNode *optimise() const = 0;

  virtual m_Value get_value_if_possible() const { return m_Value(); }

  virtual void buildFunctionParams(llvm::Function *function_decl) {
    cout << "buildFunctionParams called on base class | "
         << nodeTypeToString(type) << endl;
  }

  virtual vector<string> getPlainSymbol() const {
    throw std::runtime_error("getPlainSymbol called on base class | " +
                             nodeTypeToString(type));
    return {""};
  }

  virtual void dump_llvm(string out_filename) final {

    std::error_code EC;
    llvm::raw_fd_ostream out(out_filename, EC);
    codeGenerator.global_module->print(out, nullptr);
  }

  virtual llvm::Type *getValueType() {
    cout << "getValueType called on base class | " << nodeTypeToString(type)
         << endl;

    return nullptr;
  }

  virtual void fixFunctionParams() {
    cout << "fixFunctionParams called on base class | "
         << nodeTypeToString(type) << endl;
  }

  virtual void modifyDeclarationType() {
    cout << "modifyDeclarationType called on base class | "
         << nodeTypeToString(type) << endl;
  }

  bool variadic = false;
  vector<ASTNode *> children;
  NodeType type;
};

static std::string dumpParameters(const ASTNode *base,
                                  std::vector<ASTNode *> vec_of_nodes,
                                  int depth = 0, bool is_list = false) {
  string result = formatSpacing(depth);
  result += nodeTypeToString(base->getNodeType());
  result += is_list ? "[ \n" : "{ \n";

  for (auto child : vec_of_nodes) {
    result += child->dump_ast(depth + 1);
    result += is_list ? formatSpacing(depth) + ",\n" : "";
  }

  if (is_list) {
    result[result.size() - 2] = ']';
  } else {
    result += formatSpacing(depth) + "} \n";
  }

  if (base->getNodeType() == NodeType::ParameterList) {
    result += formatSpacing(depth) +
              "Variadic: " + (base->variadic ? "true" : "false") + "\n";
  }
  return result;
}

// Helps prevent seg faults due to nullptr reference

class NullPtrNode : public ASTNode {
public:
  NullPtrNode() : ASTNode(NodeType::Unimplemented) {}

  string dump_ast(int depth = 0) const {
    string result = "";
    result += formatSpacing(depth);
    result += "\n";
    return result;
  }

  bool check_semantics() { return true; }

  ASTNode *optimise() const {
    auto ret = new NullPtrNode(*this);
    return ret;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  llvm::Value *codegen() {
    cout << "Codegen called for NullPtrNode " << endl;
    return nullptr;
  }

  m_Value get() const {
    throw std::runtime_error("get() called on a NullPtr.");
  }
};

class IConstantNode : public ASTNode {
public:
  IConstantNode(int value) : ASTNode(NodeType::IConstant), value(value) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += "Integer:" + to_string(value) + "\n";
    return result;
  }

  vector<string> getPlainSymbol() const { return {}; }

  m_Value get_value_if_possible() const { return m_Value(value); }

  ASTNode *optimise() const {
    auto ret = new IConstantNode(*this);
    ret->value = value;
    return ret;
  }

  Value *codegen() {
    return llvm::ConstantInt::get(codeGenerator.getContext(),
                                  llvm::APInt(32, value));
  }

public:
  int value;
};

class FConstantNode : public ASTNode {
public:
  FConstantNode(float value) : ASTNode(NodeType::FConstant), value(value) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += "Float:" + to_string(value) + "\n";
    return result;
  }

  m_Value get_value_if_possible() const { return m_Value(value); }

  ASTNode *optimise() const {
    auto ret = new FConstantNode(*this);
    ret->value = value;
    return ret;
  }

  vector<string> getPlainSymbol() const { return {}; }

  Value *codegen() {

    return llvm::ConstantFP::get(codeGenerator.getContext(),
                                 llvm::APFloat(value));
  }

public:
  float value;
};

class StringNode : public ASTNode {
public:
  StringNode(string value) : ASTNode(NodeType::String), value(value) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += "String Literal : " + value + " \n";
    return result;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  vector<string> getPlainSymbol() const { return {}; }

  ASTNode *optimise() const {
    auto ret = new StringNode(*this);
    ret->value = value;
    return ret;
  }

  Value *codegen() {

    if (value[0] == '\'') {
      char charValue = value[1]; // Assuming the character is the second
                                 // character after the single quote
                                 //
      cout << "Hi, here to print " << charValue << endl;

      return llvm::ConstantInt::get(codeGenerator.getContext(),
                                    llvm::APInt(8, charValue));
    }

    value = convertRawString(value);

    cout << "Hi, here to print " << value << endl;

    llvm::LLVMContext &context = codeGenerator.global_module->getContext();

    llvm::Constant *strConstant =
        llvm::ConstantDataArray::getString(context, value);
    llvm::GlobalVariable *strVar = new llvm::GlobalVariable(
        *codeGenerator.global_module, strConstant->getType(), true,
        llvm::GlobalValue::ExternalLinkage, strConstant, "str");

    strVar->setConstant(false);

    return codeGenerator.getBuilder().CreatePointerCast(
        strVar, codeGenerator.getBuilder().getInt8PtrTy());
  }

public:
  string value;
};

class TranslationUnitNode : public ASTNode {
public:
  TranslationUnitNode() : ASTNode(NodeType::TranslationUnit) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  bool check_semantics() {
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {

    auto ret = new TranslationUnitNode(*this);

    for (auto &child : ret->children) {

      get_as_lvalue = false;
      child = child->optimise();
    }
    return ret;
  }

  Value *codegen() {

    for (auto child : children) {
      declaration_type = nullptr;
      function_params.clear();
      global_is_variadic = false;
      labels.clear();
      get_as_lvalue = false;
      is_declaration_global = true;
      child->codegen();
    }

    return nullptr;
  }

  // Add member variables to hold information about translation units
};

// Node class for function definitions
class FunctionDefinitionNode : public ASTNode {
public:
  FunctionDefinitionNode() : ASTNode(NodeType::FunctionDefinition) {}

  FunctionDefinitionNode(ASTNode *declaration_specifiers, ASTNode *declarator,
                         ASTNode *declaration_list, ASTNode *compound_statement)
      : ASTNode(NodeType::FunctionDefinition),
        declaration_specifiers(declaration_specifiers), declarator(declarator),
        declaration_list(declaration_list),
        compound_statement(compound_statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this,
                          {declaration_specifiers, declarator, declaration_list,
                           compound_statement},
                          depth, false);
  }

  ASTNode *optimise() const {
    auto ret = new FunctionDefinitionNode(*this);

    ret->compound_statement = compound_statement->optimise();
    return ret;
  }

  bool check_semantics() {

    m_Value func_name = declarator->get();

    if (func_name.type != ActualValueType::STRING) {
      throw std::runtime_error("Function declarator is incorrect");
      return false;
    }

    if (scoperStack.declared(func_name.s)) {
      throw std::runtime_error("Function " + func_name.s +
                               " has already been declared.");
    }

    scoperStack.add(func_name.s);
    scoperStack.push();
    declarator->check_semantics();

    /* Smrtly make all the declarators carry over the variables from the last
     * scope */

    scoperStack.stack.back().carry_over_variables =
        scoperStack.stack.back().scope;

    compound_statement->check_semantics();
    scoperStack.pop();

    return true;
  }

  // Got rid of the functions map here, and used the module insteads

  Value *codegen() {
    declaration_type = nullptr;
    function_params.clear();
    global_is_variadic = false;
    labels.clear();
    is_declaration_global = false;

    string func_name = declarator->get().s;

    llvm::Type *func_ret_type = nullptr;

    llvm::Function *function_decl = nullptr;
    if (codeGenerator.isFunctionDeclaredButNotDefined(func_name)) {
      function_decl = codeGenerator.declared_functions[func_name];
      func_ret_type = function_decl->getReturnType();
      codeGenerator.declared_functions.erase(func_name);
    } else {

      if (codeGenerator.isFunctionDefined(func_name)) {

        throw std::runtime_error("Function " + func_name +
                                 " has already been defined.");
        return nullptr;
      }
      declaration_specifiers->getValueType();
      declarator->modifyDeclarationType();
      func_ret_type = declaration_type;
      declarator->fixFunctionParams();

      llvm::FunctionType *function_type = llvm::FunctionType::get(
          func_ret_type, function_params, global_is_variadic);
      function_decl =
          llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                                 func_name, codeGenerator.global_module.get());
    }

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "entry", function_decl);

    codeGenerator.getBuilder().SetInsertPoint(basic_block);

    codeGenerator.pushContext();

    declarator->buildFunctionParams(function_decl);

    compound_statement->codegen();

    codeGenerator.popContext();

    if (func_ret_type->isVoidTy()) {
      // If func_ret_type is void, generate a return void instruction
      codeGenerator.getBuilder().CreateRetVoid();
    } else {
      // Otherwise, generate a return instruction with null value
      codeGenerator.getBuilder().CreateRet(
          llvm::Constant::getNullValue(func_ret_type));
    }

    return nullptr;
  }

private:
  ASTNode *declaration_specifiers;
  ASTNode *declarator;
  ASTNode *declaration_list;
  ASTNode *compound_statement;
};

class DeclarationSpecifiersNode : public ASTNode {
public:
  DeclarationSpecifiersNode() : ASTNode(NodeType::DeclarationSpecifiers) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  bool check_semantics() {
    throw std::runtime_error(
        "Checking Semantics for declaration specifiers does not make sense");
  }

  ASTNode *optimise() const {
    auto ret = new DeclarationSpecifiersNode(*this);
    for (auto child : ret->children) {
      child->optimise();
    }
    return ret;
    ;
  }

  llvm::Type *getValueType() {

    for (auto child : children) {
      llvm::Type *val = child->getValueType();
      if (val != nullptr) {
        declaration_type = val;
      }
    }

    return declaration_type;
  }
};

class SpecifierNode : public ASTNode {
public:
  SpecifierNode(SpecifierEnum specifier)
      : ASTNode(NodeType::Specifier), specifier(specifier) {}

  string dump_ast(int depth = 0) const {
    string result = "";
    result += formatSpacing(depth);
    result += specifierEnumToString(specifier);
    result += "\n";
    return result;
  }

  bool check_semantics() {
    throw std::runtime_error(
        "Checking Semantics for specifiers does not make sense");
  }

  ASTNode *optimise() const {
    auto ret = new SpecifierNode(*this);
    switch (specifier) {
    case SpecifierEnum::FLOAT:
    case SpecifierEnum::DOUBLE:
      dec_type_for_optimisation = SpecifierEnum::DOUBLE;
      break;
    default:
      dec_type_for_optimisation = SpecifierEnum::INT;
    }

    return ret;
  }

  llvm::Type *getValueType() {
    llvm::Type *val = getCurrType(specifier, codeGenerator.getContext());

    if (val) {
      cout << "For the specifer " << specifierEnumToString(specifier)
           << " the type is " << val->getTypeID() << endl;
    } else {
      cout << "Bruh the val is null for " << specifierEnumToString(specifier)
           << endl;
    }

    return val;
  }

  SpecifierEnum specifier;
};

class CompoundStatementNode : public ASTNode {

public:
  CompoundStatementNode() : ASTNode(NodeType::CompoundStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  ASTNode *optimise() const {
    auto ret = new CompoundStatementNode(*this);
    for (auto &child : ret->children) {
      child = child->optimise();
    }
    return ret;
  }

  bool check_semantics() {
    scoperStack.push();
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    scoperStack.pop();
    return true;
  }

  Value *codegen() {
    codeGenerator.pushContext();
    for (auto child : children) {
      child->codegen();
    }
    codeGenerator.popContext();
    return nullptr;
  }
};

class LabelStatementNode : public ASTNode {
public:
  LabelStatementNode(ASTNode *label, ASTNode *statement)
      : ASTNode(NodeType::LabelStatement), label(label), statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {label, statement}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new LabelStatementNode(*this);
    ret->statement = statement->optimise();
    return ret;
  }

  Value *codegen() {

    m_Value label_name = label->get();

    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();
    llvm::BasicBlock *labelBlock = nullptr;

    if (labels.find(label_name.s) == labels.end()) {
      labelBlock = llvm::BasicBlock::Create(codeGenerator.getContext(), "label",
                                            function);
      labels[label_name.s] = labelBlock;
    } else {
      labelBlock = labels[label_name.s];
    }
    llvm::BasicBlock *mergeBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "labelcont");

    codeGenerator.getBuilder().CreateBr(labelBlock);
    codeGenerator.getBuilder().SetInsertPoint(labelBlock);

    statement->codegen();

    codeGenerator.getBuilder().CreateBr(mergeBlock);
    function->getBasicBlockList().push_back(mergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(mergeBlock);

    return nullptr;
  }

private:
  ASTNode *label;
  ASTNode *statement;
};

class CaseLabelStatementNode : public ASTNode {

public:
  CaseLabelStatementNode(ASTNode *constant_expression, ASTNode *statement)
      : ASTNode(NodeType::CaseLabelStatement),
        constant_expression(constant_expression), statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {constant_expression, statement}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new CaseLabelStatementNode(*this);
    ret->constant_expression = constant_expression->optimise();
    ret->statement = statement->optimise();
    return ret;
  }

public:
  ASTNode *constant_expression;
  ASTNode *statement;
};

class DefaultLabelStatementNode : public ASTNode {
public:
  DefaultLabelStatementNode(ASTNode *statement)
      : ASTNode(NodeType::DefaultLabelStatement), statement(statement) {}

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new DefaultLabelStatementNode(*this);
    ret->statement = statement->optimise();
    return ret;
  }

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {statement}, depth, false);
  }

public:
  ASTNode *statement;
};

class IfElseStatementNode : public ASTNode {
public:
  IfElseStatementNode(ASTNode *expression, ASTNode *statement,
                      ASTNode *else_statement)
      : ASTNode(NodeType::IfElseStatement), expression(expression),
        statement(statement), else_statement(else_statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {expression, statement, else_statement}, depth,
                          false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {

    auto ret = new IfElseStatementNode(*this);

    ret->expression = expression->optimise();
    ret->statement = statement->optimise();
    ret->else_statement = else_statement->optimise();
    m_Value val = expression->get_value_if_possible();
    if (val.type == ActualValueType::INTEGER) {
      return val.i ? ret->statement : ret->else_statement;
    }
    if (val.type == ActualValueType::FLOATING) {
      return val.f ? ret->statement : ret->else_statement;
    }
    return ret;
  }

  bool check_semantics() {

    if (!expression->check_semantics()) {
      return false;
    }

    scoperStack.push();
    if (!statement->check_semantics()) {
      return false;
    }
    scoperStack.pop();
    scoperStack.push();
    if (!else_statement->check_semantics()) {
      return false;
    }
    scoperStack.pop();
    return true;
  }

  Value *codegen() {
    llvm::Value *conditionValue = expression->codegen();
    if (!conditionValue) {
      throw std::runtime_error("Condition value is null");
    }

    make_bool(conditionValue);

    llvm::Value *condition = codeGenerator.getBuilder().CreateICmpNE(
        conditionValue,
        llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(codeGenerator.getContext()), 0, true),
        "ifcond");

    llvm::BasicBlock *current_block =
        codeGenerator.getBuilder().GetInsertBlock();
    llvm::Function *function = current_block->getParent();

    llvm::BasicBlock *then_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "then", function);
    llvm::BasicBlock *else_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "else");
    llvm::BasicBlock *merge_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "ifcont");

    codeGenerator.getBuilder().CreateCondBr(condition, then_block, else_block);

    codeGenerator.getBuilder().SetInsertPoint(then_block);

    codeGenerator.pushContext();
    statement->codegen();
    codeGenerator.popContext();

    codeGenerator.getBuilder().CreateBr(merge_block);

    function->getBasicBlockList().push_back(else_block);
    codeGenerator.getBuilder().SetInsertPoint(else_block);
    if (else_statement->getNodeType() != NodeType::Unimplemented) {
      codeGenerator.pushContext();
      else_statement->codegen();
      codeGenerator.popContext();
    }
    codeGenerator.getBuilder().CreateBr(merge_block);

    function->getBasicBlockList().push_back(merge_block);
    codeGenerator.getBuilder().SetInsertPoint(merge_block);

    return nullptr;
  }

private:
  ASTNode *expression;
  ASTNode *statement;
  ASTNode *else_statement;
};

class SwitchStatementNode : public ASTNode {
public:
  SwitchStatementNode(ASTNode *expression, ASTNode *statement)
      : ASTNode(NodeType::SwitchStatement), expression(expression),
        statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {expression, statement}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new SwitchStatementNode(*this);
    ret->expression = expression->optimise();
    ret->statement = statement->optimise();
    return ret;
  }

  bool check_semantics() {
    if (!expression->check_semantics()) {
      return false;
    }

    scoperStack.push();
    if (!statement->check_semantics()) {
      return false;
    }
    scoperStack.pop();
    return true;
  }

  Value *codegen() {
    llvm::Value *conditionValue = expression->codegen();

    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();
    llvm::BasicBlock *switchBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "switch", function);
    llvm::BasicBlock *defaultBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "default");
    llvm::BasicBlock *loopmergeBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "switchcont");

    codeGenerator.getBuilder().CreateBr(switchBlock);
    codeGenerator.getBuilder().SetInsertPoint(switchBlock);

    codeGenerator.pushContext();
    auto statements = statement->getChildren();

    /* int numCases = statements.size(); */
  int numCases = 0;
    ASTNode *defaultCase = nullptr;
    for(auto statement: statements){
      if(statement->getNodeType() == NodeType::CaseLabelStatement){
        numCases++;
      }else if(statement->getNodeType() == NodeType::DefaultLabelStatement){
        defaultCase = statement;
      }
    }
  
      llvm::BasicBlock *oldMergeBlock = merge_block;
      merge_block = loopmergeBlock;


    llvm::SwitchInst *switchInst = codeGenerator.getBuilder().CreateSwitch(
        conditionValue, defaultBlock, numCases);

    for (int i = 0; i < statements.size(); i++) {
      if (statements[i]->getNodeType() == NodeType::DefaultLabelStatement) {
        continue;
      }else if (statements[i]->getNodeType() == NodeType::BreakStatement) {
    codeGenerator.getBuilder().SetInsertPoint(loopmergeBlock);
    codeGenerator.getBuilder().CreateBr(loopmergeBlock);
    continue;
      }
      llvm::BasicBlock *caseBlock = llvm::BasicBlock::Create(
          codeGenerator.getContext(), "case" + std::to_string(i), function);
      codeGenerator.getBuilder().SetInsertPoint(caseBlock);

      CaseLabelStatementNode *caseNode =
          dynamic_cast<CaseLabelStatementNode *>(statements[i]);

      if (!caseNode) {
        throw std::runtime_error("Case node is null");
      }

      caseNode->statement->codegen();
      codeGenerator.getBuilder().CreateBr(loopmergeBlock);

      llvm::ConstantInt *caseValue = llvm::dyn_cast<llvm::ConstantInt>(
          caseNode->constant_expression->codegen());
      switchInst->addCase(caseValue, caseBlock);
    }

    // Generate LLVM IR code for the default case, if present

    if (defaultCase != nullptr) {
      function->getBasicBlockList().push_back(defaultBlock);
      codeGenerator.getBuilder().SetInsertPoint(defaultBlock);

      DefaultLabelStatementNode *defaultNode =
          dynamic_cast<DefaultLabelStatementNode *>(defaultCase);

      defaultNode->statement->codegen();
      codeGenerator.getBuilder().CreateBr(loopmergeBlock);
    }

    codeGenerator.popContext();

    // Add the merge block to the function
    function->getBasicBlockList().push_back(loopmergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopmergeBlock);

    return nullptr;
  }

private:
  ASTNode *expression;
  ASTNode *statement;
};

class WhileStatementNode : public ASTNode {
public:
  WhileStatementNode(ASTNode *expression, ASTNode *statement)
      : ASTNode(NodeType::WhileStatement), expression(expression),
        statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {expression, statement}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new WhileStatementNode(*this);
    auto expression_copy = expression->optimise();
    ret->statement = statement->optimise();

    m_Value val = expression_copy->get_value_if_possible();

    if (val.type == ActualValueType::INTEGER) {
      if (val.i == 0) {
        return new NullPtrNode();
      }
    }

    if (val.type == ActualValueType::FLOATING) {
      if (val.f == 0) {
        return new NullPtrNode();
      }
    }

    return ret;
  }

  bool check_semantics() {
    if (!expression->check_semantics()) {
      return false;
    }
    scoperStack.push();
    if (!statement->check_semantics()) {
      return false;
    }
    scoperStack.pop();
    return true;
  }

  Value *codegen() {
    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();
    llvm::BasicBlock *whileBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "while", function);
    llvm::BasicBlock *loopBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "loop");
    llvm::BasicBlock *mergeBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "whilecont");

    codeGenerator.getBuilder().CreateBr(whileBlock);
    codeGenerator.getBuilder().SetInsertPoint(whileBlock);
    llvm::Value *conditionValue = expression->codegen();

    make_bool(conditionValue);

    codeGenerator.getBuilder().CreateCondBr(conditionValue, loopBlock,
                                            mergeBlock);
    function->getBasicBlockList().push_back(loopBlock);

    codeGenerator.getBuilder().SetInsertPoint(loopBlock);

    llvm::BasicBlock *old_loop_block = loop_block;
    llvm::BasicBlock *old_merge_block = merge_block;
    loop_block = loopBlock;
    merge_block = mergeBlock;

    codeGenerator.pushContext();
    statement->codegen();
    codeGenerator.popContext();

    loop_block = old_loop_block;
    merge_block = old_merge_block;

    codeGenerator.getBuilder().CreateBr(whileBlock);
    function->getBasicBlockList().push_back(mergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(mergeBlock);
    return nullptr;
  }

private:
  ASTNode *expression;
  ASTNode *statement;
};

class DoWhileStatementNode : public ASTNode {
public:
  DoWhileStatementNode(ASTNode *expression, ASTNode *statement)
      : ASTNode(NodeType::DoWhileStatement), expression(expression),
        statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {statement, expression}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const {
    auto ret = new DoWhileStatementNode(*this);
    ret->statement = statement->optimise();

    // Can't do the same optimisation as while statement here because of break
    // and continue

    return ret;
    ;
  }

  bool check_semantics() {
    if (!expression->check_semantics()) {
      return false;
    }
    scoperStack.push();
    if (!statement->check_semantics()) {
      return false;
    }
    scoperStack.pop();
    return true;
  }

  Value *codegen() {
    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();
    llvm::BasicBlock *doBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "do", function);
    llvm::BasicBlock *loopBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "loop");
    llvm::BasicBlock *mergeBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "docont");

    codeGenerator.getBuilder().CreateBr(doBlock);
    codeGenerator.getBuilder().SetInsertPoint(doBlock);

    llvm::BasicBlock *old_loop_block = loop_block;
    llvm::BasicBlock *old_merge_block = merge_block;
    loop_block = loopBlock;
    merge_block = mergeBlock;

    codeGenerator.pushContext();
    statement->codegen();
    codeGenerator.popContext();

    loop_block = old_loop_block;
    merge_block = old_merge_block;

    codeGenerator.getBuilder().CreateBr(loopBlock);
    function->getBasicBlockList().push_back(loopBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopBlock);

    llvm::Value *conditionValue = expression->codegen();

    make_bool(conditionValue);
    // Create the loop condition branch instruction
    codeGenerator.getBuilder().CreateCondBr(conditionValue, doBlock,
                                            mergeBlock);

    // Add merge block to the function
    function->getBasicBlockList().push_back(mergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(mergeBlock);

    return nullptr;
  }

private:
  ASTNode *expression;
  ASTNode *statement;
};

class ForStatementNode : public ASTNode {
public:
  ForStatementNode(ASTNode *expression1, ASTNode *expression2,
                   ASTNode *expression3, ASTNode *statement)
      : ASTNode(NodeType::ForStatement), expression1(expression1),
        expression2(expression2), expression3(expression3),
        statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {expression1, expression2, expression3, statement}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {

    auto ret = new ForStatementNode(*this);

    ret->expression1 = expression1->optimise();
    auto expression2_copy = expression2->optimise();
    ret->expression3 = expression3->optimise();

    m_Value val = expression2_copy->get_value_if_possible();

    if (val.type == ActualValueType::INTEGER) {
      if (val.i == 0) {
        return new NullPtrNode();
      }
    }

    if (val.type == ActualValueType::FLOATING) {
      if (val.f == 0) {
        return new NullPtrNode();
      }
    }

    ret->statement = statement->optimise();
    return ret;
  }

  bool check_semantics() {

    scoperStack.push();

    if (!expression1->check_semantics()) {
      return false;
    }
    if (!expression2->check_semantics()) {
      return false;
    }
    if (!expression3->check_semantics()) {
      return false;
    }

    scoperStack.push();

    if (!statement->check_semantics()) {
      return false;
    }

    scoperStack.pop();
    scoperStack.pop();

    return true;
  }

  Value *codegen() {
    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();

    // Create basic blocks for the for loop
    llvm::BasicBlock *initBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "for.init", function);
    llvm::BasicBlock *loopConditionBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "for.cond", function);
    llvm::BasicBlock *loopBodyBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "for.body", function);
    llvm::BasicBlock *loopIterBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "for.iter", function);
    llvm::BasicBlock *mergeBlock = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "for.merge", function);

    // Jump to the initialization block
    codeGenerator.getBuilder().CreateBr(initBlock);
    codeGenerator.getBuilder().SetInsertPoint(initBlock);

    codeGenerator.pushContext();
    // Generate LLVM IR code for the initialization expression

    if (expression1->getNodeType() != NodeType::Unimplemented) {
      expression1->codegen();
    }

    codeGenerator.pushContext();

    // Jump to the loop condition block
    codeGenerator.getBuilder().CreateBr(loopConditionBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopConditionBlock);

    // Generate LLVM IR code for the loop condition check
    // The default value is true
    llvm::Value *conditionValue =
        llvm::ConstantInt::get(codeGenerator.getContext(), llvm::APInt(1, 1));

    if (expression2->getNodeType() != NodeType::Unimplemented) {
      conditionValue = expression2->codegen();
    }
    make_bool(conditionValue);
    // Create the loop condition branch instruction
    codeGenerator.getBuilder().CreateCondBr(conditionValue, loopBodyBlock,
                                            mergeBlock);

    // Set the insert point to the loop body block
    function->getBasicBlockList().push_back(loopBodyBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopBodyBlock);

    // Generate LLVM IR code for the loop body

    llvm::BasicBlock *old_loop_block = loop_block;
    llvm::BasicBlock *old_merge_block = merge_block;
    loop_block = loopIterBlock;
    merge_block = mergeBlock;
    statement->codegen();
    loop_block = old_loop_block;
    merge_block = old_merge_block;

    // Jump to the loop iteration block
    codeGenerator.getBuilder().CreateBr(loopIterBlock);

    // Set the insert point to the loop iteration block
    function->getBasicBlockList().push_back(loopIterBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopIterBlock);

    // Generate LLVM IR code for the iteration expression
    //
    if (expression3->getNodeType() != NodeType::Unimplemented) {
      expression3->codegen();
    }
    // Branch back to the loop condition block
    codeGenerator.getBuilder().CreateBr(loopConditionBlock);

    // Add the merge block to the function
    function->getBasicBlockList().push_back(mergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(mergeBlock);

    codeGenerator.popContext();
    codeGenerator.popContext();

    return nullptr;
  }

private:
  ASTNode *expression1;
  ASTNode *expression2;
  ASTNode *expression3;
  ASTNode *statement;
};

class GotoStatementNode : public ASTNode {
public:
  GotoStatementNode(ASTNode *identifier)
      : ASTNode(NodeType::GotoStatement), identifier(identifier) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {identifier}, depth, false);
  }

  ASTNode *optimise() const { return new GotoStatementNode(*this); }

  m_Value get_value_if_possible() const { return m_Value(); }

  Value *codegen() {

    m_Value label = identifier->get();

    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();
    llvm::BasicBlock *labelBlock = nullptr;

    if (labels.find(label.s) == labels.end()) {
      labelBlock = llvm::BasicBlock::Create(codeGenerator.getContext(), "label",
                                            function);
      labels[label.s] = labelBlock;
    } else {
      labelBlock = labels[label.s];
    }

    codeGenerator.getBuilder().CreateBr(labelBlock);
    return nullptr;
  }

private:
  ASTNode *identifier;
};

class ContinueStatementNode : public ASTNode {
public:
  ContinueStatementNode() : ASTNode(NodeType::ContinueStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {}, depth, false);
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const { return new ContinueStatementNode(*this); }

  Value *codegen() {

    if (loop_block == nullptr) {
      throw std::runtime_error("Continue statement outside of loop");
    }

    codeGenerator.getBuilder().CreateBr(loop_block);
    return nullptr;
  }
};

class BreakStatementNode : public ASTNode {
public:
  BreakStatementNode() : ASTNode(NodeType::BreakStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {}, depth, false);
  }

  ASTNode *optimise() const { return new BreakStatementNode(*this); }
  m_Value get_value_if_possible() const { return m_Value(); }

  Value *codegen() {
    if (merge_block == nullptr) {
      throw std::runtime_error("Break statement outside of loop");
    }

    codeGenerator.getBuilder().CreateBr(merge_block);
    return nullptr;
  }
};

class ReturnStatementNode : public ASTNode {
public:
  ReturnStatementNode(ASTNode *expression)
      : ASTNode(NodeType::ReturnStatement), expression(expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {expression}, depth, false);
  }

  bool check_semantics() { return expression->check_semantics(); }

  ASTNode *optimise() const {
    auto ret = new ReturnStatementNode(*this);
    ret->expression = expression->optimise();

    m_Value v = expression->get_value_if_possible();
    return ret;
  }

  m_Value get_value_if_possible() const {
    return expression->get_value_if_possible();
  }

  Value *codegen() {
    if (expression->getNodeType() != NodeType::Unimplemented) {

      llvm::Value *ret_val = expression->codegen();

      llvm::Function *function =
          codeGenerator.getBuilder().GetInsertBlock()->getParent();

      llvm::Type *ret_type = function->getReturnType();

      llvm::Value *random_alloca = codeGenerator.getBuilder().CreateAlloca(
          ret_type, nullptr, "return_alloca");

      make_store_compatible(random_alloca, ret_val);

      llvm::Value *val_to_ret = codeGenerator.getBuilder().CreateRet(ret_val);

      return val_to_ret;
    }
    return codeGenerator.getBuilder().CreateRetVoid();
  }

private:
  ASTNode *expression;
};

class DeclarationListNode : public ASTNode {
public:
  DeclarationListNode() : ASTNode(NodeType::DeclarationList) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  ASTNode *optimise() const {
    auto ret = new DeclarationListNode(*this);
    for (auto &child : ret->children) {
      child = child->optimise();
    }
    return ret;
    ;
  }
};

class DeclarationNode : public ASTNode {
public:
  DeclarationNode(ASTNode *declaration_specifiers,
                  ASTNode *init_declarator_list)
      : ASTNode(NodeType::Declaration),
        declaration_specifiers(declaration_specifiers),
        init_declarator_list(init_declarator_list) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {declaration_specifiers, init_declarator_list},
                          depth, false);
  }

  bool check_semantics() { return init_declarator_list->check_semantics(); }

  ASTNode *optimise() const {
    auto ret = new DeclarationNode(*this);
    ret->declaration_specifiers = declaration_specifiers->optimise();
    ret->init_declarator_list = init_declarator_list->optimise();
    return ret;
  }

  Value *codegen() {

    auto type_of_decl = declaration_specifiers->getValueType();

    init_declarator_list->codegen();
    return nullptr;
  }

private:
  ASTNode *declaration_specifiers;
  ASTNode *init_declarator_list;
  // Add member variables to hold information about declarations
};

class InitDeclartorNode : public ASTNode {
public:
  InitDeclartorNode(ASTNode *declarator, ASTNode *initializer)
      : ASTNode(NodeType::InitDeclartor), declarator(declarator),
        initializer(initializer) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {declarator, initializer}, depth, false);
  }

  ASTNode *optimise() const {
    auto ret = new InitDeclartorNode(*this);
    ret->initializer = initializer->optimise();

    m_Value v = initializer->get_value_if_possible();
    m_Value name = declarator->get_for_optim();

    codeGenerator.put_mval(name.s, v);

    return ret;
  }

  bool check_semantics() {

    if (!declarator->check_semantics()) {
      return false;
    }

    m_Value v_name = declarator->get();

    if (v_name.type != ActualValueType::STRING) {
      throw std::runtime_error("Declarator is not a string");
      return false;
    }

    if (scoperStack.declared(v_name.s)) {
      throw std::runtime_error("Variable " + v_name.s +
                               " has already been declared.");
      return false;
    }

    return scoperStack.add(declarator->get().s);
  }

  Value *codegen() {
    llvm::Type *old_type = declaration_type;
    array_type = nullptr;
    declarator->modifyDeclarationType();
    string name = declarator->get().s;

    am_i_initialising = true;
    declarator->codegen();
    am_i_initialising = false;

    llvm::Type *declaration_type_copy = declaration_type;
    declaration_type = old_type;

    if (array_type != nullptr) {
      declaration_type_copy = array_type;
    }

    if (codeGenerator.isFunctionDefined(declarator->get().s)) {
      return nullptr;
    }

    llvm::Value *val = nullptr;
    if (initializer->getNodeType() != NodeType::Unimplemented) {
      val = initializer->codegen();
    } else if (array_type == nullptr) {
      val = llvm::Constant::getNullValue(declaration_type_copy);
    }

    cout << "Name " << declarator->get().s << endl;

    llvm::Value *alloca = nullptr;

    if (is_declaration_global) {

      llvm::Constant *init_val = nullptr;

      if (llvm::isa<llvm::Constant>(val)) {
        init_val = llvm::dyn_cast<llvm::Constant>(val);
      } else {
        throw std::runtime_error(
            "Global variable must be initialized with a constant");
      }

      alloca = new llvm::GlobalVariable(
          *codeGenerator.global_module, declaration_type_copy, false,
          llvm::GlobalValue::ExternalLinkage, init_val, name);
    } else {
      alloca = codeGenerator.getBuilder().CreateAlloca(declaration_type_copy,
                                                       nullptr, name);
      if (val) {
        make_store_compatible(alloca, val);
        codeGenerator.getBuilder().CreateStore(val, alloca);
      }
    }

    codeGenerator.getsymbolTable()[declarator->get().s] = alloca;
    return alloca;
  }

private:
  ASTNode *declarator;
  ASTNode *initializer;
};

class InitDeclartorListNode : public ASTNode {
public:
  InitDeclartorListNode() : ASTNode(NodeType::InitDeclartorList) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  bool check_semantics() {
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }

  ASTNode *optimise() const {
    auto ret = new InitDeclartorListNode(*this);
    for (auto &child : ret->children) {
      child = child->optimise();
    }
    return ret;
  }

  Value *codegen() {
    for (auto child : children) {
      llvm::Type *old_type = declaration_type;
      child->codegen();
      declaration_type = old_type;
    }
    return nullptr;
  }
};

class DeclaratorNode : public ASTNode {
public:
  DeclaratorNode(ASTNode *pointer, ASTNode *direct_declarator)
      : ASTNode(NodeType::Declarator), pointer(pointer),
        direct_declarator(direct_declarator) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth) + "Declarator{\n";
    result += pointer->dump_ast(depth + 1);
    result += direct_declarator->dump_ast(depth + 1);
    result += formatSpacing(depth);
    result += "}\n";
    return result;
  }

  bool check_semantics() { return direct_declarator->check_semantics(); }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const { return new DeclaratorNode(*this); }

  m_Value get() const { return direct_declarator->get(); }

  m_Value get_for_optim() const {
    m_Value val = direct_declarator->get_for_optim();
    val.s = pointer->get_for_optim().s + val.s;
    return val;
  }

  void fixFunctionParams() {
    llvm::Type *old_type = declaration_type;
    direct_declarator->fixFunctionParams();
    declaration_type = old_type;
  }

  void modifyDeclarationType() {
    pointer->modifyDeclarationType();
    direct_declarator->modifyDeclarationType();
  }

  void buildFunctionParams(llvm::Function *function_decl) {
    direct_declarator->buildFunctionParams(function_decl);
  }

  Value *codegen() {
    auto old_type = declaration_type;
    pointer->modifyDeclarationType();
    auto val_to_ret = direct_declarator->codegen();
    declaration_type = old_type;
    return val_to_ret;
  }

private:
  ASTNode *pointer;
  ASTNode *direct_declarator;
};

class PointerNode : public ASTNode {
public:
  PointerNode(ASTNode *pointer)
      : ASTNode(NodeType::Pointer), pointer(pointer) {}

  m_Value get() const {
    m_Value val = m_Value("");
    if (pointer->getNodeType() != NodeType::Unimplemented) {
      val = pointer->get();
    }

    val.s += "*";
    return val;
  }

  m_Value get_for_optim() const {
    m_Value val = m_Value("");
    if (pointer->getNodeType() != NodeType::Unimplemented) {
      val = pointer->get_for_optim();
    }
    val.s += "*";
    return val;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {
    auto ret = new PointerNode(*this);
    ret->pointer = pointer->optimise();
    return ret;
  }

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth) + "Pointer*\n";
    if (pointer->getNodeType() != NodeType::Unimplemented) {
      result += pointer->dump_ast(depth);
    }
    return result;
  }

  void modifyDeclarationType() {

    cout << "Modifying declaration type for pointer" << endl;
    declaration_type = llvm::PointerType::get(declaration_type, 0);

    if (pointer->getNodeType() != NodeType::Unimplemented) {
      pointer->modifyDeclarationType();
    }
  }

private:
  ASTNode *pointer;
};

class ArrayDeclaratorNode : public ASTNode {
public:
  ArrayDeclaratorNode(ASTNode *direct_declarator,
                      ASTNode *assignment_expression)
      : ASTNode(NodeType::ArrayDeclarator),
        direct_declarator(direct_declarator),
        assignment_expression(assignment_expression) {}
  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {direct_declarator, assignment_expression},
                          depth, false);
  }

  void modifyDeclarationType() { direct_declarator->modifyDeclarationType(); }

  m_Value get_value_if_possible() const {

    if (!constant_prop)
      return m_Value();
    m_Value val = get_for_optim();

    return codeGenerator.get_mval(val.s);
  }

  m_Value get_for_optim() const {
    m_Value val = direct_declarator->get_for_optim();
    val.s += "[" + assignment_expression->dump_ast() + "]";
    return val;
  }

  ASTNode *optimise() const {

    auto ret = new ArrayDeclaratorNode(*this);
    ret->assignment_expression = assignment_expression->optimise();
    return ret;
  }

  m_Value get() const { return direct_declarator->get(); }

  Value *codegen() {

    direct_declarator->codegen();

    Value *array_size = nullptr;
    if (assignment_expression->getNodeType() == NodeType::Unimplemented) {
      array_size = llvm::ConstantInt::get(codeGenerator.getContext(),
                                          llvm::APInt(32, 0, true));
    } else {
      array_size = assignment_expression->codegen();
    }

    if (array_size->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      array_size = codeGenerator.getBuilder().CreateIntCast(
          array_size, llvm::Type::getInt32Ty(codeGenerator.getContext()), true);
    }

    llvm::ConstantInt *arraySizeInt =
        llvm::dyn_cast<llvm::ConstantInt>(array_size);
    uint64_t array_size_val = arraySizeInt->getZExtValue();

    if (array_type == nullptr) {
      array_type = declaration_type;
    }

    array_type = llvm::ArrayType::get(array_type, array_size_val);

    return nullptr;
    /**/
    /* llvm::Type *array_type = llvm::ArrayType::get(element_type,
     * array_size_val); */
    /**/
    /* arrat_type = llvm::PointerType::get(array_type, 0); */

    /* llvm::AllocaInst *alloca = codeGenerator.getBuilder().CreateAlloca( */
    /* array_type, nullptr, direct_declarator->get().s); */
    /* codeGenerator.getsymbolTable()[direct_declarator->get().s] = alloca; */
    /* return alloca; */
  }

private:
  ASTNode *direct_declarator;
  ASTNode *assignment_expression;
};

class FunctionDeclarationNode : public ASTNode {
public:
  FunctionDeclarationNode(ASTNode *direct_declarator,
                          ASTNode *parameter_type_list)
      : ASTNode(NodeType::FunctionDeclator),
        direct_declarator(direct_declarator),
        parameter_type_list(parameter_type_list) {}

  string dump_ast(int depth = 0) const {

    return dumpParameters(this, {direct_declarator, parameter_type_list}, depth,
                          false);
  }

  m_Value get() const { return direct_declarator->get(); }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {
    auto ret = new FunctionDeclarationNode(*this);
    return ret;
  }

  bool check_semantics() { return parameter_type_list->check_semantics(); }

  void fixFunctionParams() {

    llvm::Type *current_type = declaration_type;

    if (parameter_type_list->getNodeType() == NodeType::Unimplemented) {
      return;
    }
    parameter_type_list->fixFunctionParams();

    declaration_type = current_type;
  }

  void modifyDeclarationType() { direct_declarator->modifyDeclarationType(); }

  void buildFunctionParams(llvm::Function *function_decl) {
    if (parameter_type_list->getNodeType() == NodeType::Unimplemented) {
      return;
    }
    parameter_type_list->buildFunctionParams(function_decl);
  }

  Value *codegen() {

    global_is_variadic = false;

    if (codeGenerator.isFunctionDeclaredButNotDefined(
            direct_declarator->get().s)) {
      return nullptr;
    }

    string func_name = direct_declarator->get().s;
    llvm::Type *current_type = declaration_type;

    function_params.clear();
    fixFunctionParams();

    llvm::FunctionType *function_type = llvm::FunctionType::get(
        current_type, function_params, global_is_variadic);

    llvm::Function *function_decl =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                               func_name, codeGenerator.global_module.get());

    codeGenerator.declared_functions[func_name] = function_decl;
    return nullptr;
  }

private:
  ASTNode *direct_declarator;
  ASTNode *parameter_type_list;
};

class ParameterListNode : public ASTNode {
public:
  ParameterListNode() : ASTNode(NodeType::ParameterList) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  ASTNode *optimise() const {
    auto ret = new ParameterListNode(*this);
    return ret;
  }

  bool check_semantics() {
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }

  void fixFunctionParams() {

    global_is_variadic = variadic;

    for (auto child : children) {
      child->fixFunctionParams();
    }
  }

  void buildFunctionParams(llvm::Function *function_decl) {
    parameter_list_index = 0;
    for (auto child : children) {
      child->buildFunctionParams(function_decl);
      parameter_list_index += 1;
    }
  }
};

class ParameterDeclarationNode : public ASTNode {
public:
  ParameterDeclarationNode(ASTNode *declaration_specifiers, ASTNode *declarator)
      : ASTNode(NodeType::ParameterDeclaration),
        declaration_specifiers(declaration_specifiers), declarator(declarator) {
  }

  ASTNode *optimise() const {
    auto ret = new ParameterDeclarationNode(*this);
    return ret;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {declaration_specifiers, declarator}, depth,
                          false);
  }

  bool check_semantics() { return declarator->check_semantics(); }

  void fixFunctionParams() {
    declaration_type = declaration_specifiers->getValueType();
    declarator->modifyDeclarationType();
    function_params.push_back(declaration_type);
  }

  void buildFunctionParams(llvm::Function *function_decl) {
    declaration_type = declaration_specifiers->getValueType();
    declarator->modifyDeclarationType();

    string name = declarator->get().s;

    llvm::AllocaInst *p = codeGenerator.getBuilder().CreateAlloca(
        declaration_type, nullptr, name);
    codeGenerator.getBuilder().CreateStore(
        function_decl->arg_begin() + parameter_list_index, p);

    codeGenerator.getsymbolTable().add_to_carryover(name, p);
  }

private:
  ASTNode *declaration_specifiers;
  ASTNode *declarator;
};

class IdentifierNode : public ASTNode {
public:
  IdentifierNode(std::string name)
      : ASTNode(NodeType::Identifier), name(name) {}

  string dump_ast(int depth = 0) const {
    string result = "";
    result += formatSpacing(depth);
    result += "Identifier: " + name + "\n";
    return result;
  }

  vector<string> getPlainSymbol() const { return {name}; }

  m_Value get() const { return m_Value(name); }

  bool check_semantics() { return scoperStack.exists(name); }

  void modifyDeclarationType() {}

  m_Value get_value_if_possible() const {

    if (!constant_prop)
      return m_Value();
    return codeGenerator.get_mval(name);
  }

  ASTNode *optimise() const {
    auto ret = new IdentifierNode(*this);
    return ret;
  }

  Value *codegen() {

    if (am_i_initialising) {
      return nullptr;
    }

    llvm::Value *val = codeGenerator.findSymbol(name);

    if (val == nullptr) {
      throw std::runtime_error("Variable " + name + " has not been declared.");
    }

    if (get_as_lvalue) {
      return val;
    }

    llvm::Type *val_type = val->getType()->getPointerElementType();
    return codeGenerator.getBuilder().CreateLoad(val_type, val, name.c_str());
  }

private:
  string name;
};

class ExpressionListNode : public ASTNode {
public:
  ExpressionListNode() : ASTNode(NodeType::ExpressionList) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }

  m_Value get_value_if_possible() const {
    if (children.size() == 1) {
      return children[0]->get_value_if_possible();
    }
    return m_Value();
  }

  ASTNode *optimise() const {
    auto ret = new ExpressionListNode(*this);
    for (auto &child : ret->children) {
      child = child->optimise();
    }
    return ret;
  }

  vector<string> getPlainSymbol() const {
    vector<string> to_ret = {};
    for (auto &child : children) {
      auto got = child->getPlainSymbol();
      to_ret.insert(to_ret.end(), got.begin(), got.end());
    }
    return to_ret;
  }

  bool check_semantics() {
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }
  Value *codegen() {
    llvm::Value *val = nullptr;

    for (auto child : children) {
      val = child->codegen();
    }
    return val;
  }
};

class AssignmentExpressionNode : public ASTNode {
public:
  AssignmentExpressionNode(ASTNode *unary_expression, AssignmentOperator assOp,
                           ASTNode *assignment_expression)
      : ASTNode(NodeType::AssignmentExpression),
        unary_expression(unary_expression), assOp(assOp),
        assignment_expression(assignment_expression) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += assignmentOperatorToString(assOp) + "{ \n";
    result += unary_expression->dump_ast(depth + 1);
    result += assignment_expression->dump_ast(depth + 1);
    result += formatSpacing(depth) + "} \n";
    return result;
  }

  bool check_semantics() {
    return unary_expression->check_semantics() &&
           assignment_expression->check_semantics();
  }

  vector<string> getPlainSymbol() const {
    return assignment_expression->getPlainSymbol();
  }

  m_Value get_value_if_possible() const {
    return unary_expression->get_value_if_possible();
  }

  ASTNode *optimise() const {

    auto ret = new AssignmentExpressionNode(*this);

    vector<string> lhs_names = unary_expression->getPlainSymbol();
    vector<string> rhs_names = assignment_expression->getPlainSymbol();

    for (auto &name : rhs_names) {
      if (name == "*") {
        return ret;
      }
    }

    for (auto &name : lhs_names) {
      for (auto &name2 : rhs_names) {
        if (name == name2) {
          return ret;
        }
      }
    }

    m_Value lhs_name = unary_expression->get_for_optim();
    ret->assignment_expression = assignment_expression->optimise();

    m_Value rhs = ret->assignment_expression->get_value_if_possible();
    m_Value lhs = ret->unary_expression->get_value_if_possible();

    m_Value v = m_Value();

    switch (assOp) {
    case AssignmentOperator::ASSIGN: {
      break;
    }
    case AssignmentOperator::ADD_ASSIGN: {
      v = lhs + rhs;
      break;
    }
    case AssignmentOperator::MUL_ASSIGN: {
      v = lhs * rhs;
      break;
    }
    case AssignmentOperator::DIV_ASSIGN: {
      v = lhs / rhs;
      break;
    }
    case AssignmentOperator::SUB_ASSIGN: {
      v = lhs - rhs;
      break;
    }
    case AssignmentOperator::MOD_ASSIGN: {
      v = lhs % rhs;
      break;
    }
    case AssignmentOperator::LEFT_ASSIGN: {
      v = lhs << rhs;

      break;
    }
    case AssignmentOperator::RIGHT_ASSIGN: {
      v = lhs >> rhs;
      break;
    }
    case AssignmentOperator::AND_ASSIGN: {
      v = lhs & rhs;
      break;
    }
    case AssignmentOperator::XOR_ASSIGN: {
      v = lhs ^ rhs;
      break;
    }
    case AssignmentOperator::OR_ASSIGN: {
      v = lhs | rhs;
      break;
    }
    }
    if (v.type == ActualValueType::INTEGER) {
      ret->assignment_expression = new IConstantNode(v.i);
    } else if (v.type == ActualValueType::FLOATING) {
      ret->assignment_expression = new FConstantNode(v.f);
    }

    if (v.type != NO_VALUE) {
      ret->assOp = AssignmentOperator::ASSIGN;
    }

    v = ret->assignment_expression->get_value_if_possible();
    if (ret->assOp == AssignmentOperator::ASSIGN) {
      if (v.type != NO_VALUE) {
        codeGenerator.put_mval(lhs_name.s, v);
      }
    }

    return ret;
  }

  Value *codegen() override {
    // Generate LLVM IR for the right-hand side expression

    Value *rhsValue = assignment_expression->codegen();
    // Get the address of the left-hand side variable or expression

    get_as_lvalue = true;
    Value *lhsAddr = unary_expression->codegen();
    get_as_lvalue = false;

    // Make sure number of bits in lhs and rhs is same, if not cast

    llvm::Type *lhsType = lhsAddr->getType()->getPointerElementType();

    // Perform the assignment based on the operator

    switch (assOp) {
    case AssignmentOperator::ASSIGN: {
      // Store the value to the address

      make_store_compatible(lhsAddr, rhsValue);
      codeGenerator.getBuilder().CreateStore(rhsValue, lhsAddr, "eqassign");
      break;
    }
    case AssignmentOperator::MUL_ASSIGN: {
      llvm::Value *lhsVal =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "mulload");
      make_lhs_rhs_compatible(lhsVal, rhsValue);
      llvm::Value *mul =
          codeGenerator.getBuilder().CreateMul(lhsVal, rhsValue, "mulcreate");
      make_store_compatible(lhsAddr, mul);
      codeGenerator.getBuilder().CreateStore(mul, lhsAddr, "mulstore");

      break;
    }
    case AssignmentOperator::DIV_ASSIGN: {
      llvm::Value *lhsValDiv =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "divload");
      make_lhs_rhs_compatible(lhsValDiv, rhsValue);
      llvm::Value *div = codeGenerator.getBuilder().CreateSDiv(
          lhsValDiv, rhsValue, "divcreate");
      make_store_compatible(lhsAddr, div);
      codeGenerator.getBuilder().CreateStore(div, lhsAddr, "divstore");
      break;
    }
    case AssignmentOperator::MOD_ASSIGN: {
      llvm::Value *lhsValMod =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "modload");
      make_lhs_rhs_compatible(lhsValMod, rhsValue);
      llvm::Value *mod = codeGenerator.getBuilder().CreateSRem(
          lhsValMod, rhsValue, "modcreate");
      make_store_compatible(lhsAddr, mod);
      codeGenerator.getBuilder().CreateStore(mod, lhsAddr, "modstore");
      break;
    }
    case AssignmentOperator::ADD_ASSIGN: {
      llvm::Value *lhsValAdd =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "addload");
      make_lhs_rhs_compatible(lhsValAdd, rhsValue);
      llvm::Value *add = codeGenerator.getBuilder().CreateAdd(
          lhsValAdd, rhsValue, "addcreate");
      make_store_compatible(lhsAddr, add);
      codeGenerator.getBuilder().CreateStore(add, lhsAddr, "addstore");
      break;
    }
    case AssignmentOperator::SUB_ASSIGN: {
      llvm::Value *lhsValSub =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "subload");
      make_lhs_rhs_compatible(lhsValSub, rhsValue);
      llvm::Value *sub = codeGenerator.getBuilder().CreateSub(
          lhsValSub, rhsValue, "subcreate");

      make_store_compatible(lhsAddr, sub);
      codeGenerator.getBuilder().CreateStore(sub, lhsAddr, "substore");
      break;
    }
    case AssignmentOperator::LEFT_ASSIGN: {
      llvm::Value *lhsValLeft =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "leftload");
      make_lhs_rhs_compatible(lhsValLeft, rhsValue);
      if (rhsValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
        throw std::runtime_error(
            "Right hand side of AN operator is not an integer");
      }
      llvm::Value *left = codeGenerator.getBuilder().CreateShl(
          lhsValLeft, rhsValue, "leftcreate");
      make_store_compatible(lhsAddr, left);

      codeGenerator.getBuilder().CreateStore(left, lhsAddr, "leftstore");
      break;
    }
    case AssignmentOperator::RIGHT_ASSIGN: {
      llvm::Value *lhsValRight =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "rightload");
      make_lhs_rhs_compatible(lhsValRight, rhsValue);
      if (rhsValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
        throw std::runtime_error(
            "Right hand side of AN operator is not an integer");
      }
      llvm::Value *right = codeGenerator.getBuilder().CreateAShr(
          lhsValRight, rhsValue, "rightcreate");
      make_store_compatible(lhsAddr, right);
      codeGenerator.getBuilder().CreateStore(right, lhsAddr, "rightstore");
      break;
    }
    case AssignmentOperator::AND_ASSIGN: {
      llvm::Value *lhsValAnd =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "andload");
      make_lhs_rhs_compatible(lhsValAnd, rhsValue);

      if (rhsValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
        throw std::runtime_error(
            "Right hand side of AN operator is not an integer");
      }

      llvm::Value *and_val = codeGenerator.getBuilder().CreateAnd(
          lhsValAnd, rhsValue, "andcreate");

      make_store_compatible(lhsAddr, and_val);

      codeGenerator.getBuilder().CreateStore(and_val, lhsAddr, "andstore");
      break;
    }
    case AssignmentOperator::XOR_ASSIGN: {
      llvm::Value *lhsValXor =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "xorload");
      make_lhs_rhs_compatible(lhsValXor, rhsValue);
      if (rhsValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
        throw std::runtime_error(
            "Right hand side of AN operator is not an integer");
      }
      llvm::Value *xor_val = codeGenerator.getBuilder().CreateXor(
          lhsValXor, rhsValue, "xorcreate");
      make_store_compatible(lhsAddr, xor_val);
      codeGenerator.getBuilder().CreateStore(xor_val, lhsAddr, "xorstore");
      break;
    }
    case AssignmentOperator::OR_ASSIGN: {
      llvm::Value *lhsValOr =
          codeGenerator.getBuilder().CreateLoad(lhsType, lhsAddr, "orload");
      make_lhs_rhs_compatible(lhsValOr, rhsValue);
      if (rhsValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
        throw std::runtime_error(
            "Right hand side of AN operator is not an integer");
      }
      llvm::Value *or_val =
          codeGenerator.getBuilder().CreateOr(lhsValOr, rhsValue, "orcreate");
      make_store_compatible(lhsAddr, or_val);
      codeGenerator.getBuilder().CreateStore(or_val, lhsAddr, "orstore");
      break;
    }
    default:
      cerr << "Error: Unsupported assignment operator." << endl;
      return nullptr;
    }
    // Return the assigned value
    return rhsValue;
  }

private:
  ASTNode *unary_expression;
  AssignmentOperator assOp;
  ASTNode *assignment_expression;
};

class ArrayAccessNode : public ASTNode {
public:
  ArrayAccessNode(ASTNode *postfix_expression, ASTNode *expression)
      : ASTNode(NodeType::ArrayAccess), postfix_expression(postfix_expression),
        expression(expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {postfix_expression, expression}, depth, false);
  }

  vector<string> getPlainSymbol() const {

    auto ret = postfix_expression->getPlainSymbol();
    auto b = expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return postfix_expression->check_semantics() &&
           expression->check_semantics();
  }

  m_Value get() const { return postfix_expression->get(); }

  m_Value get_for_optim() const {
    m_Value val = postfix_expression->get_for_optim();
    val.s += "[" + expression->dump_ast() + "]";
    return val;
  }

  m_Value get_value_if_possible() const {

    if (!constant_prop)
      return m_Value();
    m_Value v = get();
    v.s += "[" + expression->dump_ast() + "]";
    return codeGenerator.get_mval(v.s);
  }

  ASTNode *optimise() const {
    auto ret = new ArrayAccessNode(*this);
    ret->postfix_expression = postfix_expression->optimise();

    ret->expression = expression->optimise();

    return ret;
  }

  Value *codegen() {

    cout << "Array access node" << endl;

    bool prev_get_as_lvalue = get_as_lvalue;
    get_as_lvalue = true;
    Value *postFixValue = postfix_expression->codegen();
    get_as_lvalue = false;
    Value *indexValue = expression->codegen();
    get_as_lvalue = prev_get_as_lvalue;

    if (!postFixValue->getType()->isPointerTy()) {
      throw std::runtime_error("Array access on non-pointer type");
    }

    llvm::Type *element_type = postFixValue->getType()->getPointerElementType();

    if (llvm::isa<llvm::ArrayType>(element_type)) {

      llvm::Value *zero_val = llvm::ConstantInt::get(codeGenerator.getContext(),
                                                     llvm::APInt(32, 0));

      llvm::Value *val_to_ret = codeGenerator.getBuilder().CreateGEP(
          element_type, postFixValue, {zero_val, indexValue}, "geparray");

      cout << "Jeeelo" << endl;

      if (get_as_lvalue)
        return val_to_ret;

      return codeGenerator.getBuilder().CreateLoad(
          element_type->getArrayElementType(), val_to_ret, "arrayload");

    } else {

      llvm::Value *load_pointer = codeGenerator.getBuilder().CreateLoad(
          element_type, postFixValue, "pointerload");

      llvm::Type *load_pointer_type =
          load_pointer->getType()->getPointerElementType();

      llvm::Value *zero_val = llvm::ConstantInt::get(codeGenerator.getContext(),
                                                     llvm::APInt(32, 0));
      llvm::Value *val_to_ret = codeGenerator.getBuilder().CreateGEP(
          load_pointer_type, load_pointer, {indexValue}, "geppointer");

      if (get_as_lvalue)
        return val_to_ret;

      return codeGenerator.getBuilder().CreateLoad(
          load_pointer_type, val_to_ret, "poitnervalload");
    }

    /* llvm::Value *zero = llvm::ConstantInt::get( */
    /*     llvm::Type::getInt32Ty(codeGenerator.getContext()), 0); */
    /* llvm::Value *indices[] = {zero, indexValue}; */
    /**/
    /* llvm::Value *val_to_ret = */
    /* codeGenerator.getBuilder().CreateGEP(element_type, postFixValue,
     * indices); */
    /* return codeGenerator.getBuilder().CreateLoad(element_type, val_to_ret);
     */
  }

private:
  ASTNode *postfix_expression;
  ASTNode *expression;
};

class FunctionCallNode : public ASTNode {
public:
  FunctionCallNode(ASTNode *postfix_expression,
                   ASTNode *argument_expression_list)
      : ASTNode(NodeType::FunctionCall), postfix_expression(postfix_expression),
        argument_expression_list(argument_expression_list) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {postfix_expression, argument_expression_list},
                          depth, false);
  }

  vector<string> getPlainSymbol() const { return {"*"}; }

  bool check_semantics() {
    return postfix_expression->check_semantics() &&
           argument_expression_list->check_semantics();
  }

  m_Value get_value_if_possible() const { return m_Value(); }
  ASTNode *optimise() const { return new FunctionCallNode(*this); }

  Value *codegen() {

    string function_name = postfix_expression->get().s;
    llvm::Function *function = codeGenerator.getFunction(function_name);

    if (function == nullptr) {
      throw std::runtime_error("Function " + function_name + " not found.");
    }

    vector<ASTNode *> args = argument_expression_list->getChildren();

    /* if (args.size() != function->arg_size()) { */
    /*   throw std::runtime_error("Function " + function_name + */
    /*                            " called with wrong number of arguments.");
     */
    /* } */

    vector<Value *> arguments;
    for (auto arg : args) {
      arguments.push_back(arg->codegen());
    }

    if (function->getReturnType()->isVoidTy()) {
      return codeGenerator.getBuilder().CreateCall(function, arguments);
    }

    return codeGenerator.getBuilder().CreateCall(function, arguments,
                                                 function_name + "_call");
  }

private:
  ASTNode *postfix_expression;
  ASTNode *argument_expression_list;
};

class ArgumentExpressionListNode : public ASTNode {
public:
  ArgumentExpressionListNode() : ASTNode(NodeType::ArgumentExpressionList) {}

  ASTNode *optimise() const {
    auto ret = new ArgumentExpressionListNode(*this);
    return ret;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }
};

class UnaryExpressionNode : public ASTNode {
public:
  UnaryExpressionNode(UnaryOperator un_op, ASTNode *unary_expression)
      : ASTNode(NodeType::UnaryExpressionNode),
        unary_expression(unary_expression), un_op(un_op) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += "Prefix: UnOp" + unaryOperatorToString(un_op) + "{ \n";
    result += unary_expression->dump_ast(depth + 1);
    result += formatSpacing(depth) + "} \n";
    return result;
  }

  vector<string> getPlainSymbol() const {
    return unary_expression->getPlainSymbol();
  }
  m_Value get() const { return unary_expression->get(); }

  m_Value get_for_optim() const {

    string pref = un_op == UnaryOperator::MUL_OP ? "*" : "";
    m_Value name = unary_expression->get_for_optim();
    name.s = pref + name.s;
    return name;
  }

  m_Value get_value_if_possible() {
    m_Value val = unary_expression->get_value_if_possible();
    m_Value name = get_for_optim();

    switch (un_op) {
    case UnaryOperator::MUL_OP: {

      if (!constant_prop)
        return m_Value();

      return codeGenerator.get_mval(name.s);
    }
    case UnaryOperator::PLUS:
      return val;
    case UnaryOperator::MINUS:
      return -val;
    case UnaryOperator::LOGICAL_NOT:
      return !val;
    case UnaryOperator::BITWISE_NOT:
      return ~val;
    default:
      return m_Value();
    }
  }

  ASTNode *optimise() const {

    if (un_op == UnaryOperator::ADDRESS_OF) {
      constant_prop = false;
    }

    auto ret = new UnaryExpressionNode(*this);
    ret->unary_expression = unary_expression->optimise();
    return ret;
  }

  Value *codegen() {
    bool old_get_as_lvalue = get_as_lvalue;
    if (un_op == UnaryOperator::INC_OP || un_op == UnaryOperator::DEC_OP ||
        un_op == UnaryOperator::ADDRESS_OF) {
      get_as_lvalue = true;
    }
    Value *val = unary_expression->codegen();
    get_as_lvalue = old_get_as_lvalue;

    switch (un_op) {
    case UnaryOperator::INC_OP: {
      llvm::Type *val_type = val->getType()->getPointerElementType();
      Value *old_val =
          codeGenerator.getBuilder().CreateLoad(val_type, val, "INC_OP_LOAD");

      Value *one;
      if (val_type->isPointerTy()) {
        // If it's a pointer type, increment by the size of the pointed-to
        // type
        one = codeGenerator.getBuilder().getInt32(
            codeGenerator.getDataLayout().getTypeAllocSize(val_type));

      } else {
        // Otherwise, increment by 1
        one = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true);
      }

      Value *new_val =
          codeGenerator.getBuilder().CreateAdd(old_val, one, "inc");

      codeGenerator.getBuilder().CreateStore(new_val, val);
      return new_val;
    }
    case UnaryOperator::DEC_OP: {
      llvm::Type *val_type = val->getType()->getPointerElementType();
      Value *old_val =
          codeGenerator.getBuilder().CreateLoad(val_type, val, "DEC_OP_LOAD");

      Value *one;
      if (val_type->isPointerTy()) {
        // If it's a pointer type, increment by the size of the pointed-to
        // type
        one = codeGenerator.getBuilder().getInt32(
            codeGenerator.getDataLayout().getTypeAllocSize(val_type));

      } else {
        // Otherwise, increment by 1
        one = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true);
      }

      Value *new_val =
          codeGenerator.getBuilder().CreateSub(old_val, one, "inc");
      codeGenerator.getBuilder().CreateStore(new_val, val);
      return new_val;
    }

    case UnaryOperator::SIZEOF:
      return llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(codeGenerator.getContext()),
          val->getType()->getPrimitiveSizeInBits() / 8, true);

    case UnaryOperator::ALIGNOF:
      return llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(codeGenerator.getContext()),
          val->getType()->getPrimitiveSizeInBits() / 8, true);

    case UnaryOperator::ADDRESS_OF:
      return val;

    case UnaryOperator::MUL_OP:
      return codeGenerator.getBuilder().CreateLoad(
          val->getType()->getPointerElementType(), val, "deref");
    case UnaryOperator::PLUS:
      return val;
    case UnaryOperator::MINUS: {
      llvm::Value *zero = llvm::ConstantInt::get(
          llvm::Type::getInt32Ty(codeGenerator.getContext()), 0);
      make_lhs_rhs_compatible(val, zero);
      return codeGenerator.getBuilder().CreateSub(zero, val, "neg");
    }
    case UnaryOperator::BITWISE_NOT:

      return codeGenerator.getBuilder().CreateNot(val, "bitnot");
    case UnaryOperator::LOGICAL_NOT: {
      llvm::Value *zero = llvm::ConstantInt::get(
          llvm::Type::getInt1Ty(codeGenerator.getContext()), 0);
      make_lhs_rhs_compatible(val, zero);
      Value *cmp = codeGenerator.getBuilder().CreateICmpNE(val, zero, "lognot");

      Value *boolVal = codeGenerator.getBuilder().CreateZExt(
          cmp, codeGenerator.getBuilder().getInt1Ty(), "lognotcmp");
      return codeGenerator.getBuilder().CreateNot(boolVal, "lognot2");
    }
    }

    return val;
  }

private:
  ASTNode *unary_expression;
  UnaryOperator un_op;
};

class PostfixExpressionNode : public ASTNode {
public:
  PostfixExpressionNode(ASTNode *primary_expression, UnaryOperator postFixOp)
      : ASTNode(NodeType::PostfixExpression),
        primary_expression(primary_expression), postFixOp(postFixOp) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += unaryOperatorToString(postFixOp) + "{ \n";
    result += primary_expression->dump_ast(depth + 1);
    result += formatSpacing(depth) + "} \n";
    return result;
  }

  m_Value get() const { return primary_expression->get(); }

  vector<string> getPlainSymbol() const {
    return primary_expression->getPlainSymbol();
  }

  ASTNode *optimise() const {
    auto ret = new PostfixExpressionNode(*this);
    ret->primary_expression = primary_expression->optimise();
    return ret;
  }

  m_Value get_value_if_possible() const { return m_Value(); }

  bool check_semantics() { return primary_expression->check_semantics(); }

  Value *codegen() {
    bool old_get_as_lvalue = get_as_lvalue;
    if (postFixOp == UnaryOperator::INC_OP ||
        postFixOp == UnaryOperator::DEC_OP) {
      get_as_lvalue = true;
    }
    Value *val = primary_expression->codegen();
    get_as_lvalue = old_get_as_lvalue;

    switch (postFixOp) {

    case UnaryOperator::INC_OP: {
      llvm::Type *val_type = val->getType()->getPointerElementType();
      Value *old_val =
          codeGenerator.getBuilder().CreateLoad(val_type, val, "INC_OP_LOAD2");

      Value *one;
      if (val_type->isPointerTy()) {
        // If it's a pointer type, increment by the size of the pointed-to
        // type
        one = codeGenerator.getBuilder().getInt32(
            codeGenerator.getDataLayout().getTypeAllocSize(val_type));

      } else {
        // Otherwise, increment by 1
        one = llvm::ConstantInt::get(

            llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true);
      }

      make_lhs_rhs_compatible(one, old_val);

      Value *new_val =
          codeGenerator.getBuilder().CreateAdd(old_val, one, "inc");
      codeGenerator.getBuilder().CreateStore(new_val, val);
      return old_val;
    }
    case UnaryOperator::DEC_OP: {

      llvm::Type *val_type = val->getType()->getPointerElementType();

      Value *old_val =
          codeGenerator.getBuilder().CreateLoad(val_type, val, "DEC_OP_LOAD2");

      Value *one;
      if (val_type->isPointerTy()) {
        // If it's a pointer type, increment by the size of the pointed-to
        // type
        one = codeGenerator.getBuilder().getInt32(
            codeGenerator.getDataLayout().getTypeAllocSize(val_type));

      } else {
        // Otherwise, increment by 1
        one = llvm::ConstantInt::get(
            llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true);
      }

      Value *new_val =
          codeGenerator.getBuilder().CreateSub(old_val, one, "dec");
      codeGenerator.getBuilder().CreateStore(new_val, val);
      return old_val;
    }
    default:
      return val;
    }
  }

private:
  ASTNode *primary_expression;
  UnaryOperator postFixOp;
};

class ConditionalExpressionNode : public ASTNode {
public:
  ConditionalExpressionNode(ASTNode *logical_or_expression, ASTNode *expression,
                            ASTNode *conditional_expression)
      : ASTNode(NodeType::ConditionalExpression),
        logical_or_expression(logical_or_expression), expression(expression),
        conditional_expression(conditional_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {logical_or_expression, expression, conditional_expression},
        depth, false);
  }

  bool check_semantics() {
    return logical_or_expression->check_semantics() &&
           expression->check_semantics() &&
           conditional_expression->check_semantics();
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = logical_or_expression->getPlainSymbol();
    vector<string> b = expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    b = conditional_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  m_Value get_value_if_possible() const {

    m_Value logical_or = logical_or_expression->get_value_if_possible();

    if (logical_or.type == ActualValueType::INTEGER) {
      if (logical_or.i) {
        return expression->get_value_if_possible();
      } else {
        return conditional_expression->get_value_if_possible();
      }
    }

    if (logical_or.type == ActualValueType::FLOATING) {
      if (logical_or.f) {
        return expression->get_value_if_possible();
      } else {
        return conditional_expression->get_value_if_possible();
      }
    }

    return m_Value();
  }

  ASTNode *optimise() const {

    auto ret = new ConditionalExpressionNode(*this);
    ret->logical_or_expression->optimise();
    ret->expression = expression->optimise();
    ret->conditional_expression = conditional_expression->optimise();

    m_Value logical_or = logical_or_expression->get_value_if_possible();

    if (logical_or.type == ActualValueType::INTEGER) {
      if (logical_or.i) {
        return ret->expression;
      } else {
        return ret->conditional_expression;
      }
    }

    if (logical_or.type == ActualValueType::FLOATING) {
      if (logical_or.f) {
        return ret->expression;
      } else {
        return ret->conditional_expression;
      }
    }

    return ret;
  }

  Value *codegen() {

    Value *conditionValue = logical_or_expression->codegen();

    make_bool(conditionValue);

    if (conditionValue->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      conditionValue = codeGenerator.getBuilder().CreateIntCast(
          conditionValue, llvm::Type::getInt1Ty(codeGenerator.getContext()),
          true);
    }

    Value *condition = codeGenerator.getBuilder().CreateICmpNE(
        conditionValue,
        llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(codeGenerator.getContext()), 0, true),
        "ifcond");

    llvm::Function *function =
        codeGenerator.getBuilder().GetInsertBlock()->getParent();

    llvm::BasicBlock *then_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "then", function);
    llvm::BasicBlock *else_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "else");
    llvm::BasicBlock *merge_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "ifcont");

    codeGenerator.getBuilder().CreateCondBr(condition, then_block, else_block);

    codeGenerator.getBuilder().SetInsertPoint(then_block);
    Value *thenValue = expression->codegen();
    codeGenerator.getBuilder().CreateBr(
        merge_block); // Branch to merge block after executing thenBlock

    // Set insertion point for the "else" block
    function->getBasicBlockList().push_back(else_block);
    codeGenerator.getBuilder().SetInsertPoint(else_block);
    Value *elseValue = conditional_expression->codegen();
    codeGenerator.getBuilder().CreateBr(
        merge_block); // Branch to merge block after executing elseBlock

    // Set insertion point for the merge block
    function->getBasicBlockList().push_back(merge_block);
    codeGenerator.getBuilder().SetInsertPoint(merge_block);

    // Create phi node to merge the results from thenBlock and elseBlock
    llvm::PHINode *phiNode =
        codeGenerator.getBuilder().CreatePHI(thenValue->getType(), 2, "iftmp");
    phiNode->addIncoming(thenValue, then_block);
    phiNode->addIncoming(elseValue, else_block);

    return phiNode;
  }

private:
  ASTNode *logical_or_expression;
  ASTNode *expression;
  ASTNode *conditional_expression;
};

/**/
/*   Value *codegen() { */
/*     Value *lhs = logical_or_expression->codegen(); */
/*     Value *rhs = logical_and_expression->codegen(); */
/*     if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) { */
/*       lhs = codeGenerator.getBuilder().CreateIntCast( */
/*           lhs, llvm::Type::getInt1Ty(codeGenerator.getContext()), true); */
/*     } */
/**/
/*     if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) { */
/*       rhs = codeGenerator.getBuilder().CreateIntCast( */
/*           rhs, llvm::Type::getInt1Ty(codeGenerator.getContext()), true); */
/*     } */
/**/
/*     return codeGenerator.getBuilder().CreateOr(lhs, rhs, "or"); */
/*   } */
/**/
/* private: */
/*   ASTNode *logical_or_expression; */
/*   ASTNode *logical_and_expression; */
/* }; */

/* class LogicalAndExpressionNode : public ASTNode { */
/* public: */
/*   LogicalAndExpressionNode(ASTNode *logical_and_expression, */
/*                            ASTNode *inclusive_or_expression) */
/*       : ASTNode(NodeType::LogicalAndExpression), */
/*         logical_and_expression(logical_and_expression), */
/*         inclusive_or_expression(inclusive_or_expression) {} */
/**/
/*   string dump_ast(int depth = 0) const { */
/*     return dumpParameters( */
/*         this, {logical_and_expression, inclusive_or_expression}, depth,
 * false); */
/*   } */
/**/
/*   bool check_semantics() { */
/*     return logical_and_expression->check_semantics() && */
/*            inclusive_or_expression->check_semantics(); */
/*   } */
/**/
/*   Value *codegen() { */
/*     Value *lhs = logical_and_expression->codegen(); */
/*     Value *rhs = inclusive_or_expression->codegen(); */
/**/
/*     if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) { */
/*       lhs = codeGenerator.getBuilder().CreateIntCast( */
/*           lhs, llvm::Type::getInt1Ty(codeGenerator.getContext()), true); */
/*     } */
/*     if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) { */
/*       rhs = codeGenerator.getBuilder().CreateIntCast( */
/*           rhs, llvm::Type::getInt1Ty(codeGenerator.getContext()), true); */
/*     } */
/**/
/*     return codeGenerator.getBuilder().CreateAnd(lhs, rhs, "and"); */
/*   } */
/**/
/* private: */
/*   ASTNode *logical_and_expression; */
/*   ASTNode *inclusive_or_expression; */
/* }; */

class InclusiveOrExpressionNode : public ASTNode {
public:
  InclusiveOrExpressionNode(ASTNode *inclusive_or_expression,
                            ASTNode *exclusive_or_expression)
      : ASTNode(NodeType::InclusiveOrExpression),
        inclusive_or_expression(inclusive_or_expression),
        exclusive_or_expression(exclusive_or_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {inclusive_or_expression, exclusive_or_expression}, depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = inclusive_or_expression->getPlainSymbol();
    vector<string> b = exclusive_or_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return inclusive_or_expression->check_semantics() &&
           exclusive_or_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = inclusive_or_expression->get_value_if_possible();
    m_Value rhs = exclusive_or_expression->get_value_if_possible();
    if (lhs.type == ActualValueType::NO_VALUE ||
        rhs.type == ActualValueType::NO_VALUE) {
      return m_Value();
    }

    if (lhs.type != ActualValueType::INTEGER ||
        rhs.type != ActualValueType::INTEGER) {
      throw std::runtime_error("lhs or rhs is not an integer");
    }

    lhs.i = lhs.i | rhs.i;
    return lhs;
  }

  ASTNode *optimise() const {
    auto ret = new InclusiveOrExpressionNode(*this);
    ret->inclusive_or_expression = inclusive_or_expression->optimise();
    ret->exclusive_or_expression = exclusive_or_expression->optimise();

    m_Value v = get_value_if_possible();

    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }

    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = inclusive_or_expression->codegen();
    Value *rhs = exclusive_or_expression->codegen();

    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateOr(lhs, rhs, "or");
  }

private:
  ASTNode *inclusive_or_expression;
  ASTNode *exclusive_or_expression;
};

class ExclusiveOrExpressionNode : public ASTNode {
public:
  ExclusiveOrExpressionNode(ASTNode *exclusive_or_expression,
                            ASTNode *and_expression)
      : ASTNode(NodeType::ExclusiveOrExpression),
        exclusive_or_expression(exclusive_or_expression),
        and_expression(and_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {exclusive_or_expression, and_expression},
                          depth, false);
  }

  bool check_semantics() {
    return exclusive_or_expression->check_semantics() &&
           and_expression->check_semantics();
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = exclusive_or_expression->getPlainSymbol();
    vector<string> b = and_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = exclusive_or_expression->get_value_if_possible();
    m_Value rhs = and_expression->get_value_if_possible();
    if (lhs.type == ActualValueType::NO_VALUE ||
        rhs.type == ActualValueType::NO_VALUE) {
      return m_Value();
    }
    if (lhs.type != ActualValueType::INTEGER ||
        rhs.type != ActualValueType::INTEGER) {
      throw std::runtime_error("lhs or rhs is not an integer");
    }
    lhs.i = lhs.i ^ rhs.i;
    return lhs;
  }

  ASTNode *optimise() const {
    auto ret = new ExclusiveOrExpressionNode(*this);
    ret->exclusive_or_expression = exclusive_or_expression->optimise();
    ret->and_expression = and_expression->optimise();

    m_Value v = get_value_if_possible();

    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }

    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = exclusive_or_expression->codegen();
    Value *rhs = and_expression->codegen();

    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateXor(lhs, rhs, "xor");
  }

private:
  ASTNode *exclusive_or_expression;
  ASTNode *and_expression;
};

class AndExpressionNode : public ASTNode {
public:
  AndExpressionNode(ASTNode *and_expression, ASTNode *equality_expression)
      : ASTNode(NodeType::AndExpression), and_expression(and_expression),
        equality_expression(equality_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {and_expression, equality_expression}, depth,
                          false);
  }

  bool check_semantics() {
    return and_expression->check_semantics() &&
           equality_expression->check_semantics();
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = and_expression->getPlainSymbol();
    vector<string> b = equality_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = and_expression->get_value_if_possible();
    m_Value rhs = equality_expression->get_value_if_possible();
    if (lhs.type == ActualValueType::NO_VALUE ||
        rhs.type == ActualValueType::NO_VALUE) {
      return m_Value();
    }
    if (lhs.type != ActualValueType::INTEGER ||
        rhs.type != ActualValueType::INTEGER) {
      throw std::runtime_error("lhs or rhs is not an integer");
    }
    lhs.i = lhs.i & rhs.i;
    return lhs;
  }

  ASTNode *optimise() const {
    auto ret = new AndExpressionNode(*this);
    ret->and_expression = and_expression->optimise();
    ret->equality_expression = equality_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = and_expression->codegen();
    Value *rhs = equality_expression->codegen();

    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateAnd(lhs, rhs, "and");
  }

private:
  ASTNode *and_expression;
  ASTNode *equality_expression;
};

class EqualityExpressionNode : public ASTNode {
public:
  EqualityExpressionNode(ASTNode *equality_expression,
                         ASTNode *relational_expression)
      : ASTNode(NodeType::EqualityExpression),
        equality_expression(equality_expression),
        relational_expression(relational_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {equality_expression, relational_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = equality_expression->getPlainSymbol();
    vector<string> b = relational_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = equality_expression->get_value_if_possible();
    m_Value rhs = relational_expression->get_value_if_possible();

    return (lhs == rhs);
  }

  ASTNode *optimise() const {
    auto ret = new EqualityExpressionNode(*this);
    ret->equality_expression = equality_expression->optimise();
    ret->relational_expression = relational_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);
    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {
      return codeGenerator.getBuilder().CreateFCmpOEQ(lhs, rhs, "equal");
    }

    return codeGenerator.getBuilder().CreateICmpEQ(lhs, rhs, "equal");
  }

private:
  ASTNode *equality_expression;
  ASTNode *relational_expression;
};

class NonEqualityExpressionNode : public ASTNode {
public:
  NonEqualityExpressionNode(ASTNode *equality_expression,
                            ASTNode *relational_expression)
      : ASTNode(NodeType::NonEqualityExpression),
        equality_expression(equality_expression),
        relational_expression(relational_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {equality_expression, relational_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = equality_expression->getPlainSymbol();
    vector<string> b = relational_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = equality_expression->get_value_if_possible();
    m_Value rhs = relational_expression->get_value_if_possible();

    return (lhs != rhs);
  }

  ASTNode *optimise() const {
    auto ret = new NonEqualityExpressionNode(*this);
    ret->equality_expression = equality_expression->optimise();
    ret->relational_expression = relational_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }

    return ret;
  }
  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {
      return codeGenerator.getBuilder().CreateFCmpONE(lhs, rhs, "nequal");
    }

    return codeGenerator.getBuilder().CreateICmpNE(lhs, rhs, "nequal");
  }

private:
  ASTNode *equality_expression;
  ASTNode *relational_expression;
};

class LessThanExpressionNode : public ASTNode {
public:
  LessThanExpressionNode(ASTNode *relational_expression,
                         ASTNode *shift_expression)
      : ASTNode(NodeType::LessThanExpression),
        relational_expression(relational_expression),
        shift_expression(shift_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {relational_expression, shift_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = relational_expression->getPlainSymbol();
    vector<string> b = shift_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = relational_expression->get_value_if_possible();
    m_Value rhs = shift_expression->get_value_if_possible();

    return (lhs < rhs);
  }

  ASTNode *optimise() const {
    auto ret = new LessThanExpressionNode(*this);
    ret->relational_expression = relational_expression->optimise();
    ret->shift_expression = shift_expression->optimise();

    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }
  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {
      codeGenerator.getBuilder().CreateFCmpOLT(lhs, rhs, "lt");
    }

    return codeGenerator.getBuilder().CreateICmpSLT(lhs, rhs, "lt");
  }

private:
  ASTNode *relational_expression;
  ASTNode *shift_expression;
};

class GreaterThanExpressionNode : public ASTNode {
public:
  GreaterThanExpressionNode(ASTNode *relational_expression,
                            ASTNode *shift_expression)
      : ASTNode(NodeType::GreaterThanExpression),
        relational_expression(relational_expression),
        shift_expression(shift_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {relational_expression, shift_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = relational_expression->getPlainSymbol();
    vector<string> b = shift_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }
  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = relational_expression->get_value_if_possible();
    m_Value rhs = shift_expression->get_value_if_possible();

    return (lhs > rhs);
  }

  ASTNode *optimise() const {
    auto ret = new GreaterThanExpressionNode(*this);
    ret->relational_expression = relational_expression->optimise();
    ret->shift_expression = shift_expression->optimise();

    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {
      return codeGenerator.getBuilder().CreateFCmpOGT(lhs, rhs, "gt");
    }
    return codeGenerator.getBuilder().CreateICmpSGT(lhs, rhs, "gt");
  }

private:
  ASTNode *relational_expression;
  ASTNode *shift_expression;
};

class LessOrEqualExpressionNode : public ASTNode {
public:
  LessOrEqualExpressionNode(ASTNode *relational_expression,
                            ASTNode *shift_expression)
      : ASTNode(NodeType::LessOrEqualExpression),
        relational_expression(relational_expression),
        shift_expression(shift_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {relational_expression, shift_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = relational_expression->getPlainSymbol();
    vector<string> b = shift_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }
  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = relational_expression->get_value_if_possible();
    m_Value rhs = shift_expression->get_value_if_possible();

    return (lhs <= rhs);
  }

  ASTNode *optimise() const {
    auto ret = new LessOrEqualExpressionNode(*this);
    ret->relational_expression = relational_expression->optimise();
    ret->shift_expression = shift_expression->optimise();

    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }
  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {
      return codeGenerator.getBuilder().CreateFCmpOLE(lhs, rhs, "le");
    }
    return codeGenerator.getBuilder().CreateICmpSLE(lhs, rhs, "le");
  }

private:
  ASTNode *relational_expression;
  ASTNode *shift_expression;
};

class GreaterOrEqualExpressionNode : public ASTNode {
public:
  GreaterOrEqualExpressionNode(ASTNode *relational_expression,
                               ASTNode *shift_expression)
      : ASTNode(NodeType::GreaterOrEqualExpression),
        relational_expression(relational_expression),
        shift_expression(shift_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {relational_expression, shift_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = relational_expression->getPlainSymbol();
    vector<string> b = shift_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }
  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    m_Value lhs = relational_expression->get_value_if_possible();
    m_Value rhs = shift_expression->get_value_if_possible();

    return (lhs >= rhs);
  }

  ASTNode *optimise() const {
    auto ret = new GreaterOrEqualExpressionNode(*this);
    ret->relational_expression = relational_expression->optimise();
    ret->shift_expression = shift_expression->optimise();

    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);
    if (lhs->getType()->getTypeID() == llvm::Type::DoubleTyID) {

      return codeGenerator.getBuilder().CreateFCmpOGE(lhs, rhs, "ge");
    }
    return codeGenerator.getBuilder().CreateICmpSGE(lhs, rhs, "ge");
  }

private:
  ASTNode *relational_expression;
  ASTNode *shift_expression;
};

class LeftShiftExpressionNode : public ASTNode {
public:
  LeftShiftExpressionNode(ASTNode *shift_expression,
                          ASTNode *additive_expression)
      : ASTNode(NodeType::LeftShiftExpression),
        shift_expression(shift_expression),
        additive_expression(additive_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {shift_expression, additive_expression}, depth,
                          false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = shift_expression->getPlainSymbol();
    vector<string> b = additive_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    auto lhs = shift_expression->get_value_if_possible();
    auto rhs = additive_expression->get_value_if_possible();
    return (lhs << rhs);
  }

  ASTNode *optimise() const {
    auto ret = new LeftShiftExpressionNode(*this);
    ret->shift_expression = shift_expression->optimise();
    ret->additive_expression = additive_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateShl(lhs, rhs, "shl");
  }

private:
  ASTNode *shift_expression;
  ASTNode *additive_expression;
};

class RightShiftExpressionNode : public ASTNode {
public:
  RightShiftExpressionNode(ASTNode *shift_expression,
                           ASTNode *additive_expression)
      : ASTNode(NodeType::RightShiftExpression),
        shift_expression(shift_expression),
        additive_expression(additive_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {shift_expression, additive_expression}, depth,
                          false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = shift_expression->getPlainSymbol();
    vector<string> b = additive_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  m_Value get_value_if_possible() const {
    auto lhs = shift_expression->get_value_if_possible();
    auto rhs = additive_expression->get_value_if_possible();
    return (lhs >> rhs);
  }

  ASTNode *optimise() const {
    auto ret = new RightShiftExpressionNode(*this);
    ret->shift_expression = shift_expression->optimise();
    ret->additive_expression = additive_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type != ActualValueType::NO_VALUE) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }
  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }
  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateAShr(lhs, rhs, "ashr");
  }

private:
  ASTNode *shift_expression;
  ASTNode *additive_expression;
};

class AdditiveExpressionNode : public ASTNode {
public:
  AdditiveExpressionNode(ASTNode *additive_expression,
                         ASTNode *multiplicative_expression)
      : ASTNode(NodeType::AdditiveExpression),
        additive_expression(additive_expression),
        multiplicative_expression(multiplicative_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {additive_expression, multiplicative_expression}, depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = additive_expression->getPlainSymbol();
    vector<string> b = multiplicative_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    auto lhs = additive_expression->get_value_if_possible();
    auto rhs = multiplicative_expression->get_value_if_possible();
    return (lhs + rhs);
  }

  ASTNode *optimise() const {
    auto ret = new AdditiveExpressionNode(*this);
    ret->additive_expression = additive_expression->optimise();
    ret->multiplicative_expression = multiplicative_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    return codeGenerator.getBuilder().CreateAdd(lhs, rhs, "add");
  }

private:
  ASTNode *additive_expression;
  ASTNode *multiplicative_expression;
};

class SubExpressionNode : public ASTNode {
public:
  SubExpressionNode(ASTNode *additive_expression,
                    ASTNode *multiplicative_expression)
      : ASTNode(NodeType::SubExpression),
        additive_expression(additive_expression),
        multiplicative_expression(multiplicative_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {additive_expression, multiplicative_expression}, depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = additive_expression->getPlainSymbol();
    vector<string> b = multiplicative_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    auto lhs = additive_expression->get_value_if_possible();
    auto rhs = multiplicative_expression->get_value_if_possible();
    return (lhs - rhs);
  }

  ASTNode *optimise() const {
    auto ret = new SubExpressionNode(*this);
    ret->additive_expression = additive_expression->optimise();
    ret->multiplicative_expression = multiplicative_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);

    return codeGenerator.getBuilder().CreateSub(lhs, rhs, "sub");
  }

private:
  ASTNode *additive_expression;
  ASTNode *multiplicative_expression;
};

class MultiplicativeExpressionNode : public ASTNode {
public:
  MultiplicativeExpressionNode(ASTNode *multiplicative_expression,
                               ASTNode *cast_expression)
      : ASTNode(NodeType::MultiplicativeExpression),
        multiplicative_expression(multiplicative_expression),
        cast_expression(cast_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {multiplicative_expression, cast_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = multiplicative_expression->getPlainSymbol();
    vector<string> b = cast_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    auto lhs = multiplicative_expression->get_value_if_possible();
    auto rhs = cast_expression->get_value_if_possible();
    return (lhs * rhs);
  }

  ASTNode *optimise() const {
    auto ret = new MultiplicativeExpressionNode(*this);
    ret->multiplicative_expression = multiplicative_expression->optimise();
    ret->cast_expression = cast_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }
  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();

    make_lhs_rhs_compatible(lhs, rhs);
    return codeGenerator.getBuilder().CreateMul(lhs, rhs, "mul");
  }

private:
  ASTNode *multiplicative_expression;
  ASTNode *cast_expression;
};

class DivExpressionNode : public ASTNode {
public:
  DivExpressionNode(ASTNode *multiplicative_expression,
                    ASTNode *cast_expression)
      : ASTNode(NodeType::DivExpression),
        multiplicative_expression(multiplicative_expression),
        cast_expression(cast_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {multiplicative_expression, cast_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = multiplicative_expression->getPlainSymbol();
    vector<string> b = cast_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }
  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  m_Value get_value_if_possible() const {
    auto lhs = multiplicative_expression->get_value_if_possible();
    auto rhs = cast_expression->get_value_if_possible();
    return (lhs / rhs);
  }

  ASTNode *optimise() const {
    auto ret = new DivExpressionNode(*this);
    ret->multiplicative_expression = multiplicative_expression->optimise();
    ret->cast_expression = cast_expression->optimise();
    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    make_lhs_rhs_compatible(lhs, rhs);

    return codeGenerator.getBuilder().CreateSDiv(lhs, rhs, "div");
  }

private:
  ASTNode *multiplicative_expression;
  ASTNode *cast_expression;
};

class ModExpressionNode : public ASTNode {
public:
  ModExpressionNode(ASTNode *multiplicative_expression,
                    ASTNode *cast_expression)
      : ASTNode(NodeType::ModExpression),
        multiplicative_expression(multiplicative_expression),
        cast_expression(cast_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {multiplicative_expression, cast_expression},
                          depth, false);
  }

  vector<string> getPlainSymbol() const {
    vector<string> ret = multiplicative_expression->getPlainSymbol();
    vector<string> b = cast_expression->getPlainSymbol();
    ret.insert(ret.end(), b.begin(), b.end());
    return ret;
  }
  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }
  m_Value get_value_if_possible() const {
    auto lhs = multiplicative_expression->get_value_if_possible();
    auto rhs = cast_expression->get_value_if_possible();
    return (lhs % rhs);
  }

  ASTNode *optimise() const {
    auto ret = new ModExpressionNode(*this);
    ret->multiplicative_expression = multiplicative_expression->optimise();
    ret->cast_expression = cast_expression->optimise();

    m_Value v = get_value_if_possible();
    if (v.type == ActualValueType::INTEGER) {
      return new IConstantNode(v.i);
    }
    if (v.type == ActualValueType::FLOATING) {
      return new FConstantNode(v.f);
    }
    return ret;
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    if (lhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("lhs is not an integer");
    }
    if (rhs->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      throw std::runtime_error("rhs is not an integer");
    }

    return codeGenerator.getBuilder().CreateSRem(lhs, rhs, "mod");
  }

private:
  ASTNode *multiplicative_expression;
  ASTNode *cast_expression;
};

#endif

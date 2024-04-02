#ifndef AST_HPP
#define AST_HPP

#include "AST_enums.hpp"
#include "codegen.h"
#include "scoper.h"
#include <iostream>
#include <llvm-14/llvm/IR/BasicBlock.h>
#include <llvm-14/llvm/IR/Function.h>
#include <llvm-14/llvm/IR/IRBuilder.h>
#include <llvm-14/llvm/IR/Instructions.h>
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

static llvm::Type *declaration_type = nullptr;
static vector<llvm::Type *> function_params;

static llvm::IRBuilder<> *curent_builder = nullptr;

static int parameter_list_index = 0;

static llvm::Type *func_ret_type = nullptr;

static map<string, Value *> current_symbol_table;

class ASTNode {
public:
  ASTNode(NodeType type) : type(type) {}

  virtual ~ASTNode() {}

  NodeType getNodeType() const { return type; }

  virtual string dump_ast(int depth = 0) const {

    return formatSpacing(depth) + "Interface Node \n";
    // Implement this method to dump the AST as a string
  }

  void addChild(ASTNode *child) { children.push_back(child); }

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

  virtual m_Value get() {
    throw std::runtime_error("Unimplemented get() function.");
  }

  virtual llvm::Value *codegen() {

    cerr << "Codegen called for " << nodeTypeToString(type) << endl;

    throw std::runtime_error("Unimplemented codegen() function.");
  }

  virtual void buildFunctionParams(llvm::Function *function_decl) {
    cout << "buildFunctionParams called on base class | "
         << nodeTypeToString(type) << endl;
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

  llvm::Value *codegen() {
    cout << "Codegen called for NullPtrNode " << endl;
    return nullptr;
  }

  m_Value get() { throw std::runtime_error("get() called on a NullPtr."); }
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

  Value *codegen() {

    for (auto child : children) {
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

  Value *codegen() {
    declaration_type = nullptr;
    function_params.clear();
    current_symbol_table.clear();

    func_ret_type = declaration_specifiers->getValueType();
    string func_name = declarator->get().s;

    if (codeGenerator.isFunctionDefined(func_name)) {
      return codeGenerator.functions[func_name];
    }

    declarator->fixFunctionParams();

    llvm::FunctionType *function_type =
        llvm::FunctionType::get(func_ret_type, function_params, false);
    llvm::Function *function_decl =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage,
                               func_name, codeGenerator.global_module.get());

    // Store the function in the functions map
    codeGenerator.functions[func_name] = function_decl;

    // Create a new basic block to start insertion into.
    llvm::BasicBlock *basic_block = llvm::BasicBlock::Create(
        codeGenerator.getContext(), "entry", function_decl);

    curent_builder = new llvm::IRBuilder<>(basic_block);

    declarator->buildFunctionParams(function_decl);

    codeGenerator.push_symbol_table();
    codeGenerator.push_func_args(current_symbol_table);

    compound_statement->codegen();

    if (func_ret_type->isVoidTy()) {
      curent_builder->CreateRetVoid();
    }

    codeGenerator.pop_func_args();
    codeGenerator.pop_symbol_table();

    curent_builder->CreateRet(llvm::Constant::getNullValue(func_ret_type));

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

  llvm::Type *getValueType() {
    llvm::Type *val = getCurrType(specifier, codeGenerator.getContext());
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
    for (auto child : children) {
      child->codegen();
    }
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

private:
  ASTNode *constant_expression;
  ASTNode *statement;
};

class DefaultLabelStatementNode : public ASTNode {
public:
  DefaultLabelStatementNode(ASTNode *statement)
      : ASTNode(NodeType::DefaultLabelStatement), statement(statement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {statement}, depth, false);
  }

private:
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
    llvm::Value *condition = curent_builder->CreateICmpNE(
        conditionValue,
        llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(codeGenerator.getContext()), 0, true),
        "ifcond");

    llvm::BasicBlock *current_block = curent_builder->GetInsertBlock();
    llvm::Function *function = current_block->getParent();

    llvm::BasicBlock *then_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "then", function);
    llvm::BasicBlock *else_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "else");
    llvm::BasicBlock *merge_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "ifcont");

    curent_builder->CreateCondBr(condition, then_block, else_block);

    curent_builder->SetInsertPoint(then_block);
    codeGenerator.push_symbol_table();
    statement->codegen();
    codeGenerator.pop_symbol_table();
    curent_builder->CreateBr(merge_block);

    function->getBasicBlockList().push_back(else_block);
    curent_builder->SetInsertPoint(else_block);
    if (else_statement->getNodeType() != NodeType::Unimplemented) {
      codeGenerator.push_symbol_table();
      else_statement->codegen();
      codeGenerator.pop_symbol_table();
    }
    curent_builder->CreateBr(merge_block);

    function->getBasicBlockList().push_back(merge_block);
    curent_builder->SetInsertPoint(merge_block);

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

private:
  ASTNode *identifier;
};

class ContinueStatementNode : public ASTNode {
public:
  ContinueStatementNode() : ASTNode(NodeType::ContinueStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {}, depth, false);
  }
};

class BreakStatementNode : public ASTNode {
public:
  BreakStatementNode() : ASTNode(NodeType::BreakStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {}, depth, false);
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

  Value *codegen() {
    if (expression->getNodeType() != NodeType::Unimplemented) {
      return curent_builder->CreateRet(expression->codegen());
    }
    return curent_builder->CreateRetVoid();
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

    llvm::Value *val = nullptr;
    if (initializer->getNodeType() != NodeType::Unimplemented) {
      val = initializer->codegen();
    }
    if (val == nullptr) {
      val = llvm::Constant::getNullValue(declaration_type);
    }
    llvm::AllocaInst *alloca = curent_builder->CreateAlloca(
        declaration_type, nullptr, declarator->get().s);
    curent_builder->CreateStore(val, alloca);
    current_symbol_table[declarator->get().s] = alloca;
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

  Value *codegen() {
    for (auto child : children) {
      child->codegen();
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

  m_Value get() { return direct_declarator->get(); }

  void fixFunctionParams() { direct_declarator->fixFunctionParams(); }

  void buildFunctionParams(llvm::Function *function_decl) {
    direct_declarator->buildFunctionParams(function_decl);
  }

private:
  ASTNode *pointer;
  ASTNode *direct_declarator;
};

class DirectDeclaratorNode : public ASTNode {
public:
  DirectDeclaratorNode(ASTNode *direct_declarator, ASTNode *parameter_type_list)
      : ASTNode(NodeType::DirectDeclarator),
        direct_declarator(direct_declarator),
        parameter_type_list(parameter_type_list) {}

  string dump_ast(int depth = 0) const {

    return dumpParameters(this, {direct_declarator, parameter_type_list}, depth,
                          false);
  }

  m_Value get() { return direct_declarator->get(); }

  bool check_semantics() { return parameter_type_list->check_semantics(); }

  void fixFunctionParams() {

    if (parameter_type_list->getNodeType() == NodeType::Unimplemented) {
      return;
    }
    parameter_type_list->fixFunctionParams();
  }

  void buildFunctionParams(llvm::Function *function_decl) {
    if (parameter_type_list->getNodeType() == NodeType::Unimplemented) {
      return;
    }
    parameter_type_list->buildFunctionParams(function_decl);
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

  bool check_semantics() {
    for (auto child : children) {
      if (!child->check_semantics()) {
        return false;
      }
    }
    return true;
  }

  void fixFunctionParams() {
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

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {declaration_specifiers, declarator}, depth,
                          false);
  }

  bool check_semantics() { return declarator->check_semantics(); }

  void fixFunctionParams() {
    declaration_type = declaration_specifiers->getValueType();
    function_params.push_back(declaration_type);
  }

  void buildFunctionParams(llvm::Function *function_decl) {
    declaration_type = declaration_specifiers->getValueType();

    string name = declarator->get().s;

    llvm::AllocaInst *p =
        curent_builder->CreateAlloca(declaration_type, nullptr, name);
    curent_builder->CreateStore(
        function_decl->arg_begin() + parameter_list_index, p);

    current_symbol_table[name] = p;
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

  m_Value get() { return m_Value(name); }

  bool check_semantics() { return scoperStack.exists(name); }

  Value *codegen() { return current_symbol_table[name]; }

private:
  string name;
};

class IdentifierListNode : public ASTNode {
public:
  IdentifierListNode() : ASTNode(NodeType::IdentifierList) {}

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
};

class ExpressionListNode : public ASTNode {
public:
  ExpressionListNode() : ASTNode(NodeType::ExpressionList) {}

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
  Value *codegen() {
    for (auto child : children) {
      child->codegen();
    }
    return nullptr;
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

  Value *codegen() override {
    // Generate LLVM IR for the right-hand side expression
    Value *rhsValue = assignment_expression->codegen();

    // Get the address of the left-hand side variable or expression
    Value *lhsAddr = unary_expression->codegen();

    // Perform the assignment based on the operator
    switch (assOp) {
    case AssignmentOperator::ASSIGN:
      // Store the value to the address
      curent_builder->CreateStore(rhsValue, lhsAddr);
      break;
    /* case AssignmentOperator::MUL_ASSIGN: */
    /*     // Load the current value from the address, perform multiplication,
     * and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateMul(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::DIV_ASSIGN: */
    /*     // Perform division and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateSDiv(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::MOD_ASSIGN: */
    /*     // Perform modulus and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateSRem(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::ADD_ASSIGN: */
    /*     // Perform addition and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateAdd(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::SUB_ASSIGN: */
    /*     // Perform subtraction and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateSub(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::LEFT_ASSIGN: */
    /*     // Perform left shift and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateShl(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::RIGHT_ASSIGN: */
    /*     // Perform right shift and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateAShr(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::AND_ASSIGN: */
    /*     // Perform bitwise AND and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateAnd(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::XOR_ASSIGN: */
    /*     // Perform bitwise XOR and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateXor(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::OR_ASSIGN: */
    /*     // Perform bitwise OR and store the result */
    /*     curent_builder->CreateStore( */
    /*         curent_builder->CreateOr(curent_builder->CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
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

  bool check_semantics() {
    return postfix_expression->check_semantics() &&
           expression->check_semantics();
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

  bool check_semantics() {
    return postfix_expression->check_semantics() &&
           argument_expression_list->check_semantics();
  }

private:
  ASTNode *postfix_expression;
  ASTNode *argument_expression_list;
};

class ArgumentExpressionListNode : public ASTNode {
public:
  ArgumentExpressionListNode() : ASTNode(NodeType::ArgumentExpressionList) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, children, depth, true);
  }
};

class MemberAccessNode : public ASTNode {
public:
  MemberAccessNode(ASTNode *postfix_expression, ASTNode *identifier)
      : ASTNode(NodeType::MemberAccess), postfix_expression(postfix_expression),
        identifier(identifier) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {postfix_expression, identifier}, depth, false);
  }

  bool check_semantics() { return postfix_expression->check_semantics(); }

private:
  ASTNode *postfix_expression;
  ASTNode *identifier;
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

  bool check_semantics() { return primary_expression->check_semantics(); }

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

  Value *codegen() {

    Value *conditionValue = logical_or_expression->codegen();
    Value *condition = curent_builder->CreateICmpNE(
        conditionValue,
        llvm::ConstantInt::get(
            llvm::Type::getInt1Ty(codeGenerator.getContext()), 0, true),
        "ifcond");

    llvm::Function *function = curent_builder->GetInsertBlock()->getParent();

    llvm::BasicBlock *then_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "then", function);
    llvm::BasicBlock *else_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "else");
    llvm::BasicBlock *merge_block =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "ifcont");

    curent_builder->CreateCondBr(condition, then_block, else_block);

    curent_builder->SetInsertPoint(then_block);
    Value *thenValue = expression->codegen();
    curent_builder->CreateBr(
        merge_block); // Branch to merge block after executing thenBlock

    // Set insertion point for the "else" block
    function->getBasicBlockList().push_back(else_block);
    curent_builder->SetInsertPoint(else_block);
    Value *elseValue = conditional_expression->codegen();
    curent_builder->CreateBr(
        merge_block); // Branch to merge block after executing elseBlock

    // Set insertion point for the merge block
    function->getBasicBlockList().push_back(merge_block);
    curent_builder->SetInsertPoint(merge_block);

    // Create phi node to merge the results from thenBlock and elseBlock
    llvm::PHINode *phiNode = curent_builder->CreatePHI(thenValue->getType(), 2);
    phiNode->addIncoming(thenValue, then_block);
    phiNode->addIncoming(elseValue, else_block);

    return phiNode;
  }

private:
  ASTNode *logical_or_expression;
  ASTNode *expression;
  ASTNode *conditional_expression;
};

class LogicalOrExpressionNode : public ASTNode {
public:
  LogicalOrExpressionNode(ASTNode *logical_or_expression,
                          ASTNode *logical_and_expression)
      : ASTNode(NodeType::LogicalOrExpression),
        logical_or_expression(logical_or_expression),
        logical_and_expression(logical_and_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {logical_or_expression, logical_and_expression},
                          depth, false);
  }

  bool check_semantics() {
    return logical_or_expression->check_semantics() &&
           logical_and_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = logical_or_expression->codegen();
    Value *rhs = logical_and_expression->codegen();
    return curent_builder->CreateOr(lhs, rhs, "or");
  }

private:
  ASTNode *logical_or_expression;
  ASTNode *logical_and_expression;
};

class LogicalAndExpressionNode : public ASTNode {
public:
  LogicalAndExpressionNode(ASTNode *logical_and_expression,
                           ASTNode *inclusive_or_expression)
      : ASTNode(NodeType::LogicalAndExpression),
        logical_and_expression(logical_and_expression),
        inclusive_or_expression(inclusive_or_expression) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(
        this, {logical_and_expression, inclusive_or_expression}, depth, false);
  }

  bool check_semantics() {
    return logical_and_expression->check_semantics() &&
           inclusive_or_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = logical_and_expression->codegen();
    Value *rhs = inclusive_or_expression->codegen();
    return curent_builder->CreateAnd(lhs, rhs, "and");
  }

private:
  ASTNode *logical_and_expression;
  ASTNode *inclusive_or_expression;
};

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

  bool check_semantics() {
    return inclusive_or_expression->check_semantics() &&
           exclusive_or_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = inclusive_or_expression->codegen();
    Value *rhs = exclusive_or_expression->codegen();
    return curent_builder->CreateOr(lhs, rhs, "or");
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

  Value *codegen() {
    Value *lhs = exclusive_or_expression->codegen();
    Value *rhs = and_expression->codegen();
    return curent_builder->CreateXor(lhs, rhs, "xor");
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

  Value *codegen() {
    Value *lhs = and_expression->codegen();
    Value *rhs = equality_expression->codegen();
    return curent_builder->CreateAnd(lhs, rhs, "and");
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

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();
    return curent_builder->CreateICmpEQ(lhs, rhs, "equal");
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

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();
    return curent_builder->CreateICmpNE(lhs, rhs, "nequal");
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
    return curent_builder->CreateICmpSLT(lhs, rhs, "lt");
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
    return curent_builder->CreateICmpSGT(lhs, rhs, "gt");
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
    return curent_builder->CreateICmpSLE(lhs, rhs, "le");
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
    return curent_builder->CreateICmpSGE(lhs, rhs, "ge");
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

  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
    return curent_builder->CreateShl(lhs, rhs, "shl");
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

  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }
  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
    return curent_builder->CreateAShr(lhs, rhs, "ashr");
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

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }
  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();
    return curent_builder->CreateAdd(lhs, rhs, "add");
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

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();
    return curent_builder->CreateSub(lhs, rhs, "sub");
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    return curent_builder->CreateMul(lhs, rhs, "mul");
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    return curent_builder->CreateSDiv(lhs, rhs, "div");
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    return curent_builder->CreateSRem(lhs, rhs, "mod");
  }

private:
  ASTNode *multiplicative_expression;
  ASTNode *cast_expression;
};

class IConstantNode : public ASTNode {
public:
  IConstantNode(int value) : ASTNode(NodeType::IConstant), value(value) {}

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth);
    result += "Integer:" + to_string(value) + "\n";
    return result;
  }

  Value *codegen() {
    return llvm::ConstantInt::get(codeGenerator.getContext(),
                                  llvm::APInt(32, value));
  }

private:
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

  Value *codegen() {
    return llvm::ConstantFP::get(codeGenerator.getContext(),
                                 llvm::APFloat(value));
  }

private:
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

  Value *codegen() { return curent_builder->CreateGlobalStringPtr(value); }

private:
  string value;
};

#endif // AST_HPP

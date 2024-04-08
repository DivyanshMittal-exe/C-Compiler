#ifndef AST_HPP
#define AST_HPP

#include "AST_enums.hpp"
#include "codegen.h"
#include "scoper.h"
#include <cstdint>
#include <iostream>
#include <llvm-14/llvm/IR/BasicBlock.h>
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

static llvm::Type *declaration_type = nullptr;
static vector<llvm::Type *> function_params;

static int parameter_list_index = 0;

static bool get_as_lvalue = false;

static bool global_is_variadic = false;

static bool am_i_initialising = false;

static llvm::BasicBlock *loop_block = nullptr;
static llvm::BasicBlock *merge_block = nullptr;

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

  vector<ASTNode *> getChildren() { return children; }

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

  // Got rid of the functions map here, and used the module insteads

  Value *codegen() {
    declaration_type = nullptr;
    function_params.clear();
    global_is_variadic = false;

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

    codeGenerator.getBuilder().CreateRet(
        llvm::Constant::getNullValue(func_ret_type));

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

public:
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
    llvm::BasicBlock *mergeBlock =
        llvm::BasicBlock::Create(codeGenerator.getContext(), "switchcont");

    codeGenerator.getBuilder().CreateBr(switchBlock);
    codeGenerator.getBuilder().SetInsertPoint(switchBlock);

    codeGenerator.pushContext();
    auto statements = statement->getChildren();

    int numCases = statements.size();
    ASTNode *defaultCase = nullptr;
    for (auto statement : statements) {
      if (statement->getNodeType() == NodeType::DefaultLabelStatement) {
        defaultCase = statement;
        numCases--;
      }
    }

    llvm::SwitchInst *switchInst = codeGenerator.getBuilder().CreateSwitch(
        conditionValue, defaultBlock, numCases);

    for (int i = 0; i < statements.size(); i++) {
      if (statements[i]->getNodeType() == NodeType::DefaultLabelStatement) {
        continue;
      }
      llvm::BasicBlock *caseBlock = llvm::BasicBlock::Create(
          codeGenerator.getContext(), "case" + std::to_string(i), function);
      codeGenerator.getBuilder().SetInsertPoint(caseBlock);
      statements[i]->codegen();
      codeGenerator.getBuilder().CreateBr(mergeBlock);

      CaseLabelStatementNode *caseNode =
          dynamic_cast<CaseLabelStatementNode *>(statements[i]);

      llvm::ConstantInt *caseValue = llvm::dyn_cast<llvm::ConstantInt>(
          caseNode->constant_expression->codegen());
      switchInst->addCase(caseValue, caseBlock);
    }

    // Generate LLVM IR code for the default case, if present

    if (defaultCase != nullptr) {
      function->getBasicBlockList().push_back(defaultBlock);
      codeGenerator.getBuilder().SetInsertPoint(defaultBlock);
      defaultCase->codegen();
      codeGenerator.getBuilder().CreateBr(mergeBlock);
    }

    codeGenerator.popContext();

    // Add the merge block to the function
    function->getBasicBlockList().push_back(mergeBlock);
    codeGenerator.getBuilder().SetInsertPoint(mergeBlock);

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

    // Create the loop condition branch instruction
    codeGenerator.getBuilder().CreateCondBr(conditionValue, loopBlock,
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
    expression1->codegen();
    codeGenerator.pushContext();

    // Jump to the loop condition block
    codeGenerator.getBuilder().CreateBr(loopConditionBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopConditionBlock);

    // Generate LLVM IR code for the loop condition check
    llvm::Value *conditionValue = expression2->codegen();

    // Create the loop condition branch instruction
    codeGenerator.getBuilder().CreateCondBr(conditionValue, loopBodyBlock,
                                            mergeBlock);

    // Set the insert point to the loop body block
    function->getBasicBlockList().push_back(loopBodyBlock);
    codeGenerator.getBuilder().SetInsertPoint(loopBodyBlock);

    // Generate LLVM IR code for the loop body

    llvm::BasicBlock *old_loop_block = loop_block;
    llvm::BasicBlock *old_merge_block = merge_block;
    loop_block = loopConditionBlock;
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
    expression3->codegen();

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

private:
  ASTNode *identifier;
};

class ContinueStatementNode : public ASTNode {
public:
  ContinueStatementNode() : ASTNode(NodeType::ContinueStatement) {}

  string dump_ast(int depth = 0) const {
    return dumpParameters(this, {}, depth, false);
  }

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

  Value *codegen() {
    if (loop_block == nullptr) {
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

  Value *codegen() {
    if (expression->getNodeType() != NodeType::Unimplemented) {

      llvm::Value *val_to_ret =
          codeGenerator.getBuilder().CreateRet(expression->codegen());

      cout << "Returning value from function" << endl;
      cout << "Value returned is " << val_to_ret->getType()->getTypeID()
           << endl;

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
    llvm::Type *old_type = declaration_type;
    declarator->modifyDeclarationType();
    llvm::Type *declaration_type_copy = declaration_type;
    declaration_type = old_type;
    string name = declarator->get().s;

    am_i_initialising = true;
    declarator->codegen();
    am_i_initialising = false;

    if (codeGenerator.isFunctionDefined(declarator->get().s)) {
      return nullptr;
    }

    llvm::Value *val = nullptr;
    if (initializer->getNodeType() != NodeType::Unimplemented) {
      val = initializer->codegen();
    } else {
      val = llvm::Constant::getNullValue(declaration_type_copy);
    }

    cout << "Name " << declarator->get().s << endl;

    llvm::AllocaInst *alloca = codeGenerator.getBuilder().CreateAlloca(
        declaration_type_copy, nullptr, declarator->get().s);
    codeGenerator.getBuilder().CreateStore(val, alloca);
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

  m_Value get() { return direct_declarator->get(); }

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

  string dump_ast(int depth = 0) const {
    string result = formatSpacing(depth) + "*\n";
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

  Value *codegen() {
    Value *array_size = assignment_expression->codegen();
    if (array_size->getType()->getTypeID() != llvm::Type::IntegerTyID) {
      array_size = codeGenerator.getBuilder().CreateIntCast(
          array_size, llvm::Type::getInt32Ty(codeGenerator.getContext()), true);
    }
    llvm::Type *element_type = declaration_type;

    llvm::ConstantInt *arraySizeInt =
        llvm::dyn_cast<llvm::ConstantInt>(array_size);
    uint64_t array_size_val = arraySizeInt->getZExtValue();

    llvm::Type *array_type = llvm::ArrayType::get(element_type, array_size_val);
    llvm::AllocaInst *alloca = codeGenerator.getBuilder().CreateAlloca(
        array_type, nullptr, direct_declarator->get().s);
    codeGenerator.getsymbolTable()[direct_declarator->get().s] = alloca;
    return nullptr;
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

  m_Value get() { return direct_declarator->get(); }

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

  m_Value get() { return m_Value(name); }

  bool check_semantics() { return scoperStack.exists(name); }

  void modifyDeclarationType() {}

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

  Value *codegen() override {
    // Generate LLVM IR for the right-hand side expression

    Value *rhsValue = assignment_expression->codegen();
    // Get the address of the left-hand side variable or expression

    get_as_lvalue = true;
    Value *lhsAddr = unary_expression->codegen();
    get_as_lvalue = false;

    // Perform the assignment based on the operator
    switch (assOp) {
    case AssignmentOperator::ASSIGN:
      // Store the value to the address
      codeGenerator.getBuilder().CreateStore(rhsValue, lhsAddr);
      break;
    /* case AssignmentOperator::MUL_ASSIGN: */
    /*     // Load the current value from the address, perform multiplication,
     * and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateMul(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::DIV_ASSIGN: */
    /*     // Perform division and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateSDiv(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::MOD_ASSIGN: */
    /*     // Perform modulus and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateSRem(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::ADD_ASSIGN: */
    /*     // Perform addition and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateAdd(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::SUB_ASSIGN: */
    /*     // Perform subtraction and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateSub(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::LEFT_ASSIGN: */
    /*     // Perform left shift and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateShl(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::RIGHT_ASSIGN: */
    /*     // Perform right shift and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateAShr(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::AND_ASSIGN: */
    /*     // Perform bitwise AND and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateAnd(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::XOR_ASSIGN: */
    /*     // Perform bitwise XOR and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateXor(codeGenerator.getBuilder().CreateLoad(lhsAddr),
     * rhsValue), lhsAddr); */
    /*     break; */
    /* case AssignmentOperator::OR_ASSIGN: */
    /*     // Perform bitwise OR and store the result */
    /*     codeGenerator.getBuilder().CreateStore( */
    /*         codeGenerator.getBuilder().CreateOr(codeGenerator.getBuilder().CreateLoad(lhsAddr),
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

  Value *codegen() {

    cout << "Array access node" << endl;

    Value *postFixValue = postfix_expression->codegen();

    bool prev_get_as_lvalue = get_as_lvalue;
    get_as_lvalue = false;
    Value *indexValue = expression->codegen();
    get_as_lvalue = prev_get_as_lvalue;

    if (postFixValue->getType()->getTypeID() != llvm::Type::PointerTyID) {
      throw std::runtime_error("Array access on non-pointer type");
    }

    llvm::Type *element_type = postFixValue->getType()->getPointerElementType();

    llvm::Value *val_to_ret = codeGenerator.getBuilder().CreateGEP(
        element_type, postFixValue, {indexValue});

    /* llvm::Value *zero = llvm::ConstantInt::get( */
    /*     llvm::Type::getInt32Ty(codeGenerator.getContext()), 0); */
    /* llvm::Value *indices[] = {zero, indexValue}; */
    /**/
    /* llvm::Value *val_to_ret = */
    /* codeGenerator.getBuilder().CreateGEP(element_type, postFixValue,
     * indices); */
    return codeGenerator.getBuilder().CreateLoad(element_type, val_to_ret);
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

  Value *codegen() {

    string function_name = postfix_expression->get().s;
    llvm::Function *function = codeGenerator.getFunction(function_name);

    if (function == nullptr) {
      throw std::runtime_error("Function " + function_name + " not found.");
    }

    vector<ASTNode *> args = argument_expression_list->getChildren();

    /* if (args.size() != function->arg_size()) { */
    /*   throw std::runtime_error("Function " + function_name + */
    /*                            " called with wrong number of arguments."); */
    /* } */

    vector<Value *> arguments;
    for (auto arg : args) {
      arguments.push_back(arg->codegen());
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
    result += "Prefix: " + unaryOperatorToString(un_op) + "{ \n";
    result += unary_expression->dump_ast(depth + 1);
    result += formatSpacing(depth) + "} \n";
    return result;
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
      Value *old_val = codeGenerator.getBuilder().CreateLoad(val_type, val);
      Value *new_val = codeGenerator.getBuilder().CreateAdd(
          old_val,
          llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true),
          "inc");
      codeGenerator.getBuilder().CreateStore(new_val, val);
      return new_val;
    }
    case UnaryOperator::DEC_OP: {
      llvm::Type *val_type = val->getType()->getPointerElementType();
      Value *old_val = codeGenerator.getBuilder().CreateLoad(val_type, val);
      Value *new_val = codeGenerator.getBuilder().CreateSub(
          old_val,
          llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true),
          "inc");
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
          val->getType()->getPointerElementType(), val);
    case UnaryOperator::PLUS:
      return val;
    case UnaryOperator::MINUS:
      return codeGenerator.getBuilder().CreateNeg(val);
    case UnaryOperator::BITWISE_NOT:
      return codeGenerator.getBuilder().CreateNot(val);
    case UnaryOperator::LOGICAL_NOT: {
      Value *cmp = codeGenerator.getBuilder().CreateICmpNE(
          val,
          llvm::ConstantInt::get(codeGenerator.getBuilder().getInt32Ty(), 0));
      Value *boolVal = codeGenerator.getBuilder().CreateZExt(
          cmp, codeGenerator.getBuilder().getInt1Ty());
      return codeGenerator.getBuilder().CreateNot(boolVal);
    }
    }

    return val;
  }

private:
  ASTNode *unary_expression;
  UnaryOperator un_op;
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
      Value *old_val = codeGenerator.getBuilder().CreateLoad(val_type, val);
      Value *new_val = codeGenerator.getBuilder().CreateAdd(
          old_val,
          llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true),
          "inc");
      codeGenerator.getBuilder().CreateStore(new_val, val);
      return old_val;
    }
    case UnaryOperator::DEC_OP: {

      llvm::Type *val_type = val->getType()->getPointerElementType();
      Value *old_val = codeGenerator.getBuilder().CreateLoad(val_type, val);
      Value *new_val = codeGenerator.getBuilder().CreateSub(
          old_val,
          llvm::ConstantInt::get(
              llvm::Type::getInt32Ty(codeGenerator.getContext()), 1, true),
          "inc");
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

  Value *codegen() {

    Value *conditionValue = logical_or_expression->codegen();
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
        codeGenerator.getBuilder().CreatePHI(thenValue->getType(), 2);
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
    return codeGenerator.getBuilder().CreateOr(lhs, rhs, "or");
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
    return codeGenerator.getBuilder().CreateAnd(lhs, rhs, "and");
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

  Value *codegen() {
    Value *lhs = exclusive_or_expression->codegen();
    Value *rhs = and_expression->codegen();
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

  Value *codegen() {
    Value *lhs = and_expression->codegen();
    Value *rhs = equality_expression->codegen();
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

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();
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

  bool check_semantics() {
    return equality_expression->check_semantics() &&
           relational_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = equality_expression->codegen();
    Value *rhs = relational_expression->codegen();
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
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

  bool check_semantics() {
    return relational_expression->check_semantics() &&
           shift_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = relational_expression->codegen();
    Value *rhs = shift_expression->codegen();
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

  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
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

  bool check_semantics() {
    return shift_expression->check_semantics() &&
           additive_expression->check_semantics();
  }
  Value *codegen() {
    Value *lhs = shift_expression->codegen();
    Value *rhs = additive_expression->codegen();
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

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }
  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();
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

  bool check_semantics() {
    return additive_expression->check_semantics() &&
           multiplicative_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = additive_expression->codegen();
    Value *rhs = multiplicative_expression->codegen();
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
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

  bool check_semantics() {
    return multiplicative_expression->check_semantics() &&
           cast_expression->check_semantics();
  }

  Value *codegen() {
    Value *lhs = multiplicative_expression->codegen();
    Value *rhs = cast_expression->codegen();
    return codeGenerator.getBuilder().CreateSRem(lhs, rhs, "mod");
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

private:
  string value;
};

#endif // AST_HP

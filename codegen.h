#ifndef INCLUDE_CC_CODEGEN_H_
#define INCLUDE_CC_CODEGEN_H_

#include "AST_enums.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <iostream>
#include <llvm-14/llvm/IR/Function.h>
#include <llvm-14/llvm/IR/Value.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

class m_Context {
public:
  bool is_global;
  map<string, llvm::Value *> variables;
  map<string, llvm::Value *> carry_over_variables;

  m_Context() { is_global = false; }
  m_Context(const m_Context &other) {
    this->variables = other.carry_over_variables;
    is_global = false;
  }
  m_Context(bool is_global) { this->is_global = is_global; }

  // Overload the [] operator to access the variables map
  llvm::Value *&operator[](string key) {

    if (!is_global && variables.find(key) != variables.end()) {
      std::runtime_error("Variable " + key + " has been previously defined");
    }
    return variables[key];
  }
  llvm::Value *operator[](string key) const {
    if (variables.find(key) == variables.end()) {
      std::runtime_error("Variable " + key + " has not been defined");
    }
    return variables.at(key);
  }

  void add_to_carryover(string key, llvm::Value *value) {
    if (carry_over_variables.find(key) != carry_over_variables.end()) {
      std::runtime_error(
          "Variable " + key +
          " has been previously defined in the function definition");
    }
    carry_over_variables[key] = value;
  }
};

class CodeGenerator {
public:
  vector<unique_ptr<llvm::LLVMContext>> contexts;

  unique_ptr<llvm::IRBuilder<>> builder;

  unique_ptr<llvm::Module> global_module;

  vector<unique_ptr<m_Context>> symbol_tables;

  map<string, llvm::Function *> declared_functions;

  CodeGenerator() {
    unique_ptr<llvm::LLVMContext> global_context =
        make_unique<llvm::LLVMContext>();
    builder = make_unique<llvm::IRBuilder<>>(*global_context);
    global_module = make_unique<llvm::Module>("global_module", *global_context);
    contexts.push_back(std::move(global_context));
    symbol_tables.push_back(make_unique<m_Context>(m_Context(true)));
  }

  llvm::IRBuilder<> &getBuilder() { return *builder; }

  bool isFunctionDeclaredButNotDefined(string name) {
    return declared_functions.find(name) != declared_functions.end();
  }

  bool isFunctionDefined(string name) {
    return global_module->getFunction(name) != nullptr;
  }

  llvm::Function *getFunction(string name) {
    return global_module->getFunction(name);
  }

  llvm::LLVMContext &getContext() { return *contexts.back(); }

  void pushContext() {
    m_Context &current_symbol_table = *symbol_tables.back();
    unique_ptr<m_Context> new_symbol_table =
        make_unique<m_Context>(current_symbol_table);
    symbol_tables.push_back(std::move(new_symbol_table));
  }

  void popContext() { symbol_tables.pop_back(); }

  m_Context &getsymbolTable() { return *symbol_tables.back(); }

  llvm::Value *findSymbol(string name) {

    for (auto it = symbol_tables.rbegin(); it != symbol_tables.rend(); ++it) {
      auto &x = *it; // Dereference the reverse iterator to get the element

      // Check if the variable exists in the current symbol_table
      if (x->variables.find(name) != x->variables.end()) {
        return x->variables[name]; // Return the value if found
      }
    }

    throw std::runtime_error("Variable " + name + " not found");
  }
};

static llvm::Type *getCurrType(SpecifierEnum specifier,
                               llvm::LLVMContext &llvmContext) {

  llvm::Type *curr_type;
  switch (specifier) {
  case SpecifierEnum::VOID:
    curr_type = llvm::Type::getVoidTy(llvmContext);
    break;
  case SpecifierEnum::CHAR:
    curr_type = llvm::Type::getInt8Ty(llvmContext);
    break;
  case SpecifierEnum::SHORT:
    curr_type = llvm::Type::getInt16Ty(llvmContext);
    break;
  case SpecifierEnum::INT:
    curr_type = llvm::Type::getInt32Ty(llvmContext);
    break;
  case SpecifierEnum::LONG:
    curr_type = llvm::Type::getInt64Ty(llvmContext);
    break;
  case SpecifierEnum::FLOAT:
    curr_type = llvm::Type::getFloatTy(llvmContext);
    break;
  case SpecifierEnum::DOUBLE:
    curr_type = llvm::Type::getDoubleTy(llvmContext);
    break;
  case SpecifierEnum::BOOL:
    curr_type = llvm::Type::getInt1Ty(llvmContext);
    break;

  default:
    curr_type = nullptr;
  }

  return curr_type;
}

#endif // INCLUDE_CC_CODEGEN_H_

#ifndef INCLUDE_CC_CODEGEN_H_
#define INCLUDE_CC_CODEGEN_H_

#include "AST_enums.hpp"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <iostream>
#include <llvm-14/llvm/IR/Value.h>
#include <map>
#include <memory>
#include <string>
using namespace std;

class CodeGenerator {
public:
  vector<unique_ptr<llvm::LLVMContext>> contexts;
  vector<unique_ptr<llvm::IRBuilder<>>> builders;

  unique_ptr<llvm::Module> global_module;

  map<string, llvm::Value *> functions;

  vector<map<string, llvm::Value *>> symbol_tables;
  vector<map<string, llvm::Value *>> func_args;

  CodeGenerator() {
    unique_ptr<llvm::LLVMContext> global_context =
        make_unique<llvm::LLVMContext>();
    unique_ptr<llvm::IRBuilder<>> global_builder =
        make_unique<llvm::IRBuilder<>>(*global_context);
    global_module = make_unique<llvm::Module>("global_module", *global_context);
    contexts.push_back(std::move(global_context));
    builders.push_back(std::move(global_builder));
  }

  bool isFunctionDefined(string name) {
    return functions.find(name) != functions.end();
  }

  llvm::LLVMContext &getContext() { return *contexts.back(); }

  void push_func_args(map<string, llvm::Value *> symbol_table) {
    func_args.push_back(symbol_table);
  }

  void pop_func_args() { func_args.pop_back(); }

  void push_symbol_table() {

    map<string, llvm::Value *> new_table;
    symbol_tables.push_back(new_table);
  }

  void pop_symbol_table() { symbol_tables.pop_back(); }
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
    std::cout << "Error: Unknown type specifier" << std::endl;
  }

  return curr_type;
}

#endif // INCLUDE_CC_CODEGEN_H_

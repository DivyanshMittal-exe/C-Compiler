#include "AST.hpp"
#include "c.tab.hpp"
#include "scoper.h"
#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

extern "C" ASTNode *root;

static void usage() {
  std::cout << "Usage: cc <prog.c>" << std::endl;
  std::cout << "Additional arguments:" << std::endl;
  std::cout << "-o <filename>: Specify output file" << std::endl;
  std::cout << "--no-optimise: Disable optimization" << std::endl;

  std::cout << "--dump-ast: Dump abstract syntax tree" << std::endl;
  std::cout << "--semantics: Explicit Checking for semantics, this is merged with codegen step and should not be used" << std::endl;
}

int main(int argc, char **argv) {

  bool unknownArgument = false;
  bool hasProgC = false;
  bool skipSemantics = true;

  std::string out_filename = "a.ll";
  bool optimise = true;
  bool dump_ast = false;
  std::string prog_filename;

  cout << "argc = " << argc << endl;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-o" && i + 1 < argc) {
      out_filename = argv[i + 1];
      i++;
    } else if (arg == "--no-optimise") { // Check for --optimise flag
      optimise = false;
    } else if (arg == "--dump-ast") { // Check for --dump-ast flag
      dump_ast = true;
    } else if (arg == "--semantics") {
      skipSemantics = false;
    } else if (!hasProgC) { // Check for <prog.c>
      hasProgC = true;
      prog_filename = arg;
    } else {
      unknownArgument = true;
    }
  }

  if (!hasProgC) {
    std::cout << "Error: Missing source file <prog.c>" << std::endl;
    usage();
    return 1;
  }

  if (unknownArgument) {
    std::cout << "Error: Unknown Argument Specified" << std::endl;
    usage();
    exit(1);
  }
  cout << "prog_filename = " << prog_filename << endl;
  yyin = fopen(prog_filename.c_str(), "r");
  assert(yyin);
  int ret = yyparse();

  printf("retv = %d\n", ret);

  if (ret != 0) {
    printf("Parsing failed\n");
    exit(1);
  }

  printf("Parsing successful\n");

  string ast_dump = root->dump_ast();
  if (dump_ast) {
    printf("AST:\n\n");
    std::cout << ast_dump << std::endl;
  }

  if (skipSemantics) {
    printf("Skipping semantics\n");

  } else {
    printf("Checking semantics\n");
    bool scoping = root->check_semantics();
    if (!scoping) {
      printf("Scoping Failed\n");
      exit(1);
    }
  }

  if (optimise) {
    ASTNode *old = new NullPtrNode();
    string old_dump = old->dump_ast();
    int i = 0;
    while (old_dump != root->dump_ast()) {
      old_dump = root->dump_ast();
      root = root->optimise();
      if (i > 100) {
        break;
      }
      i++;
    }
  }

  if (dump_ast) {
    printf("AST->After optimise:\n\n");
    std::cout << root->dump_ast() << std::endl;
  }

  root->codegen();
  root->dump_llvm(out_filename);
  exit(0);
}

// TODO:
//  Implement ~

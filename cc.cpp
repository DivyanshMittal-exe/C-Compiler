#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "c.tab.hpp"
#include "AST.hpp"

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

extern "C" ASTNode* root;

static void usage()
{
  printf("Usage: cc <prog.c>\n");
}

int
main(int argc, char **argv)
{
  if (argc != 2) {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);
  int ret = yyparse();

  printf("retv = %d\n", ret);

    if (ret == 0) {
        printf("Parsing successful\n");
        printf("AST:\n");
        printf("%s\n", root->dump_ast().c_str());
    } else {
        printf("Parsing failed\n");
    }



  exit(0);
}

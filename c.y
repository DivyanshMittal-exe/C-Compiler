%code requires{
	#include "AST.hpp"
}


%{
#include <cstdio>
#include <iostream>
#include "AST.hpp"
using namespace std;

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

void yyerror(const char *s);

ASTNode* root = NULL;
%}

%union {
	ASTNode* base_node;
	char* str;
	int integer;
	float real;
	UnaryOperator un_op;
	AssignmentOperator ass_op;
}

%token <str> IDENTIFIER STRING_LITERAL
%token <integer> I_CONSTANT
%token <real> F_CONSTANT
%token  FUNC_NAME SIZEOF
%token	PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME ENUMERATION_CONSTANT

%token	TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token	CONST RESTRICT VOLATILE
%token	BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token	COMPLEX IMAGINARY
%token	STRUCT UNION ENUM ELLIPSIS

%token	CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL


%type <base_node> translation_unit external_declaration function_definition declaration_list
%type <base_node> declaration_specifiers type_qualifier function_specifier alignment_specifier declaration declarator storage_class_specifier type_specifier struct_or_union_specifier enum_specifier

%type <base_node> init_declarator init_declarator_list initializer statement labeled_statement compound_statement block_item_list block_item expression_statement selection_statement iteration_statement jump_statement
%type <base_node> direct_declarator
%type <base_node> constant_expression expression
%type <base_node> primary_expression postfix_expression argument_expression_list unary_expression  cast_expression multiplicative_expression additive_expression shift_expression relational_expression equality_expression and_expression exclusive_or_expression inclusive_or_expression logical_and_expression logical_or_expression conditional_expression assignment_expression
%type <base_node> constant string identifier_list parameter_type_list parameter_list parameter_declaration abstract_declarator
%type <base_node> pointer initializer_list

%type <un_op> unary_operator
%type <ass_op> assignment_operator


%start translation_unit
%%


primary_expression
	: IDENTIFIER { $$ = new IdentifierNode($1); }
	| constant { $$ = $1; }
	| string { $$ = $1; }
	| '(' expression ')' { $$ = $2; }
	| generic_selection { $$ = new NullPtrNode(); }
	;

constant
	: I_CONSTANT  {$$ = new	IConstantNode($1);}
	| F_CONSTANT  {$$ = new	FConstantNode($1);}
	| ENUMERATION_CONSTANT {$$ = new NullPtrNode(); }
	;

enumeration_constant		/* before it has been defined as such */
	: IDENTIFIER
	;

string
	: STRING_LITERAL { $$ = new StringNode($1); }
	| FUNC_NAME { $$ = new NullPtrNode(); }
	;

generic_selection
	: GENERIC '(' assignment_expression ',' generic_assoc_list ')'
	;

generic_assoc_list
	: generic_association
	| generic_assoc_list ',' generic_association
	;

generic_association
	: type_name ':' assignment_expression
	| DEFAULT ':' assignment_expression
	;

postfix_expression
	: primary_expression { $$ = $1; }
	| postfix_expression '[' expression ']' {$$ = new ArrayAccessNode($1, $3);}
	| postfix_expression '(' ')' {$$ = new FunctionCallNode($1, new NullPtrNode());}
	| postfix_expression '(' argument_expression_list ')' {$$ = new FunctionCallNode($1, $3);}
	| postfix_expression '.' IDENTIFIER {$$ = new MemberAccessNode($1, new IdentifierNode($3));}
	| postfix_expression PTR_OP IDENTIFIER {$$ = new MemberAccessNode($1, new IdentifierNode($3));}
	| postfix_expression INC_OP {$$ = new PostfixExpressionNode($1, UnaryOperator::INC_OP);}
	| postfix_expression DEC_OP {$$ = new PostfixExpressionNode($1, UnaryOperator::DEC_OP);}
	| '(' type_name ')' '{' initializer_list '}' { $$ = $5; }
	| '(' type_name ')' '{' initializer_list ',' '}' { $$ = $5; }
	;



argument_expression_list
	: assignment_expression {$$  = new ArgumentExpressionListNode(); $$->addChild($1);}
	| argument_expression_list ',' assignment_expression {$$ = $1; $1->addChild($3);}
	;

unary_expression
	: postfix_expression { $$ = $1; }
	| INC_OP unary_expression { $$ = new UnaryExpressionNode(UnaryOperator::INC_OP,$2); }
	| DEC_OP unary_expression { $$ = new UnaryExpressionNode(UnaryOperator::DEC_OP,$2); }
	| unary_operator cast_expression { $$ = new UnaryExpressionNode($1,$2); }
	| SIZEOF unary_expression { $$ = new UnaryExpressionNode(UnaryOperator::SIZEOF,$2); }
	| SIZEOF '(' type_name ')' { $$ = new NullPtrNode(); }
	| ALIGNOF '(' type_name ')' { $$ = new NullPtrNode(); }
	;

unary_operator
	: '&' {$$ = UnaryOperator::ADDRESS_OF; }
	| '*' {$$ = UnaryOperator::MUL_OP; }
	| '+' {$$ = UnaryOperator::PLUS; }
	| '-' {$$ = UnaryOperator::MINUS; }
	| '~' {$$ = UnaryOperator::BITWISE_NOT; }
	| '!' {$$ = UnaryOperator::LOGICAL_NOT; }
	;

cast_expression
	: unary_expression { $$ = $1; }
	| '(' type_name ')' cast_expression { $$ = new NullPtrNode(); }
	;

multiplicative_expression
	: cast_expression {$$ = $1; }
	| multiplicative_expression '*' cast_expression { $$ = new MultiplicativeExpressionNode($1, $3); }
	| multiplicative_expression '/' cast_expression {$$ = new DivExpressionNode($1, $3); }
	| multiplicative_expression '%' cast_expression {$$ = new ModExpressionNode($1, $3); }
	;

additive_expression
	: multiplicative_expression {$$ = $1; }
	| additive_expression '+' multiplicative_expression { $$ = new AdditiveExpressionNode($1, $3); }
	| additive_expression '-' multiplicative_expression {$$ = new SubExpressionNode($1, $3); }
	;

shift_expression
	: additive_expression { $$ = $1; }
	| shift_expression LEFT_OP additive_expression { $$ = new LeftShiftExpressionNode($1, $3); }
	| shift_expression RIGHT_OP additive_expression { $$ = new RightShiftExpressionNode($1, $3); }
	;

relational_expression
	: shift_expression { $$ = $1; }
	| relational_expression '<' shift_expression { $$ = new LessThanExpressionNode($1, $3); }
	| relational_expression '>' shift_expression { $$ = new GreaterThanExpressionNode($1, $3); }
	| relational_expression LE_OP shift_expression { $$ = new LessOrEqualExpressionNode($1, $3); }
	| relational_expression GE_OP shift_expression { $$ = new GreaterOrEqualExpressionNode($1, $3); }
	;

equality_expression
	: relational_expression { $$ = $1; }
	| equality_expression EQ_OP relational_expression { $$ = new EqualityExpressionNode($1, $3); }
	| equality_expression NE_OP relational_expression { $$ = new NonEqualityExpressionNode($1, $3); }
	;

and_expression
	: equality_expression { $$ = $1; }
	| and_expression '&' equality_expression { $$ = new AndExpressionNode($1, $3); }
	;

exclusive_or_expression
	: and_expression { $$ = $1; }
	| exclusive_or_expression '^' and_expression { $$ = new ExclusiveOrExpressionNode($1, $3); }
	;

inclusive_or_expression
	: exclusive_or_expression { $$ = $1; }
	| inclusive_or_expression '|' exclusive_or_expression { $$ = new InclusiveOrExpressionNode($1, $3); }
	;

logical_and_expression
	: inclusive_or_expression { $$ = $1; }
	| logical_and_expression AND_OP inclusive_or_expression { $$ = new ConditionalExpressionNode($1, $3, new IConstantNode(0) ); }
	;

logical_or_expression
	: logical_and_expression { $$ = $1; }
	| logical_or_expression OR_OP logical_and_expression { $$ = new ConditionalExpressionNode($1, new IConstantNode(1), $3); }
	;

conditional_expression
	: logical_or_expression { $$ = $1; }
	| logical_or_expression '?' expression ':' conditional_expression { $$ = new ConditionalExpressionNode($1, $3, $5); }
	;

assignment_expression
	: conditional_expression { $$ = $1; }
	| unary_expression assignment_operator assignment_expression { $$ = new AssignmentExpressionNode($1, $2, $3); }
	;

assignment_operator
    : '='             { $$ = AssignmentOperator::ASSIGN; }
    | MUL_ASSIGN      { $$ = AssignmentOperator::MUL_ASSIGN; }
    | DIV_ASSIGN      { $$ = AssignmentOperator::DIV_ASSIGN; }
    | MOD_ASSIGN      { $$ = AssignmentOperator::MOD_ASSIGN; }
    | ADD_ASSIGN      { $$ = AssignmentOperator::ADD_ASSIGN; }
    | SUB_ASSIGN      { $$ = AssignmentOperator::SUB_ASSIGN; }
    | LEFT_ASSIGN     { $$ = AssignmentOperator::LEFT_ASSIGN; }
    | RIGHT_ASSIGN    { $$ = AssignmentOperator::RIGHT_ASSIGN; }
    | AND_ASSIGN      { $$ = AssignmentOperator::AND_ASSIGN; }
    | XOR_ASSIGN      { $$ = AssignmentOperator::XOR_ASSIGN; }
    | OR_ASSIGN       { $$ = AssignmentOperator::OR_ASSIGN; }
    ;

expression
	: assignment_expression { $$ = new ExpressionListNode(); $$->addChild($1); }
	| expression ',' assignment_expression {$$ = $1; $1->addChild($3);}
	;

constant_expression
	: conditional_expression {$$ = $1;} /* with constraints */
	;

declaration
	: declaration_specifiers ';' { $$ = new DeclarationNode($1, new NullPtrNode());}
	| declaration_specifiers init_declarator_list ';' { $$ = new DeclarationNode($1, $2) ;}
	| static_assert_declaration { $$ = new NullPtrNode(); }
	;

declaration_specifiers
	: storage_class_specifier declaration_specifiers {$$ = $2; $2->addChild($1);}
	| storage_class_specifier {$$ = new DeclarationSpecifiersNode(); $$->addChild($1);}
	| type_specifier declaration_specifiers {$$ = $2; $2->addChild($1);}
	| type_specifier {$$ = new DeclarationSpecifiersNode(); $$->addChild($1);}
	| type_qualifier declaration_specifiers {$$ = $2; $2->addChild($1);}
	| type_qualifier {$$ = new DeclarationSpecifiersNode(); $$->addChild($1);}
	| function_specifier declaration_specifiers {$$ = $2; $2->addChild($1);}
	| function_specifier {$$ = new DeclarationSpecifiersNode(); $$->addChild($1);}
	| alignment_specifier declaration_specifiers {$$ = $2; $2->addChild($1);}
	| alignment_specifier {$$ = new DeclarationSpecifiersNode(); $$->addChild($1);}
	;

init_declarator_list
	: init_declarator { $$ = new InitDeclartorListNode(); $$->addChild($1); }
	| init_declarator_list ',' init_declarator {$$ = $1; $1->addChild($3);}
	;


init_declarator
	: declarator '=' initializer { $$ = new InitDeclartorNode($1,$3); }
	| declarator { $$ = new InitDeclartorNode($1, new NullPtrNode()); }
	;


storage_class_specifier
    : TYPEDEF { $$ = new SpecifierNode(SpecifierEnum::TYPEDEF); }
    | EXTERN  { $$ = new SpecifierNode(SpecifierEnum::EXTERN); }
    | STATIC  { $$ = new SpecifierNode(SpecifierEnum::STATIC); }
    | THREAD_LOCAL  { $$ = new SpecifierNode(SpecifierEnum::THREAD_LOCAL); }
    | AUTO    { $$ = new SpecifierNode(SpecifierEnum::AUTO); }
    | REGISTER { $$ = new SpecifierNode(SpecifierEnum::REGISTER); }
    ;

type_specifier
    : VOID    { $$ = new SpecifierNode(SpecifierEnum::VOID); }
    | CHAR    { $$ = new SpecifierNode(SpecifierEnum::CHAR); }
    | SHORT   { $$ = new SpecifierNode(SpecifierEnum::SHORT); }
    | INT     { $$ = new SpecifierNode(SpecifierEnum::INT); }
    | LONG    { $$ = new SpecifierNode(SpecifierEnum::LONG); }
    | FLOAT   { $$ = new SpecifierNode(SpecifierEnum::FLOAT); }
    | DOUBLE  { $$ = new SpecifierNode(SpecifierEnum::DOUBLE); }
    | SIGNED  { $$ = new SpecifierNode(SpecifierEnum::SIGNED); }
    | UNSIGNED { $$ = new SpecifierNode(SpecifierEnum::UNSIGNED); }
    | BOOL    { $$ = new SpecifierNode(SpecifierEnum::BOOL); }
    | COMPLEX { $$ = new SpecifierNode(SpecifierEnum::COMPLEX); }
    | IMAGINARY { $$ = new SpecifierNode(SpecifierEnum::IMAGINARY); }
    | atomic_type_specifier {$$ = new NullPtrNode();}
    | struct_or_union_specifier {$$ = new NullPtrNode();}
    | enum_specifier {$$ = new NullPtrNode();}
    | TYPEDEF_NAME { $$ = new SpecifierNode(SpecifierEnum::TYPEDEF_NAME); }
    ;

struct_or_union_specifier
	: struct_or_union '{' struct_declaration_list '}' { $$ = new NullPtrNode(); }
	| struct_or_union IDENTIFIER '{' struct_declaration_list '}' { $$ = new NullPtrNode(); }
	| struct_or_union IDENTIFIER { $$ = new NullPtrNode(); }
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list ';'	/* for anonymous struct/union */
	| specifier_qualifier_list struct_declarator_list ';'
	| static_assert_declaration
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
	| type_specifier
	| type_qualifier specifier_qualifier_list
	| type_qualifier
	;

struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: ':' constant_expression
	| declarator ':' constant_expression
	| declarator
	;

enum_specifier
	: ENUM '{' enumerator_list '}' { $$ = new NullPtrNode(); }
	| ENUM '{' enumerator_list ',' '}' { $$ = new NullPtrNode(); }
	| ENUM IDENTIFIER '{' enumerator_list '}' { $$ = new NullPtrNode(); }
	| ENUM IDENTIFIER '{' enumerator_list ',' '}' { $$ = new NullPtrNode(); }
	| ENUM IDENTIFIER { $$ = new NullPtrNode(); }
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator	/* identifiers must be flagged as ENUMERATION_CONSTANT */
	: enumeration_constant '=' constant_expression
	| enumeration_constant
	;

atomic_type_specifier
	: ATOMIC '(' type_name ')'
	;

type_qualifier
	: CONST { $$ = new SpecifierNode(SpecifierEnum::CONST); }
	| RESTRICT { $$ = new SpecifierNode(SpecifierEnum::RESTRICT); }
	| VOLATILE { $$ = new SpecifierNode(SpecifierEnum::VOLATILE); }
	| ATOMIC { $$ = new SpecifierNode(SpecifierEnum::ATOMIC); }
	;

function_specifier
	: INLINE { $$ = new SpecifierNode(SpecifierEnum::INLINE); }
	| NORETURN { $$ = new SpecifierNode(SpecifierEnum::NORETURN); }
	;

alignment_specifier
	: ALIGNAS '(' type_name ')' { $$ = new NullPtrNode(); }
	| ALIGNAS '(' constant_expression ')' { $$ = new NullPtrNode(); }
	;

declarator
	: pointer direct_declarator { $$ = new DeclaratorNode($1, $2); }
	| direct_declarator { $$ = $1;}
	;

direct_declarator
	: IDENTIFIER {$$ = new IdentifierNode($1);}
	| '(' declarator ')' { $$ = $2; }
	| direct_declarator '[' ']' {$$ = new ArrayDeclaratorNode($1, new NullPtrNode());}
	| direct_declarator '[' '*' ']' {$$ = new ArrayDeclaratorNode($1, new NullPtrNode());} 
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' STATIC assignment_expression ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' type_qualifier_list '*' ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' type_qualifier_list assignment_expression ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' type_qualifier_list ']' {$$ = new NullPtrNode();}
	| direct_declarator '[' assignment_expression ']' {$$ = new ArrayDeclaratorNode($1, $3);}
	| direct_declarator '(' parameter_type_list ')' {$$ = new FunctionDeclarationNode($1, $3 ) ;}
	| direct_declarator '(' ')' {$$ = new FunctionDeclarationNode($1, new NullPtrNode()) ;}
	| direct_declarator '(' identifier_list ')'  {$$ = new NullPtrNode();}
	;

pointer
	: '*' type_qualifier_list pointer {$$ = new PointerNode( $3); }
	| '*' type_qualifier_list {$$ = new PointerNode(new NullPtrNode()); }
	| '*' pointer { $$ = new PointerNode($2); }
	| '*' {$$ = new PointerNode(new NullPtrNode()); }
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list ',' ELLIPSIS {$$ = $1; $1->variadic = true;}
	| parameter_list {$$ = $1;}
	;

parameter_list
	: parameter_declaration { $$ = new ParameterListNode(); $$->addChild($1); }
	| parameter_list ',' parameter_declaration { $$ = $1; $1->addChild($3); }
	;

parameter_declaration
	: declaration_specifiers declarator { $$ = new ParameterDeclarationNode($1, $2); }
	| declaration_specifiers abstract_declarator { $$ = new ParameterDeclarationNode($1, $2); }
	| declaration_specifiers { $$ = new ParameterDeclarationNode($1, new NullPtrNode()); }
	;

identifier_list
	: IDENTIFIER { $$ = new IdentifierListNode(); $$->addChild(new IdentifierNode($1));}
	| identifier_list ',' IDENTIFIER { $$ = $1; $1->addChild(new IdentifierNode($3)); }
	;

type_name
	: specifier_qualifier_list abstract_declarator
	| specifier_qualifier_list
	;

abstract_declarator
	: pointer direct_abstract_declarator { $$ = new NullPtrNode(); }
	| pointer { $$ = new NullPtrNode(); }
	| direct_abstract_declarator { $$ = new NullPtrNode(); }
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' '*' ']'
	| '[' STATIC type_qualifier_list assignment_expression ']'
	| '[' STATIC assignment_expression ']'
	| '[' type_qualifier_list STATIC assignment_expression ']'
	| '[' type_qualifier_list assignment_expression ']'
	| '[' type_qualifier_list ']'
	| '[' assignment_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' '*' ']'
	| direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list ']'
	| direct_abstract_declarator '[' assignment_expression ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: '{' initializer_list '}' { $$ = $2; }
	| '{' initializer_list ',' '}' { $$ = $2; }
	| assignment_expression { $$ = $1; }
	;

initializer_list
	: designation initializer {$$ = new NullPtrNode();}
	| initializer {$$ = new InitializerListNode(); $$->addChild($1);}
	| initializer_list ',' designation initializer
	| initializer_list ',' initializer {$$ = $1; $$->addChild($3);}
	;

designation
	: designator_list '='
	;

designator_list
	: designator
	| designator_list designator
	;

designator
	: '[' constant_expression ']'
	| '.' IDENTIFIER
	;

static_assert_declaration
	: STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' ';'
	;

statement
	: labeled_statement { $$ = $1; }
	| compound_statement { $$ = $1; }
	| expression_statement { $$ = $1; }
	| selection_statement { $$ = $1; }
	| iteration_statement { $$ = $1; }
	| jump_statement { $$ = $1; }
	;

labeled_statement
	: IDENTIFIER ':' statement { $$ = new LabelStatementNode(new IdentifierNode($1), $3); }
	| CASE constant_expression ':' statement {$$ = new CaseLabelStatementNode($2, $4);}
	| DEFAULT ':' statement {$$ = new DefaultLabelStatementNode($3);}
	;

compound_statement
	: '{' '}' {$$ = new CompoundStatementNode();}
	| '{'  block_item_list '}' {$$ = $2;}
	;

block_item_list
	: block_item {$$ = new CompoundStatementNode(); $$->addChild($1);}
	| block_item_list block_item {$$ = $1; $1->addChild($2);}
	;

block_item
	: declaration {$$ = $1;}
	| statement {$$ = $1;}
	;

expression_statement
	: ';' {$$ = new NullPtrNode();}
	| expression ';' {$$ = $1;}
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement { $$ = new IfElseStatementNode($3, $5, $7); }
	| IF '(' expression ')' statement { $$ = new IfElseStatementNode($3, $5, new NullPtrNode()); }
	| SWITCH '(' expression ')' statement { $$ = new SwitchStatementNode($3, $5); }
	;

iteration_statement
	: WHILE '(' expression ')' statement { $$ = new WhileStatementNode($3, $5); }
	| DO statement WHILE '(' expression ')' ';' { $$ = new DoWhileStatementNode($5, $2); }
	| FOR '(' expression_statement expression_statement ')' statement { $$ = new ForStatementNode($3, $4, new NullPtrNode(), $6); }
	| FOR '(' expression_statement expression_statement expression ')' statement { $$ = new ForStatementNode($3, $4, $5, $7); }
	| FOR '(' declaration expression_statement ')' statement { $$ = new ForStatementNode($3, $4, new NullPtrNode(), $6); }
	| FOR '(' declaration expression_statement expression ')' statement { $$ = new ForStatementNode($3, $4, $5, $7); }
	;

jump_statement
	: GOTO IDENTIFIER ';' { $$ = new GotoStatementNode(new IdentifierNode($2)); }
	| CONTINUE ';' { $$ = new ContinueStatementNode(); }
	| BREAK ';' { $$ = new BreakStatementNode(); }
	| RETURN ';' { $$ = new ReturnStatementNode(new NullPtrNode()); }
	| RETURN expression ';' { $$ = new ReturnStatementNode($2); }
	;

translation_unit
	: external_declaration {$$ = new TranslationUnitNode() ;root = $$; root->addChild($1);}
	| translation_unit external_declaration {$$ = $1; $1->addChild($2);}
	;

external_declaration
	: function_definition {$$ = $1;}
	| declaration {$$ = $1;}
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement {$$ = new FunctionDefinitionNode($1,$2,$3,$4); }
	| declaration_specifiers declarator compound_statement {$$ = new FunctionDefinitionNode($1,$2,new NullPtrNode(),$3); }
	;

declaration_list
	: declaration {$$ = new DeclarationListNode(); $$->addChild($1);}
	| declaration_list declaration {$$ = $1; $1->addChild($2);}
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "*** %s\n", s);
}

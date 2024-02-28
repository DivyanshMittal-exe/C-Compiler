#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>

using namespace std;

class ASTNode;


enum class NodeType {
    TranslationUnit,
    FunctionDefinition,
    Expression,
    DeclarationSpecifiers,
    Specifier
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
    ASSIGN,         // '='
    MUL_ASSIGN,     // '*='
    DIV_ASSIGN,     // '/='
    MOD_ASSIGN,     // '%='
    ADD_ASSIGN,     // '+='
    SUB_ASSIGN,     // '-='
    LEFT_ASSIGN,    // '<<='
    RIGHT_ASSIGN,   // '>>='
    AND_ASSIGN,     // '&='
    XOR_ASSIGN,     // '^='
    OR_ASSIGN       // '|='
};


enum class UnaryOperator {
    INC_OP,         // '++'
    DEC_OP,         // '--'
    SIZEOF,         // 'sizeof'
    ALIGNOF,        // '_Alignof'
    ADDRESS_OF,         // '&'
    MUL_OP,           // '*'
    PLUS,           // '+'
    MINUS,          // '-'
    BITWISE_NOT,          // '~'
    LOGICAL_NOT             // '!'

};


class ASTNode {
public:
    ASTNode(NodeType type) : type(type) {}

    virtual ~ASTNode() {}

    NodeType getNodeType() const { return type; }

    static string formatSpacing(int depth) {
        string result = "";
        for (int i = 0; i < depth; i++) {
            result += "  ";
        }
        return result;
    }

    virtual string dump_ast(int depth = 0) const {
        return "BaseNode";
        // Implement this method to dump the AST as a string
    }

    void addChild(ASTNode *child) {
        children.push_back(child);
    }

    vector<ASTNode *> children;

private:
    NodeType type;


};


class TranslationUnitNode : public ASTNode {
public:
    TranslationUnitNode()
            : ASTNode(NodeType::TranslationUnit) {

    }


    string dump_ast(int depth = 0) const {
        string result = "";
        for (auto child: children) {
            result += child->dump_ast(depth);
            result += '\n';
        }
        return result;
    }


    // Add member variables to hold information about translation units

};


// Node class for function definitions
class FunctionDefinitionNode : public ASTNode {
public:
    FunctionDefinitionNode()
            : ASTNode(NodeType::FunctionDefinition) {

    }

    FunctionDefinitionNode(ASTNode *declaration_specifiers, ASTNode *declarator, ASTNode *declaration_list,
                           ASTNode *compound_statement)
            : ASTNode(NodeType::FunctionDefinition), declaration_specifiers(declaration_specifiers),
              declarator(declarator),
              declaration_list(declaration_list), compound_statement(compound_statement) {
    }


    string dump_ast(int depth = 0) const {
        string result = "FunctionDefinition {";

        result += declaration_specifiers->dump_ast(depth + 1);
        result += declarator->dump_ast(depth + 1);
        result += declaration_list->dump_ast(depth + 1);
        result += compound_statement->dump_ast(depth + 1);
        result += formatSpacing(depth);
        result += "}\n";

        return result;
    }


private:
    ASTNode *declaration_specifiers;
    ASTNode *declarator;
    ASTNode *declaration_list;
    ASTNode *compound_statement;

};

// Node class for expressions
class ExpressionNode : public ASTNode {
public:
    ExpressionNode()
            : ASTNode(NodeType::Expression) {
    }

private:

};


class DeclarationSpecifiersNode : public ASTNode {
public:
    DeclarationSpecifiersNode()
            : ASTNode(NodeType::DeclarationSpecifiers) {

    }


};


class SpecifierNode : public ASTNode {
public:
    SpecifierNode(SpecifierEnum specifier)
            : ASTNode(NodeType::Specifier) , specifier(specifier){
    }


    SpecifierEnum specifier;
};

class CompoundStatementNode : public ASTNode {

public:
    CompoundStatementNode()
            : ASTNode(NodeType::Expression) {

    }


    string dump_ast(int depth = 0) const {
        string result = "CompoundStatement";
        return result;
    }


};


class LabelStatementNode : public ASTNode {
public:
    LabelStatementNode(ASTNode *label, ASTNode *statement)
            : ASTNode(NodeType::Expression), label(label), statement(statement){

    }

    string dump_ast(int depth = 0) const {
        string result = "LabelStatement";
        return result;
    }

private:
    ASTNode *label;
    ASTNode *statement;

};


class CaseLabelStatementNode : public ASTNode {

public:
    CaseLabelStatementNode(ASTNode *constant_expression, ASTNode *statement)
            : ASTNode(NodeType::Expression) , constant_expression(constant_expression), statement(statement){
    }

    string dump_ast(int depth = 0) const {
        string result = "CaseLabelStatement";
        return result;
    }

private:
    ASTNode *constant_expression;
    ASTNode *statement;


};

class DefaultLabelStatementNode : public ASTNode {
public:
    DefaultLabelStatementNode(ASTNode *statement)
            : ASTNode(NodeType::Expression) , statement(statement){
    }

    string dump_ast(int depth = 0) const {
        string result = "DefaultLabelStatement";
        return result;
    }

private:
    ASTNode *statement;
};


class IfElseStatementNode : public ASTNode {
public:
    IfElseStatementNode(ASTNode *expression, ASTNode *statement, ASTNode *else_statement)
            : ASTNode(NodeType::Expression), expression(expression), statement(statement), else_statement(else_statement){
    }


    string dump_ast(int depth = 0) const {
        string result = "IfElseStatement";
        return result;
    }

private:
    ASTNode *expression;
    ASTNode *statement;
    ASTNode *else_statement;


};


class IfStatementNode : public ASTNode {
public:
    IfStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::Expression), expression(expression), statement(statement){
    }

    string dump_ast(int depth = 0) const {
        string result = "IfStatement";
        return result;
    }

private:
    ASTNode *expression;
    ASTNode *statement;


};


class SwitchStatementNode : public ASTNode {
public:
    SwitchStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::Expression), expression(expression), statement(statement){
    }

    string dump_ast(int depth = 0) const {
        string result = "SwitchStatement";
        return result;
    }

private:
    ASTNode *expression;
    ASTNode *statement;


};

class WhileStatementNode : public ASTNode {
public:
    WhileStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::Expression), expression(expression), statement(statement){
    }

    string dump_ast(int depth = 0) const {
        string result = "WhileStatement";
        return result;
    }

private:
    ASTNode *expression;
    ASTNode *statement;


};

class DoWhileStatementNode : public ASTNode {
public:
    DoWhileStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::Expression), expression(expression), statement(statement) {
        expression = expression;
        statement = statement;
    }

    string dump_ast(int depth = 0) const {
        string result = "DoWhileStatement";
        return result;
    }

private:
    ASTNode *expression;
    ASTNode *statement;


};


class ForStatementNode : public ASTNode {
public:
    ForStatementNode(ASTNode *expression1, ASTNode *expression2, ASTNode *expression3, ASTNode *statement)
            : ASTNode(NodeType::Expression) {
        expression1 = expression1;
        expression2 = expression2;
        expression3 = expression3;
        statement = statement;
    }

    string dump_ast(int depth = 0) const {
        string result = "ForStatement";
        return result;
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
            : ASTNode(NodeType::Expression) {
        identifier = identifier;
    }

    string dump_ast(int depth = 0) const {
        string result = "GotoStatement";
        return result;
    }

private:
    ASTNode *identifier;


};

class ContinueStatementNode : public ASTNode {
public:
    ContinueStatementNode()
            : ASTNode(NodeType::Expression) {
    }

    string dump_ast(int depth = 0) const {
        string result = "ContinueStatement";
        return result;
    }

};

class BreakStatementNode : public ASTNode {
public:
    BreakStatementNode()
            : ASTNode(NodeType::Expression) {
    }

    string dump_ast(int depth = 0) const {
        string result = "BreakStatement";
        return result;
    }

};

class ReturnStatementNode : public ASTNode {
public:
    ReturnStatementNode()
            : ASTNode(NodeType::Expression) {
        expression = nullptr;
    }

    ReturnStatementNode(ASTNode *expression)
            : ASTNode(NodeType::Expression) {
        expression = expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "ReturnStatement";
        return result;
    }

private:
    ASTNode *expression;


};

class DeclarationNode : public ASTNode {

public:

    DeclarationNode(ASTNode *declaration_specifiers, ASTNode *init_declarator_list)
            : ASTNode(NodeType::Expression) {
        declaration_specifiers = declaration_specifiers;
        init_declarator_list = init_declarator_list;
    }

    string dump_ast(int depth = 0) const {
        string result = "DeclarationList";
        return result;
    }


private:
    ASTNode *declaration_specifiers;
    ASTNode *init_declarator_list;
    // Add member variables to hold information about declarations
};


class InitDeclartorNode : public ASTNode {
public:
    InitDeclartorNode(ASTNode *declarator, ASTNode *initializer)
            : ASTNode(NodeType::Expression) {

        declarator = declarator;
        initializer = initializer;
    }


    string dump_ast(int depth = 0) const {
        string result = "InitDeclartor";
        return result;
    }


private:
    ASTNode *declarator;
    ASTNode *initializer;

};

class InitDeclartorListNode : public ASTNode {
public:
    InitDeclartorListNode()
            : ASTNode(NodeType::Expression) {
    }

    string dump_ast(int depth = 0) const {
        string result = "InitDeclartorList";
        return result;
    }

};

class DeclaratorNode : public ASTNode {
public:
    DeclaratorNode(ASTNode *pointer, ASTNode *direct_declarator)
            : ASTNode(NodeType::Expression) {

        pointer = pointer;
        direct_declarator = direct_declarator;
    }


    string dump_ast(int depth = 0) const {
        string result = "Declarator";
        return result;
    }

private:
    ASTNode *pointer;
    ASTNode *direct_declarator;


};

class IdentifierNode : public ASTNode {
public:
    IdentifierNode(string name)
            : ASTNode(NodeType::Expression) {

        name = name;
    }

    IdentifierNode(void *ptr)
            : ASTNode(NodeType::Expression) {

        name = "UNKNOWN";
    }

    string dump_ast(int depth = 0) const {
        string result = "Identifier";
        return result;
    }

private:
    string name;

};

class ExpressionListNode : public ASTNode {
public:
    ExpressionListNode()
            : ASTNode(NodeType::Expression) {
    }

    string dump_ast(int depth = 0) const {
        string result = "ExpressionList";
        return result;
    }

};

class AssignmentExpressionNode : public ASTNode {

public:
    AssignmentExpressionNode(ASTNode *unary_expression, AssignmentOperator assOp, ASTNode *assignment_expression)
            : ASTNode(NodeType::Expression) {
        unary_expression = unary_expression;
        assOp = assOp;
        assignment_expression = assignment_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "AssignmentExpression";
        return result;
    }

private:
    ASTNode *unary_expression;
    AssignmentOperator assOp;
    ASTNode *assignment_expression;


};

class ArrayAccessNode : public ASTNode {
public:
    ArrayAccessNode(ASTNode *postfix_expression, ASTNode *expression)
            : ASTNode(NodeType::Expression) {
        postfix_expression = postfix_expression;
        expression = expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "ArrayAccess";
        return result;
    }

private:
    ASTNode *postfix_expression;
    ASTNode *expression;

};

class FunctionCallNode : public ASTNode {
public:
    FunctionCallNode(ASTNode *postfix_expression, ASTNode *argument_expression_list)
            : ASTNode(NodeType::Expression) {
        postfix_expression = postfix_expression;
        argument_expression_list = argument_expression_list;

    }

    string dump_ast(int depth = 0) const {
        string result = "FunctionCall";
        return result;
    }

private:
    ASTNode *argument_expression_list;
    ASTNode *postfix_expression;

};


class MemberAccessNode : public ASTNode {
public:
    MemberAccessNode(ASTNode *postfix_expression, ASTNode *identifier)
            : ASTNode(NodeType::Expression) {
        postfix_expression = postfix_expression;
        identifier = identifier;
    }

    string dump_ast(int depth = 0) const {
        string result = "MemberAccess";
        return result;
    }

private:
    ASTNode *postfix_expression;
    ASTNode *identifier;

};

class PostfixExpressionNode : public ASTNode {
public:
    PostfixExpressionNode(ASTNode *primary_expression, UnaryOperator postFixOp)
            : ASTNode(NodeType::Expression) {
        primary_expression = primary_expression;
        postFixOp = postFixOp;
    }

    string dump_ast(int depth = 0) const {
        string result = "PostfixExpression";
        return result;
    }

private:
    ASTNode *primary_expression;
    UnaryOperator postFixOp;

};

class ConditionalExpressionNode : public ASTNode {
public:
    ConditionalExpressionNode(ASTNode *logical_or_expression, ASTNode *expression, ASTNode *conditional_expression)
            : ASTNode(NodeType::Expression) {
        logical_or_expression = logical_or_expression;
        expression = expression;
        conditional_expression = conditional_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "ConditionalExpression";
        return result;
    }

private:
    ASTNode *logical_or_expression;
    ASTNode *expression;
    ASTNode *conditional_expression;
};

class LogicalOrExpressionNode : public ASTNode {
public:
    LogicalOrExpressionNode(ASTNode *logical_or_expression, ASTNode *logical_and_expression)
            : ASTNode(NodeType::Expression) {
        logical_and_expression = logical_and_expression;
        logical_or_expression = logical_or_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "LogicalOrExpression";
        return result;
    }

private:
    ASTNode *logical_and_expression;
    ASTNode *logical_or_expression;

};

class LogicalAndExpressionNode : public ASTNode {
public:
    LogicalAndExpressionNode(ASTNode *logical_and_expression, ASTNode *inclusive_or_expression)
            : ASTNode(NodeType::Expression) {
        logical_and_expression = logical_and_expression;
        inclusive_or_expression = inclusive_or_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "LogicalAndExpression";
        return result;
    }

private:
    ASTNode *logical_and_expression;
    ASTNode *inclusive_or_expression;

};

class InclusiveOrExpressionNode : public ASTNode {
public:
    InclusiveOrExpressionNode(ASTNode *inclusive_or_expression, ASTNode *exclusive_or_expression)
            : ASTNode(NodeType::Expression) {
        inclusive_or_expression = inclusive_or_expression;
        exclusive_or_expression = exclusive_or_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "InclusiveOrExpression";
        return result;
    }

private:
    ASTNode *inclusive_or_expression;
    ASTNode *exclusive_or_expression;

};

class ExclusiveOrExpressionNode : public ASTNode {
public:
    ExclusiveOrExpressionNode(ASTNode *exclusive_or_expression, ASTNode *and_expression)
            : ASTNode(NodeType::Expression) {
        exclusive_or_expression = exclusive_or_expression;
        and_expression = and_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "ExclusiveOrExpression";
        return result;
    }

private:
    ASTNode *exclusive_or_expression;
    ASTNode *and_expression;


};

class AndExpressionNode : public ASTNode {
public:
    AndExpressionNode(ASTNode *and_expression, ASTNode *equality_expression)
            : ASTNode(NodeType::Expression) {
        and_expression = and_expression;
        equality_expression = equality_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "AndExpression";
        return result;
    }

private:
    ASTNode *and_expression;
    ASTNode *equality_expression;

};

class EqualityExpressionNode : public ASTNode {
public:
    EqualityExpressionNode(ASTNode *equality_expression, ASTNode *relational_expression)
            : ASTNode(NodeType::Expression) {
        equality_expression = equality_expression;
        relational_expression = relational_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "EqualityExpression";
        return result;
    }

private:
    ASTNode *equality_expression;
    ASTNode *relational_expression;

};

class NonEqualityExpressionNode : public ASTNode {
public:
    NonEqualityExpressionNode(ASTNode *equality_expression, ASTNode *relational_expression)
            : ASTNode(NodeType::Expression) {
        equality_expression = equality_expression;
        relational_expression = relational_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "NonEqualityExpression";
        return result;
    }

private:
    ASTNode *equality_expression;
    ASTNode *relational_expression;

};

class LessThanExpressionNode : public ASTNode {
public:
    LessThanExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::Expression) {
        relational_expression = relational_expression;
        shift_expression = shift_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "LessThanExpression";
        return result;
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;

};

class GreaterThanExpressionNode : public ASTNode {
public:
    GreaterThanExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::Expression) {
        relational_expression = relational_expression;
        shift_expression = shift_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "GreaterThanExpression";
        return result;
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;

};

class LessOrEqualExpressionNode : public ASTNode {
public:
    LessOrEqualExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::Expression) {
        relational_expression = relational_expression;
        shift_expression = shift_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "LessOrEqualExpression";
        return result;
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;

};

class GreaterOrEqualExpressionNode : public ASTNode {
public:
    GreaterOrEqualExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::Expression) {
        relational_expression = relational_expression;
        shift_expression = shift_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "GreaterOrEqualExpression";
        return result;
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;

};

class LeftShiftExpressionNode : public ASTNode {

public:
    LeftShiftExpressionNode(ASTNode *shift_expression, ASTNode *additive_expression)
            : ASTNode(NodeType::Expression) {
        shift_expression = shift_expression;
        additive_expression = additive_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "LeftShiftExpression";
        return result;
    }

private:
    ASTNode *shift_expression;
    ASTNode *additive_expression;
};

class RightShiftExpressionNode : public ASTNode {
public:

    RightShiftExpressionNode(ASTNode *shift_expression, ASTNode *additive_expression)
            : ASTNode(NodeType::Expression) {
        shift_expression = shift_expression;
        additive_expression = additive_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "RightShiftExpression";
        return result;
    }

private:
    ASTNode *shift_expression;
    ASTNode *additive_expression;

};

class AdditiveExpressionNode : public ASTNode {
public:

    AdditiveExpressionNode(ASTNode *additive_expression, ASTNode *multiplicative_expression)
            : ASTNode(NodeType::Expression) {
        additive_expression = additive_expression;
        multiplicative_expression = multiplicative_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "AdditiveExpression";
        return result;
    }

private:
    ASTNode *additive_expression;
    ASTNode *multiplicative_expression;

};

class SubExpressionNode : public ASTNode {
public:

    SubExpressionNode(ASTNode *additive_expression, ASTNode *multiplicative_expression)
            : ASTNode(NodeType::Expression) {
        additive_expression = additive_expression;
        multiplicative_expression = multiplicative_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "SubExpression";
        return result;
    }

private:
    ASTNode *additive_expression;
    ASTNode *multiplicative_expression;

};

class MultiplicativeExpressionNode : public ASTNode {
public:

    MultiplicativeExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::Expression) {
        multiplicative_expression = multiplicative_expression;
        cast_expression = cast_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "MultiplicativeExpression";
        return result;
    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;

};

class DivExpressionNode : public ASTNode {
public:

    DivExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::Expression) {
        multiplicative_expression = multiplicative_expression;
        cast_expression = cast_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "DivExpression";
        return result;
    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;

};

class ModExpressionNode : public ASTNode {
public:

    ModExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::Expression) {
        multiplicative_expression = multiplicative_expression;
        cast_expression = cast_expression;
    }

    string dump_ast(int depth = 0) const {
        string result = "ModExpression";
        return result;
    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;

};

class IConstantNode : public ASTNode {
public:
    IConstantNode(int value)
            : ASTNode(NodeType::Expression) {
        value = value;
    }


    string dump_ast(int depth = 0) const {
        string result = "Constant";
        return result;
    }

private:
    int value;

};

class FConstantNode : public ASTNode {
public:
    FConstantNode(float value)
            : ASTNode(NodeType::Expression) {
        value = value;
    }

    string dump_ast(int depth = 0) const {
        string result = "FConstant";
        return result;
    }

private:
    float value;

};

class StringNode : public ASTNode {
public:
    StringNode(string value)
            : ASTNode(NodeType::Expression) {
        value = value;
    }


    string dump_ast(int depth = 0) const {
        string result = "String";
        return result;
    }

private:
    string value;

};

#endif // AST_HPP

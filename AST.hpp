#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include "AST_enums.hpp"
#include "scoper.h"



using namespace std;

static ScoperStack scoperStack;

class ASTNode {
public:
    ASTNode(NodeType type) : type(type) {}

    virtual ~ASTNode() {}

    NodeType getNodeType() const { return type; }


    virtual string dump_ast(int depth = 0) const {

        return formatSpacing(depth) + "Interface Node \n";
        // Implement this method to dump the AST as a string
    }

    void addChild(ASTNode *child) {
        children.push_back(child);
    }

    virtual bool check_semantics() {

    }


    vector<ASTNode *> children;
    NodeType type;
};


static std::string
dumpParameters(const ASTNode *base, std::vector<ASTNode *> vec_of_nodes, int depth = 0, bool is_list = false) {
    string result = formatSpacing(depth);
    result += nodeTypeToString(base->getNodeType());
    result += is_list ? "[ \n" : "{ \n";

    for (auto child: vec_of_nodes) {
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
    NullPtrNode()
            : ASTNode(NodeType::Unimplemented) {
    }

    string dump_ast(int depth = 0) const {
        string result = "";
        result += formatSpacing(depth);
        result += "Unimplemented\n";
        return result;
    }

    bool check_semantics() {
        return true;
    }
};


class TranslationUnitNode : public ASTNode {
public:
    TranslationUnitNode()
            : ASTNode(NodeType::TranslationUnit) {

    }


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
        return dumpParameters(this, {declaration_specifiers, declarator, declaration_list, compound_statement}, depth,
                              false);
    }

    bool check_semantics() {
        scoperStack.push();
        if (declaration_list != nullptr) {
            declaration_list->check_semantics();
        }
        compound_statement->check_semantics();
        scoperStack.pop();
        return true;
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

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }


};


class SpecifierNode : public ASTNode {
public:
    SpecifierNode(SpecifierEnum specifier)
            : ASTNode(NodeType::Specifier), specifier(specifier) {
    }

    string dump_ast(int depth = 0) const {
        string result = "";
        result += formatSpacing(depth);
        result += specifierEnumToString(specifier);
        result += "\n";
        return result;
    }


    SpecifierEnum specifier;
};

class CompoundStatementNode : public ASTNode {

public:
    CompoundStatementNode()
            : ASTNode(NodeType::CompoundStatement) {

    }


    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }


};


class LabelStatementNode : public ASTNode {
public:
    LabelStatementNode(ASTNode *label, ASTNode *statement)
            : ASTNode(NodeType::LabelStatement), label(label), statement(statement) {

    }

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
            : ASTNode(NodeType::CaseLabelStatement), constant_expression(constant_expression), statement(statement) {
    }

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
            : ASTNode(NodeType::DefaultLabelStatement), statement(statement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {statement}, depth, false);

    }

private:
    ASTNode *statement;
};


class IfElseStatementNode : public ASTNode {
public:
    IfElseStatementNode(ASTNode *expression, ASTNode *statement, ASTNode *else_statement)
            : ASTNode(NodeType::IfElseStatement), expression(expression), statement(statement),
              else_statement(else_statement) {
    }


    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {expression, statement, else_statement}, depth, false);

    }

private:
    ASTNode *expression;
    ASTNode *statement;
    ASTNode *else_statement;


};


class IfStatementNode : public ASTNode {
public:
    IfStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::IfStatement), expression(expression), statement(statement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {expression, statement}, depth, false);

    }

private:
    ASTNode *expression;
    ASTNode *statement;
};


class SwitchStatementNode : public ASTNode {
public:
    SwitchStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::SwitchStatement), expression(expression), statement(statement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {expression,statement}, depth, false);

    }

private:
    ASTNode *expression;
    ASTNode *statement;


};

class WhileStatementNode : public ASTNode {
public:
    WhileStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::WhileStatement), expression(expression), statement(statement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {expression, statement}, depth, false);

    }

private:
    ASTNode *expression;
    ASTNode *statement;


};

class DoWhileStatementNode : public ASTNode {
public:
    DoWhileStatementNode(ASTNode *expression, ASTNode *statement)
            : ASTNode(NodeType::DoWhileStatement), expression(expression), statement(statement) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {statement, expression}, depth, false);

    }

private:
    ASTNode *expression;
    ASTNode *statement;
};


class ForStatementNode : public ASTNode {
public:
    ForStatementNode(ASTNode *expression1, ASTNode *expression2, ASTNode *expression3, ASTNode *statement)
            : ASTNode(NodeType::ForStatement), expression1(expression1), expression2(expression2),
              expression3(expression3), statement(statement) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {expression1, expression2, expression3, statement}, depth, false);

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
    ContinueStatementNode()
            : ASTNode(NodeType::ContinueStatement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {}, depth, false);

    }

};

class BreakStatementNode : public ASTNode {
public:
    BreakStatementNode()
            : ASTNode(NodeType::BreakStatement) {
    }

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {}, depth, false);

    }

};

class ReturnStatementNode : public ASTNode {
public:
    ReturnStatementNode()
            : ASTNode(NodeType::ReturnStatement) {
        expression = nullptr;
    }

    ReturnStatementNode(ASTNode *expression)
            : ASTNode(NodeType::ReturnStatement) {
        expression = expression;
    }

    string dump_ast(int depth = 0) const {
        if(expression != nullptr)
            return dumpParameters(this, {expression}, depth, false);
        else
            return dumpParameters(this, {}, depth, false);
    }

private:
    ASTNode *expression;


};


class DeclarationListNode : public ASTNode {
public:
    DeclarationListNode()
            : ASTNode(NodeType::DeclarationList) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }


};

class DeclarationNode : public ASTNode {
public:
    DeclarationNode(ASTNode *declaration_specifiers, ASTNode *init_declarator_list)
            : ASTNode(NodeType::Declaration), declaration_specifiers(declaration_specifiers),
              init_declarator_list(init_declarator_list) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {declaration_specifiers, init_declarator_list}, depth, false);
    }

private:
    ASTNode *declaration_specifiers;
    ASTNode *init_declarator_list;
    // Add member variables to hold information about declarations
};


class InitDeclartorNode : public ASTNode {
public:
    InitDeclartorNode(ASTNode *declarator, ASTNode *initializer)
            : ASTNode(NodeType::InitDeclartor), declarator(declarator), initializer(initializer) {}


    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {declarator, initializer}, depth, false);

    }


private:
    ASTNode *declarator;
    ASTNode *initializer;

};

class InitDeclartorListNode : public ASTNode {
public:
    InitDeclartorListNode()
            : ASTNode(NodeType::InitDeclartorList) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }

};

class DeclaratorNode : public ASTNode {
public:
    DeclaratorNode(ASTNode *pointer, ASTNode *direct_declarator)
            : ASTNode(NodeType::Declarator), pointer(pointer), direct_declarator(direct_declarator) {}


    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth) + "Declarator{\n";
        result += pointer->dump_ast(depth + 1);
        result += direct_declarator->dump_ast(depth + 1);
        result += formatSpacing(depth);
        result += "}\n";
        return result;
    }

private:
    ASTNode *pointer;
    ASTNode *direct_declarator;
};

class DirectDeclaratorNode : public ASTNode {
public:
    DirectDeclaratorNode(ASTNode *direct_declarator, ASTNode *parameter_type_list)
            : ASTNode(NodeType::DirectDeclarator), direct_declarator(direct_declarator),
              parameter_type_list(parameter_type_list) {}

    string dump_ast(int depth = 0) const {

        return dumpParameters(this, {direct_declarator, parameter_type_list}, depth, false);
    }

private:
    ASTNode *direct_declarator;
    ASTNode *parameter_type_list;

};

class ParameterListNode : public ASTNode {
public:
    ParameterListNode()
            : ASTNode(NodeType::ParameterList) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);

    }
};

class ParameterDeclarationNode : public ASTNode {
public:
    ParameterDeclarationNode(ASTNode *declaration_specifiers, ASTNode *declarator)
            : ASTNode(NodeType::ParameterDeclaration), declaration_specifiers(declaration_specifiers),
              declarator(declarator) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {declaration_specifiers, declarator}, depth, false);
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

private:
    string name;
};

class IdentifierListNode : public ASTNode {
public:
    IdentifierListNode()
            : ASTNode(NodeType::IdentifierList) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }


};

class ExpressionListNode : public ASTNode {
public:
    ExpressionListNode()
            : ASTNode(NodeType::ExpressionList) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, children, depth, true);
    }

};

class AssignmentExpressionNode : public ASTNode {
public:
    AssignmentExpressionNode(ASTNode *unary_expression, AssignmentOperator assOp, ASTNode *assignment_expression)
            : ASTNode(NodeType::AssignmentExpression), unary_expression(unary_expression), assOp(assOp),
              assignment_expression(assignment_expression) {}

    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth);
        result += assignmentOperatorToString(assOp) + "{ \n";
        result += unary_expression->dump_ast(depth + 1);
        result += assignment_expression->dump_ast(depth + 1);
        result += formatSpacing(depth) + "} \n";
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
            : ASTNode(NodeType::ArrayAccess), postfix_expression(postfix_expression), expression(expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {postfix_expression, expression}, depth, false);
    }

private:
    ASTNode *postfix_expression;
    ASTNode *expression;
};

class FunctionCallNode : public ASTNode {
public:
    FunctionCallNode(ASTNode *postfix_expression, ASTNode *argument_expression_list)
            : ASTNode(NodeType::FunctionCall), postfix_expression(postfix_expression),
              argument_expression_list(argument_expression_list) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {postfix_expression, argument_expression_list}, depth, false);
    }

private:
    ASTNode *postfix_expression;
    ASTNode *argument_expression_list;
};

class MemberAccessNode : public ASTNode {
public:
    MemberAccessNode(ASTNode *postfix_expression, ASTNode *identifier)
            : ASTNode(NodeType::MemberAccess), postfix_expression(postfix_expression), identifier(identifier) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {postfix_expression, identifier}, depth, false);
    }

private:
    ASTNode *postfix_expression;
    ASTNode *identifier;
};


class PostfixExpressionNode : public ASTNode {
public:
    PostfixExpressionNode(ASTNode *primary_expression, UnaryOperator postFixOp)
            : ASTNode(NodeType::PostfixExpression), primary_expression(primary_expression), postFixOp(postFixOp) {}

    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth);
        result += unaryOperatorToString(postFixOp) + "{ \n";
        result += primary_expression->dump_ast(depth + 1);
        result += formatSpacing(depth) + "} \n";
        return result;
    }

private:
    ASTNode *primary_expression;
    UnaryOperator postFixOp;
};

class ConditionalExpressionNode : public ASTNode {
public:
    ConditionalExpressionNode(ASTNode *logical_or_expression, ASTNode *expression, ASTNode *conditional_expression)
            : ASTNode(NodeType::ConditionalExpression), logical_or_expression(logical_or_expression),
              expression(expression), conditional_expression(conditional_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {logical_or_expression, expression, conditional_expression}, depth, false);
    }

private:
    ASTNode *logical_or_expression;
    ASTNode *expression;
    ASTNode *conditional_expression;
};

class LogicalOrExpressionNode : public ASTNode {
public:
    LogicalOrExpressionNode(ASTNode *logical_or_expression, ASTNode *logical_and_expression)
            : ASTNode(NodeType::LogicalOrExpression), logical_or_expression(logical_or_expression),
              logical_and_expression(logical_and_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {logical_or_expression, logical_and_expression}, depth, false);
    }

private:
    ASTNode *logical_or_expression;
    ASTNode *logical_and_expression;
};

class LogicalAndExpressionNode : public ASTNode {
public:
    LogicalAndExpressionNode(ASTNode *logical_and_expression, ASTNode *inclusive_or_expression)
            : ASTNode(NodeType::LogicalAndExpression), logical_and_expression(logical_and_expression),
              inclusive_or_expression(inclusive_or_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {logical_and_expression, inclusive_or_expression}, depth, false);
    }

private:
    ASTNode *logical_and_expression;
    ASTNode *inclusive_or_expression;
};

class InclusiveOrExpressionNode : public ASTNode {
public:
    InclusiveOrExpressionNode(ASTNode *inclusive_or_expression, ASTNode *exclusive_or_expression)
            : ASTNode(NodeType::InclusiveOrExpression), inclusive_or_expression(inclusive_or_expression),
              exclusive_or_expression(exclusive_or_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {inclusive_or_expression, exclusive_or_expression}, depth, false);
    }

private:
    ASTNode *inclusive_or_expression;
    ASTNode *exclusive_or_expression;
};

class ExclusiveOrExpressionNode : public ASTNode {
public:
    ExclusiveOrExpressionNode(ASTNode *exclusive_or_expression, ASTNode *and_expression)
            : ASTNode(NodeType::ExclusiveOrExpression), exclusive_or_expression(exclusive_or_expression),
              and_expression(and_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {exclusive_or_expression, and_expression}, depth, false);
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
        return dumpParameters(this, {and_expression, equality_expression}, depth, false);

    }

private:
    ASTNode *and_expression;
    ASTNode *equality_expression;
};

class EqualityExpressionNode : public ASTNode {
public:
    EqualityExpressionNode(ASTNode *equality_expression, ASTNode *relational_expression)
            : ASTNode(NodeType::EqualityExpression), equality_expression(equality_expression),
              relational_expression(relational_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {equality_expression, relational_expression}, depth, false);
    }

private:
    ASTNode *equality_expression;
    ASTNode *relational_expression;
};

class NonEqualityExpressionNode : public ASTNode {
public:
    NonEqualityExpressionNode(ASTNode *equality_expression, ASTNode *relational_expression)
            : ASTNode(NodeType::NonEqualityExpression), equality_expression(equality_expression),
              relational_expression(relational_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {equality_expression, relational_expression}, depth, false);
    }

private:
    ASTNode *equality_expression;
    ASTNode *relational_expression;
};

class LessThanExpressionNode : public ASTNode {
public:
    LessThanExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::LessThanExpression), relational_expression(relational_expression),
              shift_expression(shift_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {relational_expression, shift_expression}, depth, false);
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;
};

class GreaterThanExpressionNode : public ASTNode {
public:
    GreaterThanExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::GreaterThanExpression), relational_expression(relational_expression),
              shift_expression(shift_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {relational_expression, shift_expression}, depth, false);
    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;
};

class LessOrEqualExpressionNode : public ASTNode {
public:
    LessOrEqualExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::LessOrEqualExpression), relational_expression(relational_expression),
              shift_expression(shift_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {relational_expression, shift_expression}, depth, false);

    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;
};

class GreaterOrEqualExpressionNode : public ASTNode {
public:
    GreaterOrEqualExpressionNode(ASTNode *relational_expression, ASTNode *shift_expression)
            : ASTNode(NodeType::GreaterOrEqualExpression), relational_expression(relational_expression),
              shift_expression(shift_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {relational_expression, shift_expression}, depth, false);

    }

private:
    ASTNode *relational_expression;
    ASTNode *shift_expression;
};

class LeftShiftExpressionNode : public ASTNode {
public:
    LeftShiftExpressionNode(ASTNode *shift_expression, ASTNode *additive_expression)
            : ASTNode(NodeType::LeftShiftExpression), shift_expression(shift_expression),
              additive_expression(additive_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {shift_expression, additive_expression}, depth, false);

    }

private:
    ASTNode *shift_expression;
    ASTNode *additive_expression;
};

class RightShiftExpressionNode : public ASTNode {
public:
    RightShiftExpressionNode(ASTNode *shift_expression, ASTNode *additive_expression)
            : ASTNode(NodeType::RightShiftExpression), shift_expression(shift_expression),
              additive_expression(additive_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {shift_expression, additive_expression}, depth, false);

    }

private:
    ASTNode *shift_expression;
    ASTNode *additive_expression;
};

class AdditiveExpressionNode : public ASTNode {
public:
    AdditiveExpressionNode(ASTNode *additive_expression, ASTNode *multiplicative_expression)
            : ASTNode(NodeType::AdditiveExpression), additive_expression(additive_expression),
              multiplicative_expression(multiplicative_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {additive_expression, multiplicative_expression}, depth, false);

    }

private:
    ASTNode *additive_expression;
    ASTNode *multiplicative_expression;
};

class SubExpressionNode : public ASTNode {
public:
    SubExpressionNode(ASTNode *additive_expression, ASTNode *multiplicative_expression)
            : ASTNode(NodeType::SubExpression), additive_expression(additive_expression),
              multiplicative_expression(multiplicative_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {additive_expression, multiplicative_expression}, depth, false);

    }

private:
    ASTNode *additive_expression;
    ASTNode *multiplicative_expression;
};

class MultiplicativeExpressionNode : public ASTNode {
public:
    MultiplicativeExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::MultiplicativeExpression), multiplicative_expression(multiplicative_expression),
              cast_expression(cast_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {multiplicative_expression, cast_expression}, depth, false);

    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;
};

class DivExpressionNode : public ASTNode {
public:
    DivExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::DivExpression), multiplicative_expression(multiplicative_expression),
              cast_expression(cast_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {multiplicative_expression, cast_expression}, depth, false);

    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;
};

class ModExpressionNode : public ASTNode {
public:
    ModExpressionNode(ASTNode *multiplicative_expression, ASTNode *cast_expression)
            : ASTNode(NodeType::ModExpression), multiplicative_expression(multiplicative_expression),
              cast_expression(cast_expression) {}

    string dump_ast(int depth = 0) const {
        return dumpParameters(this, {multiplicative_expression, cast_expression}, depth, false);

    }

private:
    ASTNode *multiplicative_expression;
    ASTNode *cast_expression;
};

class IConstantNode : public ASTNode {
public:
    IConstantNode(int value)
            : ASTNode(NodeType::IConstant), value(value) {}

    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth);
        result += "Integer:" + to_string(value) + "\n";
        return result;
    }

private:
    int value;
};

class FConstantNode : public ASTNode {
public:
    FConstantNode(float value)
            : ASTNode(NodeType::FConstant), value(value) {}

    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth);
        result += "Float:" + to_string(value) + "\n";
        return result;
    }

private:
    float value;
};

class StringNode : public ASTNode {
public:
    StringNode(string value)
            : ASTNode(NodeType::String), value(value) {}

    string dump_ast(int depth = 0) const {
        string result = formatSpacing(depth);
        result += "String Literal : " + value + " \n";
        return result;
    }

private:
    string value;
};


#endif // AST_HPP

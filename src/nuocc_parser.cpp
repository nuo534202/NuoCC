#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <iostream>

#include "nodes/nuocc_scanner_nodes.hpp"
#include "utils/nuocc_print.hpp"

namespace nuocc
{

AstNodePtr Parser::Parse(const std::vector<NodePtr>& token_list)
{
    idx_t i = 0;
    AstNodePtr program = CompoundStatement(token_list, i);

    if (TokenTag(token_list[i]) != T_EOF)
    {
        std::cerr << "syntax error: unexpected token ";
        std::cerr << NodeTagToString(TokenTag(token_list[i]));
        std::cerr << " after the program!" << std::endl;
        std::exit(1);
    }

    return program;
}

/*
 * compound_statement: '{' '}'
 *      |      '{' statement '}'
 *      |      '{' statement statements '}'
 *      ;
 */
AstNodePtr Parser::CompoundStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_LBrace, "{");

    AstNodePtr left = nullptr;

    while (true)
    {
        if (TokenTag(token_list[i]) == T_EOF)
        {
            std::cerr << "syntax error: expect }!" << std::endl;
            std::exit(1);
        }

        if (TokenTag(token_list[i]) == T_RBrace)
            break;

        AstNodePtr tree = Statement(token_list, i);

        /* Glue each statement onto the statements parsed before it. */
        if (!left)
        {
            left = std::move(tree);
        }
        else
        {
            AstNodePtr glued = std::make_unique<AstGlue>(left, tree);
            left = std::move(glued);
        }
    }

    i++;    /* step over the right brace */

    return left;
}

/*
 * statement: print_statement
 *      |     declaration
 *      |     assignment_statement
 *      |     if_statement
 *      ;
 */
AstNodePtr Parser::Statement(const std::vector<NodePtr>& token_list, idx_t& i)
{
    switch (TokenTag(token_list[i]))
    {
        case T_Print:
            return PrintStatement(token_list, i);
        case T_Int:
            return DeclareStatement(token_list, i);
        case T_If:
            return IfStatement(token_list, i);
        case T_Identifier:
            return AssignStatement(token_list, i);
        default:
            std::cerr << "syntax error: unexpected token ";
            std::cerr << NodeTagToString(TokenTag(token_list[i])) << "!";
            std::cerr << std::endl;
            std::exit(1);
    }
}

/* print_statement: 'print' expression ';'  ; */
AstNodePtr Parser::PrintStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Print, "print");

    AstNodePtr expression = BinaryExpression(token_list, i, 0);
    AstNodePtr root = std::make_unique<AstPrint>(expression);

    Match(token_list, i, T_Semicolon, ";");

    return root;
}

/* declaration: 'int' identifier ';'  ; */
AstNodePtr Parser::DeclareStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Int, "int");

    if (TokenTag(token_list[i]) != T_Identifier)
    {
        std::cerr << "syntax error: expect an identifier!" << std::endl;
        std::exit(1);
    }

    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());
    Symbol symbol_name = ident->GetName();

    symbol_table_.AddSymbol(symbol_name);

    i++;

    Match(token_list, i, T_Semicolon, ";");

    return std::make_unique<AstDeclare>(symbol_name);
}

/* assignment_statement: identifier '=' expression ';'  ; */
AstNodePtr Parser::AssignStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());
    Symbol symbol_name = ident->GetName();
    idx_t sym_idx = symbol_table_.FindSymbol(symbol_name);

    if (!sym_idx)
    {
        std::cerr << "syntax error: undeclared variable " << symbol_name;
        std::cerr << "!" << std::endl;
        std::exit(1);
    }

    /* The identifier names the target of the assignment, it is an lvalue. */
    AstNodePtr right = MakeAstIdentLeaf(token_list[i], sym_idx, true);

    i++;

    Match(token_list, i, T_Assign, "=");

    AstNodePtr left = BinaryExpression(token_list, i, 0);
    AstNodePtr root = std::make_unique<AstOperator>(left, right, T_Assign);

    Match(token_list, i, T_Semicolon, ";");

    return root;
}

/*
 * if_statement: if_head
 *      |        if_head 'else' compound_statement
 *      ;
 *
 * if_head: 'if' '(' true_false_expression ')' compound_statement  ;
 */
AstNodePtr Parser::IfStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_If, "if");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);

    if (condition->GetAstNodeTag() != A_AstOperator ||
        !IsComparisonOperator(
            static_cast<const AstOperator *>(condition.get())->GetOpType()))
    {
        std::cerr << "syntax error: an if condition must be a comparison";
        std::cerr << "!" << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_RParen, ")");

    AstNodePtr true_branch = CompoundStatement(token_list, i);
    AstNodePtr false_branch = nullptr;
    bool has_else = false;

    if (TokenTag(token_list[i]) == T_Else)
    {
        has_else = true;
        i++;
        false_branch = CompoundStatement(token_list, i);
    }

    return std::make_unique<AstIf>(condition,
                                   true_branch,
                                   false_branch,
                                   has_else);
}

AstNodePtr Parser::BinaryExpression(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    uint8 ptp) /* previous token precedence */
{
    AstNodePtr left = ParsePrimary(token_list[i++]);

    /*
     * Only a binary operator has a non-zero precedence, so the loop
     * stops as soon as a semicolon, an EOF or any other token shows up.
     */
    while (GetOpPrecedence(TokenTag(token_list[i])) > ptp)
    {
        idx_t op_idx = i;
        uint8 op_prec = GetOpPrecedence(TokenTag(token_list[op_idx]));

        i++;

        AstNodePtr right = BinaryExpression(token_list, i, op_prec);
        left = MakeAstNode(left, right, token_list[op_idx]);
    }

    return left;
}

AstNodePtr
Parser::ParsePrimary(const NodePtr& token)
{
    switch (TokenTag(token))
    {
        case T_IntLit:
            return MakeAstLeaf(token);
        case T_Identifier:
        {
            const Identifier *ident =
                static_cast<const Identifier *>(token.get());
            idx_t sym_idx = symbol_table_.FindSymbol(ident->GetName());

            if (!sym_idx)
            {
                std::cerr << "syntax error: undeclared variable ";
                std::cerr << ident->GetName() << "!" << std::endl;
                std::exit(1);
            }

            return MakeAstIdentLeaf(token, sym_idx, false);
        }
        default:
            std::cerr << "syntax error: unexpected token ";
            std::cerr << NodeTagToString(TokenTag(token));
            std::cerr << ", expect an expression!" << std::endl;
            std::exit(1);
    }

    return nullptr;
}

AstNodePtr Parser::MakeAstNode(AstNodePtr& left,
                               AstNodePtr& right,
                               const NodePtr& node)
{
    switch (node->GetNodeTag())
    {
        case T_IntLit:
        {
            const Literal<int, T_IntLit>* intlit_node =
                static_cast<const Literal<int, T_IntLit>*>(node.get());
            return std::make_unique<AstIntLit>(left, right,
                                               intlit_node->GetValue());
        }
        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
            return std::make_unique<AstOperator>(left, right,
                                                 node->GetNodeTag());
        default:
            std::cerr << "code error: token ";
            std::cerr << NodeTagToString(node->GetNodeTag());
            std::cerr << " is not a binary operator!" << std::endl;
            std::exit(1);
    }

    return nullptr;
}

AstNodePtr Parser::MakeAstLeaf(const NodePtr& node)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return MakeAstNode(left, right, node);
}

AstNodePtr Parser::MakeAstUnary(AstNodePtr& left,
    const NodePtr& node)
{
    AstNodePtr right = nullptr;

    return MakeAstNode(left, right, node);
}

AstNodePtr Parser::MakeAstIdent(AstNodePtr& left,
    AstNodePtr& right,
    const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    const Identifier *ident =
        static_cast<const Identifier *>(node.get());
    return std::make_unique<AstIdentifier>(left,
        right, ident->GetName(), ident_idx, is_lv_ident);
}

AstNodePtr Parser::MakeAstIdentLeaf(const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    AstNodePtr left = nullptr, right = nullptr;
    return MakeAstIdent(left, right, node, ident_idx, is_lv_ident);
}

AstNodePtr Parser::MakeAstIdentUnary(AstNodePtr& left,
    const NodePtr& node,
    idx_t ident_idx,
    bool is_lv_ident)
{
    AstNodePtr right = nullptr;
    return MakeAstIdent(left, right, node, ident_idx, is_lv_ident);
}

NodeTag Parser::TokenTag(const NodePtr& token)
{
    if (token->GetNodeTag() == T_KeyWord)
        return static_cast<const KeyWord *>(token.get())->GetWord();

    return token->GetNodeTag();
}

bool Parser::IsComparisonOperator(NodeTag tag)
{
    switch (tag)
    {
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
            return true;
        default:
            return false;
    }
}

void Parser::Match(const std::vector<NodePtr>& token_list,
    idx_t& i,
    NodeTag tag,
    std::string_view name)
{
    if (TokenTag(token_list[i]) != tag)
    {
        std::cerr << "syntax error: expect " << name << "!" << std::endl;
        std::exit(1);
    }

    i++;
}

uint8 Parser::GetOpPrecedence(NodeTag tag)
{
    auto it = kOpPrecedence.find(tag);

    /* Anything which is not a binary operator has zero precedence. */
    if (it == kOpPrecedence.end())
        return 0;

    return it->second;
}

/*
 * Operator precedence, following the C language. The values themselves are
 * meaningless, only their relative order matters: a higher value binds
 * more tightly.
 */
const std::unordered_map<NodeTag, uint8> Parser::kOpPrecedence = {
    {T_EQ, 10}, {T_NE, 10},

    {T_LT, 20}, {T_GT, 20}, {T_LE, 20}, {T_GE, 20},

    {T_Plus, 30}, {T_Minus, 30},

    {T_Star, 40}, {T_Slash, 40}
};

}   /* namespace nuocc */

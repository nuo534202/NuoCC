#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <iostream>

#include "nodes/nuocc_scanner_nodes.hpp"
#include "utils/nuocc_print.hpp"
#include "utils/nuocc_runtime.hpp"

namespace nuocc
{

namespace
{

/*
 * The simple statements are the ones terminated by a semicolon. The
 * statements which end in a compound statement, if, while and for, are
 * not, which is why the terminator is matched by the caller.
 */
bool IsSimpleStatement(AstNodeTag tag)
{
    switch (tag)
    {
        case A_AstPrint:
        case A_AstDeclare:
        case A_AstFuncCall:
        case A_AstReturn:
        case A_AstOperator:     /* an assignment */
            return true;
        default:
            return false;
    }
}

}   /* namespace */

std::vector<AstNodePtr> Parser::Parse(const std::vector<NodePtr>& token_list)
{
    /*
     * printint() is provided by the runtime the generated code is linked
     * with, so it is already known before any of the program is parsed.
     */
    symbol_table_.AddSymbol(Symbol{.name = kPrintIntName,
                                   .type = PrimitiveType::kVoid,
                                   .stype = StructuralType::kFunction});

    idx_t i = 0;
    std::vector<AstNodePtr> functions;

    while (TokenTag(token_list[i]) != T_EOF)
        functions.push_back(FunctionDeclaration(token_list, i));

    if (functions.empty())
    {
        std::cerr << "syntax error: the program has no function!" << std::endl;
        std::exit(1);
    }

    return functions;
}

/*
 * function_declaration: type identifier '(' ')' compound_statement  ;
 */
AstNodePtr Parser::FunctionDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    PrimitiveType type = ParseType(token_list, i);

    if (TokenTag(token_list[i]) != T_Identifier)
    {
        std::cerr << "syntax error: expect a function name!" << std::endl;
        std::exit(1);
    }

    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());

    Symbol symbol{.name = ident->GetName(),
                  .type = type,
                  .stype = StructuralType::kFunction};

    symbol_table_.AddSymbol(symbol);

    Match(token_list, i, T_Identifier, "a function name");
    Match(token_list, i, T_LParen, "(");
    Match(token_list, i, T_RParen, ")");

    current_function_type_ = type;

    AstNodePtr body = CompoundStatement(token_list, i);

    current_function_type_ = PrimitiveType::kNone;

    /*
     * A function which returns a value must end with a return statement, as
     * that is the only way its caller can be given a result.
     */
    if (type != PrimitiveType::kVoid)
    {
        const AstNodePtr& last = (body && body->GetAstNodeTag() == A_AstGlue)
                                 ? body->GetRight()
                                 : body;

        if (!last || last->GetAstNodeTag() != A_AstReturn)
        {
            std::cerr << "syntax error: function " << symbol.name;
            std::cerr << " does not end with a return!" << std::endl;
            std::exit(1);
        }
    }

    return std::make_unique<AstFunction>(body, symbol);
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

        /* A simple statement is terminated by a semicolon. */
        if (IsSimpleStatement(tree->GetAstNodeTag()))
            Match(token_list, i, T_Semicolon, ";");

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
 *      |     function_call
 *      |     if_statement
 *      |     while_statement
 *      |     for_statement
 *      |     return_statement
 *      ;
 */
AstNodePtr Parser::Statement(const std::vector<NodePtr>& token_list, idx_t& i)
{
    switch (TokenTag(token_list[i]))
    {
        case T_Print:
            return PrintStatement(token_list, i);
        case T_Int:
        case T_Char:
        case T_Long:
            return DeclareStatement(token_list, i);
        case T_If:
            return IfStatement(token_list, i);
        case T_While:
            return WhileStatement(token_list, i);
        case T_For:
            return ForStatement(token_list, i);
        case T_Return:
            return ReturnStatement(token_list, i);
        case T_Identifier:
            /* A '(' after the name turns the statement into a call. */
            if (TokenTag(token_list[i + 1]) == T_LParen)
                return FuncCall(token_list, i);

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

    /* printint() always takes an int, so widen what it is given. */
    TypeMatch match = MatchTypes(PrimitiveType::kInt,
                                 expression->GetType(),
                                 false);

    if (!match.compatible)
    {
        std::cerr << "syntax error: this value cannot be printed!";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (match.widen_right)
        expression = MakeWiden(expression, PrimitiveType::kInt);

    return std::make_unique<AstPrint>(expression);
}

/* declaration: type identifier ';'  ; */
AstNodePtr Parser::DeclareStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    PrimitiveType type = ParseType(token_list, i);

    if (TokenTag(token_list[i]) != T_Identifier)
    {
        std::cerr << "syntax error: expect a variable name!" << std::endl;
        std::exit(1);
    }

    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());

    Symbol symbol{.name = ident->GetName(),
                  .type = type,
                  .stype = StructuralType::kVariable};

    symbol_table_.AddSymbol(symbol);

    Match(token_list, i, T_Identifier, "a variable name");

    return std::make_unique<AstDeclare>(symbol);
}

/* assignment_statement: identifier '=' expression ';'  ; */
AstNodePtr Parser::AssignStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Symbol target = LookupTyped(token_list[i], StructuralType::kVariable,
                                "variable");

    /* The identifier names the target of the assignment, it is an lvalue. */
    AstNodePtr lvalue = MakeIdentLeaf(target, true);

    Match(token_list, i, T_Identifier, "a variable name");
    Match(token_list, i, T_Assign, "=");

    AstNodePtr value = BinaryExpression(token_list, i, 0);

    /*
     * The variable keeps its own type, so a value which would have to be
     * narrowed to fit it is rejected.
     */
    TypeMatch match = MatchTypes(value->GetType(), target.type, true);

    if (!match.compatible)
    {
        std::cerr << "syntax error: cannot store this value in ";
        std::cerr << target.name << "!" << std::endl;
        std::exit(1);
    }

    if (match.widen_left)
        value = MakeWiden(value, target.type);

    return std::make_unique<AstOperator>(value, lvalue, T_Assign,
                                         target.type);
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

    AstNodePtr condition = Condition(token_list, i, "an if statement");
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

/*
 * while_statement: 'while' '(' true_false_expression ')' compound_statement  ;
 */
AstNodePtr Parser::WhileStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_While, "while");

    AstNodePtr condition = Condition(token_list, i, "a while statement");
    AstNodePtr body = CompoundStatement(token_list, i);

    return std::make_unique<AstWhile>(condition, body);
}

/*
 * for_statement: 'for' '(' preop_statement ';'
 *                        true_false_expression ';'
 *                        postop_statement ')' compound_statement  ;
 *
 * The post statement is parsed before the body but has to run after it, so
 * the loop is desugared into an augmented while loop:
 *
 *      preop;
 *      while (condition) {
 *          body;
 *          postop;
 *      }
 */
AstNodePtr Parser::ForStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_For, "for");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr preop = Statement(token_list, i);
    Match(token_list, i, T_Semicolon, ";");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);
    CheckComparison(condition, "a for statement");

    Match(token_list, i, T_Semicolon, ";");

    AstNodePtr postop = Statement(token_list, i);
    Match(token_list, i, T_RParen, ")");

    AstNodePtr body = CompoundStatement(token_list, i);

    /* The post statement runs at the end of each pass of the loop. */
    AstNodePtr loop_body = std::make_unique<AstGlue>(body, postop);
    AstNodePtr loop = std::make_unique<AstWhile>(condition, loop_body);

    return std::make_unique<AstGlue>(preop, loop);
}

/*
 * return_statement: 'return' '(' expression ')'  ;
 */
AstNodePtr Parser::ReturnStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    if (current_function_type_ == PrimitiveType::kVoid)
    {
        std::cerr << "syntax error: a void function cannot return a value!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_Return, "return");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr expression = BinaryExpression(token_list, i, 0);

    /* The value has to fit the return type of the enclosing function. */
    TypeMatch match = MatchTypes(expression->GetType(),
                                 current_function_type_,
                                 true);

    if (!match.compatible)
    {
        std::cerr << "syntax error: this value does not fit the return type";
        std::cerr << " of the function!" << std::endl;
        std::exit(1);
    }

    if (match.widen_left)
        expression = MakeWiden(expression, current_function_type_);

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstReturn>(expression);
}

/*
 * function_call: identifier '(' expression ')'  ;
 *
 * The current token is the function's name, so a single token of lookahead
 * is all it takes to tell a call apart from a variable.
 */
AstNodePtr Parser::FuncCall(const std::vector<NodePtr>& token_list, idx_t& i)
{
    Symbol function = LookupTyped(token_list[i], StructuralType::kFunction,
                                  "function");

    Match(token_list, i, T_Identifier, "a function name");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr argument = BinaryExpression(token_list, i, 0);

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstFuncCall>(argument, function);
}

/*
 * Parse the parenthesised condition shared by if and while statements.
 */
AstNodePtr Parser::Condition(const std::vector<NodePtr>& token_list,
    idx_t& i,
    std::string_view statement)
{
    Match(token_list, i, T_LParen, "(");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);
    CheckComparison(condition, statement);

    Match(token_list, i, T_RParen, ")");

    return condition;
}

/*
 * The language has no truth values of its own yet, so every condition has
 * to be one of the six comparison operators.
 */
void Parser::CheckComparison(const AstNodePtr& condition,
    std::string_view statement)
{
    if (condition->GetAstNodeTag() != A_AstOperator ||
        !IsComparisonOperator(
            static_cast<const AstOperator *>(condition.get())->GetOpType()))
    {
        std::cerr << "syntax error: the condition of " << statement;
        std::cerr << " must be a comparison!" << std::endl;
        std::exit(1);
    }
}

AstNodePtr Parser::BinaryExpression(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    uint8 ptp) /* previous token precedence */
{
    AstNodePtr left = PrefixExpression(token_list, i);

    /*
     * Only a binary operator has a non-zero precedence, so the loop
     * stops as soon as a semicolon, a right parenthesis, an EOF or any
     * other token shows up.
     */
    while (GetOpPrecedence(TokenTag(token_list[i])) > ptp)
    {
        idx_t op_idx = i;
        uint8 op_prec = GetOpPrecedence(TokenTag(token_list[op_idx]));

        i++;

        AstNodePtr right = BinaryExpression(token_list, i, op_prec);

        WidenOperands(left, right);

        left = MakeOperatorNode(left, right, token_list[op_idx]);
    }

    return left;
}

/*
 * prefix_expression: primary
 *      |           '*' prefix_expression
 *      |           '&' prefix_expression
 *      ;
 *
 * The two operators only take the operands they can mean something for:
 * '&' a variable, and '*' a pointer which an identifier or another '*'
 * produced. Anything else is rejected rather than turned into a tree the
 * code generator cannot make sense of.
 */
AstNodePtr Parser::PrefixExpression(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    switch (TokenTag(token_list[i]))
    {
        case T_Amper:
        {
            i++;

            AstNodePtr operand = PrefixExpression(token_list, i);

            if (operand->GetAstNodeTag() != A_AstIdentifier)
            {
                std::cerr << "syntax error: & must be followed by a";
                std::cerr << " variable!" << std::endl;
                std::exit(1);
            }

            const AstIdentifier *ident =
                static_cast<const AstIdentifier *>(operand.get());

            return std::make_unique<AstAddress>(ident->GetSymbol());
        }
        case T_Star:
        {
            i++;

            AstNodePtr operand = PrefixExpression(token_list, i);
            AstNodeTag tag = operand->GetAstNodeTag();

            if (tag != A_AstIdentifier && tag != A_AstDeref)
            {
                std::cerr << "syntax error: * must be followed by a";
                std::cerr << " variable or another *!" << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstDeref>(operand,
                                              ValueAt(operand->GetType()));
        }
        default:
            return ParsePrimary(token_list, i);
    }
}

AstNodePtr Parser::ParsePrimary(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    switch (TokenTag(token_list[i]))
    {
        case T_IntLit:
        {
            const Literal<int, T_IntLit> *lit =
                static_cast<const Literal<int, T_IntLit>*>(token_list[i].get());
            int32 value = lit->GetValue();

            /*
             * A small literal is given the type char so that it can be
             * stored in a char variable; anything else needs the wider
             * type and will be rejected there.
             */
            PrimitiveType type = (value >= 0 && value < 256)
                                 ? PrimitiveType::kChar
                                 : PrimitiveType::kInt;

            const NodePtr& token = token_list[i];
            i++;

            return MakeIntLitLeaf(token, type);
        }
        case T_Identifier:
        {
            /* A '(' after the name turns this into a function call. */
            if (TokenTag(token_list[i + 1]) == T_LParen)
                return FuncCall(token_list, i);

            Symbol symbol = LookupTyped(token_list[i],
                                        StructuralType::kVariable,
                                        "variable");
            i++;

            return MakeIdentLeaf(symbol, false);
        }
        default:
            std::cerr << "syntax error: unexpected token ";
            std::cerr << NodeTagToString(TokenTag(token_list[i]));
            std::cerr << ", expect an expression!" << std::endl;
            std::exit(1);
    }
}

/*
 * Parse the type which starts a declaration, along with the '*' of every
 * pointer level, and step over all of it.
 */
PrimitiveType Parser::ParseType(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    PrimitiveType type;

    switch (TokenTag(token_list[i]))
    {
        case T_Char:
            type = PrimitiveType::kChar;
            break;
        case T_Int:
            type = PrimitiveType::kInt;
            break;
        case T_Long:
            type = PrimitiveType::kLong;
            break;
        case T_Void:
            type = PrimitiveType::kVoid;
            break;
        default:
            std::cerr << "syntax error: expect a type!" << std::endl;
            std::exit(1);
    }

    i++;

    while (TokenTag(token_list[i]) == T_Star)
    {
        type = PointerTo(type);
        i++;
    }

    return type;
}

Symbol Parser::LookupTyped(const NodePtr& token,
    StructuralType wanted,
    std::string_view what)
{
    const Identifier *ident = static_cast<const Identifier *>(token.get());

    std::optional<Symbol> symbol = symbol_table_.FindSymbol(ident->GetName());

    if (!symbol)
    {
        std::cerr << "syntax error: undeclared " << what << " ";
        std::cerr << ident->GetName() << "!" << std::endl;
        std::exit(1);
    }

    if (symbol->stype != wanted)
    {
        std::cerr << "syntax error: " << ident->GetName();
        std::cerr << " is not a " << what << "!" << std::endl;
        std::exit(1);
    }

    return *symbol;
}

void Parser::WidenOperands(AstNodePtr& left, AstNodePtr& right)
{
    TypeMatch match = MatchTypes(left->GetType(), right->GetType(), false);

    if (!match.compatible)
    {
        std::cerr << "syntax error: incompatible types!" << std::endl;
        std::exit(1);
    }

    /* Only one of the two ever needs widening. */
    if (match.widen_left)
        left = MakeWiden(left, right->GetType());

    if (match.widen_right)
        right = MakeWiden(right, left->GetType());
}

AstNodePtr Parser::MakeIntLitLeaf(const NodePtr& token, PrimitiveType type)
{
    const Literal<int, T_IntLit> *lit =
        static_cast<const Literal<int, T_IntLit>*>(token.get());

    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIntLit>(left, right, lit->GetValue(), type);
}

AstNodePtr Parser::MakeIdentLeaf(const Symbol& symbol, bool is_lv_ident)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIdentifier>(left, right, symbol, is_lv_ident);
}

AstNodePtr Parser::MakeOperatorNode(AstNodePtr& left,
    AstNodePtr& right,
    const NodePtr& node)
{
    PrimitiveType type = left->GetType();

    switch (node->GetNodeTag())
    {
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
                                                 node->GetNodeTag(), type);
        default:
            std::cerr << "code error: token ";
            std::cerr << NodeTagToString(node->GetNodeTag());
            std::cerr << " is not a binary operator!" << std::endl;
            std::exit(1);
    }
}

AstNodePtr Parser::MakeWiden(AstNodePtr& expression, PrimitiveType type)
{
    return std::make_unique<AstWiden>(expression, type);
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

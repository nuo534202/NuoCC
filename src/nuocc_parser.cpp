#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <algorithm>
#include <iostream>
#include <limits>

#include "nodes/nuocc_scanner_nodes.hpp"
#include "utils/nuocc_print.hpp"
#include "utils/nuocc_runtime.hpp"

namespace nuocc
{

Program Parser::Parse(const std::vector<NodePtr>& token_list)
{
    /*
     * printint() is provided by the runtime the generated code is linked
     * with, so it is already known before any of the program is parsed.
     */
    symbol_table_.AddSymbol(Symbol{.name = kPrintIntName,
                                   .type = PrimitiveType::kVoid,
                                   .stype = StructuralType::kFunction,
                                   .parameter_type = PrimitiveType::kInt});

    idx_t i = 0;
    Program program;

    while (TokenTag(token_list[i]) != T_EOF)
        GlobalDeclaration(token_list, i, program);

    if (program.functions.empty())
    {
        std::cerr << "syntax error: the program has no function!" << std::endl;
        std::exit(1);
    }

    return program;
}

/*
 * global_declaration: function_declaration | var_declaration  ;
 */
void Parser::GlobalDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    Program& program)
{
    PrimitiveType type = ParseType(token_list, i);
    std::string name = MatchIdentifier(token_list, i, "a name");

    if (TokenTag(token_list[i]) == T_LParen)
    {
        program.functions.push_back(
            FunctionDeclaration(token_list, i, type, name));
        return;
    }

    /* A '[' after the name declares an array of that many elements. */
    if (TokenTag(token_list[i]) == T_LBracket)
    {
        Symbol symbol = ArrayDeclaration(token_list, i, type, name,
                                         StorageClass::kGlobal);

        Match(token_list, i, T_Semicolon, ";");
        program.globals.push_back(std::make_unique<AstDeclare>(symbol));
        return;
    }

    IdentifierList(token_list, i, type, name, program.globals,
                   StorageClass::kGlobal);

    Match(token_list, i, T_Semicolon, ";");
}

/*
 * function_declaration: type identifier '(' opt_parameter ')'
 *                       compound_statement  ;
 */
AstNodePtr Parser::FunctionDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    PrimitiveType type,
    const std::string& name)
{
    Symbol symbol{.name = name,
                  .type = type,
                  .stype = StructuralType::kFunction};

    local_stack_size_ = 0;
    Match(token_list, i, T_LParen, "(");
    std::optional<Symbol> parameter = FunctionParameter(token_list, i, symbol);
    symbol_table_.AddSymbol(symbol);
    const idx_t symbol_mark = symbol_table_.Mark();
    if (parameter)
        symbol_table_.AddSymbol(*parameter);
    Match(token_list, i, T_RParen, ")");

    current_function_type_ = type;
    AstNodePtr body = CompoundStatement(token_list, i);

    current_function_type_ = PrimitiveType::kNone;
    const int32 local_size = local_stack_size_;
    symbol_table_.Restore(symbol_mark);

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

    return std::make_unique<AstFunction>(body, symbol, local_size, parameter);
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

        ParsedStatement statement = Statement(token_list, i);

        /* A statement which needs one is terminated by a semicolon. */
        if (statement.semicolon)
            Match(token_list, i, T_Semicolon, ";");

        /* Glue each statement onto the statements parsed before it. */
        if (!left)
        {
            left = std::move(statement.tree);
        }
        else
        {
            AstNodePtr glued = std::make_unique<AstGlue>(left, statement.tree);
            left = std::move(glued);
        }
    }

    i++;    /* step over the right brace */

    return left;
}

/*
 * statement: print_statement
 *      |     declaration
 *      |     expression_statement
 *      |     if_statement
 *      |     while_statement
 *      |     for_statement
 *      |     return_statement
 *      ;
 *
 * An assignment is an expression and not a statement of its own, because
 * '=' is a binary operator: `x= 1;` is the expression `x= 1` followed by
 * the semicolon every expression statement ends with.
 */
ParsedStatement Parser::Statement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    switch (TokenTag(token_list[i]))
    {
        case T_Print:
            return {PrintStatement(token_list, i), true};
        case T_Int:
        case T_Char:
        case T_Long:
            return {DeclareStatement(token_list, i), true};
        case T_If:
            return {IfStatement(token_list, i), false};
        case T_While:
            return {WhileStatement(token_list, i), false};
        case T_For:
            return {ForStatement(token_list, i), false};
        case T_Return:
            return {ReturnStatement(token_list, i), true};
        default:
            /* This covers an assignment, a call and any other expression. */
            return {BinaryExpression(token_list, i, 0), true};
    }
}

/* print_statement: 'print' expression ';'  ; */
AstNodePtr Parser::PrintStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Print, "print");

    AstNodePtr expression = BinaryExpression(token_list, i, 0);

    /* printint() always takes an int, so widen what it is given. */
    if (!ModifyType(expression, PrimitiveType::kInt, std::nullopt))
    {
        std::cerr << "syntax error: this value cannot be printed!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return std::make_unique<AstPrint>(expression);
}

/* declaration: type identifier_list ';' | type array_declaration ';'  ; */
AstNodePtr Parser::DeclareStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    PrimitiveType type = ParseType(token_list, i);

    std::string name = MatchIdentifier(token_list, i, "a variable name");

    if (TokenTag(token_list[i]) == T_LBracket)
    {
        Symbol symbol = ArrayDeclaration(token_list, i, type, name,
                                         StorageClass::kLocal);

        return std::make_unique<AstDeclare>(symbol);
    }

    /* The declaration may name more than one variable of that type. */
    std::vector<AstNodePtr> declarations;
    IdentifierList(token_list, i, type, name, declarations,
                   StorageClass::kLocal);

    return GlueStatements(declarations);
}

/*
 * identifier_list: identifier | identifier ',' identifier_list  ;
 */
void Parser::IdentifierList(const std::vector<NodePtr>& token_list,
    idx_t& i,
    PrimitiveType type,
    const std::string& name,
    std::vector<AstNodePtr>& declarations,
    StorageClass storage)
{
    std::string current = name;

    while (true)
    {
        Symbol symbol{.name = current,
                      .type = type,
                      .stype = StructuralType::kVariable,
                      .storage = storage,
                      .stack_offset = storage == StorageClass::kLocal
                                          ? AllocateLocal(type)
                                          : 0};

        symbol_table_.AddSymbol(symbol);

        declarations.push_back(std::make_unique<AstDeclare>(symbol));

        /* A ',' continues the list with another name of the same type. */
        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;
        current = MatchIdentifier(token_list, i, "a variable name");
    }
}

/*
 * array_declaration: identifier '[' number ']'  ;
 *
 * Unlike a list of variables, an array declares one name only: the number
 * is how many elements of the declared type it holds, and the room for all
 * of them is reserved together so that the elements stand next to each
 * other. The size is fixed at the declaration and cannot be changed.
 */
Symbol Parser::ArrayDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    PrimitiveType element_type,
    const std::string& name,
    StorageClass storage)
{
    Match(token_list, i, T_LBracket, "[");

    if (TokenTag(token_list[i]) != T_IntLit)
    {
        std::cerr << "syntax error: expect an array size!" << std::endl;
        std::exit(1);
    }

    const Literal<int, T_IntLit> *lit =
        static_cast<const Literal<int, T_IntLit>*>(token_list[i].get());
    const int32 element_count = lit->GetValue();
    i++;

    if (element_count <= 0)
    {
        std::cerr << "syntax error: an array must have at least one element!";
        std::cerr << std::endl;
        std::exit(1);
    }

    const int32 element_size = PrimitiveSize(element_type);

    if (element_size == 0)
    {
        std::cerr << "syntax error: an array needs a value type!";
        std::cerr << std::endl;
        std::exit(1);
    }

    /*
     * An array is measured from its first element, so indexing it asks for
     * the address of that element: a pointer to the element type. That
     * pointer does not exist yet when the elements are pointers themselves,
     * the same way an int pointer to a pointer is refused.
     */
    if (IsPointerType(element_type))
    {
        std::cerr << "Error: there is no pointer to this type yet!";
        std::cerr << std::endl;
        std::exit(1);
    }

    /* The room for the whole array has to be countable in bytes. */
    if (element_count > std::numeric_limits<int32>::max() / element_size)
    {
        std::cerr << "syntax error: the array " << name << " is too large!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_RBracket, "]");

    Symbol symbol{.name = name,
                  .type = element_type,
                  .stype = StructuralType::kArray,
                  .storage = storage,
                  .stack_offset = storage == StorageClass::kLocal
                                      ? AllocateLocal(element_size * element_count,
                                                      std::min<int32>(element_size, 8))
                                      : 0,
                  .element_count = element_count};

    symbol_table_.AddSymbol(symbol);

    return symbol;
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

    AstNodePtr condition = Condition(token_list, i);
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

    AstNodePtr condition = Condition(token_list, i);
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

    ParsedStatement preop = Statement(token_list, i);
    Match(token_list, i, T_Semicolon, ";");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);
    MakeCondition(condition);

    Match(token_list, i, T_Semicolon, ";");
    ParsedStatement postop = Statement(token_list, i);
    Match(token_list, i, T_RParen, ")");

    AstNodePtr body = CompoundStatement(token_list, i);

    /* The post statement runs at the end of each pass of the loop. */
    AstNodePtr loop_body = std::make_unique<AstGlue>(body, postop.tree);
    AstNodePtr loop = std::make_unique<AstWhile>(condition, loop_body);

    return std::make_unique<AstGlue>(preop.tree, loop);
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
    if (!ModifyType(expression, current_function_type_, std::nullopt))
    {
        std::cerr << "syntax error: this value does not fit the return type";
        std::cerr << " of the function!" << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstReturn>(expression);
}

/*
 * function_call: identifier '(' opt_expression ')'  ;
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

    AstNodePtr argument = nullptr;
    if (TokenTag(token_list[i]) != T_RParen)
        argument = BinaryExpression(token_list, i, 0);

    if (function.parameter_type == PrimitiveType::kNone)
    {
        if (argument)
        {
            std::cerr << "syntax error: function " << function.name;
            std::cerr << " does not take an argument!" << std::endl;
            std::exit(1);
        }
    }
    else
    {
        if (!argument || !ModifyType(argument, function.parameter_type,
                                     std::nullopt))
        {
            std::cerr << "syntax error: invalid argument for function ";
            std::cerr << function.name << "!" << std::endl;
            std::exit(1);
        }
    }

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstFuncCall>(argument, function);
}

std::optional<Symbol> Parser::FunctionParameter(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    Symbol& function)
{
    if (TokenTag(token_list[i]) == T_RParen)
        return std::nullopt;

    PrimitiveType type = ParseType(token_list, i);
    if (type == PrimitiveType::kVoid)
    {
        std::cerr << "syntax error: a parameter needs a value type!";
        std::cerr << std::endl;
        std::exit(1);
    }

    std::string name = MatchIdentifier(token_list, i, "a parameter name");
    function.parameter_type = type;

    Symbol parameter{.name = name,
                     .type = type,
                     .stype = StructuralType::kVariable,
                     .storage = StorageClass::kLocal,
                     .stack_offset = AllocateLocal(type)};
    return parameter;
}

/*
 * Parse the parenthesised condition shared by if and while statements.
 */
AstNodePtr Parser::Condition(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_LParen, "(");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);
    MakeCondition(condition);

    Match(token_list, i, T_RParen, ")");

    return condition;
}

/*
 * A condition has to be either false or true. A comparison already is, so
 * it is left alone; anything else is an integer, and an integer is false
 * when it is zero and true otherwise, so it is converted into a boolean.
 */
void Parser::MakeCondition(AstNodePtr& expression)
{
    if (expression->GetAstNodeTag() == A_AstOperator &&
        IsComparisonOperator(
            static_cast<const AstOperator *>(expression.get())->GetOpType()))
        return;

    expression = std::make_unique<AstUnary>(expression,
                                            A_AstToBool,
                                            PrimitiveType::kInt);
}

AstNodePtr Parser::BinaryExpression(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    uint8 ptp) /* previous token precedence */
{
    AstNodePtr left = PrefixExpression(token_list, i);

    /*
     * Only a binary operator has a non-zero precedence, so the loop stops
     * as soon as a semicolon, a right parenthesis, an EOF or any other
     * token shows up. A right associative operator also carries on when
     * its precedence only matches, which is what makes `a= b= 3` store
     * into b first instead of into a.
     */
    while (GetOpPrecedence(TokenTag(token_list[i])) > ptp ||
           (IsRightAssociative(TokenTag(token_list[i])) &&
            GetOpPrecedence(TokenTag(token_list[i])) == ptp))
    {
        idx_t op_idx = i;
        uint8 op_prec = GetOpPrecedence(TokenTag(token_list[op_idx]));

        i++;

        AstNodePtr right = BinaryExpression(token_list, i, op_prec);
        NodeTag op_type = TokenTag(token_list[op_idx]);

        if (op_type == T_Assign)
        {
            /*
             * An assignment is the one operator whose two operands are not
             * made to agree with each other: the target keeps the type it
             * was declared with, and it is the value which has to fit it.
             *
             * The two are then switched around, so that the value ends up
             * in the left child and the target in the right one. That is
             * the order the code generator wants: the value has to be
             * worked out before the location it is stored in.
             */
            if (!IsLvalue(left->GetAstNodeTag()))
            {
                std::cerr << "syntax error: an assignment can only store";
                std::cerr << " into a variable or into a * pointer!";
                std::cerr << std::endl;
                std::exit(1);
            }

            if (!ModifyType(right, left->GetType(), std::nullopt))
            {
                std::cerr << "syntax error: this value does not fit the";
                std::cerr << " target of the assignment!" << std::endl;
                std::exit(1);
            }

            std::swap(left, right);
        }
        else
        {
            /*
             * Try to make each operand fit the type of the other one. One
             * of them may have to be widened or scaled, which also means
             * the other one cannot be made to fit, so the two only clash
             * when neither of them can.
             */
            PrimitiveType left_type = left->GetType();
            PrimitiveType right_type = right->GetType();

            bool left_fits = ModifyType(left, right_type, op_type);
            bool right_fits = ModifyType(right, left_type, op_type);

            if (!left_fits && !right_fits)
            {
                std::cerr << "syntax error: incompatible types!" << std::endl;
                std::exit(1);
            }
        }

        /*
         * '&&' and '||' answer a question about their operands and not
         * about their bits, so each side becomes the zero or one which
         * says whether it holds: the bitwise operations which follow then
         * give the right answer, as those are the only values left.
         */
        if (op_type == T_LogAnd || op_type == T_LogOr)
        {
            left = std::make_unique<AstUnary>(left, A_AstToBool,
                                              PrimitiveType::kInt);
            right = std::make_unique<AstUnary>(right, A_AstToBool,
                                               PrimitiveType::kInt);
        }

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
 * '&' a name or a dereference, which both name a location, and '*' any
 * value which holds an address. Anything else is rejected rather than
 * turned into a tree the code generator cannot make sense of.
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

            /*
             * A '*' and a '&' written one after the other are two ways of
             * saying the same thing twice, so they undo each other: the
             * address of what a '*' reads is the address it was given,
             * which is also how '&' takes the address of an array element.
             */
            if (operand->GetAstNodeTag() == A_AstDeref)
                return operand->ReleaseLeft();

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

            /*
             * What is read through is asked for by its type and not by the
             * tree it came in, so that a parenthesised expression which
             * holds an address, such as '*(ptr + 2)', is read as well.
             */
            if (!IsPointerType(operand->GetType()))
            {
                std::cerr << "syntax error: * needs a pointer value!";
                std::cerr << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstDeref>(operand,
                                              ValueAt(operand->GetType()));
        }
        case T_Minus:
        {
            i++;

            AstNodePtr operand = PrefixExpression(token_list, i);

            /*
             * A char is unsigned, so there is no such thing as a negative
             * one: the value is widened to an int before the sign flips.
             */
            if (!ModifyType(operand, PrimitiveType::kInt, std::nullopt))
            {
                std::cerr << "syntax error: - needs a number!";
                std::cerr << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstUnary>(operand,
                                              A_AstNegate,
                                              operand->GetType());
        }
        case T_Invert:
        {
            i++;

            AstNodePtr operand = PrefixExpression(token_list, i);

            /* Only the bits of an integer can be flipped. */
            if (!IsIntType(operand->GetType()))
            {
                std::cerr << "syntax error: ~ needs an integer value!";
                std::cerr << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstUnary>(operand,
                                              A_AstInvert,
                                              operand->GetType());
        }
        case T_LogNot:
        {
            i++;

            AstNodePtr operand = PrefixExpression(token_list, i);

            /* Only an integer can be asked whether it is zero. */
            if (!IsIntType(operand->GetType()))
            {
                std::cerr << "syntax error: ! needs an integer value!";
                std::cerr << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstUnary>(operand,
                                              A_AstLogNot,
                                              PrimitiveType::kInt);
        }
        case T_Inc:
        case T_Dec:
        {
            bool increment = TokenTag(token_list[i]) == T_Inc;
            i++;

            /*
             * The value has somewhere to go back to, so an increment or a
             * decrement needs a variable and not an expression.
             */
            Symbol symbol = LookupTyped(token_list[i],
                                        StructuralType::kVariable,
                                        "variable");
            i++;

            return std::make_unique<AstIncDec>(symbol, increment ? 1 : -1,
                                               false);
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
        case T_LParen:
        {
            i++;

            AstNodePtr expression = BinaryExpression(token_list, i, 0);

            Match(token_list, i, T_RParen, ")");

            return expression;
        }
        case T_Identifier:
        {
            /* A '(' after the name turns this into a function call. */
            if (TokenTag(token_list[i + 1]) == T_LParen)
                return FuncCall(token_list, i);

            /* A '[' after the name indexes an array or a pointer. */
            if (TokenTag(token_list[i + 1]) == T_LBracket)
                return ArrayAccess(token_list, i);

            Symbol symbol = Lookup(token_list[i], "variable");
            i++;

            /*
             * An array named on its own stands for the address of its
             * first element, which is what lets one be given wherever a
             * pointer is wanted. The address of an array is fixed, so it
             * cannot be the thing an increment or a decrement changes.
             */
            if (symbol.stype == StructuralType::kArray)
            {
                if (TokenTag(token_list[i]) == T_Inc ||
                    TokenTag(token_list[i]) == T_Dec)
                {
                    std::cerr << "syntax error: an array cannot be changed!";
                    std::cerr << std::endl;
                    std::exit(1);
                }

                return std::make_unique<AstAddress>(symbol);
            }

            CheckKind(symbol, StructuralType::kVariable, "variable");

            /*
             * A '++' or a '--' after the name makes this a postfix
             * operator, which yields the value held before the change.
             */
            if (TokenTag(token_list[i]) == T_Inc ||
                TokenTag(token_list[i]) == T_Dec)
            {
                bool increment = TokenTag(token_list[i]) == T_Inc;
                i++;

                return std::make_unique<AstIncDec>(symbol,
                                                   increment ? 1 : -1,
                                                   true);
            }

            return MakeIdentLeaf(symbol);
        }
        default:
            std::cerr << "syntax error: unexpected token ";
            std::cerr << NodeTagToString(TokenTag(token_list[i]));
            std::cerr << ", expect an expression!" << std::endl;
            std::exit(1);
    }
}

/*
 * array_access: identifier '[' expression ']'  ;
 *
 * The name has already been told apart from a call and from a plain read
 * of the name by the caller, which saw the '[' and came here.
 */
AstNodePtr Parser::ArrayAccess(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Symbol symbol = Lookup(token_list[i], "variable");

    /*
     * The base the index is measured from is the address of an array, or
     * the address a pointer holds when it is a pointer which is indexed:
     * both start a row of values of one type. Anything else has no row to
     * walk along.
     */
    AstNodePtr base;

    if (symbol.stype == StructuralType::kArray)
    {
        base = std::make_unique<AstAddress>(symbol);
    }
    else if (symbol.stype == StructuralType::kVariable &&
             IsPointerType(symbol.type))
    {
        base = MakeIdentLeaf(symbol);
    }
    else
    {
        std::cerr << "syntax error: " << symbol.name;
        std::cerr << " cannot be indexed!" << std::endl;
        std::exit(1);
    }

    i++;    /* step over the name */
    Match(token_list, i, T_LBracket, "[");

    AstNodePtr index = BinaryExpression(token_list, i, 0);

    Match(token_list, i, T_RBracket, "]");

    const PrimitiveType base_type = base->GetType();

    /*
     * The index counts elements and not bytes, so it becomes an offset:
     * element six of an int array starts 24 bytes along, which is what
     * scaling it by the size of one element works out. Only an integer
     * counts elements, so anything else is refused here rather than
     * scaled into an offset which would not mean anything.
     */
    if (!ModifyType(index, base_type, T_Plus))
    {
        std::cerr << "syntax error: an array index must be an integer!";
        std::cerr << std::endl;
        std::exit(1);
    }

    AstNodePtr address = std::make_unique<AstOperator>(base, index, T_Plus,
                                                       base_type);

    return std::make_unique<AstDeref>(address, ValueAt(base_type));
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

Symbol Parser::Lookup(const NodePtr& token, std::string_view what)
{
    /*
     * Only a name can be looked up, so a literal or an operator standing
     * where one is expected is refused rather than read as a name.
     */
    if (token->GetNodeTag() != T_Identifier)
    {
        std::cerr << "syntax error: expect a " << what << " name!";
        std::cerr << std::endl;
        std::exit(1);
    }

    const Identifier *ident = static_cast<const Identifier *>(token.get());

    std::optional<Symbol> symbol = symbol_table_.FindSymbol(ident->GetName());

    if (!symbol)
    {
        std::cerr << "syntax error: undeclared " << what << " ";
        std::cerr << ident->GetName() << "!" << std::endl;
        std::exit(1);
    }

    return *symbol;
}

void Parser::CheckKind(const Symbol& symbol,
    StructuralType wanted,
    std::string_view what) const
{
    if (symbol.stype == wanted)
        return;

    std::cerr << "syntax error: " << symbol.name;
    std::cerr << " is not a " << what << "!" << std::endl;
    std::exit(1);
}

Symbol Parser::LookupTyped(const NodePtr& token,
    StructuralType wanted,
    std::string_view what)
{
    Symbol symbol = Lookup(token, what);

    CheckKind(symbol, wanted, what);

    return symbol;
}

AstNodePtr Parser::MakeIntLitLeaf(const NodePtr& token, PrimitiveType type)
{
    const Literal<int, T_IntLit> *lit =
        static_cast<const Literal<int, T_IntLit>*>(token.get());

    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIntLit>(left, right, lit->GetValue(), type);
}

AstNodePtr Parser::MakeIdentLeaf(const Symbol& symbol)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIdentifier>(left, right, symbol);
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
        case T_LShift:
        case T_RShift:
        case T_Or:
        case T_Xor:
        case T_Amper:
        case T_LogAnd:
        case T_LogOr:
        case T_Assign:
            return std::make_unique<AstOperator>(left, right,
                                                 node->GetNodeTag(), type);
        default:
            std::cerr << "code error: token ";
            std::cerr << NodeTagToString(node->GetNodeTag());
            std::cerr << " is not a binary operator!" << std::endl;
            std::exit(1);
    }
}

AstNodePtr Parser::GlueStatements(std::vector<AstNodePtr>& statements)
{
    AstNodePtr tree = nullptr;

    for (AstNodePtr& statement : statements)
    {
        if (!tree)
        {
            tree = std::move(statement);
            continue;
        }

        AstNodePtr glued = std::make_unique<AstGlue>(tree, statement);
        tree = std::move(glued);
    }

    return tree;
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

std::string Parser::MatchIdentifier(const std::vector<NodePtr>& token_list,
    idx_t& i,
    std::string_view what)
{
    if (TokenTag(token_list[i]) != T_Identifier)
    {
        std::cerr << "syntax error: expect " << what << "!" << std::endl;
        std::exit(1);
    }

    const Identifier *ident =
        static_cast<const Identifier *>(token_list[i].get());

    std::string name = ident->GetName();

    i++;

    return name;
}

bool Parser::IsRightAssociative(NodeTag tag)
{
    return tag == T_Assign;
}

bool Parser::IsLvalue(AstNodeTag tag)
{
    switch (tag)
    {
        case A_AstIdentifier:
        case A_AstDeref:
            return true;
        default:
            return false;
    }
}

uint8 Parser::GetOpPrecedence(NodeTag tag)
{
    auto it = kOpPrecedence.find(tag);

    /* Anything which is not a binary operator has zero precedence. */
    if (it == kOpPrecedence.end())
        return 0;

    return it->second;
}

int32 Parser::AllocateLocal(PrimitiveType type)
{
    const int32 size = PrimitiveSize(type);
    if (size == 0)
    {
        std::cerr << "syntax error: a variable needs a value type!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return AllocateLocal(size, std::min<int32>(size, 8));
}

int32 Parser::AllocateLocal(int32 size, int32 alignment)
{
    const int32 remainder = local_stack_size_ % alignment;

    if (remainder != 0)
        local_stack_size_ += alignment - remainder;

    const int32 offset = local_stack_size_;
    local_stack_size_ += size;
    return offset;
}

/*
 * Operator precedence, following the C language. The values themselves are
 * meaningless, only their relative order matters: a higher value binds
 * more tightly. '=' binds the least tightly of all, so that everything on
 * its right is stored and not compared or added.
 */
const std::unordered_map<NodeTag, uint8> Parser::kOpPrecedence = {
    {T_Assign, 10},

    {T_LogOr, 20},

    {T_LogAnd, 30},

    {T_Or, 40},

    {T_Xor, 50},

    {T_Amper, 60},

    {T_EQ, 70}, {T_NE, 70},

    {T_LT, 80}, {T_GT, 80}, {T_LE, 80}, {T_GE, 80},

    {T_LShift, 90}, {T_RShift, 90},

    {T_Plus, 100}, {T_Minus, 100},

    {T_Star, 110}, {T_Slash, 110}
};

}   /* namespace nuocc */

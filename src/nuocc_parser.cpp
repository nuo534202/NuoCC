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
     * printint() and printchar() are provided by the runtime the
     * generated code is linked with, so they are already known before
     * any of the program is parsed.
     */
    const TypeRef int_type{.type = PrimitiveType::kInt};
    symbol_table_.AddSymbol(Symbol{.name = kPrintIntName,
                                   .type = PrimitiveType::kVoid,
                                   .stype = StructuralType::kFunction,
                                   .parameter_types = {int_type}});
    symbol_table_.AddSymbol(Symbol{.name = kPrintCharName,
                                   .type = PrimitiveType::kVoid,
                                   .stype = StructuralType::kFunction,
                                   .parameter_types = {int_type}});

    idx_t i = 0;
    Program program;

    while (TokenTag(token_list[i]) != T_EOF)
        GlobalDeclaration(token_list, i, program);

    if (program.functions.empty())
    {
        std::cerr << "syntax error: the program has no function!" << std::endl;
        std::exit(1);
    }

    program.types = std::move(types_);

    return program;
}

/*
 * global_declaration: function_declaration | var_declaration  ;
 */
void Parser::GlobalDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    Program& program)
{
    TypeRef type = ParseType(token_list, i);

    /*
     * A composite definition which no variable follows declares the type
     * and nothing else, as `struct fred { int x; };` does.
     */
    if (TokenTag(token_list[i]) == T_Semicolon &&
        type.type == PrimitiveType::kComposite)
    {
        Match(token_list, i, T_Semicolon, ";");
        return;
    }

    std::string name = MatchIdentifier(token_list, i, "a name");

    if (TokenTag(token_list[i]) == T_LParen)
    {
        /* A prototype declares no body, so it adds no function. */
        AstNodePtr function = FunctionDeclaration(token_list, i, type, name);

        if (function)
            program.functions.push_back(std::move(function));

        return;
    }

    /* A '[' after the name declares an array of that many elements. */
    if (TokenTag(token_list[i]) == T_LBracket)
    {
        Symbol symbol = ArrayDeclaration(token_list, i, type, name,
                                         StorageClass::kGlobal);
        symbol_table_.AddSymbol(symbol);

        Match(token_list, i, T_Semicolon, ";");
        program.globals.push_back(std::make_unique<AstDeclare>(symbol));
        return;
    }

    IdentifierList(token_list, i, type, name, program.globals,
                   StorageClass::kGlobal);

    Match(token_list, i, T_Semicolon, ";");
}

/*
 * function_declaration: type identifier '(' opt_parameter_list ')'
 *      |                type identifier '(' opt_parameter_list ')' ';'
 *      ;
 *
 * A declaration which ends in a semicolon is a prototype: it says what the
 * function is called and what it takes, and its body is given later, or
 * somewhere else altogether. Everything a prototype says is kept with the
 * function, so that the definition which follows has to agree with it.
 */
AstNodePtr Parser::FunctionDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    const TypeRef& type,
    const std::string& name)
{
    if (type.type == PrimitiveType::kComposite) {
        std::cerr << "syntax error: returning a composite value"
                  << " is not supported yet!" << std::endl;
        std::exit(1);
    }

    /* The name may already be known as a prototype, or as a function
     * which has been defined; a later declaration has to agree with what
     * is known, and a second body is refused. */
    std::optional<Symbol> previous = symbol_table_.FindSymbol(name);

    if (previous && previous->stype != StructuralType::kFunction)
    {
        std::cerr << "syntax error: " << name << " is not a function!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_LParen, "(");
    std::vector<Symbol> parameters = FunctionParameters(token_list, i);
    Match(token_list, i, T_RParen, ")");

    Symbol symbol{.name = name,
                  .type = type.type,
                  .composite = type.composite,
                  .stype = StructuralType::kFunction};

    for (const Symbol& parameter : parameters)
        symbol.parameter_types.push_back(
            TypeRef{.type = parameter.type, .composite = parameter.composite});

    if (previous)
        CheckPrototype(*previous, symbol);

    /* The token after the parameter list says whether a body follows. */
    const bool has_body = TokenTag(token_list[i]) == T_LBrace;

    if (previous && previous->has_body && has_body)
    {
        std::cerr << "syntax error: function " << name;
        std::cerr << " is already defined!" << std::endl;
        std::exit(1);
    }

    /* A definition has a body, and a prototype which follows one does not
     * take it away. */
    symbol.has_body = has_body || (previous.has_value() && previous->has_body);

    /*
     * The function is known before its body is read, so that the body can
     * call the function itself. A prototype which was seen earlier is
     * replaced, and the name is new only when no declaration came first.
     */
    if (previous)
        symbol_table_.UpdateSymbol(symbol);
    else
        symbol_table_.AddSymbol(symbol);

    /* A declaration which ends in a semicolon declares no body, so there
     * is no function to generate code for. */
    if (!has_body)
    {
        Match(token_list, i, T_Semicolon, ";");
        return nullptr;
    }

    const idx_t symbol_mark = symbol_table_.Mark();

    local_stack_size_ = 0;
    for (Symbol& parameter : parameters)
    {
        parameter.stack_offset = AllocateLocal(
            TypeRef{.type = parameter.type, .composite = parameter.composite});
        symbol_table_.AddSymbol(parameter);
    }

    current_function_type_ = type;
    AstNodePtr body = CompoundStatement(token_list, i);

    current_function_type_ = {};
    const int32 local_size = local_stack_size_;
    symbol_table_.Restore(symbol_mark);

    /*
     * A function which returns a value must end with a return statement, as
     * that is the only way its caller can be given a result.
     */
    if (type.type != PrimitiveType::kVoid)
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

    return std::make_unique<AstFunction>(body, symbol, local_size, parameters);
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
        case T_Struct:
        case T_Union:
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
    TypeRef type = ParseType(token_list, i);

    /* A composite definition which no variable follows declares the type
     * and nothing else; the semicolon is left to the caller. */
    if (TokenTag(token_list[i]) == T_Semicolon &&
        type.type == PrimitiveType::kComposite)
        return nullptr;

    std::string name = MatchIdentifier(token_list, i, "a variable name");

    if (TokenTag(token_list[i]) == T_LBracket)
    {
        Symbol symbol = ArrayDeclaration(token_list, i, type, name,
                                         StorageClass::kLocal);
        symbol_table_.AddSymbol(symbol);

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
    const TypeRef& type,
    const std::string& name,
    std::vector<AstNodePtr>& declarations,
    StorageClass storage)
{
    std::string current = name;

    while (true)
    {
        Symbol symbol{.name = current,
                      .type = type.type,
                      .composite = type.composite,
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
    const TypeRef& element_type,
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

    /*
     * Every element of an array is reached by counting its size along from
     * the first one, and the room a struct takes is only known from its
     * definition, which an index does not carry yet.
     */
    if (element_type.type == PrimitiveType::kComposite)
    {
        std::cerr << "syntax error: an array of composite values is not supported";
        std::cerr << " yet!" << std::endl;
        std::exit(1);
    }

    const int32 element_size = ValueSize(element_type.type,
                                         element_type.composite);

    if (element_size == 0)
    {
        std::cerr << "syntax error: an array needs a value type!";
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

    const int32 element_alignment = ValueAlignment(element_type.type,
                                                   element_type.composite);

    Symbol symbol{.name = name,
                  .type = element_type.type,
                  .composite = element_type.composite,
                  .stype = StructuralType::kArray,
                  .storage = storage,
                  .stack_offset = storage == StorageClass::kLocal
                                      ? AllocateLocal(element_size * element_count,
                                                      element_alignment)
                                      : 0,
                  .element_count = element_count};

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
    if (current_function_type_.type == PrimitiveType::kVoid)
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
 * function_call: identifier '(' opt_expression_list ')'  ;
 *
 * The current token is the function's name, so a single token of lookahead
 * is all it takes to tell a call apart from a variable. Every argument is
 * an expression of its own, and each one has to fit the type of the
 * parameter it is given to.
 */
AstNodePtr Parser::FuncCall(const std::vector<NodePtr>& token_list, idx_t& i)
{
    Symbol function = LookupTyped(token_list[i], StructuralType::kFunction,
                                  "function");

    Match(token_list, i, T_Identifier, "a function name");
    Match(token_list, i, T_LParen, "(");

    std::vector<AstNodePtr> arguments;

    if (TokenTag(token_list[i]) != T_RParen)
    {
        while (true)
        {
            arguments.push_back(BinaryExpression(token_list, i, 0));

            if (TokenTag(token_list[i]) != T_Comma)
                break;

            i++;
        }
    }

    if (arguments.size() != function.parameter_types.size())
    {
        std::cerr << "syntax error: wrong number of arguments for function ";
        std::cerr << function.name << ": " << arguments.size();
        std::cerr << " given, " << function.parameter_types.size();
        std::cerr << " wanted!" << std::endl;
        std::exit(1);
    }

    for (idx_t k = 0; k < arguments.size(); k++)
    {
        if (!ModifyType(arguments[k], function.parameter_types[k],
                        std::nullopt))
        {
            std::cerr << "syntax error: argument " << (k + 1) << " of ";
            std::cerr << function.name << " does not fit its parameter";
            std::cerr << " type!" << std::endl;
            std::exit(1);
        }
    }

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstFuncCall>(arguments, function);
}

/*
 * opt_parameter_list: <empty>
 *      |              parameter_list
 *      ;
 *
 * parameter_list: type identifier
 *      |        type identifier ',' parameter_list
 *      ;
 *
 * The type and the name of every parameter are read here. Nothing is added
 * to the symbol table yet, because a declaration which turns out to be a
 * prototype keeps only the types of its parameters; a definition binds
 * them once its body is known to follow.
 */
std::vector<Symbol> Parser::FunctionParameters(
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    std::vector<Symbol> parameters;

    if (TokenTag(token_list[i]) == T_RParen)
        return parameters;

    while (true)
    {
        if (parameters.size() == kMaxArguments)
        {
            std::cerr << "syntax error: a function takes at most ";
            std::cerr << kMaxArguments << " parameters!" << std::endl;
            std::exit(1);
        }

        TypeRef type = ParseType(token_list, i);
        if (type.type == PrimitiveType::kComposite) {
            std::cerr << "syntax error: passing a composite value"
                      << " is not supported yet!" << std::endl;
            std::exit(1);
        }
        if (type.type == PrimitiveType::kVoid)
        {
            std::cerr << "syntax error: a parameter needs a value type!";
            std::cerr << std::endl;
            std::exit(1);
        }

        std::string name = MatchIdentifier(token_list, i, "a parameter name");

        parameters.push_back(Symbol{.name = name,
                                    .type = type.type,
                                    .composite = type.composite,
                                    .stype = StructuralType::kVariable,
                                    .storage = StorageClass::kLocal});

        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;
    }

    return parameters;
}

/*
 * Every declaration of a function has to agree with the one which came
 * before it: the same return type, the same number of parameters and the
 * same type for each of them in turn, since a caller which was checked
 * against one of them has to keep working with the other.
 */
void Parser::CheckPrototype(const Symbol& previous, const Symbol& symbol)
{
    if (previous.type != symbol.type || previous.composite != symbol.composite)
    {
        std::cerr << "syntax error: return type of function ";
        std::cerr << symbol.name << " does not match its prototype!";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (previous.parameter_types.size() != symbol.parameter_types.size())
    {
        std::cerr << "syntax error: wrong number of parameters for ";
        std::cerr << symbol.name << ": ";
        std::cerr << symbol.parameter_types.size() << " given, ";
        std::cerr << previous.parameter_types.size() << " wanted!";
        std::cerr << std::endl;
        std::exit(1);
    }

    for (idx_t k = 0; k < symbol.parameter_types.size(); k++)
    {
        if (previous.parameter_types[k] != symbol.parameter_types[k])
        {
            std::cerr << "syntax error: parameter " << (k + 1) << " of ";
            std::cerr << symbol.name << " does not match its prototype!";
            std::cerr << std::endl;
            std::exit(1);
        }
    }
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
            const bool array_member = left->GetAstNodeTag() == A_AstMember &&
                static_cast<const AstMember&>(*left).GetMember().stype ==
                    StructuralType::kArray;
            if (!IsLvalue(left->GetAstNodeTag()) || array_member)
            {
                std::cerr << "syntax error: an assignment can only store";
                std::cerr << " into a variable or into a * pointer!";
                std::cerr << std::endl;
                std::exit(1);
            }

            if (!ModifyType(right, TypeRef{.type = left->GetType(),
                                           .composite = left->GetComposite()},
                            std::nullopt))
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

            if (operand->GetAstNodeTag() == A_AstMember) {
                const auto& member = static_cast<const AstMember&>(*operand);
                if (member.GetMember().stype == StructuralType::kArray) {
                    std::cerr << "syntax error: the address of a whole array"
                              << " is not supported yet!" << std::endl;
                    std::exit(1);
                }
                return std::make_unique<AstAddressExpr>(operand);
            }

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
            AstNodePtr operand = PostfixExpression(token_list, i);
            if (operand->GetAstNodeTag() != A_AstIdentifier) {
                std::cerr << "syntax error: increment and decrement need a"
                          << " standalone variable!" << std::endl;
                std::exit(1);
            }
            const Symbol& symbol =
                static_cast<const AstIdentifier&>(*operand).GetSymbol();
            if (BaseType(symbol.type) == PrimitiveType::kComposite) {
                std::cerr << "syntax error: increment and decrement of composite"
                          << " values or pointers to them are not supported yet!"
                          << std::endl;
                std::exit(1);
            }

            return std::make_unique<AstIncDec>(symbol, increment ? 1 : -1,
                                               false);
        }
        default:
            return PostfixExpression(token_list, i);
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
        case T_StrLit:
        {
            std::string text =
                static_cast<const Literal<std::string, T_StrLit>*>(
                    token_list[i].get())
                    ->GetValue();
            i++;

            return std::make_unique<AstStrLit>(text);
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
                if (BaseType(symbol.type) == PrimitiveType::kComposite) {
                    std::cerr << "syntax error: increment and decrement of"
                              << " composite values or pointers to them are not"
                              << " supported yet!"
                              << std::endl;
                    std::exit(1);
                }

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

// Member selection and indexing bind more tightly than prefix operators.
AstNodePtr Parser::PostfixExpression(const std::vector<NodePtr>& token_list,
                                    idx_t& i) {
    AstNodePtr expression = ParsePrimary(token_list, i);
    while (true) {
        const NodeTag tag = TokenTag(token_list[i]);
        if (tag == T_Dot || tag == T_Arrow) {
            expression = MemberAccess(std::move(expression), token_list, i);
        } else if (tag == T_LBracket) {
            expression = ArrayAccess(std::move(expression), token_list, i);
        } else if (tag == T_Inc || tag == T_Dec) {
            std::cerr << "syntax error: increment and decrement need a"
                      << " standalone variable!" << std::endl;
            std::exit(1);
        } else {
            return expression;
        }
    }
}

AstNodePtr Parser::MemberAccess(AstNodePtr base,
                               const std::vector<NodePtr>& token_list,
                               idx_t& i) {
    const bool through_pointer = TokenTag(token_list[i]) == T_Arrow;
    const PrimitiveType wanted = through_pointer
        ? PointerTo(PrimitiveType::kComposite) : PrimitiveType::kComposite;
    if (base->GetType() != wanted || !base->GetComposite() ||
        (!through_pointer && !IsLvalue(base->GetAstNodeTag()))) {
        std::cerr << "syntax error: " << (through_pointer ? "->" : ".")
                  << " needs " << (through_pointer ?
                                     "a pointer to a composite value" :
                                     "a composite value")
                  << "!" << std::endl;
        std::exit(1);
    }

    ++i;
    const std::string name = MatchIdentifier(token_list, i, "a member name");
    const auto& members = base->GetComposite()->members;
    const auto member = std::find_if(members.begin(), members.end(),
        [&name](const Symbol& candidate) { return candidate.name == name; });
    if (member == members.end()) {
        std::cerr << "syntax error: composite type has no member " << name << "!"
                  << std::endl;
        std::exit(1);
    }
    return std::make_unique<AstMember>(base, *member, through_pointer);
}

AstNodePtr Parser::ArrayAccess(AstNodePtr base,
                             const std::vector<NodePtr>& token_list,
                             idx_t& i) {
    if (!IsPointerType(base->GetType())) {
        std::cerr << "syntax error: this value cannot be indexed!" << std::endl;
        std::exit(1);
    }

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
TypeRef Parser::ParseType(const std::vector<NodePtr>& token_list, idx_t& i)
{
    TypeRef type;

    switch (TokenTag(token_list[i]))
    {
        /*
         * A composite type is like any other, so the keyword and whatever
         * it names are read here and the stars which follow are read with
         * every other type's below.
         */
        case T_Struct:
        case T_Union:
            type = CompositeTypeDeclaration(token_list, i);
            break;
        case T_Char:
            type.type = PrimitiveType::kChar;
            i++;
            break;
        case T_Int:
            type.type = PrimitiveType::kInt;
            i++;
            break;
        case T_Long:
            type.type = PrimitiveType::kLong;
            i++;
            break;
        case T_Void:
            type.type = PrimitiveType::kVoid;
            i++;
            break;
        default:
            std::cerr << "syntax error: expect a type!" << std::endl;
            std::exit(1);
    }

    while (TokenTag(token_list[i]) == T_Star)
    {
        type.type = PointerTo(type.type);
        i++;
    }

    return type;
}

/*
 * composite_type: struct_type | union_type
 *      ;
 *
 * The `struct` or `union` keyword has already been read. A named type is
 * looked up in the same tag space for both shapes, so a later definition
 * cannot turn one shape into the other.
 */
TypeRef Parser::CompositeTypeDeclaration(
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    const NodeTag keyword = TokenTag(token_list[i]);
    const std::string keyword_name = keyword == T_Union ? "union" : "struct";
    const CompositeKind kind = keyword == T_Union
        ? CompositeKind::kUnion : CompositeKind::kStruct;
    i++;    /* step over the keyword */

    std::string name;
    std::shared_ptr<CompositeType> type;

    if (TokenTag(token_list[i]) == T_Identifier)
    {
        name = MatchIdentifier(token_list, i,
                               keyword_name == "union" ? "a union name"
                                                       : "a struct name");
        type = symbol_table_.FindTag(name);
    }

    /* Without a brace this only names a type which is already known. */
    if (TokenTag(token_list[i]) != T_LBrace)
    {
        if (!type)
        {
            std::cerr << "syntax error: there is no " << keyword_name;
            std::cerr << " " << name << "!" << std::endl;
            std::exit(1);
        }

        return TypeRef{.type = PrimitiveType::kComposite,
                       .composite = std::move(type)};
    }

    if (type)
    {
        std::cerr << "syntax error: " << keyword_name << " " << name;
        std::cerr << " is already defined!" << std::endl;
        std::exit(1);
    }

    type = std::make_shared<CompositeType>();
    type->kind = kind;
    type->name = std::move(name);

    if (!type->name.empty())
        symbol_table_.AddTag(type);

    types_.push_back(type);

    ParseMembers(token_list, i, type);

    return TypeRef{.type = PrimitiveType::kComposite,
                   .composite = std::move(type)};
}

/*
 * member_list: type identifier ';'
 *      |       type identifier '[' number ']' ';'
 *      |       member_list member_list
 *      ;
 *
 * A struct lines its members up one after another, each at its own
 * alignment. A union puts every member at the start and takes as much
 * room as the largest one, rounded up to the alignment they share.
 */
void Parser::ParseMembers(const std::vector<NodePtr>& token_list,
    idx_t& i,
    const std::shared_ptr<CompositeType>& type)
{
    Match(token_list, i, T_LBrace, "{");

    while (TokenTag(token_list[i]) != T_RBrace)
    {
        if (TokenTag(token_list[i]) == T_EOF)
        {
            std::cerr << "syntax error: expect }!" << std::endl;
            std::exit(1);
        }

        TypeRef member_type = ParseType(token_list, i);
        std::string name = MatchIdentifier(token_list, i, "a member name");

        Symbol member{.name = name,
                      .type = member_type.type,
                      .composite = member_type.composite,
                      .stype = StructuralType::kVariable,
                      .storage = StorageClass::kMember};

        if (std::any_of(type->members.begin(), type->members.end(),
            [&name](const Symbol& previous) {
                return previous.name == name;
            })) {
            std::cerr << "syntax error: duplicate composite member " << name
                      << "!" << std::endl;
            std::exit(1);
        }

        if (TokenTag(token_list[i]) == T_LBracket)
            member = ArrayDeclaration(token_list, i, member_type, name,
                                      StorageClass::kMember);

        Match(token_list, i, T_Semicolon, ";");

        /*
         * A composite cannot hold a value of itself, because knowing how
         * much room such a member takes would take knowing the size of
         * the type it is being worked out for. A pointer to itself is a
         * value of a size which is already known, and that is how a type
         * which refers to itself is written.
         */
        if (SymbolStorageSize(member) == 0)
        {
            if (member.composite)
            {
                std::cerr << "syntax error: the member " << name;
                std::cerr << " is a composite type which is not finished!";
            }
            else
            {
                std::cerr << "syntax error: a member needs a value type!";
            }

            std::cerr << std::endl;
            std::exit(1);
        }

        type->members.push_back(std::move(member));
    }

    i++;    /* step over the right brace */

    if (type->members.empty())
    {
        std::cerr << "syntax error: a composite type needs at least one";
        std::cerr << " member!" << std::endl;
        std::exit(1);
    }

    int32 alignment = 1;

    for (const Symbol& member : type->members)
    {
        const int32 member_alignment = SymbolAlignment(member);
        if (member_alignment > alignment)
            alignment = member_alignment;
    }

    type->alignment = alignment;

    if (type->kind == CompositeKind::kUnion)
    {
        const Symbol& largest = *std::max_element(
            type->members.begin(), type->members.end(),
            [](const Symbol& left, const Symbol& right) {
                return SymbolStorageSize(left) < SymbolStorageSize(right);
            });

        const int32 remainder = SymbolStorageSize(largest) % alignment;
        type->size = SymbolStorageSize(largest) +
            (remainder == 0 ? 0 : alignment - remainder);
        return;
    }

    int32 size = 0;
    for (Symbol& member : type->members)
    {
        const int32 member_alignment = SymbolAlignment(member);

        const int32 remainder = size % member_alignment;
        if (remainder != 0)
            size += member_alignment - remainder;

        member.member_offset = size;
        size += SymbolStorageSize(member);
    }

    const int32 remainder = size % alignment;
    if (remainder != 0)
        size += alignment - remainder;

    type->size = size;
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
        case A_AstMember:
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

int32 Parser::AllocateLocal(const TypeRef& type)
{
    const int32 size = ValueSize(type.type, type.composite);

    if (size == 0)
    {
        std::cerr << "syntax error: a variable needs a value type!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return AllocateLocal(size, ValueAlignment(type.type, type.composite));
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

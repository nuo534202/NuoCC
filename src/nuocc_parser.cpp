#include "nuocc_parser.hpp"

#include <stdlib.h>

#include <algorithm>
#include <iostream>
#include <limits>

#include "nodes/nuocc_scanner_nodes.hpp"
#include "nuocc_optimise.hpp"
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

    program.types = std::move(types_);

    return program;
}

const SymbolTable& Parser::GetSymbolTable() const
{
    return symbol_table_;
}

/*
 * global_declaration: function_declaration | var_declaration  ;
 */
void Parser::GlobalDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    Program& program)
{
    /*
     * `extern` says the storage of what is declared is given in another
     * source file, so the name is added to the symbol table but nothing is
     * emitted for it here; `static` says the name belongs to this file
     * alone, and its storage is put here like any other global one.
     */
    const DeclarationPrefix prefix =
        ParseDeclarationPrefix(token_list, i, StorageClass::kGlobal);
    const bool is_extern = prefix.storage == StorageClass::kExtern;

    /* A typedef declares a name for a type and nothing else. */
    if (TokenTag(token_list[i]) == T_Typedef)
    {
        TypedefDeclaration(token_list, i);
        Match(token_list, i, T_Semicolon, ";");
        return;
    }

    TypeRef type = ParseType(token_list, i);

    /*
     * A definition which no variable follows declares the type and
     * nothing else, as `struct fred { int x; };` and `enum fred { apple };`
     * do.
     */
    if (TokenTag(token_list[i]) == T_Semicolon && type_definition_)
    {
        Match(token_list, i, T_Semicolon, ";");
        return;
    }

    /*
     * A function declares a name of its own, and it is told from a
     * variable before the name is read: the stars come first and a '('
     * after the name follows.
     */
    if (IsFunctionDeclarator(token_list, i))
    {
        TypeRef return_type = ParseStars(token_list, i, type);
        std::string name = MatchIdentifier(token_list, i, "a function name");

        /* A prototype declares no body, so it adds no function. */
        AstNodePtr function =
            FunctionDeclaration(token_list, i, return_type, name,
                                prefix.is_static);

        if (function)
            program.functions.push_back(std::move(function));

        return;
    }

    std::vector<DeclaredName> names = VariableList(token_list, i, type, prefix);

    Match(token_list, i, T_Semicolon, ";");

    /*
     * The storage belongs here for every name but an extern one, which is
     * left for the source file which defines it.
     */
    if (is_extern)
        return;

    for (const DeclaredName& name : names)
        program.globals.push_back(std::make_unique<AstDeclare>(name.symbol));
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
    const std::string& name,
    bool is_static)
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
    bool is_variadic = false;
    std::vector<Symbol> parameters =
        FunctionParameters(token_list, i, is_variadic);
    Match(token_list, i, T_RParen, ")");

    Symbol symbol{.name = name,
                  .type = type.type,
                  .composite = type.composite,
                  .stype = StructuralType::kFunction,
                  .is_static = is_static,
                  .is_variadic = is_variadic};

    for (const Symbol& parameter : parameters)
        symbol.parameter_types.push_back(
            TypeRef{.type = parameter.type, .composite = parameter.composite});

    if (previous)
        CheckPrototype(*previous, symbol);

    /* The token after the parameter list says whether a body follows. */
    const bool has_body = TokenTag(token_list[i]) == T_LBrace;

    /*
     * The extra arguments of a variadic function are read by the function
     * itself rather than by the compiler, and the language has no way of
     * naming them yet, so a function whose body is written here may not
     * take a variable number of them.
     */
    if (is_variadic && has_body)
    {
        std::cerr << "syntax error: defining the body of a function which";
        std::cerr << " takes a variable number of arguments is not";
        std::cerr << " supported yet!" << std::endl;
        std::exit(1);
    }

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
    loop_level_ = 0;
    switch_level_ = 0;
    labels_.clear();
    named_labels_.clear();
    AstNodePtr body = CompoundStatement(token_list, i);

    /*
     * Every name a goto gave has to be a label of this function, since a
     * goto reaches a statement of the function it stands in and nothing
     * else. Whether one was written is only known once the whole body has
     * been read, since a goto may name a label written after it.
     */
    for (const std::string& name : named_labels_)
    {
        if (std::find(labels_.begin(), labels_.end(), name) == labels_.end())
        {
            std::cerr << "syntax error: there is no label named " << name;
            std::cerr << "!" << std::endl;
            std::exit(1);
        }
    }

    labels_.clear();
    named_labels_.clear();

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

        /*
         * A label names the statement which follows it rather than being a
         * statement of its own, so a label in front of the return which
         * ends the body names that return: the statement the label names
         * is the one which has to be looked at.
         */
        const AstNode *ending = last.get();

        while (ending && ending->GetAstNodeTag() == A_AstLabel)
            ending = ending->GetLeft().get();

        if (!ending || ending->GetAstNodeTag() != A_AstReturn)
        {
            std::cerr << "syntax error: function " << symbol.name;
            std::cerr << " does not end with a return!" << std::endl;
            std::exit(1);
        }
    }

    /*
     * The body is optimised once it is complete, so that the code
     * generator, and anyone reading the tree, sees the tree the compiler
     * would have written down had it worked out its numbers itself.
     */
    body = Optimise(std::move(body));

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

        /*
         * A declaration which defines a type and names nothing builds no
         * tree, so there is nothing to glue onto what came before it.
         */
        if (!statement.tree)
            continue;

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
 * A statement may stand alone as the body of an if, a while or a for loop,
 * or a series of them may stand between braces as a compound statement. A
 * single statement ends in a semicolon, which is matched here.
 */
AstNodePtr Parser::BodyStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    ParsedStatement statement = Statement(token_list, i);

    if (statement.semicolon)
        Match(token_list, i, T_Semicolon, ";");

    return std::move(statement.tree);
}

/*
 * case_body: <empty>
 *      |   statement case_body
 *      ;
 *
 * The statements of a switch arm are gathered until the arm ends, which is
 * where the next case, the default or the end of the switch begins. More
 * than one statement may stand there without braces, and a '{' simply
 * begins a compound statement like it does anywhere else.
 */
AstNodePtr Parser::CaseBody(const std::vector<NodePtr>& token_list, idx_t& i)
{
    std::vector<AstNodePtr> statements;

    while (true)
    {
        const NodeTag tag = TokenTag(token_list[i]);

        if (tag == T_Case || tag == T_Default || tag == T_RBrace)
            break;

        if (tag == T_EOF)
        {
            std::cerr << "syntax error: expect }!" << std::endl;
            std::exit(1);
        }

        ParsedStatement statement = Statement(token_list, i);

        if (statement.semicolon)
            Match(token_list, i, T_Semicolon, ";");

        if (statement.tree)
            statements.push_back(std::move(statement.tree));
    }

    return GlueStatements(statements);
}

/*
 * expression_list: <empty>
 *      |   expression
 *      |   expression ',' expression_list
 *      ;
 *
 * The expressions are glued left to right, so that they are worked out in
 * the order they are written. An empty clause is allowed, which is how
 * `for (; x < 3; x= x + 1)` is written.
 */
AstNodePtr Parser::ExpressionList(const std::vector<NodePtr>& token_list,
    idx_t& i,
    NodeTag end)
{
    std::vector<AstNodePtr> expressions;

    if (TokenTag(token_list[i]) == end)
        return nullptr;

    while (true)
    {
        expressions.push_back(BinaryExpression(token_list, i, 0));

        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;
    }

    return GlueStatements(expressions);
}

/*
 * statement: print_statement
 *      |     declaration
 *      |     expression_statement
 *      |     if_statement
 *      |     while_statement
 *      |     do_statement
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
        /*
         * Braces begin a compound statement, which is a statement of its
         * own and needs no semicolon after it.
         */
        case T_LBrace:
            return {CompoundStatement(token_list, i), false};
        /*
         * A ';' on its own is a statement which does nothing, which is how
         * a loop with nothing to do on a pass is written. It is stepped
         * over here, so there is nothing left for the caller to match.
         */
        case T_Semicolon:
            i++;
            return {nullptr, false};
        case T_Print:
            return {PrintStatement(token_list, i), true};
        case T_Int:
        case T_Char:
        case T_Short:
        case T_Long:
        case T_Void:
        case T_Struct:
        case T_Union:
        case T_Enum:
        case T_Typedef:
        case T_Extern:
        case T_Static:
            return {DeclareStatement(token_list, i), true};
        case T_If:
            return {IfStatement(token_list, i), false};
        case T_While:
            return {WhileStatement(token_list, i), false};
        case T_Do:
            return {DoStatement(token_list, i), false};
        case T_For:
            return {ForStatement(token_list, i), false};
        case T_Return:
            return {ReturnStatement(token_list, i), true};
        case T_Break:
            return {BreakStatement(token_list, i), true};
        case T_Continue:
            return {ContinueStatement(token_list, i), true};
        case T_Switch:
            return {SwitchStatement(token_list, i), false};
        case T_Goto:
            return {GotoStatement(token_list, i), true};
        default:
            /*
             * A name with a ':' after it is a label, which names the
             * statement that follows it and which a goto reaches by that
             * name. Every other name begins an expression.
             */
            if (TokenTag(token_list[i]) == T_Identifier &&
                TokenTag(token_list[i + 1]) == T_Colon)
                return LabelStatement(token_list, i);

            /*
             * A statement may declare a variable whose type is a name
             * made with typedef, so a name which is one of those starts a
             * declaration and not an expression.
             */
            if (TokenTag(token_list[i]) == T_Identifier &&
                IsTypedefName(token_list[i]))
                return {DeclareStatement(token_list, i), true};

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
    /*
     * An `extern` declaration names a global whose storage is given
     * elsewhere, even when it is written inside a function, so it is given
     * no room in the stack frame.
     */
    const DeclarationPrefix prefix =
        ParseDeclarationPrefix(token_list, i, StorageClass::kLocal);

    /* A typedef declares a name for a type and nothing else. */
    if (TokenTag(token_list[i]) == T_Typedef)
    {
        TypedefDeclaration(token_list, i);
        return nullptr;
    }

    TypeRef type = ParseType(token_list, i);

    /* A definition which no variable follows declares the type and
     * nothing else; the semicolon is left to the caller. */
    if (TokenTag(token_list[i]) == T_Semicolon && type_definition_)
        return nullptr;

    /* The declaration may name more than one variable of that base type. */
    std::vector<DeclaredName> names = VariableList(token_list, i, type, prefix);

    std::vector<AstNodePtr> declarations;

    for (DeclaredName& name : names)
    {
        declarations.push_back(std::make_unique<AstDeclare>(name.symbol));

        /* A name given a value is given it by an assignment. */
        if (name.initialiser)
            declarations.push_back(std::move(name.initialiser));
    }

    return GlueStatements(declarations);
}

/*
 * Stop unless the storage of a name is given where the name is declared.
 * The value of a global goes into its own storage and a local one is given
 * its value by an assignment, but the storage of an extern name belongs to
 * another source file, so a value may not be put in it from here.
 */
void Parser::CheckInitialisable(StorageClass storage,
    const std::string& name) const
{
    if (storage == StorageClass::kExtern)
    {
        std::cerr << "syntax error: " << name;
        std::cerr << " cannot be given a value where it is declared!";
        std::cerr << std::endl;
        std::exit(1);
    }

    /*
     * A member is not a place of its own: the room it takes is part of a
     * value of the composite it stands in, and there is nothing to put a
     * value in until a value of that composite is made.
     */
    if (storage == StorageClass::kMember)
    {
        std::cerr << "syntax error: " << name << " is a member, so it";
        std::cerr << " cannot be given a value where it is declared!";
        std::cerr << std::endl;
        std::exit(1);
    }
}

/*
 * The number a constant is written with: an integer literal, and the minus
 * in front of it which makes the number negative. A minus which is not
 * followed by a number is the subtraction operator and is not read here.
 */
int32 Parser::SizeOf(const TypeRef& type)
{
    const int32 size = ValueSize(type.type, type.composite);

    if (size == 0)
    {
        std::cerr << "syntax error: sizeof needs a type which holds a";
        std::cerr << " value!" << std::endl;
        std::exit(1);
    }

    return size;
}

int32 Parser::SizeOfValue(const AstNodePtr& value)
{
    if (value->GetAstNodeTag() == A_AstAddress)
        return SymbolStorageSize(
            static_cast<const AstAddress&>(*value).GetSymbol());

    return SizeOf(TypeRef{.type = value->GetType(),
                          .composite = value->GetComposite()});
}

int32 Parser::ParseConstantNumber(const std::vector<NodePtr>& token_list,
    idx_t& i,
    std::string_view what)
{
    /*
     * The number is written as an expression, which is worked out while the
     * program is being compiled: `2 + 3` is five, and a minus in front of a
     * number is part of it rather than a subtraction to wait for.
     */
    AstNodePtr tree = Optimise(BinaryExpression(token_list, i, 0));

    int32 value = 0;

    if (!ConstantValue(*tree, value))
    {
        std::cerr << "syntax error: expect " << what << "!" << std::endl;
        std::exit(1);
    }

    return value;
}

bool Parser::ConstantValue(const AstNode& node, int32& value) const
{
    /*
     * The expression has been optimised, so every number the compiler
     * could work out is a literal by now: a larger one, or one written
     * with a minus in front of it, is a literal of its own rather than an
     * operation. What is left is an expression which needs the program to
     * run, and that is not a constant.
     */
    if (node.GetAstNodeTag() != A_AstIntLit)
        return false;

    value = static_cast<const AstIntLit&>(node).GetValue();
    return true;
}

/*
 * Build the assignment which gives a local variable the value it is
 * declared with. The value is worked out and stored where the variable is,
 * which is what an assignment written by hand does, and what the caller
 * runs in place of the declaration once the variable exists.
 */
AstNodePtr Parser::InitialiserAssignment(const Symbol& symbol,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    /*
     * A composite value cannot be read as a value, so the value one is
     * given is written one of the two ways a composite value ever is: as
     * the values of the members of the type, or as another place holding a
     * value of the same type.
     */
    if (symbol.type == PrimitiveType::kComposite)
        return CompositeInitialiser(symbol, token_list, i);

    AstNodePtr expression = BinaryExpression(token_list, i, 0);

    /* The value has to fit the type the variable was declared with. */
    if (!ModifyType(expression,
                    TypeRef{.type = symbol.type,
                            .composite = symbol.composite},
                    std::nullopt))
    {
        std::cerr << "syntax error: this value does not fit the variable ";
        std::cerr << symbol.name << "!" << std::endl;
        std::exit(1);
    }

    const PrimitiveType type = expression->GetType();
    AstNodePtr target = MakeIdentLeaf(symbol);

    /*
     * The value is the left child and the variable it is stored in is the
     * right one, which is the order every assignment is built in.
     */
    return std::make_unique<AstOperator>(expression, target, T_Assign, type);
}

/*
 * Build the statement which gives a local value of a composite type the
 * value it is declared with, and leave the token after that value for the
 * caller.
 *
 * A composite value cannot be read as a value, so what it is given is
 * written one of the two ways a composite value is ever written: as the
 * values of the members of its type, between braces, or as another place
 * holding a value of the same type, which is copied whole the way item 69
 * gives one place what another holds.
 */
AstNodePtr Parser::CompositeInitialiser(const Symbol& symbol,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    if (TokenTag(token_list[i]) != T_LBrace)
    {
        AstNodePtr source = BinaryExpression(token_list, i, 0);

        if (source->GetType() != PrimitiveType::kComposite ||
            source->GetComposite() != symbol.composite ||
            !IsLvalue(source->GetAstNodeTag()))
        {
            std::cerr << "syntax error: a value of a composite type can only";
            std::cerr << " be given another place holding one of the same";
            std::cerr << " type, or the values of its members!" << std::endl;
            std::exit(1);
        }

        AstNodePtr target = MakeIdentLeaf(symbol);

        return std::make_unique<AstOperator>(source, target, T_Assign,
                                             PrimitiveType::kComposite);
    }

    std::vector<AstNodePtr> statements;

    /*
     * The room of the variable holds nothing before the values written for
     * its members are stored into it, so every part of the value the list
     * does not reach starts out at zero, the way an element of an array
     * which is not given a value does.
     */
    statements.push_back(ClearArray(
        symbol, ValueSize(PrimitiveType::kComposite, symbol.composite)));

    std::vector<const Symbol*> path;

    CompositeValues(symbol.composite, symbol, path, token_list, i, statements,
                    true);

    return GlueStatements(statements);
}

/*
 * Read the values written for the members of a composite value, storing
 * each of them into the member it is written for. The members are written
 * in order, and a member which is an array or a composite value of its own
 * takes a list of values of its own between braces; a union takes the value
 * written for its first member, since its members share one piece of room.
 *
 * The list is written between braces of its own unless `braced` is false,
 * which is how the members of an anonymous member are written: they are
 * members of the type which holds it, so they take their place in the list
 * of that type rather than in a list of their own.
 */
void Parser::CompositeValues(const std::shared_ptr<CompositeType>& composite,
    const Symbol& symbol,
    std::vector<const Symbol*>& path,
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    std::vector<AstNodePtr>& statements,
    bool braced)
{
    if (braced)
        Match(token_list, i, T_LBrace, "{");

    const idx_t count = composite->kind == CompositeKind::kUnion
                            ? 1
                            : composite->members.size();

    for (idx_t k = 0; k < count; k++)
    {
        /*
         * The braces may be closed before a value has been written for
         * every member, since a member which is not given a value starts
         * out holding nothing at all.
         */
        if (TokenTag(token_list[i]) == T_RBrace)
            break;

        const Symbol& member = composite->members[k];

        /*
         * What a message about this member calls it, since an anonymous
         * member has no name to be written with.
         */
        const std::string written_as =
            member.name.empty() ? "an anonymous member" : member.name;

        path.push_back(&member);

        if (member.stype == StructuralType::kArray)
        {
            if (member.type == PrimitiveType::kComposite)
            {
                std::cerr << "syntax error: the member " << member.name;
                std::cerr << " is an array of composite values, which cannot";
                std::cerr << " be given values where it is declared!"
                          << std::endl;
                std::exit(1);
            }

            /*
             * An array of char may be given a string in place of the list
             * of values, the way one declared on its own may.
             */
            if (TokenTag(token_list[i]) == T_StrLit)
            {
                if (member.type != PrimitiveType::kChar)
                {
                    std::cerr << "syntax error: only an array of char may be";
                    std::cerr << " given a string where it is declared!";
                    std::cerr << std::endl;
                    std::exit(1);
                }

                const std::string text = StringLiteralText(token_list, i);

                StringElementValues(member.element_count, symbol, path, text,
                                    statements);
            }
            else
            {
                ElementValues(member.element_count, symbol, path,
                              TypeRef{.type = member.type,
                                      .composite = member.composite},
                              token_list, i, statements);
            }
        }
        else if (member.type == PrimitiveType::kComposite)
        {
            /*
             * The members of an anonymous member are written as the
             * members of the type which holds it are, so they take braces
             * of their own only when some are written; every other
             * composite member takes them.
             */
            const bool inner_braced =
                member.name.empty() ? TokenTag(token_list[i]) == T_LBrace
                                    : true;

            CompositeValues(member.composite, symbol, path, token_list, i,
                            statements, inner_braced);
        }
        else
        {
            AstNodePtr value = BinaryExpression(token_list, i, 0);

            if (!ModifyType(value, TypeRef{.type = member.type,
                                           .composite = member.composite},
                            std::nullopt))
            {
                std::cerr << "syntax error: this value does not fit the";
                std::cerr << " member " << member.name << "!" << std::endl;
                std::exit(1);
            }

            AstNodePtr place = MemberPlace(symbol, path);

            statements.push_back(std::make_unique<AstOperator>(
                value, place, T_Assign, value->GetType()));
        }

        path.pop_back();

        /*
         * A ',' separates the values written for two members, and the last
         * one written needs none, since what is left starts out at zero.
         */
        if (k + 1 < count && TokenTag(token_list[i]) != T_RBrace)
        {
            if (TokenTag(token_list[i]) != T_Comma)
            {
                std::cerr << "syntax error: expect a ',' or a '}' after a";
                std::cerr << " value of " << written_as << "!" << std::endl;
                std::exit(1);
            }

            i++;
        }
    }

    /*
     * A list written between braces of its own is the whole list, so
     * anything left in it is a value too many; the members of an anonymous
     * member, written in the list of the type which holds it, leave what
     * follows to that list.
     */
    if (braced)
    {
        if (TokenTag(token_list[i]) != T_RBrace)
        {
            std::cerr << "syntax error: too many values where a value of a";
            std::cerr << " composite type is given!" << std::endl;
            std::exit(1);
        }

        i++;    /* step over '}' */
    }
}

/*
 * Read the values written between braces for the elements of an array,
 * storing each of them into the element it is written for. The elements are
 * written in order, and the braces may be closed before every one of them
 * has been written for, since what is left starts out at zero.
 */
void Parser::ElementValues(int32 element_count,
    const Symbol& symbol,
    const std::vector<const Symbol*>& path,
    const TypeRef& element_type,
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    std::vector<AstNodePtr>& statements)
{
    Match(token_list, i, T_LBrace, "{");

    for (int32 k = 0; k < element_count; k++)
    {
        /* The braces may be closed before every element is written for. */
        if (TokenTag(token_list[i]) == T_RBrace)
            break;

        AstNodePtr value = BinaryExpression(token_list, i, 0);

        if (!ModifyType(value, element_type, std::nullopt))
        {
            std::cerr << "syntax error: value " << (k + 1) << " of the array";
            std::cerr << " does not fit the type of an element!" << std::endl;
            std::exit(1);
        }

        AstNodePtr place = MemberPlace(symbol, path);

        statements.push_back(PlaceElementAssignment(place, k, value));

        if (k + 1 < element_count && TokenTag(token_list[i]) != T_RBrace)
        {
            if (TokenTag(token_list[i]) != T_Comma)
            {
                std::cerr << "syntax error: expect a ',' or a '}' after an";
                std::cerr << " element of an array!" << std::endl;
                std::exit(1);
            }

            i++;
        }
    }

    if (TokenTag(token_list[i]) != T_RBrace)
    {
        std::cerr << "syntax error: too many values for an array!"
                  << std::endl;
        std::exit(1);
    }

    i++;    /* step over '}' */
}

/*
 * Store the characters of a string into the elements of an array of char,
 * which is what a string written for one gives it. The room of the array
 * holds nothing before the characters are stored into it, so the zero byte
 * the string ends with is already there and is not written here.
 */
void Parser::StringElementValues(int32 element_count,
    const Symbol& symbol,
    const std::vector<const Symbol*>& path,
    const std::string& text,
    std::vector<AstNodePtr>& statements)
{
    if (static_cast<int32>(text.size()) > element_count)
    {
        std::cerr << "syntax error: too many characters for the array!";
        std::cerr << std::endl;
        std::exit(1);
    }

    for (idx_t k = 0; k < text.size(); k++)
    {
        AstNodePtr value = MakeIntLitLeaf(
            static_cast<unsigned char>(text[k]), PrimitiveType::kChar);
        AstNodePtr place = MemberPlace(symbol, path);

        statements.push_back(PlaceElementAssignment(
            place, static_cast<int32>(k), value));
    }
}

/*
 * The place the member a path names stands at, reached from the name of the
 * value it stands in: an empty path is the value itself, and each member of
 * the path is reached through the one written before it.
 */
AstNodePtr Parser::MemberPlace(const Symbol& symbol,
    const std::vector<const Symbol*>& path)
{
    AstNodePtr place = MakeIdentLeaf(symbol);

    for (const Symbol* member : path)
        place = std::make_unique<AstMember>(place, *member, false);

    return place;
}

/*
 * Parse the one value a global variable, or one element of a global array,
 * is given where it is declared. A pointer to a char is given the
 * characters of a string, and the value becomes the address the code
 * generator stores them at; every other type takes an integer literal which
 * has to fit it. The token after the value is left for the caller.
 */
InitialValue Parser::ParseConstantValue(const TypeRef& type,
    const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    /*
     * A cast may stand in front of the value, which is what lets a value the
     * variable could not hold on its own be given to it: `char a= (char)
     * 65536;` or `char *str= (void *)0;`. The type it names has to agree
     * with the type of the variable, and a pointer to void agrees with any
     * pointer.
     */
    bool has_cast = false;

    if (TokenTag(token_list[i]) == T_LParen &&
        StartsCast(token_list, i + 1))
    {
        i++;    /* step over '(' */

        TypeRef cast = ParseCast(token_list, i);
        Match(token_list, i, T_RParen, ")");
        has_cast = true;

        if (cast != type &&
            !(IsPointerType(type.type) &&
              cast.type == PointerTo(PrimitiveType::kVoid)))
        {
            std::cerr << "syntax error: the cast does not fit the variable";
            std::cerr << " it is given to!" << std::endl;
            std::exit(1);
        }
    }

    /*
     * The value is an expression, which the compiler works out while it is
     * compiling the program: `int x= 10 + 6;` is given the value 16, and
     * nothing is left for the program to work out when it runs.
     */
    AstNodePtr tree = Optimise(BinaryExpression(token_list, i, 0));

    /* The characters of a string are what a pointer to a char is given, and
     * the value is the address they are stored at. */
    if (tree->GetAstNodeTag() == A_AstStrLit)
    {
        if (type.type != PointerTo(PrimitiveType::kChar))
        {
            std::cerr << "syntax error: only a pointer to a char may be";
            std::cerr << " given a string as its value!" << std::endl;
            std::exit(1);
        }

        return static_cast<const AstStrLit&>(*tree).GetText();
    }

    if (!IsIntType(type.type) && !IsPointerType(type.type))
    {
        std::cerr << "syntax error: this type cannot be given a value where";
        std::cerr << " it is declared!" << std::endl;
        std::exit(1);
    }

    int32 value = 0;

    if (!ConstantValue(*tree, value))
    {
        std::cerr << "syntax error: a value given where a variable is";
        std::cerr << " declared has to be one the compiler can work out!";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (IsPointerType(type.type) && !has_cast)
    {
        std::cerr << "syntax error: a number may only be given to a pointer";
        std::cerr << " behind a cast!" << std::endl;
        std::exit(1);
    }

    /*
     * A char holds 0 to 255 and a short -32768 to 32767, so a value
     * outside one of those is only taken behind a cast, which cuts it down
     * to the bits the type holds when it is stored.
     */
    if (!has_cast &&
        (type.type == PrimitiveType::kChar ||
         type.type == PrimitiveType::kShort) &&
        !ValueFitsType(value, type.type))
    {
        std::cerr << "syntax error: the value " << value;
        std::cerr << (value > 0 ? " is too big for a "
                                : " is too small for a ");
        std::cerr << (type.type == PrimitiveType::kChar ? "char" : "short");
        std::cerr << "!" << std::endl;
        std::exit(1);
    }

    return value;
}

/*
 * declarator_list: declarator | declarator ',' declarator_list  ;
 *
 * declarator: opt_pointer identifier
 *      |   opt_pointer identifier '[' expression ']'
 *      ;
 *
 * Every name adds its own stars and its own array size to the base type, so
 * one declaration can hold values, pointers and arrays at once. A name is
 * declared as soon as it has been read, so a later one can be written in
 * terms of the ones before it.
 */
std::vector<Parser::DeclaredName> Parser::VariableList(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    const TypeRef& base,
    const DeclarationPrefix& prefix)
{
    std::vector<DeclaredName> names;

    while (true)
    {
        TypeRef type = ParseStars(token_list, i, base);
        std::string name = MatchIdentifier(token_list, i, "a variable name");

        const bool is_array = TokenTag(token_list[i]) == T_LBracket;

        Symbol symbol{.name = name,
                      .type = type.type,
                      .composite = type.composite,
                      .stype = StructuralType::kVariable,
                      .storage = prefix.storage,
                      .is_static = prefix.is_static};

        AstNodePtr array_initialiser = nullptr;

        if (is_array)
        {
            symbol = ArrayDeclaration(token_list, i, type, name,
                                      prefix.storage, array_initialiser);
            symbol.is_static = prefix.is_static;
        }
        else if (prefix.storage == StorageClass::kLocal)
        {
            symbol.stack_offset = AllocateLocal(type);
        }

        /*
         * The name is declared before the value it starts with is read, so
         * that the value may name the variables declared before it in the
         * same declaration: `int x= 2, y= x + 3;`.
         *
         * A name declared at the top level may be declared again as the one
         * thing it is not yet, and the two declarations leave one name
         * behind; anywhere else a name is simply added, so that a name
         * inside a function may stand in front of the one outside it.
         */
        if (current_function_type_.type == PrimitiveType::kNone)
            AddGlobalSymbol(symbol);
        else
            symbol_table_.AddSymbol(symbol);

        DeclaredName declared;
        declared.symbol = symbol;
        declared.initialiser = std::move(array_initialiser);

        /*
         * A name may be given the value it starts with. A global one has it
         * stored in its own storage, while a local one is given it by an
         * assignment which the caller runs where the declaration stands.
         */
        if (!is_array && TokenTag(token_list[i]) == T_Assign)
        {
            CheckInitialisable(prefix.storage, name);
            i++;    /* step over '=' */

            if (prefix.storage == StorageClass::kGlobal)
            {
                declared.symbol.initial_values.push_back(
                    ParseConstantValue(type, token_list, i));

                /*
                 * The name was declared before the value was read, so that
                 * the value could name the variables before it; what the
                 * name is declared with is put in the table once the value
                 * is known, which is what the dump of the table reads.
                 */
                symbol_table_.UpdateSymbol(declared.symbol);
            }
            else
            {
                declared.initialiser =
                    InitialiserAssignment(symbol, token_list, i);
            }
        }

        names.push_back(std::move(declared));

        /* A ',' continues the list with another name of that base type. */
        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;
    }

    return names;
}

bool Parser::IsFunctionDeclarator(const std::vector<NodePtr>& token_list,
    idx_t i) const
{
    while (TokenTag(token_list[i]) == T_Star)
        i++;

    return TokenTag(token_list[i]) == T_Identifier &&
           TokenTag(token_list[i + 1]) == T_LParen;
}

/*
 * The array part of a declarator, read once the name has been read:
 *
 *      '[' opt_size ']' opt_values
 *
 * The size between the brackets is how many elements of the declared type
 * the array holds, and the room for all of them is reserved together so
 * that the elements stand next to each other. The size is an expression
 * which the compiler works out while it is compiling the program, and an
 * array written without one takes the number of values it is given. What
 * the size works out to is fixed for the life of the array.
 *
 * The values the array is given are read here as well. A global array has
 * them put in its storage by the compiler, so they travel back in the
 * symbol; a local array is given them by the program, so what comes back
 * is the `initialiser` which stores them where the declaration stands.
 *
 * An array of char may be given a string instead of a list of values, which
 * gives it the characters of the string one element each, and its zero byte
 * as one element more when it has room for it. The string is read apart
 * here, so the characters become the values of the elements and no string
 * of its own is put in the program.
 */
Symbol Parser::ArrayDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i,
    const TypeRef& element_type,
    const std::string& name,
    StorageClass storage,
    AstNodePtr& initialiser)
{
    initialiser = nullptr;

    Match(token_list, i, T_LBracket, "[");

    /*
     * The size may be left out when the array is given its values where it
     * is declared, since how many values there are is then how many
     * elements it holds. A token which is neither a size nor the closing
     * bracket is not a size at all.
     */
    int32 size = 0;
    bool has_size = false;

    if (TokenTag(token_list[i]) != T_RBracket)
    {
        size = ParseConstantNumber(token_list, i, "an array size");
        has_size = true;
    }

    if (has_size && size <= 0)
    {
        std::cerr << "syntax error: an array must have at least one element!";
        std::cerr << std::endl;
        std::exit(1);
    }

    /*
     * Every element of an array is reached by counting the room it takes
     * along from the first one, and the room a composite takes comes from
     * the definition the element type carries.
     */
    const int32 element_size = ValueSize(element_type.type,
                                         element_type.composite);

    if (element_size == 0)
    {
        std::cerr << "syntax error: an array needs a value type!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_RBracket, "]");

    Symbol symbol{.name = name,
                  .type = element_type.type,
                  .composite = element_type.composite,
                  .stype = StructuralType::kArray,
                  .storage = storage,
                  .element_count = size};

    /*
     * The values the array is given where it is declared, which may also
     * give it the size it was not written with. A global array is given
     * values the compiler can work out, since it is the compiler which
     * puts them in the storage; a local array is given values the program
     * works out, since they are stored into it once the program runs.
     */
    const bool is_global = storage == StorageClass::kGlobal;
    std::vector<AstNodePtr> elements;

    /*
     * A composite value cannot be written as a whole, so there is nothing
     * to give an element of an array of them: the array may be declared,
     * but not with the values its elements start out holding.
     */
    if (element_type.type == PrimitiveType::kComposite &&
        TokenTag(token_list[i]) == T_Assign)
    {
        std::cerr << "syntax error: the array " << name << " holds composite";
        std::cerr << " values, which cannot be given where it is declared!";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (TokenTag(token_list[i]) == T_Assign)
    {
        CheckInitialisable(storage, name);

        i++;    /* step over '=' */

        std::vector<InitialValue> values;

        if (TokenTag(token_list[i]) == T_StrLit)
        {
            /*
             * A string gives an array of char its characters, one element
             * each, and the zero byte it ends with as one element more
             * when the array has room for it. An array with too little
             * room for the characters themselves is refused, so `char
             * word[4]= "word";` is the shortest one which holds a whole
             * four letter word, with no room left for the zero byte.
             */
            if (element_type.type != PrimitiveType::kChar)
            {
                std::cerr << "syntax error: only an array of char may be";
                std::cerr << " given a string where it is declared!";
                std::cerr << std::endl;
                std::exit(1);
            }

            const std::string text = StringLiteralText(token_list, i);
            const int32 length = static_cast<int32>(text.size());

            if (has_size && size < length)
            {
                std::cerr << "syntax error: too many characters for the";
                std::cerr << " array " << name << "!" << std::endl;
                std::exit(1);
            }

            std::vector<int32> codes;

            for (char character : text)
                codes.push_back(static_cast<unsigned char>(character));

            if (!has_size || size > length)
                codes.push_back(0);

            for (int32 code : codes)
            {
                if (is_global)
                    values.push_back(code);
                else
                    elements.push_back(
                        MakeIntLitLeaf(code, PrimitiveType::kChar));
            }
        }
        else
        {
            Match(token_list, i, T_LBrace, "{");

            while (TokenTag(token_list[i]) != T_RBrace)
            {
                if (is_global)
                    values.push_back(
                        ParseConstantValue(element_type, token_list, i));
                else
                    elements.push_back(BinaryExpression(token_list, i, 0));

                /* A ',' separates two values, and the last one needs none. */
                if (TokenTag(token_list[i]) != T_Comma)
                    break;

                i++;
            }

            Match(token_list, i, T_RBrace, "}");
        }

        const idx_t given = is_global ? values.size() : elements.size();

        if (has_size && static_cast<int32>(given) > size)
        {
            std::cerr << "syntax error: too many values for the array ";
            std::cerr << name << "!" << std::endl;
            std::exit(1);
        }

        /*
         * An array written without a size holds exactly the values it was
         * given; one with room for more than it was given keeps that room
         * and the elements left over start out zero.
         */
        if (!has_size)
            symbol.element_count = static_cast<int32>(given);

        symbol.initial_values = std::move(values);
    }

    if (symbol.element_count == 0)
    {
        std::cerr << "syntax error: the array " << name;
        std::cerr << " needs a size or a list of values!" << std::endl;
        std::exit(1);
    }

    /* The room for the whole array has to be countable in bytes. */
    if (symbol.element_count >
        std::numeric_limits<int32>::max() / element_size)
    {
        std::cerr << "syntax error: the array " << name << " is too large!";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (storage == StorageClass::kLocal)
    {
        symbol.stack_offset = AllocateLocal(
            element_size * symbol.element_count,
            ValueAlignment(element_type.type, element_type.composite));

        /*
         * The elements of a local array of composite values cannot be
         * given anything where the declaration stands, since a composite
         * value cannot be written, so what the whole array starts out
         * holding is cleared instead. That is the nothing an element of
         * any other local array which was not given a value is left
         * holding, so an array of composites starts out like any other.
         */
        if (element_type.type == PrimitiveType::kComposite)
        {
            initialiser = ClearArray(symbol,
                                     element_size * symbol.element_count);
            return symbol;
        }

        /*
         * The elements of a local array are given their values one after
         * another where the declaration stands, each value being worked
         * out and stored as the ones before it were.
         */
        std::vector<AstNodePtr> stores;
        const int32 given = static_cast<int32>(elements.size());

        for (int32 k = 0; k < given; k++)
        {
            AstNodePtr value = std::move(elements[k]);

            if (!ModifyType(value, element_type, std::nullopt))
            {
                std::cerr << "syntax error: value " << (k + 1) << " of the";
                std::cerr << " array " << name << " does not fit the type of";
                std::cerr << " an element!" << std::endl;
                std::exit(1);
            }

            stores.push_back(ElementAssignment(symbol, k, value));
        }

        /*
         * An element left over is given nothing, the way an element of a
         * global array which was not given a value starts out holding
         * nothing: a number of zero, and an address which points at
         * nothing.
         */
        for (int32 k = given; k < symbol.element_count; k++)
        {
            AstNodePtr nothing = MakeIntLitLeaf(0, element_type.type);

            stores.push_back(ElementAssignment(symbol, k, nothing));
        }

        initialiser = GlueStatements(stores);
    }

    return symbol;
}

/*
 * Give every byte of an array of composite values a zero.
 *
 * A composite value cannot be written as a whole, so an array of them
 * cannot be given the values its elements start with and there is no store
 * of a whole element to clear it with. The room of the array is walked one
 * byte at a time instead, which leaves every element holding the nothing an
 * element of any other local array which was not given a value is left
 * holding.
 */
AstNodePtr Parser::ClearArray(const Symbol& symbol, int32 room)
{
    /*
     * The walk counts with a variable of its own, which lives in the frame
     * of the function beside the array. It is made here rather than read
     * from the program, so it is never named and nothing else reaches it.
     */
    Symbol counter;
    counter.type = PrimitiveType::kInt;
    counter.stype = StructuralType::kVariable;
    counter.storage = StorageClass::kLocal;
    counter.stack_offset = AllocateLocal(
        TypeRef{.type = PrimitiveType::kInt, .composite = nullptr});

    /* counter= 0; */
    AstNodePtr zero = MakeIntLitLeaf(0, PrimitiveType::kInt);
    AstNodePtr counter_target = MakeIdentLeaf(counter);
    AstNodePtr start = std::make_unique<AstOperator>(
        zero, counter_target, T_Assign, PrimitiveType::kInt);

    /* counter < room */
    AstNodePtr limit = MakeIntLitLeaf(room, PrimitiveType::kInt);
    AstNodePtr counting = MakeIdentLeaf(counter);
    AstNodePtr condition = std::make_unique<AstOperator>(counting, limit, T_LT,
                                                         PrimitiveType::kInt);

    /*
     * *((char *)array + counter)= 0;
     *
     * The array is reached as the address of its first element, which is a
     * pointer to a composite value, and the count is added to it as an
     * offset of one byte rather than of one element: the addition is given
     * the type of a pointer to a char, so what it steps by is the room a
     * char takes and the count is how many bytes of the array have been
     * walked.
     */
    AstNodePtr base = std::make_unique<AstAddress>(symbol);
    AstNodePtr offset = MakeIdentLeaf(counter);
    AstNodePtr address = std::make_unique<AstOperator>(
        base, offset, T_Plus, PointerTo(PrimitiveType::kChar));
    AstNodePtr element = std::make_unique<AstDeref>(address,
                                                    PrimitiveType::kChar);
    AstNodePtr nothing = MakeIntLitLeaf(0, PrimitiveType::kChar);
    AstNodePtr body = std::make_unique<AstOperator>(nothing, element, T_Assign,
                                                    PrimitiveType::kChar);

    /* counter= counter + 1; */
    AstNodePtr one = MakeIntLitLeaf(1, PrimitiveType::kInt);
    AstNodePtr step_target = MakeIdentLeaf(counter);
    AstNodePtr counted = MakeIdentLeaf(counter);
    AstNodePtr sum = std::make_unique<AstOperator>(counted, one, T_Plus,
                                                   PrimitiveType::kInt);
    AstNodePtr step = std::make_unique<AstOperator>(sum, step_target, T_Assign,
                                                    PrimitiveType::kInt);

    AstNodePtr loop = std::make_unique<AstFor>(condition, body, step);

    std::vector<AstNodePtr> statements;
    statements.push_back(std::move(start));
    statements.push_back(std::move(loop));

    return GlueStatements(statements);
}

/*
 * The characters a run of strings written one after another stands for. Two
 * strings written side by side are one string, which is how a string too
 * long for the line it is written on is written down over several lines.
 */
std::string Parser::StringLiteralText(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    std::string text;

    while (TokenTag(token_list[i]) == T_StrLit)
    {
        text += static_cast<const Literal<std::string, T_StrLit>*>(
                    token_list[i].get())
                    ->GetValue();
        i++;
    }

    return text;
}

/*
 * The assignment which stores one value into one element of an array: the
 * element is reached the way `name[index]` reaches it, and the value stands
 * on the left of the assignment the way it does in every other one.
 */
AstNodePtr Parser::ElementAssignment(const Symbol& symbol,
    int32 index,
    AstNodePtr& value)
{
    AstNodePtr base = std::make_unique<AstAddress>(symbol);

    return PlaceElementAssignment(base, index, value);
}

/*
 * The same for an array whose place is known rather than being reached by
 * name, which is how an element of an array a struct holds is reached: the
 * place of the array is what an address is taken of instead of a name.
 */
AstNodePtr Parser::PlaceElementAssignment(AstNodePtr& place,
    int32 index,
    AstNodePtr& value)
{
    AstNodePtr offset = MakeIntLitLeaf(index, PrimitiveType::kInt);

    /*
     * An index counts elements and not bytes, so it reaches the element it
     * names only once it has been scaled by the room one element takes,
     * which is what giving it to the operator does. An element which is a
     * composite takes the room its own definition asks for, which travels
     * with the type of the element.
     */
    if (!ModifyType(offset, TypeRef{.type = place->GetType(),
                                    .composite = place->GetComposite()},
                    T_Plus))
    {
        std::cerr << "syntax error: this element of the array cannot be";
        std::cerr << " reached!" << std::endl;
        std::exit(1);
    }

    const PrimitiveType pointer_type = place->GetType();
    AstNodePtr address = std::make_unique<AstOperator>(place, offset, T_Plus,
                                                       pointer_type);
    AstNodePtr element = std::make_unique<AstDeref>(address,
                                                    ValueAt(pointer_type));

    return std::make_unique<AstOperator>(value, element, T_Assign,
                                         value->GetType());
}

/*
 * if_statement: if_head
 *      |        if_head 'else' statement
 *      ;
 *
 * if_head: 'if' '(' true_false_expression ')' statement  ;
 *
 * Each branch is one statement, which may be a compound statement between
 * braces. An else is taken by the if it stands nearest, because the branch
 * of an if is parsed as far as it goes before the else is looked for, so
 * `if (a) if (b) ...; else ...;` leaves the else with the inner if.
 */
AstNodePtr Parser::IfStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_If, "if");

    AstNodePtr condition = Condition(token_list, i);
    AstNodePtr true_branch = BodyStatement(token_list, i);
    AstNodePtr false_branch = nullptr;
    bool has_else = false;

    if (TokenTag(token_list[i]) == T_Else)
    {
        has_else = true;
        i++;
        false_branch = BodyStatement(token_list, i);
    }

    return std::make_unique<AstIf>(condition,
                                   true_branch,
                                   false_branch,
                                   has_else);
}

/*
 * while_statement: 'while' '(' true_false_expression ')' statement  ;
 */
AstNodePtr Parser::WhileStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_While, "while");

    AstNodePtr condition = Condition(token_list, i);

    /*
     * The body stands one loop deeper, so that a break or a continue
     * written in it finds this loop.
     */
    loop_level_++;
    AstNodePtr body = BodyStatement(token_list, i);
    loop_level_--;

    return std::make_unique<AstWhile>(condition, body);
}

/*
 * do_statement: 'do' statement 'while' '(' true_false_expression ')' ';'  ;
 *
 * The body of a do loop runs once before the condition is asked at all, so
 * the condition stands after it and the loop goes round again only where
 * it held. The semicolon which closes the statement is part of the loop
 * rather than of the body, since a body which is one of the statements
 * ending in one would otherwise take it.
 */
AstNodePtr Parser::DoStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Do, "do");

    /*
     * The body stands one loop deeper, so that a break or a continue
     * written in it finds this loop.
     */
    loop_level_++;
    AstNodePtr body = BodyStatement(token_list, i);
    loop_level_--;

    Match(token_list, i, T_While, "while");

    AstNodePtr condition = Condition(token_list, i);

    Match(token_list, i, T_Semicolon, ";");

    return std::make_unique<AstDoWhile>(condition, body);
}

/*
 * for_statement: 'for' '(' expression_list ';'
 *                        true_false_expression ';'
 *                        expression_list ')' statement  ;
 *
 * The clauses before the first semicolon and after the second are lists of
 * expressions which run in the order they are written, as in `for (x= 0, y=
 * 1; x < 6; x++, y= y + 2)`. The clause between them says whether the loop
 * goes round again, and the body is one statement.
 */
AstNodePtr Parser::ForStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_For, "for");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr preop = ExpressionList(token_list, i, T_Semicolon);
    Match(token_list, i, T_Semicolon, ";");

    AstNodePtr condition = BinaryExpression(token_list, i, 0);
    MakeCondition(condition);

    Match(token_list, i, T_Semicolon, ";");
    AstNodePtr postop = ExpressionList(token_list, i, T_RParen);
    Match(token_list, i, T_RParen, ")");

    /*
     * The body stands one loop deeper, so that a break or a continue
     * written in it finds this loop.
     */
    loop_level_++;
    AstNodePtr body = BodyStatement(token_list, i);
    loop_level_--;

    /*
     * The step is kept as part of the loop rather than glued to the end of
     * the body, because a continue goes on to it: `for (i= 0; i < 9; i=
     * i + 1) { continue; }` still counts.
     */
    AstNodePtr loop = std::make_unique<AstFor>(condition, body, postop);

    /* Nothing has to run before a loop which was written without a first
     * clause, so the loop stands on its own. */
    if (!preop)
        return loop;

    return std::make_unique<AstGlue>(preop, loop);
}

/*
 * return_statement: 'return' '(' expression ')'  ;
 */
AstNodePtr Parser::ReturnStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Return, "return");

    /*
     * A function which answers with nothing leaves with a return which is
     * given no value, and one which answers with a value has to be given
     * one: there is no other way for its caller to be given the answer.
     */
    if (TokenTag(token_list[i]) != T_LParen)
    {
        if (current_function_type_.type != PrimitiveType::kVoid)
        {
            std::cerr << "syntax error: a function which answers with a";
            std::cerr << " value has to return one!" << std::endl;
            std::exit(1);
        }

        AstNodePtr nothing = nullptr;

        return std::make_unique<AstReturn>(nothing);
    }

    if (current_function_type_.type == PrimitiveType::kVoid)
    {
        std::cerr << "syntax error: a void function cannot return a value!";
        std::cerr << std::endl;
        std::exit(1);
    }

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
 * break_statement: 'break'  ;
 *
 * A break leaves the loop it stands in, so it may only be written inside
 * one; the semicolon is left to the caller.
 */
AstNodePtr Parser::BreakStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    if (loop_level_ == 0 && switch_level_ == 0)
    {
        std::cerr << "syntax error: there is no loop or switch to break";
        std::cerr << " out of!" << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_Break, "break");

    return std::make_unique<AstBreak>();
}

/*
 * continue_statement: 'continue'  ;
 *
 * A continue begins the next pass of the loop it stands in, so it may only
 * be written inside one; the semicolon is left to the caller.
 */
AstNodePtr Parser::ContinueStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    if (loop_level_ == 0)
    {
        std::cerr << "syntax error: there is no loop to continue!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_Continue, "continue");

    return std::make_unique<AstContinue>();
}

/*
 * goto_statement: 'goto' identifier  ;
 *
 * A goto reaches a label of the function it stands in, which may be written
 * after it, so the name is remembered and looked for once the whole body
 * has been read rather than here.
 */
AstNodePtr Parser::GotoStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Goto, "goto");

    if (TokenTag(token_list[i]) != T_Identifier)
    {
        std::cerr << "syntax error: a goto needs the name of a label!";
        std::cerr << std::endl;
        std::exit(1);
    }

    const std::string name =
        static_cast<const Identifier*>(token_list[i].get())->GetName();
    i++;

    named_labels_.push_back(name);

    return std::make_unique<AstGoto>(name);
}

/*
 * label_statement: identifier ':' statement  ;
 *
 * The name is a label of the function the statement stands in, and no two
 * labels of one function may have the same name. What the label names is
 * the statement after it, which may be a ';' on its own and so nothing at
 * all; a statement which ends in a semicolon leaves that to the caller.
 */
ParsedStatement Parser::LabelStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    const std::string name =
        static_cast<const Identifier*>(token_list[i].get())->GetName();
    i++;

    Match(token_list, i, T_Colon, ":");

    if (std::find(labels_.begin(), labels_.end(), name) != labels_.end())
    {
        std::cerr << "syntax error: the label " << name;
        std::cerr << " is defined twice!" << std::endl;
        std::exit(1);
    }

    labels_.push_back(name);

    ParsedStatement labelled = Statement(token_list, i);
    AstNodePtr body = std::move(labelled.tree);

    return {std::make_unique<AstLabel>(body, name), labelled.semicolon};
}

/*
 * switch_statement: 'switch' '(' expression ')' '{' case_list '}'  ;
 *
 * Every arm is a compound statement, so that the end of one falls through
 * into the next and a break is what leaves the switch altogether. The
 * value a case names has to be an integer constant, which is a literal or
 * a name an enum gave to one, and no two cases may name the same value.
 */
AstNodePtr Parser::SwitchStatement(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Switch, "switch");
    Match(token_list, i, T_LParen, "(");

    AstNodePtr expression = BinaryExpression(token_list, i, 0);

    if (!IsIntType(expression->GetType()))
    {
        std::cerr << "syntax error: the switch expression is not an integer!";
        std::cerr << std::endl;
        std::exit(1);
    }

    Match(token_list, i, T_RParen, ")");
    Match(token_list, i, T_LBrace, "{");

    std::vector<AstNodePtr> cases;
    std::vector<int32> values;
    bool seen_default = false;

    /* Every arm may hold a break which leaves this switch. */
    switch_level_++;

    while (TokenTag(token_list[i]) != T_RBrace)
    {
        if (TokenTag(token_list[i]) == T_EOF)
        {
            std::cerr << "syntax error: expect }!" << std::endl;
            std::exit(1);
        }

        const NodeTag tag = TokenTag(token_list[i]);

        if (tag != T_Case && tag != T_Default)
        {
            std::cerr << "syntax error: expect a case or a default!";
            std::cerr << std::endl;
            std::exit(1);
        }

        /* The default arm is the last one, so nothing may follow it. */
        if (seen_default)
        {
            std::cerr << "syntax error: a case or a default after the";
            std::cerr << " default!" << std::endl;
            std::exit(1);
        }

        const bool is_default = tag == T_Default;
        int32 value = 0;

        if (is_default)
        {
            i++;
            seen_default = true;
        }
        else
        {
            i++;

            /*
             * The value has to be an integer constant, so it is read as an
             * expression and then asked to be one: `case pear` is one when
             * an enum gave pear its number and `case -1` is one because the
             * minus in front of a literal is part of the number, while
             * `case 3 * y` is not.
             */
            AstNodePtr value_tree =
                Optimise(BinaryExpression(token_list, i, 0));

            if (!ConstantValue(*value_tree, value))
            {
                std::cerr << "syntax error: a case value has to be an";
                std::cerr << " integer constant!" << std::endl;
                std::exit(1);
            }

            if (std::find(values.begin(), values.end(), value) !=
                values.end())
            {
                std::cerr << "syntax error: duplicate case value " << value;
                std::cerr << "!" << std::endl;
                std::exit(1);
            }

            values.push_back(value);
        }

        Match(token_list, i, T_Colon, ":");

        AstNodePtr body = CaseBody(token_list, i);

        cases.push_back(std::make_unique<AstCase>(body, value, is_default));
    }

    switch_level_--;

    i++;    /* step over the right brace */

    if (cases.empty())
    {
        std::cerr << "syntax error: a switch needs at least one case!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return std::make_unique<AstSwitch>(expression, cases);
}

namespace
{

/*
 * The type an argument takes when nothing says otherwise, which is the
 * type it is given on its way into a function of a variable number of
 * arguments: a value narrower than an int is widened to one, and every
 * other value, a long and a pointer among them, travels as it is.
 */
PrimitiveType PromotedType(PrimitiveType type)
{
    if (!IsIntType(type) ||
        PrimitiveSize(type) >= PrimitiveSize(PrimitiveType::kInt))
        return type;

    return PrimitiveType::kInt;
}

}   /* namespace */

/*
 * function_call: identifier '(' opt_expression_list ')'  ;
 *
 * The current token is the function's name, so a single token of lookahead
 * is all it takes to tell a call apart from a variable. Every argument is
 * an expression of its own, and each one has to fit the type of the
 * parameter it is given to.
 *
 * A function of a variable number of arguments names only the first few of
 * them, so a call to one may give more arguments than that; the ones beyond
 * the named ones are given whatever type an argument is given when nothing
 * says otherwise. Every call, whatever the function is, may give at most as
 * many arguments as a function may take parameters.
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

    const idx_t named = function.parameter_types.size();

    /*
     * A function which names every argument it takes wants exactly that
     * many; one which ends in '...' wants at least that many, the rest of
     * its arguments being the ones it reads for itself.
     */
    const bool wanted_count = function.is_variadic
        ? arguments.size() >= named
        : arguments.size() == named;

    if (!wanted_count)
    {
        std::cerr << "syntax error: wrong number of arguments for function ";
        std::cerr << function.name << ": " << arguments.size();
        std::cerr << " given, ";

        /*
         * The number a variadic function wants is a number it takes at
         * least, so the message counts up to it rather than naming one.
         */
        if (function.is_variadic)
            std::cerr << "at least ";

        std::cerr << named << " wanted!" << std::endl;
        std::exit(1);
    }

    /* Every argument travels the same way, so a call may give no more of
     * them than there are places for one to travel in. */
    if (arguments.size() > kMaxArguments)
    {
        std::cerr << "syntax error: a call may give at most ";
        std::cerr << kMaxArguments << " arguments!" << std::endl;
        std::exit(1);
    }

    for (idx_t k = 0; k < named; k++)
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

    for (idx_t k = named; k < arguments.size(); k++)
    {
        PrimitiveType wanted = PromotedType(arguments[k]->GetType());

        if (wanted != arguments[k]->GetType())
            arguments[k] = std::make_unique<AstWiden>(arguments[k], wanted);
    }

    Match(token_list, i, T_RParen, ")");

    return std::make_unique<AstFuncCall>(arguments, function);
}

/*
 * opt_parameter_list: <empty>
 *      |              parameter_list
 *      |              parameter_list ',' '...'
 *      ;
 *
 * parameter_list: type identifier
 *      |        type identifier ',' parameter_list
 *      ;
 *
 * The type and the name of every parameter are read here, and the `...`
 * which may close the list says that the function takes more arguments
 * than it names parameters. Nothing is added to the symbol table yet,
 * because a declaration which turns out to be a prototype keeps only the
 * types of its parameters; a definition binds them once its body is known
 * to follow.
 */
std::vector<Symbol> Parser::FunctionParameters(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    bool& is_variadic)
{
    std::vector<Symbol> parameters;

    is_variadic = false;

    if (TokenTag(token_list[i]) == T_RParen)
        return parameters;

    /*
     * The extra arguments of a variadic function are the ones left over
     * once the named parameters have taken theirs, so there has to be a
     * parameter for the '...' to stand after. `int printf(...);` names no
     * parameter at all and is refused rather than read as a function which
     * takes any number of arguments.
     */
    if (TokenTag(token_list[i]) == T_Ellipsis)
    {
        std::cerr << "syntax error: the '...' of a function needs a";
        std::cerr << " parameter before it!" << std::endl;
        std::exit(1);
    }

    /*
     * A `void` which is the whole parameter list says that the function
     * takes nothing, which is what `int fred(void);` means. `void *` is a
     * parameter like any other and is not this: it is followed by a star
     * rather than by the ')' which ends the list.
     */
    if (TokenTag(token_list[i]) == T_Void &&
        TokenTag(token_list[i + 1]) == T_RParen)
    {
        i++;    /* step over the void */
        return parameters;
    }

    while (true)
    {
        if (parameters.size() == kMaxArguments)
        {
            std::cerr << "syntax error: a function takes at most ";
            std::cerr << kMaxArguments << " parameters!" << std::endl;
            std::exit(1);
        }

        TypeRef type = ParseType(token_list, i);
        type = ParseStars(token_list, i, type);

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

        /*
         * What a function is given is the address of the array rather than
         * the array itself, so a parameter which is an array is written as
         * a pointer, which is what its type says.
         */
        if (TokenTag(token_list[i]) == T_LBracket)
        {
            std::cerr << "syntax error: write a parameter which is given an";
            std::cerr << " array as a pointer, as in int *" << name << "!";
            std::cerr << std::endl;
            std::exit(1);
        }

        parameters.push_back(Symbol{.name = name,
                                    .type = type.type,
                                    .composite = type.composite,
                                    .stype = StructuralType::kVariable,
                                    .storage = StorageClass::kLocal});

        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;

        /*
         * The list may close with the '...' of a function which takes more
         * arguments than it names parameters, which stands where the next
         * parameter would have been.
         */
        if (TokenTag(token_list[i]) == T_Ellipsis)
        {
            i++;    /* step over the dots */
            is_variadic = true;
            break;
        }
    }

    return parameters;
}

/*
 * Every declaration of a function has to agree with the one which came
 * before it: the same return type, the same number of parameters and the
 * same type for each of them in turn, since a caller which was checked
 * against one of them has to keep working with the other. Whether the
 * function takes more arguments than it names parameters has to agree as
 * well, because that is what says whether a call may give it more.
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

    if (previous.is_variadic != symbol.is_variadic)
    {
        std::cerr << "syntax error: function " << symbol.name;
        std::cerr << " does not take the same number of arguments as its";
        std::cerr << " prototype!" << std::endl;
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
        /*
         * `?` is not a binary operator: what follows it is the value taken
         * when what came before it holds, a ':' and the value taken when it
         * does not. Both values are worked out from here, so that the one
         * which is not taken never runs.
         */
        if (TokenTag(token_list[i]) == T_Question)
        {
            i++;    /* step over '?' */

            /*
             * What stands before the '?' is a question like the condition of
             * an if, so it is asked for its truth value the same way, and
             * only the value it chooses is worked out when the program
             * runs.
             */
            MakeCondition(left);

            AstNodePtr when_true = BinaryExpression(token_list, i, 0);
            Match(token_list, i, T_Colon, ":");
            AstNodePtr when_false = BinaryExpression(token_list, i, 0);

            left = MakeTernary(left, when_true, when_false);

            continue;
        }

        idx_t op_idx = i;
        uint8 op_prec = GetOpPrecedence(TokenTag(token_list[op_idx]));

        i++;

        AstNodePtr right = BinaryExpression(token_list, i, op_prec);
        NodeTag op_type = TokenTag(token_list[op_idx]);

        if (op_type == T_Assign || IsCompoundAssign(op_type))
        {
            /*
             * An assignment is the one operator whose two operands are not
             * made to agree with each other: the target keeps the type it
             * was declared with, and it is the value which has to fit it.
             * A compound assignment combines that value with the one the
             * target holds, so the two have to agree the way the operator
             * it combines with would make them.
             *
             * The two are then switched around, so that the value ends up
             * in the left child and the target in the right one. That is
             * the order the code generator wants: the value has to be
             * worked out before the location it is stored in.
             */
            if (!IsLvalue(left->GetAstNodeTag()) || IsArrayMember(left))
            {
                std::cerr << "syntax error: an assignment can only store";
                std::cerr << " into a variable or into a * pointer!";
                std::cerr << std::endl;
                std::exit(1);
            }

            /*
             * A composite value cannot be read as a value, so an
             * assignment of one is not a value made to fit the target but
             * the room of the target given what another place holds. That
             * is only possible with '=', since there is no operation which
             * combines two composite values, and only from a place holding
             * the same composite type, since there is nothing else the
             * room could be taken from.
             */
            if (left->GetType() == PrimitiveType::kComposite)
            {
                if (IsCompoundAssign(op_type))
                {
                    std::cerr << "syntax error: a composite value cannot be";
                    std::cerr << " combined with another, only given one as a";
                    std::cerr << " whole!" << std::endl;
                    std::exit(1);
                }

                if (right->GetType() != PrimitiveType::kComposite ||
                    right->GetComposite() != left->GetComposite() ||
                    !IsLvalue(right->GetAstNodeTag()))
                {
                    std::cerr << "syntax error: a composite value can only be";
                    std::cerr << " given the contents of a place holding one";
                    std::cerr << " of the same type!" << std::endl;
                    std::exit(1);
                }

                std::swap(left, right);
            }
            else
            {
                /*
                 * A plain assignment stores the value as it stands, so
                 * there is no operator for it to be made to fit through; a
                 * compound one combines the value with what the target
                 * holds.
                 */
                std::optional<NodeTag> combine;

                if (IsCompoundAssign(op_type))
                    combine = CompoundAssignOperator(op_type);

                if (!ModifyType(right,
                                TypeRef{.type = left->GetType(),
                                        .composite = left->GetComposite()},
                                combine))
                {
                    std::cerr << "syntax error: this value does not fit the";
                    std::cerr << " target of the assignment!" << std::endl;
                    std::exit(1);
                }

                std::swap(left, right);
            }
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

            /*
             * '&&' and '||' ask a question about each of their two sides
             * rather than combining their values, so the two sides need not
             * be of one type: each has to be a value which can be asked
             * whether it holds, which is an integer or an address. An
             * address holds when it does not point at nothing.
             */
            if (op_type == T_LogAnd || op_type == T_LogOr)
            {
                if (!IsTestable(left) || !IsTestable(right))
                {
                    std::cerr << "syntax error: && and || need a value which";
                    std::cerr << " can be asked whether it holds!" << std::endl;
                    std::exit(1);
                }
            }
            else
            {
                /*
                 * Each side is given the type of the other one, and the
                 * definition of a composite travels with the type: a
                 * pointer to a composite is stepped along by the room the
                 * composite takes, which is only known from it.
                 */
                const TypeRef left_wanted{.type = left_type,
                                          .composite = left->GetComposite()};
                const TypeRef right_wanted{.type = right_type,
                                           .composite = right->GetComposite()};

                bool left_fits = ModifyType(left, right_wanted, op_type);
                bool right_fits = ModifyType(right, left_wanted, op_type);

                if (!left_fits && !right_fits)
                {
                    std::cerr << "syntax error: incompatible types!";
                    std::cerr << std::endl;
                    std::exit(1);
                }
            }
        }

        /*
         * '&&' and '||' answer with the zero or the one which says whether
         * they hold, whatever the two sides are, and the side which is not
         * needed to know the answer is not worked out when the program
         * runs: `if (p && *p == 1)` must not read through a pointer which
         * points at nothing. The two sides are therefore left as they are,
         * for the code generator to ask one after the other.
         */
        if (op_type == T_LogAnd || op_type == T_LogOr)
        {
            left = std::make_unique<AstOperator>(left, right, op_type,
                                                 PrimitiveType::kInt);
            continue;
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

            /*
             * The name of an array already stands for the address of its
             * first element, and there is no value which is the address of
             * the whole array, so `&ary` is refused: only an element of it
             * has an address of its own, as `&ary[2]` does.
             */
            if (IsArrayAddress(operand))
            {
                std::cerr << "syntax error: the address of a whole array";
                std::cerr << " is not supported yet!" << std::endl;
                std::exit(1);
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

            /* Only a number has a negative one. */
            if (!IsIntType(operand->GetType()))
            {
                std::cerr << "syntax error: - needs a number!";
                std::cerr << std::endl;
                std::exit(1);
            }

            /*
             * A char is unsigned, so there is no such thing as a negative
             * one: it is widened to a short, which holds the negative of
             * every char, before the sign flips. A value which is already
             * as wide as a short keeps its own width, so that a short has
             * a negative one of its own, and so have an int and a long.
             */
            const PrimitiveType type = operand->GetType();

            if (PrimitiveSize(type) < PrimitiveSize(PrimitiveType::kShort))
                ModifyType(operand, PrimitiveType::kShort, std::nullopt);

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
            const bool increment = TokenTag(token_list[i]) == T_Inc;

            i++;

            /*
             * The value has somewhere to go back to, so what is written
             * before the operator is a place, and the answer is the value
             * the place holds once it has been changed.
             */
            AstNodePtr place = PostfixExpression(token_list, i);

            CheckIncrementable(place);

            return std::make_unique<AstIncDec>(
                place, IncrementStep(place, increment), false);
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
             * A literal is given the narrowest type which holds it, so
             * that it can be stored where a value of that type is wanted:
             * a small one which has no sign is a char, one which is too
             * big for a char is a short, one which is too big for an int
             * of the machine the compiler is built for is a long, and a
             * value which is too big for a long as well is rejected
             * where it is used.
             */
            PrimitiveType type = PrimitiveType::kInt;

            if (value >= 0 && value < 256)
                type = PrimitiveType::kChar;
            else if (ValueFitsType(value, PrimitiveType::kShort))
                type = PrimitiveType::kShort;
            else if (!ValueFitsType(value, PrimitiveType::kInt))
                type = PrimitiveType::kLong;

            const NodePtr& token = token_list[i];
            i++;

            return MakeIntLitLeaf(token, type);
        }
        case T_StrLit:
            return std::make_unique<AstStrLit>(
                StringLiteralText(token_list, i));
        case T_Sizeof:
        {
            Match(token_list, i, T_Sizeof, "sizeof");
            Match(token_list, i, T_LParen, "(");

            /*
             * What stands between the parentheses is a type when it begins
             * like one and a value otherwise, the same way a cast is told
             * from a parenthesised expression.
             */
            if (StartsCast(token_list, i))
            {
                TypeRef type = ParseType(token_list, i);
                type = ParseStars(token_list, i, type);
                Match(token_list, i, T_RParen, ")");

                return MakeIntLitLeaf(SizeOf(type), PrimitiveType::kInt);
            }

            /*
             * The value is written there to say which type is asked about,
             * and is not worked out at all: `sizeof(count= 5)` answers with
             * the room an int takes and leaves count as it was.
             */
            AstNodePtr value = BinaryExpression(token_list, i, 0);
            Match(token_list, i, T_RParen, ")");

            return MakeIntLitLeaf(SizeOfValue(value), PrimitiveType::kInt);
        }
        case T_LParen:
        {
            /*
             * A '(' may begin a cast, which names a type straight after it,
             * or a parenthesised expression, which does not.
             */
            if (StartsCast(token_list, i + 1))
            {
                i++;    /* step over '(' */

                TypeRef type = ParseCast(token_list, i);
                Match(token_list, i, T_RParen, ")");

                /*
                 * A cast holds on to the one expression which follows it, the
                 * way a unary operator does, so `(char)x + 1` adds one to the
                 * cast value rather than casting the sum.
                 */
                AstNodePtr expression = PrefixExpression(token_list, i);

                return std::make_unique<AstCast>(expression, type);
            }

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
             * An enum value is a name for a number, so reading it yields
             * the number it was declared with and not a place holding it.
             */
            if (symbol.stype == StructuralType::kEnumValue)
                return MakeIntLitLeaf(symbol.enum_value, PrimitiveType::kInt);

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
            /*
             * A '++' or a '--' written after a place changes what the
             * place holds and yields the value it held before the change.
             * The place is anything the new value can go back into, so the
             * name of a variable, a place a pointer reaches and a member
             * of a value are all taken here.
             */
            const bool increment = tag == T_Inc;

            i++;
            CheckIncrementable(expression);

            expression = std::make_unique<AstIncDec>(
                expression, IncrementStep(expression, increment), true);
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

    /*
     * The name may be a member of an anonymous member as well as one of the
     * type itself, so what is looked up is the path of members which
     * reaches it: the member the name is written for, or an anonymous
     * member and the way through it.
     */
    const std::shared_ptr<CompositeType> composite = base->GetComposite();
    std::vector<const Symbol*> path;

    if (!MemberPath(composite, name, path)) {
        std::cerr << "syntax error: composite type has no member " << name << "!"
                  << std::endl;
        std::exit(1);
    }

    /*
     * A member stands at its own offset from the base of what holds it, so
     * a name reached through an anonymous member is the place of the
     * anonymous member with the place of the member inside it, and the two
     * offsets add up. Only the first step is taken through a pointer, since
     * that is the one the value itself is.
     */
    AstNodePtr place = std::move(base);

    for (idx_t k = 0; k < path.size(); k++)
        place = std::make_unique<AstMember>(place, *path[k],
                                            through_pointer && k == 0);

    return place;
}

bool Parser::MemberPath(const std::shared_ptr<CompositeType>& composite,
                        const std::string& name,
                        std::vector<const Symbol*>& path) const
{
    /*
     * A member written for the name is the one meant, whether or not an
     * anonymous member also reaches it: two members of one type never share
     * a name, so at most one of them is.
     */
    for (const Symbol& member : composite->members)
    {
        if (member.name == name)
        {
            path.push_back(&member);
            return true;
        }
    }

    for (const Symbol& member : composite->members)
    {
        if (!member.name.empty() || !member.composite)
            continue;

        path.push_back(&member);

        if (MemberPath(member.composite, name, path))
            return true;

        path.pop_back();
    }

    return false;
}

void Parser::CollectMemberNames(
    const std::shared_ptr<CompositeType>& composite,
    std::vector<std::string>& names) const
{
    for (const Symbol& member : composite->members)
    {
        if (!member.name.empty())
            names.push_back(member.name);
        else if (member.composite)
            CollectMemberNames(member.composite, names);
    }
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
     * scaling it by the size of one element works out. An element which is
     * a composite takes the room its own definition asks for, which the
     * type of what is indexed carries with it. Only an integer counts
     * elements, so anything else is refused here rather than scaled into
     * an offset which would not mean anything.
     */
    if (!ModifyType(index, TypeRef{.type = base_type,
                                   .composite = base->GetComposite()},
                    T_Plus))
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
 * Parse the base type which starts a declaration and step over it, without
 * the stars: those belong to the name they stand in front of and are read
 * with it, so that each name of a list can bring its own.
 */
TypeRef Parser::ParseType(const std::vector<NodePtr>& token_list, idx_t& i)
{
    TypeRef type;

    /*
     * Only a definition of a tag may stand alone in a declaration, so the
     * flag is cleared here and set by the two cases which are one.
     */
    type_definition_ = false;

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
            type_definition_ = true;
            break;
        /*
         * An enum is a tag of its own, but a value of it is an int, so
         * the type which comes back is one whatever the enum was called.
         */
        case T_Enum:
            type = EnumType(token_list, i);
            type_definition_ = true;
            break;
        case T_Char:
            type.type = PrimitiveType::kChar;
            i++;
            break;
        case T_Short:
            type.type = PrimitiveType::kShort;
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
        /*
         * Any other name for a type is one made with typedef, which the
         * symbol table says the type behind.
         */
        case T_Identifier:
            type = TypedefType(token_list[i]);
            i++;
            break;
        default:
            std::cerr << "syntax error: expect a type!" << std::endl;
            std::exit(1);
    }

    return type;
}

/*
 * Whether a '(' begins a cast, which is a type written straight after it: a
 * type keyword, or a name made with typedef. Anything else begins a
 * parenthesised expression.
 */
bool Parser::StartsCast(const std::vector<NodePtr>& token_list, idx_t i) const
{
    switch (TokenTag(token_list[i]))
    {
        case T_Void:
        case T_Char:
        case T_Short:
        case T_Int:
        case T_Long:
        case T_Struct:
        case T_Union:
        case T_Enum:
            return true;
        case T_Identifier:
            return IsTypedefName(token_list[i]);
        default:
            return false;
    }
}

/*
 * Parse the type a cast names, which stands between the '(' and its ')', and
 * step over it. A cast may name any type which can hold a value, so a
 * struct, a union and a plain void are refused: a pointer to void is a value
 * which stands for any other pointer, and is allowed.
 */
TypeRef Parser::ParseCast(const std::vector<NodePtr>& token_list, idx_t& i)
{
    TypeRef type = ParseType(token_list, i);
    type = ParseStars(token_list, i, type);

    if (type.type == PrimitiveType::kComposite ||
        type.type == PrimitiveType::kVoid)
    {
        std::cerr << "syntax error: a cast cannot name a struct, a union or";
        std::cerr << " void!" << std::endl;
        std::exit(1);
    }

    return type;
}

/*
 * opt_pointer: <empty>
 *      |      '*' opt_pointer
 *      ;
 *
 * Add a level of indirection to the base type for every star which stands
 * in front of one name, which is what makes `int *b` a pointer to an int
 * and `int a, *b` a variable and a pointer in one declaration.
 */
TypeRef Parser::ParseStars(const std::vector<NodePtr>& token_list,
    idx_t& i,
    const TypeRef& base)
{
    TypeRef type = base;

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
 * enum_type: 'enum' opt_tag
 *      |   'enum' opt_tag '{' enum_value_list '}'
 *      ;
 *
 * An enum shares the tag namespace with structs and unions, so a tag
 * which is already a struct or a union cannot become an enum, and one
 * which is already an enum cannot be defined over. Every value between
 * the braces is a name for a number in the ordinary namespace, where a
 * variable also lives, so a name may be used once and no more.
 */
TypeRef Parser::EnumType(const std::vector<NodePtr>& token_list, idx_t& i)
{
    i++;    /* step over the keyword */

    std::string name;
    std::shared_ptr<CompositeType> tag;

    if (TokenTag(token_list[i]) == T_Identifier)
    {
        name = MatchIdentifier(token_list, i, "an enum name");
        tag = symbol_table_.FindTag(name);

        /* A tag of another kind is a definition of the same name already. */
        if (tag && !tag->is_enum)
        {
            std::cerr << "syntax error: enum " << name
                      << " is already defined!" << std::endl;
            std::exit(1);
        }
    }

    /* Without a brace this only names a tag which is already known. */
    if (TokenTag(token_list[i]) != T_LBrace)
    {
        if (!tag)
        {
            std::cerr << "syntax error: there is no enum " << name << "!";
            std::cerr << std::endl;
            std::exit(1);
        }

        return TypeRef{.type = PrimitiveType::kInt};
    }

    if (tag)
    {
        std::cerr << "syntax error: enum " << name
                  << " is already defined!" << std::endl;
        std::exit(1);
    }

    tag = std::make_shared<CompositeType>();
    tag->is_enum = true;
    tag->name = std::move(name);

    if (!tag->name.empty())
        symbol_table_.AddTag(tag);

    Match(token_list, i, T_LBrace, "{");

    /*
     * The number the next name stands for, counting up from zero and
     * starting again from every number written out.
     */
    int32 value = 0;

    while (true)
    {
        std::string value_name =
            MatchIdentifier(token_list, i, "an enum value name");

        if (symbol_table_.FindSymbol(value_name))
        {
            std::cerr << "syntax error: enum value " << value_name
                      << " is already defined!" << std::endl;
            std::exit(1);
        }

        if (TokenTag(token_list[i]) == T_Assign)
        {
            i++;
            value = ParseConstantNumber(token_list, i, "an enum value");
        }

        symbol_table_.AddSymbol(Symbol{.name = value_name,
                                       .type = PrimitiveType::kInt,
                                       .stype = StructuralType::kEnumValue,
                                       .enum_value = value});
        value++;

        if (TokenTag(token_list[i]) != T_Comma)
            break;

        i++;
    }

    Match(token_list, i, T_RBrace, "}");

    return TypeRef{.type = PrimitiveType::kInt};
}

/*
 * typedef_declaration: 'typedef' type identifier  ;
 *
 * The type is read with the same rule which reads the type of any other
 * declaration, so a typedef may name a type made with another typedef, a
 * struct, a union or an enum as well as a primitive one. The name becomes
 * another way of writing that type and is looked up wherever a type is
 * expected.
 */
void Parser::TypedefDeclaration(const std::vector<NodePtr>& token_list,
    idx_t& i)
{
    Match(token_list, i, T_Typedef, "typedef");

    TypeRef type = ParseType(token_list, i);

    /* The stars before the name are part of the type it names, so
     * `typedef char *charptr;` gives a name to a pointer to a char. */
    type = ParseStars(token_list, i, type);

    std::string name = MatchIdentifier(token_list, i, "a type name");

    if (symbol_table_.FindSymbol(name))
    {
        std::cerr << "syntax error: " << name << " is already defined!";
        std::cerr << std::endl;
        std::exit(1);
    }

    symbol_table_.AddSymbol(Symbol{.name = name,
                                   .type = type.type,
                                   .composite = type.composite,
                                   .stype = StructuralType::kTypedef});
}

/*
 * member_list: type identifier ';'
 *      |       type identifier '[' expression ']' ';'
 *      |       'struct' '{' member_list '}' ';'
 *      |       'union' '{' member_list '}' ';'
 *      |       member_list member_list
 *      ;
 *
 * A struct lines its members up one after another, each at its own
 * alignment. A union puts every member at the start and takes as much
 * room as the largest one, rounded up to the alignment they share.
 *
 * A member written as an unnamed struct or union definition has no name of
 * its own, and the names of its members are the ones reached from the type
 * which holds it: the two are one piece of room written two ways. Such a
 * member is held with no name, since every other member is given one where
 * it is declared and a name is what tells them apart.
 */
void Parser::ParseMembers(const std::vector<NodePtr>& token_list,
    idx_t& i,
    const std::shared_ptr<CompositeType>& type)
{
    Match(token_list, i, T_LBrace, "{");

    /*
     * Every name the type reaches so far, the ones an anonymous member
     * brings with it among them. Two members of one type may never share a
     * name, since a name written for a value has to say which member is
     * meant.
     */
    std::vector<std::string> reachable;

    while (TokenTag(token_list[i]) != T_RBrace)
    {
        if (TokenTag(token_list[i]) == T_EOF)
        {
            std::cerr << "syntax error: expect }!" << std::endl;
            std::exit(1);
        }

        TypeRef member_type = ParseType(token_list, i);

        /*
         * A member written as an unnamed struct or union definition and
         * nothing else is an anonymous member, whose own members are the
         * names reached from this type.
         */
        const bool is_anonymous =
            member_type.type == PrimitiveType::kComposite &&
            member_type.composite && member_type.composite->name.empty() &&
            TokenTag(token_list[i]) == T_Semicolon;

        member_type = ParseStars(token_list, i, member_type);

        /*
         * A definition written with a tag and nothing after it names the
         * tag and no member of this type, the way `struct fred { int x;
         * };` does where a variable is declared.
         */
        if (!is_anonymous && TokenTag(token_list[i]) == T_Semicolon &&
            type_definition_)
        {
            Match(token_list, i, T_Semicolon, ";");
            continue;
        }

        std::string name;

        if (!is_anonymous)
            name = MatchIdentifier(token_list, i, "a member name");

        Symbol member{.name = name,
                      .type = member_type.type,
                      .composite = member_type.composite,
                      .stype = StructuralType::kVariable,
                      .storage = StorageClass::kMember};

        /*
         * What this member adds to the names the type reaches: its own
         * name, or every name an anonymous member brings with it.
         */
        std::vector<std::string> added;

        if (is_anonymous)
            CollectMemberNames(member.composite, added);
        else
            added.push_back(name);

        for (const std::string& candidate : added)
        {
            if (std::any_of(reachable.begin(), reachable.end(),
                [&candidate](const std::string& previous) {
                    return previous == candidate;
                })) {
                std::cerr << "syntax error: duplicate composite member "
                          << candidate << "!" << std::endl;
                std::exit(1);
            }
        }

        reachable.insert(reachable.end(), added.begin(), added.end());

        if (TokenTag(token_list[i]) == T_LBracket)
        {
            /*
             * A member is given no value where it is declared: the room a
             * value of the struct takes is what the members are for, and
             * the value is put in when the program runs.
             */
            AstNodePtr no_initialiser = nullptr;

            member = ArrayDeclaration(token_list, i, member_type, name,
                                      StorageClass::kMember, no_initialiser);
        }

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

TypeRef Parser::TypedefType(const NodePtr& token) const
{
    const Identifier *ident = static_cast<const Identifier *>(token.get());

    std::optional<Symbol> symbol = symbol_table_.FindSymbol(ident->GetName());

    if (!symbol || symbol->stype != StructuralType::kTypedef)
    {
        std::cerr << "syntax error: unknown type " << ident->GetName() << "!";
        std::cerr << std::endl;
        std::exit(1);
    }

    return TypeRef{.type = symbol->type, .composite = symbol->composite};
}

Parser::DeclarationPrefix Parser::ParseDeclarationPrefix(
    const std::vector<NodePtr>& token_list,
    idx_t& i,
    StorageClass plain)
{
    DeclarationPrefix prefix;
    prefix.storage = plain;

    bool seen_extern = false;
    bool seen_static = false;

    while (TokenTag(token_list[i]) == T_Extern ||
           TokenTag(token_list[i]) == T_Static)
    {
        const bool is_extern = TokenTag(token_list[i]) == T_Extern;

        if (is_extern && seen_static)
        {
            std::cerr << "syntax error: a name cannot be both extern and";
            std::cerr << " static!" << std::endl;
            std::exit(1);
        }

        if (!is_extern && seen_extern)
        {
            std::cerr << "syntax error: a name cannot be both extern and";
            std::cerr << " static!" << std::endl;
            std::exit(1);
        }

        /*
         * A static name belongs to the file it is written in, so it can
         * only be declared where the whole file can be seen rather than
         * inside one function.
         */
        if (!is_extern && plain == StorageClass::kLocal)
        {
            std::cerr << "syntax error: a static declaration is only";
            std::cerr << " supported at the top level of a program!";
            std::cerr << std::endl;
            std::exit(1);
        }

        if (is_extern)
        {
            seen_extern = true;
            prefix.storage = StorageClass::kExtern;
        }
        else
        {
            seen_static = true;
            prefix.is_static = true;
        }

        i++;
    }

    return prefix;
}

bool Parser::IsTypedefName(const NodePtr& token) const
{
    if (token->GetNodeTag() != T_Identifier)
        return false;

    const Identifier *ident = static_cast<const Identifier *>(token.get());
    std::optional<Symbol> symbol = symbol_table_.FindSymbol(ident->GetName());

    return symbol && symbol->stype == StructuralType::kTypedef;
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

AstNodePtr Parser::MakeIntLitLeaf(int32 value, PrimitiveType type)
{
    AstNodePtr left = nullptr;
    AstNodePtr right = nullptr;

    return std::make_unique<AstIntLit>(left, right, value, type);
}

/*
 * Build a ternary expression out of the condition and the two values it
 * chooses between. Only one of the two is worked out when the program
 * runs, so both have to be of the type the expression answers with: each
 * is tried against the other the way the two sides of an operator are, and
 * whichever can be made to fit the other says what type the expression is.
 */
AstNodePtr Parser::MakeTernary(AstNodePtr& condition,
    AstNodePtr& when_true,
    AstNodePtr& when_false)
{
    TypeRef type{.type = when_true->GetType(),
                 .composite = when_true->GetComposite()};

    /*
     * An expression whose two values are calls which answer with nothing is
     * one of those itself: there is no value for the two to agree about.
     */
    if (type.type == PrimitiveType::kVoid &&
        when_false->GetType() == PrimitiveType::kVoid)
        return std::make_unique<AstTernary>(condition, when_true, when_false,
                                            PrimitiveType::kVoid, nullptr);

    if (!ModifyType(when_false, type, std::nullopt))
    {
        TypeRef other{.type = when_false->GetType(),
                      .composite = when_false->GetComposite()};

        if (!ModifyType(when_true, other, std::nullopt))
        {
            std::cerr << "syntax error: the two values of a ternary";
            std::cerr << " expression have to be of one type!" << std::endl;
            std::exit(1);
        }

        type = other;
    }

    return std::make_unique<AstTernary>(condition, when_true, when_false,
                                        type.type, type.composite);
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
    /*
     * A comparison answers with the zero or the one which says which way it
     * went, whatever the two values it was given are; every other operator
     * answers with the type of its left side.
     */
    PrimitiveType type = IsComparisonOperator(node->GetNodeTag())
                             ? PrimitiveType::kInt
                             : left->GetType();

    switch (node->GetNodeTag())
    {
        case T_Plus:
        case T_Minus:
        case T_Star:
        case T_Slash:
        case T_Percent:
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
        case T_PlusAssign:
        case T_MinusAssign:
        case T_StarAssign:
        case T_SlashAssign:
        case T_PercentAssign:
        case T_AmperAssign:
        case T_OrAssign:
        case T_XorAssign:
        case T_LShiftAssign:
        case T_RShiftAssign:
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
    return IsComparisonOp(tag);
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
    /* Every assignment, plain or compound, takes what stands on its right
     * first, so `a= b= 3` and `a += b += 3` both start at the right. */
    return tag == T_Assign || IsCompoundAssign(tag);
}

/*
 * Whether two declarations of one name say the same thing about it, which
 * is what has to hold when a global name and an extern name of the same
 * name are written together.
 */
namespace
{

bool SameDeclaration(const Symbol& left, const Symbol& right)
{
    return left.stype == right.stype &&
           left.type == right.type &&
           left.composite == right.composite &&
           left.is_static == right.is_static &&
           left.element_count == right.element_count;
}

}   /* namespace */

void Parser::AddGlobalSymbol(const Symbol& symbol)
{
    std::optional<Symbol> previous = symbol_table_.FindSymbol(symbol.name);

    /* A name which has not been declared before is simply added. */
    if (!previous)
    {
        symbol_table_.AddSymbol(symbol);
        return;
    }

    /*
     * Otherwise the two declarations have to be the one pair which may
     * stand together: a global name and an extern name of the same one. One
     * says where the storage is and the other says nothing about it, so the
     * table keeps the declaration which came last, which is the one which
     * knows.
     */
    const bool pair = (previous->storage == StorageClass::kGlobal &&
                       symbol.storage == StorageClass::kExtern) ||
                      (previous->storage == StorageClass::kExtern &&
                       symbol.storage == StorageClass::kGlobal);

    if (!pair || !SameDeclaration(*previous, symbol))
    {
        std::cerr << "syntax error: " << symbol.name;
        std::cerr << " is already declared at the top level!" << std::endl;
        std::exit(1);
    }

    symbol_table_.UpdateSymbol(symbol);
}

bool Parser::IsTestable(const AstNodePtr& tree)
{
    return IsIntType(tree->GetType()) || IsPointerType(tree->GetType());
}

int32 Parser::IncrementStep(const AstNodePtr& place, bool increment) const
{
    const PrimitiveType type = place->GetType();
    int32 step = 1;

    /*
     * A pointer moves on by the room one value of what it points at takes
     * rather than by one byte. A composite takes the room its definition
     * asks for, which travels with the type of the place, and every other
     * type takes the room its own type gives it.
     */
    if (IsPointerType(type))
        step = ValueSize(ValueAt(type), place->GetComposite());

    return increment ? step : -step;
}

void Parser::CheckIncrementable(const AstNodePtr& operand) const
{
    /*
     * The new value has somewhere to go back to when what is written is a
     * place: the name of a variable, a place a pointer reaches, or a member
     * of a value. Everything else leaves it nowhere: the name of an array
     * stands for an address rather than a place, and so does the name of an
     * array which is a member; an increment is not a place at all; and a
     * value worked out is not a place either.
     */
    if (IsArrayAddress(operand) || IsArrayMember(operand))
    {
        std::cerr << "syntax error: an array cannot be changed!";
        std::cerr << std::endl;
    }
    else if (operand->GetAstNodeTag() == A_AstIncDec)
    {
        std::cerr << "syntax error: an increment or a decrement cannot be";
        std::cerr << " changed again!" << std::endl;
    }
    else if (operand->GetType() == PrimitiveType::kComposite)
    {
        /*
         * A composite value cannot be read as a value, so there is nothing
         * for the operator to add one to.
         */
        std::cerr << "syntax error: a whole composite value cannot be";
        std::cerr << " incremented or decremented!" << std::endl;
    }
    else if (!IsLvalue(operand->GetAstNodeTag()))
    {
        std::cerr << "syntax error: increment and decrement need a variable";
        std::cerr << " or a place a pointer reaches!" << std::endl;
    }
    else
        return;

    std::exit(1);
}

bool Parser::IsArrayAddress(const AstNodePtr& tree)
{
    if (tree->GetAstNodeTag() != A_AstAddress)
        return false;

    return static_cast<const AstAddress&>(*tree).GetSymbol().stype ==
           StructuralType::kArray;
}

bool Parser::IsArrayMember(const AstNodePtr& tree)
{
    if (tree->GetAstNodeTag() != A_AstMember)
        return false;

    return static_cast<const AstMember&>(*tree).GetMember().stype ==
           StructuralType::kArray;
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
    {T_PlusAssign, 10}, {T_MinusAssign, 10},
    {T_StarAssign, 10}, {T_SlashAssign, 10}, {T_PercentAssign, 10},
    {T_AmperAssign, 10}, {T_OrAssign, 10}, {T_XorAssign, 10},
    {T_LShiftAssign, 10}, {T_RShiftAssign, 10},

    {T_Question, 15},

    {T_LogOr, 20},

    {T_LogAnd, 30},

    {T_Or, 40},

    {T_Xor, 50},

    {T_Amper, 60},

    {T_EQ, 70}, {T_NE, 70},

    {T_LT, 80}, {T_GT, 80}, {T_LE, 80}, {T_GE, 80},

    {T_LShift, 90}, {T_RShift, 90},

    {T_Plus, 100}, {T_Minus, 100},

    {T_Star, 110}, {T_Slash, 110}, {T_Percent, 110}
};

}   /* namespace nuocc */

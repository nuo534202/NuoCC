#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "nodes/nuocc_nodes_tag.hpp"
#include "nodes/nuocc_nodes.hpp"
#include "utils/nuocc_symbol_table.hpp"
#include "utils/nuocc_type_check.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

/*
 * One statement of a compound statement. The grammar ends most statements
 * with a semicolon, but a statement which ends in a compound statement is
 * complete on its own. The caller matches the semicolon rather than the
 * rule which parses the statement, because the same rules also parse the
 * three parts of a for loop, where a semicolon separates them instead of
 * ending them.
 */
struct ParsedStatement
{
    /*
     * The tree of the statement. One statement can declare more than one
     * variable, so the tree can hold more than one node.
     */
    AstNodePtr tree;
    /* Whether the grammar follows the statement with a semicolon. */
    bool semicolon = false;
};

class Parser
{
private:
    /*
     * One name a declaration declared, and what has to run to give it the
     * value it starts with. A global name has that value stored in its own
     * storage and needs nothing to run; a local one is given it by an
     * assignment which stands where the declaration does.
     */
    struct DeclaredName
    {
        Symbol symbol;
        AstNodePtr initialiser = nullptr;
    };

    /*
     * What stands in front of the type of a declaration: `extern`, which
     * says the storage of the name belongs to another source file, or
     * `static`, which says the name is visible only in this one. Neither
     * changes where the storage of a global name is put; what changes is
     * who may reach it.
     */
    struct DeclarationPrefix
    {
        StorageClass storage = StorageClass::kGlobal;
        bool is_static = false;
    };

    /* Operator precedence: every binary operator has a precedence above 0 */
    static const std::unordered_map<NodeTag, uint8> kOpPrecedence;

public:
    /*
     * Parse the whole token list and return the program. A program is a
     * list of global declarations, each of them a function or a list of
     * global variables.
     */
    Program Parse(const std::vector<NodePtr>& token_list);

    /*
     * The names the file declared, which is what the dump of the symbol
     * table reads once the whole file has been parsed.
     */
    const SymbolTable& GetSymbolTable() const;

private:
    /*
     * global_declaration: function_declaration | var_declaration  ;
     *
     * Both start with a type and a name, so it is the token after the name
     * which tells them apart: a '(' begins the parameter list of a function,
     * a '[' the size of an array, and a ',' or a ';' continues a list of
     * variables. A function is told from a variable before its name is read,
     * because the stars of a declaration come before the name and a list of
     * names may follow it.
     */
    void GlobalDeclaration(const std::vector<NodePtr>& token_list,
        idx_t& i,
        Program& program);

    /*
     * function_declaration: type identifier '(' opt_parameter_list ')'
     *      |                type identifier '(' opt_parameter_list ')' ';'
     *      ;
     *
     * A declaration which ends in a semicolon is a prototype: it names the
     * function and the types it takes, and the body comes later. The type
     * and the name have already been parsed by the caller.
     */
    AstNodePtr FunctionDeclaration(const std::vector<NodePtr>& token_list,
        idx_t& i,
        const TypeRef& type,
        const std::string& name,
        bool is_static);

    /*
     * var_declaration: type declarator_list ';'  ;
     *
     * declarator_list: declarator
     *      |           declarator ',' declarator_list
     *      ;
     *
     * declarator: opt_pointer identifier opt_value
     *      |   opt_pointer identifier '[' opt_number ']' opt_values
     *      ;
     *
     * The type has already been parsed by the caller, but it is only the
     * base of every type in the list: each name adds its own stars and its
     * own array size to it, so `int a, *b, c[40], *d[100];` declares four
     * variables of four different types. Each name is declared as it is
     * read, so that the value it starts with may name the variables before
     * it, and the names come back in the order they are written together
     * with anything which has to run to give a local one its value. The
     * semicolon is left to the caller, the same way it is for every other
     * statement.
     */
    std::vector<DeclaredName> VariableList(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        const TypeRef& base,
        const DeclarationPrefix& prefix);

    /*
     * Whether the name which follows the type declares a function: some
     * stars, a name and a '('. A function and a variable are told apart
     * before the name is read, because the stars belong to the declaration
     * and a variable may be the first of a list.
     */
    bool IsFunctionDeclarator(const std::vector<NodePtr>& token_list,
                              idx_t i) const;

    /*
     * Stop unless the storage of a name is given where the name is
     * declared. A global one has the value stored in its own storage and a
     * local one is given it by an assignment, but the storage of an extern
     * name belongs to another source file, so no value may be put in it
     * from here.
     */
    void CheckInitialisable(StorageClass storage,
                            const std::string& name) const;

    /*
     * Parse the one value a global variable or one element of it is given
     * where it is declared, and leave the token after it for the caller.
     * The value is an expression which the compiler works out while it is
     * compiling the program, so `int x= 10 + 6;` is given the value 16. A
     * pointer to a char is given a string, whose address the value becomes;
     * every other type takes a number which has to fit it.
     */
    InitialValue ParseConstantValue(const TypeRef& type,
                                    const std::vector<NodePtr>& token_list,
                                    idx_t& i);

    /*
     * The room one value of a type takes, which is what sizeof answers
     * with. A type which cannot hold a value at all, such as void or a
     * composite whose members are not known yet, is refused rather than
     * answered with a size of nothing.
     */
    int32 SizeOf(const TypeRef& type);

    /*
     * The room the value of an expression takes. An array named on its own
     * stands for the address of its first element, so the room asked about
     * comes from its declaration rather than from that address.
     */
    int32 SizeOfValue(const AstNodePtr& value);

    /*
     * The number a constant is written with, which is an expression the
     * compiler works out while it is compiling the program, so an array
     * size of `2 + 3` is five and an enum value of `-1` is one below zero.
     * The message names what the number was wanted for, and is used when
     * there is no such number where one was expected.
     */
    int32 ParseConstantNumber(const std::vector<NodePtr>& token_list,
                              idx_t& i,
                              std::string_view what);

    /*
     * The number a constant expression stands for. The expression has been
     * worked out already, so a number the compiler could work out is a
     * literal by now and anything else is not a constant at all, which is
     * what a case value has to be.
     */
    bool ConstantValue(const AstNode& node, int32& value) const;

    /*
     * Build the assignment which gives a local variable the value it is
     * declared with, and leave the token after the value for the caller.
     * The expression is worked out and stored where the variable is, which
     * is exactly what an assignment written by hand does.
     */
    AstNodePtr InitialiserAssignment(const Symbol& symbol,
                                     const std::vector<NodePtr>& token_list,
                                     idx_t& i);

    /*
     * array_declaration: identifier '[' number ']'  ;
     *      |           identifier '[' ']'
     *      |           identifier '[' number ']' '=' '{' value_list '}'
     *      |           identifier '[' ']' '=' '{' value_list '}'
     *      ;
     *
     * The type and the name have already been parsed by the caller. The
     * number is how many elements the array holds, and the whole array
     * takes room for all of them at once: one element of the declared
     * type for each. The number may be left out when the values the array
     * is given are its size instead, and the caller matches the semicolon.
     */
    Symbol ArrayDeclaration(const std::vector<NodePtr>& token_list,
                            idx_t& i,
                            const TypeRef& element_type,
                            const std::string& name,
                            StorageClass storage);

    /*
     * composite_type: struct_type | union_type
     *      ;
     *
     * struct_type: 'struct' opt_tag '{' member_list '}'
     *      |       'struct' tag
     *      ;
     *
     * union_type: 'union' opt_tag '{' member_list '}'
     *      |      'union' tag
     *      ;
     *
     * The `struct` or `union` keyword has been read. What follows is
     * either the tag of a type which is already known, which is how a
     * variable of it, or a pointer to it, is written, or the members of a
     * new type between braces. A type may be written without a tag, which
     * is how a type which is used once and never named again is declared.
     */
    TypeRef CompositeTypeDeclaration(const std::vector<NodePtr>& token_list,
                                     idx_t& i);

    /*
     * enum_type: 'enum' opt_tag
     *      |   'enum' opt_tag '{' enum_value_list '}'
     *      ;
     *
     * enum_value_list: enum_value
     *      |           enum_value ',' enum_value_list
     *      ;
     *
     * enum_value: identifier
     *      |      identifier '=' number
     *      ;
     *
     * The `enum` keyword has been read. Without a brace what follows names
     * a tag which is already known, and the type it yields is an int
     * because that is all a value of an enum is. With a brace, every name
     * between them is added to the symbol table as an ordinary name for
     * the number it stands for, counting up from zero and starting again
     * from every number written out.
     */
    TypeRef EnumType(const std::vector<NodePtr>& token_list, idx_t& i);

    /*
     * typedef_declaration: 'typedef' type identifier ';'  ;
     *
     * The `typedef` keyword has been read. The type it names has already
     * been read by the time the new name is reached, and the name becomes
     * another way of writing that type. The caller matches the semicolon.
     */
    void TypedefDeclaration(const std::vector<NodePtr>& token_list, idx_t& i);

    /*
     * member_list: type identifier ';'
     *      |       type identifier '[' number ']' ';'
     *      |       member_list member_list
     *      ;
     *
     * Every member is given the place it takes inside a value of the
     * type, and the room the whole type takes is worked out once they
     * have all been read.
     */
    void ParseMembers(const std::vector<NodePtr>& token_list,
                      idx_t& i,
                      const std::shared_ptr<CompositeType>& type);

    AstNodePtr CompoundStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    ParsedStatement Statement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * The body of an if, a while or a for loop, which is one statement or a
     * compound statement of several. Braces are only needed when more than
     * one statement stands there; a single one ends in a semicolon, which
     * is matched here.
     */
    AstNodePtr BodyStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * case_body: <empty>
     *      |   statement case_body
     *      ;
     *
     * The statements of a switch arm run one after another, until the next
     * case, the default or the end of the switch. Unlike the body of an if
     * or a loop, more than one statement may stand there without braces,
     * and a '{' simply begins a compound statement like it does anywhere.
     */
    AstNodePtr CaseBody(const std::vector<NodePtr>& token_list, idx_t& i);

    /*
     * expression_list: <empty>
     *      |   expression
     *      |   expression ',' expression_list
     *      ;
     *
     * The expressions run in the order they are written, which is how the
     * first and the last clause of a for loop are written. The clause ends
     * at the token the caller names.
     */
    AstNodePtr ExpressionList(const std::vector<NodePtr>& token_list,
        idx_t& i,
        NodeTag end);

    AstNodePtr PrintStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr DeclareStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr IfStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr WhileStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr ForStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr ReturnStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * break_statement: 'break'  ;
     * continue_statement: 'continue'  ;
     *
     * A break leaves the loop or the switch it stands in, and a continue
     * begins the next pass of the loop it stands in, so each refuses to be
     * written where there is nothing to leave or to go on with. The caller
     * matches the semicolon.
     */
    AstNodePtr BreakStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    AstNodePtr ContinueStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * switch_statement: 'switch' '(' expression ')' '{' case_list '}'  ;
     *
     * case_list: case_item
     *      |   case_item case_list
     *      ;
     *
     * case_item: 'case' number ':' compound_statement
     *      |   'default' ':' compound_statement
     *      ;
     *
     * The arms are tried in the order they are written and the first one
     * whose value matches is taken; the default arm is taken when none
     * does. An arm which does not end in a break falls through into the
     * arm after it, and a switch may hold a break like a loop does.
     */
    AstNodePtr SwitchStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * Parse a function call. The current token is the function's name and
     * the next one is the opening parenthesis.
     */
    AstNodePtr FuncCall(const std::vector<NodePtr>& token_list,
                        idx_t& i);

    /*
     * Parse the parameter list of a function, which is empty or one or
     * more `type identifier` pairs separated by commas. The parameters
     * come back in the order they are written, and nothing is added to the
     * symbol table here: a prototype keeps only their types, while a
     * definition binds them once its body is known to follow.
     */
    std::vector<Symbol> FunctionParameters(
        const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * Compare a declaration of a function with the one which is already
     * known, which is a prototype or a function which has been defined.
     * The return type, the number of parameters and the type of every
     * parameter have to agree.
     */
    void CheckPrototype(const Symbol& previous, const Symbol& symbol);

    AstNodePtr Condition(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /*
     * Turn an expression into a condition: a comparison already says
     * whether it holds, any other value is false when it is zero.
     */
    void MakeCondition(AstNodePtr& expression);

    AstNodePtr BinaryExpression(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        uint8 ptp); /* previous token precedence */

    /*
     * Parse an expression which may start with the prefix operators '*'
     * and '&', and fall back to a postfix expression when it does not.
     */
    AstNodePtr PrefixExpression(const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr ParsePrimary(const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr PostfixExpression(const std::vector<NodePtr>& token_list,
                                idx_t& i);

    /*
     * array_access: postfix_expression '[' expression ']'  ;
     *
     * The base is the array to index, or the pointer to walk along: both
     * start a row of values of one type. The index counts elements and is
     * scaled into an offset before it is added to the base, and what
     * comes back is the element itself, which may stand on the left of an
     * assignment as well as be read.
     */
    AstNodePtr ArrayAccess(AstNodePtr base,
                          const std::vector<NodePtr>& token_list,
                          idx_t& i);

    AstNodePtr MemberAccess(AstNodePtr base,
                           const std::vector<NodePtr>& token_list,
                           idx_t& i);

    /*
     * Parse the type which starts a declaration and step over it. Only the
     * base type is read: the stars of a declarator belong to the name they
     * stand in front of, so they are read by ParseStars where that name is
     * read. The `type_definition_` flag is left saying whether the type was
     * written as a definition of a tag, which is the one shape a
     * declaration may end at without naming anything, as `enum fred { ... };`
     * does.
     */
    TypeRef ParseType(const std::vector<NodePtr>& token_list, idx_t& i);

    /*
     * opt_pointer: <empty>
     *      |      '*' opt_pointer
     *      ;
     *
     * Read the stars in front of one name and add a level of indirection to
     * the base type for each of them. Every declarator has its own, which is
     * what lets `int a, *b;` declare a variable and a pointer in one
     * declaration.
     */
    TypeRef ParseStars(const std::vector<NodePtr>& token_list,
                       idx_t& i,
                       const TypeRef& base);

    /*
     * Return the declaration a name refers to, whatever kind of symbol it
     * names, and stop if there is no such name. A name can only be read
     * where one is expected, so a literal or an operator standing there
     * is refused rather than read as a name.
     */
    Symbol Lookup(const NodePtr& token, std::string_view what);

    /*
     * Whether a '(' begins a cast, which is a type written straight after
     * it: a type keyword, or a name made with typedef. Anything else begins
     * a parenthesised expression.
     */
    bool StartsCast(const std::vector<NodePtr>& token_list, idx_t i) const;

    /*
     * Parse the type a cast names, which stands between a '(' and its ')',
     * and step over it. A cast may name any type which can hold a value, so
     * a struct, a union and a plain void are refused: a pointer to void is a
     * value which stands for any other pointer and is allowed.
     */
    TypeRef ParseCast(const std::vector<NodePtr>& token_list, idx_t& i);

    /*
     * The type a name made with typedef stands for. The token has to be a
     * name which is one; a name which is not is refused with a message
     * saying that the type is unknown.
     */
    TypeRef TypedefType(const NodePtr& token) const;

    /*
     * Whether a token is a name made with typedef, which is what lets a
     * declaration be told from an expression where both may start with a
     * name.
     */
    bool IsTypedefName(const NodePtr& token) const;

    /*
     * Read the storage a declaration is given before its type, which is
     * what `plain` says when neither word stands there. `extern` says the
     * storage of the name belongs to another source file and `static` that
     * it belongs to this one alone; the two may not be written together,
     * and a static name may only be declared at the top level of a program.
     */
    DeclarationPrefix ParseDeclarationPrefix(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        StorageClass plain);

    /*
     * Stop unless a symbol is the kind of symbol the grammar wants there,
     * such as a variable where a value is read.
     */
    void CheckKind(const Symbol& symbol,
                   StructuralType wanted,
                   std::string_view what) const;

    /*
     * Return the declaration a name refers to, checking that it really names
     * a variable or a function as the grammar expects there.
     */
    Symbol LookupTyped(const NodePtr& token,
                       StructuralType wanted,
                       std::string_view what);

    /*
     * Match an identifier and return the name it holds. A declaration is
     * the only place where a name may be a new one, so this is where the
     * name of a variable or of a function is first read; everywhere else a
     * name has to be looked up in the symbol table instead.
     */
    std::string MatchIdentifier(const std::vector<NodePtr>& token_list,
                                idx_t& i,
                                std::string_view what);

    /*
     * Glue a list of statements into one tree, so that a piece of grammar
     * which stands for several statements still produces a single tree.
     */
    AstNodePtr GlueStatements(std::vector<AstNodePtr>& statements);

    AstNodePtr MakeIntLitLeaf(const NodePtr& token, PrimitiveType type);
    AstNodePtr MakeIntLitLeaf(int32 value, PrimitiveType type);
    AstNodePtr MakeIdentLeaf(const Symbol& symbol);

    /*
     * Build a ternary expression out of the condition and the two values it
     * chooses between, making the two agree on a type.
     */
    AstNodePtr MakeTernary(AstNodePtr& condition,
                           AstNodePtr& when_true,
                           AstNodePtr& when_false);
    AstNodePtr MakeOperatorNode(AstNodePtr& left,
                                AstNodePtr& right,
                                const NodePtr& node);

    /*
     * The tag a token is matched by. A keyword carries its own tag inside
     * a T_KeyWord node, every other token is matched by its node tag.
     */
    static NodeTag TokenTag(const NodePtr& token);
    static bool IsComparisonOperator(NodeTag tag);

    /*
     * Whether an operator binds more tightly to the expression on its right
     * than to the one on its left. '=' is the only one: in `a= b= 3` the 3
     * is stored in b first, and the result of that is stored in a, so the
     * operator on the right has to bind first.
     */
    static bool IsRightAssociative(NodeTag tag);

    /*
     * Whether a tree names a location which can be stored into. Only a
     * variable and the location a pointer holds can, so these are the two
     * trees which may stand on the left of an assignment.
     */
    static bool IsLvalue(AstNodeTag tag);

    void Match(const std::vector<NodePtr>& token_list,
               idx_t& i,
               NodeTag tag,
               std::string_view name);

    uint8 GetOpPrecedence(NodeTag tag);

    /*
     * Reserve room in the stack frame of the function being parsed, and
     * return the offset the room starts at. The room is aligned to no
     * more than eight bytes, which is the widest value the language has.
     */
    int32 AllocateLocal(int32 size, int32 alignment);

    /*
     * Reserve the room one value of a type takes. A type which cannot hold
     * a value at all, such as void, has no room to reserve and is an error.
     */
    int32 AllocateLocal(const TypeRef& type);

private:
    SymbolTable symbol_table_;

    /*
     * The return type of the function being parsed, kNone while no function
     * body is open. The caller sets and restores it.
     */
    TypeRef current_function_type_;
    int32 local_stack_size_ = 0;

    /*
     * How many loops and how many switches are open around the statement
     * being parsed. A continue has to be written inside a loop and a break
     * inside a loop or a switch, and both counts are put back to zero at
     * the start of every function.
     */
    int32 loop_level_ = 0;
    int32 switch_level_ = 0;

    /*
     * Whether the type just parsed was written as a definition of a tag.
     * A declaration which names no variable may end where such a type
     * ends, so the callers of ParseType ask this before they ask for a
     * name.
     */
    bool type_definition_ = false;

    /* The struct types the program declared, in the order it declared
     * them, so that the whole program holds them once it is parsed. */
    std::vector<std::shared_ptr<CompositeType>> types_;
};

}   /* namespace nuocc */

#pragma once

#include <optional>
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
    /* Operator precedence: every binary operator has a precedence above 0 */
    static const std::unordered_map<NodeTag, uint8> kOpPrecedence;

public:
    /*
     * Parse the whole token list and return the program. A program is a
     * list of global declarations, each of them a function or a list of
     * global variables.
     */
    Program Parse(const std::vector<NodePtr>& token_list);

private:
    /*
     * global_declaration: function_declaration | var_declaration  ;
     *
     * Both start with a type and a name, so it is the token after the name
     * which tells them apart: a '(' begins the parameter list of a
     * function, a ',' or a ';' continues a list of variables.
     */
    void GlobalDeclaration(const std::vector<NodePtr>& token_list,
        idx_t& i,
        Program& program);

    /*
     * function_declaration: type identifier '(' opt_parameter ')'
     *                       compound_statement  ;
     *
     * The type and the name have already been parsed by the caller.
     */
    AstNodePtr FunctionDeclaration(const std::vector<NodePtr>& token_list,
        idx_t& i,
        PrimitiveType type,
        const std::string& name);

    /*
     * var_declaration: type identifier_list ';'  ;
     *
     * identifier_list: identifier | identifier ',' identifier_list  ;
     *
     * The type and the first name have already been parsed by the caller,
     * so what is left is every ', name' which follows. Each name is
     * declared as a variable of that type, either at the top level of the
     * program or in the current function. The semicolon is left to the
     * caller, the same way it is for every other statement.
     */
    void IdentifierList(const std::vector<NodePtr>& token_list,
                        idx_t& i,
                        PrimitiveType type,
                        const std::string& name,
                        std::vector<AstNodePtr>& declarations,
                        StorageClass storage);

    AstNodePtr CompoundStatement(const std::vector<NodePtr>& token_list,
        idx_t& i);
    ParsedStatement Statement(const std::vector<NodePtr>& token_list,
        idx_t& i);

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
     * Parse a function call. The current token is the function's name and
     * the next one is the opening parenthesis.
     */
    AstNodePtr FuncCall(const std::vector<NodePtr>& token_list,
                        idx_t& i);

    std::optional<Symbol> FunctionParameter(
        const std::vector<NodePtr>& token_list,
        idx_t& i,
        Symbol& function);

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
     * and '&', and fall back to a primary when it does not.
     */
    AstNodePtr PrefixExpression(const std::vector<NodePtr>& token_list,
        idx_t& i);

    AstNodePtr ParsePrimary(const std::vector<NodePtr>& token_list,
        idx_t& i);

    /* Parse the type which starts a declaration and step over it. */
    PrimitiveType ParseType(const std::vector<NodePtr>& token_list,
                            idx_t& i);

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
    AstNodePtr MakeIdentLeaf(const Symbol& symbol);
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
    int32 AllocateLocal(PrimitiveType type);

private:
    SymbolTable symbol_table_;

    /*
     * The return type of the function being parsed, kNone while no function
     * body is open. The caller sets and restores it.
     */
    PrimitiveType current_function_type_ = PrimitiveType::kNone;
    int32 local_stack_size_ = 0;
};

}   /* namespace nuocc */

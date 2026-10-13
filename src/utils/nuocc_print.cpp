#include "utils/nuocc_print.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

std::string NodeTagToString(const NodeTag& tag)
{
    std::string out;    /* output */

    switch (tag)
    {
        case T_UnknownToken:
            out = "UnknownToken";
            break;
        case T_Plus:
            out = "+";
            break;
        case T_Minus:
            out = "-";
            break;
        case T_Star:
            out = "*";
            break;
        case T_Slash:
            out = "/";
            break;
        case T_Assign:
            out = "=";
            break;
        case T_Amper:
            out = "&";
            break;
        case T_LogAnd:
            out = "&&";
            break;
        case T_LogOr:
            out = "||";
            break;
        case T_Or:
            out = "|";
            break;
        case T_Xor:
            out = "^";
            break;
        case T_LShift:
            out = "<<";
            break;
        case T_RShift:
            out = ">>";
            break;
        case T_Inc:
            out = "++";
            break;
        case T_Dec:
            out = "--";
            break;
        case T_Invert:
            out = "~";
            break;
        case T_LogNot:
            out = "!";
            break;
        case T_EQ:
            out = "==";
            break;
        case T_NE:
            out = "!=";
            break;
        case T_LT:
            out = "<";
            break;
        case T_GT:
            out = ">";
            break;
        case T_LE:
            out = "<=";
            break;
        case T_GE:
            out = ">=";
            break;
        case T_IntLit:
            out = "IntLit";
            break;
        case T_StrLit:
            out = "StrLit";
            break;
        case T_KeyWord:
            out = "KeyWord";
            break;
        case T_Int:
            out = "int";
            break;
        case T_Char:
            out = "char";
            break;
        case T_Long:
            out = "long";
            break;
        case T_Struct:
            out = "struct";
            break;
        case T_Union:
            out = "union";
            break;
        case T_Print:
            out = "print";
            break;
        case T_If:
            out = "if";
            break;
        case T_Else:
            out = "else";
            break;
        case T_While:
            out = "while";
            break;
        case T_For:
            out = "for";
            break;
        case T_Void:
            out = "void";
            break;
        case T_Return:
            out = "return";
            break;
        case T_Identifier:
            out = "Identifier";
            break;
        case T_LBrace:
            out = "{";
            break;
        case T_RBrace:
            out = "}";
            break;
        case T_LParen:
            out = "(";
            break;
        case T_RParen:
            out = ")";
            break;
        case T_LBracket:
            out = "[";
            break;
        case T_RBracket:
            out = "]";
            break;
        case T_Dot:
            out = ".";
            break;
        case T_Arrow:
            out = "->";
            break;
        case T_Semicolon:
            out = ";";
            break;
        case T_Comma:
            out = ",";
            break;
        case T_EOF:
            out = "EOF";
            break;
        default:
            break;
    }

    return out;
}

void PrintTokenList(const Scanner& scanner)
{
    const std::vector<NodePtr>& token_list
        = scanner.GetTokenList();

    for (auto& token : token_list)
    {
        std::cout << NodeTagToString(token->GetNodeTag());

        if (token->GetNodeTag() == T_IntLit)
        {
            auto lit = static_cast<Literal<int, T_IntLit>*>(token.get());
            std::cout << " " << lit->GetValue();
        }
        else if (token->GetNodeTag() == T_StrLit)
        {
            auto lit =
                static_cast<Literal<std::string, T_StrLit>*>(token.get());
            std::cout << " " << lit->GetValue();
        }
        else if (token->GetNodeTag() == T_KeyWord)
        {
            auto key_word = static_cast<KeyWord*>(token.get());
            std::cout << " " << NodeTagToString(key_word->GetWord());
        }
        else if (token->GetNodeTag() == T_Identifier)
        {
            auto identifier = static_cast<Identifier*>(token.get());
            std::cout << " " << identifier->GetName();
        }

        std::cout << std::endl;
    }
}

namespace
{

/* The name of a type, for the line the tree dump prints: the primitive
 * type and one star for every level of indirection it has. A struct is
 * named by its tag when it has one, which is the only way the dump can say
 * which struct a value is of. */
std::string TypeName(PrimitiveType type,
    const std::shared_ptr<CompositeType>& composite = nullptr)
{
    std::string name;

    switch (BaseType(type))
    {
        case PrimitiveType::kVoid:
            name = "void";
            break;
        case PrimitiveType::kChar:
            name = "char";
            break;
        case PrimitiveType::kInt:
            name = "int";
            break;
        case PrimitiveType::kLong:
            name = "long";
            break;
        case PrimitiveType::kComposite:
            if (!composite || composite->kind == CompositeKind::kStruct)
                name = "struct";
            else
                name = "union";
            if (composite && !composite->name.empty())
                name += " " + composite->name;
            break;
        default:
            return "none";
    }

    for (uint8 level = 0; level < Indirection(type); level++)
        name += " *";

    return name;
}

/* What one node stands for, which is everything about it but its
 * children. A function call is the only node whose arguments are not
 * children, so its line names them as well. */
std::string DescribeNode(const AstNode& node)
{
    PrimitiveType type = node.GetType();

    switch (node.GetAstNodeTag())
    {
        case A_AstFunction:
        {
            const AstFunction *function =
                static_cast<const AstFunction*>(&node);
            std::string out = "function " + function->GetSymbol().name;
            out += " returns " + TypeName(function->GetSymbol().type,
                                          function->GetSymbol().composite);
            out += ", " + std::to_string(function->GetParameters().size());
            out += " parameters, " + std::to_string(function->GetLocalSize());
            out += " bytes of locals";
            return out;
        }
        case A_AstDeclare:
        {
            const AstDeclare *declare = static_cast<const AstDeclare*>(&node);
            return "declare " + TypeName(declare->GetSymbol().type,
                                         declare->GetSymbol().composite) +
                   " " + declare->GetSymbol().name;
        }
        case A_AstPrint:
            return "print";
        case A_AstGlue:
            return "glue";
        case A_AstIf:
            return static_cast<const AstIf*>(&node)->HasElse() ? "if else"
                                                              : "if";
        case A_AstWhile:
            return "while";
        case A_AstReturn:
            return "return";
        case A_AstOperator:
        {
            const AstOperator *op = static_cast<const AstOperator*>(&node);
            return "operator " + NodeTagToString(op->GetOpType()) + " : " +
                   TypeName(type);
        }
        case A_AstIntLit:
        {
            const AstIntLit *lit = static_cast<const AstIntLit*>(&node);
            return "int " + std::to_string(lit->GetValue()) + " : " +
                   TypeName(type);
        }
        case A_AstStrLit:
        {
            const AstStrLit *lit = static_cast<const AstStrLit*>(&node);
            return "string \"" + lit->GetText() + "\"";
        }
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(&node);
            return "read " + ident->GetSymbol().name + " : " +
                   TypeName(type, ident->GetSymbol().composite);
        }
        case A_AstWiden:
            return "widen to " + TypeName(type);
        case A_AstScale:
        {
            const AstScale *scale = static_cast<const AstScale*>(&node);
            return "scale by " + std::to_string(scale->GetSize());
        }
        case A_AstAddress:
        {
            const AstAddress *address = static_cast<const AstAddress*>(&node);
            return "address of " + address->GetSymbol().name;
        }
        case A_AstMember:
        {
            const AstMember *member = static_cast<const AstMember*>(&node);
            return "member " + member->GetMember().name + " : " +
                   TypeName(type, member->GetMember().composite);
        }
        case A_AstAddressExpr:
            return "address expression";
        case A_AstDeref:
            return "read through pointer : " + TypeName(type);
        case A_AstFuncCall:
        {
            const AstFuncCall *call = static_cast<const AstFuncCall*>(&node);
            return "call " + call->GetSymbol().name + " returns " +
                   TypeName(type, call->GetSymbol().composite) + ", " +
                   std::to_string(call->GetArguments().size()) + " arguments";
        }
        case A_AstNegate:
            return "negate";
        case A_AstInvert:
            return "invert";
        case A_AstLogNot:
            return "logical not";
        case A_AstToBool:
            return "to condition";
        case A_AstIncDec:
        {
            const AstIncDec *inc = static_cast<const AstIncDec*>(&node);
            return std::string(inc->IsPost() ? "after" : "before") +
                   " increment of " + inc->GetSymbol().name + " by " +
                   std::to_string(inc->GetDelta());
        }
        default:
            return "node";
    }
}

void PrintNode(const AstNodePtr& node, int32 depth)
{
    if (!node)
        return;

    std::cout << std::string(depth * 2, ' ') << DescribeNode(*node);
    std::cout << std::endl;

    /* The arguments of a call are not children, so they are printed
     * here as if they were, one after another. */
    if (node->GetAstNodeTag() == A_AstFuncCall)
    {
        const AstFuncCall *call = static_cast<const AstFuncCall*>(node.get());

        for (const AstNodePtr& argument : call->GetArguments())
            PrintNode(argument, depth + 1);
    }

    PrintNode(node->GetLeft(), depth + 1);
    PrintNode(node->GetMid(), depth + 1);
    PrintNode(node->GetRight(), depth + 1);
}

}   /* namespace */

void PrintSyntaxTree(const Program& program)
{
    for (const std::shared_ptr<CompositeType>& type : program.types)
    {
        std::cout << (type->kind == CompositeKind::kUnion ? "union "
                                                            : "struct ");
        std::cout << (type->name.empty() ? "(no tag)" : type->name);
        std::cout << ": " << type->size << " bytes, aligned to ";
        std::cout << type->alignment << std::endl;

        for (const Symbol& member : type->members)
        {
            std::cout << "  at " << member.member_offset << ": ";
            std::cout << TypeName(member.type, member.composite);
            std::cout << " " << member.name;

            if (member.stype == StructuralType::kArray)
                std::cout << "[" << member.element_count << "]";

            std::cout << std::endl;
        }
    }

    for (const AstNodePtr& global : program.globals)
        PrintNode(global, 0);

    for (const AstNodePtr& function : program.functions)
        PrintNode(function, 0);
}

}   /* namespace nuocc */

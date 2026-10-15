#pragma once

namespace nuocc
{

typedef enum NodeTag
{
    /* Scanner Tags */
    T_UnknownToken = 0,

    /* Operator */
    T_Plus = 1,
    T_Minus,
    T_Star,
    T_Slash,
    T_Assign,
    T_Amper,
    T_LogAnd,
    T_LogOr,
    T_Or,
    T_Xor,
    T_LShift,
    T_RShift,
    T_Inc,
    T_Dec,
    T_Invert,
    T_LogNot,
    T_PlusAssign,
    T_MinusAssign,
    T_StarAssign,
    T_SlashAssign,
    T_Question,

    /* Comparison Operator */
    T_EQ,
    T_NE,
    T_LT,
    T_GT,
    T_LE,
    T_GE,

    /* Literal Type */
    T_IntLit,
    T_StrLit,
    // T_BoolLit,
    // T_CharLit,
    // T_FloatLit,
    // T_DoubleLit,

    T_KeyWord,
    T_Int,
    T_Char,
    T_Long,
    T_Struct,
    T_Union,
    T_Print,
    T_If,
    T_Else,
    T_While,
    T_For,
    T_Void,
    T_Return,
    T_Enum,
    T_Typedef,
    T_Extern,
    T_Break,
    T_Continue,
    T_Switch,
    T_Case,
    T_Default,
    T_Sizeof,
    T_Static,

    T_Identifier,

    T_LBrace,
    T_RBrace,
    T_LParen,
    T_RParen,
    T_LBracket,
    T_RBracket,
    T_Dot,
    T_Arrow,
    T_Semicolon,
    T_Comma,
    T_Colon,
    T_EOF,

    /* Parser/AST Tags */
} NodeTag;

typedef enum AstNodeTag
{
    A_AstNode = 0,
    A_AstOperator = 1,
    A_AstIntLit,
    A_AstIdentifier,
    A_AstPrint,
    A_AstGlue,
    A_AstIf,
    A_AstWhile,
    A_AstDeclare,
    A_AstFunction,
    A_AstWiden,
    A_AstScale,
    A_AstReturn,
    A_AstFuncCall,
    A_AstAddress,
    A_AstDeref,
    A_AstNegate,
    A_AstInvert,
    A_AstLogNot,
    A_AstToBool,
    A_AstIncDec,
    A_AstStrLit,
    A_AstMember,
    A_AstAddressExpr,
    A_AstFor,
    A_AstBreak,
    A_AstContinue,
    A_AstSwitch,
    A_AstCase,
    A_AstCast,
    A_AstTernary
} AstNodeTag;

/*
 * Whether an assignment combines the value on its right with the one the
 * target holds rather than storing it as it stands. `=` stores, while `+=`
 * and its three companions combine.
 */
inline bool IsCompoundAssign(NodeTag tag)
{
    return tag == T_PlusAssign || tag == T_MinusAssign ||
           tag == T_StarAssign || tag == T_SlashAssign;
}

/*
 * The operator a compound assignment combines its two sides with: `+=`
 * adds, `-=` subtracts, `*=` multiplies and `/=` divides. The caller asks
 * this only after IsCompoundAssign has said which tags are the compound
 * ones, so any other tag is answered with itself and used by nobody.
 */
inline NodeTag CompoundAssignOperator(NodeTag tag)
{
    switch (tag)
    {
        case T_PlusAssign:
            return T_Plus;
        case T_MinusAssign:
            return T_Minus;
        case T_StarAssign:
            return T_Star;
        case T_SlashAssign:
            return T_Slash;
        default:
            return tag;
    }
}

}   /* namespace nuocc */

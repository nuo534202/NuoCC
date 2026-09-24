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

    /* Comparison Operator */
    T_EQ,
    T_NE,
    T_LT,
    T_GT,
    T_LE,
    T_GE,

    /* Literal Type */
    T_IntLit,
    // T_BoolLit,
    // T_CharLit,
    // T_FloatLit,
    // T_DoubleLit,

    T_KeyWord,
    T_Int,
    T_Print,

    T_Identifier,

    T_Semicolon,
    T_EOF,

    /* Parser/AST Tags */
} NodeTag;

typedef enum AstNodeTag
{
    A_AstNode = 0,
    A_AstOperator = 1,
    A_AstIntLit,
    A_AstIdentifier
} AstNodeTag;

}   /* namespace nuocc */
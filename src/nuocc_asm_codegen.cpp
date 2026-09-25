#include "nuocc_asm_codegen.hpp"

#include <stdlib.h>

#include <iostream>
#include <unordered_map>

#include "utils/nuocc_runtime.hpp"
#include "utils/nuocc_type_check.hpp"

namespace nuocc
{

namespace
{

/* The x86-64 instructions which implement a comparison operator. */
struct ComparisonInstr
{
    /* Sets a register to 0 or 1, used inside an expression. */
    std::string_view set;
    /* Jumps away when the comparison does not hold, used as a condition. */
    std::string_view jump;
};

const std::unordered_map<NodeTag, ComparisonInstr> kComparisonInstr = {
    {T_EQ, {"sete",  "jne"}},
    {T_NE, {"setne", "je"}},
    {T_LT, {"setl",  "jge"}},
    {T_GT, {"setg",  "jle"}},
    {T_LE, {"setle", "jg"}},
    {T_GE, {"setge", "jl"}}
};

}   /* namespace */

AsmCodegen::AsmCodegen(const std::string& output_file)
    : next_label_(1),
      function_end_label_(0),
      function_return_type_(PrimitiveType::kNone),
      ofs_(output_file, std::ios::out | std::ios::trunc)
{
    if (!ofs_.is_open())
    {
        std::cerr << "Error: cannot open output file!" << std::endl;
        std::exit(1);
    }

    for (idx_t i = 0; i < kRegSize; i++)
    {
        is_free_[i] = true;
        reg_list_[i] = "%r" + std::to_string(i + 8);
        breg_list_[i] = reg_list_[i] + "b";
        dreg_list_[i] = reg_list_[i] + "d";
    }
}

AsmCodegen::~AsmCodegen()
{
    ofs_.close();
}

void AsmCodegen::GenProgram(const std::vector<AstNodePtr>& functions)
{
    FreeAllRegister();

    GenPreamble();

    for (const AstNodePtr& function : functions)
        GenFunction(function);
}

/*
 * The preamble is what every program needs regardless of the functions it
 * declares. The only thing the language can currently call is printint(),
 * which is a normal C function linked in with the generated code.
 */
void AsmCodegen::GenPreamble()
{
    ofs_ << "\t.text" << std::endl;
}

void AsmCodegen::GenFunction(const AstNodePtr& root)
{
    const AstFunction *function =
        static_cast<const AstFunction*>(root.get());

    /*
     * Every return statement jumps here, and the node which tells us which
     * register the result of a return has to end up in.
     */
    function_end_label_ = NewLabel();
    function_return_type_ = function->GetSymbol().type;

    GenFunctionPreamble(function->GetSymbol());

    GenStatement(root->GetLeft());

    FreeAllRegister();

    GenFunctionPostamble();

    function_end_label_ = 0;
    function_return_type_ = PrimitiveType::kNone;
}

void AsmCodegen::GenFunctionPreamble(const Symbol& symbol)
{
    ofs_ << "\t.text" << std::endl;
    ofs_ << "\t.globl\t" << symbol.name << std::endl;
    ofs_ << "\t.type\t" << symbol.name << ", @function" << std::endl;
    ofs_ << symbol.name << ":" << std::endl;
    ofs_ << "\tpushq\t%rbp" << std::endl;
    ofs_ << "\tmovq\t%rsp, %rbp" << std::endl;
}

void AsmCodegen::GenFunctionPostamble()
{
    EmitLabel(function_end_label_);

    ofs_ << "\tpopq %rbp" << std::endl;
    ofs_ << "\tret" << std::endl;
}

/*
 * Generate the code for one statement. A statement yields no value, so
 * nothing is returned. A null node is an empty compound statement.
 */
void AsmCodegen::GenStatement(const AstNodePtr& root)
{
    if (!root)
        return;

    switch (root->GetAstNodeTag())
    {
        case A_AstGlue:
            /* The statements were glued left to right, so run them in order. */
            GenStatement(root->GetLeft());
            FreeAllRegister();
            GenStatement(root->GetRight());
            FreeAllRegister();
            return;

        case A_AstPrint:
        {
            reg_idx reg = GenExpr(root->GetLeft());
            PrintInt(reg);
            return;
        }

        case A_AstDeclare:
        {
            const AstDeclare *decl =
                static_cast<const AstDeclare*>(root.get());
            GenGlobSymbol(decl->GetSymbol());
            return;
        }

        case A_AstIf:
            GenIf(root);
            return;

        case A_AstWhile:
            GenWhile(root);
            return;

        case A_AstReturn:
        {
            reg_idx reg = GenExpr(root->GetLeft());
            GenReturn(reg);
            FreeRegister(reg);
            return;
        }

        case A_AstFuncCall:
        {
            /* The result of a call is discarded when it stands alone. */
            reg_idx reg = GenCall(root);
            FreeRegister(reg);
            return;
        }

        case A_AstOperator:
        {
            const AstOperator *ast_op =
                static_cast<const AstOperator*>(root.get());

            if (ast_op->GetOpType() != T_Assign)
                break;

            /* The left child is the value to store, the right one the target. */
            const AstNodePtr& target = root->GetRight();

            if (!target || target->GetAstNodeTag() != A_AstIdentifier)
            {
                std::cerr << "Error: assignment target is not an identifier!";
                std::cerr << std::endl;
                std::exit(1);
            }

            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(target.get());

            if (!ident->GetLvIdent())
            {
                std::cerr << "Error: assignment target ";
                std::cerr << ident->GetSymbol().name;
                std::cerr << " is not an lvalue!" << std::endl;
                std::exit(1);
            }

            reg_idx value_reg = GenExpr(root->GetLeft());

            StoreGlobSymbol(ident->GetSymbol(), value_reg);
            FreeRegister(value_reg);
            return;
        }

        default:
            break;
    }

    std::cerr << "Error: node " << root->GetAstNodeTag();
    std::cerr << " is not a statement!" << std::endl;
    std::exit(1);
}

/*
 * Generate the code for an if statement:
 *
 *      <condition>, jumping to Lfalse when it does not hold
 *      <true branch>
 *      jmp Lend
 * Lfalse:
 *      <else branch>
 * Lend:
 *
 * Without an else clause the false label is also the end label, so the
 * code simply falls through to it.
 */
void AsmCodegen::GenIf(const AstNodePtr& root)
{
    const AstIf *ast_if = static_cast<const AstIf*>(root.get());

    label_idx false_label = NewLabel();
    label_idx end_label = false_label;

    if (ast_if->HasElse())
        end_label = NewLabel();

    GenCondition(root->GetLeft(), false_label);
    FreeAllRegister();

    GenStatement(root->GetMid());
    FreeAllRegister();

    if (ast_if->HasElse())
        EmitJump(end_label);

    EmitLabel(false_label);

    if (ast_if->HasElse())
    {
        GenStatement(root->GetRight());
        FreeAllRegister();
        EmitLabel(end_label);
    }
}

/*
 * Generate the code for a while loop:
 *
 * Lstart:
 *      <condition>, jumping to Lend when it does not hold
 *      <body>
 *      jmp Lstart
 * Lend:
 */
void AsmCodegen::GenWhile(const AstNodePtr& root)
{
    label_idx start_label = NewLabel();
    label_idx end_label = NewLabel();

    EmitLabel(start_label);

    GenCondition(root->GetLeft(), end_label);
    FreeAllRegister();

    GenStatement(root->GetRight());
    FreeAllRegister();

    EmitJump(start_label);
    EmitLabel(end_label);
}

/*
 * Generate the code for an if condition. The condition has to be a
 * comparison, so it becomes a compare followed by a jump to the false
 * label when the comparison does not hold.
 */
void AsmCodegen::GenCondition(const AstNodePtr& condition,
    label_idx false_label)
{
    const AstOperator *ast_op =
        static_cast<const AstOperator*>(condition.get());

    reg_idx left_reg = GenExpr(condition->GetLeft());
    reg_idx right_reg = GenExpr(condition->GetRight());

    CompareAndJump(ast_op->GetOpType(), left_reg, right_reg, false_label);
}

/*
 * Generate the code for an expression and return the register which holds
 * its value. The caller owns that register and has to free it.
 */
reg_idx AsmCodegen::GenExpr(const AstNodePtr& root)
{
    switch (root->GetAstNodeTag())
    {
        case A_AstIntLit:
        {
            const AstIntLit *int_lit =
                static_cast<const AstIntLit*>(root.get());
            return Load(int_lit->GetValue());
        }
        case A_AstIdentifier:
        {
            const AstIdentifier *ident =
                static_cast<const AstIdentifier*>(root.get());

            /* An lvalue only names a location, it holds no value. */
            if (ident->GetLvIdent())
            {
                std::cerr << "Error: lvalue identifier ";
                std::cerr << ident->GetSymbol().name;
                std::cerr << " is used as a value!" << std::endl;
                std::exit(1);
            }

            return LoadGlobSymbol(ident->GetSymbol());
        }
        case A_AstWiden:
        {
            reg_idx reg = GenExpr(root->GetLeft());

            return Widen(reg, root->GetLeft()->GetType(), root->GetType());
        }
        case A_AstFuncCall:
            return GenCall(root);
        case A_AstOperator:
        {
            const AstOperator *ast_op =
                static_cast<const AstOperator*>(root.get());
            NodeTag op_type = ast_op->GetOpType();

            if (op_type == T_Assign)
            {
                std::cerr << "Error: an assignment is not an expression!";
                std::cerr << std::endl;
                std::exit(1);
            }

            reg_idx left_reg = GenExpr(root->GetLeft());
            reg_idx right_reg = GenExpr(root->GetRight());

            return GenOperator(op_type, left_reg, right_reg);
        }
        default:
            break;
    }

    std::cerr << "Error: node " << root->GetAstNodeTag();
    std::cerr << " is not an expression!" << std::endl;
    std::exit(1);
}

reg_idx AsmCodegen::GenOperator(NodeTag op_type,
    reg_idx left_reg,
    reg_idx right_reg)
{
    switch (op_type)
    {
        case T_Plus:
            return Add(left_reg, right_reg);
        case T_Minus:
            return Sub(left_reg, right_reg);
        case T_Star:
            return Mul(left_reg, right_reg);
        case T_Slash:
            return Div(left_reg, right_reg);
        case T_EQ:
        case T_NE:
        case T_LT:
        case T_GT:
        case T_LE:
        case T_GE:
            return CompareAndSet(op_type, left_reg, right_reg);
        default:
            break;
    }

    std::cerr << "Error: token " << op_type;
    std::cerr << " is not a binary operator!" << std::endl;
    std::exit(1);
}

/*
 * Call a function, passing the value held in one register as its single
 * argument. The result comes back in %rax, so it is copied straight into a
 * fresh register.
 *
 * The call is allowed to clobber every register, so the ones which still
 * hold a value have to be saved across it. An odd number of saved
 * registers would leave the stack misaligned for the call, so it is padded
 * back to a multiple of sixteen bytes.
 */
reg_idx AsmCodegen::GenCall(const AstNodePtr& root)
{
    const AstFuncCall *call = static_cast<const AstFuncCall*>(root.get());

    reg_idx arg_reg = GenExpr(root->GetLeft());
    int32 saved = 0;

    for (reg_idx reg = 0; reg < kRegSize; reg++)
    {
        if (!is_free_[reg] && reg != arg_reg)
        {
            ofs_ << "\tpushq\t" << reg_list_[reg] << std::endl;
            saved++;
        }
    }

    if (saved % 2 != 0)
        ofs_ << "\tsubq\t$8, %rsp" << std::endl;

    ofs_ << "\tmovq\t" << reg_list_[arg_reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\t" << call->GetSymbol().name << std::endl;

    if (saved % 2 != 0)
        ofs_ << "\taddq\t$8, %rsp" << std::endl;

    for (reg_idx reg = kRegSize; reg > 0; reg--)
    {
        if (!is_free_[reg - 1] && reg - 1 != arg_reg)
            ofs_ << "\tpopq\t" << reg_list_[reg - 1] << std::endl;
    }

    reg_idx out_reg = AllocRegister();

    ofs_ << "\tmovq\t%rax, " << reg_list_[out_reg] << std::endl;

    FreeRegister(arg_reg);

    return out_reg;
}

/*
 * Return from the function being generated. The result goes in %rax, where
 * the caller expects to find it, and control jumps to the end label.
 */
void AsmCodegen::GenReturn(reg_idx reg)
{
    switch (function_return_type_)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovzbl\t" << breg_list_[reg] << ", %eax" << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << dreg_list_[reg] << ", %eax" << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rax" << std::endl;
            break;
        default:
            std::cerr << "Error: bad return type in a return statement!";
            std::cerr << std::endl;
            std::exit(1);
    }

    EmitJump(function_end_label_);
}

void AsmCodegen::GenGlobSymbol(const Symbol& symbol)
{
    /* The storage a variable needs is decided by its type. */
    int32 size = PrimitiveSize(symbol.type);

    ofs_ << "\t.comm\t" << symbol.name << "," << size << "," << size;
    ofs_ << std::endl;
}

reg_idx AsmCodegen::LoadGlobSymbol(const Symbol& symbol)
{
    reg_idx idx = AllocRegister();

    /*
     * Every one of these leaves the whole register holding the value: the
     * widening loads clear the rest of it, and writing to a 32-bit register
     * clears its upper half.
     */
    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovzbq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << symbol.name << "(%rip), ";
            ofs_ << dreg_list_[idx] << std::endl;
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << symbol.name << "(%rip), ";
            ofs_ << reg_list_[idx] << std::endl;
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    return idx;
}

reg_idx AsmCodegen::StoreGlobSymbol(const Symbol& symbol, reg_idx reg)
{
    switch (symbol.type)
    {
        case PrimitiveType::kChar:
            ofs_ << "\tmovb\t" << breg_list_[reg] << ", ";
            break;
        case PrimitiveType::kInt:
            ofs_ << "\tmovl\t" << dreg_list_[reg] << ", ";
            break;
        case PrimitiveType::kLong:
            ofs_ << "\tmovq\t" << reg_list_[reg] << ", ";
            break;
        default:
            std::cerr << "Error: bad type for variable ";
            std::cerr << symbol.name << "!" << std::endl;
            std::exit(1);
    }

    ofs_ << symbol.name << "(%rip)" << std::endl;

    return reg;
}

reg_idx AsmCodegen::AllocRegister()
{
    for (reg_idx i = 0; i < kRegSize; i++)
        if (is_free_[i])
        {
            is_free_[i] = false;
            return i;
        }

    std::cerr << "Error: out of registers!" << std::endl;
    std::exit(1);
}

void AsmCodegen::FreeRegister(reg_idx reg)
{
    if (is_free_[reg])
    {
        std::cerr << "Error: trying to free unused register ";
        std::cerr << reg << "!" << std::endl;
        std::exit(1);
    }

    is_free_[reg] = true;
}

void AsmCodegen::FreeAllRegister()
{
    for (idx_t i = 0; i < kRegSize; i++)
        is_free_[i] = true;
}

label_idx AsmCodegen::NewLabel()
{
    return next_label_++;
}

void AsmCodegen::EmitLabel(label_idx label)
{
    ofs_ << "L" << label << ":" << std::endl;
}

void AsmCodegen::EmitJump(label_idx label)
{
    ofs_ << "\tjmp\tL" << label << std::endl;
}

reg_idx AsmCodegen::Load(int32 value)
{
    reg_idx reg = AllocRegister();

    ofs_ << "\tmovq\t$" << value << ", " << reg_list_[reg] << std::endl;

    return reg;
}

reg_idx AsmCodegen::Add(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\taddq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx AsmCodegen::Sub(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tsubq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx AsmCodegen::Mul(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\timulq\t" << reg_list_[reg1] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

reg_idx AsmCodegen::Div(reg_idx reg1, reg_idx reg2)
{
    ofs_ << "\tmovq\t" << reg_list_[reg1] << ", %rax" << std::endl;
    ofs_ << "\tcqo" << std::endl;
    ofs_ << "\tidivq\t" << reg_list_[reg2] << std::endl;
    ofs_ << "\tmovq\t%rax, " << reg_list_[reg1] << std::endl;

    FreeRegister(reg2);

    return reg1;
}

reg_idx AsmCodegen::Widen(reg_idx reg,
    PrimitiveType /*old_type*/,
    PrimitiveType /*new_type*/)
{
    /*
     * Nothing to do on x86-64. A char is loaded with movzbq, which already
     * zeroes the whole register, and a value computed from chars is just
     * as clean; storing it back truncates to a byte again.
     */
    return reg;
}

reg_idx AsmCodegen::CompareAndSet(NodeTag op_type, reg_idx reg1, reg_idx reg2)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /*
     * cmpq computes reg1 - reg2, so the setX instruction reports the
     * requested relation between the two registers. It only writes the
     * low byte of reg2, so movzbq extends it to a clean 0 or 1.
     */
    ofs_ << "\tcmpq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;
    ofs_ << "\t" << it->second.set << "\t" << breg_list_[reg2] << std::endl;
    ofs_ << "\tmovzbq\t" << breg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg2] << std::endl;

    FreeRegister(reg1);

    return reg2;
}

void AsmCodegen::CompareAndJump(NodeTag op_type,
    reg_idx reg1,
    reg_idx reg2,
    label_idx label)
{
    auto it = kComparisonInstr.find(op_type);

    if (it == kComparisonInstr.end())
    {
        std::cerr << "Error: token " << op_type;
        std::cerr << " is not a comparison!" << std::endl;
        std::exit(1);
    }

    /*
     * The jump is taken when the comparison does not hold, so the code
     * which follows only runs when the condition is true.
     */
    ofs_ << "\tcmpq\t" << reg_list_[reg2] << ", ";
    ofs_ << reg_list_[reg1] << std::endl;
    ofs_ << "\t" << it->second.jump << "\tL" << label << std::endl;

    FreeAllRegister();
}

void AsmCodegen::PrintInt(reg_idx reg)
{
    ofs_ << "\tmovq\t" << reg_list_[reg] << ", %rdi" << std::endl;
    ofs_ << "\tcall\t" << kPrintIntName << std::endl;
    ofs_ << std::endl;

    FreeRegister(reg);
}

}   /* namespace nuocc */

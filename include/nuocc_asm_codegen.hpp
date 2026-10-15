#pragma once

#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nodes/nuocc_ast_nodes.hpp"
#include "utils/nuocc_types.hpp"

namespace nuocc
{

constexpr int kRegSize = 4;

/*
 * How many values each register may have set aside at once when an
 * expression needs more registers than there are, and how many places the
 * frame of a function keeps for them. An expression never needs more values
 * set aside than it has values standing at once, and one which needs more
 * than this many is refused rather than worked out wrongly.
 */
constexpr int kSpillDepthPerRegister = 4;
constexpr int kSpillSlots = kRegSize * kSpillDepthPerRegister;
constexpr int32 kSpillAreaSize = kSpillSlots * 8;

/*
 * The generic code generator. It walks the syntax tree of the program and
 * asks the target to emit the instructions, so everything which is not
 * specific to a machine lives here and only the instruction selection is
 * left to the subclasses.
 */
class AsmCodegen
{
public:
    explicit AsmCodegen(const std::string& output_file);
    virtual ~AsmCodegen();

public:
    /*
     * Generate the code of the whole program: the storage for the global
     * variables first, then the code of every function.
     */
    void GenProgram(const Program& program);

protected:
    /* What every target has to provide. */

    virtual void EmitPreamble() = 0;
    virtual void EmitFunctionPreamble(
        const Symbol& symbol,
        int32 local_size,
        const std::vector<Symbol>& parameters) = 0;
    virtual void EmitFunctionPostamble() = 0;
    virtual void EmitLabel(label_idx label) = 0;
    virtual void EmitJump(label_idx label) = 0;

    /*
     * The name of a global symbol as the assembler of this target wants it
     * written. Mach-O decorates every global symbol with a leading
     * underscore, ELF leaves the name alone.
     */
    virtual std::string GlobName(const std::string& name) const;

    virtual reg_idx LoadInt(int32 value) = 0;
    virtual reg_idx LoadSymbol(const Symbol& symbol) = 0;
    virtual reg_idx StoreSymbol(const Symbol& symbol, reg_idx reg) = 0;

    virtual reg_idx Add(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Sub(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Mul(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Div(reg_idx reg1, reg_idx reg2) = 0;

    /*
     * What is left of the first value once the second has been taken from
     * it as many whole times as it holds. A division by zero is left to
     * the machine, which is where the program is stopped for it.
     */
    virtual reg_idx Mod(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx And(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Or(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Xor(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx ShiftLeft(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx ShiftRight(reg_idx reg1, reg_idx reg2) = 0;
    virtual reg_idx Widen(reg_idx reg,
                          PrimitiveType old_type,
                          PrimitiveType new_type) = 0;

    /*
     * Set the value of a register aside in one of the places the frame of
     * the function keeps for it, and bring it back again. The places are
     * room of its own and nothing else, so that setting a value aside does
     * not move the stack pointer and a call does not disturb what is set
     * aside. The slot is which of them to use.
     */
    virtual void SpillRegister(reg_idx reg, int32 slot) = 0;
    virtual void ReloadRegister(reg_idx reg, int32 slot) = 0;

    /*
     * Give a value the width of the type it was cast to. A value is kept in
     * a register as the whole of it, so a cast to a narrower type has to
     * leave the bits which that type holds while a cast to a wider one has
     * nothing to do. The old type is what the value had before.
     */
    virtual reg_idx Cast(reg_idx reg,
                         PrimitiveType old_type,
                         PrimitiveType new_type) = 0;
    virtual reg_idx Scale(reg_idx reg, int32 scale) = 0;

    /*
     * Copy the value of one register into another, which is how a ternary
     * expression leaves whichever of its two values it took in the one
     * register it answers with. Both registers stay in use.
     */
    virtual void Move(reg_idx dest, reg_idx src) = 0;

    /* The unary operations, each of them on the value in one register. */
    virtual reg_idx Negate(reg_idx reg) = 0;
    virtual reg_idx Invert(reg_idx reg) = 0;
    virtual reg_idx LogNot(reg_idx reg) = 0;
    virtual reg_idx ToBool(reg_idx reg) = 0;

    /*
     * Add delta to the value a variable holds, and leave either the value
     * it held before or the one it holds afterwards in a register, which is
     * what an increment or a decrement written after or before the variable
     * yields.
     */
    virtual reg_idx IncDec(const Symbol& symbol, int32 delta, bool post) = 0;

    virtual reg_idx CompareAndSet(NodeTag op_type,
                                  reg_idx reg1,
                                  reg_idx reg2) = 0;
    virtual void CompareAndJump(NodeTag op_type,
                                reg_idx reg1,
                                reg_idx reg2,
                                label_idx label) = 0;

    /*
     * Jump to the label when the two registers hold the same value, leaving
     * both of them as they were: a switch compares its value against the
     * value of every case in turn and has to keep it while it does.
     */
    virtual void JumpIfEqual(reg_idx reg1,
                             reg_idx reg2,
                             label_idx label) = 0;

    /*
     * Jump when the value in a register is zero, which is the question a
     * condition which is not a comparison asks.
     */
    virtual void JumpIfZero(reg_idx reg, label_idx label) = 0;

    /*
     * The same for a value which is not zero, which is what the right side
     * of an '||' is asked: it settles the answer when it holds.
     */
    virtual void JumpIfNonZero(reg_idx reg, label_idx label) = 0;

    /*
     * Put the value of one argument on the stack while a call is being
     * made, ready for Call() to hand it to the function.
     */
    virtual void PushArgument(reg_idx reg) = 0;

    /*
     * Call a function with the argument values which have been pushed,
     * and give back the register which holds its result.
     */
    virtual reg_idx Call(const Symbol& symbol, int32 arg_count) = 0;
    virtual void Return(reg_idx reg) = 0;
    virtual void PrintInt(reg_idx reg) = 0;

    virtual reg_idx AddressOf(const Symbol& symbol) = 0;
    virtual void AddOffset(reg_idx reg, int32 offset) = 0;
    virtual reg_idx LoadStrAddress(label_idx label) = 0;
    virtual reg_idx Deref(reg_idx reg, PrimitiveType pointer_type) = 0;

    /*
     * Read the value at an address into a register of its own, leaving the
     * address where it is. A read followed by a write to the same place
     * needs the address to survive the read, which is what a compound
     * assignment does; the type is the type of the value read.
     */
    virtual reg_idx LoadDeref(reg_idx address_reg, PrimitiveType type) = 0;
    virtual void StoreDeref(reg_idx value_reg,
                            reg_idx address_reg,
                            PrimitiveType type) = 0;

    /* The parts of the walk which are the same everywhere. */

    /*
     * Reserve the storage of a global variable. The variables are laid out
     * one after another in the data section, in the order they are
     * declared, so that a program can reach one by adding an offset to the
     * address of another.
     */
    void GenGlobSymbol(const Symbol& symbol);

    /*
     * Write the start of a global variable's storage: the data section, its
     * name as a global, the room its alignment needs and the label the
     * storage begins at. The label comes after the alignment, or it would
     * point at the padding in front of the storage.
     */
    void EmitGlobHeading(const Symbol& symbol);

    /*
     * Give a string literal storage in the data section, under a label
     * of its own, so that the code can take the address of it.
     */
    void GenStrStorage(label_idx label, const std::string& text);

    void GenStatement(const AstNodePtr& root);
    void GenIf(const AstNodePtr& root);
    void GenWhile(const AstNodePtr& root);
    void GenFor(const AstNodePtr& root);
    void GenSwitch(const AstNodePtr& root);
    reg_idx GenTernary(const AstNodePtr& root);

    /*
     * Generate the code for '&&' or '||', which answer with the zero or the
     * one saying whether they hold. The side which is not needed to know
     * the answer is not worked out at all, which is why the two sides are
     * asked one after the other rather than worked out before the
     * operation is applied to them.
     */
    reg_idx GenLogic(const AstNodePtr& root, bool is_and);
    void GenCondition(const AstNodePtr& condition, label_idx false_label);
    reg_idx GenExpr(const AstNodePtr& root);
    reg_idx GenCall(const AstNodePtr& root);
    reg_idx GenAssign(const AstNodePtr& root);

    /*
     * Generate the code for a compound assignment, which combines the value
     * the target holds now with the value on its right and stores the
     * result back where the target is. The operator is the one the
     * assignment combines with.
     */
    reg_idx GenAssignOp(const AstNodePtr& root, NodeTag op_type);
    reg_idx GenAddress(const AstNodePtr& root);
    reg_idx GenOperator(NodeTag op_type, reg_idx left_reg, reg_idx right_reg);

    reg_idx AllocRegister();
    void FreeRegister(reg_idx reg);
    void FreeAllRegister();

    label_idx NewLabel();

    /* The registers which are free, which a target may want to know. */
    const bool* FreeRegisters() const;

protected:
    bool is_free_[kRegSize];
    /*
     * How many values each register has set aside, and which register is
     * next in turn to have one set aside. A register with values set aside
     * may still be in use, holding a newer value of its own.
     */
    int32 spill_count_[kRegSize] = {};
    reg_idx spill_next_ = 0;
    label_idx next_label_;
    /* The function whose code is being generated. */
    label_idx function_end_label_;
    PrimitiveType function_return_type_;
    int32 function_local_size_;
    /*
     * Where the two kinds of loop jump leave the loop being generated: a
     * break goes to the label after it and a continue to the label the
     * next pass starts at. They are saved and put back around each loop,
     * so a loop within a loop keeps its own pair. Neither is set while no
     * loop is open, and the parser refuses a break or a continue there.
     */
    label_idx loop_break_label_ = 0;
    label_idx loop_continue_label_ = 0;
    std::ofstream ofs_;
};

/*
 * Build the code generator for the machine this compiler was built for.
 * Each target implements it, the build picks one of them.
 */
std::unique_ptr<AsmCodegen> MakeCodegen(const std::string& output_file);

}   /* namespace nuocc */

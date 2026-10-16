#pragma once

#include <string>

#include "nuocc_asm_codegen.hpp"

namespace nuocc
{

/*
 * The part of a code generator which writes the assembly text around the
 * instructions: the storage of the global variables and the storage of the
 * string literals, which both machine targets are written with. What the
 * two of them differ in is the instructions, which only they know.
 */
class AssemblyCodegen : public AsmCodegen
{
public:
    explicit AssemblyCodegen(const std::string& output_file);
    ~AssemblyCodegen() override = default;

protected:
    void EmitEndOfProgram() override;
    void BeginGlobal(const Symbol& symbol, int32 alignment) override;
    void EmitGlobalSpace(int32 size) override;
    void EmitGlobalValue(int32 value, int32 size) override;
    void EmitGlobalLabelRef(label_idx label) override;
    void EndGlobal() override;
    void GenStrStorage(label_idx label, const std::string& text) override;
};

}   /* namespace nuocc */

// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Masahiro Goshima.
// Copyright (c) 2005-2017 Ryota Shioya.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>

#include "Emu/STRAIGHT64Linux/STRAIGHT64Converter.h"

//#include "Emu/Utility/GenericOperation.h"
//#include "Emu/Utility/OpEmulationState.h"

#include "Emu/STRAIGHT64Linux/STRAIGHT64Info.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64OpInfo.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Decoder.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Operation.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Converter.h"
#include "Emu/RISCV32Linux/RISCV32Operation.h"
#include "Emu/RISCV64Linux/RISCV64Operation.h"

using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::STRAIGHT64Linux;
using namespace Onikiri::EmulatorUtility::Operation;
using namespace Onikiri::STRAIGHT64Linux::Operation;
using namespace Onikiri::RISCV32Linux::Operation;
using namespace Onikiri::RISCV64Linux::Operation;

namespace {
    // オペランドのテンプレート
    // [RegTemplateBegin, RegTemplateEnd] は，命令中のオペランドレジスタ番号に置き換えられる
    // [ImmTemplateBegin, RegTemplateEnd] は，即値に置き換えられる

    // レジスタ・テンプレートに使用する番号
    // 本物のレジスタ番号を使ってはならない
    const int RegTemplateBegin = -20;    // 命令中のレジスタ番号に変換 (数値に意味はない．本物のレジスタ番号と重ならずかつ一意であればよい)
    const int RegTemplateEnd = RegTemplateBegin + 4 - 1;
    const int ImmTemplateBegin = -30;    // 即値に変換
    const int ImmTemplateEnd = ImmTemplateBegin + 2 - 1;

    const int R0 = RegTemplateBegin + 0;
    const int R1 = RegTemplateBegin + 1;
    const int R2 = RegTemplateBegin + 2;
    const int R3 = RegTemplateBegin + 3;

    const int I0 = ImmTemplateBegin + 0;
    const int I1 = ImmTemplateBegin + 1;
}


//
// 命令の定義
//

namespace {
    // 各命令形式に対するオペコードを得るためのマスク (0のビットが引数)
    const u32 MASK_EXACT = 0xffffffff;  // 全bitが一致

    const u32 MASK_STB = 0x3f; // Store/Branchは最下位6bitがOPCODE
    const u32 MASK_ONEREG = 0x1fff; // 最下位13bitを見る
    const u32 MASK_SFTIMM32 = 0x183ffff; // 最下位18bitに加えて2bitが0のはず
    const u32 MASK_SFTIMM64 = 0x103ffff; // 最下位18bitに加えて1bitが0のはず
    const u32 MASK_TWOREG = 0x3ffff; // 最下位18bit
    const u32 MASK_NOREG = 0xfff; // 最下位12bit
    const u32 MASK_FLOAT = 0x3ffff; // 最下位18bit
}

#define STRAIGHT64_DSTOP(n) STRAIGHT64DstOperand<n>
#define STRAIGHT64_SRCOP(n) STRAIGHT64SrcOperand<n>
#define STRAIGHT64_SRCOPFLOAT(n) AsFP<f32, Cast<u32, SrcOperand<n> > > // lower 32-bit holds float value
#define STRAIGHT64_SRCOPDOUBLE(n) AsFP<f64, SrcOperand<n> >

#define OPCODE_ONEREG32(opcode) (((opcode)<<8) | 0b10'000'0'1001111)
#define OPCODE_ONEREG64(opcode) (((opcode)<<8) | 0b10'000'1'1001111)

#define OPCODE_CTRLMEM(opcode) (0b0001111 | ((opcode) << 7))

                                   // ..' imm ' x   '..'opc'.'.......
#define OPCODE_SFTIMM32(opcode, a) (0b00'00000'00000'10'000'0'1001111 | ((opcode)<<8) | ((a)<<16))
#define OPCODE_SFTIMM64(opcode, a) (0b00'00000'00000'10'000'1'1001111 | ((opcode)<<8) | ((a)<<16))

#define OPCODE_TWOREG32(op1, op2) (0b00000'11'000'0'1001111 | (op1 << 13) | (op2 << 8))
#define OPCODE_TWOREG64(op1, op2) (0b00000'11'000'1'1001111 | (op1 << 13) | (op2 << 8))

#define OPCODE_NOREG(opcode) (0b00'011'0001111 | (opcode << 9))

                                  //    ' RM'b'
#define OPCODE_FLOAT32(opcode) (0b00'000'0'1001111 | (opcode<<13))
#define OPCODE_FLOAT64(opcode) (0b00'000'1'1001111 | (opcode<<13))

#define D0 STRAIGHT64_DSTOP(0)
#define D1 STRAIGHT64_DSTOP(1)
#define S0 STRAIGHT64_SRCOP(0)
#define S1 STRAIGHT64_SRCOP(1)
#define S2 STRAIGHT64_SRCOP(2)
#define S3 STRAIGHT64_SRCOP(3)
#define SF0 STRAIGHT64_SRCOPFLOAT(0)
#define SF1 STRAIGHT64_SRCOPFLOAT(1)
#define SF2 STRAIGHT64_SRCOPFLOAT(2)
#define SF3 STRAIGHT64_SRCOPFLOAT(3)
#define SD0 STRAIGHT64_SRCOPDOUBLE(0)
#define SD1 STRAIGHT64_SRCOPDOUBLE(1)
#define SD2 STRAIGHT64_SRCOPDOUBLE(2)
#define SD3 STRAIGHT64_SRCOPDOUBLE(3)

// 投機的にフェッチされたときにはエラーにせず，実行されたときにエラーにする
// syscallにすることにより，直前までの命令が完了してから実行される (実行は投機的でない)
STRAIGHT64Converter::OpDef STRAIGHT64Converter::m_OpDefUnknown = 
    {"unknown", MASK_EXACT, 0,  1, {{OpClassCode::UNDEF,    {-1, -1}, {I0, -1, -1}, STRAIGHTUnknownOperation}}};


// branchは，OpInfo 列の最後じゃないとだめ
STRAIGHT64Converter::OpDef STRAIGHT64Converter::m_OpDefsBase[] =
{
    //{Name,    Mask,        Opcode,        nOp, { OpClassCode,          Dst[],      Src[],          OpInfoType::EmulationFunc}[]}
    {"ST.8",    MASK_STB,    0b000111,      1,   { OpClassCode::iST,     {R0, -1},   {R1, R2, I0},   Set<D0, STRAIGHT64Store<u8, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"ST.16",   MASK_STB,    0b100111,      1,   { OpClassCode::iST,     {R0, -1},   {R1, R2, I0},   Set<D0, STRAIGHT64Store<u16, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"ST.32",   MASK_STB,    0b010111,      1,   { OpClassCode::iST,     {R0, -1},   {R1, R2, I0},   Set<D0, STRAIGHT64Store<u32, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"ST.64",   MASK_STB,    0b110111,      1,   { OpClassCode::iST,     {R0, -1},   {R1, R2, I0},   Set<D0, STRAIGHT64Store<u64, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"BLT",     MASK_STB,    0b000011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondLessSigned<u64> > >}},
    {"BGE",     MASK_STB,    0b100011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondGreaterEqualSigned<u64> > >}},
    {"BLTu",    MASK_STB,    0b010011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondLessUnsigned<u64> > >}},
    {"BGEu",    MASK_STB,    0b110011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondGreaterEqualUnsigned<u64> > >}},
    {"BEQ",     MASK_STB,    0b001011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondEqual<u64> > >}},
    {"BNE",     MASK_STB,    0b101011,      1,   { OpClassCode::iBC,     {-1, -1},   {R1, R2, I0},   RISCV64BranchRelCond<LShiftL<u64, S2, IntConst<u64, 1>, 0x3f >, Compare<S0, S1, IntCondNotEqual<u64> > >}},

    // ecall
    { "ECALL", MASK_EXACT, 0b010'0001111, 2, {
        { OpClassCode::syscall,        { R0, -1 }, { 1,  2,  3 },  STRAIGHT64SyscallSetArg },
        { OpClassCode::syscall_branch, { R0, -1 }, { R0,  4,  5 }, STRAIGHT64SyscallCore },
    } },

    // OneReg
    {"NOP/RPINC",  MASK_ONEREG, OPCODE_CTRLMEM(0b000'000), 1, { OpClassCode::iNOP, {-1, -1}, {I0, -1, -1}, NoOperation}}, // フロントエンドで処理するのでRPINCもNOP扱い
    {"JR",         MASK_ONEREG, OPCODE_CTRLMEM(0b000'001), 1, { OpClassCode::RET,  {R0, -1}, {R1, I0, -1}, RISCV64CallAbsUncond<D0, S0, S1>}},
    {"JALR",       MASK_ONEREG, OPCODE_CTRLMEM(0b001'001), 1, { OpClassCode::CALL_JUMP,  {R0, -1}, {R1, I0, -1}, RISCV64CallAbsUncond<D0, S0, S1>}},
    {"SPLD.8",     MASK_ONEREG, OPCODE_CTRLMEM(0b000'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<s8,  STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.16",    MASK_ONEREG, OPCODE_CTRLMEM(0b001'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<s16, STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.32",    MASK_ONEREG, OPCODE_CTRLMEM(0b010'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<s32, STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.64",    MASK_ONEREG, OPCODE_CTRLMEM(0b011'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<u64, STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.8u",    MASK_ONEREG, OPCODE_CTRLMEM(0b100'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<u8,  STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.16u",   MASK_ONEREG, OPCODE_CTRLMEM(0b101'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<u16, STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.32u",   MASK_ONEREG, OPCODE_CTRLMEM(0b110'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, Load<u32, STRAIGHT64Addr<S0, S1> > >}},
    {"SPLD.32f",   MASK_ONEREG, OPCODE_CTRLMEM(0b111'100), 1, { OpClassCode::iLD,  {R0, -1}, {I0, I1, -1}, Set<D0, RISCV64NanBoxing<AsFP<f32, Load<u32, STRAIGHT64Addr<S0, S1> > > > >}},
    {"SPST.8",     MASK_ONEREG, OPCODE_CTRLMEM(0b000'101), 1, { OpClassCode::iST,  {R0, -1}, {R1, I0, I1}, Set<D0, STRAIGHT64Store<u8,  S0, STRAIGHT64Addr<S1, S2> > > }},
    {"SPST.16",    MASK_ONEREG, OPCODE_CTRLMEM(0b001'101), 1, { OpClassCode::iST,  {R0, -1}, {R1, I0, I1}, Set<D0, STRAIGHT64Store<u16, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"SPST.32",    MASK_ONEREG, OPCODE_CTRLMEM(0b010'101), 1, { OpClassCode::iST,  {R0, -1}, {R1, I0, I1}, Set<D0, STRAIGHT64Store<u32, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"SPST.64",    MASK_ONEREG, OPCODE_CTRLMEM(0b011'101), 1, { OpClassCode::iST,  {R0, -1}, {R1, I0, I1}, Set<D0, STRAIGHT64Store<u64, S0, STRAIGHT64Addr<S1, S2> > > }},
    {"LD.8",       MASK_ONEREG, OPCODE_CTRLMEM(0b000'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<s8,  STRAIGHT64Addr<S0, S1> > >}},
    {"LD.16",      MASK_ONEREG, OPCODE_CTRLMEM(0b001'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<s16, STRAIGHT64Addr<S0, S1> > >}},
    {"LD.32",      MASK_ONEREG, OPCODE_CTRLMEM(0b010'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<s32, STRAIGHT64Addr<S0, S1> > >}},
    {"LD.64",      MASK_ONEREG, OPCODE_CTRLMEM(0b011'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<u64, STRAIGHT64Addr<S0, S1> > >}},
    {"LD.8u",      MASK_ONEREG, OPCODE_CTRLMEM(0b100'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<u8,  STRAIGHT64Addr<S0, S1> > >}},
    {"LD.16u",     MASK_ONEREG, OPCODE_CTRLMEM(0b101'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<u16, STRAIGHT64Addr<S0, S1> > >}},
    {"LD.32u",     MASK_ONEREG, OPCODE_CTRLMEM(0b110'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, Load<u32, STRAIGHT64Addr<S0, S1> > >}},
    {"LD.32f",     MASK_ONEREG, OPCODE_CTRLMEM(0b111'110), 1, { OpClassCode::iLD,  {R0, -1}, {R1, I0, -1}, Set<D0, RISCV64NanBoxing<AsFP<f32, Load<u32, STRAIGHT64Addr<S0, S1> > > > >}},


    // OneReg ALU
    {"ADDi.32",  MASK_ONEREG, OPCODE_ONEREG32(0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, SetSext<D0, IntAdd<u32, S0, S1> >}},
    {"SLTi.32",  MASK_ONEREG, OPCODE_ONEREG32(0b010), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, RISCV32Compare<S0, S1, IntCondLessSigned<u32> > >}},
    {"SLTiu.32", MASK_ONEREG, OPCODE_ONEREG32(0b011), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, RISCV32Compare<S0, S1, IntCondLessUnsigned<u32> > >}},
    {"XORi.32",  MASK_ONEREG, OPCODE_ONEREG32(0b100), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, SetSext<D0, BitXor<u32, S0, S1> >}},
    {"ORi.32",   MASK_ONEREG, OPCODE_ONEREG32(0b110), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, SetSext<D0, BitOr<u32, S0, S1> >}},
    {"ANDi.32",  MASK_ONEREG, OPCODE_ONEREG32(0b111), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, SetSext<D0, BitAnd<u32, S0, S1> >}},
    {"SLLi.32",  MASK_SFTIMM32, OPCODE_SFTIMM32(0b001, 0), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, SetSext<D0, LShiftL<u32, S0, S1, 0x1f> >}},
    {"SRLi.32",  MASK_SFTIMM32, OPCODE_SFTIMM32(0b101, 0), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, SetSext<D0, LShiftR<u32, S0, S1, 0x1f> >}},
    {"SRAi.32",  MASK_SFTIMM32, OPCODE_SFTIMM32(0b101, 1), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, SetSext<D0, AShiftR<u32, S0, S1, 0x1f> >}},
    
    {"ADDi.64",  MASK_ONEREG, OPCODE_ONEREG64(0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, IntAdd<u64, S0, S1> >}},
    {"SLTi.64",  MASK_ONEREG, OPCODE_ONEREG64(0b010), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, RISCV64Compare<S0, S1, IntCondLessSigned<u64> > >}},
    {"SLTiu.64", MASK_ONEREG, OPCODE_ONEREG64(0b011), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, RISCV64Compare<S0, S1, IntCondLessUnsigned<u64> > >}},
    {"XORi.64",  MASK_ONEREG, OPCODE_ONEREG64(0b100), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, BitXor<u64, S0, S1> >}},
    {"ORi.64",   MASK_ONEREG, OPCODE_ONEREG64(0b110), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, BitOr<u64, S0, S1> >}},
    {"ANDi.64",  MASK_ONEREG, OPCODE_ONEREG64(0b111), 1, { OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, BitAnd<u64, S0, S1> >}},
    {"SLLi.64",  MASK_SFTIMM64, OPCODE_SFTIMM64(0b001, 0), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, Set<D0, LShiftL<u64, S0, S1, 0x3f> >}},
    {"SRLi.64",  MASK_SFTIMM64, OPCODE_SFTIMM64(0b101, 0), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, Set<D0, LShiftR<u64, S0, S1, 0x3f> >}},
    {"SRAi.64",  MASK_SFTIMM64, OPCODE_SFTIMM64(0b101, 1), 1, { OpClassCode::iSFT, {R0, -1}, {R1, I0, -1}, Set<D0, AShiftR<u64, S0, S1, 0x3f> >}},

    // TwoReg
    {"ADD.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntAdd<u32, S0, S1> >}},
    {"SUB.32",   MASK_TWOREG, OPCODE_TWOREG32(0b01000, 0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntSub<u32, S0, S1> >}},
    {"SLL.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b001), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, SetSext<D0, LShiftL<u32, S0, S1, 0x1f> >}},
    {"SLT.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b010), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, Compare<S0, S1, IntCondLessSigned<u32> > >}},
    {"SLTu.32",  MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b011), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, Compare<S0, S1, IntCondLessUnsigned<u32> > >}},
    {"XOR.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b100), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, SetSext<D0, BitXor<u32, S0, S1> >}},
    {"SRL.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b101), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, SetSext<D0, LShiftR<u32, S0, S1, 0x1f> >}},
    {"SRA.32",   MASK_TWOREG, OPCODE_TWOREG32(0b01000, 0b101), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, SetSext<D0, AShiftR<u32, S0, S1, 0x1f> >}},
    {"OR.32",    MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b110), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, SetSext<D0, BitOr<u32, S0, S1> >}},
    {"AND.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00000, 0b111), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, SetSext<D0, BitAnd<u32, S0, S1> >}},
    {"MUL.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b000), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntMul<u32, S0, S1> >}},
    {"MULH.32",  MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b001), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntUMulh32<s64, s64, Cast<s32, S0>, Cast<s32, S1> > >}},
    {"MULHsu.32",MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b010), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntUMulh32<s64, u64, Cast<s32, S0>, Cast<u32, S1> > >}},
    {"MULHu.32", MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b011), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, SetSext<D0, IntUMulh32<u64, u64, Cast<u32, S0>, Cast<u32, S1> > >}},
    {"DIV.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b100), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, SetSext<D0, RISCV32IntDiv<S0, S1> >}},
    {"DIVu.32",  MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b101), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, SetSext<D0, RISCV32IntDivu<S0, S1> >}},
    {"REM.32",   MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b110), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, SetSext<D0, RISCV32IntRem<S0, S1> >}},
    {"REMu.32",  MASK_TWOREG, OPCODE_TWOREG32(0b00001, 0b111), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, SetSext<D0, RISCV32IntRemu<S0, S1> >}},

    {"ADD.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, IntAdd<u64, S0, S1> >}},
    {"SUB.64",   MASK_TWOREG, OPCODE_TWOREG64(0b01000, 0b000), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, IntSub<u64, S0, S1> >}},
    {"SLL.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b001), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, Set<D0, LShiftL<u64, S0, S1, 0x3f> >}},
    {"SLT.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b010), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, Compare<S0, S1, IntCondLessSigned<u64> > >}},
    {"SLTu.64",  MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b011), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, Compare<S0, S1, IntCondLessUnsigned<u64> > >}},
    {"XOR.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b100), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, BitXor<u64, S0, S1> >}},
    {"SRL.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b101), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, Set<D0, LShiftR<u64, S0, S1, 0x3f> >}},
    {"SRA.64",   MASK_TWOREG, OPCODE_TWOREG64(0b01000, 0b101), 1, { OpClassCode::iSFT, {R0, -1}, {R1, R2, -1}, Set<D0, AShiftR<u64, S0, S1, 0x3f> >}},
    {"OR.64",    MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b110), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, BitOr<u64, S0, S1> >}},
    {"AND.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00000, 0b111), 1, { OpClassCode::iALU, {R0, -1}, {R1, R2, -1}, Set<D0, BitAnd<u64, S0, S1> >}},
    {"MUL.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b000), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, Set<D0, IntMul<u64, S0, S1> >}},
    {"MULH.64",  MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b001), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, Set<D0, IntSMulh64<S0, S1> >}},
    {"MULHsu.64",MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b010), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, Set<D0, IntSUMulh64<S0, S1> >}},
    {"MULHu.64", MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b011), 1, { OpClassCode::iMUL, {R0, -1}, {R1, R2, -1}, Set<D0, IntUMulh64<S0, S1> >}},
    {"DIV.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b100), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64IntDiv<S0, S1> >}},
    {"DIVu.64",  MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b101), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64IntDivu<S0, S1> >}},
    {"REM.64",   MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b110), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64IntRem<S0, S1> >}},
    {"REMu.64",  MASK_TWOREG, OPCODE_TWOREG64(0b00001, 0b111), 1, { OpClassCode::iDIV, {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64IntRemu<S0, S1> >}},


    // No Reg
    {"J",       MASK_NOREG, OPCODE_NOREG(0b00'0), 1,  { { OpClassCode::iJUMP,     {-1, -1},   {I0, -1, -1},   RISCV64BranchRelUncond<LShiftL<u64, S0, IntConst<u64, 1>, 0x3f > > } } },
    {"JAL",     MASK_NOREG, OPCODE_NOREG(0b01'0), 1,  { { OpClassCode::CALL_JUMP, {R0, -1},   {I0, -1, -1},   RISCV64CallRelUncond<D0, LShiftL<u64, S0, IntConst<u64, 1>, 0x3f > > } } },
    {"LUi",     MASK_NOREG, OPCODE_NOREG(0b10'0), 1,  { { OpClassCode::iALU, {R0, -1}, {I0, -1, -1},   SetSext<D0, RISCV64Lui<S0> >} } },
    {"AUiPC",   MASK_NOREG, OPCODE_NOREG(0b11'0), 1,  { { OpClassCode::iALU,   {R0, -1},   {I0, -1, -1},   SetSext<D0, RISCV64Auipc<S0> >} } },
    {"SPADDi",  MASK_NOREG, OPCODE_NOREG(0b00'1), 1,  { { OpClassCode::iNOP, {-1, -1}, {I0, -1, -1},   NoOperation} } },
    {"AUiSP",   MASK_NOREG, OPCODE_NOREG(0b11'1), 1,  { { OpClassCode::iALU,   {R0, -1},   {I0, I1, -1},   Set<D0, STRAIGHT64Auisp<S0, S1> >} } },


    // pseudo instruction (special case of ADDi.64)
    {"RMOV", 0b0000000'111111111111'1111111111111, OPCODE_ONEREG64(0b000), 1, {OpClassCode::iMOV, {R0, -1}, {R1, I0, -1}, Set<D0, IntAdd<u64, S0, S1> >}},
    {"ADDi.64", MASK_EXACT, OPCODE_ONEREG64(0b000), 1, {OpClassCode::iALU, {R0, -1}, {R1, I0, -1}, Set<D0, IntAdd<u64, S0, S1> >}}, // more specific case: ADDi.64 $Zero, 0


    // Float
    { "FADD.s",     MASK_FLOAT, OPCODE_FLOAT32(0b00000), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<FPAdd<f32, SF0, SF1> > >} }, // ToDo: RM!!
    { "FSUB.s",     MASK_FLOAT, OPCODE_FLOAT32(0b00001), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<FPSub<f32, SF0, SF1> > >} }, // ToDo: RM!!
    { "FMUL.s",     MASK_FLOAT, OPCODE_FLOAT32(0b00010), 1, {OpClassCode::fMUL,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<FPMul<f32, SF0, SF1> > >} }, // ToDo: RM!!
    { "FDIV.s",     MASK_FLOAT, OPCODE_FLOAT32(0b00011), 1, {OpClassCode::fDIV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<FPDiv<f32, SF0, SF1> > >} }, // ToDo: RM!!
    { "FSQRT.s",    MASK_FLOAT, OPCODE_FLOAT32(0b00100), 1, {OpClassCode::fELEM,   {R0, -1}, {R1, I0, -1}, Set<D0, RISCV64NanBoxing<FPSqrt<f32, SF0> > >} }, // ToDo: RM!!
    { "FSGNJ.s",    MASK_FLOAT, OPCODE_FLOAT32(0b01000), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<AsFP<f32, RISCV64Linux::Operation::FPFLOATCopySign<S1, S0> > > >} },
    { "FSGNJN.s",   MASK_FLOAT, OPCODE_FLOAT32(0b01001), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<AsFP<f32, RISCV64Linux::Operation::FPFLOATCopySignNeg<S1, S0> > > >} },
    { "FSGNJX.s",   MASK_FLOAT, OPCODE_FLOAT32(0b01010), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<AsFP<f32, RISCV64Linux::Operation::FPFLOATCopySignXor<S1, S0> > > >} },
    { "FMIN.s",     MASK_FLOAT, OPCODE_FLOAT32(0b01100), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<RISCV64FPMIN<f32, SF0, SF1> > >} },
    { "FMAX.s",     MASK_FLOAT, OPCODE_FLOAT32(0b01101), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<RISCV64FPMAX<f32, SF0, SF1> > >} },
    { "FCLASS.s",   MASK_FLOAT, OPCODE_FLOAT32(0b10000), 1, {OpClassCode::fADD,    {R0, -1}, {R1, -1, -1}, Set<D0, RISCV64FCLASS<f32, SF0> >} },
    { "FEQ.s",      MASK_FLOAT, OPCODE_FLOAT32(0b10001), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPEqual<f32, SF0, SF1> >} },
    { "FLT.s",      MASK_FLOAT, OPCODE_FLOAT32(0b10010), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPLess<f32, SF0, SF1> >} },
    { "FLE.s",      MASK_FLOAT, OPCODE_FLOAT32(0b10011), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPLessEqual<f32, SF0, SF1> >} },
    { "FCVT.s.d",   MASK_FLOAT, OPCODE_FLOAT32(0b10100), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<CastFP<f32, SD0> > >} }, // ToDo: RM!!
    { "FCVT.32.s",  MASK_FLOAT, OPCODE_FLOAT32(0b11000), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetSext<D0, RISCV64FPToInt<s32, SF0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.32u.s", MASK_FLOAT, OPCODE_FLOAT32(0b11001), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetSext<D0, RISCV64FPToInt<u32, SF0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.s.32",  MASK_FLOAT, OPCODE_FLOAT32(0b11010), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<CastFP<f32, Cast<s32, S0> > > >} }, // ToDo: RM!!
    { "FCVT.s.32u", MASK_FLOAT, OPCODE_FLOAT32(0b11011), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<CastFP<f32, Cast<u32, S0> > > >} }, // ToDo: RM!!
    { "FCVT.64.s",  MASK_FLOAT, OPCODE_FLOAT32(0b11100), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64FPToInt<s64, SF0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.64u.s", MASK_FLOAT, OPCODE_FLOAT32(0b11101), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64FPToInt<u64, SF0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.s.64",  MASK_FLOAT, OPCODE_FLOAT32(0b11110), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<CastFP<f32, Cast<s64, S0> > > >} }, // ToDo: RM!!
    { "FCVT.s.64u", MASK_FLOAT, OPCODE_FLOAT32(0b11111), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64NanBoxing<CastFP<f32, Cast<u64, S0> > > >} }, // ToDo: RM!!

    // Double
    { "FADD.d",     MASK_FLOAT, OPCODE_FLOAT64(0b00000), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, SetFP<D0, FPAdd<f64, SD0, SD1> >} }, // ToDo: RM!!
    { "FSUB.d",     MASK_FLOAT, OPCODE_FLOAT64(0b00001), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, I0}, SetFP<D0, FPSub<f64, SD0, SD1> >} }, // ToDo: RM!!
    { "FMUL.d",     MASK_FLOAT, OPCODE_FLOAT64(0b00010), 1, {OpClassCode::fMUL,    {R0, -1}, {R1, R2, I0}, SetFP<D0, FPMul<f64, SD0, SD1> >} }, // ToDo: RM!!
    { "FDIV.d",     MASK_FLOAT, OPCODE_FLOAT64(0b00011), 1, {OpClassCode::fDIV,    {R0, -1}, {R1, R2, I0}, SetFP<D0, FPDiv<f64, SD0, SD1> >} }, // ToDo: RM!!
    { "FSQRT.d",    MASK_FLOAT, OPCODE_FLOAT64(0b00100), 1, {OpClassCode::fELEM,   {R0, -1}, {R1, I0, -1}, SetFP<D0, FPSqrt<f64, SD0> >} }, // ToDo: RM!!
    { "FSGNJ.d",    MASK_FLOAT, OPCODE_FLOAT64(0b01000), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64Linux::Operation::FPDoubleCopySign<S1, S0> >} },
    { "FSGNJN.d",   MASK_FLOAT, OPCODE_FLOAT64(0b01001), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64Linux::Operation::FPDoubleCopySignNeg<S1, S0> >} },
    { "FSGNJX.d",   MASK_FLOAT, OPCODE_FLOAT64(0b01010), 1, {OpClassCode::fMOV,    {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64Linux::Operation::FPDoubleCopySignXor<S1, S0> >} },
    { "FMIN.d",     MASK_FLOAT, OPCODE_FLOAT64(0b01100), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, SetFP<D0, RISCV64FPMIN<f64, SD0, SD1> >} },
    { "FMAX.d",     MASK_FLOAT, OPCODE_FLOAT64(0b01101), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, SetFP<D0, RISCV64FPMAX<f64, SD0, SD1> >} },
    { "FCLASS.d",   MASK_FLOAT, OPCODE_FLOAT64(0b10000), 1, {OpClassCode::fADD,    {R0, -1}, {R1, -1, -1}, SetFP<D0, RISCV64FCLASS<f64, SD0> >} },
    { "FEQ.d",      MASK_FLOAT, OPCODE_FLOAT64(0b10001), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPEqual<f64, SD0, SD1> >} },
    { "FLT.d",      MASK_FLOAT, OPCODE_FLOAT64(0b10010), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPLess<f64, SD0, SD1> >} },
    { "FLE.d",      MASK_FLOAT, OPCODE_FLOAT64(0b10011), 1, {OpClassCode::fADD,    {R0, -1}, {R1, R2, -1}, Set<D0, RISCV64FPLessEqual<f64, SD0, SD1> >} },
    { "FCVT.d.s",   MASK_FLOAT, OPCODE_FLOAT64(0b10100), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetFP<D0, CastFP<f64, SF0> >} }, // ToDo: RM!!
    { "FCVT.32.d",  MASK_FLOAT, OPCODE_FLOAT64(0b11000), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetSext<D0, RISCV64FPToInt<s32, SD0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.32u.d", MASK_FLOAT, OPCODE_FLOAT64(0b11001), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetSext<D0, RISCV64FPToInt<u32, SD0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.d.32",  MASK_FLOAT, OPCODE_FLOAT64(0b11010), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetFP<D0, CastFP<f64, Cast<s32, S0> > >} }, // ToDo: RM!!
    { "FCVT.d.32u", MASK_FLOAT, OPCODE_FLOAT64(0b11011), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetFP<D0, CastFP<f64, Cast<u32, S0> > >} }, // ToDo: RM!!
    { "FCVT.64.d",  MASK_FLOAT, OPCODE_FLOAT64(0b11100), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64FPToInt<s64, SD0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.64u.d", MASK_FLOAT, OPCODE_FLOAT64(0b11101), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, Set<D0, RISCV64FPToInt<u64, SD0, IntConst<int, FE_TOWARDZERO> > >} }, // ToDo: RM!!
    { "FCVT.d.64",  MASK_FLOAT, OPCODE_FLOAT64(0b11110), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetFP<D0, CastFP<f64, Cast<s64, S0> > >} }, // ToDo: RM!!
    { "FCVT.d.64u", MASK_FLOAT, OPCODE_FLOAT64(0b11111), 1, {OpClassCode::ifCONV,  {R0, -1}, {R1, R2, I0}, SetFP<D0, CastFP<f64, Cast<u64, S0> > >} }, // ToDo: RM!!


};

//
// STRAIGHT64Converter
//


STRAIGHT64Converter::STRAIGHT64Converter()
{
    AddToOpMap(m_OpDefsBase, sizeof(m_OpDefsBase)/sizeof(OpDef));
}

STRAIGHT64Converter::~STRAIGHT64Converter()
{
}

// srcTemplate に対応するオペランドの種類と，レジスタならば番号を，即値ならばindexを返す
std::pair<STRAIGHT64Converter::OperandType, int> STRAIGHT64Converter::GetActualSrcOperand(int srcTemplate, const DecodedInsn& decoded) const
{
    typedef std::pair<OperandType, int> RetType;
    if (srcTemplate == -1) {
        return RetType(OpInfoType::NONE, 0);
    }
    else if (ImmTemplateBegin <= srcTemplate && srcTemplate <= ImmTemplateEnd) {
        return RetType(OpInfoType::IMM, srcTemplate - ImmTemplateBegin);
    }
    else  {
        return RetType(OpInfoType::REG, GetActualRegNumber(srcTemplate, decoded) );
    }
}

// regTemplate から実際のレジスタ番号を取得する
int STRAIGHT64Converter::GetActualRegNumber(int regTemplate, const DecodedInsn& decoded) const
{
    if (regTemplate == -1) {
        return -1;
    }
    else if (RegTemplateBegin <= regTemplate && regTemplate <= RegTemplateEnd) {
        return decoded.Reg[regTemplate - RegTemplateBegin];
    }
    else if (0 <= regTemplate && regTemplate < STRAIGHT64Info::RegisterCount) {
        return regTemplate;
    }
    else {
        ASSERT(0, "STRAIGHT64Converter Logic Error : invalid regTemplate");
        return -1;
    }
}

// レジスタ番号regNumがゼロレジスタかどうかを判定する
bool STRAIGHT64Converter::IsZeroReg(int regNum) const
{
    return regNum == STRAIGHT64Info::ZeroRegIndex;
}


void STRAIGHT64Converter::STRAIGHTUnknownOperation(OpEmulationState* opState)
{
    u32 codeWord = static_cast<u32>( SrcOperand<0>()(opState) );

    DecoderType decoder;
    DecodedInsn decoded;
    decoder.Decode( codeWord, &decoded);

    stringstream ss;
    u32 opcode = (codeWord & 0xfc000000) >> 26;
    ss << "unknown instruction : " << hex << setw(8) << codeWord << endl;
    ss << "\topcode : " << hex << opcode << endl;
    ss << "\timm[1] : " << hex << decoded.Imm[1] << endl;
    ss << "(PC = " << hex << opState->GetPC() << ")" << endl;

    THROW_RUNTIME_ERROR(ss.str().c_str());
}

const STRAIGHT64Converter::OpDef* STRAIGHT64Converter::GetOpDefUnknown() const
{
    return &m_OpDefUnknown;
}

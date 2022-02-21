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

#include "Emu/STRAIGHT64Linux/STRAIGHT64Decoder.h"

#include "SysDeps/Endian.h"
#include "Emu/Utility/DecoderUtility.h"


using namespace std;
using namespace boost;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::STRAIGHT64Linux;


STRAIGHT64Decoder::DecodedInsn::DecodedInsn()
{
    clear();
}

void STRAIGHT64Decoder::DecodedInsn::clear()
{
    CodeWord = 0;

    std::fill(Imm.begin(), Imm.end(), 0);
    std::fill(Reg.begin(), Reg.end(), -1);
}

void STRAIGHT64Decoder::Decode(u32 codeWord, DecodedInsn* out)
{
    out->clear();
    out->CodeWord = codeWord;
    out->instType = GetInstType(codeWord);
    out->Reg[0] = 0;

    switch (out->instType) {
    case INSTTYPE_STB:
        out->Imm[0] = ExtractBits(static_cast<u64>(codeWord), 6, 12, true);
        out->Reg[2] = ExtractBits(codeWord, 18, 7, false);
        out->Reg[1] = ExtractBits(codeWord, 25, 7, false);
        break;
    case INSTTYPE_ECALL:
        break;
    case INSTTYPE_RPINC:
        out->Imm[0] = ExtractBits(static_cast<u64>(codeWord), 25, 7, false);
        break;
    case INSTTYPE_ONEREG:
        out->Imm[0] = ExtractBits(static_cast<u64>(codeWord), 13, 12, true);
        out->Reg[1] = ExtractBits(codeWord, 25, 7, false);

        // shift instructions
        if (((codeWord >> 8) & 0b111) == 0b001 || ((codeWord >> 8) & 0b111) == 0b101)
        {
            bool is64bit = (codeWord >> 7) & 1;
            out->Imm[0] = ExtractBits(static_cast<u64>(codeWord), 18, is64bit ? 6 : 5, false);
        }

        break;
    case INSTTYPE_TWOREG:
        out->Reg[2] = ExtractBits(codeWord, 18, 7, false);
        out->Reg[1] = ExtractBits(codeWord, 25, 7, false);
        break;
    case INSTTYPE_NOREG:
        out->Imm[0] = ExtractBits(static_cast<u64>(codeWord), 12, 20, true);
        break;
    case INSTTYPE_FLOAT:
        out->Imm[0] = ExtractBits(codeWord, 8, 3, false);   // RM
        out->Reg[2] = ExtractBits(codeWord, 18, 7, false);
        out->Reg[1] = ExtractBits(codeWord, 25, 7, false);
        break;
    default:;
        // 投機的フェッチなどにより変な命令が来ることはある
        // そのため、THROW_RUNTIME_ERRORは不適切
    }


    for (unsigned int i = 1; i < out->Reg.size(); i++)
    {
        if (out->Reg[i] == 0)
        {
            out->Reg[i] = STRAIGHT64Info::ZeroRegIndex;
        }
    }
}

INSTTYPE STRAIGHT64Decoder::GetInstType(const u32 codeWord)
{
    switch (codeWord & 0x1f)
    {
    case 0b000111: // ST.8
    case 0b100111: // ST.16
    case 0b010111: // ST.32
    case 0b110111: // ST.64
    case 0b000011: // Blt
    case 0b100011: // Bge
    case 0b010011: // Bltu
    case 0b110011: // Bgeu
    case 0b001011: // Beq
    case 0b101011: // Bne
        return INSTTYPE_STB;
    }

    if ((codeWord & 0x1ffffff) == 0b1111)
        return INSTTYPE_RPINC;

    if (codeWord == 0b010'0001111)
        return INSTTYPE_ECALL;

    if ((codeWord & 0x7f) == 0xf && (codeWord >> 7 & 0x3) != 0x3)
        return INSTTYPE_ONEREG;     // SPLD/SPST/LD/...
    if ((codeWord & 0x7f) == 0x4f && (codeWord >> 11 & 0b11) == 0b10)
        return INSTTYPE_ONEREG; // ADDi/SLTi/...
    
    if ((codeWord & 0x7f) == 0x4f && (codeWord >> 11 & 0b11) == 0b11)
        return INSTTYPE_TWOREG;

    if ((codeWord & 0x1ff) == 0x18f)
        return INSTTYPE_NOREG; // J/JAL/LUi/AUiPC/SPADDi/AUiSP

    if ((codeWord & 0x7f) == 0x4f && (codeWord >> 11 & 0b11) == 0b00)
        return INSTTYPE_FLOAT; // FADD/FCVT/...

    // 投機的フェッチなどにより変な命令が来ることはある
    // そのため、THROW_RUNTIME_ERRORは不適切
    return INSTTYPE_ERROR;
}

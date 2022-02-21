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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_DECODER_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_DECODER_H

#include "Emu/STRAIGHT64Linux/STRAIGHT64Info.h"

namespace Onikiri
{
    namespace STRAIGHT64Linux
    {
        enum INSTTYPE
        {
            INSTTYPE_STB,    // Store or Branch
            INSTTYPE_ONEREG, // One Reg (imm12bit)
            INSTTYPE_ECALL,  // ECALL
            INSTTYPE_RPINC,  // NOP/RPINC
            INSTTYPE_TWOREG, // Two Reg
            INSTTYPE_NOREG,  // No Reg
            INSTTYPE_FLOAT,  // Float
            INSTTYPE_ERROR,
        };
        class STRAIGHT64Decoder
        {
        public:
            struct DecodedInsn
            {
                // 即値
                boost::array<u64, 2> Imm;
                // オペランド・レジスタ(dest, src1, src2)
                boost::array<int, 3> Reg;

                INSTTYPE instType;

                u32 CodeWord;

                DecodedInsn();
                void clear();
            };
            STRAIGHT64Decoder(){}

            // 命令codeWordをデコードし，outに格納する
            void Decode(u32 codeWord, DecodedInsn* out);

        private:
            INSTTYPE GetInstType(u32 codeword);
        };

    } // namespace STRAIGHT64Linux
} // namespace Onikiri

#endif

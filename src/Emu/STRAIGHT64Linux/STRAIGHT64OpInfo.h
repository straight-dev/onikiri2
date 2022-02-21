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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_OPINFO_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_OPINFO_H

#include "Emu/Utility/CommonOpInfo.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Info.h"

namespace Onikiri {
    namespace STRAIGHT64Linux {

        class STRAIGHT64OpInfo : public EmulatorUtility::CommonOpInfo<STRAIGHT64Info>
        {
            bool m_cached;
            bool m_isSPADDi;
            bool m_isRPINC;
            bool m_isSPLDSTorAUiSP;
            bool m_isSPST;
            bool m_isSyscall;
            bool m_isSyscallBranch;
        public:
            explicit STRAIGHT64OpInfo(OpClass opClass) : CommonOpInfo<STRAIGHT64Info>(opClass), m_cached(false) {}
            
            void createCache() {
                if (!m_cached) {
                    m_cached = true;
                    m_isSPADDi = strcmp(GetMnemonic(), "SPADDi") == 0;
                    m_isRPINC = strcmp(GetMnemonic(), "NOP/RPINC") == 0;
                    m_isSPLDSTorAUiSP = strstr(GetMnemonic(), "SPLD") != nullptr || strstr(GetMnemonic(), "SPST") != nullptr || strcmp(GetMnemonic(), "AUiSP") == 0;
                    m_isSPST = strstr(GetMnemonic(), "SPST") != nullptr;
                    m_isSyscall = GetOpClass().GetCode() == OpClassCode::OpClassCode::syscall;
                    m_isSyscallBranch = GetOpClass().GetCode() == OpClassCode::OpClassCode::syscall_branch;
                }
            }

            bool isSPADDi() { ASSERT(m_cached); return m_isSPADDi; }
            bool isRPINC() { ASSERT(m_cached); return m_isRPINC; }
            bool isSPLDSTorAUiSP() { ASSERT(m_cached); return m_isSPLDSTorAUiSP; }
            bool isSPST() { ASSERT(m_cached); return m_isSPST; }

            bool isSyscall() { ASSERT(m_cached); return m_isSyscall; }
            bool isSyscallBranch() { ASSERT(m_cached); return m_isSyscallBranch; }
        };

    } // namespace STRAIGHT64Linux
} // namespace Onikiri

#endif


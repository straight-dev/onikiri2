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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_EMULATOR_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_EMULATOR_H

#include "Emu/Utility/CommonEmulator.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxTraits.h"

#define STRAIGHTSYSTEM_NOCHECK

namespace Onikiri
{
    namespace STRAIGHT64Linux
    {

#ifdef STRAIGHTSYSTEM_NOCHECK
        inline STRAIGHT64OpInfo* cast_STRAIGHT64OpInfo(OpInfo* p) { return static_cast<STRAIGHT64OpInfo*>(p); }
#else
        inline STRAIGHT64OpInfo* cast_STRAIGHT64OpInfo(Op* p) {
            auto* q = dynamic_cast<STRAIGHT64OpInfo*>(p);
            if (q == nullptr)
            {
                THROW_RUNTIME_ERROR("OpInfo doesn't have STRAIGHT64OpInfo type.");
            }
            return q;
        }
#endif

        class STRAIGHT64LinuxEmulator : public EmulatorUtility::CommonEmulator<STRAIGHT64LinuxTraits>
        {
        public:
            typedef EmulatorUtility::CommonEmulator<STRAIGHT64LinuxTraits> BaseEmulator;
            STRAIGHT64LinuxEmulator(SystemIF* simSystem) : BaseEmulator(simSystem)
            {
                m_opInfoArrayIndex = 0;
                for (int i = 0; i < OPINFO_ARRAY_CAPACITY; i++)
                {
                    m_opInfoArray[i] = new STRAIGHT64OpInfo(OpClassCode::UNDEF);
                }
            }

            virtual std::pair<OpInfo**, int> GetOp(PC pc)
            {
                auto opInfoTuple = EmulatorUtility::CommonEmulator<STRAIGHT64LinuxTraits>::GetOp(pc);
                if (m_opInfoArrayIndex + opInfoTuple.second >= OPINFO_ARRAY_CAPACITY) { m_opInfoArrayIndex = 0; }
                auto opInfoArrayIndex = m_opInfoArrayIndex;
                for (int i = 0; i < opInfoTuple.second; i++)
                {
                    cast_STRAIGHT64OpInfo(opInfoTuple.first[i])->createCache();
                    *(m_opInfoArray[m_opInfoArrayIndex]) = *cast_STRAIGHT64OpInfo(opInfoTuple.first[i]);
                    m_opInfoArrayIndex = (m_opInfoArrayIndex + 1) % OPINFO_ARRAY_CAPACITY;
                }
                assert(opInfoTuple.second <= m_opInfoArrayIndex);
                return std::make_pair(reinterpret_cast<OpInfo**>(&m_opInfoArray[opInfoArrayIndex]), opInfoTuple.second);
            }

        private:
            static const int OPINFO_ARRAY_CAPACITY = 100000;
            STRAIGHT64OpInfo* m_opInfoArray[OPINFO_ARRAY_CAPACITY];
            int m_opInfoArrayIndex;
        };

    } // namespace STRAIGHT64Linux
} // namespace Onikiri

#endif

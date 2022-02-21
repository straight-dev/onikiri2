//
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
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

#ifndef __EMULATORUTILITY_STRAIGHTSYSTEM_H__
#define __EMULATORUTILITY_STRAIGHTSYSTEM_H__

#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Op/OpContainer/OpExtraStateTable.h"
#include "Sim/Foundation/Resource/ResourceNode.h"
#include "Sim/Core/Core.h"
#include "Sim/System/ForwardEmulator.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxEmulator.h"

namespace Onikiri
{
    namespace STRAIGHT64Linux
    {
        class STRAIGHTSystem : public PhysicalResourceNode
        {
        public:
            STRAIGHTSystem();
            virtual ~STRAIGHTSystem();
            virtual void Initialize(InitPhase phase);
            virtual void Finalize();
            virtual void ChangeSimulationMode(SimulationMode mode);
            u64 GetRP(OpIterator op) { return m_rpTable[op]; }
            void SetRP(OpIterator op, u64 rp) { m_rpTable[op] = rp; }
            u64 GetSP(OpIterator op) { return m_spTable[op]; }
            void SetSP(OpIterator op, u64 sp) { m_spTable[op] = sp; }

            void Rename(std::pair<OpInfo**, int> ops, u64 rp);

            void AfterEmulatorGetOp(STRAIGHT64Linux::STRAIGHT64LinuxEmulator::GetOpHookParam* param);
            void OnFetch(Fetcher::FetchHookParam* param);
            void AfterFetcherGetOp(Fetcher::GetOpHookParam* param);
            void AfterForwardEmulatorGetOp(ForwardEmulator::GetOpHookParam* param);
            void BeforeFlush(OpIterator op);

            BEGIN_PARAM_MAP("")
                BEGIN_PARAM_PATH(GetParamPath())
                END_PARAM_PATH()
                BEGIN_PARAM_PATH(GetResultPath())
                END_PARAM_PATH()
            END_PARAM_MAP()

            BEGIN_RESOURCE_MAP()
                RESOURCE_ENTRY(Core, "core", m_core)
                RESOURCE_ENTRY(EmulatorIF, "emulator", m_emulator);
            END_RESOURCE_MAP()

        private:
            u64 m_rp;
            u64 m_emuRP; // RP for Emulator & ForwardEmulator
            OpExtraStateTable<u64> m_rpTable; // hold RP when the op is decoded

            u64 m_sp;
            u64 m_emuSP; // SP for Emulator & ForwardEmulator
            OpExtraStateTable<u64> m_spTable; // hold SP when the op is decoded

            Core* m_core;
            EmulatorIF* m_emulator;
        };

        static const char g_straightSystemModuleParam[] = "     \n\
                                                    \n\
    <?xml version='1.0' encoding='UTF-8'?>          \n\
    <Session>                                       \n\
      <Simulator>                                   \n\
        <Configurations>                            \n\
          <DefaultConfiguration>                    \n\
                                                    \n\
            <Structure>                             \n\
              <Copy>                                \n\
                <STRAIGHTSystem                     \n\
                  Count = 'CoreCount'               \n\
                  Name  = 'straightSystem'          \n\
                >                                   \n\
                  <Core Name='core'/>  \n\
                  <EmulatorIF Name='emulator'/>     \n\
                </STRAIGHTSystem>                   \n\
              </Copy>                               \n\
            </Structure>                            \n\
                                                    \n\
            <Parameter>                             \n\
              <STRAIGHTSystem                       \n\
                Name  = 'straightSystem'            \n\
              />                                    \n\
            </Parameter>                            \n\
          </DefaultConfiguration>                   \n\
        </Configurations>                           \n\
      </Simulator>                                  \n\
    </Session>                                      \n\
    ";
    } // namespace STRAIGHT
} // namespace Onikiri

#endif

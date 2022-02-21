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

#include <pch.h>
#include "Sim/Foundation/Hook/Hook.h"
#include "Sim/Pipeline/Fetcher/Fetcher.h"
#include "Sim/Dumper/Dumper.h"
#include "Sim/InorderList/InorderList.h"
#include "Sim/System/ForwardEmulator.h"
#include "Sim/Op/Op.h"
#include "Emu/STRAIGHT64Linux/STRAIGHTSystem.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxEmulator.h"

using namespace std;

using namespace Onikiri;
using namespace STRAIGHT64Linux;

STRAIGHTSystem::STRAIGHTSystem() :
    m_emuRP(0),
    m_emuSP(0),
    m_core(nullptr),
    m_emulator(nullptr)
{
}

STRAIGHTSystem::~STRAIGHTSystem()
{
}



void STRAIGHTSystem::Initialize(InitPhase phase)
{
    if (phase == INIT_PRE_CONNECTION)
    {
        LoadParam();
    }
    else if (phase == INIT_POST_CONNECTION)
    {
        CheckNodeInitialized("core", m_core);
        CheckNodeInitialized("emulator", m_emulator);

        m_spTable.Resize(*m_core->GetOpArray());
        m_rpTable.Resize(*m_core->GetOpArray());

        const u64 pid = 0; // TODO: get actual pid
        m_emuSP = m_emulator->GetInitialRegValue(pid, STRAIGHT64Info::StackPointerRegIndex);

        STRAIGHT64LinuxEmulator::s_getOpHook.Register(this, &STRAIGHTSystem::AfterEmulatorGetOp, 0, HookType::HOOK_AFTER);
        Fetcher::s_fetchHook.Register(this, &STRAIGHTSystem::OnFetch, 0, HookType::HOOK_AFTER);
        Fetcher::s_getOpHook.Register(this, &STRAIGHTSystem::AfterFetcherGetOp, 0, HookType::HOOK_AFTER);
        ForwardEmulator::s_getOpHook.Register(this, &STRAIGHTSystem::AfterForwardEmulatorGetOp, 0, HookType::HOOK_AFTER);
        InorderList::s_opFlushHook.Register(this, &STRAIGHTSystem::BeforeFlush, 0, HookType::HOOK_BEFORE);
    }

}

void STRAIGHTSystem::Finalize()
{
    ReleaseParam();
}

void STRAIGHTSystem::ChangeSimulationMode(SimulationMode mode)
{
    if (mode == SM_SIMULATION)
    {
        m_rp = m_emuRP; // m_emuRPはEmulationの時から使われていて正しい値を保持しているのでSimulation用にコピーする
        m_sp = m_emuSP; // m_emuSPも同様
    }
}

void STRAIGHTSystem::Rename(std::pair<OpInfo**, int> ops, u64 rp)
{
    for (int i = 0; i < ops.second; i++)
    {
        STRAIGHT64OpInfo* opInfo = cast_STRAIGHT64OpInfo(ops.first[i]);

        opInfo->SetDstReg(0, static_cast<int>(rp));
        for (int j = 0; j < opInfo->GetSrcNum(); j++)
        {
            auto distance = opInfo->GetSrcOperand(j);
            auto src = STRAIGHT64Info::CalcRP(rp, -distance);
            if (distance == STRAIGHT64Info::ZeroRegIndex) { continue; }
            opInfo->SetSrcReg(j, static_cast<int>(src));
        }
    }
}

void STRAIGHTSystem::AfterEmulatorGetOp(STRAIGHT64LinuxEmulator::GetOpHookParam* param)
{
    Rename(*param, m_emuRP);

    STRAIGHT64OpInfo* opInfo = cast_STRAIGHT64OpInfo(*(param->first));

    // Process RPINC
    if (opInfo->isRPINC())
    {
        m_emuRP = STRAIGHT64Info::CalcRP(m_emuRP, opInfo->GetImm(0) + 1);
    }
    else
    {
        m_emuRP = STRAIGHT64Info::CalcRP(m_emuRP, 1);
    }

    // Process SPADDi/SPLD/SPST
    if (opInfo->isSPADDi())
    {
        m_emuSP += opInfo->GetImm(0);
    }
    else if (opInfo->isSPLDSTorAUiSP())
    {
        if (opInfo->GetImmNum() == 3) {
            ASSERT(opInfo->isSPST());
            // ソースレジスタがZeroRegであるため、即値扱いのものが一個多くて後ろにずれる
            // Imm0が0、Imm1がOffset、Imm2がSPの値となる
            opInfo->SetImm(2, m_emuSP);
        } else {
            // Imm0が即値、Imm1がSPの値
            opInfo->SetImm(1, m_emuSP);
        }
    }
}

void STRAIGHTSystem::AfterFetcherGetOp(Fetcher::GetOpHookParam* param)
{
    Rename(*param, m_rp);
}

void STRAIGHTSystem::AfterForwardEmulatorGetOp(ForwardEmulator::GetOpHookParam* param)
{
    auto opInfos = param->opInfos;
    auto microOpIndex = param->context->microOpIndex;
    Rename(opInfos, m_emuRP);

    STRAIGHT64OpInfo* opInfo = cast_STRAIGHT64OpInfo(opInfos.first[microOpIndex]);

    // Skip Syscall Branch
    if (opInfo->isSyscallBranch())
    {
        return;
    }


    // Process RPINC
    if (opInfo->isRPINC())
    {
        m_emuRP = STRAIGHT64Info::CalcRP(m_emuRP, opInfo->GetImm(0) + 1);
    }
    else
    {
        m_emuRP = STRAIGHT64Info::CalcRP(m_emuRP, 1);
    }

    // Process SPADDi/SPLD/SPST
    if (opInfo->isSPADDi())
    {
        m_emuSP += opInfo->GetImm(0);
    }
    else if (opInfo->isSPLDSTorAUiSP())
    {
        if (opInfo->GetImmNum() == 3) {
            ASSERT(opInfo->isSPST());
            // ソースレジスタがZeroRegであるため、即値扱いのものが一個多くて後ろにずれる
            // Imm0が0、Imm1がOffset、Imm2がSPの値となる
            opInfo->SetImm(2, m_emuSP);
        } else {
            // Imm0が即値、Imm1がSPの値
            opInfo->SetImm(1, m_emuSP);
        }
    }

}

void STRAIGHTSystem::OnFetch(Fetcher::FetchHookParam* param)
{
    auto op = param->op;

    STRAIGHT64OpInfo* opInfo = cast_STRAIGHT64OpInfo(op->GetOpInfo());

    m_rpTable[op] = m_rp;
    m_spTable[op] = m_sp;

    // Skip Syscall Branch
    if (opInfo->isSyscallBranch())
    {
        return;
    }

    // Process RPINC
    if (opInfo->isRPINC())
    {
        m_rp = STRAIGHT64Info::CalcRP(m_rp, opInfo->GetImm(0) + 1);
    }
    else
    {
        m_rp = STRAIGHT64Info::CalcRP(m_rp, 1);
    }

    // Process SPADDi/SPLD/SPST
    if (opInfo->isSPADDi())
    {
        m_sp += opInfo->GetImm(0);
    }
    else if (opInfo->isSPLDSTorAUiSP())
    {
        if (opInfo->GetImmNum() == 3) {
            ASSERT(opInfo->isSPST());
            // ソースレジスタがZeroRegであるため、即値扱いのものが一個多くて後ろにずれる
            // Imm0が0、Imm1がOffset、Imm2がSPの値となる
            opInfo->SetImm(2, m_sp);
        } else {
            // Imm0が即値、Imm1がSPの値
            opInfo->SetImm(1, m_sp);
        }
    }
}

void STRAIGHTSystem::BeforeFlush(OpIterator op)
{
    m_rp = m_rpTable[op];
    m_sp = m_spTable[op];
}

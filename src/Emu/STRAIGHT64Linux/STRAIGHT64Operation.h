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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_OPERATION_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_OPERATION_H

#include "SysDeps/fenv.h"
#include "Utility/RuntimeError.h"
#include "Emu/Utility/GenericOperation.h"
#include "Emu/Utility/System/Syscall/SyscallConvIF.h"
#include "Emu/Utility/System/ProcessState.h"


namespace Onikiri {
namespace STRAIGHT64Linux {
namespace Operation {

using namespace EmulatorUtility;
using namespace EmulatorUtility::Operation;

typedef u64 STRAIGHT64RegisterType;

static const STRAIGHT64RegisterType REG_VALUE_TRUE = 1;
static const STRAIGHT64RegisterType REG_VALUE_FALSE = 0;

// Operands
template <int OperandIndex>
class STRAIGHT64DstOperand
{
public:
    typedef STRAIGHT64RegisterType type;
    static void SetOperand(OpEmulationState* opState, STRAIGHT64RegisterType value)
    {
        opState->SetDst(OperandIndex, value);
    }
};

template <int OperandIndex>
struct STRAIGHT64SrcOperand : public std::unary_function<OpEmulationState, STRAIGHT64RegisterType>
{
    typedef STRAIGHT64RegisterType type;
    STRAIGHT64RegisterType operator()(OpEmulationState* opState)
    {
        return (STRAIGHT64RegisterType)opState->GetSrc(OperandIndex);
    }
};

// Add Upper Immidiate SP
template<typename TSrc1, typename TSrc2>
struct STRAIGHT64Auisp : public std::unary_function<OpEmulationState, RegisterType>
{
    STRAIGHT64RegisterType operator()(OpEmulationState* opState) const
    {
        // current SP value that is told by STRAIGHTSystem
        STRAIGHT64RegisterType SP = TSrc2()(opState);
        return (TSrc1()(opState) << 12) + SP;
    }
};

//
// Load/Store
//
template<typename TSrc1, typename TSrc2>
struct STRAIGHT64Addr : public std::unary_function<EmulatorUtility::OpEmulationState, STRAIGHT64RegisterType>
{
    STRAIGHT64RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        return static_cast<u64>(TSrc1()(opState) + EmulatorUtility::cast_to_signed(TSrc2()(opState)));
    }
};

template <typename Type, typename TValue, typename TAddr>
struct STRAIGHT64Store : public std::unary_function<EmulatorUtility::OpEmulationState, STRAIGHT64RegisterType>
{
    STRAIGHT64RegisterType operator()(EmulatorUtility::OpEmulationState* opState) const
    {
        WriteMemory<Type>(opState, TAddr()(opState), static_cast<Type>(TValue()(opState)));
        return static_cast<u64>(static_cast<Type>(TValue()(opState)));
    }
};


void STRAIGHT64SyscallSetArg(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(0, SrcOperand<0>()(opState));
    syscallConv->SetArg(1, SrcOperand<1>()(opState));
    syscallConv->SetArg(2, SrcOperand<2>()(opState));
    DstOperand<0>::SetOperand(opState, SrcOperand<0>()(opState));
}

// invoke syscall, get result&error and branch if any
void STRAIGHT64SyscallCore(EmulatorUtility::OpEmulationState* opState)
{
    EmulatorUtility::SyscallConvIF* syscallConv = opState->GetProcessState()->GetSyscallConv();
    syscallConv->SetArg(3, SrcOperand<1>()(opState));
    syscallConv->SetArg(4, SrcOperand<2>()(opState));
    //syscallConv->SetArg(5, SrcOperand<2>()(opState));
    syscallConv->Execute(opState);

    u64 error = syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex);
    u64 val = syscallConv->GetResult(EmulatorUtility::SyscallConvIF::RetValueIndex);
    DstOperand<0>::SetOperand(opState, error ? (u64)-1 : val);
    //DstOperand<1>::SetOperand(opState, syscallConv->GetResult(EmulatorUtility::SyscallConvIF::ErrorFlagIndex) );
}

} // namespace Operation {
} // namespace STRAIGHT64Linux {
} // namespace Onikiri

#endif // #ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_OPERATION_H


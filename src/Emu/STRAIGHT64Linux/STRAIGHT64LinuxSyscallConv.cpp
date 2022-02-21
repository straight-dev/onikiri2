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

#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxSyscallConv.h"
#include "Emu/Utility/OpEmulationState.h"

using namespace Onikiri::EmulatorUtility;
using namespace Onikiri::STRAIGHT64Linux;

STRAIGHT64LinuxSyscallConv::STRAIGHT64LinuxSyscallConv(ProcessState* processState) : 
    RISCV64LinuxSyscallConv(processState)
{
}

STRAIGHT64LinuxSyscallConv::~STRAIGHT64LinuxSyscallConv()
{
}

void STRAIGHT64LinuxSyscallConv::Execute(OpEmulationState* opState)
{
    // Ad-hoc putchar systemcall
    if( GetArg( 0 ) == 'putc' ) {
        std::cout << (unsigned char)GetArg(1); // writes the character, cast to an unsigned char, to stdout.
        opState->SetTakenPC(opState->GetPC() + 4);
        opState->SetTaken(true);
        SetResult(true, (unsigned char)GetArg(1)); // returns the character written as an unsigned char cast to an int
        return;
    }

    RISCV64LinuxSyscallConv::Execute(opState);
}

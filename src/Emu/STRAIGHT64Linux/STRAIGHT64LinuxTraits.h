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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_TRAITS_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_TRAITS_H

#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxSyscallConv.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64LinuxLoader.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Info.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64Converter.h"
#include "Emu/STRAIGHT64Linux/STRAIGHT64OpInfo.h"

namespace Onikiri {
    namespace STRAIGHT64Linux {

        struct STRAIGHT64LinuxTraits {
            typedef STRAIGHT64Info ISAInfoType;
            typedef STRAIGHT64OpInfo OpInfoType;
            typedef STRAIGHT64Converter ConverterType;
            typedef STRAIGHT64LinuxLoader LoaderType;
            typedef STRAIGHT64LinuxSyscallConv SyscallConvType;

            static const bool IsBigEndian = false;
        };

    } // namespace STRAIGHT64Linux
} // namespace Onikiri

#endif

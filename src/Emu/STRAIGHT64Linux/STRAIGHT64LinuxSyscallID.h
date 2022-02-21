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


#ifndef EMU_STRAIGHT64LINUX_STRAIGHT64_SYSCALLID_H
#define EMU_STRAIGHT64LINUX_STRAIGHT64_SYSCALLID_H

namespace Onikiri {
    namespace STRAIGHT64Linux {

        // https://github.com/riscv/riscv-linux/blob/riscv-next/arch/riscv/include/uapi/asm/unistd.h
        // https://github.com/riscv/riscv-linux/blob/riscv-next/include/uapi/asm-generic/unistd.h
        namespace SyscallID {
            const int syscall_id_getcwd = 17;
            const int syscall_id_fcntl = 25;
            const int syscall_id_ioctl = 29;
            const int syscall_id_mkdirat = 34;
            const int syscall_id_unlinkat = 35;
            const int syscall_id_renameat = 38;
            const int syscall_id_ftruncate = 46;
            const int syscall_id_faccessat = 48;
            const int syscall_id_chdir = 49;
            const int syscall_id_openat = 56;
            const int syscall_id_close = 57;
            const int syscall_id_pipe2 = 59;
            const int syscall_id_getdents64 = 61;
            const int syscall_id_lseek = 62;
            const int syscall_id_read = 63;
            const int syscall_id_write = 64;
            const int syscall_id_writev = 66;
            const int syscall_id_readlinkat = 78;
            const int syscall_id_fstatat = 79;
            const int syscall_id_fstat = 80;
            const int syscall_id_exit = 93;
            const int syscall_id_exit_group = 94;
            const int syscall_id_set_tid_address = 96;
            const int syscall_id_clock_gettime = 113;
            const int syscall_id_sigaction = 134;
            const int syscall_id_times = 153;
            const int syscall_id_uname = 160;
            const int syscall_id_getrlimit = 163;
            const int syscall_id_setrlimit = 164;
            const int syscall_id_getrusage = 165;
            const int syscall_id_gettimeofday = 169;
            const int syscall_id_getpid = 172;
            const int syscall_id_getppid = 173;
            const int syscall_id_getuid = 174;
            const int syscall_id_geteuid = 175;
            const int syscall_id_getgid = 176;
            const int syscall_id_getegid = 177;
            const int syscall_id_sysinfo = 179;
            const int syscall_id_brk = 214;
            const int syscall_id_munmap = 215;
            const int syscall_id_mremap = 216;
            const int syscall_id_clone = 220;
            const int syscall_id_mmap = 222;
            const int syscall_id_madvise = 233;
            const int syscall_id_prlimit64 = 261;
            const int syscall_id_renameat2 = 276;
            const int syscall_id_open = 1024;
            const int syscall_id_unlink = 1026;
            const int syscall_id_stat = 1038;
        } // namespace SyscallID

        using namespace SyscallID;

    }   // namespace STRAIGHT64Linux
} // namespace Onikiri

#endif

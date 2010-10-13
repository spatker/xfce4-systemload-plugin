/*
 * Copyright (c) 2003 Riccardo Persichetti <riccardo.persichetti@tin.it>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "memswap.h"


#if defined(__linux__) || defined(__FreeBSD_kernel__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define MEMINFOBUFSIZE (2 * 1024)
static char MemInfoBuf[MEMINFOBUFSIZE];

static unsigned long MTotal = 0;
static unsigned long MFree = 0;
static unsigned long MBuffers = 0;
static unsigned long MCached = 0;
static unsigned long MUsed = 0;
static unsigned long STotal = 0;
static unsigned long SFree = 0;
static unsigned long SUsed = 0;

gint read_memswap(gulong *mem, gulong *swap, gulong *MT, gulong *MU, gulong *ST, gulong *SU)
{
    int fd;
    size_t n;
    int o_MTotal, o_MFree, o_MBuffers, o_MCached, o_STotal, o_SFree;
    char *b_MTotal, *b_MFree, *b_MBuffers, *b_MCached, *b_STotal, *b_SFree;

    if ((fd = open("/proc/meminfo", O_RDONLY)) < 0)
    {
        g_warning("Cannot open \'/proc/meminfo\'");
        return -1;
    }
    if ((n = read(fd, MemInfoBuf, MEMINFOBUFSIZE - 1)) == MEMINFOBUFSIZE - 1)
    {
        g_warning("Internal buffer too small to read \'/proc/mem\'");
        close(fd);
        return -1;
    }
    close(fd);

    MemInfoBuf[n] = '\0';

    b_MTotal = strstr(MemInfoBuf, "MemTotal");
    if (b_MTotal)
        o_MTotal = sscanf(b_MTotal + strlen("MemTotal"), ": %lu", &MTotal);

    b_MFree = strstr(MemInfoBuf, "MemFree");
    if (b_MFree)
        o_MFree = sscanf(b_MFree + strlen("MemFree"), ": %lu", &MFree);

    b_MBuffers = strstr(MemInfoBuf, "Buffers");
    if (b_MBuffers)
        o_MBuffers = sscanf(b_MBuffers + strlen("Buffers"), ": %lu", &MBuffers);

    b_MCached = strstr(MemInfoBuf, "Cached");
    if (b_MCached)
        o_MCached = sscanf(b_MCached + strlen("Cached"), ": %lu", &MCached);

    b_STotal = strstr(MemInfoBuf, "SwapTotal");
    if (b_STotal)
        o_STotal = sscanf(b_STotal + strlen("SwapTotal"), ": %lu", &STotal);

    b_SFree = strstr(MemInfoBuf, "SwapFree");
    if (b_SFree)
        o_SFree = sscanf(b_SFree + strlen("SwapFree"), ": %lu", &SFree);

    MFree += MCached + MBuffers;
    MUsed = MTotal - MFree;
    SUsed = STotal - SFree;
    *mem = MUsed * 100 / MTotal;

    if(STotal)
        *swap = SUsed * 100 / STotal;
    else
        *swap = 0;

    *MT = MTotal;
    *MU = MUsed;
    *ST = STotal;
    *SU = SUsed;

    return 0;
}

#elif defined(__FreeBSD__)
/*
 * This is inspired by /usr/src/usr.bin/top/machine.c 
 *
 * Adapted by Thorsten Greiner <thorsten.greiner@web.de>
 *
 * Original authors: Christos Zoulas <christos@ee.cornell.edu>
 *                   Steven Wallace  <swallace@freebsd.org>
 *                   Wolfram Schneider <wosch@FreeBSD.org>
 *                   Thomas Moestl <tmoestl@gmx.net>
 */

/*
 * FreeBSD defines MAX and MIN in sys/param.h, so undef the glib macros first
 */
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <vm/vm_param.h>
#include <sys/vmmeter.h>
#include <unistd.h>
#include <kvm.h>

#define GETSYSCTL(name, var) getsysctl(name, &(var), sizeof(var))

static int getsysctl (char *name, void *ptr, size_t len)
{
    size_t nlen = len;
    if (sysctlbyname(name, ptr, &nlen, NULL, 0) == -1) {
        return -1;
    }
    if (nlen != len) {
        return -1;
    }
    return 0;
}

static kvm_t *kd = NULL;

static int swapmode(int *retavail, int *retfree)
{
    int n;
    int pagesize = getpagesize();
    struct kvm_swap swapary[1];
    static int kd_init = TRUE;

    if(kd_init) {
        kd_init = FALSE;
        if ((kd = kvm_open("/dev/null", "/dev/null", "/dev/null", 
                           O_RDONLY, "kvm_open")) == NULL) {
            g_warning("Cannot read kvm.");
            return -1;
        }
    }
    if(kd == NULL) {
        return -1;
    }

    *retavail = 0;
    *retfree = 0;

#define CONVERT(v)	((quad_t)(v) * pagesize / 1024)

    n = kvm_getswapinfo(kd, swapary, 1, 0);
    if (n < 0 || swapary[0].ksw_total == 0)
            return(0);

    *retavail = CONVERT(swapary[0].ksw_total);
    *retfree = CONVERT(swapary[0].ksw_total - swapary[0].ksw_used);

    n = (int)((double)swapary[0].ksw_used * 100.0 /
        (double)swapary[0].ksw_total);
    return(n);
}

gint read_memswap(gulong *mem, gulong *swap, gulong *MT, gulong *MU, gulong *ST, gulong *SU)
{
    int total_pages;
    int free_pages;
    int inactive_pages;
    int pagesize = getpagesize();
    int swap_avail;
    int swap_free;

    if(GETSYSCTL("vm.stats.vm.v_page_count", total_pages)) {
        g_warning("Cannot read sysctl \"vm.stats.vm.v_page_count\"");
        return -1;
    }
    if(GETSYSCTL("vm.stats.vm.v_free_count", free_pages)) {
        g_warning("Cannot read sysctl \"vm.stats.vm.v_free_count\"");
        return -1;
    }
    if(GETSYSCTL("vm.stats.vm.v_inactive_count", inactive_pages)) {
        g_warning("Cannot read sysctl \"vm.stats.vm.v_inactive_count\"");
        return -1;
    }

    *MT = (total_pages*pagesize) >> 10;
    *MU = ((total_pages-free_pages-inactive_pages) * pagesize) >> 10;
    *mem = *MU * 100 / *MT;

    if((*swap = swapmode(&swap_avail, &swap_free)) >= 0) {
        *ST = swap_avail;
        *SU = (swap_avail - swap_free);
    }
    else {
        *swap = 0;
        *ST = 0;
        *SU = 0;
    }

    return 0;
}

#elif defined(__NetBSD__)
/*
 * NetBSD defines MAX and MIN in sys/param.h, so undef the glib macros first
 */
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/vmmeter.h>
#include <unistd.h>
/* NetBSD-current post 1.6U uses swapctl */
#if __NetBSD_Version__ > 106210000
#include <sys/swap.h>
#elif __NetBSD_Version__ >= 105010000
/* Everything post 1.5.x uses uvm/uvm_* includes */
#include <uvm/uvm_param.h>
#else
#include <vm/vm_param.h>
#endif

static size_t MTotal = 0;
static size_t MFree = 0;
static size_t MUsed = 0;
static size_t STotal = 0;
static size_t SFree = 0;
static size_t SUsed = 0;

gint read_memswap(gulong *mem, gulong *swap, gulong *MT, gulong *MU, gulong *ST, gulong *SU)
{
    int pagesize;
    size_t len;

#define ARRLEN(X) (sizeof(X)/sizeof(X[0]))
    {
        static int mib[2];
        /* 64-bit datatype */
        if(sizeof(size_t) == 8) {
            mib[0] = CTL_HW;
            mib[1] = HW_PHYSMEM64;
        }
        /* assume 32-bit datatype */
        else {
            mib[0] = CTL_HW;
            mib[1] = HW_PHYSMEM;
        }
        len = sizeof(MTotal);
        sysctl(mib, ARRLEN(mib), &MTotal, &len, NULL, 0);
        MTotal >>= 10;
    }

    {
      static int mib[] = {CTL_HW, HW_PAGESIZE};
      len = sizeof(pagesize);
      sysctl(mib, ARRLEN(mib), &pagesize, &len, NULL, 0);
    }

#if __NetBSD_Version__ > 106210000
    {
      struct swapent* swap;
      int nswap, n;
      STotal = SUsed = SFree = 0;
      if ((nswap = swapctl(SWAP_NSWAP, NULL, 0)) > 0) {
        swap = (struct swapent*)malloc(nswap * sizeof(*swap));
        if (swapctl(SWAP_STATS, (void*)swap, nswap) == nswap) {
          for (n = 0; n < nswap; n++) {
            STotal += swap[n].se_nblks;
            SUsed  += swap[n].se_inuse;
          }

          STotal = dbtob(STotal >> 10);
          SUsed  = dbtob(SUsed >> 10);
          SFree  = STotal - SUsed;
        }
        free(swap);
      }
    }
#else
    {
        struct uvmexp x;
        static int mib[] = { CTL_VM, VM_UVMEXP };
        len = sizeof(x);
        STotal = SUsed = SFree = -1;
        pagesize = 1;
        if (-1 < sysctl(mib, ARRLEN(mib), &x, &len, NULL, 0)) {
            //pagesize = x.pagesize;
            STotal = (pagesize*x.swpages) >> 10;
            SUsed = (pagesize*x.swpginuse) >> 10;
            SFree = STotal - SUsed;
        }
    }
#endif

    {
        static int mib[]={ CTL_VM, VM_METER };
        struct vmtotal x;

        len = sizeof(x);
        MFree = MUsed = -1;
        if (sysctl(mib, ARRLEN(mib), &x, &len, NULL, 0) > -1) {
            MFree = (x.t_free * pagesize) >> 10;
            MUsed = (x.t_rm * pagesize) >> 10;
        }
    }

    *mem = MUsed * 100 / MTotal;
    if(STotal)
        *swap = SUsed * 100 / STotal;
    else
        *swap = 0;

    *MT = MTotal;
    *MU = MUsed;
    *ST = STotal;
    *SU = SUsed;

    return 0;
}

#elif defined(__OpenBSD__)
/*
 * OpenBSD defines MAX and MIN in sys/param.h, so undef the glib macros first
 */
#ifdef MAX
#undef MAX
#endif
#ifdef MIN
#undef MIN
#endif

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/vmmeter.h>
#include <unistd.h>
#include <uvm/uvm_param.h>

static size_t MTotal = 0;
static size_t MFree = 0;
static size_t MUsed = 0;
static size_t STotal = 0;
static size_t SFree = 0;
static size_t SUsed = 0;

gint read_memswap(gulong *mem, gulong *swap, gulong *MT, gulong *MU, gulong *ST, gulong *SU)
{
    long pagesize;
    size_t len;

#define ARRLEN(X) (sizeof(X)/sizeof(X[0]))
    {
        static int mib[2];
        /* 64-bit datatype */
        if(sizeof(size_t) == 8) {
            mib[0] = CTL_HW;
            mib[1] = HW_PHYSMEM64;
        }
        /* assume 32-bit datatype */
        else {
            mib[0] = CTL_HW;
            mib[1] = HW_PHYSMEM;
        }
        len = sizeof(MTotal);
        sysctl(mib, ARRLEN(mib), &MTotal, &len, NULL, 0);
        MTotal >>= 10;
    }

    {
        struct uvmexp x;
        static int mib[] = { CTL_VM, VM_UVMEXP };
        len = sizeof(x);
        STotal = SUsed = SFree = -1;
        pagesize = 1;
        if (-1 < sysctl(mib, ARRLEN(mib), &x, &len, NULL, 0)) {
            pagesize = x.pagesize;
            STotal = (pagesize*x.swpages) >> 10;
            SUsed = (pagesize*x.swpginuse) >> 10;
            SFree = STotal - SUsed;
        }
    }

    {
        static int mib[]={ CTL_VM, VM_METER };
        struct vmtotal x;

        len = sizeof(x);
        MFree = MUsed = -1;
        if (sysctl(mib, ARRLEN(mib), &x, &len, NULL, 0) > -1) {
            MFree = (x.t_free * pagesize) >> 10;
            MUsed = (x.t_rm * pagesize) >> 10;
        }
    }

    *mem = MUsed * 100 / MTotal;
    if(STotal)
        *swap = SUsed * 100 / STotal;
    else
        *swap = 0;

    *MT = MTotal;
    *MU = MUsed;
    *ST = STotal;
    *SU = SUsed;

    return 0;
}

#else
#error "Your platform is not yet support"
#endif


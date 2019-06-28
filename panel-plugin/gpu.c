/*
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

#include "gpu.h"


#if defined(__linux__)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

gint read_gpu(gulong *load, gulong *mem, gulong *mem_total, gulong *mem_used)
{
    FILE *fp;
    fp = popen("nvidia-smi --query-gpu=utilization.gpu,utilization.memory,memory.total,memory.used --format=csv,noheader,nounits", "r");
    if (fp == NULL) {
        g_warning("nvidia-smi not found!");
        return 0;
    }

    unsigned long long int nvidia_load, nvidia_mem, nvidia_mem_total, nvidia_mem_used;
    int nb_read;
    nb_read = fscanf (fp, "%llu, %llu, %llu, %llu",
	    &nvidia_load, &nvidia_mem, &nvidia_mem_total, &nvidia_mem_used);
    pclose(fp);

    if(nb_read != 4)
    {
        g_warning("Failed to parse nvidia-smi output");
        return 0;
    }

    *load = nvidia_load;
    *mem = nvidia_mem;
    *mem_total = nvidia_mem_total;
    *mem_used = nvidia_mem_used;

    return 1;
}

#else
#error "Your platform is not yet supported"
#endif

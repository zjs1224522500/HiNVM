/*
 * Copyright 2015-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * pmemobj_alloc.c - An example to show how to use
 * pmemobj_alloc()
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libpmemobj.h>

#define die(...) do {fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define POOL "/mnt/pmem/paintball"
#define LAYOUT "paintball"

typedef uint32_t color;

// 在分配内存时被调用，这个函数接受一个池和对象指针
// 计算彩弹颜色的随机十六进制值，并持续地将其写入池。当写入完成时，程序退出
static int paintball_init(PMEMobjpool *pop, void *ptr, void *arg)
{
    *(color *)ptr = time(0) & 0xffffff;
    pmemobj_persist(pop, ptr, sizeof(color));
    return 0;
}

int main()
{   
    // 打开或创建一个池并获取一个指向池中的根对象的指针。
    PMEMobjpool *pool = pmemobj_open(POOL, LAYOUT);
    if (!pool) {
        pool = pmemobj_create(POOL, LAYOUT, PMEMOBJ_MIN_POOL, 0666);
        if (!pool)
            die("Couldn't open pool: %m\n");
        
    }
    // 获取根对象
    PMEMoid root = pmemobj_root(pool, sizeof(PMEMoid) * 6);
    if (OID_IS_NULL(root))
        die("Couldn't access root object.\n");

    // 获取指向根节点的一定偏移量的指针
    PMEMoid *chamber = (PMEMoid *)pmemobj_direct(root) + (getpid() % 6);
    if (OID_IS_NULL(*chamber)) {
        // 如果指针不是一个有效的对象
        printf("Reloading.\n");
        // 分配内存，调用 init
        if (pmemobj_alloc(pool, chamber, sizeof(color), 0, paintball_init, 0))
            die("Failed to alloc: %m\n");
    } else {
        // 如果是一个有效的对象，读取 color value
        printf("Shooting %06x colored bullet.\n", *(color *)pmemobj_direct(*chamber));
        // 释放内存
        pmemobj_free(chamber);
    }

    pmemobj_close(pool);
    return 0;
}
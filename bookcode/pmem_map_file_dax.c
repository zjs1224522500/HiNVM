/*
 * Copyright (c) 2020, Intel Corporation
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
 *     * Neither the name of Intel Corporation nor the names of its
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

// 根据不同的平台使用不同的头文件
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <string.h>

// 使用 libpmem
#include <libpmem.h>

/* Using 4K of pmem for this example */
#define PMEM_LEN 4096

int
main(int argc, char *argv[])
{
	char *pmemaddr;
	size_t mapped_len;
	int is_pmem;

    // 判断参数个数
	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", 
			argv[0]);
		exit(1);
	}

    // 调用 pmdk 的 API 创建一个 pmem file 映射，映射到 pmemaddr
    // pmem_map_file 打开一个文件并映射到地址空间
    // 因为文件驻留在 PM，操作系统编程了 CPU 中的 MMU 硬件来映射持久性内存区域到应用程序的虚拟地址空间
    // pmemaddr 指向了 region 的起始位置
    // pmem_map_file 也可以映射基于磁盘的文件，或者直接映射持久内存、
    // is_pmem 为 true 表明文件位于持久内存，false 表明通过主存映射
	/* Create a pmem file and memory map it. */
	if ((pmemaddr = pmem_map_file(argv[1], PMEM_LEN, 
			PMEM_FILE_CREATE, 0666, &mapped_len, 
			&is_pmem)) == NULL) {
		perror("pmem_map_file");
		exit(1);
	}

    // 拷贝数据到 PM
	/* Store a string to the persistent memory. */
	char s[] = "This is new data written to the file";
	strcpy(pmemaddr, s);

    // 判断是否为 pmem
	/* Flush our string to persistence. */
	if (is_pmem)
        // 是的话，执行 pmdk 的 API pmem_persist
        // 使用用户空间的机器指令来确保字符串从 CPU Cache 中刷出到了持久化域
		pmem_persist(pmemaddr, sizeof(s));
	else
        // 不是的话，执行 pmem_msync，本质则是 mmap
        // 但是这里可以使用比 page 更小的大小，这里使用的字符串大小
		pmem_msync(pmemaddr, sizeof(s));

    // 使用完后删除映射关系
	/* Delete the mappings. */
	pmem_unmap(pmemaddr, mapped_len);

	printf("Done.\n");
	exit(0);
}
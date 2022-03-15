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
 * reserve_publish.c – An example using the
 * reserve/publish libpmemobj API
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libpmemobj.h>

#define die(...) do {fprintf(stderr, __VA_ARGS__); exit(1);} while(0)
#define POOL "/mnt/pmem/balance"

static PMEMobjpool *pool;

// 定义了一个账户数据结构
struct account {
    PMEMoid name;
    uint64_t balance;
};

TOID_DECLARE(struct account, 0);

/*
 * Even though we return the oid in a volatile register, there's no
 * persistent leak as all "struct account" (type 1) allocations are
 * reachable via POBJ_FOREACH_TYPE().
 */
static PMEMoid new_account(const char *name, int deposit)
{
    int len = strlen(name) + 1;

    
    struct pobj_action act[2];
    // 为字符串分配内存，即为 name 分配内存
    PMEMoid str = pmemobj_reserve(pool, act + 0, len, 0);
    if (OID_IS_NULL(str))
        die("Can't allocate string: %m\n");
    /*
     * memcpy below must flush, but doesn't need to drain -- even just a
     * single drain after all flushes is enough.
     */
    // 执行数据拷贝，把参数拷贝到持久内存中的 str 中，但是不 drain
    pmemobj_memcpy(pool, pmemobj_direct(str), name, len, PMEMOBJ_F_MEM_NODRAIN);
    
    // 声明定义 acc，将使用 oid 标识
    TOID(struct account) acc;
    // 保留内存，得到 acc_oid
    PMEMoid acc_oid = pmemobj_reserve(pool, act + 1, sizeof(struct account), 1);
    // 把 oid 分配给 acc 对象
    TOID_ASSIGN(acc, acc_oid);
    if (TOID_IS_NULL(acc))
        die("Can't allocate account: %m\n");
    
    // 更新 name 和 account
    // 写 acc->name 和 acc->balance
    D_RW(acc)->name = str;
    D_RW(acc)->balance = deposit;

    // 持久化更改
    pmemobj_persist(pool, D_RW(acc), sizeof(struct account));

    // 发布更新
    pmemobj_publish(pool, act, 2);
    return acc_oid;
}

int main()
{
    if (!(pool = pmemobj_create(POOL, "", PMEMOBJ_MIN_POOL, 0600)))
        die("Can't create pool “%s”: %m\n", POOL);

    // 声明账户 A 和 B
    TOID(struct account) account_a, account_b;
    // 创建帐号并分配给 A 和 B
    TOID_ASSIGN(account_a, new_account("Julius Caesar", 100));
    TOID_ASSIGN(account_b, new_account("Mark Anthony", 50));

    int price = 42;
    struct pobj_action act[2];
    // 账户 a 减去价格
    pmemobj_set_value(pool, &act[0], &D_RW(account_a)->balance, D_RW(account_a)->balance - price);
    // 账户 b 加上价格
    pmemobj_set_value(pool, &act[1], &D_RW(account_b)->balance, D_RW(account_b)->balance + price);
    // 发布持久化
    pmemobj_publish(pool, act, 2);

    pmemobj_close(pool);
    return 0;
}
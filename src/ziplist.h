/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZIPLIST_H
#define _ZIPLIST_H

#define ZIPLIST_HEAD 0
#define ZIPLIST_TAIL 1

/*
    ziplist，是一个压缩的双链表，实现了针对 CPU cache 的优化。ziplist 实际上一个字符串，通过一系列的算法来实现压缩双链表

    在 redis 中，list 有两种存储方式：双链表（LinkedList）和压缩双链表（ziplist）。双链表即普通数据结构中遇到的，
    在 adlist.h 和 adlist.c 中实现。压缩双链表以连续的内存空间来表示双链表，压缩双链表节省前驱和后驱指针的空间（8B），
    这在小的 list 上，压缩效率是非常明显的；压缩双链表在 ziplist.h 和 ziplist.c 中实现

    在压缩双链表中，节省了前驱和后驱指针的空间，共 8个字节，这让数据在内存中更为紧凑。只要清晰的描述每个数据项的边界，
    就可以轻易得到后驱数据项的位置；只要描述前驱数据项的大小，就可以定位前驱数据项的位置，redis 就是这么做的

    ziplist 的格式可以表示为：
    <zlbytes><zltail><zllen><entry>...<entry><zlend>
        zlbytes 是 ziplist 占用的空间；
        zltail 是最后一个数据项的偏移位置，这方便逆向遍历链表，也是双链表的特性；
        zllen 是数据项 entry 的个数；
        zlend 就是 255，占 1B.详细展开 entry 的结构

    entry 的格式即为典型的 type-lenght-value，即 TLV，表述如下：
        |<prelen><<encoding+lensize><len>><data>|
        |---1----------------2--------------3---|
        域 1）是前驱数据项的大小。因为不用描述前驱的数据类型，描述较为简单。
        域 2） 是此数据项的的类型和数据大小。为了节省空间，redis 预设定了多种长度的字符串和整数
            3种长度的字符串
            #define ZIP_STR_06B (0 << 6)
            #define ZIP_STR_14B (1 << 6)
            #define ZIP_STR_32B (2 << 6)

            5种长度的整数
            #define ZIP_INT_16B (0xc0 | 0<<4)
            #define ZIP_INT_32B (0xc0 | 1<<4)
            #define ZIP_INT_64B (0xc0 | 2<<4)
            #define ZIP_INT_24B (0xc0 | 3<<4)
            #define ZIP_INT_8B 0xfe
        域 3）为真正的数据。

    注意，ziplist 每次插入新的数据都要 realloc  ？（自己确定）

    实际上，HSET 底层所使用的数据结构正是上面所说的 ziplist，而不是平时所说的 hashtable  ？（自己确定）

    那为什么要使用 ziplist，反对的理由是查找来说，（ziplist O(N)）VS（hashtable O(1)）？redis 可是为内存节省想破了头。
        首先 ziplist 比 hashtable 更节省内存，再者，redis 考虑到如果数据紧凑的 ziplist 能够放入 CPU 缓存（hashtable 很难，因为它是非线性的），
        那么查找算法甚至会比 hashtable 要快！。ziplist 由此有性能和内存空间的有事
*/

unsigned char *ziplistNew(void);
unsigned char *ziplistMerge(unsigned char **first, unsigned char **second);
unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where);
unsigned char *ziplistIndex(unsigned char *zl, int index);
unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);
unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);
unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval);
unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);
unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p);
unsigned char *ziplistDeleteRange(unsigned char *zl, int index, unsigned int num);
unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen);
unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip);
unsigned int ziplistLen(unsigned char *zl);
size_t ziplistBlobLen(unsigned char *zl);

#ifdef REDIS_TEST
int ziplistTest(int argc, char *argv[]);
#endif

#endif /* _ZIPLIST_H */

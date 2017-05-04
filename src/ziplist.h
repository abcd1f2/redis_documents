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
    ziplist����һ��ѹ����˫����ʵ������� CPU cache ���Ż���ziplist ʵ����һ���ַ�����ͨ��һϵ�е��㷨��ʵ��ѹ��˫����

    �� redis �У�list �����ִ洢��ʽ��˫����LinkedList����ѹ��˫����ziplist����˫������ͨ���ݽṹ�������ģ�
    �� adlist.h �� adlist.c ��ʵ�֡�ѹ��˫�������������ڴ�ռ�����ʾ˫����ѹ��˫�����ʡǰ���ͺ���ָ��Ŀռ䣨8B����
    ����С�� list �ϣ�ѹ��Ч���Ƿǳ����Եģ�ѹ��˫������ ziplist.h �� ziplist.c ��ʵ��

    ��ѹ��˫�����У���ʡ��ǰ���ͺ���ָ��Ŀռ䣬�� 8���ֽڣ������������ڴ��и�Ϊ���ա�ֻҪ����������ÿ��������ı߽磬
    �Ϳ������׵õ������������λ�ã�ֻҪ����ǰ��������Ĵ�С���Ϳ��Զ�λǰ���������λ�ã�redis ������ô����

    ziplist �ĸ�ʽ���Ա�ʾΪ��
    <zlbytes><zltail><zllen><entry>...<entry><zlend>
        zlbytes �� ziplist ռ�õĿռ䣻
        zltail �����һ���������ƫ��λ�ã��ⷽ�������������Ҳ��˫��������ԣ�
        zllen �������� entry �ĸ�����
        zlend ���� 255��ռ 1B.��ϸչ�� entry �Ľṹ

    entry �ĸ�ʽ��Ϊ���͵� type-lenght-value���� TLV���������£�
        |<prelen><<encoding+lensize><len>><data>|
        |---1----------------2--------------3---|
        �� 1����ǰ��������Ĵ�С����Ϊ��������ǰ�����������ͣ�������Ϊ�򵥡�
        �� 2�� �Ǵ�������ĵ����ͺ����ݴ�С��Ϊ�˽�ʡ�ռ䣬redis Ԥ�趨�˶��ֳ��ȵ��ַ���������
            3�ֳ��ȵ��ַ���
            #define ZIP_STR_06B (0 << 6)
            #define ZIP_STR_14B (1 << 6)
            #define ZIP_STR_32B (2 << 6)

            5�ֳ��ȵ�����
            #define ZIP_INT_16B (0xc0 | 0<<4)
            #define ZIP_INT_32B (0xc0 | 1<<4)
            #define ZIP_INT_64B (0xc0 | 2<<4)
            #define ZIP_INT_24B (0xc0 | 3<<4)
            #define ZIP_INT_8B 0xfe
        �� 3��Ϊ���������ݡ�

    ע�⣬ziplist ÿ�β����µ����ݶ�Ҫ realloc  �����Լ�ȷ����

    ʵ���ϣ�HSET �ײ���ʹ�õ����ݽṹ����������˵�� ziplist��������ƽʱ��˵�� hashtable  �����Լ�ȷ����

    ��ΪʲôҪʹ�� ziplist�����Ե������ǲ�����˵����ziplist O(N)��VS��hashtable O(1)����redis ����Ϊ�ڴ��ʡ������ͷ��
        ���� ziplist �� hashtable ����ʡ�ڴ棬���ߣ�redis ���ǵ�������ݽ��յ� ziplist �ܹ����� CPU ���棨hashtable ���ѣ���Ϊ���Ƿ����Եģ���
        ��ô�����㷨������� hashtable Ҫ�죡��ziplist �ɴ������ܺ��ڴ�ռ������
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

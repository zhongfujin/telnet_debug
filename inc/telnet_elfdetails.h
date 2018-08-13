#ifndef __TELNET_ELFDETAILS__H
#define __TELNET_ELFDETAILS__H
#include <stdint.h>
#include <elf.h>
#include <features.h>
#include "telnet_com.h"

#if __WORDSIZE == 64
    typedef Elf64_Ehdr Elf_Ehdr;
    typedef Elf64_Phdr Elf_Phdr;
    typedef Elf64_Shdr Elf_Shdr;
    typedef Elf64_Sym Elf_Sym;
#else
    typedef Elf32_Ehdr Elf_Ehdr;
    typedef Elf32_Phdr Elf_Phdr;
    typedef Elf32_Shdr Elf_Shdr;
    typedef Elf32_Sym Elf_Sym;
#endif


enum 
{
    SYMBOL_TYPE,
    UNKNOWN
};

/**
 *\brief elf_interp结构体
 */
struct elf_interp 
{
    char *name;
    size_t length;
    uintptr_t ph_addr;
};

/**
 *\brief 符号信息结构体
 */
struct elf_symbol 
{
    char *name;
    uintptr_t address;
    unsigned int type;
    size_t size; 
};

/**
 *\brief ELF文件的类型
 */
enum elf_bit 
{
    EXE_IS_NEITHER,
    EXE_IS_32BIT,
    EXE_IS_64BIT
};

/**
 *\brief ELF头信息结构体
 */
struct elf_internals 
{
    unsigned int fd;
    off_t proghdr_offset;
    void *proghdrs;
    size_t proghdr_num;
    size_t proghdr_size;
    off_t sechdr_offset;
    void *sechdrs; 
    size_t sechdr_num;
    size_t sechdr_size; 
    size_t secnametbl_idx;
    char *strsectbl;
    size_t strsectbl_size;
   
    struct elf_symbol *symbols;
    size_t symbols_num;
    struct elf_interp interp;
};

/**
 *\brief符号类型
 */
enum 
{
    SYMBOL_IS_UNKNOWN,
    SYMBOL_IS_FUNCTION,
    SYMBOL_IS_FILENAME,
    SYMBOL_IS_SECTION,
    SYMBOL_IS_OBJECT
};
unsigned int exe_get_type(unsigned int info, unsigned int group);
unsigned int exe_open_filename(const char *filename);
unsigned int exe_elf_identify(char *e_ident, size_t size);
unsigned int exe_load_symbol_table(struct elf_internals *ei, Elf_Shdr *symh, Elf_Shdr *strh);
unsigned int exe_load_section_headers(struct elf_internals *ei);
unsigned int exe_load_program_headers(struct elf_internals *ei);
unsigned int exe_load_headers(struct elf_internals *ei);
struct elf_symbol *exe_load_symbols(const char *filename,size_t *symbols_num,struct elf_interp *interp);

#endif

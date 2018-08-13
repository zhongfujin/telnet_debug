#include "telnet_elfdetails.h"

/*获取符号类型*/
unsigned int exe_get_type(unsigned int info, unsigned int group)
{
    if (group == SYMBOL_TYPE) 
    {
        unsigned int value = ELF64_ST_TYPE(info);
        if (value == STT_FUNC)
            return SYMBOL_IS_FUNCTION;
        else if (value == STT_FILE)
            return SYMBOL_IS_FILENAME;
        else if (value == STT_SECTION)
            return SYMBOL_IS_SECTION; 
        else if (value == STT_OBJECT)
            return SYMBOL_IS_OBJECT;
        else
            return SYMBOL_IS_UNKNOWN;
    }
    return -1;
}


/*打开文件*/
unsigned int exe_open_filename(const char *filename)
{
    unsigned int fd = -1;
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        TEL_ERROR("open failed!\n");
        return -1;
    }
    return fd;
}

/*解析文件信息*/
unsigned int exe_elf_identify(char *elf_ident, size_t size)
{
    unsigned int is64 = 0;

    /*解析elf文件的e_ident*/
    if (elf_ident != NULL && size > 0) 
    {
        /*获取魔数*/
        if ((elf_ident[EI_MAG0] == ELFMAG0) && (elf_ident[EI_MAG1] == ELFMAG1) && \
            (elf_ident[EI_MAG2] == ELFMAG2) && (elf_ident[EI_MAG3] == ELFMAG3)) 
        {
            unsigned int is64 = EXE_IS_NEITHER;

            /*判断32位还是64位*/
            switch (elf_ident[EI_CLASS]) 
            {
            case ELFCLASS32:
                is64 = EXE_IS_32BIT;
            
                break;
            case ELFCLASS64:
                is64 = EXE_IS_64BIT;
            
                break;
            case ELFCLASSNONE:
            default:
                is64 = EXE_IS_NEITHER;
                
                break;
            }
        }
        if (is64 != EXE_IS_NEITHER) 
        {
            unsigned int isbigendian = -1;
            unsigned int iscurrent = 0;
            unsigned int islinux = 0;

            /*判断大小端*/
            switch (elf_ident[EI_DATA]) 
            {
            case ELFDATA2LSB:
                isbigendian = 0;
               
                break;
            case ELFDATA2MSB:
                isbigendian = 1;
               
                break;
            case ELFDATANONE:
            default:
                isbigendian = -1;
               
                break;
            }
            if (elf_ident[EI_VERSION] == EV_CURRENT) 
            {
                iscurrent = 1;
            }
           
            if (elf_ident[EI_OSABI] == ELFOSABI_LINUX || elf_ident[EI_OSABI] == ELFOSABI_SYSV) 
            {
                islinux = 1;
            }
            if (islinux && isbigendian == 0 && iscurrent) 
            {
                return is64;
            }
        }
    }
    return EXE_IS_NEITHER;
}

/*解析符号表*/
unsigned int exe_load_symbol_table(struct elf_internals *ei, Elf_Shdr *symh, Elf_Shdr *strh)
{
    char *strsymtbl = NULL;
    size_t strsymtbl_size = 0;
    while (ei && symh && strh) 
    {
      
        /*读取字符串表*/
        if (lseek(ei->fd, strh->sh_offset, SEEK_SET) < 0) 
        {
            TEL_ERROR("lseek failed!\n");
            break;
        }
        strsymtbl_size = strh->sh_size + 0;
        strsymtbl = malloc(strh->sh_size);
        if (strsymtbl == NULL) 
        {
            TEL_ERROR("malloc failed!\n");
            break;
        }
        if (read(ei->fd, strsymtbl, strh->sh_size) < 0) 
        {
            TEL_ERROR("read failed!\n");
            break;
        }
        
        /*get sym_num*/
        if (symh->sh_entsize > 0 && symh->sh_size > 0) 
        {
            size_t idx;
            size_t sym_num = symh->sh_size / symh->sh_entsize;

            /*为符号表开辟内存空间*/
            Elf_Sym *syms = malloc(symh->sh_size);
            if (syms == NULL) 
            {
                TEL_ERROR("malloc failed!\n");
                break;
            }
            
            /*get symh start addr*/
            if (lseek(ei->fd, symh->sh_offset, SEEK_SET) < 0) 
            {
                TEL_ERROR("lseek failed!\n");
                free(syms);
                break;
            }
            
            /*get sym table*/
            if (read(ei->fd, syms, symh->sh_size) < 0) 
            {
                TEL_ERROR("read failed!\n");
                free(syms);
                break;
            }
            
            /* there might already exist symbols from another section.
             * hence using realloc() takes care of that.
             * */
             /*start  symbols_num==0*/
             
            ei->symbols = realloc(ei->symbols,(sym_num + ei->symbols_num) * sizeof(*ei->symbols));
            if (!ei->symbols) 
            {
                TEL_ERROR("relloc failed!\n");
                break;
            }
         
            memset(ei->symbols, 0, (sym_num + ei->symbols_num) * sizeof(*ei->symbols));
            
            /* index 0 is always NULL */
            for (idx = 1; idx < sym_num; ++idx) 
            {
                const char *name = syms[idx].st_name > 0 ? &strsymtbl[syms[idx].st_name] : "";
                if (name != NULL) 
                {
                    char *name2;
                    unsigned int symtype = exe_get_type(syms[idx].st_info,SYMBOL_TYPE);
                    
                    name2 = strdup(name);
                    if (name2 == NULL) 
                    {
                        continue;
                    }

                    /*符号名称*/
                    ei->symbols[ei->symbols_num].name = name2;

                    /*符号地址*/
                    ei->symbols[ei->symbols_num].address = (uintptr_t)syms[idx].st_value;

                    /*符号大小*/
                    ei->symbols[ei->symbols_num].size = (size_t)syms[idx].st_size;

                    /*符号类型*/
                    ei->symbols[ei->symbols_num].type = symtype;
                    
                    ei->symbols_num++;
                    
                }
            }
            free(syms);
            if (strsymtbl != NULL)
            {
                free(strsymtbl);
            }
            return 0;
        }
    }
    if (strsymtbl != NULL)
    {
        free(strsymtbl);
    }
    return -1;
}

/*解析节表*/
unsigned int exe_load_section_headers(struct elf_internals *ei)
{
    Elf_Shdr *strsectblhdr = NULL;
    Elf_Shdr *sechdrs = NULL;
    size_t idx = 0;
    ssize_t symtab = -1;
    ssize_t strtab = -1;

    if (ei == NULL || ei->sechdr_offset == 0 || ei->sechdr_size == 0)
    {
        return -1;
    }
  
    ei->sechdrs = malloc(ei->sechdr_size);
    if (ei->sechdrs == NULL) 
    {
        TEL_ERROR("malloc failed!\n");
        return -1;
    }
    
    /*get section header ,write to sechdrs*/
    memset(ei->sechdrs, 0, ei->sechdr_size);
   
    if (lseek(ei->fd, ei->sechdr_offset, SEEK_SET) < 0) 
    {
        TEL_ERROR("lseek failed!\n");
        return -1;
    }
    if (read(ei->fd, ei->sechdrs, ei->sechdr_size) < 0) 
    {
        TEL_ERROR("read failed!\n");
        return -1;
    }
    
    /*the index of string table*/
    sechdrs = (Elf_Shdr *)ei->sechdrs;
    strsectblhdr = &sechdrs[ei->secnametbl_idx];
    if (lseek(ei->fd, strsectblhdr->sh_offset, SEEK_SET) < 0) 
    {
        TEL_ERROR("lseek failed!\n");
        return -1;
    }
    
    ei->strsectbl = malloc(strsectblhdr->sh_size);
    if (!ei->strsectbl) 
    {
        TEL_ERROR("malloc failed!\n");
        return -1;
    }
    ei->strsectbl_size = strsectblhdr->sh_size + 0;
    if (read(ei->fd, ei->strsectbl, strsectblhdr->sh_size) < 0)
    {
        TEL_ERROR("read failed!\n");
        return -1;
    }

   /*根据节区类型找到字符串表和符号表的索引*/
    for (idx = 0; idx < ei->sechdr_num; ++idx) 
    {
       // const char *name = &ei->strsectbl[sechdrs[idx].sh_name];
       
        switch (sechdrs[idx].sh_type) 
        {
        case SHT_SYMTAB:
        case SHT_DYNSYM:
            symtab = idx;
            break;
        case SHT_STRTAB:
            if (idx != ei->secnametbl_idx) 
            {
                strtab = idx;
               
                if (symtab >= 0 && exe_load_symbol_table(ei, &sechdrs[symtab],&sechdrs[strtab]) < 0)
                {
                    TEL_ERROR("Failed to retrieve symbol table.\n");
                }
                symtab = -1;
            }
            break;
        default:
            break;
        }
    }
    return 0;
}

/*解析程序头*/
unsigned int exe_load_program_headers(struct elf_internals *ei)
{
    Elf_Phdr *proghdrs = NULL;
    size_t idx = 0;
    unsigned int rc = 0;
    if (ei == NULL || ei->proghdr_offset == 0 || ei->proghdr_size == 0)
    {
        return -1;
    }
    ei->proghdrs = malloc(ei->proghdr_size);
    if (ei->proghdrs == NULL) 
    {
        return -1;
    }
    memset(ei->proghdrs, 0, ei->proghdr_size);

    /*读取程序头表*/
    if (lseek(ei->fd, ei->proghdr_offset, SEEK_SET) < 0) 
    {
        TEL_ERROR("lseek failed!\n");
        return -1;
    }
    if (read(ei->fd, ei->proghdrs, ei->proghdr_size) < 0)
    {
        TEL_ERROR("read failed!\n");
        return -1;
    }
    
    proghdrs = (Elf_Phdr *)ei->proghdrs;
    for (idx = 0; idx < ei->proghdr_num; ++idx) 
    {
        rc = 0;
       
        if (proghdrs[idx].p_type == PT_INTERP)
        {
            
            if (proghdrs[idx].p_filesz == 0)
                continue;
            if (lseek(ei->fd, proghdrs[idx].p_offset, SEEK_SET) < 0) 
            {
                TEL_ERROR("lseek failed!\n");
                rc = -1;
                break;
            }
            if (ei->interp.name) 
            {
                free(ei->interp.name);
                memset(&ei->interp, 0, sizeof(ei->interp));
            }
            ei->interp.name = malloc(proghdrs[idx].p_filesz);
            if (!ei->interp.name) 
            {
                TEL_ERROR("malloc failed!\n");
                rc = -1;
                break;
            }
            if (read(ei->fd, ei->interp.name, proghdrs[idx].p_filesz) < 0) 
            {
                TEL_ERROR("read failed!\n");
                rc = -1;
                break;
            }
            ei->interp.length = proghdrs[idx].p_filesz;
            ei->interp.ph_addr = proghdrs[idx].p_vaddr;
            
            
        } 
        
    }
    return rc;
}

/*解析文件头*/
unsigned int exe_load_headers(struct elf_internals *ei)
{
    Elf_Ehdr hdr;
    unsigned int fd = -1;
    if (ei == NULL) 
    {
        return -1;
    }
    fd = ei->fd;
    memset(&hdr, 0, sizeof(hdr));
    if (lseek(fd, 0, SEEK_SET) < 0) 
    {
        TEL_ERROR("lseek failed!\n");
        return -1;
    }

    /*get elf header ,save in hdr*/
    if (read(fd, &hdr, sizeof(hdr)) < 0) 
    {
        TEL_ERROR("read failed!\n");
        return -1;
    }

    /*support x86_64 i386 ppc ARM架构*/
    if (hdr.e_machine != EM_X86_64 && hdr.e_machine != EM_386 && 
        hdr.e_machine != EM_PPC && hdr.e_machine != EM_ARM)
    {
       
        return -1;
    }
    
    /*节头的起始偏移，数量，大小以及字符串节的索引*/
    if (hdr.e_shoff > 0)
    {
        ei->sechdr_offset = 0 + hdr.e_shoff;
        ei->sechdr_num = 0 + hdr.e_shnum;
        ei->sechdr_size = 0 + hdr.e_shnum * hdr.e_shentsize;
        ei->secnametbl_idx = 0 + hdr.e_shstrndx;
    }
    
    /*程序头表的其实偏移，数量，大小*/
    if (hdr.e_phoff > 0) 
    {
        ei->proghdr_offset = 0 + hdr.e_phoff;
        ei->proghdr_num = 0 + hdr.e_phnum;
        ei->proghdr_size = 0 + hdr.e_phnum * hdr.e_phentsize;
    }

    /*解析节区头*/
    if (exe_load_section_headers(ei) < 0)
    {
        TEL_ERROR("Error in loading section headers\n");
        return -1;
    }

    /*解析程序头*/
    if (exe_load_program_headers(ei) < 0)
    {
        TEL_ERROR("Error in loading section headers\n");
        return -1;
    }
    return 0;
}

/*加载符号表*/
struct elf_symbol *exe_load_symbols(const char *filename,size_t *symbols_num,struct elf_interp *interp)
{
    unsigned int rc = 0;
    
    struct elf_symbol *symbols = NULL;
    struct elf_internals ei;
    memset(&ei, 0, sizeof(ei));

    ei.fd = exe_open_filename(filename);
    if (ei.fd < 0) 
    {
        TEL_ERROR("exe_open_filename failed!\n");
        return NULL;
    }
    if ((rc = exe_load_headers(&ei)) < 0) 
    {
        TEL_ERROR("Unable to load Elf details for %s\n",filename);
    }
   
    if (ei.fd >= 0)
    {
        close(ei.fd);
    }
    ei.fd = -1;
    ei.strsectbl_size = 0;
    if (ei.strsectbl) 
    {
        free(ei.strsectbl);
        ei.strsectbl = NULL;
    }
    if (ei.sechdrs) 
    {
        free(ei.sechdrs);
        ei.sechdrs = NULL;
    }
    if (ei.proghdrs) 
    {
        free(ei.proghdrs);
        ei.proghdrs = NULL;
    }
    if (rc < 0) 
    {
        if (ei.interp.name)
            free(ei.interp.name);
        ei.interp.name = NULL;
        if (ei.symbols) 
        {
            
            size_t idx;
            for (idx = 0; idx < ei.symbols_num; ++idx) 
            {
                free(ei.symbols[idx].name);
                ei.symbols[idx].name = NULL;
            }
            free(ei.symbols);
        }
        ei.symbols = NULL;
        ei.symbols_num = 0;
    } 
    else 
    {
        symbols = ei.symbols;
        *symbols_num = ei.symbols_num;
        if (interp) 
        {
            interp->name = ei.interp.name;
            interp->length = ei.interp.length;
            interp->ph_addr = ei.interp.ph_addr;
            
        } 
        else 
        {
            if (ei.interp.name)
                free(ei.interp.name);
            ei.interp.name = NULL;
        }
       
    }
    return symbols;
}









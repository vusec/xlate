#pragma once

int gelf_find_sym(GElf_Sym *ret, Elf *elf, const char *key);
void *gelf_find_sym_ptr(Elf *elf, const char *key);

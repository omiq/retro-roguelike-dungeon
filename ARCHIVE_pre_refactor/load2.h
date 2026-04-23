#ifndef __CC65_LOADSTUB_PET_H
#define __CC65_LOADSTUB_PET_H

//Function call to more efficiently load a program stub into memory.
//The stub is a standard .prg file with the first two bytes being the
//load address.
//
//s is the name of the file, and nsize is the length of the filename
//excluding the NUL terminator.
void __fastcall__ load2 (char *s, unsigned char nsize);

#endif

/***************************************************************************
 *
 * Simple CBM I/O printf() Text Replacement for More Efficient code.
 * By Joseph Rose, a.k.a. Harry Potter
 *
 * This library can make small programs even smaller by separating the
 * features of printf() into smaller units if the full functionality of
 * printf() is not needed.  
 *
 * This library replaces putchar()'s functionality with direct kernal
 * calls.  You should also be able to use these functions to write
 * directly to an open file by setting it to listen.
 *
 * I would like to thank Jakub on the cc65 mailing list for commenting not
 * to use printf().
 *
 ***************************************************************************/

#ifndef __cc65_SimpleIO__
#define __cc65_SimpleIO__

//Prints a null-delimeted string to the screen, up to 256 bytes.
void __fastcall__ prints (char*);

//Same as above, but outputs a CR after the string.
void __fastcall__ printscr (char*);

//Prints a single character.  Directly calls chrout.
void __fastcall__ printc (char);
//Prints a return character.
void __fastcall__ printcr (void);

void __fastcall__ printi (int);

//Prints an unsigned number.
void __fastcall__ printu (unsigned);

//Gets a key-press.
unsigned char __fastcall__ getkey (void);

//Blinks the cursor, gets a key-press.
unsigned char __fastcall__ getkey2 (void);

//Gets a line from the user into In and returns the length.
unsigned char __fastcall__ getline (char* In);
void __fastcall__ home (void);
void __fastcall__ clrscr (void);

//Turn reverse video on or off.
void reverson  (void);
void reversoff (void);

//Clear screen and print title.
//(For the C128 version, pass the # columns in the current
//screen mode to columns.)
#if defined __C128__ || defined __PET__
void __fastcall__ writetitle (unsigned char columns, char* title);
#else
void __fastcall__ writetitle (char* title);
#endif
//Colors as variables:
//Note that the C128 version only works in 40-column mode!
//Also note that the Vic 20 version doesn't work--although you can make
//textcol available through a little extra programming.
//extern unsigned char backcol, brdrcol, textcol;
#ifdef __C128__
//#pragma zpsym ("textcol")
void backcol (unsigned char);
void brdrcol (unsigned char);
void textcol (unsigned char);
#elif defined __VIC20__
extern unsigned char backbrdrcol, textcol;
#else
extern unsigned char backcol, brdrcol, textcol;
#endif
#endif

extern unsigned char tabx, taby;
#pragma zpsym ("tabx")
#pragma zpsym ("taby")
void __fastcall__ clearline (unsigned char);
void __fastcall__ goline (unsigned char);


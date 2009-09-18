#ifndef __GFX_CON_ESC_H__
#define __GFX_CON_ESC_H__

#define CON_RESET				"\ec"

#define CON_ESC					"\e["

#define CON_CLEAR				CON_ESC "2J"
#define CON_HOME				CON_ESC "H"

#define CON_UP(rows)			CON_ESC #rows "A"
#define CON_DOWN(rows)			CON_ESC #rows "B"
#define CON_FORWARD(columns)	CON_ESC #columns "C"
#define CON_BACKWARD(columns)	CON_ESC #columns "D"

#define CON_POS(row, column)	CON_ESC #row ";" #column "H"
#define CON_SAVE				CON_ESC "s"
#define CON_RESTORE				CON_ESC "u"
#define CON_SAVEATTR			"\e7"
#define CON_RESTOREATTR			"\e8"

#define CON_COLRESET			CON_ESC "0m"

#define C_BLACK					CON_ESC "30m"
#define C_RED					CON_ESC "31m"
#define C_GREEN					CON_ESC "32m"
#define C_YELLOW				CON_ESC "33m"
#define C_BLUE					CON_ESC "34m"
#define C_MAGENTA				CON_ESC "35m"
#define C_CYAN					CON_ESC "36m"
#define C_WHITE					CON_ESC "37m"
#define C_IBLACK				CON_ESC "30;1m"
#define C_IRED					CON_ESC "31;1m"
#define C_IGREEN				CON_ESC "32;1m"
#define C_IYELLOW				CON_ESC "33;1m"
#define C_IBLUE					CON_ESC "34;1m"
#define C_IMAGENTA				CON_ESC "35;1m"
#define C_ICYAN					CON_ESC "36;1m"
#define C_IWHITE				CON_ESC "37;1m"

#define B_BLACK					CON_ESC "40m"
#define B_RED					CON_ESC "41m"
#define B_GREEN					CON_ESC "42m"
#define B_YELLOW				CON_ESC "43m"
#define B_BLUE					CON_ESC "44m"
#define B_MAGENTA				CON_ESC "45m"
#define B_CYAN					CON_ESC "46m"
#define B_WHITE					CON_ESC "47m"
#define B_IBLACK				CON_ESC "40;1m"
#define B_IRED					CON_ESC "41;1m"
#define B_IGREEN				CON_ESC "42;1m"
#define B_IYELLOW				CON_ESC "43;1m"
#define B_IBLUE					CON_ESC "44;1m"
#define B_IMAGENTA				CON_ESC "45;1m"
#define B_ICYAN					CON_ESC "46;1m"
#define B_IWHITE				CON_ESC "47;1m"

#define S_BLACK(s)				C_BLACK s C_WHITE
#define S_RED(s)				C_RED s C_WHITE
#define S_GREEN(s)				C_GREEN s C_WHITE
#define S_YELLOW(s)				C_YELLOW s C_WHITE
#define S_BLUE(s)				C_BLUE s C_WHITE
#define S_MAGENTA(s)			C_MAGENTA s C_WHITE
#define S_CYAN(s)				C_CYAN s C_WHITE
#define S_WHITE(s)				C_WHITE s
#define S_IBLACK(s)				C_IBLACK s C_WHITE
#define S_IRED(s)				C_IRED s C_WHITE
#define S_IGREEN(s)				C_IGREEN s C_WHITE
#define S_IYELLOW(s)			C_IYELLOW s C_WHITE
#define S_IBLUE(s)				C_IBLUE s C_WHITE
#define S_IMAGENTA(s)			C_IMAGENTA s C_WHITE
#define S_ICYAN(s)				C_ICYAN s C_WHITE
#define S_IWHITE(s)				C_IWHITE s C_WHITE

#endif


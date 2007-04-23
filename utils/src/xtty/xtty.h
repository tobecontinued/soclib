/*
 * $Log: xtty_old.h,v $
 * Revision 1.1  2005/10/17 21:16:47  nipo
 * Adding utils to kola
 *
 * Revision 1.1.1.1  2005/09/15 13:17:24  nipo
 * Imported soclib
 *
 * Revision 1.1.1.1  2005/01/27 13:42:45  wahid
 * First project import
 * Wahid
 *
 * Revision 1.1  2002/03/19 15:03:22  boris
 * Vcitty model addition
 *
 * Revision 1.1.1.1  2002/02/28 12:58:33  disydent
 * Creation of Disydent CVS Tree
 *
 * Revision 1.1.1.1  2001/11/19 16:55:32  pwet
 * Changing the CVS tree structure of disydent
 *
 * Revision 1.1.1.1  2001/07/24 13:31:45  pwet
 * cvs tree of part of disydent
 *
 * Revision 1.1.1.1  2001/07/19 14:32:25  pwet
 * New cvs tree
 *
 * Revision 1.2  1998/09/16 16:11:32  pwet
 * passage a cvs
 *
 * Revision 1.1  1998/09/01 09:49:14  pwet
 * Initial revision
 *
 * Revision 1.1  1998/07/16 18:04:12  pwet
 * Initial revision
 *
 * Authors: Frédéric Pétrot and Denis Hommais 
 */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#if 0
#define FONT            "-misc-fixed-bold-r-normal--15-140-75-75-c-90-iso8859-1"
#define TTYWIDTH        80
#define TTYHEIGHT       40
#define MAXCHAR         16
#else
#define FONT            "fixed"
#define TTYWIDTH        80
#define TTYHEIGHT       24
#define MAXCHAR         16
#endif

typedef struct {
   Display *display;
   Window   window;
   GC       gc;
   int      font_height;
   int      font_width;
   short    xcurse;
   short    ycurse;
   char     video[TTYHEIGHT][TTYWIDTH];  /* video character memory */
   short    rptr;
   short    wptr;
   FILE    *file;            /* redirection file if required */
   short    keybuf[MAXCHAR];
} xtty;



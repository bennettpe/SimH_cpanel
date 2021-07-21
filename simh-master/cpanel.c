/* cpanel.c: simulator control panel simulation

   Copyright (c) 2017-2020, Roberto Sancho

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   ROBERT M SUPNIK BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of Robert M Supnik shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from Robert M Supnik.

   27-Jun-17    RSV     First version control panel support (on HP2100 family)
   10-Nov-17    RSV     control panel support for rcornwell sims IBM 7000 family
   14-May-20    RSV     Control panel support for IBM 650 use standard LIB PNG bitmaps
   27-Jan-21    RSV     Control panel support for IBM NORC mouse drag and zoom
   22-Jun-21    RSV     Control panel support for IBM 701 support for big panels, reduce cpu usage

   These simulator commands controls the control panel

      SET CPANEL ON          --> Load the control panel definition file
                                 attached, process it, and open a
                                 GUI Similator Window for control panel.
                                 Controls defined are drawn on it.

      SET CPANEL OFF         --> Closes the control panel simulator GUI Window
                                 if this scp command is issued from control 
                                 panel, the interactive mode is turned off

      SET CPANEL OPTION = <NONE> | name
                             --> Defines an option with the given name that 
                                 can be tested for existence in DF with tags 
                                 ifdef, ifndef or elseif. Maximum len for Name 
                                 are 32 characters. Can define more than one option
                                 If <NONE> is used, all options are deleted. 
                                 Must be issued after ATT CPANEL 
                                 
      SET CPANEL TICKCOUNT = 0|1
                             --> Set to 0 to disable the variable intensity 
                                 display of lights. Set to 1 to enable them.
                                 The default value 1 is set on attach.
                                 

      SET CPANEL INTERACTIVE --> Allows to operate the control panel in an 
                                 interactive way. This command returns when
                                 user click on GUI window close icon (or if
                                 control panel issues a set cpanel off 
                                 command) or user press the WRU key (default
                                 Control-E) on console 

      SET CPANEL PRESS= name --> Simulates mouse clicking on given control name.
                                 Must be a single control, cannot be a control
                                 array name. This command allows to script
                                 the operation of control panel 

      SET CPANEL STATE= { <ALL> | name } / { * | n }
                             --> Forces on GUI window the state of given 
                                 control to n (a number), so it can be seen on 
                                 this state. If a control array name is given, 
                                 all the individual controls belonging to 
                                 control array are set to the state n. If 
                                 <ALL> is given as name, all controls state
                                 are set. If an asterisk (*) is given instead 
                                 of a number, the control is reset to normal
                                 operation 

      SET CPANEL MARK=<ALL> | <NONE> | name 
                             --> Marks on GUI control panel window the given 
                                 control, so it can be identified. If a control 
                                 array name is given, all the individual controls 
                                 belonging to control array are marked. If 
                                 <ALL> is given as name, all controls that have 
                                 OnClick event are marked (thus showing the
                                 clickable controls). If <NONE> is given as name
                                 all marks are removed.

      SET CPANEL LIST=<ALL> | <FREE> | name 
                             --> List the properties defined in control panel
                                 definition file for the given sigle control
                                 name, or control array name. 
                                 
                                 If <ALL> is given as name, then all controls 
                                 defined will be listed, the numbers of 
                                 defined/free controls are listed, and the 
                                 defined options are listed

                                 if <FREE> is given as name, only free 
                                 controls space is listed

      SET CPANEL SCALE=n     --> Sets the GUI window scale. Default is 100 
                                 (GUI has same size as defined in control panel 
                                 definition file). Use 50 to have a GUI window 
                                 of half size. Use 200 to have GUI window at
                                 double size. n ranges from 10 to 200.
                                 This command can be issued even if panel is not 
                                 displayed on GUI window (apply when window opens)
                                 Must be issued after ATT CPANEL 

      SET CPANEL POS=x/y     --> Sets the GUI window position at coord x,y
                                 (can be negative values)
                                 Must be issued after SET CPANEL ON

      ATT CPANEL <file>      --> Define the control panel definition file to be used.
                                 File is opened to check it can be read, and then is 
                                 closed, so it can be modified even if attached. It 
                                 is opened again, read and closed again on set cpanel on.
                                 Attach also resets the options defined. 

      DET CPANEL             --> Detach control panel definition file 

      defaults: SET CPANEL OFF, TICKCOUNT=1, SCALE=100, no file attached

      CPANEL device cannot be disabled. If CPU is stopped, GUI window is not refreshed 
      nor polled, so it will not react to mouse clicks (this is indicated by a slashed 
      circle). Refresh and clicks reactions occurs on next scp command execution. 
      Issue a SET CPANEL ON at any moment (or just press enter) to refresh the GUI 
      window and see the results of previous scp command.

      When Simulated CPU is executing code, clicking on the close button of GUI windows 
      ends the emulation (same as bye scp command). If CPU is stopped, the GUI close 
      button does nothing. To just close the GUI window, use the SCP command
      set cpanel off.
 
*/

// cpanel.c needs the preprocessor symbol CPANEL, USE_SIM_VIDEO and HAVE_LIBSDL to be defined

#if defined(CPANEL)

#if defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)
// ok, both preprocessor vars are defined
#else
#error cpanel.c needs the preprocessor symbol CPANEL, USE_SIM_VIDEO and HAVE_LIBSDL to be defined
#endif


#include <math.h>
#include "cpanel.h"

// the control panel impementation for simulted computer
// provides this array with the needed callbacks
extern CP_TYPE cp_types[];

DEVICE cp_dev;                                          /* control panel device */

#define UNIT_V_POWERON  (UNIT_V_UF + 0)                 /* unit powered on */
#define UNIT_POWERON    (1 << UNIT_V_POWERON)

t_stat cp_set_cpanel_on (UNIT *uptr, int32 value, CONST char *cptr, void *desc);
t_stat cp_set_param     (UNIT *uptr, int32 value, CONST char *cptr, void *desc);
t_stat cp_interactive   (UNIT *uptr, int32 value, CONST char *cptr, void *desc);
t_stat cp_attach        (UNIT *uptr, CONST char *cptr);
t_stat cp_dettach       (UNIT *uptr);
t_stat cp_reset         (DEVICE *dptr);
const char *cp_description (DEVICE *dptr);

DEBTAB sim_cp_debug[] = {
  {"DF",        CP_DF,             "Control Panel Definition File processing"},
  {"CMDS",      CP_CMDS,           "Control Panel Commands"},
  {"REFRESH",   CP_REFRESH,        "GUI Refresh events"},
  {"SDL",       CP_SDL,            "GUI SDL Events processing"},
  {0}
};


UNIT cp_unit = {
    UDATA (NULL, UNIT_SEQ+UNIT_ATTABLE+UNIT_TEXT+UNIT_RO, 0)
    };

MTAB cp_mod[] = {
    { UNIT_POWERON, UNIT_POWERON,  "cpanel on",              "ON",           cp_set_cpanel_on },
    { UNIT_POWERON, 0,             "cpanel off",             "OFF",          cp_set_cpanel_on },
    { MTAB_XTD | MTAB_VDV, 1,       NULL,                    "PRESS",        &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 2,       NULL,                    "MARK",         &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 3,       NULL,                    "LIST",         &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 4,       NULL,                    "STATE",        &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 5,       NULL,                    "OPTION",       &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 6,       NULL,                    "TICKCOUNT",    &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 7,       NULL,                    "SCALE",        &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 8,       NULL,                    "POS",          &cp_set_param, NULL, &cp_dev },
    { MTAB_XTD | MTAB_VDV, 0,       NULL,                    "INTERACTIVE",  &cp_interactive, NULL, &cp_dev },
    { 0 }
    };

DEVICE cp_dev = {
    "CPANEL", &cp_unit, NULL /* cp_reg */, cp_mod,      // unit needed to attach the definition file
    1, 10, 31, 1, 8, 8,                                 //??? what are these numbers ???
    NULL, NULL, &cp_reset,
    NULL, &cp_attach, &cp_dettach,
    NULL /* &cp_dib */, DEV_DEBUG, 0, sim_cp_debug,     /* unit allways enabled */
    NULL, NULL, NULL, NULL, NULL, &cp_description
    };




#define MAX_CONTROLS            1000                    // max number of controls per control panel
#define MAX_CARRAYS              128                    // max number of controls arrays per control panel
#define MAX_CARRAYITEMS         1000                    // max number of controls in control arrays
#define MAX_PIXELS      40*1024*1024                    // max number of pixels on control panel
#define MAX_IMAGES                32                    // max number of images for controls that can be loaded at same time
#define MAX_OPTIONS             2048                    // max number of chars to store options defined for control panel
#define MAX_TICK_COUNT           128                    // max mumber of register tick count 

#define CARRAY_START       (MAX_CONTROLS+1)         // first Id for Control Arrays, so Ids 0..MAX_CONTROLS are controls
                                                    // and Ids > MAX_CONTROLS are control arrays. This allows to get the
                                                    // type (single control or control array) just from Id value
// global vars
char cpanel_name[128] = "";                         // control panel name. Displayed on GUI window title
char cpanel_default_filename[128] = "";             // default control panel file name (without path). To be set by SET CPU scp command
int cpanel_ControlRedrawNeeded = 0;                 // if set to 1 -> asure that next SetState call will update the control on GUI screen even if state set is same that the control already had
int cpanel_State_has_changed = 0;                   // SetState call sets this to 0 or 1. 1=the state set was not the same the control had
int cpanel_on = 0;                                  // Control Panel GUI window. indicates if window created. 
                                                    // Inits as =0 -> power off (no GUI window) 
                                                    // if >0 -> control panel. cpanel_on holds the control panel type id
int cpanel_interactive = 0;                         // =1 to read scp commands from control panel 

int cpanel_measured_fps = 0;                        // measured fps value
CP_CLICK CP_Click;                                  // global variable with clicked control data
int cpanel_stop_flag = 0;                           // global variable to notify close icon clicked on GUI window
int Refresh_Frames_Count = 0;                       // frames done counter. At 60 FPS. Wraps after 9942 hours
int Refresh_tnow;                                   // value of sim_os_msec() at starting of last refresh. 

// private vars
static struct {                                     // Control Panel main data
    int ControlItems;                               // total number of defined controls (= items in Control[]
    int CArrayItems;                                // total number of defined control arrays (= items in CArray[]
    int SurfaceItems;                               // total number of defined pixels in surface arrays (= items in surface[]
    int ImgItems;                                   // total number of loaded images
    int TickCountItems;                             // total number of tick count registers in use
    struct {
        int State, nStates;                         // current state of the control, max number of possible states (=0 if control has no pixels (transparent) and only senses the click event 
        char Name[32];         
        int X, Y, W, H;                             // x,y position and witfth height. relative to top left corner of window. 
                                                    // if x=-1, control not draw nor sensed
        int MaxH;                                   // if >0, draw upo to this height (must be < to H)
        CP_CALLBACK OnClick;                        // onclick event function
        int iPos_surface;                           // start of pixel raw data in surface array
        int ByRef;                                  // =1 -> controls SameAs this one points to this control pixels instead on getting a copy of them
        int LastState;                              // LastState drawn on GUI (needed to draw only changes)
        int Mark;                                   // =1 -> show the control marked with and B/W semitransparent overlay 
                                                    // if bit 8 set, force the state for Mark & $ff
        struct {
            int State;                              // the refresh routine automaticaly set this state on control ...
            uint32 Duration_msec;                   // ... after this time has expired in wallclock ...
            uint32 Time0_msec;                      // ... counting from this moment in time
        } AutoNext;
        int nTick;                                  // entry used in TickCount array (-1 if none used) 
    } Control[MAX_CONTROLS];                        // controls defined
    struct {
        int Items;                                  // items in control array 
        int CId0;                                   // 1st controls inside control array
        char Name[32];     
        CP_CALLBACK OnClick;                        // onclick event function
        int nTick;                                  // entry used in TickCount array (-1 if none used) 
    } CArray[MAX_CARRAYS];
    int CArrayCidItems[MAX_CARRAYITEMS];            // control id items inside CArray
    uint32 * surface;                               // raw pixel data for controls
    struct {
        char Name[32];                              // images loaded to be used for control's states
        uint32 * surface;                           // raw pixel data 
        int W,H;
    } Img[MAX_IMAGES];
    struct {
        int32 Num;                                  /* total number of ticks counted (-1 -> entry is free ) */
        int32 Bit[64];                              /* number of ticks counted on that bit */
    } TickCount[MAX_TICK_COUNT];
    char Options[MAX_OPTIONS];                      // options defined (separated by ;)
    int FullRedrawNeeded;                           // =1 if a full redraw is needed
    int FullSurfaceRedrawTm0;                       // sim_os_msec value when a full surface is redraw 
    int EnableTickCount;                            // =0 -> do not use TickCount callback and disable SetStateWithIntensity
    int Scale;                                      // Scale factor for GUI window. 100 -> 100% -> GUI window with size as defined. 50% -> half size
    uint32 LastRefreshInit;                         /* sim_os_msec() at starting of last refresh. used to calc FPS */
    uint32 LastRefreshDone;                         /* sim_os_msec() at ending of last refresh. used to calc if refresh needed */
    uint32 LastTickIntensityDone;                   /* sim_os_msec() at start of last tick intensity callback. used to calc if callback is needed */
    int LastTickCountNum;                           // number of tickcount callbacks exec in current frame
    CP_CALLBACK CP_Refresh;                         // Control Panel refresh Callback
    CP_CALLBACK CP_Reset;                           // Control Panel reset Callback. Called on cpanel reset
    CP_CALLBACK CP_Done;                            // Control Panel done Callback. Called on cpanel off
    CP_CALLBACK CP_TickIntensityCount;              // callback to increment the tick count to calculate lights/leds intensity
} CP = { 0, 0, 0, 0, 0 }; 

UNIT * cp_unit_ptr = NULL;                          // pointer to control panel unit and device
DEVICE * cp_dev_ptr = NULL;

#define MAX_SCP_CMDS        16                      // max number scp commnads can be queued by control panel
#define MAX_SCP_CMD_LEN     80                      // max len of a scp command
char SCP_cmd[MAX_SCP_CMDS * MAX_SCP_CMD_LEN];       // stores scp commnads list queued by control panel pending to be executed
int  SCP_cmd_count = 0;                             // number of commands in list
int  SCP_cmd_echo  = 1;                             // echo DoSCP commands

int cpanel_TypeId = 0;                              // control panel type id from definition file
extern char vid_title[128];

int upcase(int c)
{
    if ((c >= 'a') && (c <= 'z')) return c - 'a' + 'A';
    return c;
}

// compare str up to len, case insensitive. len can be zero (compare upt to \0 char as end of strings)
// return 1 if equal, 0 if not
int cmpstr(const char * s1, const char * s2, int len) 
{
    int i,c1,c2; 

    if ((s1 == NULL) || (s2 == NULL)) return 0;  // no strings -> not equals
    if ((s1[0] == 0) || (s2[0] == 0)) return 0;  // strings connot be empty
    for (i=0;;i++) {
        c1 = s1[i]; c2 = s2[i];
        if ((len > 0) && (i==len))  return 1;  // no diferences found in len chars -> str are equal
        if ((c1 == 0) && (c2 == 0)) return 1;  // strings have same len -> str are equal
        if (upcase(c2) != upcase(c1)) return 0;
    }
}

int GetByName(char *Name) 
{
    int i,CId;
    if ((Name == NULL) || (Name[0] == 0)) {
        return -1;
    }
    // search a Control called Name
    for (i=0;i<CP.ControlItems;i++) {
        CId = i;
        if (cmpstr(CP.Control[i].Name, Name, 32)) return CId;
    }
    // control not found, now search as control array
    for (i=0;i<CP.CArrayItems;i++) {
        if (cmpstr(CP.CArray[i].Name, Name, 32)) {
            CId = i + CARRAY_START;
            return CId;
        }
    }
    return -1;
}

// Add definition of a new control. 
int AddControl(char *Name, int nStates,
               int X, int Y, int W, int H)
{
    int CId, i, n;
    char c; 
    // Control Id
    // check for room and get the control id
    if (CP.ControlItems >= MAX_CONTROLS) {
        fprintf(stderr, "No room to add control %s\n", Name);
        return -1; 
    }
    if ((Name != NULL) && (Name[0] != 0) && (GetByName(Name) >= 0)) {
        fprintf(stderr, "Already exists a Control or Control Array with name %s\n", Name);
        return -1; 
    }
    CId = CP.ControlItems++;
    // add the data
    CP.Control[CId].X = X;
    CP.Control[CId].Y = Y;
    CP.Control[CId].W = W;
    CP.Control[CId].H = H;
    CP.Control[CId].MaxH = 0;
    CP.Control[CId].nStates = nStates;      // if nStates==0 -> control not drawn. Just it is used for click on its area
    CP.Control[CId].OnClick = NULL;
    CP.Control[CId].nTick = -1;             // no tickcount by defautt
    CP.Control[CId].ByRef = 0;              // =0 no ByReference on SameAs by default

    // init auto next state (automaticaly changes the state after msec time)
    CP.Control[CId].AutoNext.State = -1;    // no auto next 
    // copy the name up to 31 chars. zero terminated. 
    if (Name == NULL) {
        CP.Control[CId].Name[0] = 0;
    } else for (i=0;i<32;i++) {
        if (i==31) {c = 0;} else {c = Name[i]; }
        CP.Control[CId].Name[i] = c;
        if (c == 0) break;
    }
    // see if room for pixels of control, and get space for pixels
    CP.Control[CId].iPos_surface = CP.SurfaceItems;
    n = CP.SurfaceItems + nStates * W * H + 1;
    if (n >= MAX_PIXELS) {
        fprintf(stderr, "No room for control image\n");
        return -1; // no room for pixels of control!
    }
    for (i=CP.Control[CId].iPos_surface; i<n;i++) CP.surface[i] = 0;     // fill with transparent 
    CP.SurfaceItems = n; 
    // int state 
    CP.Control[CId].State = 0;
    CP.Control[CId].Mark = 0;
    CP.Control[CId].LastState = -1;                                      // To force refresh
    // finished
    return CId;
}

int CHK_CId(int CId)
{
    if ((CId < 0) || (CId > CP.ControlItems)) {
        fprintf(stderr, "CId %d out of range (Items %d)\n", CId, CP.ControlItems); 
        return -1; 
    }
    return 0;
}

#define CHK_CArrayId(CArrayId)  if ((CArrayId < CARRAY_START) || (CArrayId > CARRAY_START+CP.CArrayItems)) {fprintf(stderr, "CArrayId %d out of range (Items %d)\n", CArrayId, CP.CArrayItems); return -1; }
#define CHK_Item(CArrayId,n)    if ((n < 0) || (n >= CP.CArray[CArrayId-CARRAY_START].Items)) {fprintf(stderr, "Item %d out of range (CArrayId %d, Items %d)\n", n, CArrayId, CP.CArray[(CArrayId)-(CARRAY_START)].Items); return -1; }

uint8 *Font = NULL;     // pointer to block of malloc mem that stores the internal font

static const char *FontBmp[] = {
    "xxxxx ..... ..x.. .x.x. .x.x. ..x.. ....x ..xx. ",
    "x...x ..... ..x.. .x.x. .x.x. .xxx. xx.x. .x..x ",
    "x...x ..... ..x.. .x.x. xxxxx x.x.. xx.x. ..x.. ",
    "x...x ..... ..x.. ..... .x.x. .xxx. ..x.. .x.x. ",
    "x...x ..... ..x.. ..... xxxxx ..x.x .x.xx x.... ",
    "x...x ..... ..... ..... .x.x. .xxx. .x.xx x...x ",
    "xxxxx ..... ..x.. ..... .x.x. ..x.. x.... .xxx. ",

    "..x.. ...x. .x... ..x.. ..x.. ..... ..... ..... ",
    "..x.. ..x.. ..x.. x.x.x ..x.. ..... ..... ..... ",
    "..x.. .x... ...x. .xxx. ..x.. ..... ..... ..... ",
    "..... .x... ...x. xxxxx xxxxx ..... xxxxx ..... ",
    "..... .x... ...x. .xxx. ..x.. ..x.. ..... ..... ",
    "..... ..x.. ..x.. x.x.x ..x.. ..x.. ..... .xx.. ",
    "..... ...x. .x... ..x.. ..x.. .x... ..... .xx.. ",

    "...x. .xxx. ..x.. .xxx. .xxx. ...x. xxxx. .xxx. ",
    "..x.. x..xx .xx.. x...x x...x ..xx. x.... x...x ",
    "..x.. x.x.x ..x.. ....x ....x .x.x. x.... x.... ",
    ".x... x.x.x ..x.. .xxx. .xxx. x..x. xxxx. xxxx. ",
    ".x... xx..x ..x.. x.... ....x xxxxx ....x x...x ",
    "x.... xx..x ..x.. x.... x...x ...x. x...x x...x ",
    "x.... .xxx. .xxx. xxxxx .xxx. ...x. .xxx. .xxx. ",

    "xxxxx .xxx. .xxx. ..... ..... ...x. ..... x.... ",
    "....x x...x x...x ..... ..... ..x.. ..... .x... ",
    "...x. x...x x...x .xx.. ..x.. .x... xxxxx ..x.. ",
    "..x.. .xxx. .xxxx .xx.. ..... x.... ..... ...x. ",
    "..x.. x...x ....x ..... ..x.. .x... xxxxx ..x.. ",
    "..x.. x...x x...x .xx.. ..x.. ..x.. ..... .x... ",
    "..x.. .xxx. .xxx. .xx.. .x... ...x. ..... x.... ",

    ".xxx. .xxx. ..x.. xxxx. .xxx. xxxx. xxxxx xxxxx ",
    "x...x x.xxx .x.x. x...x x...x x...x x.... x.... ",
    "....x x.x.x x...x x...x x.... x...x x.... x.... ",
    "...x. x.x.x xxxxx xxxx. x.... x...x xxx.. xxx.. ",
    "..x.. x.xx. x...x x...x x.... x...x x.... x.... ",
    "..... x...x x...x x...x x...x x...x x.... x.... ",
    "..x.. .xxx. x...x xxxx. .xxx. xxxx. xxxxx x.... ",

    ".xxx. x...x .xxx. ..xxx x...x x.... x...x x...x ",
    "x...x x...x ..x.. ...x. x...x x.... xx.xx xx..x ",
    "x.... x...x ..x.. ...x. x..x. x.... x.x.x x.x.x ",
    "x..xx xxxxx ..x.. ...x. xxx.. x.... x.x.x x..xx ",
    "x...x x...x ..x.. ...x. x..x. x.... x.x.x x...x ",
    "x...x x...x ..x.. x..x. x...x x.... x...x x...x ",
    ".xxx. x...x .xxx. .xx.. x...x xxxxx x...x x...x ",

    ".xxx. xxxx. .xxx. xxxx. .xxx. xxxxx x...x x...x ",
    "x...x x...x x...x x...x x...x ..x.. x...x x...x ",
    "x...x x...x x...x x...x x.... ..x.. x...x x...x ",
    "x...x xxxx. x...x xxxx. .xxx. ..x.. x...x x...x ",
    "x...x x.... x.x.x x.x.. ....x ..x.. x...x x...x ",
    "x...x x.... x..x. x..x. x...x ..x.. x...x .x.x. ",
    ".xxx. x.... .xx.x x...x .xxx. ..x.. .xxx. ..x.. ",

    "x...x x...x x...x xxxxx .xxxx x.... xxxx. ..x.. ",
    "x...x x...x x...x ....x .x... .x... ...x. .x.x. ",
    "x...x .x.x. .x.x. ...x. .x... .x... ...x. x...x ",
    "x...x ..x.. ..x.. ..x.. .x... ..x.. ...x. ..... ",
    "x.x.x .x.x. ..x.. .x... .x... ..x.. ...x. ..... ",
    "x.x.x x...x ..x.. x.... .x... ...x. ...x. ..... ",
    ".x.x. x...x ..x.. xxxxx .xxxx ...x. xxxx. ..... ",
     
    "..... .x... ...xx ..x.. xx... ..... ..... ..... ",
    "..... .x... ..x.. ..x.. ..x.. ..... ..... ..... ",
    "..... ..x.. ..x.. ..x.. ..x.. .x... ..... ..... ",
    "..... ..... xx... ..x.. ...xx x.x.x ..... ..... ",
    "..... ..... ..x.. ..x.. ..x.. ...x. ..... ..... ",
    "..... ..... ..x.. ..x.. ..x.. ..... ..... ..... ",
    "xxxxx ..... ...xx ..x.. xx... ..... ..... ..... ",

    "",
};

// load internal font (if bLoad==1), else unload internal font
void FontLoad(int bLoad) 
{
    int i,b,g,c,ix,iy,x,y;
    if (bLoad == 0) {
        if (Font) free(Font);
        Font = NULL;
        return;
    }
    Font = (uint8 *)malloc (129*7*sizeof(*Font));
    // clear all
    for (i=0;i<129*7;i++) Font[i]=0;
    b=0; ix=0; iy=0;
    while (b < 70) {
        for (y=0;y<7;y++) {
            g = 0;
            for (x=4;x>=0;x--) {
               c = FontBmp[iy*7+y][ix*6+x];
               g = (g << 1);
               if (c == 'x') g = g | 1;
            }
            Font[b*7+y] = g;
        }
        b++;
        ix++;
        if (ix>7) {
            ix=0;
            iy++;
        }
    }
}

// return pointer to surface for given state on control CId, so surface[0]=first pixel of image 
// also sets W and H with control width and heigt (if param are not NULL)
// if State == -1 -> return pos X,Y in main GUI windows in ww,hh params
// if error, return zero
uint32 * GetControlSurface(int CId, int State, int * ww, int * hh)
{
    int W, H, n; 
    if (CHK_CId(CId) < 0) return 0; 
    if (State == -1) {
        if (ww) *ww=CP.Control[CId].X;
        if (hh) *hh=CP.Control[CId].Y;
        return 0;
    }
    if (State >= CP.Control[CId].nStates) return 0;
    if (State < 0) return 0;
    W = CP.Control[CId].W; if (ww) *ww=W;
    H = CP.Control[CId].H; if (hh) *hh=H;
    n = CP.Control[CId].iPos_surface;                                   // start of control's pixels
    n = n + State * W * H;                                              // start of this state 
    if (n >= MAX_PIXELS) return 0;                                      // sanity check
    return &CP.surface[n]; 
}

// copy pixels from one control to another. Copy pixels from given state to given destination state image
// copy rectangle x0,y0,w,h from source state image (can be partially outside source image)
// place copied rectangle at x1,y1 in destination state image (can be partially outside destination image)
// source and destination control can be the same. Source and destination state can be the same but
// in this case, source and destination rectangle cannot overlap
// if w=h=0 -> use source control w and h
// x0,y0 are relative to FromCId (0,0=top left image corner), x1,y1 are relative to ToCId (0,0=top left image corner)
// if error return zero
// if copies image (FromCId) has alfa channel set, its alfa channel is applied when drawing it on ToCId image
int CopyControlImage(int FromCId, int FromState, int x0, int y0, int w, int h,
                     int ToCId,   int ToState,   int x1, int y1)
{
    uint32 * surface0; 
    uint32 * surface1;
    uint32 col; 
    int w0, h0, ww, hh, w1, h1; 
    int x,y,p0,p1; 
    uint32 AlphaMask = surface_rgb_color(0,0,0);
    int rr,gg,bb, alpha, rr_bg,gg_bg,bb_bg;
    
    if (CHK_CId(FromCId) < 0) return 0;                                 // sanity check
    if (CHK_CId(ToCId) < 0)   return 0;                                 // sanity check
    if ((w < 0) || (h < 0))   return 0;
    
    surface0 = GetControlSurface(FromCId, FromState, &ww, &hh);
    w0=ww; h0=hh;
    if ((!surface0) || (ww<1) || (hh<1)) return 0;

    // if w=0 or h=0, use source control size
    if (w==0) w=w0; if (h==0) h=h0;

    // check source rectange (x0, y0, w, h) is not outside source surface 
    if ((x0>=w0) || (y0>=h0)) return 0;
    if ((x0+w < 0) || (y0+h < 0)) return 0;
    if ((x1+w < 0) || (y1+h < 0)) return 0;
    
    surface1 = GetControlSurface(ToCId, ToState, &ww, &hh);
    w1=ww; h1=hh;
    if ((!surface1) || (ww<1) || (hh<1)) return 0;
    
    // check destination rectange (x1, y1, w, h) is not outside destination surface 
    if ((x1>=w1) || (y1>=h1)) return 0;
    
    // simple-minded surface copy algorith, but easy to understand and debug.
    // programmed on the hope that smart-minded simulator writer will not call this function
    // to copy rectangles from outside source surface to outside destination surface
    for(y=0;y<h;y++) {
        if (y0+y <  0) continue;
        if (y0+y >=h0) break;
        if (y1+y <  0) continue;
        if (y1+y >=h1) break;
        p0=x0+(y0+y)*w0;
        p1=x1+(y1+y)*w1;
        for (x=0;x<w;x++) {
           if (x0+x <  0) continue;
           if (x0+x >=w0) break;
           if (x1+x <  0) continue;
           if (x1+x >=w1) break;
           col = surface0[p0+x];
           if (col==0) continue; // source pixel color is transparent
           if ((col & AlphaMask) != AlphaMask) {
               // apply alpha on background
               alpha=get_surface_rgb_color(col,&rr,&gg,&bb)+1;           // get control image pixel with alpha value
               get_surface_rgb_color(surface1[p1+x],&rr_bg,&gg_bg,&bb_bg); // get background pixel
               // if alpha=255 -> color is rr. If alfa=0 -> color is rr_bg
               rr = rr_bg + (alpha * (rr-rr_bg)) / 256; 
               gg = gg_bg + (alpha * (gg-gg_bg)) / 256; 
               bb = bb_bg + (alpha * (bb-bb_bg)) / 256; 
               col = surface_rgb_color(rr,gg,bb);
           }            
           surface1[p1+x]=col;
        }
    }
    return 1; // success
}                    

// draw text in a control state bitmap. Used color is rr,gg,bb. Text is draw at x0, y0,w,h in
// source state image (can be partially outside bitmap)
// x0,y0 are relative to ToCId (0,0=top left image corner)
// use 7x7 internal font (upper case only) in a 8x10 box. LF or CR allowed in buf to continue text in 
// next line starting at x0
// if error return zero
int DrawTextImage   (int ToCId, int ToState,                 // ToCId, ToState: draw text in this control
                     int x0, int y0, int w0, int h0,         // x0, y0, w0, h0: position and box in control where to draw text, if w0/h0 = 0 use control width/heigh
                     int rr, int gg, int bb,                 // r,g,b: color of text
                     char * buf, int chrsz)                  // string to print, char size: 1-> x1, 2-> x2
{
    uint32 * surface0; 
    uint32 col; 
    int ww, hh; 
    int x,y,p0,g,ix,iy,n,x1; 
    char c; 
    
    if (CHK_CId(ToCId)  < 0)    return 0;                                 // sanity check
    if ((buf == NULL) || (buf[0]==0))   return 0;
    if ((rr < 0) || (gg < 0) || (bb < 0))   return 0;
    
    surface0 = GetControlSurface(ToCId, ToState, &ww, &hh);
    if ((!surface0) || (ww<1) || (hh<1)) return 0;

    if (w0 == 0) w0=ww; 
    if (h0 == 0) h0=hh; 
    if ((x0 >= w0) || (y0 >= h0)) return 0;

    col = surface_rgb_color(rr,gg,bb);

    // draw chars 
    n = 0;
    x1 = x0; 
    while((c=buf[n++]) > 0) {
        c = upcase(c);                   // convert to uppercase
        if ((c==10) || (c==13)) {
            y0 += 10*chrsz;
            x1 = x0;
            continue; 
        }
        if (c > 123) c=c-27;             // because lower case not defined in font
        if ((c < 32) || (c > 127)) c=31; // sets undef char if any
        c=c-31;
        for (iy=0;iy<7*chrsz;iy++) {
            y = y0 + iy; 
            if (y >= h0) break; if (y < 0) continue;             
            p0 = y * ww;
            g = Font[c*7 + (iy / chrsz)];
            for (ix=0;ix<5*chrsz;ix++) {
                x = x1 + ix; 
                if (x >= w0) break; 
                if ((g & 1) && (x >= 0)) surface0[p0+x]=col;
                if (   (chrsz==1) || 
                       ((ix % chrsz)==(chrsz-1))   ) g = (g >> 1);
            }
        }
        x1 += 8*chrsz;  
    }
    return 1; // success
}


// add pixels to control for given state. 
// if col == 1 -> return pixel color instead of setting it
uint32 AddControlPixel(int CId, int State, int X, int Y, uint32 col)
{
    int W, H, n;
    if (CHK_CId(CId) < 0) return 0; 
    if (State >= CP.Control[CId].nStates) return 0;
    if (State < 0) return 0;
    W = CP.Control[CId].W;
    H = CP.Control[CId].H;
    if ((X < 0) || (Y < 0) || (X >= W) || (Y >= H)) return 0;           // sanity check;
    // set the pixel
    n = CP.Control[CId].iPos_surface;                                   // start of control's pixels
    n = n + State * W * H;                                              // start of this state 
    n = n + Y * W + X;                                                  // pos of this pixel
    if (n >= MAX_PIXELS) return 0;                                      // sanity check
    if (col == 1) {
        return CP.surface[n]; 
    }
    CP.surface[n] = col;                                                // set the pixel
    return 0;
}

// copy pixels from one control to another for all states
int CopyControlPixels(int OrgCId, int DestCId)
{
    int i, iOrg, iDest, W,H,nStates;

    if (CHK_CId(OrgCId) < 0) return -1;                                 // sanity check
    if (CHK_CId(DestCId) < 0) return -1;                                // sanity check
    iOrg  = CP.Control[OrgCId].iPos_surface;                            // start of origine control's pixels
    iDest = CP.Control[DestCId].iPos_surface;                           // start of destination control's pixels
    W = CP.Control[OrgCId].W;
    H = CP.Control[OrgCId].H;
    nStates = CP.Control[OrgCId].nStates;
    if ( (W != CP.Control[DestCId].W) || 
         (H != CP.Control[DestCId].H) ||
         (nStates != CP.Control[DestCId].nStates) ) {
        fprintf(stderr, "Cannot copy pixels: Org and Dest controls have diferent geometry\n");
        return -1; 
    }
    for (i=0;i< nStates * W * H;i++) {
        CP.surface[iDest + i] = CP.surface[iOrg + i];
    }
    return 0;
}

// draw circle
int AddCircle(int CId, int State, int W, int H, uint32 col1)
{
    int i,x,y,r0,g0,b0,r1,g1,b1,rr,gg,bb, r, w2, h2;
    int bAntiAliased; 
    uint32 col0;
    double nd, xd, yd;

    if (CHK_CId(CId) < 0) return -1;                                // sanity check
    if (State >= CP.Control[CId].nStates) return -1;
    if (W > CP.Control[CId].W) return -1; 
    if (H > CP.Control[CId].H) return -1;
    if ((W < 2) || (H < 2)) return -1;
    col0 = AddControlPixel(CId, State, 0, 0, 1);                    // get the color under filled circle. Use control's pixel 0,0 
    if (col0 == 0) {
        bAntiAliased = 0;                                           // under circle no color (== 0 -> transparent) -> no antialiasing
    } else {
        get_surface_rgb_color(col0, &r0, &g0, &b0);                 // there is a color! convert it to rgb
        bAntiAliased = 1; 
    }
    w2 = W / 2; h2 = H / 2; 
    if (col1 == 0) {                                                // col==0 -> paint with transparent color
        bAntiAliased = 0; 
        r1 = g1 = b1 = 0;
    } else {
        get_surface_rgb_color(col1, &r1, &g1, &b1);                     // convert circle color to rgb
    }
    r = w2; 
    for(x=0;x<2*r*0.35+1;x++) {
        yd = sqrt((double)((r-1)*(r-1)-x*x)) * (double)(H) / (double)(W);
        y  = (int) yd;
        nd = (yd - y);                                              // antialiase to look great
        if ((nd > 0) && (y > 0) && (y < H-1) && (bAntiAliased)) {   // there is an antialiased pixel and room for it
            rr = (int) ((r1 - r0) * nd + r0);                       // calc the color for antialiased pixel
            gg = (int) ((g1 - g0) * nd + g0);
            bb = (int) ((b1 - b0) * nd + b0);
            col0 = surface_rgb_color(rr, gg, bb);
            AddControlPixel(CId, State, w2 + x, h2 + y + 1, col0);  // set antialiased pixel 
            AddControlPixel(CId, State, w2 + x, h2 - y - 1, col0); 
            AddControlPixel(CId, State, w2 - x, h2 + y + 1, col0); 
            AddControlPixel(CId, State, w2 - x, h2 - y - 1, col0); 
        }
        for (i=-y;i<=y;i++) { 
            AddControlPixel(CId, State, w2 + x, h2 + i, col1);      // set pixel color 
            AddControlPixel(CId, State, w2 - x, h2 + i, col1);      // set pixel color 
        }
    }
    r = h2; 
    for(y=0;y<2*r*0.35+1;y++) {
        xd = sqrt((double)((r-1)*(r-1)-y*y)) * (double)(W) / (double)(H);
        x  = (int) xd;
        nd = (xd - x);                                              // antialiase to look great
        if ((nd > 0) && (x > 0) && (x < W-1) && (bAntiAliased)) {   // there is an antialiased pixel and room for it
            rr = (int) ((r1 - r0) * nd + r0);                       // calc the color for antialiased pixel
            gg = (int) ((g1 - g0) * nd + g0);
            bb = (int) ((b1 - b0) * nd + b0);
            col0 = surface_rgb_color(rr, gg, bb);
            AddControlPixel(CId, State, w2 + x + 1, h2 + y, col0);  // set antialiased pixel 
            AddControlPixel(CId, State, w2 - x - 1, h2 + y, col0); 
            AddControlPixel(CId, State, w2 + x + 1, h2 - y, col0); 
            AddControlPixel(CId, State, w2 - x - 1, h2 - y, col0); 
        }
        for (i=-x;i<=x;i++) { 
            AddControlPixel(CId, State, w2 + i, h2 + y, col1);      // set pixel color 
            AddControlPixel(CId, State, w2 + i, h2 - y, col1);      // set pixel color 
        }
    }
    return 0;
}

// interpolate pixels for state nState to generate
// intermediate intensities mixing State0 (color for
// intensity 0%) and State1 (color for intensity 100%)
int InterpolateControlPixels(int CId, int nState)
{
    int n, iOrg, iOrg0, iOrg1, W,H,nStates;
    double r;
    int r0,g0,b0,r1,g1,b1,rr,gg,bb;
    uint32 col0, col1, col;

    if (CHK_CId(CId) < 0) return -1;                                // sanity check
    W = CP.Control[CId].W;
    H = CP.Control[CId].H;
    nStates = CP.Control[CId].nStates;
    if ((nState < 2) || (nState >= nStates)) return -1;
    iOrg0 = CP.Control[CId].iPos_surface;                           // start of origin control's pixels
    iOrg1 = iOrg0 + W * H;
    iOrg  = iOrg0 + nState * W * H;
    r = (nState-1) / (double) nStates;
    for (n=0;n<W*H;n++) {
        col0 = CP.surface[iOrg0 + n];
        col1 = CP.surface[iOrg1 + n];
        if ((col0 == 0) || (col1 == 0)) {
            col = 0;                                                // transparent pixels are kept
        } else {
            get_surface_rgb_color(col0, &r0, &g0, &b0);             // there is a color! convert it to rgb
            get_surface_rgb_color(col1, &r1, &g1, &b1);      
            rr = (int) ((r1 - r0) * r + r0);                        // calc the color for mixed pixel
            gg = (int) ((g1 - g0) * r + g0);
            bb = (int) ((b1 - b0) * r + b0);
            col = surface_rgb_color(rr, gg, bb);
        }
        CP.surface[iOrg + n] = col;
    }
    return 0;
}

// Add definition of a new control array. returns the control array id, or -1 if no room
int AddCArray(char *Name)
{
    int CArrayId, CId0, i;
    char c; 
    // Control Array Id
    // check for room and get the control array id
    if (CP.CArrayItems >= MAX_CARRAYS) {
        fprintf(stderr, "No room to add control array %s\n", Name);
        return -1;
    }
    if ((Name != NULL) && (Name[0] != 0) && (GetByName(Name) >= 0)) {
        fprintf(stderr, "Already exists a Control or Control Array with name %s\n", Name);
        return -1; 
    }
    CArrayId = CARRAY_START+CP.CArrayItems;
    if (CP.CArrayItems == 0) {
        CId0 = 0;
    } else {
        // next free item CId0 is next from previos CArray
        CId0 = CP.CArray[CArrayId-CARRAY_START-1].CId0 + CP.CArray[CArrayId-CARRAY_START-1].Items;
    }
    CP.CArray[CArrayId-CARRAY_START].CId0 = CId0;
    CP.CArray[CArrayId-CARRAY_START].Items = 0;
    CP.CArray[CArrayId-CARRAY_START].nTick = -1; // no tick count by default
    CP.CArrayItems++;
    // add the data
    CP.CArray[CArrayId-CARRAY_START].OnClick = NULL;
    // copy the name up to 31 chars. zero terminated. 
    if (Name == NULL) {
        CP.CArray[CArrayId-CARRAY_START].Name[0] = 0;
    } else for (i=0;i<32;i++) {
        if (i==31) {c = 0;} else {c = Name[i]; }
        CP.CArray[CArrayId-CARRAY_START].Name[i] = c;
        if (c == 0) break;
    }
    // finished
    return CArrayId;
}

int AddCArrayItem(int CArrayId, int CId)
{
    int CId0, n;
    if (CHK_CId(CId) < 0) return -1;                                // sanity check
    CHK_CArrayId(CArrayId)              
    if (CArrayId != CARRAY_START+CP.CArrayItems - 1) {
        fprintf(stderr, "Can only add controls to last defined CArray (Current CArrayId %d, Last defined CArrayId %d)\n", 
            CArrayId, CARRAY_START+CP.CArrayItems - 1);
        return -1;
    }
    n = CP.CArray[CArrayId-CARRAY_START].Items;
    CId0 = CP.CArray[CArrayId-CARRAY_START].CId0;
    if (CId0 + n >= MAX_CARRAYITEMS) {
        fprintf(stderr, "No room to add control %d to control array %d\n", CId, CArrayId);
        return -1;
    }
    CP.CArray[CArrayId-CARRAY_START].Items++;
    CP.CArrayCidItems[CId0 + n] = CId;
    return 0;
}

int AddPNGImage(char * ImgName, char * FileName)
{
    int i,j, nImage, W,H;
    uint32 *pngsurface;

    // get room in images array
    for (i=0;i<CP.ImgItems;i++) {
        if (cmpstr(CP.Img[i].Name, ImgName, -1) == 0) continue;
        // image already defined -> free it
        if (CP.Img[i].surface) {
            free(CP.Img[i].surface);
            CP.Img[i].surface = NULL; 
        }
        nImage = i;
        break;
    }
    if (i>=CP.ImgItems) {
        // new image
        if (CP.ImgItems >= MAX_IMAGES) {
            fprintf(stderr, "Too many Images loaded\n");
            return -1;
        }
        i=CP.ImgItems++;
        // set the name
        for (j=0;ImgName[j];j++) CP.Img[i].Name[j] = ImgName[j];
        CP.Img[i].Name[j] = 0;
        nImage = i;
    }
    // load whole PNG image file into buf buffer
    // read_png allocated the surface memory
    pngsurface = read_png_file(FileName, &W, &H);
    if (!pngsurface) {
        fprintf(stderr, "Error decoding PNG image\n");
        return -1;
    }
    CP.Img[nImage].surface = pngsurface;
    CP.Img[nImage].W = W;
    CP.Img[nImage].H = H;
    return 0;
    
}


char * IsOptionParam; // points to / of found OPT/PARAM, else NULL
int nIsOption; // char num in Options array of found OPT/PARAM, else 0
// if bAllowOptParam = 0 -> return 1 if Name or Name/param is in options buffer 
//                          return 0 if not extactly Name/param found.
// if bAllowOptParam = 1 -> return 1 if Name or Name/whatever is in options buffer.
int IsOptionOrOptionParam(char * Name, int bAllowOptParam)
{
    int iOpt, iName;
    char cOpt, cName; 

    nIsOption = 0;
    IsOptionParam = NULL;
    if ((Name == NULL) || (Name[0] == 0)) return 0;
    iOpt = 0; iName = 0; 
    while(1) {
        cName = Name[iName];
        cOpt  = CP.Options[iOpt];
        if (((cName == 0) || (cName == '/') && (bAllowOptParam)) && ( (cOpt == 0) || (cOpt == ',') || (cOpt == '/') ) ) {
            if (cOpt == '/') IsOptionParam = &CP.Options[iOpt];
            return 1; // Option found
        }
        if (upcase(cName) == upcase(cOpt)) {
            iName++; iOpt++; // Name partially found ... keep comparing
            continue;
        }
        // not found. Skip current option up to start of next one
        for (; (cOpt=CP.Options[iOpt]) && (cOpt != ','); iOpt++);
        if (cOpt == 0) return 0; // no more options
        iOpt++; iName = 0;
        nIsOption = iOpt;
    }
}

// return 1 if Name or Name/whatever is in options buffer.
int IsOption(char * Name)
{
    return IsOptionOrOptionParam(Name, 1);
}


// remove Name from CP.Options array 
// remove option "ABC" and option "ABC/XXX" whatever is XXX
void RemoveOption(char * Name)
{
    int i, n, c; 

    if ((Name == NULL) || (Name[0] == 0)) return;
    if (IsOption(Name)==0) return; 

    // option already present, remove it
    for (i=nIsOption;; i++) { // i points to start of option found 
        c=CP.Options[i]; 
        if (c == 0) { // end of option, so the found one was the last
            CP.Options[nIsOption]=0;
            break;
        }
        if (c == ',') { // end of this option, i points to next one
            n = i+1;
            for (i=nIsOption;c;) c=CP.Options[i++]=CP.Options[n++]; // copy from i+1 up to end of screen
            break;
        }
    }
}

// add Name to CP.Options array if does not already in
// add "ABC" or  "ABC/DEF" replacing "ABC/whatever"
// if Name = "<NONE>" clears the defined options
// return -1 if invalid name because options buffer is full, of becuase Name is empty
int AddOption(char * Name)
{
    int i,j,n;

    if ((Name == NULL) || (Name[0] == 0)) return -1;
    if (cmpstr(Name, "<NONE>",0)) {
        // empty options buffer
        CP.Options[0] = 0;
        return 0;
    } 
    // option already present, remove it
    RemoveOption(Name);
    // check if room to add Name to options array
    for (i=0; CP.Options[i]; i++); // i points to end of string NULL
    for (n=0; Name[n]; n++); // n = chars length
    if (n > 32) return -1;   // option too long
    if (i+n+2 >= MAX_OPTIONS) 
        return -1;           // no room for option
    // if buffer no empty, add a separator
    if (i > 0) {
        CP.Options[i++] = ',';
        CP.Options[i] = 0;
    }
    // add Name to buffer
    for(j=0;j<n;j++) CP.Options[i++] = Name[j];
    CP.Options[i] = 0;
    return 0;
}

#define CPDF_TAG_COMMENT            1
#define CPDF_TAG_CPNAME             2
#define CPDF_TAG_CPTYPE             3
#define CPDF_TAG_LOAD_IMG           4
#define CPDF_TAG_CNAME              5
#define CPDF_TAG_BASEXY             6
#define CPDF_TAG_XYWH               11
#define CPDF_TAG_WH                 12
#define CPDF_TAG_NSTATES            13
#define CPDF_TAG_INITIALSTATE       14
#define CPDF_TAG_STATEIMG           20
#define CPDF_TAG_STATEIMGMASK       21
#define CPDF_TAG_STATERGB           22
#define CPDF_TAG_STATERGB_CIRCLE    23
#define CPDF_TAG_STATERGB_AREA      24
#define CPDF_TAG_STATERGB_TEXT      25
#define CPDF_TAG_CARRAY             31
#define CPDF_TAG_CARRAYITEM         32
#define CPDF_TAG_SAME               33
#define CPDF_TAG_HEIGHT             34
#define CPDF_TAG_SAME_BYREF         35
#define CPDF_TAG_AUTOSTATES         36
#define CPDF_TAG_TRANSPARENT        37
#define CPDF_TAG_IFOPT              41
#define CPDF_TAG_IFNOPT             42
#define CPDF_TAG_ELSE               43
#define CPDF_TAG_ELSEIF             44
#define CPDF_TAG_ENDIF              45
#define CPDF_TAG_END                46
#define CPDF_TAG_DEFOPT             47
#define CPDF_TAG_UNDEFOPT           48
#define CPDF_TAG_ECHO               49

struct {
    char *Name;
    int nTag;
} tags[] = {
    {";",                   CPDF_TAG_COMMENT}, 
    {"ControlPanelName",    CPDF_TAG_CPNAME},
    {"ControlPanelType",    CPDF_TAG_CPTYPE},
    {"ControlName",         CPDF_TAG_CNAME}, 
    {"LoadImage",           CPDF_TAG_LOAD_IMG},
    {"BaseXY",              CPDF_TAG_BASEXY},
    {"XYWH",                CPDF_TAG_XYWH}, 
    {"WH",                  CPDF_TAG_WH}, 
    {"nStates",             CPDF_TAG_NSTATES}, 
    {"InitialState",        CPDF_TAG_INITIALSTATE},
    {"StateImg",            CPDF_TAG_STATEIMG}, 
    {"StateImgMask",        CPDF_TAG_STATEIMGMASK}, 
    {"AutoStates",          CPDF_TAG_AUTOSTATES}, 
    {"TransparentColor",    CPDF_TAG_TRANSPARENT}, 
    {"StateRgb",            CPDF_TAG_STATERGB}, 
    {"StateRgbCircle",      CPDF_TAG_STATERGB_CIRCLE}, 
    {"StateRgbArea",        CPDF_TAG_STATERGB_AREA}, 
    {"StateRgbText",        CPDF_TAG_STATERGB_TEXT}, 
    {"XYSameAs",            CPDF_TAG_SAME}, 
    {"Height",              CPDF_TAG_HEIGHT}, 
    {"SameAsByReference",   CPDF_TAG_SAME_BYREF}, 
    {"ControlArrayName",    CPDF_TAG_CARRAY}, 
    {"ControlArrayItems",   CPDF_TAG_CARRAYITEM}, 
    {"IfOpt",               CPDF_TAG_IFOPT}, 
    {"IfnOpt",              CPDF_TAG_IFNOPT}, 
    {"else",                CPDF_TAG_ELSE}, 
    {"elseif",              CPDF_TAG_ELSEIF}, 
    {"endif",               CPDF_TAG_ENDIF}, 
    {"end",                 CPDF_TAG_END}, 
    {"DefOpt",              CPDF_TAG_DEFOPT}, 
    {"UnDefOpt",            CPDF_TAG_UNDEFOPT}, 
    {"echo",                CPDF_TAG_ECHO},
    {NULL}
};

// get len of str in control panel definition file
// string = chr(33)..chr(127) except "=" and ","
int tag_len(char * s) {
    int i; 
    for (i=0;;i++) {
        if ((s[i] < 33) || (s[i] > 127)) break;
        if ((s[i] == '=') || (s[i] == ',')) break;
    }
    return i;
}
// remove leading spaces, control chars and comments
void trim_str(char * s)
{
    int i;
    char * p;
    p=s;
    while ((p != NULL) && (*p != 0)) {
        if ((*p==';') && (p==s)) {*p=0;break;}                      // convert comment at start of line in end of line
        if ((*p==';') && (p!=s) && (*(p-1)==32)) {*p=0;break;}      // convert comment in middle of line in end of line
        if ((*p==10) || (*p==13)) {*p=0;break;}                     // convert start/cr/lf in end of line
        if (*p<32) *p=32;                                           // convert non printable control ascii char to space
        p++;
    }
    while ((s != NULL) && (s[0] == 32)) {
        for(i=0;s[i]>0;i++) s[i]=s[i+1];
    }
}
// trim s for everything up to char ch
void rtrim_str(char * s, int ch)
{
    int i, j;
    if ((s == NULL) || (*s == 0)) return;
    i=0;
    while(1) {
        if (s[i] == 0) break;
        if (s[i] == ch) break;
        i++;
    }
    if (s[i] == 0) { // ch not found -> return empty string
        s[0] = 0;
        return;
    }
    // ch found -> preserve rest of string
    j=0; i++; 
    while(1) {
        s[j]=s[i];
        if (s[i] == 0) break;
        i++;j++;
    }
}

// return the tag found, remove "tag=" it from s
int cpdf_tag(char * s) {

    int n,len;
    trim_str(s);
    len=tag_len(s);
    if (len < 1) return CPDF_TAG_COMMENT;
    for (n=0;;n++) {
        if (tags[n].Name == NULL) break;
        if ((cmpstr(tags[n].Name, s, len)) && (len==tag_len(tags[n].Name))) {
            // tag found. skip tag string and trailing spaces up to =
            rtrim_str(s, '=');
            return tags[n].nTag;
        }
    }
    return 0;
}
// copy str to Name (max nSize-1 chars), remove "str," from s
void cpdf_str_param(char * s, char * Name, int nSize)
{
    int i,l;
    if ((Name == NULL) || (s == NULL) || (nSize < 2))  return;
    Name[0] = 0;
    trim_str(s);
    l = (nSize > 32) ? nSize: tag_len(s);
    for(i=0;i<l;i++) {
        if (i == nSize-1) break;
        if (s[i] == 0) {
            s[0] = 0; // all source s string used. Mark as empty
            break;
        }
        Name[i]=s[i];
    }
    Name[i] = 0;
    // remove trailing spaces from Name
    while (i > 0) {
        if (Name[i-1] != ' ') break;
        Name[--i] = 0;
    }
    // remove up to trailing , from command string
    rtrim_str(s, ',');
}

// return -1 if minus signs, 0 if no sign, +1 if plus sign
// removes the sign
int cpdf_num_sign(char * s) 
{
    int n = 0;
    if (s == NULL) return 0;
    if (s[0] == '+') {n = 1;} else if (s[0] == '-') {n = -1;}
    if (n == 0) return 0;
    s[0] = ' ';
    trim_str(s);
    return n;
}

// return value (0 to 99999) of str, remove "str," from s. 
int cpdf_num_param(char * s)
{
    char str[8];
    int32 n,i,c,ok;

    cpdf_str_param(s, str, 6);
    n = i = ok = 0;
    while (1) {
        c = str[i++];
        if ((c < '0') || (c > '9')) break;
        n = n * 10 + c - '0';
        ok = 1;
    }
    return ok ? n : -1;
}

// initialize intentisty ticks counter. 
void InitTickCount(int nTick)
{
    int i;
    for (i=0;i<64;i++) CP.TickCount[nTick].Bit[i] = 0;
    CP.TickCount[nTick].Num = 0;
}

// return free intentisty ticks counter (and init it)
int GetTickCountItem()
{
    int i;
    for (i=0;i<CP.TickCountItems;i++) {
        if (CP.TickCount[i].Num < 0) {
            InitTickCount(i);
            return i;
        }
    }
    if (CP.TickCountItems >= MAX_TICK_COUNT) return -1;
    i = CP.TickCountItems++;
    InitTickCount(i);
    return i;
}

// return 1 if file exists (i.e. can be opened), else return 0
int file_exists(char * filename)
{
    FILE *file;

    if (filename == NULL) return 0;
    file = sim_fopen (filename, "r");
    if (file == NULL)                          // open failed? 
        return 0;
    fclose (file);
    return 1;
}



#define DF_ERR_NLIN             fprintf(stderr, "Control Panel Definition File line %d: ", nLin)
#define DF_ERR(str)             DF_ERR_NLIN; fprintf(stderr, str); fprintf(stderr, "\n")
#define DF_ERR2(str,param)      DF_ERR_NLIN; fprintf(stderr, str, param); fprintf(stderr, "\n")
#define DF_ERR3(str,p1,p2)      DF_ERR_NLIN; fprintf(stderr, str, p1, p2); fprintf(stderr, "\n")
#define DF_ERR_INV_NUM(str)     DF_ERR2("invalid number in %s", str)

// load & process control panel definition file (using attached filename)
void ControlPanelLoad(CP_TYPE *cp_type, UNIT *uptr, DEVICE *dptr)
{
    FILE *file;
    char *DF_filename; 
    char lbuf[255];
    char ControlName[255];
    char Name[255];
    char Name2[32];
    int CId, CArrayId, OrgCId, save_SurfaceItems; 
    int nLin,X,Y,W,H,n,ix,iy,wx,wy,r,g,b,i,tag,nImg,nTick,ifopt, BaseX, BaseY; 
    int col, TransparentColor, TransparentValue, rr,gg,bb, nBase, nMask, alpha;
    // clear CP struct to receive new control panel data
    CP.ControlItems     = 0;       // total number of defined controls (= items in Control[]
    CP.CArrayItems      = 0;       // total number of defined control arrays (= items in CArray[]
    CP.SurfaceItems     = 0;       // total number of defined pixels in surface arrays (= items in surface[]
    CP.ImgItems         = 0;
    CP.TickCountItems   = 0;
    cpanel_TypeId   = 0;       

    if (uptr->filename) {
        DF_filename = uptr->filename;
    } else {
        // Control Panel definition file not attached, check default name
        if ((!cpanel_default_filename[0]) || (!file_exists(cpanel_default_filename))) {
            fprintf(stderr, "Control Panel definition file not attached\n");
            return;
        }
        // use default name
        DF_filename = cpanel_default_filename;
    }
    file = sim_fopen (DF_filename, "r");
    if (file == NULL) {                         // open failed? 
        fprintf(stderr, "Cannot open Control Panel definition file %s\n", uptr->filename);
        return;
    }
    sim_debug (CP_DF, dptr, "Control Panel definition file used: %s\n", uptr->filename);
    sim_debug (CP_DF, dptr, "Options defined at start of DF processing:\n");
    sim_debug (CP_DF, dptr, "   %s\n", CP.Options);

    // load internal font
    FontLoad(1);
    // set CP.Surface to its max size
    CP.surface = (uint32 *)malloc ((MAX_PIXELS)*sizeof(*CP.surface)); // 40M pixels x 4bytes = 160 Mb max
    cpanel_name[0] = 0;     // control panel's name
    ControlName[0] = 0;     // current control name being defined
    CId = CArrayId = X = Y = W = H = n = col = wx = wy = r = g = b = i = -1; 
    nLin = 0; ifopt = 0; TransparentColor = 0;
    BaseX = BaseY = 0;
    while (fgets (lbuf, sizeof(lbuf)-1, file)) {
        if (ifopt==10) break; // terminate file processing
        nLin++;
        sim_debug (CP_DF, dptr, "Lin %d: %s\n", nLin, lbuf);
        lbuf[sizeof(lbuf)-1] = 0;    // paranoic end of string
        trim_str(lbuf);
        tag=cpdf_tag(lbuf);
        if (ifopt == 2) {      // skip until else or endif 
            if ((tag != CPDF_TAG_ELSE) && (tag != CPDF_TAG_ELSEIF) && (tag != CPDF_TAG_ENDIF)) {
                tag = CPDF_TAG_COMMENT;   // skip line by setting tag as it was comment
                sim_debug (CP_DF, dptr, "(Lin %d Skipped)\n", nLin);
            }
        } else if (ifopt == 3) {      // skip until endif 
            if (tag != CPDF_TAG_ENDIF) {
                tag = CPDF_TAG_COMMENT;   // skip line by setting tag as it was comment
                sim_debug (CP_DF, dptr, "(Lin %d Skipped)\n", nLin);
            }
        }
process_tag:
        switch( tag ) {
            case CPDF_TAG_COMMENT:
                // ; is the comment char
                lbuf[0] = 0;     // assure line is cleared
                break;
            case CPDF_TAG_CPNAME: 
                // ControlPanelName= name shown on windows title up to 127 chars
                if (cpanel_name[0]) {
                    DF_ERR("control panel name redefinition");
                    break;
                }
                cpdf_str_param(lbuf, cpanel_name, 128);
                sim_debug (CP_DF, dptr, "Control Panel Name: %s\n", cpanel_name);
                break;
            case CPDF_TAG_CPTYPE: 
                // ControlPanelType= name of control panel type 
                if (cpanel_TypeId) {
                    DF_ERR("control panel type redefinition");
                    break;
                }
                cpdf_str_param(lbuf, Name, 32);
                sim_debug (CP_DF, dptr, "Control Panel Type: %s\n", Name);
                for (n=0;;n++) {
                    if (cp_type[n].Name == NULL) break;
                    if (cmpstr(cp_type[n].Name, Name, 32)) {
                        cpanel_TypeId = cp_type[n].Id;
                        break;
                    }
                } 
                if (cpanel_TypeId < 1) {
                    DF_ERR2("control panel type %s not defined", Name);
                    cpanel_TypeId = -1;
                }
                break;
            case CPDF_TAG_LOAD_IMG:
                // LoadImage=Name up to 31 chars, filename up to 255 chars
                // to be used with StateImg=
                cpdf_str_param(lbuf, Name2, 32);
                cpdf_str_param(lbuf, Name, 255);
                { // get path from definition file
                    char path[PATH_MAX+1];
                    int i,j;
                    for (i=0;(i<PATH_MAX-1) && (uptr->filename[i]);i++) path[i]=uptr->filename[i];
                    for (j=i-1;(j>=0) && (path[j] != '\\') && (path[j] != '/'); j--);
                    path[++j]=0;
                    // add path to name
                    for(i=0;(j<PATH_MAX-1) && (Name[i]);i++) path[j++] = Name[i];
                    path[j]=0;
                    sim_debug (CP_DF, dptr, "Load Image Name: %s, FullFileName %s\n", Name2, path);
                    if ((Name2[0]==0) || (Name[0]==0) || (AddPNGImage(Name2, path) < 0)) {
                        DF_ERR2("invalid LoadImage %s", path);
                        break;
                    }
                }
                break;
            case CPDF_TAG_CNAME: 
                // ControlName=Name up to 31 chars 
                cpdf_str_param(lbuf, ControlName, 32);
                sim_debug (CP_DF, dptr, "Control Name: %s\n", ControlName);
                break;
            case CPDF_TAG_BASEXY:
                // BaseXY=value to be added to pos X of control, value to be added to pos Y of control
                // only affects XYWH= and XYSame= tags
                // if values have a sign +/-, sets the new BaseXY values relative to the previous set ones
                wx = cpdf_num_sign(lbuf);
                X = cpdf_num_param(lbuf);
                wy = cpdf_num_sign(lbuf);
                Y = cpdf_num_param(lbuf);
                sim_debug (CP_DF, dptr, "BaseX=%d (sign %d), BaseY=%d (sign %d)\n", X,wx,Y,wy);
                if ( (X<0) || (Y<0) ) {
                    DF_ERR_INV_NUM( (X<0) ? "X": (Y<0) ? "Y": "?");
                    break;
                }
                if ((wx == 0) && (wy == 0)) {
                    BaseX = X; BaseY = Y;
                } else {
                    if (wx == 0) wx = 1; if (wy == 0) wy = 1; 
                    BaseX += wx * X; BaseY += wy * Y;
                }
                break;
            case CPDF_TAG_XYWH: 
                // XYWH=pos x of control, pos y of control, width, heigth
                // in pixel coordinates. (origin top left). If user click in this arean, the OnClick event callback is called
                X = cpdf_num_param(lbuf);
                Y = cpdf_num_param(lbuf);
                W = cpdf_num_param(lbuf);
                H = cpdf_num_param(lbuf);
                sim_debug (CP_DF, dptr, "X=%d, Y=%d, W=%d, H=%d\n", X,Y,W,H);
                if ( (X<0) || (Y<0) || (W<0) || (H<0) ) {
                    DF_ERR_INV_NUM( (X<0) ? "X": (Y<0) ? "Y": (W<0) ? "W": (H<0) ? "H": "?");
                    break;
                }
                X += BaseX; Y += BaseY; // add BaseXY offset 
                break;
            case CPDF_TAG_WH: 
                // WH=width, heigth of control. X is not defined and the control is not draw nor sensed. It is used
                // to define generic controls to be used in SameAs
                X = -1;
                Y = -1;
                W = cpdf_num_param(lbuf);
                H = cpdf_num_param(lbuf);
                sim_debug (CP_DF, dptr, "W=%d, H=%d\n", W,H);
                if ( (W<0) || (H<0) ) {
                    DF_ERR_INV_NUM( (W<0) ? "W": (H<0) ? "H": "?");
                    break;
                }
                break;
            case CPDF_TAG_NSTATES: 
                // nStates= number of possible states for the control
                // Each state has a different image
                // is nState = 0 -> no image. Just the click area is sensed
                // this tag (and XYSameAs) effectivelly creates the control
                n = cpdf_num_param(lbuf);
                TransparentColor = 0; // reset transparet color
                sim_debug (CP_DF, dptr, "nStates=%d\n", n);
                if (( n<0 ) || (n > 255))  {
                    DF_ERR_INV_NUM("nStates");
                    break;
                }
                if (n<0) {
                    DF_ERR_INV_NUM( (n<0) ? "nStates": "?");
                    break;
                }
                if (ControlName[0] == 0) {
                    DF_ERR("Missing ControlName tag");
                    break;
                }
                CId = AddControl(ControlName, n, X,Y,W,H);
                sim_debug (CP_DF, dptr, "AddControl(Name %s, nStates=%d, X=%d, Y=%d, W=%d, H=%d) returns CId %d\n", ControlName, n, X,Y,W,H, CId);
                if (CId < 0) {
                    DF_ERR("control definition failed");
                    break;
                }
                sim_debug (CP_DF, dptr, "   Total controls defined %d (max %d), total pixels used %d (max %d)\n", 
                    CP.ControlItems, MAX_CONTROLS, CP.SurfaceItems, MAX_PIXELS);
                ControlName[0]=0;
                break;
            case CPDF_TAG_INITIALSTATE: 
                // InitialState=initial state at panel creation
                n = cpdf_num_param(lbuf);
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control");
                    break;
                } 
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Initial State %d\n", 
                    CId,CP.Control[CId].Name, n);
                if (n<0) {
                    DF_ERR_INV_NUM( (n<0) ? "n": "?");
                    break;
                }
                if (n >= CP.Control[CId].nStates) {
                    DF_ERR3("invalid state %d (nStates %d)", n, CP.Control[CId].nStates);
                    break;
                }
                // set state
                SetState(CId, n);
                break;
            case CPDF_TAG_STATEIMG: 
                // StateImg=state number n, pos x, pos y, image name
                // sets the image for control state n. Pixels are taken from image name (previously loaded with LoadImage),
                // at pos X,Y for the defined Width and Heigh of control. If no image filename is given, last image 
                // filemane is used
                n = cpdf_num_param(lbuf);
                X = cpdf_num_param(lbuf);
                Y = cpdf_num_param(lbuf);
                cpdf_str_param(lbuf, Name, 32);
                nImg = -1;
                if (Name[0] == 0) {
                    DF_ERR("Missing image name");
                    break;
                } else {
                    for (i=0;i<CP.ImgItems;i++) {
                        if (cmpstr(CP.Img[i].Name, Name, -1) == 0) continue;
                        nImg = i;
                        break;
                    }
                    if (nImg < 0) {
                        DF_ERR2("cannot found image name %s", Name);
                        break;
                    }
                }
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Def State %d with Image %s using data from X=%d, Y=%d\n", 
                    CId,CP.Control[CId].Name, n, Name[0] ? Name : "(current)",X,Y);
                if ( (n<0) || (X<0) || (Y<0) ) {
                    DF_ERR_INV_NUM( (n<0) ? "n": (X<0) ? "X": (Y<0) ? "Y": "?");
                    break;
                }
                if (CP.Img[nImg].surface == NULL) {
                    DF_ERR("no image");
                    break;
                }
                // copy pixels
                W = CP.Control[CId].W;
                H = CP.Control[CId].H;
                wx = CP.Img[nImg].W;
                wy = CP.Img[nImg].H;
                for (ix=X;ix<X+W;ix++) {
                    for (iy=Y;iy<Y+H;iy++) {
                        if ((ix < wx) && (iy < wy)) {
                            col = CP.Img[nImg].surface[wx * iy + ix]; 
                            if (TransparentColor > 0) {
                                get_surface_rgb_color(col, &rr, &gg, &bb); // get rgb color of image pixel being copied
                                if (TransparentColor == 1) {
                                    // back color is transparent
                                    if ((rr < TransparentValue) && (gg < TransparentValue) && (bb < TransparentValue)) col = 0;
                                } else if (TransparentColor == 2) {
                                    // white
                                    if ((rr > TransparentValue) && (gg > TransparentValue) && (bb > TransparentValue)) col = 0;
                                } else if (TransparentColor == 3) {
                                    // red transparent
                                    if ((rr <= bb) || (rr <= gg)) {
                                        // red color not dominant -> keep pixel
                                    } else if ((bb < 20) && (gg < 20) && (rr < 20)) {
                                        // quasi black color -> keep pixel
                                    } else if ((bb == 0) && (gg == 0)) {
                                        // very dark pure red -> keep pixel 
                                        if (rr > 50) col = 0;
                                    } else if (rr > gg) {
                                        if (rr - gg < TransparentValue) {
                                            // not too much red -> keep pixel
                                        } else if ((gg > 80) && ( (((100*(rr-gg))/gg) < TransparentValue) )) {
                                            // not too much red % -> keep pixel
                                        } else {
                                            col = 0;
                                        }
                                    } else if (rr > bb) {
                                        if (rr - bb < TransparentValue) {
                                            // not too much red -> keep pixel
                                        } else if ((bb > 80) && ( (((100*(rr-bb))/bb) < TransparentValue) )) {
                                            // not too much red % -> keep pixel
                                        } else {
                                            col = 0;
                                        }
                                    }
                                }
                                if (col == 0) {
                                    continue;
                                }
                            }
                            AddControlPixel(CId, n, ix-X, iy-Y, col);
                        };
                    }
                }
                break;
            case CPDF_TAG_STATEIMGMASK: 
                // StateImgMask=state number n, state b as base image, state number m to be used as alpha mask
                // state n image = state b image MASKED with state m as alpha mask
                // if pixel in mask image is white, then the base pixel will be 100% visible, background 0% visible
                // if pixel in mask image is black, then the base pixel will be NOT visible, background fully visible visible
                n = cpdf_num_param(lbuf);
                nBase = cpdf_num_param(lbuf);
                nMask = cpdf_num_param(lbuf);
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Def State %d with alpha channel using data image from state %d as a mask\n", 
                    CId,CP.Control[CId].Name, n, r);
                // apply alpha mask to pixels
                W = CP.Control[CId].W;
                H = CP.Control[CId].H;
                for (ix=0;ix<W;ix++) {
                    for (iy=0;iy<H;iy++) {
                        col = AddControlPixel(CId, nMask, ix, iy, 1);     // get mask pixel
                        get_surface_rgb_color(col, &rr, &gg, &bb);        // get rgb color of mask pixel
                        alpha = (rr + gg + bb) / 3;                       // alpha channel value is the color average
                        col = AddControlPixel(CId, nBase, ix, iy, 1);     // get base pixel
                        if (col==0) {
                            // if base pixel is transparent, leave it transparent
                        } else if (alpha == 0) {
                            // if alpha=0 -> background 100% visible -> make base pixel transparent
                            col=0;
                        } else if (alpha == 255) {
                            // if alpha=255 -> base image pixel is 100% visible
                        } else {
                            get_surface_rgb_color(col, &rr, &gg, &bb);      // get rgb color of base pixel
                            col=surface_rgb_color_alpha(rr, gg, bb, alpha); // apply alpha channel ...
                        }
                        AddControlPixel(CId, n, ix, iy, col);               // and set it as state image pixel
                    }
                }
                break;
            case CPDF_TAG_STATERGB_AREA: 
                // StateRgbArea=state number n, red color, green, blue, pos X, pos Y, Width, Heigh
                // sets the image for control state n. Pixels are set to given r,g,b color (values 0 up to 255)
                // in an area from X,Y for given Width and Heigh. Pixels outside control W and H are ignored
            case CPDF_TAG_STATERGB_CIRCLE: 
                // StateRgbCircle=state number n, red color, green, blue
                // sets the image for control state n. Draw a filled circle with given r,g,b color (values 0 up to 255)
                // up to the defined Width and Heigh of control. If state pixel 0,0 is 0 (transparent) no antialiasing is
                // used. Else, the colour at pixel 0,0 is used to calc the antialiasing colour for circle border
            case CPDF_TAG_STATERGB: 
                // StateRgb=state number n, red color, green, blue
                // sets the image for control state n. Pixels are set to given r,g,b color (values 0 up to 255)
                // to the defined Width and Heigh of control. 
                n = cpdf_num_param(lbuf);
                r = cpdf_num_param(lbuf);
                g = cpdf_num_param(lbuf);
                b = cpdf_num_param(lbuf);
                if (tag == CPDF_TAG_STATERGB_AREA) {
                    X = cpdf_num_param(lbuf);
                    Y = cpdf_num_param(lbuf);
                    W = cpdf_num_param(lbuf);
                    H = cpdf_num_param(lbuf);
                } else {
                    W = CP.Control[CId].W;
                    H = CP.Control[CId].H;
                    X = Y = 0;
                }
                if (r==999) {
                    col = 0;
                } else {
                    col = surface_rgb_color(r,g,b);
                }
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Def State %d with Rgb %d,%d,%d, X=%d, Y=%d, W=%d, H=%d\n", 
                    CId,CP.Control[CId].Name, n, r,g,b,X,Y,W,H);
                if ( (n<0) || (r<0) || (g<0) || (b<0) || (X<0) || (Y<0) || (W<0) || (H<0) ) {
                    DF_ERR_INV_NUM( 
                        (n<0) ? "n": (r<0) ? "r": (g<0) ? "g": (b<0) ? "b": (X<0) ? "X": (Y<0) ? "Y": (W<0) ? "W": (H<0) ? "H": "?");
                    break;
                }
                // set pixels
                if (tag==CPDF_TAG_STATERGB_CIRCLE) {
                    AddCircle(CId, n, W, H, col);
                } else {
                    for (ix=X;ix<X+W;ix++) {
                        for (iy=Y;iy<Y+H;iy++) {
                            AddControlPixel(CId, n, ix, iy, col);
                        }
                    }
                }
                break;
            case CPDF_TAG_STATERGB_TEXT: 
                // StateRgbText=state number n, red color, green, blue, pos X, pos Y, text
                // sets the image for control state n. Text are painted in 5x7 font with given r,g,b color (values 0 up to 255)
                // starting at X,Y (upper left corner of first char). 2 Pixels separation between chars
                if (Font == NULL) break;        // internal font not ready
                n = cpdf_num_param(lbuf);
                r = cpdf_num_param(lbuf);
                g = cpdf_num_param(lbuf);
                b = cpdf_num_param(lbuf);
                X = cpdf_num_param(lbuf);
                Y = cpdf_num_param(lbuf);
                cpdf_str_param(lbuf, Name, 255);
                col = surface_rgb_color(r,g,b);
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Def State %d with Rgb %d,%d,%d Text at X=%d, Y=%d: %s\n", 
                    CId,CP.Control[CId].Name, n, r,g,b,X,Y,Name);
                if ( (n<0) || (r<0) || (g<0) || (b<0) || (X<0) || (Y<0) || (W<0) || (H<0) ) {
                    DF_ERR_INV_NUM( 
                        (n<0) ? "n": (r<0) ? "r": (g<0) ? "g": (b<0) ? "b": (X<0) ? "X": (Y<0) ? "Y": "?");
                    break;
                }
                // draw chars 
                r = 0;
                while((b=Name[r++]) > 0) {
                    b = upcase(b);                   // convert to uppercase
                    if (b > 123) b=b-27;             // because lower case not defined in font
                    if ((b < 32) || (b > 127)) b=31; // sets undef char if any
                    b=b-31;
                    for (iy=0;iy<7;iy++) {
                        g = Font[b*7 + iy];
                        for (ix=0;ix<5;ix++) {
                            if (g & 1) AddControlPixel(CId, n, X+ix, Y+iy, col);
                            g = (g >> 1);
                        }
                    }
                    X += 7;
                }
                break;
            case CPDF_TAG_SAME_BYREF:
                // SameAsByReference   
                // Sets the current control as ByRef. A new control created by XYSameAs= tag based
                // on this one will bot get the pixels copies, but instead will get a reference to 
                // current control pixels. This is the way to have several SameAs controls sharing
                // a copy of pixels images on the generic control instead on having each one its own copy
                // this tag must be afterward nState=
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                CP.Control[CId].ByRef = 1;
                sim_debug (CP_DF, dptr, "Control %d (Name %s): Set ByRef flag\n", 
                    CId,CP.Control[CId].Name);
                break;
            case CPDF_TAG_SAME: 
                // XYSameAs=pos x of control, pos y of control, control name to copy H W nStates MaxH and each state pixels data from
                // sets the image for all control states to be the same as give control (must already extists). 
                // this tag effectivelly creates the control
                X = cpdf_num_param(lbuf);
                Y = cpdf_num_param(lbuf);
                cpdf_str_param(lbuf, Name2, 32);
                TransparentColor = 0; // reset transparet color
                sim_debug (CP_DF, dptr, "Same As X=%d, Y=%d, Control Name to copy %s\n", X,Y,Name2);
                if ( (X<0) || (Y<0) ) {
                    DF_ERR_INV_NUM( 
                        (X<0) ? "X": (Y<0) ? "Y": "?");
                    break;
                }
                OrgCId = GetByName(Name2);
                if (OrgCId < 0) {
                    DF_ERR("cannot found same control");
                    break;
                } else if (OrgCId >= CARRAY_START) {
                    DF_ERR("cannot use a control array");
                    break;
                } 
                if (ControlName[0] == 0) {
                    DF_ERR("Missing ControlName tag");
                    break;
                }
                X += BaseX; Y += BaseY; // add BaseXY offset 
                // create control like SameAs one. 
                n = CP.Control[OrgCId].nStates;
                W = CP.Control[OrgCId].W;
                H = CP.Control[OrgCId].H;
                nTick = CP.Control[OrgCId].nTick;
                save_SurfaceItems = CP.SurfaceItems;
                CId = AddControl(ControlName, n, X,Y,W,H);
                sim_debug (CP_DF, dptr, "AddControl(Name %s, nStates=%d, X=%d, Y=%d, W=%d, H=%d) returns CId %d\n", 
                    ControlName, n, X,Y,W,H, CId);
                if (CId < 0) {
                    DF_ERR("control definition failed");
                    break;
                } else {
                    // copy MaxH
                    CP.Control[CId].MaxH = CP.Control[OrgCId].MaxH;
                    if (CP.Control[OrgCId].ByRef) {             // OrgCId y ByRef -> do not copy pixels from it, but instead juts let CId_iPos point to OrgCId_iPos
                        CP.SurfaceItems = save_SurfaceItems;    // restore SurfaceItems to free pixels space 
                        CP.Control[CId].iPos_surface = CP.Control[OrgCId].iPos_surface;
                    }
                    sim_debug (CP_DF, dptr, "   Total controls defined %d (max %d), total pixels used %d (max %d)\n", 
                        CP.ControlItems, MAX_CONTROLS, CP.SurfaceItems, MAX_PIXELS);
                    if (CP.Control[OrgCId].ByRef) {    
                        // ByRef -> no pixels to copy
                    } else {
                        // copy pixels
                        n = CopyControlPixels(OrgCId, CId);
                        if (n < 0) {
                            DF_ERR("copy pixels failed");
                            break;
                        }
                    }
                    if (nTick >= 0) {
                        // origin control has a tick count. Get the destination own's control tick count
                        nTick = GetTickCountItem(); 
                        if (nTick < 0) {
                            DF_ERR("get tick count failed");
                            break;
                        }
                        CP.Control[CId].nTick = nTick;
                    }
                }
                ControlName[0]=0;
                break;
            case CPDF_TAG_HEIGHT:
                // Height = n
                // set the control height to given value

                H = cpdf_num_param(lbuf);
                sim_debug (CP_DF, dptr, "Height H=%d\n", H);
                if ( (H<0) ) {
                    DF_ERR_INV_NUM( 
                        (H<0) ? "H": "?");
                    break;
                }
                if (CId < 0) {
                    DF_ERR("Invalid Control Id");
                    break;
                } 
                CP.Control[CId].MaxH = H;
                break;
            case CPDF_TAG_TRANSPARENT:
                // TransparentColor = W | B | R , n
                // this tag allows subsequent StateImg(s) to replace White/black/red pixels by transparent color
                // if W, tranparent pixel is the one that has r,g,b > n,n,n 
                // if B, tranparent pixel is the one that has r,g,b < n,n,n
                // if R, tranparent pixel is the one that has r-g >n ,r-b > n
                // resets on nState or XYSameAs 
                cpdf_str_param(lbuf, Name, 32);
                n = cpdf_num_param(lbuf);
                if ((Name[0] == 'b') || (Name[0] == 'B')) {TransparentColor = 1;} else
                if ((Name[0] == 'w') || (Name[0] == 'W')) {TransparentColor = 2;} else 
                if ((Name[0] == 'r') || (Name[0] == 'R')) {TransparentColor = 3;} else {
                    DF_ERR("Transparent color needs to be set to B (black), W (white) or R (red)");
                    break;
                }
                if (n<0) {
                    DF_ERR_INV_NUM( 
                        (X<0) ? "X": "?");
                    break;
                }
                TransparentValue = n;
                sim_debug (CP_DF, dptr, "TransparentColor %s, TransparentValue=%d\n", Name, TransparentValue);
                break;
            case CPDF_TAG_AUTOSTATES:
                // AutoStates   requieres nStates == 2
                // Creates (nStates-2) states from State 2 to nStates -1 with pixels being of color 
                // at intermedite between state 0 and state 1. Used to create intermedite intensities
                // for register lights/leds. Gets a tickcount new entry
                // create control like SameAs one. 
                // this tag must be afterward nState=
                if ((CId < 0) || (CId >= MAX_CONTROLS)) {
                    DF_ERR("invalid Control Id");
                    break;
                } 
                n = CP.Control[CId].nStates;
                if (n <= 2) {
                    DF_ERR("AutoStates requires nStates > 2");
                    break;
                }
                for (i=2;i<n;i++) InterpolateControlPixels(CId, i);
                nTick = GetTickCountItem(); 
                if (nTick < 0) {
                    DF_ERR("get tick count failed");
                    break;
                }
                CP.Control[CId].nTick = nTick;
                break;
            case CPDF_TAG_CARRAY: 
                // ControlArrayName=name of control array
                cpdf_str_param(lbuf, Name, 32);
                CArrayId = AddCArray(Name);
                sim_debug (CP_DF, dptr, "AddControlArray(Name %s) returns CArrayId %d\n", Name, CArrayId);
                if (CArrayId < 0) {
                    DF_ERR("control array definition failed");
                    break;
                }
                break;
            case CPDF_TAG_CARRAYITEM: 
                // ControlArrayItems=name of control item 0 in array, name of control 1, ...
                // controls must be previously defined
                if (CArrayId < CARRAY_START) {
                    DF_ERR("control array not set");
                    break;
                } 
                while (1) {
                    cpdf_str_param(lbuf, Name, 32);
                    if (Name[0] == 0) break;
                    CId = GetByName(Name);
                    if (CId < 0) {
                        DF_ERR2("control %s not found", Name);
                        break;
                    }
                    if (CP.Control[CId].nTick >= 0) {
                        // single control in control array has tickcount set 
                        // whole control array (if not already set) tickcount
                        if (CP.CArray[CArrayId - CARRAY_START].nTick < 0) {
                            nTick = GetTickCountItem(); 
                            if (nTick < 0) {
                                DF_ERR("get tick count failed");
                                break;
                            }
                            CP.CArray[CArrayId - CARRAY_START].nTick = nTick;
                        }
                        // remove tickcount from single control within control array
                        nTick = CP.Control[CId].nTick;
                        CP.Control[CId].nTick = -1;
                        CP.TickCount[nTick].Num = -1;
                    }
                    AddCArrayItem(CArrayId, CId);
                    sim_debug (CP_DF, dptr, "AddControl %d (Name %s) to Control Array %d (Name %s)\n", 
                        CId, CP.Control[CId].Name, CArrayId, CP.CArray[CArrayId-CARRAY_START].Name );
                }
                sim_debug (CP_DF, dptr, "   Total items in control array %d\n", 
                    CP.CArray[CArrayId-CARRAY_START].Items);
                break;
            case CPDF_TAG_DEFOPT:
            case CPDF_TAG_UNDEFOPT:
                // defopt=name         -> add name to options
                // defopt=name1/name2  -> add name1/name2 to options, replacion name1/whatever
                // undefopt=name       -> remove name (or name/whatever)
                cpdf_str_param(lbuf, Name, 32);
                if ((Name == NULL) || (Name[0] == 0)) {
                    DF_ERR("Missing option name in [un]defopt");
                    break;
                }
                sim_debug (CP_DF, dptr, "remove option %s\n", Name);
                RemoveOption(Name);
                if (tag == CPDF_TAG_DEFOPT) {
                    sim_debug (CP_DF, dptr, "add option %s\n", Name);
                    n=AddOption(Name);
                    if (n<0) {
                        DF_ERR2("Cannot Add Option: %s", Name);
                        break;
                    }
                }
                break;
            case CPDF_TAG_IFOPT:
            case CPDF_TAG_IFNOPT:
                // if[n]opt=name   or  name1/name2
                // continue if name is defined/not defiend in options array. if not, skip lines until else tag
                // ifopt and ifnopt cannot be nested
                cpdf_str_param(lbuf, Name, 32);
                if ((Name == NULL) || (Name[0] == 0)) {
                    DF_ERR("Missing option name in if[n]opt");
                    break;
                }
                if (ifopt > 0) {
                    DF_ERR("if[n]opt cannot be nested");
                    break;
                }
                n = IsOptionOrOptionParam(Name, 0); 
                if ((tag == CPDF_TAG_IFOPT) && (n != 0)) {
                    ifopt = 1; // if evaluates to true ... execute, then skip on next elseif or else, up to endif
                    sim_debug (CP_DF, dptr, "ifopt|elseif %s evaluates to true (option set)\n", Name);
                } else if ((tag == CPDF_TAG_IFNOPT) && (n == 0)) {
                    ifopt = 1; // if evaluates to true ... execute, then skip on next elseif or else, up to endif
                    sim_debug (CP_DF, dptr, "ifnopt %s evaluates to true (option not set)\n", Name);
                } else {
                    ifopt = 2; // if evaulates to false ... skip until else|elseif|endif
                    sim_debug (CP_DF, dptr, "if[n]opt|elseif %s evaluates to false\n", Name);
                }
                break;
            case CPDF_TAG_ELSEIF:
                if (ifopt == 0) {     
                    DF_ERR("elseif without if");
                    break;
                } else if (ifopt == 1) {     // if true ... elseif -> then skip to next endif
                    ifopt = 3;        // skip until endif
                    lbuf[0] = 0;      // ignore rest of line
                    sim_debug (CP_DF, dptr, "elseif found: skip until endif\n");
                } else if (ifopt == 2) {     // if false ... elseif -> evaluate
                    ifopt = 0;
                    tag = CPDF_TAG_IFOPT;
                    sim_debug (CP_DF, dptr, "elseif found evaluate opt again\n");
                    goto process_tag;           // ugly but very handly
                }
                break;
            case CPDF_TAG_ELSE:
                if (ifopt == 0) {     
                    DF_ERR("else without if");
                    break;
                } else if (ifopt == 1) {     // if true ... else -> then skip to endif
                    ifopt = 3;        // skip until endif
                    lbuf[0] = 0;      // ignore rest of line
                    sim_debug (CP_DF, dptr, "else found: skip until endif\n");
                } else if (ifopt == 2) {     // if false ... else -> execute until elseif|else|endif
                    ifopt = 1; 
                }
                break;
            case CPDF_TAG_ENDIF:
                if (ifopt == 0) {
                    DF_ERR("endif without if");
                    break;
                }
                ifopt = 0;
                break;
            case CPDF_TAG_END:
                // end processing of DF file
                sim_debug (CP_DF, dptr, "end tag marks end of input\n");
                ifopt = 10;
                break;
            case CPDF_TAG_ECHO:
                // echo=text to console
                cpdf_str_param(lbuf, Name, 255);
                for(i=0;Name[i];i++) sim_putchar(Name[i]);
                sim_putchar(13); sim_putchar(10); 
                break;
            default: 
                DF_ERR2("unknown tag: %s", lbuf);
                lbuf[0] = 0; // clear line
                break;
        }
        if (lbuf[0]) {
            DF_ERR2("unexpected data: %s", lbuf);
        }
    }
    fclose (file);
    // adjust control panel type
    if (cpanel_TypeId < 0) {
        // given panel type not defined -> set to 0 to prevent GUI open
        cpanel_TypeId = 0;
    } else if (cpanel_TypeId == 0) {
        // no panel type is set in definition file, use 1 as default
        cpanel_TypeId = 1;
    }
    // resize CP.Surface to its actual size
    CP.surface = (uint32 *)realloc (CP.surface, (CP.SurfaceItems+1)*sizeof(*CP.surface));
    // free Images 
    for (i=0;i<CP.ImgItems;i++) 
        if (CP.Img[i].surface) {
            free(CP.Img[i].surface);
            CP.Img[i].surface = NULL; 
        }
    sim_debug (CP_DF, dptr, "Total Defined in DF\n");
    sim_debug (CP_DF, dptr, "   Single Controls     : %d (Max %d)\n", CP.ControlItems, MAX_CONTROLS);
    sim_debug (CP_DF, dptr, "   Control Arrays      : %d (Max %d)\n", CP.CArrayItems, MAX_CARRAYS);
    sim_debug (CP_DF, dptr, "   Loaded Images       : %d (Max %d)\n", CP.ImgItems, MAX_IMAGES);
    sim_debug (CP_DF, dptr, "   Control State Pixels: %d (Max %d)\n", CP.SurfaceItems, MAX_PIXELS);
    n = 0; for (i=0;i<CP.TickCountItems;i++) if (CP.TickCount[i].Num >= 0) n++;
    sim_debug (CP_DF, dptr, "   Tick Count used     : %d (Max %d)\n", n, MAX_TICK_COUNT);
    sim_debug (CP_DF, dptr, "Options defined at end of DF processing:\n");
    sim_debug (CP_DF, dptr, "   %s\n", CP.Options);
    sim_debug (CP_DF, dptr, "End of DF processing:\n");
}

extern int   icon_rgb_defined; // flag: when 1 -> icon defined
extern int16 icon_r[32*32];    // holds icon bitmap in RGB (0..255, -1 for transparent)
extern int16 icon_g[32*32];
extern int16 icon_b[32*32];

void ControlPanel_icon(void)
{
    int i, p, h, w, x0, y0, x1, y1, x, y, r, g, b;
    int Id; 
    uint32 col;

    // init icon bitmap to transparent
    icon_rgb_defined = 0;
    for (i=0;i<32*32; i++) icon_r[i] = icon_g[i] = icon_b[i] = -1; 
    // if control defined with name "icon", use if as bitmap image for icon
    Id = GetByName("icon");
    if (Id < 0) return;
    // copy pixels to icon rgb bitmap (max 32 x 32)
    icon_rgb_defined = 1;
    h = CP.Control[Id].H; 
    w = CP.Control[Id].W; 
    x0 = y0 = 0;
    if (w < 32) x0 = (32 - w) / 2; // center image
    if (h < 32) y0 = (32 - h) / 2;
    for (y=0;y<h;y++) {
        y1 = y0 + y;
        if (y1 >= 32) break;
        for (x=0;x<w;x++) {
           x1 = x0 + x; 
           if (x1 >= 32) break;
           p = CP.Control[Id].iPos_surface + x + y * w;
           col = CP.surface[p];
           if (col == 0) continue; // transparent pixel
           get_surface_rgb_color(col, &r, &g, &b);
           icon_r[x1+ 32 * y1] = r; 
           icon_g[x1+ 32 * y1] = g; 
           icon_b[x1+ 32 * y1] = b; 
        }
    }
}

// bind loaded control panel (or internal one) to SimH variables and event handling routines
void ControlPanel_Bind(CP_TYPE *cp_type, UNIT *uptr, DEVICE *dptr) 
{
    int i, n;
    int *Id;
    CP_DEF *cp_def;
    CP_CALLBACK CP_Init_CallBack = NULL;                        // Control Panel init Callback

    // set the unit and dev pointers
    cp_unit_ptr = uptr;                                         // pointer to control panel unit and device
    cp_dev_ptr = dptr;
    // load the resources from attached file and creates the controls & carrays
    ControlPanelLoad(cp_type, uptr, dptr);
    if (cpanel_TypeId < 1) return; 
    // locate the cp_def to use based on control panel type set in definition file
    cp_def = NULL;
    for (n=0;;n++) {
        if (cp_type[n].Name == NULL) break;
        if (cpanel_TypeId == cp_type[n].Id) {                   // control panel type found
            cp_def = cp_type[n].cp_def;                         // get the control mapping for this type
            CP.CP_Refresh = cp_type[n].Refresh_CallBack;        // set the refresh callback
            CP_Init_CallBack = cp_type[n].Init_CallBack;        // set the init callback
            CP.CP_Reset = cp_type[n].Reset_CallBack;            // set the reset callback
            CP.CP_Done = cp_type[n].Done_CallBack;              // set the done callback
            CP.CP_TickIntensityCount = cp_type[n].TickIntensity_CallBack;
            break;
        }
    }
    if (cp_def == NULL) {
        cpanel_TypeId = 0;
        return; 
    }
    // bind the variables and callbacks given in cp_def with created controls
    i = -1;
    while(Id=cp_def[++i].IdVar) {
        n = cp_def[i].UsedByType;   // this control is used by this cpanel type
        if (n > 0) {                // = 0 -> used by any type. >0 -> must match cpanel_TypeId
            if ((cpanel_TypeId & n) == 0) {
                *Id = -1; // control not bind
                continue;
            }
        }
        if (cp_def[i].OptName) {
            // this control is used only if OptName option set either by SET OPTION OptName scp command, 
            // or by DefOpt=OptName on definition file 
            if (IsOptionOrOptionParam(cp_def[i].OptName, 0)==0) {
                // not exactly match, so control not bind
                *Id = -1; // control not bind
                continue;
            }
        };
        *Id = GetByName(cp_def[i].Name);
        if (*Id < 0) {
            fprintf(stderr, "control %s not found. ControlPanel_Bind failed\n", cp_def[i].Name);
        } else if (*Id < CARRAY_START) {    
            // single control OnClick event
            CP.Control[*Id].OnClick = cp_def[i].CallBack;   
        } else {   
            // control array OnClick event
            CP.CArray[*Id - CARRAY_START].OnClick = cp_def[i].CallBack;    
        }
    };
    // process icon image
    ControlPanel_icon();
    // call the init routine for the control panel loaded
    cpanel_on = cpanel_TypeId; 
    if (CP_Init_CallBack) CP_Init_CallBack();
	// but then set again to zero. Will be set again is GUI window is created without errors
	cpanel_on = 0;
}

void ControlPanel_Refresh(void);
void ControlPanel_init(DEVICE *dptr)
{
    int wx, wy, xSize, ySize, xPos, yPos; 
    char buf[128]; 

    if (CP.ControlItems < 1) {
        fprintf(stderr, "No controls loaded. Cannot init Control Panel GUI window\n");
        return;
    }
    if (vid_active) {
        fprintf(stderr, "Cannot display Control Panel. GUI Window already in use: %s\n", vid_title);
        return;
    }
    // first control is backgound and sets the control panel size
    wx = CP.Control[0].W;
    wy = CP.Control[0].H;
    if (!cpvid_init(cpanel_name, wx, wy, dptr )) {
        fprintf(stderr, "Control Panel GUI window initialization failed\n");
        return;
    }
    sim_debug (CP_CMDS, cp_dev_ptr, "GUI window created Title: %s\n", vid_title);
    xPos=90; yPos=50;
    sim_debug (CP_CMDS, cp_dev_ptr, "GUI Initial Pos x=%d, y=%d \n", xPos, yPos);
    vid_SetWindowSizeAndPos(1,xPos,yPos);
    if (CP.Scale != 100) {
        xSize = wx * CP.Scale / 100;
        ySize = wy * CP.Scale / 100;
        sim_debug (CP_CMDS, cp_dev_ptr, "GUI initial Size %d x %d \n", xSize, ySize);
        vid_SetWindowSizeAndPos(2, xSize, ySize);
    }
    sprintf(buf, "%s (Scale %d%%)", cpanel_name, CP.Scale);
    vid_set_title(buf);
    cpanel_on = cpanel_TypeId;  
    cpanel_interactive = 0;
    CP.LastTickCountNum = 0;
    CP.FullRedrawNeeded=1;
    CP.FullSurfaceRedrawTm0=0;
    RectList.MaxCount = RECTLIST_MAX;
    ControlPanel_Refresh();
}

void ControlPanel_done(void)
{    
    sim_debug (CP_CMDS, cp_dev_ptr, "GUI window close requested\n");
    if ((cpanel_on) && (CP.CP_Done != NULL)) CP.CP_Done();
    cpanel_on = 0; 
    cpanel_interactive = 0;
    cpvid_shutdown();
    if (CP.surface) free(CP.surface);
    CP.surface = NULL; 
    FontLoad(0); // unload internal font
    sim_debug (CP_CMDS, cp_dev_ptr, "GUI window closed\n");
}
// load and pop-up GUI window
// power = 0 or 1 to create/destry the GUI window
void ControlPanel_Load(int power, 
                       CP_TYPE *cp_type,  
                       UNIT *uptr, DEVICE *dptr)
{
    if (power) { 
        // power on control panel. 
        if (cpanel_on) {
            ControlPanel_Refresh();
            return;  // cpanel already there. nothing to do. just refresh
        }
        ControlPanel_Bind(cp_type, uptr, dptr); 
        // if control panel type still undefined, do not load the control panel
        if (cpanel_TypeId == 0) 
            return; 
        ControlPanel_init(dptr);
        // ControlPanel_init sets cpanel_on variable to cpanel_TypeId on success (0 if fail)
    } else {
        // ControlPanel_done sets cpanel_on variable to 0
        if (cpanel_on) {
            ControlPanel_done();
        }
    }
}

// set/get state from single control/control array
// if Id < 0 return state. Else sets state
// for control arrays, sets up to 64 items as Set(item n, bit n from State)
// this allows to move at once lights or switches in a control array
// for control arrays, returns bit n at returnes State set if bit0 for item n set
t_uint64 SetGetState(int Id, t_uint64 State, char SetGetFlag);
t_uint64 GetState(int Id)                 { return (Id < 0) ? 0 : SetGetState(Id, 0,     'G');}
t_uint64 SetState(int Id, t_uint64 State) { return (Id < 0) ? 0 : SetGetState(Id, State, 'S');}
t_uint64 SetGetState(int Id, t_uint64 State, char SetGetFlag)
{
    int bGetFlag = (SetGetFlag == 'G');
    if (Id < 0) return 0;
	if (Id < CARRAY_START) {    
        // single control 
        if (CHK_CId(Id) < 0) return -1;                                // sanity check
        if (bGetFlag) {
            // get and return state
            return CP.Control[Id].State;
        } else {
            // set state
            if (State >= CP.Control[Id].nStates) return -1;
            if (State < 0) return -1;
            if (CP.Control[Id].State != (int) State) {
                CP.Control[Id].State = (int) State;
                cpanel_State_has_changed = 1;                   // the state set was not the same the control had
            } else {
                cpanel_State_has_changed = 0;
            }
            if (cpanel_ControlRedrawNeeded)  {     
                // asure that next SetState call will update the control on GUI screen even if state set is same that the control already had
                CP.Control[Id].LastState = -1;
                cpanel_ControlRedrawNeeded = 0;
            }
            // clear auto next state
            CP.Control[Id].AutoNext.State = -1; 
            return 0;
        }
    } else {   
        // set control array 
        int i, CId, CId0;
        CHK_CArrayId(Id)
        if (bGetFlag) {
            State = 0;
            for (i=CP.CArray[Id - CARRAY_START].Items-1;i>=0;i--) {
                CId0 = CP.CArray[Id - CARRAY_START].CId0;
                CId = CP.CArrayCidItems[CId0 + i];
                State = (State << 1) | (CP.Control[CId].State & 1);
            }
            return State;
        } else {
            cpanel_State_has_changed = 0;
            for (i=0;i<CP.CArray[Id - CARRAY_START].Items;i++) {
                CId0 = CP.CArray[Id - CARRAY_START].CId0;
                CId = CP.CArrayCidItems[CId0 + i];
                if (CP.Control[CId].State != (int) (State & 1)) cpanel_State_has_changed = 1;
                CP.Control[CId].State = (int) State & 1;
                if (cpanel_ControlRedrawNeeded) CP.Control[CId].LastState = -1;
                State = State >> 1;
            }
            cpanel_ControlRedrawNeeded = 0;
            return 0;
        }
    }
}
// return the control id for n-th item in control array 
int GetCArrayCId(int CArrayId, int n) 
{
    int CId0, CId;
    CHK_CArrayId(CArrayId) 
    CHK_Item(CArrayId,n)
    CId0 = CP.CArray[CArrayId - CARRAY_START].CId0;
    CId = CP.CArrayCidItems[CId0 + n];
    return CId;
}

// schedulle state change for Id (array or single control) 
// state will be set automatically by refresh routine to NextState after Duration_msec has passed
int SetStateAutoNext(int Id, int NextState, uint32 Duration_msec)
{
    if (Id < 0) {
        fprintf(stderr, "Cannot Get AutoNext %d\n", Id);
        return -1;
    } else if (Id >= CARRAY_START) {    
        fprintf(stderr, "Cannot Set AutoNext for arrays %d\n", Id);
        return -1;
    }
    // single control 
    if (CHK_CId(Id) < 0) return -1;                                // sanity check
    if (NextState >= CP.Control[Id].nStates) return -1;
    // set next state
    CP.Control[Id].AutoNext.State = NextState;
    CP.Control[Id].AutoNext.Duration_msec = Duration_msec;
    CP.Control[Id].AutoNext.Time0_msec = sim_os_msec();
    return 0;
}

extern SDL_Window *vid_window;                                 /* window handle */
extern SDL_Renderer *vid_renderer;

int Into_Refresh = 0; // flag to avoid recursive call to ControlPanel_Refresh()

void DoActionSetScale(void)
{
    char buf[128];

    int xSize = CP.Control[0].W * CP.Scale / 100;
    int ySize = CP.Control[0].H * CP.Scale / 100;
    sim_debug (CP_CMDS, cp_dev_ptr, "GUI set Scale %d to Size %d x %d \n", CP.Scale, xSize, ySize);
    vid_SetWindowSizeAndPos(2, xSize, ySize);
    sprintf(buf, "%s (Scale %d%%)", cpanel_name, CP.Scale);
    vid_set_title(buf);
}

void DoActionPrintFreeControls(void)
{
    int i,n; 

    sim_printf("Total Defined:\n");
    sim_printf("   Single Controls     : %d (Max %d)\n", CP.ControlItems, MAX_CONTROLS);
    sim_printf("   Control Arrays      : %d (Max %d)\n", CP.CArrayItems, MAX_CARRAYS);
    sim_printf("   Loaded Images       : %d (Max %d)\n", CP.ImgItems, MAX_IMAGES);
    sim_printf("   Control State Pixels: %d (Max %d)\n", CP.SurfaceItems, MAX_PIXELS);
    n = 0; for (i=0;i<CP.TickCountItems;i++) if (CP.TickCount[i].Num >= 0) n++;
    sim_printf("   Tick Count used     : %d (Max %d)\n", n, MAX_TICK_COUNT);
}


// If Action = 0 -> calls the OnClick event of given single control/control array name
// If Action = 1 -> Signal with a mark on GUI the clickable area of given single control/control array name
// If Action = 2 -> printf info on given single control/control array name
// If Action = 3 -> set control, state
// If Action = 4 -> set scale
int DoActionByName(char * Name, int Action)
{
    int Id, i, CId0, CId, n, Mark; 

    if (Action == 0) {
        // calls the OnClick event of given single control
        Id = GetByName(Name);
        if (Id < 0) return -1; // name not found
        if (Id >= CARRAY_START) return -1; // cannot press on control arrays, only on single controls
        DoClickEvent(Id);
        return 0;
    }
    if (Action == 1) {
        // mark on GUI the clickable area of given single control/control array name
        if (cmpstr(Name, "<NONE>",0)) {
            // unmark all
            for (i=0; i<CP.ControlItems;i++) CP.Control[i].Mark = 0;
        } else if (cmpstr(Name, "<ALL>",0)) {
            // mark all single controls/control arrays with OnClick handler
            for (i=0; i<CP.ControlItems;i++) {
                if (i == 0) continue;                           // control zero is background. Skip
                if (CP.Control[i].OnClick == NULL) continue;    // no OnClick handler: cannot be clicked, so is no Marked
                CP.Control[i].Mark = 1;
            }
            // mark controls for all control arrays with OnClick handler
            for (i=0; i<CP.CArrayItems;i++) {
                if (CP.CArray[i].OnClick == NULL) continue;     // no OnClick handler: cannot be clicked, so is no Marked
                DoActionByName(CP.CArray[i].Name, 1);           // now mark all controls for this CArray
            }
        } else if ((Id = GetByName(Name)) < 0) {
            // name not found
            return -1;   
        } else if (Id < CARRAY_START) {
            // Name is a single control, Mark a single control
            CP.Control[Id].Mark = 1;
        } else {
            // Name is a control array, Mark each control in array
            CId0 = CP.CArray[Id - CARRAY_START].CId0;
            for (i=0;i<CP.CArray[Id - CARRAY_START].Items;i++) {
                CId = CP.CArrayCidItems[CId0 + i];
                CP.Control[CId].Mark = 1;
            }
        }
        CP.FullRedrawNeeded=1; // redraw the whole screen with updated marks
        if ((Into_Refresh == 0) && (cpanel_on)) {
            ControlPanel_Refresh();
        }
        return 0;
    }
    if (Action == 2) {
        // printf info on given single control/control array name
        if (cmpstr(Name, "<ALL>",0)) {
            // print all 
            for (i=0; i<CP.ControlItems;i++) DoActionByName(CP.Control[i].Name, 2);
            for (i=0; i<CP.CArrayItems;i++)  DoActionByName(CP.CArray[i].Name, 2);
            // print used and free controls
            DoActionPrintFreeControls();
            sim_printf("Options Defined:\n");
            sim_printf("   %s\n", CP.Options);
        } else if (cmpstr(Name, "<FREE>",0)) {
            DoActionPrintFreeControls();
        } else if ((Id = GetByName(Name)) < 0) {
            // name not found
            return -1;   
        } else if (Id < CARRAY_START) {
            // Name is a single control, printf definition
            sim_printf("ControlName=%s (Id=%d)\n", CP.Control[Id].Name, Id);
            if (CP.Control[Id].X < 0) {
                sim_printf("   WH=%d,%d\n", CP.Control[Id].W, CP.Control[Id].H);
            } else {
                sim_printf("   XYWH=%d,%d,%d,%d\n", CP.Control[Id].X, CP.Control[Id].Y, CP.Control[Id].W, CP.Control[Id].H);
            }
            if (CP.Control[Id].MaxH > 0) {
                sim_printf("   Heigth=%d\n", CP.Control[Id].MaxH);
            }
            sim_printf("   nStates=%d\n", CP.Control[Id].nStates);
            sim_printf("   ;   Current State: %d\n", CP.Control[Id].State);
            if (CP.Control[Id].OnClick) {
                sim_printf("   ;   OnClick Event defined\n");
            }
        } else {
            // Name is a control array, printf its control's names
            sim_printf("ControlArrayName=%s (Id=%d)\n", CP.CArray[Id - CARRAY_START].Name, Id);
            sim_printf("   ;   Number of Items: %d\n", CP.CArray[Id - CARRAY_START].Items);
            if (CP.CArray[Id - CARRAY_START].OnClick) {
                sim_printf("   ;   OnClick Event defined\n");
            }
            CId0 = CP.CArray[Id - CARRAY_START].CId0;
            for (i=0;i<CP.CArray[Id - CARRAY_START].Items;i++) {
                CId = CP.CArrayCidItems[CId0 + i];
                if ((i & 3) == 0) {
                    if (i>0) sim_printf("\n");
                    sim_printf("   ControlArrayItems=");
                } else {
                    sim_printf(", ");
                }
                sim_printf("%s", CP.Control[CId].Name);
            }
            sim_printf("\n");
        }
        return 0;
    }
    if (Action == 3) {
        // If Action = 3 -> set control name, state
        for(i=0;((Name[i] != '/') && (Name[i] != ' ') && (Name[i] != 0));i++);
        if (Name[i] == 0) return 0; // no state given
        Name[i] = 0;
        n = 0;
        for(i=i+1;Name[i];i++) {
            if ((Name[i] >= '0') && (Name[i] <= '9')) n = n * 10 + Name[i] - '0';
            if (Name[i] == '*') {n = -1; break;}
        }
        Mark = (n < 0) ? 0 : n | 256; // to force the state drawing via .Mark 
        if (cmpstr(Name, "<ALL>",0)) {
            for (Id=0; Id<CP.ControlItems;Id++) if (CP.Control[Id].nStates > n) CP.Control[Id].Mark = Mark;
        } else if ((Id = GetByName(Name)) < 0) {
            // name not found
            return -1;   
        } else if (Id < CARRAY_START) {
            // Name is a single control, Mark a single control
            if (CP.Control[Id].nStates > n) CP.Control[Id].Mark = Mark;
        } else {
            // Name is a control array, Mark each control in array
            CId0 = CP.CArray[Id - CARRAY_START].CId0;
            for (i=0;i<CP.CArray[Id - CARRAY_START].Items;i++) {
                CId = CP.CArrayCidItems[CId0 + i];
                if (CP.Control[CId].nStates > n) CP.Control[CId].Mark = Mark;
            }
        }
        CP.FullRedrawNeeded=1; // redraw the whole screen with updated marks
        if ((Into_Refresh == 0) && (cpanel_on)) {
            ControlPanel_Refresh();
        }
        return 0;
    }
    if (Action == 4) {
        // If Action = 4 -> set scale
        DoActionSetScale();
        return 0;
    }
    return -1;  // unknow action
}

// set/get control panel current scale
// if param = 0 -> return current scale value
// if param > 0 -> set GUI scale factor (100 -> full size, 50% -> half size) to this value
int cpanel_scale(int scale)
{
    if (scale == 0) return CP.Scale; 
    CP.Scale=scale; 
    DoActionSetScale();
    return 0;
}

// check hot keys active when cpanel GUI has focus, or when console has focus during set cpanel interactive execution
// c is char pressed (when key released), current_key is char being pressed down)
// return 1 if hotkey c processed
int CheckHotKeys(unsigned char c)
{
    int i, bMark; 

    if (c == sim_int_char) {
        // Control-E (^E = WRU) pressed while GUI has focus
        cpanel_stop_flag = 1; 
        return 1;
    } 
    if (c == ('T'-'A'+1)) {
        // Control-T -> toggle mark on GUI all clickable areas (^T)
        bMark = 0;
        for (i=0; i<CP.ControlItems;i++) if (CP.Control[i].Mark) bMark = 1;
        if (bMark == 0) {
            DoActionByName("<ALL>", 1);
        } else {
            DoActionByName("<NONE>", 1);
        }
        return 1;
    } 
    if (c == 'Y'-'A'+1) {
        // Control-Y -> toggle GUI window scale to 100% <-> 50% (^Y)
        CP.Scale = (CP.Scale == 100) ? 50 : 100;
        DoActionSetScale();
        return 1;
    }
    if (c == '+') {
        // '+' -> Increase scale +10% up to 200%
        CP.Scale += 10;
        if (CP.Scale > 200) CP.Scale = 200;
        DoActionSetScale();
        return 1;
    }
    if (c == '-') {
        // '-' -> decrease scale -10% up to 10%
        CP.Scale -= 10;
        if (CP.Scale < 10) CP.Scale = 10;
        DoActionSetScale();
        return 1;
    }
    return 0;
}

// calls the OnClick event of control/array on top at coords PosX,PosY
// if KeyPress_KeyRelease = 1 -> key press, =2 -> key release
// returns 0 if no clickable control at PosX/PosY. 
// returns 1 if there is one clickable control at PosX/PosY. 
// if called with KeyPress_KeyRelease = 0 -> just returns 0/1 but does not call OnClick event
int ProcessClickEventAtXY(int PosX, int PosY, int KeyPress_KeyRelease) 
{
    int i,j,X,Y,W,H, CId0, CId, bClickbleControlAtPosXY;

    bClickbleControlAtPosXY=0;
    // init clicked item data
    CP_Click.KeyPress_KeyRelease = KeyPress_KeyRelease; 
    CP_Click.X = PosX;          // click coords relative to GUI Panel Window
    CP_Click.Y = PosY;
    CP_Click.CId = -1;          // no control clicked
    CP_Click.CArrayId = -1;     // no array clicked
    CP_Click.CArrayItem = -1;   // no array item clicked
    CP_Click.OfsX = -1;         // click coords relative to control origin
    CP_Click.OfsY = -1;
    // determine the control/array clicked top (this is the last one)
    for(i=0;i<CP.ControlItems;i++) {
        X = CP.Control[i].X;    // get control boundaries
        Y = CP.Control[i].Y;
        W = CP.Control[i].W;
        H = CP.Control[i].H;
        if (X == -1) continue;   // if X == -1 control is not sensed
        if ((PosX < X) || (PosY < Y)) continue; // because click outside control
        X = PosX - X; 
        Y = PosY - Y; 
        if ((W < X) || (H < Y)) continue;  // because click outside control
        // control clicked. Set in OfsX/Y the click pos relative to controls origin
        CP_Click.CId = i;
        CP_Click.OfsX = X;
        CP_Click.OfsY = Y;
    }
    // determine if control clicked belongs to an array 
    CP_Click.CArrayId = -1;  // no array clicked
    CP_Click.CArrayItem = -1; // no array item clicked
    for(i=0;i<CP.CArrayItems;i++) {
        for (j=0;j<CP.CArray[i].Items;j++) {
            CId0 = CP.CArray[i].CId0;
            CId = CP.CArrayCidItems[CId0 + j];
            if (CP_Click.CId == CId) {
                CP_Click.CArrayItem = j;
                CP_Click.CArrayId = i + CARRAY_START;
                break;
            }
        }
        if (CP_Click.CArrayItem >= 0) break;
    }
   // if array clicked then call array event handler (if any)
   // else call the control event handler (if any)
   if ((CP_Click.CArrayId >= 0) && (CP.CArray[CP_Click.CArrayId-CARRAY_START].OnClick)) {
       bClickbleControlAtPosXY=1; 
       if (KeyPress_KeyRelease>0) {
           CP.CArray[CP_Click.CArrayId-CARRAY_START].OnClick();
       }
   } else if ((CP_Click.CId >= 0) && (CP.Control[CP_Click.CId].OnClick)) {
       bClickbleControlAtPosXY=1; 
       if (KeyPress_KeyRelease>0) {
           CP.Control[CP_Click.CId].OnClick();
       }
   }
   return bClickbleControlAtPosXY;
}

// calls the OnClick event of given control
int DoClickEvent(int CId)
{
    int X,Y;

    X = CP.Control[CId].X;
    Y = CP.Control[CId].Y;
    ProcessClickEventAtXY(X, Y, 1); // call OnClick for mouse button keypress event
    ProcessClickEventAtXY(X, Y, 2); // call OnClick for mouse button keyrelease event
    return 0;
}
// aply current control panel scale to convert GUI window mouse x,y coordinates
// to x,y corrdinates in control panel
void GetMousePosXY(int * Mouse_PosX, int * Mouse_PosY)
{
    // mode 1 uses value set by cpvid_poll when on mouse events
    // mode 0 uses CurX set by cpvid_poll even if mouse event returns SCPE_EOF
    *Mouse_PosX = cpvid.X;
    *Mouse_PosY = cpvid.Y;
    if ((CP.Scale != 100) && (CP.Scale >= 10)) { // if GUI scaled, adjust click position to match the unscaled control position
        *Mouse_PosX = *Mouse_PosX * 100 / CP.Scale;
        *Mouse_PosY = *Mouse_PosY * 100 / CP.Scale;
    }
}

void ControlPanel_Refresh(void)
{
    int xpixels, ypixels, ix,iy,n,p,X,Y,W,H,i,State,c,Mark,nStates, nStart, MaxH, MarkCol;
    int rr,gg,bb, alpha, rr_bg,gg_bg,bb_bg;
    uint32 t0, t1, RefreshInterval;
    uint32 *surface = (uint32 *) get_surface(&xpixels, &ypixels);
    uint32 AlphaMask = surface_rgb_color(0,0,0);
    int bShouldUpdateGUI, bShouldUpdateRect, xx, yy, bClickbleControlAtPosXY;
    int Mouse_PosX, Mouse_PosY;
    static int ButtonPressed = 0; 

    Into_Refresh = 1;
    Refresh_Frames_Count++;
    // calculate RefreshInterval = wallclock time in msec pased from start on last refresh
    t0 = CP.LastRefreshInit;
    Refresh_tnow    = CP.LastRefreshInit = sim_os_msec();
    RefreshInterval = CP.LastRefreshInit - t0;
    if ((RefreshInterval > 1000000) || (RefreshInterval < 1)) RefreshInterval = 1000000;

    // call control panel refresh routine callback
    if (CP.CP_Refresh != NULL) CP.CP_Refresh();
    // update states autonext: states thats automatically changes after a given msec duration
    t1 = CP.LastRefreshInit;
    for(i=0;i<CP.ControlItems;i++) {
        if (CP.Control[i].nStates == 0) continue;  // if nStates==0 -> control not drawn. Just it is used for click sense on its area
        if (CP.Control[i].AutoNext.State == -1) continue; // no AutoNext in this control
        t0 = CP.Control[i].AutoNext.Time0_msec;
        if ((t0 > t1) || ((t1 - t0) > CP.Control[i].AutoNext.Duration_msec)) {
            // duration expired, change state to the one defined
            CP.Control[i].State = CP.Control[i].AutoNext.State;
            CP.Control[i].AutoNext.State = -1;   // disable AutoNext in this control
        }
    }
    MarkCol = ((Refresh_Frames_Count >> 4) & 1);  // at 60 FPS >> 4 -> 3.8 FPS alternate marks colors
    // init to check if something has changed in surface and should be redraw on GUI
    bShouldUpdateGUI=0;
    RectList.Count=0;
    RectList.Scale=CP.Scale; 
    // itearate on controls and draw the ones that changed / are marked
    for(i=0;i<CP.ControlItems;i++) {
        State = CP.Control[i].State;
        Mark = CP.Control[i].Mark;  
        nStates = CP.Control[i].nStates; 
        if ((nStates == 0) && (Mark != 1)) continue; // if nStates==0 -> control not drawn. Just it is used for click sense on its area
        X = CP.Control[i].X;                         // get control boundaries
        Y = CP.Control[i].Y;
        W = CP.Control[i].W;
        H = CP.Control[i].H;
        MaxH = CP.Control[i].MaxH;
        if (X == -1) continue;                       // if X == -1 -> control not drawn nor sensed
        if ((X+W <  0) || (Y+H <  0)) continue;      // sanity check
        if ((X   > xpixels) || (Y   > ypixels)) continue;  // sanity check
        if (Mark >= 256) State = Mark & 255;         // Mark > 255 -> holds the state to force redraw control with this state
        if ((CP.Control[i].LastState == State) && (CP.FullRedrawNeeded == 0) && (Mark == 0)) continue; // skip control, no need to redraw
        // control needs to be redrawn. 
        CP.Control[i].LastState = State;             // save the last state drawn on GUI
        nStart = CP.Control[i].iPos_surface          // start of control's pixels
                 + State * W * H;                    // start of this state 
        // set max height to be draw
        if ((MaxH == 0) || (MaxH > H)) MaxH = H;
        bShouldUpdateRect=0;
        // copy control pixels to GUI surface
        for (iy=Y;iy<Y+MaxH;iy++) {
            if (iy < 0) continue;
            if (iy >= ypixels) break;
            p = iy*xpixels + X;
            n = nStart + (iy-Y) * W;
            for (ix=0;ix<W;ix++) {
                xx=ix + X; 
                if (xx >= xpixels) break;
                if ((p >= xpixels * ypixels) || (n >= MAX_PIXELS)) break;
                if (Mark==0) {                     // draw control on surface
                    if (nStates > 0) {
                        c = CP.surface[n];         // use pixel color if has one
                        if ((c) && (c & AlphaMask) != AlphaMask) {
                            // apply alpha on background
                            alpha=get_surface_rgb_color(c,&rr,&gg,&bb)+1;           // get control image pixel with alpha value
                            get_surface_rgb_color(surface[p],&rr_bg,&gg_bg,&bb_bg); // get background pixel
                            // if alpha=255 -> color is rr. If alfa=0 -> color is rr_bg
                            rr = rr_bg + (alpha * (rr-rr_bg)) / 256; 
                            gg = gg_bg + (alpha * (gg-gg_bg)) / 256; 
                            bb = bb_bg + (alpha * (bb-bb_bg)) / 256; 
                            c = surface_rgb_color(rr,gg,bb);
                        }
                    } else {
                        c = 0;                     // control has no states -> set transparent
                    }
                } else {                           // draw control with mark
                    if (nStates > 0) {c = CP.surface[n];}         
                    else             {c = surface[p];}  // control has no states -> get background color
                    get_surface_rgb_color(c,&rr,&gg,&bb);
                    if (MarkCol) {  // alternate marks colors
                        if (gg <= 192) {gg += 64;} else {gg -= 64;}
                    } else {
                        if (rr <= 192) {rr += 64;} else {rr -= 64;}
                    }
                    c = surface_rgb_color(rr,gg,bb);
                }
                if ((xx >= 0) && (c)) {
                   surface[p] = c;         // c==0 -> transparent pixel, else draw pixel with color c on GUI window
                   bShouldUpdateRect=1; 
                }
                p++; n++;
            }
        }
        if (bShouldUpdateRect) {
            // add control to be draw to rectangle list to be updated in surface and sent to 
            // SDL texture+Renderer (the actual VRAM video memory). If rectangle list count is <1 then
            // the full surface will be sent to VRAM. 
            n=RectList.Count; 
            if (n==-1) {
                // rect table full
            } else if (n>=RectList.MaxCount) {
                // set rect table as full
                RectList.Count=-1; 
            } else {
                // room left on rectable table
                ix=X; xx=X+W; 
                iy=Y; yy=Y+MaxH;
                if (ix < 0) ix=0; if (xx < 0) xx=0; if (ix >= xpixels) ix=xpixels; if (xx >= xpixels) xx=xpixels;
                if (iy < 0) iy=0; if (yy < 0) yy=0; if (iy >= ypixels) iy=ypixels; if (yy >= ypixels) yy=ypixels;
                RectList.x[n]=ix; RectList.w[n]=xx-ix; 
                RectList.y[n]=iy; RectList.h[n]=yy-iy; 
                RectList.Count++;
                bShouldUpdateGUI=1;
            }
        }
    }
    // printf("RectList.Count %d \n", RectList.Count);    

    //update GUI window
    if (cpanel_on) {
        cpvid_poll ();
        // if full redraw asked, update all the surface, not the rectlist
        if (CP.FullRedrawNeeded) RectList.Count=-1;      
        // if more than 6 sec without updating all the surface do a full surface update ignoring RectList
        // this is a catch-up for any missing refresh skipped
        if ((CP.FullSurfaceRedrawTm0) && ((Refresh_tnow - CP.FullSurfaceRedrawTm0 < 0) || (Refresh_tnow - CP.FullSurfaceRedrawTm0 > 6000))) {
            sim_debug (CP_REFRESH, cp_dev_ptr, "GUI doing a full surface Catch-up Refresh \n");
            RectList.Count=-1; 
        }
        // if updating all the surface set FullSurfaceSyncTm0 with current sim_os_msec
        if (RectList.Count==-1) {
            CP.FullSurfaceRedrawTm0 = 0; // will do a catch-up full redraw
            bShouldUpdateGUI=1; 
        }
        if (bShouldUpdateGUI) {
            if (0==cpvid_sync (1)) {
                sim_debug (CP_REFRESH, cp_dev_ptr, "GUI Refresh skipped - Event Queue too long/already redraw in Event Queue\n");
                if (CP.FullSurfaceRedrawTm0==0) {
                    CP.FullSurfaceRedrawTm0 = Refresh_tnow; // set catch-up full redraw because this refresh has been skipped
                    sim_debug (CP_REFRESH, cp_dev_ptr, "GUI request a full surface Catch-up Refresh \n");
                } else {
                    sim_debug (CP_REFRESH, cp_dev_ptr, "GUI Catch-up Refresh already requested, pending to be executed\n");
                }
            }
        } else {
           sim_debug (CP_REFRESH, cp_dev_ptr, "GUI Refresh skipped - bShouldUpdateGUI=0\n");
        }
    }
    CP.FullRedrawNeeded=0;
    // check key pressed
    if (cpvid.last_char && (CheckHotKeys(cpvid.last_char))) {
        cpvid.last_char = 0; // clear key as it is processed
    }
    // check if mouse button pressed (or release)
    bClickbleControlAtPosXY=-1; // don't know yet if clickable control under mouse
    GetMousePosXY(&Mouse_PosX, &Mouse_PosY); // get mouse posxy where button was clicked
    if (cpvid.mouse_b1) {
        if (ButtonPressed == 0) {
            // call event for mouse button keypress, if any clickable control at PosXY
            bClickbleControlAtPosXY=ProcessClickEventAtXY(Mouse_PosX, Mouse_PosY, 1); 
            ButtonPressed = 1;
        }
    } else {
        if (ButtonPressed) {
            ButtonPressed = 0;
            // call event for mouse button keyrelease, if any clickable control at PosXY
            bClickbleControlAtPosXY=ProcessClickEventAtXY(Mouse_PosX, Mouse_PosY, 2); 
        }
    }
    // check if must change mouse cursor icon 
    if (bClickbleControlAtPosXY<0) {
        // if bClickbleControlAtPosXY not set, just check if clickable 
        // control under mouse pointer without calling any OnClick callback
        bClickbleControlAtPosXY=ProcessClickEventAtXY(Mouse_PosX, Mouse_PosY, 0); 
    }
    if (bClickbleControlAtPosXY==1) {
        // if mouse over clickable control -> show hand icon as mouse
        vid_set_system_cursor(2); // current set hand cursor
    } else {
        vid_set_system_cursor(1); // current set arrow cursor
    }
    
    // finalize
    CP.LastRefreshDone = sim_os_msec();
    CP.LastTickCountNum = 0;
    cpanel_measured_fps = (RefreshInterval>0) ? 1000 / RefreshInterval : -1;
    //    printf("refresh duration: %d msec, starting refresh each %d msec (FPS %d) \n",
    //        CP.LastRefreshDone - CP.LastRefreshInit, RefreshInterval, cpanel_measured_fps);
    sim_debug (CP_REFRESH, cp_dev_ptr, "GUI Refresh duration: %d msec, starting refresh each %d msec (FPS %d) \n",
            CP.LastRefreshDone - CP.LastRefreshInit, RefreshInterval, 1000 / RefreshInterval);
    Into_Refresh = 0;

}

// return 1 if must call refresh callback needed (at FPS wallclock rate), 
//        -1 if must call the tick intensity callback
int Refresh_needed(void)
{
    uint32 tnow; 

    // check if GUI needed refresh. Checks if wallclock msec for target FPS are passed from last
    // refresh termination (not refresh start). This assures that refresh() will not eat all the CPU time
    // if refresh() takes 0 msec: Achieved FPS = FPS
    // if refresh() takes n msec, Achieved FPS = 1000/(1000/FPS + n)
    // For target 30 FPS (33 msec per frame) and a long 50 msec refresh, we get
    // Achieved FPS = 1000/(1000/30+50) = 12 frames per second. 
    tnow=sim_os_msec();    
    if ((tnow < CP.LastRefreshDone) || ((tnow - CP.LastRefreshDone) > (1000 / FPS) )) {
        CP.LastTickIntensityDone = tnow; 
        return 1; // return needs refresh
    } else if (tnow != CP.LastTickIntensityDone) {
        // tickintensity is called each millisec (-> 33 times per second if FPS=30)
        CP.LastTickIntensityDone = tnow; 
        return -1; // return can call tick intensity callback
    }  
    return 0; 
}

int ControlPanel_DoSCP_pending(void)
{
    return SCP_cmd_count; // return number of SCP commands issued by control panel pending to be executed
}

// set cmd with next pending scp command, and remove if from list
// if called with cmd=NULL, just remove last cmd form list
void ControlPanel_GetSCP_pending(char *cmd, int nSize)
{
    int i,c;
    int bCmdOk = 1;

    if ((nSize < 1) || (cmd == NULL)) bCmdOk = 0;
    if (bCmdOk) cmd[0] = 0;          // clear retul scp cmd 
    if (SCP_cmd_count == 0) return;  // no scp command pending
    i = 0;
    // copy first command to result cmd
    while(bCmdOk) {
        c = SCP_cmd[i];
        cmd[i++] = c;
        if (c == 0) break;
        if (i >= nSize-1) {cmd[i]=0; break;}
    }
    // remove first command from lost
    for (i=MAX_SCP_CMD_LEN; i< MAX_SCP_CMD_LEN*MAX_SCP_CMDS; i++) SCP_cmd[i-MAX_SCP_CMD_LEN] = SCP_cmd[i];
    SCP_cmd_count--;
}


void ControlPanel_Refresh_CPU_Halted(void)
{
    if (Refresh_needed()>0) {
        ControlPanel_Refresh();   // do the GUI window refresh
    }
    if (cpanel_stop_flag == 3) {
        ControlPanel_done();
        cpanel_stop_flag = 0;        
        sim_debug (CP_CMDS, cp_dev_ptr, "GUI Close Window\n");
    }
}

t_stat ControlPanel_Refresh_CPU_Running(void)
{
    int n, cpanel_stop;        

    n=Refresh_needed();
    if (n>0) {
        ControlPanel_Refresh();   // do the GUI window refresh
    } else if ((n<0) && (CP.TickCount) && (CP.CP_TickIntensityCount)) {
        CP.CP_TickIntensityCount(); // call the tick intensity callback
        CP.LastTickCountNum++;
        sim_debug (CP_REFRESH, cp_dev_ptr, "TickIntensityCount callback executed (number %d in this frame)\n",
            CP.LastTickCountNum);
    }

    // check if GUI window close has been requested by user
    // cpanel_stop_flag = 1 -> Control-E (^E = WRU) pressed while GUI has focus or Halt Cpu Button pressed
    //                  = 2 -> same, but keep interactive mode
    //                  = 3 -> close GUI, but do not stop cpu
    //                  = 4 -> close GUI, and execute Bye scp command
    cpanel_stop = cpanel_stop_flag; // make a local copy and clear the flag
    cpanel_stop_flag = 0;        

    if (cpanel_stop >= 3) {
        ControlPanel_done();
        if (cpanel_stop == 4) {
            sim_debug (CP_CMDS, cp_dev_ptr, "Exit from GUI\n");
            // stop cpu to allow bye command to be executed
            return SCPE_EXIT;
        } else {
            sim_debug (CP_CMDS, cp_dev_ptr, "Close GUI\n");
            return SCPE_OK;
        }
    } else if (cpanel_stop > 0) {
        if (cpanel_stop == 1) cpanel_interactive = 0;
        sim_debug (CP_CMDS, cp_dev_ptr, "Stop Button on GUI\n");
        return SCPE_STOP;                               // signal inst main loop that user asked to close the cpanel window 
    }
    if (ControlPanel_DoSCP_pending()) {
        // if SCP command issued while cpu running, stop cpu to allow scp command to execute from scp
        return SCPE_STOP;
    }
    return SCPE_OK;
}

// loop refreshing control panel, until either
// - close GUI window
// - ^E pressed
// - SCP command issed by control panel using DoSCP
void ControlPanel_InteractiveLoop(void)
{
    t_stat reason; 
    char c;

    fflush(stdout);   // flush stdout so if user closes sim suring cpanel interacte, the log files are updated 
    if (sim_log) fflush (sim_log);                          

    while (1) {
        reason = ControlPanel_Refresh_CPU_Running();
        if (cpanel_on == 0) {        // close GUI -> 
            cpanel_interactive = 0;  // disable interactive mode
            if (reason == SCPE_EXIT) DoSCP("bye");
            break;                   // and return to SCP
        }
        if (reason != SCPE_OK) {     // some action during refresh (e.g. DoSCP)
            break;                   // just return to SCP
        }
        sim_os_ms_sleep (25);        // this sleep is to let the user press a button without eating all the host cpu
        c = sim_poll_kbd() & 0177;   // poll keyboard to see if ^E (WRU) pressed during set cpanel interactive scp command execution
        if ((stop_cpu) || (c==13)) { // ^E or <CR> pressed -> exit interactive loop
            stop_cpu = 0;            // clean up
            cpanel_interactive = 0;  // disabling interactive mode
            break;                   // and return to SCP
        }
        CheckHotKeys(c);             // process cpanel hotkeys even if they are typed in console
    }
}


// add scp command given as param to list in SCP_cmd array
void DoSCP(char *cmd)
{
    int i; 

    if ((cmd == NULL) || (cmd[0] == 0)) return;
    if (strcmp(cmd, "<NOECHO1>") == 0) {
        SCP_cmd_echo=0; // disable DoSCP echo on next command sent
        return; 
    }
    if (SCP_cmd_count >= MAX_SCP_CMDS) {
        // if list full, discard list
        SCP_cmd_count = 0;
    }
    // copy scp command to list at next free pos
    for (i=0;(cmd[i]>0) && (i < MAX_SCP_CMD_LEN-1);i++) SCP_cmd[SCP_cmd_count * MAX_SCP_CMD_LEN + i] = cmd[i];
    SCP_cmd[SCP_cmd_count * MAX_SCP_CMD_LEN + MAX_SCP_CMD_LEN-1] = 0; // paranoic preacution
    SCP_cmd_count++;
}


char * cpanel_fgets(const char *prompt, char *cptr, int32 size, FILE *stream)
{
    if ((cpanel_on) && (cpanel_interactive) && (ControlPanel_DoSCP_pending()==0)) {
        // if cpanel interactive and no pending scp command to issue, then enter interactive 
        // loop waiting for actions on cpanels
        printf ("(Control Panel Interactive mode, press Enter to exit)\n");                              
        ControlPanel_InteractiveLoop();
    }
    if (ControlPanel_DoSCP_pending()>0) {
        // if control panel has generated a SCP command not yet executed, get it 
        ControlPanel_GetSCP_pending(cptr, size);		   
        // then print it as if it was typed by user
        if (SCP_cmd_echo) {
            if (prompt) printf ("%s", prompt);                              
            sim_printf("%s\n", cptr);
        }
    } else {
        // no pending SCP commands generated by control panel
        if (cpanel_on) {
            // if cpanel visible, do a regular refresh of GUI window
            ControlPanel_Refresh_CPU_Halted();
        }
        vid_set_system_cursor(0); // set mouse pointer to wait because fgets will block the execution until user press return
        if (prompt) printf ("%s", prompt);                              
        cptr = fgets (cptr, size, stream);     

    }
    SCP_cmd_echo=1; // restore echo of scp commands sent by DoSCP
    return cptr;
}

t_stat cpanel_interval_refresh(void)
{
    return ControlPanel_Refresh_CPU_Running();  
}

// analyze bits in param n. Increment the tick count for each bit to 1 found on n
int TickCount(int Id, t_uint64 n) 
{
    int i, nTick;
    if (Id < CARRAY_START) {
        // single control. Just count tick based on bit 0 
        if (CHK_CId(Id) < 0) return -1;                                // sanity check
        nTick = CP.Control[Id].nTick;
        if (n & 1) CP.TickCount[nTick].Bit[0]++;
        CP.TickCount[nTick].Num++;
    } else {
        // control array -> set tick count on each single control
        CHK_CArrayId(Id)
        nTick = CP.CArray[Id - CARRAY_START].nTick;
        i = 0;
        while (n) {
            if (n & 1) CP.TickCount[nTick].Bit[i]++;
            n = n >> 1;
            i++;
        }
        CP.TickCount[nTick].Num++;
    }
    return 0;
}

// return state that match the intensity of given bit number, based on ticks count
int GetStateForIntensity(int nTick, int nbit, int nStates) 
{
    int n1, n2;

    if ((nbit < 0) || (nbit > 63)) return 0;
    n1 = CP.TickCount[nTick].Bit[nbit];
    if (n1 < 1) return 0;  // 0 ticks counted -> 0 intensity
    n2 = CP.TickCount[nTick].Num;
    if (n2 < 1) return 0;  // tick count = 0 -> no data -> no intensity
    n1 = (nStates * n1) / n2;
    if (n1 < 1) return 0;  
    if (n1 >= nStates-1) return 1; // state = 1 = full intensity
    return n1+1;           // state = 2 .. nStates-2 = intermediate intensities
}

int SetStateWithIntensity(int Id, t_uint64 n) 
{
    int nStates = 0;
    int CId, CId0, i, sta, nTick;
    if (CP.EnableTickCount == 0) {
        SetState(Id, n);
        return 0;
    }
    if (sim_is_running == 0) {
        // if cpu not running or no intensity tick data just do a normal setstate and reset tick counter
        SetState(Id, n);
        if (Id < CARRAY_START) {
            if (CHK_CId(Id) < 0) return -1;                                // sanity check
            nTick = CP.Control[Id].nTick;
        } else {
            CHK_CArrayId(Id)
            nTick = CP.CArray[Id - CARRAY_START].nTick;
        }
        if (nTick >= 0) InitTickCount(nTick);
        return 0;
    }
    // cpu is running and tick data available: go on for setting the intensity in function
    // of number of times (tick count) a bit is set respect the total number of ticks counted
    if (Id < CARRAY_START) {
        // single control. Just calc intensity based on bit 0 and set it as control state
        if (CHK_CId(Id) < 0) return -1;                                // sanity check
        nTick = CP.Control[Id].nTick;
        if ((nTick < 0) || (CP.TickCount[nTick].Num < 2)) {
            SetState(Id, n);
        } else {
            nStates = CP.Control[Id].nStates;
            sta = GetStateForIntensity(nTick, 0, nStates);
            CP.Control[Id].State = sta;
        }
    } else {
        // control array -> set intensity on each single control
        CHK_CArrayId(Id)
        nTick = CP.CArray[Id - CARRAY_START].nTick;
        if ((nTick < 0) || (CP.TickCount[nTick].Num < 2)) {
            SetState(Id, n);
        } else {
            for (i=0;i<CP.CArray[Id - CARRAY_START].Items;i++) {
                CId0 = CP.CArray[Id - CARRAY_START].CId0;
                CId = CP.CArrayCidItems[CId0 + i];
                nStates = CP.Control[CId].nStates;
                sta = GetStateForIntensity(nTick, i, nStates);
                CP.Control[CId].State = sta;
            }
        }
    }
    if (nTick >= 0) InitTickCount(nTick);
    return 0;
}


t_stat cp_set_cpanel_on (UNIT *uptr, int32 value, CONST char *cptr, void *desc) 
{
    int power = value ? 1:0;
    ControlPanel_Load(
        power, 
        cp_types, 
        &cp_unit, &cp_dev); 

    return SCPE_OK;
}

t_stat cp_set_param(UNIT *uptr, int32 value, CONST char *cptr, void *desc) 
{ 
    DEVICE *dptr = (DEVICE *) desc;
    int32 num, xPos,yPos;
    CONST char *tptr;
    t_stat r;

    if (cptr == NULL) return SCPE_ARG;
    if (desc == NULL) return SCPE_IERR;

    if (value == 1) { // SET PRESS=name
        if (DoActionByName((char *) cptr,0) < 0) {
            fprintf(stderr, "Control %s not found\n", cptr);
        }
    } else if (value == 2) { // SET MARK=name|<ALL>|<NONE>
        if (DoActionByName((char *) cptr,1) < 0) {
            fprintf(stderr, "Control %s not found\n", cptr);
        }
    } else if (value == 3) { // SET LIST=name|<ALL>
        if (DoActionByName((char *) cptr,2) < 0) {
            fprintf(stderr, "Control %s not found\n", cptr);
        }
    } else if (value == 4) { // SET STATE=name|<ALL> / n|*
        if (DoActionByName((char *) cptr,3) < 0) {
            fprintf(stderr, "Control %s not found\n", cptr);
        }
    } else if (value == 5) { // SET OPTION=name|<NONE> 
        if (AddOption((char *) cptr) < 0) {
            fprintf(stderr, "Cannot set option\n");
        }
    } else if (value == 6) { // SET TICKCOUNT=0|1
        num = (int32) get_uint (cptr, 10, 1, &r);
        if (r != SCPE_OK) return r;
        CP.EnableTickCount = num; 
    } else if (value == 7) { // SET SCALE=10..200
        num = (int32) get_uint (cptr, 10, 200, &r);
        if (r != SCPE_OK) return r;
        if (num < 10) return SCPE_ARG;
        CP.Scale = num; 
        if (cpanel_on == 0) {
            return SCPE_OK; // no panel opened
        }
        DoActionByName("", 4);
    } else if (value == 8) { // SET POS=X/Y
        xPos = (int) strtotsv (cptr, &tptr, 0);
        if (cptr == tptr) return SCPE_ARG;
        if (*tptr != '/') return SCPE_ARG;
        yPos = (int) strtotsv (++tptr, &tptr, 0);
        if (cptr == tptr) return SCPE_ARG;
        if (cpanel_on == 0) {
            fprintf(stderr, "No cpanel open\n");
            return SCPE_OK; // no panel opened
        }
        sim_debug (CP_CMDS, cp_dev_ptr, "GUI Pos x=%d, y=%d \n", xPos, yPos);
        vid_SetWindowSizeAndPos(1,xPos,yPos);
    } else {
        // undefined SET 
        return SCPE_IERR;
    }
    return SCPE_OK;
}

static void cp_quit_callback (void)
{
    cpanel_stop_flag = 4; // request exit simh
}

/* Reset routine */

t_stat cp_reset (DEVICE *dptr)
{
    sim_debug (CP_CMDS, &cp_dev, "Control Panel Reset\n");
    if (!(dptr->flags & DEV_DIS)) {
        vid_register_quit_callback (&cp_quit_callback);
    }
    if ((cpanel_on) && (CP.CP_Reset != NULL)) CP.CP_Reset();

    return SCPE_OK;
}

t_stat cp_interactive(UNIT *uptr, int32 value, CONST char *cptr, void *desc)     
{ 
    cpanel_interactive = 1; 
    return SCPE_OK; 
}

t_stat cp_attach (UNIT *uptr, CONST char *file) 
{ 
    DEVICE             *dptr = find_dev_from_unit(uptr);
    int                 unit = (uptr - dptr->units);
    t_stat              r;

    if ((r = attach_unit(uptr, file)) != SCPE_OK) return r;

    CP.Options[0] = 0;
    CP.EnableTickCount = 1; // default value
    CP.Scale = 100; // set default scale to 100% 
    return SCPE_OK;
}
t_stat cp_dettach(UNIT *uptr)
{
    sim_cancel(uptr); // cancel any pending command
    return detach_unit (uptr);                             /* detach unit */
}

const char * cp_description (DEVICE *dptr) {
    return "Control Panel GUI";
}


// Matches the ifdef(CPANEL) at beginning of file

#else

#endif
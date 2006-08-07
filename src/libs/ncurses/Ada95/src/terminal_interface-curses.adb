------------------------------------------------------------------------------
--                                                                          --
--                           GNAT ncurses Binding                           --
--                                                                          --
--                        Terminal_Interface.Curses                         --
--                                                                          --
--                                 B O D Y                                  --
--                                                                          --
------------------------------------------------------------------------------
-- Copyright (c) 1998,2004 Free Software Foundation, Inc.                   --
--                                                                          --
-- Permission is hereby granted, free of charge, to any person obtaining a  --
-- copy of this software and associated documentation files (the            --
-- "Software"), to deal in the Software without restriction, including      --
-- without limitation the rights to use, copy, modify, merge, publish,      --
-- distribute, distribute with modifications, sublicense, and/or sell       --
-- copies of the Software, and to permit persons to whom the Software is    --
-- furnished to do so, subject to the following conditions:                 --
--                                                                          --
-- The above copyright notice and this permission notice shall be included  --
-- in all copies or substantial portions of the Software.                   --
--                                                                          --
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  --
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               --
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   --
-- IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   --
-- DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    --
-- OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    --
-- THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               --
--                                                                          --
-- Except as contained in this notice, the name(s) of the above copyright   --
-- holders shall not be used in advertising or otherwise to promote the     --
-- sale, use or other dealings in this Software without prior written       --
-- authorization.                                                           --
------------------------------------------------------------------------------
--  Author: Juergen Pfeifer, 1996
--  Version Control:
--  $Revision: 1.32 $
--  $Date: 2004/08/21 21:37:00 $
--  Binding Version 01.00
------------------------------------------------------------------------------
with System;

with Terminal_Interface.Curses.Aux;
with Interfaces.C;                  use Interfaces.C;
with Interfaces.C.Strings;          use Interfaces.C.Strings;
with Interfaces.C.Pointers;
with Ada.Characters.Handling;       use Ada.Characters.Handling;
with Ada.Strings.Fixed;
with Ada.Unchecked_Conversion;

package body Terminal_Interface.Curses is

   use Aux;
   use type System.Bit_Order;

   package ASF renames Ada.Strings.Fixed;

   type chtype_array is array (size_t range <>)
      of aliased Attributed_Character;
   pragma Convention (C, chtype_array);

------------------------------------------------------------------------------
   generic
      type Element is (<>);
   function W_Get_Element (Win    : in Window;
                           Offset : in Natural) return Element;

   function W_Get_Element (Win    : in Window;
                           Offset : in Natural) return Element is
      type E_Array is array (Natural range <>) of aliased Element;
      package C_E_Array is new
        Interfaces.C.Pointers (Natural, Element, E_Array, Element'Val (0));
      use C_E_Array;

      function To_Pointer is new
        Ada.Unchecked_Conversion (Window, Pointer);

      P : Pointer := To_Pointer (Win);
   begin
      if Win = Null_Window then
         raise Curses_Exception;
      else
         P := P + ptrdiff_t (Offset);
         return P.all;
      end if;
   end W_Get_Element;

   function W_Get_Int   is new W_Get_Element (C_Int);
   function W_Get_Short is new W_Get_Element (C_Short);
   function W_Get_Byte  is new W_Get_Element (Interfaces.C.unsigned_char);

   function Get_Flag (Win    : Window;
                      Offset : Natural) return Boolean;

   function Get_Flag (Win    : Window;
                      Offset : Natural) return Boolean
   is
      Res : C_Int;
   begin
      case Sizeof_bool is
         when 1 => Res := C_Int (W_Get_Byte  (Win, Offset));
         when 2 => Res := C_Int (W_Get_Short (Win, Offset));
         when 4 => Res := C_Int (W_Get_Int   (Win, Offset));
         when others => raise Curses_Exception;
      end case;

      case Res is
         when 0       => return False;
         when others  => return True;
      end case;
   end Get_Flag;

------------------------------------------------------------------------------
   function Key_Name (Key : in Real_Key_Code) return String
   is
      function Keyname (K : C_Int) return chars_ptr;
      pragma Import (C, Keyname, "keyname");

      Ch : Character;
   begin
      if Key <= Character'Pos (Character'Last) then
         Ch := Character'Val (Key);
         if Is_Control (Ch) then
            return Un_Control (Attributed_Character'(Ch    => Ch,
                                                     Color => Color_Pair'First,
                                                     Attr  => Normal_Video));
         elsif Is_Graphic (Ch) then
            declare
               S : String (1 .. 1);
            begin
               S (1) := Ch;
               return S;
            end;
         else
            return "";
         end if;
      else
         return Fill_String (Keyname (C_Int (Key)));
      end if;
   end Key_Name;

   procedure Key_Name (Key  : in  Real_Key_Code;
                       Name : out String)
   is
   begin
      ASF.Move (Key_Name (Key), Name);
   end Key_Name;

------------------------------------------------------------------------------
   procedure Init_Screen
   is
      function Initscr return Window;
      pragma Import (C, Initscr, "initscr");

      W : Window;
   begin
      W := Initscr;
      if W = Null_Window then
         raise Curses_Exception;
      end if;
   end Init_Screen;

   procedure End_Windows
   is
      function Endwin return C_Int;
      pragma Import (C, Endwin, "endwin");
   begin
      if Endwin = Curses_Err then
         raise Curses_Exception;
      end if;
   end End_Windows;

   function Is_End_Window return Boolean
   is
      function Isendwin return Curses_Bool;
      pragma Import (C, Isendwin, "isendwin");
   begin
      if Isendwin = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Is_End_Window;
------------------------------------------------------------------------------
   procedure Move_Cursor (Win    : in Window := Standard_Window;
                          Line   : in Line_Position;
                          Column : in Column_Position)
   is
      function Wmove (Win    : Window;
                      Line   : C_Int;
                      Column : C_Int
                     ) return C_Int;
      pragma Import (C, Wmove, "wmove");
   begin
      if Wmove (Win, C_Int (Line), C_Int (Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Move_Cursor;
------------------------------------------------------------------------------
   procedure Add (Win : in Window := Standard_Window;
                  Ch  : in Attributed_Character)
   is
      function Waddch (W  : Window;
                       Ch : C_Chtype) return C_Int;
      pragma Import (C, Waddch, "waddch");
   begin
      if Waddch (Win, AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add;

   procedure Add (Win : in Window := Standard_Window;
                  Ch  : in Character)
   is
   begin
      Add (Win,
           Attributed_Character'(Ch    => Ch,
                                 Color => Color_Pair'First,
                                 Attr  => Normal_Video));
   end Add;

   procedure Add
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Ch     : in Attributed_Character)
   is
      function mvwaddch (W  : Window;
                         Y  : C_Int;
                         X  : C_Int;
                         Ch : C_Chtype) return C_Int;
      pragma Import (C, mvwaddch, "mvwaddch");
   begin
      if mvwaddch (Win, C_Int (Line),
                   C_Int (Column),
                   AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add;

   procedure Add
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Ch     : in Character)
   is
   begin
      Add (Win,
           Line,
           Column,
           Attributed_Character'(Ch    => Ch,
                                 Color => Color_Pair'First,
                                 Attr  => Normal_Video));
   end Add;

   procedure Add_With_Immediate_Echo
     (Win : in Window := Standard_Window;
      Ch  : in Attributed_Character)
   is
      function Wechochar (W  : Window;
                          Ch : C_Chtype) return C_Int;
      pragma Import (C, Wechochar, "wechochar");
   begin
      if Wechochar (Win, AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add_With_Immediate_Echo;

   procedure Add_With_Immediate_Echo
     (Win : in Window := Standard_Window;
      Ch  : in Character)
   is
   begin
      Add_With_Immediate_Echo
        (Win,
         Attributed_Character'(Ch    => Ch,
                               Color => Color_Pair'First,
                               Attr  => Normal_Video));
   end Add_With_Immediate_Echo;
------------------------------------------------------------------------------
   function Create (Number_Of_Lines       : Line_Count;
                    Number_Of_Columns     : Column_Count;
                    First_Line_Position   : Line_Position;
                    First_Column_Position : Column_Position) return Window
   is
      function Newwin (Number_Of_Lines       : C_Int;
                       Number_Of_Columns     : C_Int;
                       First_Line_Position   : C_Int;
                       First_Column_Position : C_Int) return Window;
      pragma Import (C, Newwin, "newwin");

      W : Window;
   begin
      W := Newwin (C_Int (Number_Of_Lines),
                   C_Int (Number_Of_Columns),
                   C_Int (First_Line_Position),
                   C_Int (First_Column_Position));
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end Create;

   procedure Delete (Win : in out Window)
   is
      function Wdelwin (W : Window) return C_Int;
      pragma Import (C, Wdelwin, "delwin");
   begin
      if Wdelwin (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
      Win := Null_Window;
   end Delete;

   function Sub_Window
     (Win                   : Window := Standard_Window;
      Number_Of_Lines       : Line_Count;
      Number_Of_Columns     : Column_Count;
      First_Line_Position   : Line_Position;
      First_Column_Position : Column_Position) return Window
   is
      function Subwin
        (Win                   : Window;
         Number_Of_Lines       : C_Int;
         Number_Of_Columns     : C_Int;
         First_Line_Position   : C_Int;
         First_Column_Position : C_Int) return Window;
      pragma Import (C, Subwin, "subwin");

      W : Window;
   begin
      W := Subwin (Win,
                   C_Int (Number_Of_Lines),
                   C_Int (Number_Of_Columns),
                   C_Int (First_Line_Position),
                   C_Int (First_Column_Position));
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end Sub_Window;

   function Derived_Window
     (Win                   : Window := Standard_Window;
      Number_Of_Lines       : Line_Count;
      Number_Of_Columns     : Column_Count;
      First_Line_Position   : Line_Position;
      First_Column_Position : Column_Position) return Window
   is
      function Derwin
        (Win                   : Window;
         Number_Of_Lines       : C_Int;
         Number_Of_Columns     : C_Int;
         First_Line_Position   : C_Int;
         First_Column_Position : C_Int) return Window;
      pragma Import (C, Derwin, "derwin");

      W : Window;
   begin
      W := Derwin (Win,
                   C_Int (Number_Of_Lines),
                   C_Int (Number_Of_Columns),
                   C_Int (First_Line_Position),
                   C_Int (First_Column_Position));
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end Derived_Window;

   function Duplicate (Win : Window) return Window
   is
      function Dupwin (Win : Window) return Window;
      pragma Import (C, Dupwin, "dupwin");

      W : constant Window := Dupwin (Win);
   begin
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end Duplicate;

   procedure Move_Window (Win    : in Window;
                          Line   : in Line_Position;
                          Column : in Column_Position)
   is
      function Mvwin (Win    : Window;
                      Line   : C_Int;
                      Column : C_Int) return C_Int;
      pragma Import (C, Mvwin, "mvwin");
   begin
      if Mvwin (Win, C_Int (Line), C_Int (Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Move_Window;

   procedure Move_Derived_Window (Win    : in Window;
                                  Line   : in Line_Position;
                                  Column : in Column_Position)
   is
      function Mvderwin (Win    : Window;
                         Line   : C_Int;
                         Column : C_Int) return C_Int;
      pragma Import (C, Mvderwin, "mvderwin");
   begin
      if Mvderwin (Win, C_Int (Line), C_Int (Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Move_Derived_Window;

   procedure Set_Synch_Mode (Win  : in Window  := Standard_Window;
                             Mode : in Boolean := False)
   is
      function Syncok (Win  : Window;
                       Mode : Curses_Bool) return C_Int;
      pragma Import (C, Syncok, "syncok");
   begin
      if Syncok (Win, Curses_Bool (Boolean'Pos (Mode))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Synch_Mode;
------------------------------------------------------------------------------
   procedure Add (Win : in Window := Standard_Window;
                  Str : in String;
                  Len : in Integer := -1)
   is
      function Waddnstr (Win : Window;
                         Str : char_array;
                         Len : C_Int := -1) return C_Int;
      pragma Import (C, Waddnstr, "waddnstr");

      Txt    : char_array (0 .. Str'Length);
      Length : size_t;
   begin
      To_C (Str, Txt, Length);
      if Waddnstr (Win, Txt, C_Int (Len)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add;

   procedure Add
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Str    : in String;
      Len    : in Integer := -1)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Add (Win, Str, Len);
   end Add;
------------------------------------------------------------------------------
   procedure Add
     (Win : in Window := Standard_Window;
      Str : in Attributed_String;
      Len : in Integer := -1)
   is
      function Waddchnstr (Win : Window;
                           Str : chtype_array;
                           Len : C_Int := -1) return C_Int;
      pragma Import (C, Waddchnstr, "waddchnstr");

      Txt : chtype_array (0 .. Str'Length);
   begin
      for Length in 1 .. size_t (Str'Length) loop
         Txt (Length - 1) := Str (Natural (Length));
      end loop;
      Txt (Str'Length) := Default_Character;
      if Waddchnstr (Win,
                     Txt,
                     C_Int (Len)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add;

   procedure Add
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Str    : in Attributed_String;
      Len    : in Integer := -1)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Add (Win, Str, Len);
   end Add;
------------------------------------------------------------------------------
   procedure Border
     (Win                       : in Window := Standard_Window;
      Left_Side_Symbol          : in Attributed_Character := Default_Character;
      Right_Side_Symbol         : in Attributed_Character := Default_Character;
      Top_Side_Symbol           : in Attributed_Character := Default_Character;
      Bottom_Side_Symbol        : in Attributed_Character := Default_Character;
      Upper_Left_Corner_Symbol  : in Attributed_Character := Default_Character;
      Upper_Right_Corner_Symbol : in Attributed_Character := Default_Character;
      Lower_Left_Corner_Symbol  : in Attributed_Character := Default_Character;
      Lower_Right_Corner_Symbol : in Attributed_Character := Default_Character)
   is
      function Wborder (W   : Window;
                        LS  : C_Chtype;
                        RS  : C_Chtype;
                        TS  : C_Chtype;
                        BS  : C_Chtype;
                        ULC : C_Chtype;
                        URC : C_Chtype;
                        LLC : C_Chtype;
                        LRC : C_Chtype) return C_Int;
      pragma Import (C, Wborder, "wborder");
   begin
      if Wborder (Win,
                  AttrChar_To_Chtype (Left_Side_Symbol),
                  AttrChar_To_Chtype (Right_Side_Symbol),
                  AttrChar_To_Chtype (Top_Side_Symbol),
                  AttrChar_To_Chtype (Bottom_Side_Symbol),
                  AttrChar_To_Chtype (Upper_Left_Corner_Symbol),
                  AttrChar_To_Chtype (Upper_Right_Corner_Symbol),
                  AttrChar_To_Chtype (Lower_Left_Corner_Symbol),
                  AttrChar_To_Chtype (Lower_Right_Corner_Symbol)
                  ) = Curses_Err
      then
         raise Curses_Exception;
      end if;
   end Border;

   procedure Box
     (Win               : in Window := Standard_Window;
      Vertical_Symbol   : in Attributed_Character := Default_Character;
      Horizontal_Symbol : in Attributed_Character := Default_Character)
   is
   begin
      Border (Win,
              Vertical_Symbol, Vertical_Symbol,
              Horizontal_Symbol, Horizontal_Symbol);
   end Box;

   procedure Horizontal_Line
     (Win         : in Window := Standard_Window;
      Line_Size   : in Natural;
      Line_Symbol : in Attributed_Character := Default_Character)
   is
      function Whline (W   : Window;
                       Ch  : C_Chtype;
                       Len : C_Int) return C_Int;
      pragma Import (C, Whline, "whline");
   begin
      if Whline (Win,
                 AttrChar_To_Chtype (Line_Symbol),
                 C_Int (Line_Size)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Horizontal_Line;

   procedure Vertical_Line
     (Win         : in Window := Standard_Window;
      Line_Size   : in Natural;
      Line_Symbol : in Attributed_Character := Default_Character)
   is
      function Wvline (W   : Window;
                       Ch  : C_Chtype;
                       Len : C_Int) return C_Int;
      pragma Import (C, Wvline, "wvline");
   begin
      if Wvline (Win,
                 AttrChar_To_Chtype (Line_Symbol),
                 C_Int (Line_Size)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Vertical_Line;

------------------------------------------------------------------------------
   function Get_Keystroke (Win : Window := Standard_Window)
     return Real_Key_Code
   is
      function Wgetch (W : Window) return C_Int;
      pragma Import (C, Wgetch, "wgetch");

      C : constant C_Int := Wgetch (Win);
   begin
      if C = Curses_Err then
         return Key_None;
      else
         return Real_Key_Code (C);
      end if;
   end Get_Keystroke;

   procedure Undo_Keystroke (Key : in Real_Key_Code)
   is
      function Ungetch (Ch : C_Int) return C_Int;
      pragma Import (C, Ungetch, "ungetch");
   begin
      if Ungetch (C_Int (Key)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Undo_Keystroke;

   function Has_Key (Key : Special_Key_Code) return Boolean
   is
      function Haskey (Key : C_Int) return C_Int;
      pragma Import (C, Haskey, "has_key");
   begin
      if Haskey (C_Int (Key)) = Curses_False then
         return False;
      else
         return True;
      end if;
   end Has_Key;

   function Is_Function_Key (Key : Special_Key_Code) return Boolean
   is
      L : constant Special_Key_Code  := Special_Key_Code (Natural (Key_F0) +
        Natural (Function_Key_Number'Last));
   begin
      if (Key >= Key_F0) and then (Key <= L) then
         return True;
      else
         return False;
      end if;
   end Is_Function_Key;

   function Function_Key (Key : Real_Key_Code)
                          return Function_Key_Number
   is
   begin
      if Is_Function_Key (Key) then
         return Function_Key_Number (Key - Key_F0);
      else
         raise Constraint_Error;
      end if;
   end Function_Key;

   function Function_Key_Code (Key : Function_Key_Number) return Real_Key_Code
   is
   begin
      return Real_Key_Code (Natural (Key_F0) + Natural (Key));
   end Function_Key_Code;
------------------------------------------------------------------------------
   procedure Standout (Win : Window  := Standard_Window;
                       On  : Boolean := True)
   is
      function wstandout (Win : Window) return C_Int;
      pragma Import (C, wstandout, "wstandout");
      function wstandend (Win : Window) return C_Int;
      pragma Import (C, wstandend, "wstandend");

      Err : C_Int;
   begin
      if On then
         Err := wstandout (Win);
      else
         Err := wstandend (Win);
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Standout;

   procedure Switch_Character_Attribute
     (Win  : in Window := Standard_Window;
      Attr : in Character_Attribute_Set := Normal_Video;
      On   : in Boolean := True)
   is
      function Wattron (Win    : Window;
                        C_Attr : C_AttrType) return C_Int;
      pragma Import (C, Wattron, "wattr_on");
      function Wattroff (Win    : Window;
                         C_Attr : C_AttrType) return C_Int;
      pragma Import (C, Wattroff, "wattr_off");
      --  In Ada we use the On Boolean to control whether or not we want to
      --  switch on or off the attributes in the set.
      Err : C_Int;
      AC  : constant Attributed_Character := (Ch    => Character'First,
                                              Color => Color_Pair'First,
                                              Attr  => Attr);
   begin
      if On then
         Err := Wattron  (Win, AttrChar_To_AttrType (AC));
      else
         Err := Wattroff (Win, AttrChar_To_AttrType (AC));
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Switch_Character_Attribute;

   procedure Set_Character_Attributes
     (Win   : in Window := Standard_Window;
      Attr  : in Character_Attribute_Set := Normal_Video;
      Color : in Color_Pair := Color_Pair'First)
   is
      function Wattrset (Win    : Window;
                         C_Attr : C_AttrType) return C_Int;
      pragma Import (C, Wattrset, "wattrset"); -- ??? wattr_set
   begin
      if Wattrset (Win,
                   AttrChar_To_AttrType (Attributed_Character'
                                         (Ch    => Character'First,
                                          Color => Color,
                                          Attr  => Attr))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Character_Attributes;

   function Get_Character_Attribute (Win : Window := Standard_Window)
                                     return Character_Attribute_Set
   is
      function Wattrget (Win : Window;
                         Atr : access C_AttrType;
                         Col : access C_Short;
                         Opt : System.Address) return C_Int;
      pragma Import (C, Wattrget, "wattr_get");

      Attr : aliased C_AttrType;
      Col  : aliased C_Short;
      Res  : constant C_Int := Wattrget (Win, Attr'Access, Col'Access,
                                         System.Null_Address);
      Ch   : Attributed_Character;
   begin
      if Res = Curses_Ok then
         Ch := AttrType_To_AttrChar (Attr);
         return Ch.Attr;
      else
         raise Curses_Exception;
      end if;
   end Get_Character_Attribute;

   function Get_Character_Attribute (Win : Window := Standard_Window)
                                     return Color_Pair
   is
      function Wattrget (Win : Window;
                         Atr : access C_AttrType;
                         Col : access C_Short;
                         Opt : System.Address) return C_Int;
      pragma Import (C, Wattrget, "wattr_get");

      Attr : aliased C_AttrType;
      Col  : aliased C_Short;
      Res  : constant C_Int := Wattrget (Win, Attr'Access, Col'Access,
                                         System.Null_Address);
      Ch   : Attributed_Character;
   begin
      if Res = Curses_Ok then
         Ch := AttrType_To_AttrChar (Attr);
         return Ch.Color;
      else
         raise Curses_Exception;
      end if;
   end Get_Character_Attribute;

   procedure Set_Color (Win  : in Window := Standard_Window;
                        Pair : in Color_Pair)
   is
      function Wset_Color (Win   : Window;
                           Color : C_Short;
                           Opts  : C_Void_Ptr) return C_Int;
      pragma Import (C, Wset_Color, "wcolor_set");
   begin
      if Wset_Color (Win,
                     C_Short (Pair),
                     C_Void_Ptr (System.Null_Address)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Color;

   procedure Change_Attributes
     (Win   : in Window := Standard_Window;
      Count : in Integer := -1;
      Attr  : in Character_Attribute_Set := Normal_Video;
      Color : in Color_Pair := Color_Pair'First)
   is
      function Wchgat (Win   : Window;
                       Cnt   : C_Int;
                       Attr  : C_AttrType;
                       Color : C_Short;
                       Opts  : System.Address := System.Null_Address)
                       return C_Int;
      pragma Import (C, Wchgat, "wchgat");

      Ch : constant Attributed_Character :=
        (Ch => Character'First, Color => Color_Pair'First, Attr => Attr);
   begin
      if Wchgat (Win, C_Int (Count), AttrChar_To_AttrType (Ch),
                 C_Short (Color)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Change_Attributes;

   procedure Change_Attributes
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position := Line_Position'First;
      Column : in Column_Position := Column_Position'First;
      Count  : in Integer := -1;
      Attr   : in Character_Attribute_Set := Normal_Video;
      Color  : in Color_Pair := Color_Pair'First)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Change_Attributes (Win, Count, Attr, Color);
   end Change_Attributes;
------------------------------------------------------------------------------
   procedure Beep
   is
      function Beeper return C_Int;
      pragma Import (C, Beeper, "beep");
   begin
      if Beeper = Curses_Err then
         raise Curses_Exception;
      end if;
   end Beep;

   procedure Flash_Screen
   is
      function Flash return C_Int;
      pragma Import (C, Flash, "flash");
   begin
      if Flash = Curses_Err then
         raise Curses_Exception;
      end if;
   end Flash_Screen;
------------------------------------------------------------------------------
   procedure Set_Cbreak_Mode (SwitchOn : in Boolean := True)
   is
      function Cbreak return C_Int;
      pragma Import (C, Cbreak, "cbreak");
      function NoCbreak return C_Int;
      pragma Import (C, NoCbreak, "nocbreak");

      Err : C_Int;
   begin
      if SwitchOn then
         Err := Cbreak;
      else
         Err := NoCbreak;
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Cbreak_Mode;

   procedure Set_Raw_Mode (SwitchOn : in Boolean := True)
   is
      function Raw return C_Int;
      pragma Import (C, Raw, "raw");
      function NoRaw return C_Int;
      pragma Import (C, NoRaw, "noraw");

      Err : C_Int;
   begin
      if SwitchOn then
         Err := Raw;
      else
         Err := NoRaw;
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Raw_Mode;

   procedure Set_Echo_Mode (SwitchOn : in Boolean := True)
   is
      function Echo return C_Int;
      pragma Import (C, Echo, "echo");
      function NoEcho return C_Int;
      pragma Import (C, NoEcho, "noecho");

      Err : C_Int;
   begin
      if SwitchOn then
         Err := Echo;
      else
         Err := NoEcho;
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Echo_Mode;

   procedure Set_Meta_Mode (Win      : in Window := Standard_Window;
                            SwitchOn : in Boolean := True)
   is
      function Meta (W : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Meta, "meta");
   begin
      if Meta (Win, Curses_Bool (Boolean'Pos (SwitchOn))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Meta_Mode;

   procedure Set_KeyPad_Mode (Win      : in Window := Standard_Window;
                              SwitchOn : in Boolean := True)
   is
      function Keypad (W : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Keypad, "keypad");
   begin
      if Keypad (Win, Curses_Bool (Boolean'Pos (SwitchOn))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_KeyPad_Mode;

   function Get_KeyPad_Mode (Win : in Window := Standard_Window)
                             return Boolean
   is
   begin
      return Get_Flag (Win, Offset_use_keypad);
   end Get_KeyPad_Mode;

   procedure Half_Delay (Amount : in Half_Delay_Amount)
   is
      function Halfdelay (Amount : C_Int) return C_Int;
      pragma Import (C, Halfdelay, "halfdelay");
   begin
      if Halfdelay (C_Int (Amount)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Half_Delay;

   procedure Set_Flush_On_Interrupt_Mode
     (Win  : in Window := Standard_Window;
      Mode : in Boolean := True)
   is
      function Intrflush (Win : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Intrflush, "intrflush");
   begin
      if Intrflush (Win, Curses_Bool (Boolean'Pos (Mode))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Flush_On_Interrupt_Mode;

   procedure Set_Queue_Interrupt_Mode
     (Win   : in Window := Standard_Window;
      Flush : in Boolean := True)
   is
      procedure Qiflush;
      pragma Import (C, Qiflush, "qiflush");
      procedure No_Qiflush;
      pragma Import (C, No_Qiflush, "noqiflush");
   begin
      if Win = Null_Window then
         raise Curses_Exception;
      end if;
      if Flush then
         Qiflush;
      else
         No_Qiflush;
      end if;
   end Set_Queue_Interrupt_Mode;

   procedure Set_NoDelay_Mode
     (Win  : in Window := Standard_Window;
      Mode : in Boolean := False)
   is
      function Nodelay (Win : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Nodelay, "nodelay");
   begin
      if Nodelay (Win, Curses_Bool (Boolean'Pos (Mode))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_NoDelay_Mode;

   procedure Set_Timeout_Mode (Win    : in Window := Standard_Window;
                               Mode   : in Timeout_Mode;
                               Amount : in Natural)
   is
      function Wtimeout (Win : Window; Amount : C_Int) return C_Int;
      pragma Import (C, Wtimeout, "wtimeout");

      Time : C_Int;
   begin
      case Mode is
         when Blocking     => Time := -1;
         when Non_Blocking => Time := 0;
         when Delayed      =>
            if Amount = 0 then
               raise Constraint_Error;
            end if;
            Time := C_Int (Amount);
      end case;
      if Wtimeout (Win, Time) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Timeout_Mode;

   procedure Set_Escape_Timer_Mode
     (Win       : in Window := Standard_Window;
      Timer_Off : in Boolean := False)
   is
      function Notimeout (Win : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Notimeout, "notimeout");
   begin
      if Notimeout (Win, Curses_Bool (Boolean'Pos (Timer_Off)))
        = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Escape_Timer_Mode;

------------------------------------------------------------------------------
   procedure Set_NL_Mode (SwitchOn : in Boolean := True)
   is
      function NL return C_Int;
      pragma Import (C, NL, "nl");
      function NoNL return C_Int;
      pragma Import (C, NoNL, "nonl");

      Err : C_Int;
   begin
      if SwitchOn then
         Err := NL;
      else
         Err := NoNL;
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_NL_Mode;

   procedure Clear_On_Next_Update
     (Win      : in Window := Standard_Window;
      Do_Clear : in Boolean := True)
   is
      function Clear_Ok (W : Window; Flag : Curses_Bool) return C_Int;
      pragma Import (C, Clear_Ok, "clearok");
   begin
      if Clear_Ok (Win, Curses_Bool (Boolean'Pos (Do_Clear))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Clear_On_Next_Update;

   procedure Use_Insert_Delete_Line
     (Win    : in Window := Standard_Window;
      Do_Idl : in Boolean := True)
   is
      function IDL_Ok (W : Window; Flag : Curses_Bool) return C_Int;
      pragma Import (C, IDL_Ok, "idlok");
   begin
      if IDL_Ok (Win, Curses_Bool (Boolean'Pos (Do_Idl))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Use_Insert_Delete_Line;

   procedure Use_Insert_Delete_Character
     (Win    : in Window := Standard_Window;
      Do_Idc : in Boolean := True)
   is
      function IDC_Ok (W : Window; Flag : Curses_Bool) return C_Int;
      pragma Import (C, IDC_Ok, "idcok");
   begin
      if IDC_Ok (Win, Curses_Bool (Boolean'Pos (Do_Idc))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Use_Insert_Delete_Character;

   procedure Leave_Cursor_After_Update
     (Win      : in Window := Standard_Window;
      Do_Leave : in Boolean := True)
   is
      function Leave_Ok (W : Window; Flag : Curses_Bool) return C_Int;
      pragma Import (C, Leave_Ok, "leaveok");
   begin
      if Leave_Ok (Win, Curses_Bool (Boolean'Pos (Do_Leave))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Leave_Cursor_After_Update;

   procedure Immediate_Update_Mode
     (Win  : in Window := Standard_Window;
      Mode : in Boolean := False)
   is
      function Immedok (Win : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Immedok, "immedok");
   begin
      if Immedok (Win, Curses_Bool (Boolean'Pos (Mode))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Immediate_Update_Mode;

   procedure Allow_Scrolling
     (Win  : in Window  := Standard_Window;
      Mode : in Boolean := False)
   is
      function Scrollok (Win : Window; Mode : Curses_Bool) return C_Int;
      pragma Import (C, Scrollok, "scrollok");
   begin
      if Scrollok (Win, Curses_Bool (Boolean'Pos (Mode))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Allow_Scrolling;

   function Scrolling_Allowed (Win : Window := Standard_Window)
                               return Boolean
   is
   begin
      return Get_Flag (Win, Offset_scroll);
   end Scrolling_Allowed;

   procedure Set_Scroll_Region
     (Win         : in Window := Standard_Window;
      Top_Line    : in Line_Position;
      Bottom_Line : in Line_Position)
   is
      function Wsetscrreg (Win : Window;
                           Lin : C_Int;
                           Col : C_Int) return C_Int;
      pragma Import (C, Wsetscrreg, "wsetscrreg");
   begin
      if Wsetscrreg (Win, C_Int (Top_Line), C_Int (Bottom_Line))
        = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Scroll_Region;
------------------------------------------------------------------------------
   procedure Update_Screen
   is
      function Do_Update return C_Int;
      pragma Import (C, Do_Update, "doupdate");
   begin
      if Do_Update = Curses_Err then
         raise Curses_Exception;
      end if;
   end Update_Screen;

   procedure Refresh (Win : in Window := Standard_Window)
   is
      function Wrefresh (W : Window) return C_Int;
      pragma Import (C, Wrefresh, "wrefresh");
   begin
      if Wrefresh (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh;

   procedure Refresh_Without_Update
     (Win : in Window := Standard_Window)
   is
      function Wnoutrefresh (W : Window) return C_Int;
      pragma Import (C, Wnoutrefresh, "wnoutrefresh");
   begin
      if Wnoutrefresh (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh_Without_Update;

   procedure Redraw (Win : in Window := Standard_Window)
   is
      function Redrawwin (Win : Window) return C_Int;
      pragma Import (C, Redrawwin, "redrawwin");
   begin
      if Redrawwin (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Redraw;

   procedure Redraw
     (Win        : in Window := Standard_Window;
      Begin_Line : in Line_Position;
      Line_Count : in Positive)
   is
      function Wredrawln (Win : Window; First : C_Int; Cnt : C_Int)
                          return C_Int;
      pragma Import (C, Wredrawln, "wredrawln");
   begin
      if Wredrawln (Win,
                    C_Int (Begin_Line),
                    C_Int (Line_Count)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Redraw;

------------------------------------------------------------------------------
   procedure Erase (Win : in Window := Standard_Window)
   is
      function Werase (W : Window) return C_Int;
      pragma Import (C, Werase, "werase");
   begin
      if Werase (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Erase;

   procedure Clear (Win : in Window := Standard_Window)
   is
      function Wclear (W : Window) return C_Int;
      pragma Import (C, Wclear, "wclear");
   begin
      if Wclear (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Clear;

   procedure Clear_To_End_Of_Screen (Win : in Window := Standard_Window)
   is
      function Wclearbot (W : Window) return C_Int;
      pragma Import (C, Wclearbot, "wclrtobot");
   begin
      if Wclearbot (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Clear_To_End_Of_Screen;

   procedure Clear_To_End_Of_Line (Win : in Window := Standard_Window)
   is
      function Wcleareol (W : Window) return C_Int;
      pragma Import (C, Wcleareol, "wclrtoeol");
   begin
      if Wcleareol (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Clear_To_End_Of_Line;
------------------------------------------------------------------------------
   procedure Set_Background
     (Win : in Window := Standard_Window;
      Ch  : in Attributed_Character)
   is
      procedure WBackground (W : in Window; Ch : in C_Chtype);
      pragma Import (C, WBackground, "wbkgdset");
   begin
      WBackground (Win, AttrChar_To_Chtype (Ch));
   end Set_Background;

   procedure Change_Background
     (Win : in Window := Standard_Window;
      Ch  : in Attributed_Character)
   is
      function WChangeBkgd (W : Window; Ch : C_Chtype) return C_Int;
      pragma Import (C, WChangeBkgd, "wbkgd");
   begin
      if WChangeBkgd (Win, AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Change_Background;

   function Get_Background (Win : Window := Standard_Window)
     return Attributed_Character
   is
      function Wgetbkgd (Win : Window) return C_Chtype;
      pragma Import (C, Wgetbkgd, "getbkgd");
   begin
      return Chtype_To_AttrChar (Wgetbkgd (Win));
   end Get_Background;
------------------------------------------------------------------------------
   procedure Change_Lines_Status (Win   : in Window := Standard_Window;
                                  Start : in Line_Position;
                                  Count : in Positive;
                                  State : in Boolean)
   is
      function Wtouchln (Win : Window;
                         Sta : C_Int;
                         Cnt : C_Int;
                         Chg : C_Int) return C_Int;
      pragma Import (C, Wtouchln, "wtouchln");
   begin
      if Wtouchln (Win, C_Int (Start), C_Int (Count),
                   C_Int (Boolean'Pos (State))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Change_Lines_Status;

   procedure Touch (Win : in Window := Standard_Window)
   is
      Y : Line_Position;
      X : Column_Position;
   begin
      Get_Size (Win, Y, X);
      Change_Lines_Status (Win, 0, Positive (Y), True);
   end Touch;

   procedure Untouch (Win : in Window := Standard_Window)
   is
      Y : Line_Position;
      X : Column_Position;
   begin
      Get_Size (Win, Y, X);
      Change_Lines_Status (Win, 0, Positive (Y), False);
   end Untouch;

   procedure Touch (Win   : in Window := Standard_Window;
                    Start : in Line_Position;
                    Count : in Positive)
   is
   begin
      Change_Lines_Status (Win, Start, Count, True);
   end Touch;

   function Is_Touched
     (Win  : Window := Standard_Window;
      Line : Line_Position) return Boolean
   is
      function WLineTouched (W : Window; L : C_Int) return Curses_Bool;
      pragma Import (C, WLineTouched, "is_linetouched");
   begin
      if WLineTouched (Win, C_Int (Line)) = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Is_Touched;

   function Is_Touched
     (Win : Window := Standard_Window) return Boolean
   is
      function WWinTouched (W : Window) return Curses_Bool;
      pragma Import (C, WWinTouched, "is_wintouched");
   begin
      if WWinTouched (Win) = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Is_Touched;
------------------------------------------------------------------------------
   procedure Copy
     (Source_Window            : in Window;
      Destination_Window       : in Window;
      Source_Top_Row           : in Line_Position;
      Source_Left_Column       : in Column_Position;
      Destination_Top_Row      : in Line_Position;
      Destination_Left_Column  : in Column_Position;
      Destination_Bottom_Row   : in Line_Position;
      Destination_Right_Column : in Column_Position;
      Non_Destructive_Mode     : in Boolean := True)
   is
      function Copywin (Src : Window;
                        Dst : Window;
                        Str : C_Int;
                        Slc : C_Int;
                        Dtr : C_Int;
                        Dlc : C_Int;
                        Dbr : C_Int;
                        Drc : C_Int;
                        Ndm : C_Int) return C_Int;
      pragma Import (C, Copywin, "copywin");
   begin
      if Copywin (Source_Window,
                  Destination_Window,
                  C_Int (Source_Top_Row),
                  C_Int (Source_Left_Column),
                  C_Int (Destination_Top_Row),
                  C_Int (Destination_Left_Column),
                  C_Int (Destination_Bottom_Row),
                  C_Int (Destination_Right_Column),
                  Boolean'Pos (Non_Destructive_Mode)
                ) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Copy;

   procedure Overwrite
     (Source_Window      : in Window;
      Destination_Window : in Window)
   is
      function Overwrite (Src : Window; Dst : Window) return C_Int;
      pragma Import (C, Overwrite, "overwrite");
   begin
      if Overwrite (Source_Window, Destination_Window) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Overwrite;

   procedure Overlay
     (Source_Window      : in Window;
      Destination_Window : in Window)
   is
      function Overlay (Src : Window; Dst : Window) return C_Int;
      pragma Import (C, Overlay, "overlay");
   begin
      if Overlay (Source_Window, Destination_Window) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Overlay;

------------------------------------------------------------------------------
   procedure Insert_Delete_Lines
     (Win   : in Window := Standard_Window;
      Lines : in Integer       := 1) -- default is to insert one line above
   is
      function Winsdelln (W : Window; N : C_Int) return C_Int;
      pragma Import (C, Winsdelln, "winsdelln");
   begin
      if Winsdelln (Win, C_Int (Lines)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Insert_Delete_Lines;

   procedure Delete_Line (Win : in Window := Standard_Window)
   is
   begin
      Insert_Delete_Lines (Win, -1);
   end Delete_Line;

   procedure Insert_Line (Win : in Window := Standard_Window)
   is
   begin
      Insert_Delete_Lines (Win, 1);
   end Insert_Line;
------------------------------------------------------------------------------


   procedure Get_Size
     (Win               : in Window := Standard_Window;
      Number_Of_Lines   : out Line_Count;
      Number_Of_Columns : out Column_Count)
   is
      --  Please note: in ncurses they are one off.
      --  This might be different in other implementations of curses
      Y : constant C_Int := C_Int (W_Get_Short (Win, Offset_maxy))
                          + C_Int (Offset_XY);
      X : constant C_Int := C_Int (W_Get_Short (Win, Offset_maxx))
                          + C_Int (Offset_XY);
   begin
      Number_Of_Lines   := Line_Count (Y);
      Number_Of_Columns := Column_Count (X);
   end Get_Size;

   procedure Get_Window_Position
     (Win             : in Window := Standard_Window;
      Top_Left_Line   : out Line_Position;
      Top_Left_Column : out Column_Position)
   is
      Y : constant C_Short := W_Get_Short (Win, Offset_begy);
      X : constant C_Short := W_Get_Short (Win, Offset_begx);
   begin
      Top_Left_Line   := Line_Position (Y);
      Top_Left_Column := Column_Position (X);
   end Get_Window_Position;

   procedure Get_Cursor_Position
     (Win    : in  Window := Standard_Window;
      Line   : out Line_Position;
      Column : out Column_Position)
   is
      Y : constant C_Short := W_Get_Short (Win, Offset_cury);
      X : constant C_Short := W_Get_Short (Win, Offset_curx);
   begin
      Line   := Line_Position (Y);
      Column := Column_Position (X);
   end Get_Cursor_Position;

   procedure Get_Origin_Relative_To_Parent
     (Win                : in  Window;
      Top_Left_Line      : out Line_Position;
      Top_Left_Column    : out Column_Position;
      Is_Not_A_Subwindow : out Boolean)
   is
      Y : constant C_Int := W_Get_Int (Win, Offset_pary);
      X : constant C_Int := W_Get_Int (Win, Offset_parx);
   begin
      if Y = -1 then
         Top_Left_Line   := Line_Position'Last;
         Top_Left_Column := Column_Position'Last;
         Is_Not_A_Subwindow := True;
      else
         Top_Left_Line   := Line_Position (Y);
         Top_Left_Column := Column_Position (X);
         Is_Not_A_Subwindow := False;
      end if;
   end Get_Origin_Relative_To_Parent;
------------------------------------------------------------------------------
   function New_Pad (Lines   : Line_Count;
                     Columns : Column_Count) return Window
   is
      function Newpad (Lines : C_Int; Columns : C_Int) return Window;
      pragma Import (C, Newpad, "newpad");

      W : Window;
   begin
      W := Newpad (C_Int (Lines), C_Int (Columns));
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end New_Pad;

   function Sub_Pad
     (Pad                   : Window;
      Number_Of_Lines       : Line_Count;
      Number_Of_Columns     : Column_Count;
      First_Line_Position   : Line_Position;
      First_Column_Position : Column_Position) return Window
   is
      function Subpad
        (Pad                   : Window;
         Number_Of_Lines       : C_Int;
         Number_Of_Columns     : C_Int;
         First_Line_Position   : C_Int;
         First_Column_Position : C_Int) return Window;
      pragma Import (C, Subpad, "subpad");

      W : Window;
   begin
      W := Subpad (Pad,
                   C_Int (Number_Of_Lines),
                   C_Int (Number_Of_Columns),
                   C_Int (First_Line_Position),
                   C_Int (First_Column_Position));
      if W = Null_Window then
         raise Curses_Exception;
      end if;
      return W;
   end Sub_Pad;

   procedure Refresh
     (Pad                      : in Window;
      Source_Top_Row           : in Line_Position;
      Source_Left_Column       : in Column_Position;
      Destination_Top_Row      : in Line_Position;
      Destination_Left_Column  : in Column_Position;
      Destination_Bottom_Row   : in Line_Position;
      Destination_Right_Column : in Column_Position)
   is
      function Prefresh
        (Pad                      : Window;
         Source_Top_Row           : C_Int;
         Source_Left_Column       : C_Int;
         Destination_Top_Row      : C_Int;
         Destination_Left_Column  : C_Int;
         Destination_Bottom_Row   : C_Int;
         Destination_Right_Column : C_Int) return C_Int;
      pragma Import (C, Prefresh, "prefresh");
   begin
      if Prefresh (Pad,
                   C_Int (Source_Top_Row),
                   C_Int (Source_Left_Column),
                   C_Int (Destination_Top_Row),
                   C_Int (Destination_Left_Column),
                   C_Int (Destination_Bottom_Row),
                   C_Int (Destination_Right_Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh;

   procedure Refresh_Without_Update
     (Pad                      : in Window;
      Source_Top_Row           : in Line_Position;
      Source_Left_Column       : in Column_Position;
      Destination_Top_Row      : in Line_Position;
      Destination_Left_Column  : in Column_Position;
      Destination_Bottom_Row   : in Line_Position;
      Destination_Right_Column : in Column_Position)
   is
      function Pnoutrefresh
        (Pad                      : Window;
         Source_Top_Row           : C_Int;
         Source_Left_Column       : C_Int;
         Destination_Top_Row      : C_Int;
         Destination_Left_Column  : C_Int;
         Destination_Bottom_Row   : C_Int;
         Destination_Right_Column : C_Int) return C_Int;
      pragma Import (C, Pnoutrefresh, "pnoutrefresh");
   begin
      if Pnoutrefresh (Pad,
                       C_Int (Source_Top_Row),
                       C_Int (Source_Left_Column),
                       C_Int (Destination_Top_Row),
                       C_Int (Destination_Left_Column),
                       C_Int (Destination_Bottom_Row),
                       C_Int (Destination_Right_Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh_Without_Update;

   procedure Add_Character_To_Pad_And_Echo_It
     (Pad : in Window;
      Ch  : in Attributed_Character)
   is
      function Pechochar (Pad : Window; Ch : C_Chtype)
                          return C_Int;
      pragma Import (C, Pechochar, "pechochar");
   begin
      if Pechochar (Pad, AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Add_Character_To_Pad_And_Echo_It;

   procedure Add_Character_To_Pad_And_Echo_It
     (Pad : in Window;
      Ch  : in Character)
   is
   begin
      Add_Character_To_Pad_And_Echo_It
        (Pad,
         Attributed_Character'(Ch    => Ch,
                               Color => Color_Pair'First,
                               Attr  => Normal_Video));
   end Add_Character_To_Pad_And_Echo_It;
------------------------------------------------------------------------------
   procedure Scroll (Win    : in Window := Standard_Window;
                     Amount : in Integer := 1)
   is
      function Wscrl (Win : Window; N : C_Int) return C_Int;
      pragma Import (C, Wscrl, "wscrl");

   begin
      if Wscrl (Win, C_Int (Amount)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Scroll;

------------------------------------------------------------------------------
   procedure Delete_Character (Win : in Window := Standard_Window)
   is
      function Wdelch (Win : Window) return C_Int;
      pragma Import (C, Wdelch, "wdelch");
   begin
      if Wdelch (Win) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Delete_Character;

   procedure Delete_Character
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position)
   is
      function Mvwdelch (Win : Window;
                         Lin : C_Int;
                         Col : C_Int) return C_Int;
      pragma Import (C, Mvwdelch, "mvwdelch");
   begin
      if Mvwdelch (Win, C_Int (Line), C_Int (Column)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Delete_Character;
------------------------------------------------------------------------------
   function Peek (Win : Window := Standard_Window)
     return Attributed_Character
   is
      function Winch (Win : Window) return C_Chtype;
      pragma Import (C, Winch, "winch");
   begin
      return Chtype_To_AttrChar (Winch (Win));
   end Peek;

   function Peek
     (Win    : Window := Standard_Window;
      Line   : Line_Position;
      Column : Column_Position) return Attributed_Character
   is
      function Mvwinch (Win : Window;
                        Lin : C_Int;
                        Col : C_Int) return C_Chtype;
      pragma Import (C, Mvwinch, "mvwinch");
   begin
      return Chtype_To_AttrChar (Mvwinch (Win, C_Int (Line), C_Int (Column)));
   end Peek;
------------------------------------------------------------------------------
   procedure Insert (Win : in Window := Standard_Window;
                     Ch  : in Attributed_Character)
   is
      function Winsch (Win : Window; Ch : C_Chtype) return C_Int;
      pragma Import (C, Winsch, "winsch");
   begin
      if Winsch (Win, AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Insert;

   procedure Insert
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Ch     : in Attributed_Character)
   is
      function Mvwinsch (Win : Window;
                         Lin : C_Int;
                         Col : C_Int;
                         Ch  : C_Chtype) return C_Int;
      pragma Import (C, Mvwinsch, "mvwinsch");
   begin
      if Mvwinsch (Win,
                   C_Int (Line),
                   C_Int (Column),
                   AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Insert;
------------------------------------------------------------------------------
   procedure Insert (Win : in Window := Standard_Window;
                     Str : in String;
                     Len : in Integer := -1)
   is
      function Winsnstr (Win : Window;
                         Str : char_array;
                         Len : Integer := -1) return C_Int;
      pragma Import (C, Winsnstr, "winsnstr");

      Txt    : char_array (0 .. Str'Length);
      Length : size_t;
   begin
      To_C (Str, Txt, Length);
      if Winsnstr (Win, Txt, Len) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Insert;

   procedure Insert
     (Win    : in Window := Standard_Window;
      Line   : in Line_Position;
      Column : in Column_Position;
      Str    : in String;
      Len    : in Integer := -1)
   is
      function Mvwinsnstr (Win    : Window;
                           Line   : C_Int;
                           Column : C_Int;
                           Str    : char_array;
                           Len    : C_Int) return C_Int;
      pragma Import (C, Mvwinsnstr, "mvwinsnstr");

      Txt    : char_array (0 .. Str'Length);
      Length : size_t;
   begin
      To_C (Str, Txt, Length);
      if Mvwinsnstr (Win, C_Int (Line), C_Int (Column), Txt, C_Int (Len))
        = Curses_Err then
         raise Curses_Exception;
      end if;
   end Insert;
------------------------------------------------------------------------------
   procedure Peek (Win : in  Window := Standard_Window;
                   Str : out String;
                   Len : in  Integer := -1)
   is
      function Winnstr (Win : Window;
                        Str : char_array;
                        Len : C_Int) return C_Int;
      pragma Import (C, Winnstr, "winnstr");

      N   : Integer := Len;
      Txt : char_array (0 .. Str'Length);
      Cnt : Natural;
   begin
      if N < 0 then
         N := Str'Length;
      end if;
      if N > Str'Length then
         raise Constraint_Error;
      end if;
      Txt (0) := Interfaces.C.char'First;
      if Winnstr (Win, Txt, C_Int (N)) = Curses_Err then
         raise Curses_Exception;
      end if;
      To_Ada (Txt, Str, Cnt, True);
      if Cnt < Str'Length then
         Str ((Str'First + Cnt) .. Str'Last) := (others => ' ');
      end if;
   end Peek;

   procedure Peek
     (Win    : in  Window := Standard_Window;
      Line   : in  Line_Position;
      Column : in  Column_Position;
      Str    : out String;
      Len    : in  Integer := -1)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Peek (Win, Str, Len);
   end Peek;
------------------------------------------------------------------------------
   procedure Peek
     (Win : in  Window := Standard_Window;
      Str : out Attributed_String;
      Len : in  Integer := -1)
   is
      function Winchnstr (Win : Window;
                          Str : chtype_array;             -- out
                          Len : C_Int) return C_Int;
      pragma Import (C, Winchnstr, "winchnstr");

      N   : Integer := Len;
      Txt : constant chtype_array (0 .. Str'Length)
          := (0 => Default_Character);
      Cnt : Natural := 0;
   begin
      if N < 0 then
         N := Str'Length;
      end if;
      if N > Str'Length then
         raise Constraint_Error;
      end if;
      if Winchnstr (Win, Txt, C_Int (N)) = Curses_Err then
         raise Curses_Exception;
      end if;
      for To in Str'Range loop
         exit when Txt (size_t (Cnt)) = Default_Character;
         Str (To) := Txt (size_t (Cnt));
         Cnt := Cnt + 1;
      end loop;
      if Cnt < Str'Length then
         Str ((Str'First + Cnt) .. Str'Last) :=
           (others => (Ch => ' ',
                       Color => Color_Pair'First,
                       Attr => Normal_Video));
      end if;
   end Peek;

   procedure Peek
     (Win    : in  Window := Standard_Window;
      Line   : in  Line_Position;
      Column : in  Column_Position;
      Str    : out Attributed_String;
      Len    : in Integer := -1)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Peek (Win, Str, Len);
   end Peek;
------------------------------------------------------------------------------
   procedure Get (Win : in  Window := Standard_Window;
                  Str : out String;
                  Len : in  Integer := -1)
   is
      function Wgetnstr (Win : Window;
                         Str : char_array;
                         Len : C_Int) return C_Int;
      pragma Import (C, Wgetnstr, "wgetnstr");

      N   : Integer := Len;
      Txt : char_array (0 .. Str'Length);
      Cnt : Natural;
   begin
      if N < 0 then
         N := Str'Length;
      end if;
      if N > Str'Length then
         raise Constraint_Error;
      end if;
      Txt (0) := Interfaces.C.char'First;
      if Wgetnstr (Win, Txt, C_Int (N)) = Curses_Err then
         raise Curses_Exception;
      end if;
      To_Ada (Txt, Str, Cnt, True);
      if Cnt < Str'Length then
         Str ((Str'First + Cnt) .. Str'Last) := (others => ' ');
      end if;
   end Get;

   procedure Get
     (Win    : in  Window := Standard_Window;
      Line   : in  Line_Position;
      Column : in  Column_Position;
      Str    : out String;
      Len    : in  Integer := -1)
   is
   begin
      Move_Cursor (Win, Line, Column);
      Get (Win, Str, Len);
   end Get;
------------------------------------------------------------------------------
   procedure Init_Soft_Label_Keys
     (Format : in Soft_Label_Key_Format := Three_Two_Three)
   is
      function Slk_Init (Fmt : C_Int) return C_Int;
      pragma Import (C, Slk_Init, "slk_init");
   begin
      if Slk_Init (Soft_Label_Key_Format'Pos (Format)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Init_Soft_Label_Keys;

   procedure Set_Soft_Label_Key (Label : in Label_Number;
                                 Text  : in String;
                                 Fmt   : in Label_Justification := Left)
   is
      function Slk_Set (Label : C_Int;
                        Txt   : char_array;
                        Fmt   : C_Int) return C_Int;
      pragma Import (C, Slk_Set, "slk_set");

      Txt : char_array (0 .. Text'Length);
      Len : size_t;
   begin
      To_C (Text, Txt, Len);
      if Slk_Set (C_Int (Label), Txt,
                  C_Int (Label_Justification'Pos (Fmt))) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Soft_Label_Key;

   procedure Refresh_Soft_Label_Keys
   is
      function Slk_Refresh return C_Int;
      pragma Import (C, Slk_Refresh, "slk_refresh");
   begin
      if Slk_Refresh = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh_Soft_Label_Keys;

   procedure Refresh_Soft_Label_Keys_Without_Update
   is
      function Slk_Noutrefresh return C_Int;
      pragma Import (C, Slk_Noutrefresh, "slk_noutrefresh");
   begin
      if Slk_Noutrefresh = Curses_Err then
         raise Curses_Exception;
      end if;
   end Refresh_Soft_Label_Keys_Without_Update;

   procedure Get_Soft_Label_Key (Label : in Label_Number;
                                 Text  : out String)
   is
      function Slk_Label (Label : C_Int) return chars_ptr;
      pragma Import (C, Slk_Label, "slk_label");
   begin
      Fill_String (Slk_Label (C_Int (Label)), Text);
   end Get_Soft_Label_Key;

   function Get_Soft_Label_Key (Label : in Label_Number) return String
   is
      function Slk_Label (Label : C_Int) return chars_ptr;
      pragma Import (C, Slk_Label, "slk_label");
   begin
      return Fill_String (Slk_Label (C_Int (Label)));
   end Get_Soft_Label_Key;

   procedure Clear_Soft_Label_Keys
   is
      function Slk_Clear return C_Int;
      pragma Import (C, Slk_Clear, "slk_clear");
   begin
      if Slk_Clear = Curses_Err then
         raise Curses_Exception;
      end if;
   end Clear_Soft_Label_Keys;

   procedure Restore_Soft_Label_Keys
   is
      function Slk_Restore return C_Int;
      pragma Import (C, Slk_Restore, "slk_restore");
   begin
      if Slk_Restore = Curses_Err then
         raise Curses_Exception;
      end if;
   end Restore_Soft_Label_Keys;

   procedure Touch_Soft_Label_Keys
   is
      function Slk_Touch return C_Int;
      pragma Import (C, Slk_Touch, "slk_touch");
   begin
      if Slk_Touch = Curses_Err then
         raise Curses_Exception;
      end if;
   end Touch_Soft_Label_Keys;

   procedure Switch_Soft_Label_Key_Attributes
     (Attr : in Character_Attribute_Set;
      On   : in Boolean := True)
   is
      function Slk_Attron (Ch : C_Chtype) return C_Int;
      pragma Import (C, Slk_Attron, "slk_attron");
      function Slk_Attroff (Ch : C_Chtype) return C_Int;
      pragma Import (C, Slk_Attroff, "slk_attroff");

      Err : C_Int;
      Ch  : constant Attributed_Character := (Ch    => Character'First,
                                              Attr  => Attr,
                                              Color => Color_Pair'First);
   begin
      if On then
         Err := Slk_Attron  (AttrChar_To_Chtype (Ch));
      else
         Err := Slk_Attroff (AttrChar_To_Chtype (Ch));
      end if;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Switch_Soft_Label_Key_Attributes;

   procedure Set_Soft_Label_Key_Attributes
     (Attr  : in Character_Attribute_Set := Normal_Video;
      Color : in Color_Pair := Color_Pair'First)
   is
      function Slk_Attrset (Ch : C_Chtype) return C_Int;
      pragma Import (C, Slk_Attrset, "slk_attrset");

      Ch : constant Attributed_Character := (Ch    => Character'First,
                                             Attr  => Attr,
                                             Color => Color);
   begin
      if Slk_Attrset (AttrChar_To_Chtype (Ch)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Soft_Label_Key_Attributes;

   function Get_Soft_Label_Key_Attributes return Character_Attribute_Set
   is
      function Slk_Attr return C_Chtype;
      pragma Import (C, Slk_Attr, "slk_attr");

      Attr : constant C_Chtype := Slk_Attr;
   begin
      return Chtype_To_AttrChar (Attr).Attr;
   end Get_Soft_Label_Key_Attributes;

   function Get_Soft_Label_Key_Attributes return Color_Pair
   is
      function Slk_Attr return C_Chtype;
      pragma Import (C, Slk_Attr, "slk_attr");

      Attr : constant C_Chtype := Slk_Attr;
   begin
      return Chtype_To_AttrChar (Attr).Color;
   end Get_Soft_Label_Key_Attributes;

   procedure Set_Soft_Label_Key_Color (Pair : in Color_Pair)
   is
      function Slk_Color (Color : in C_Short) return C_Int;
      pragma Import (C, Slk_Color, "slk_color");
   begin
      if Slk_Color (C_Short (Pair)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Set_Soft_Label_Key_Color;

------------------------------------------------------------------------------
   procedure Enable_Key (Key    : in Special_Key_Code;
                         Enable : in Boolean := True)
   is
      function Keyok (Keycode : C_Int;
                      On_Off  : Curses_Bool) return C_Int;
      pragma Import (C, Keyok, "keyok");
   begin
      if Keyok (C_Int (Key), Curses_Bool (Boolean'Pos (Enable)))
        = Curses_Err then
         raise Curses_Exception;
      end if;
   end Enable_Key;
------------------------------------------------------------------------------
   procedure Define_Key (Definition : in String;
                         Key        : in Special_Key_Code)
   is
      function Defkey (Def : char_array;
                       Key : C_Int) return C_Int;
      pragma Import (C, Defkey, "define_key");

      Txt    : char_array (0 .. Definition'Length);
      Length : size_t;
   begin
      To_C (Definition, Txt, Length);
      if Defkey (Txt, C_Int (Key)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Define_Key;
------------------------------------------------------------------------------
   procedure Un_Control (Ch  : in Attributed_Character;
                         Str : out String)
   is
      function Unctrl (Ch : C_Chtype) return chars_ptr;
      pragma Import (C, Unctrl, "unctrl");
   begin
      Fill_String (Unctrl (AttrChar_To_Chtype (Ch)), Str);
   end Un_Control;

   function Un_Control (Ch : in Attributed_Character) return String
   is
      function Unctrl (Ch : C_Chtype) return chars_ptr;
      pragma Import (C, Unctrl, "unctrl");
   begin
      return Fill_String (Unctrl (AttrChar_To_Chtype (Ch)));
   end Un_Control;

   procedure Delay_Output (Msecs : in Natural)
   is
      function Delayoutput (Msecs : C_Int) return C_Int;
      pragma Import (C, Delayoutput, "delay_output");
   begin
      if Delayoutput (C_Int (Msecs)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Delay_Output;

   procedure Flush_Input
   is
      function Flushinp return C_Int;
      pragma Import (C, Flushinp, "flushinp");
   begin
      if Flushinp = Curses_Err then  -- docu says that never happens, but...
         raise Curses_Exception;
      end if;
   end Flush_Input;
------------------------------------------------------------------------------
   function Baudrate return Natural
   is
      function Baud return C_Int;
      pragma Import (C, Baud, "baudrate");
   begin
      return Natural (Baud);
   end Baudrate;

   function Erase_Character return Character
   is
      function Erasechar return C_Int;
      pragma Import (C, Erasechar, "erasechar");
   begin
      return Character'Val (Erasechar);
   end Erase_Character;

   function Kill_Character return Character
   is
      function Killchar return C_Int;
      pragma Import (C, Killchar, "killchar");
   begin
      return Character'Val (Killchar);
   end Kill_Character;

   function Has_Insert_Character return Boolean
   is
      function Has_Ic return Curses_Bool;
      pragma Import (C, Has_Ic, "has_ic");
   begin
      if Has_Ic = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Has_Insert_Character;

   function Has_Insert_Line return Boolean
   is
      function Has_Il return Curses_Bool;
      pragma Import (C, Has_Il, "has_il");
   begin
      if Has_Il = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Has_Insert_Line;

   function Supported_Attributes return Character_Attribute_Set
   is
      function Termattrs return C_Chtype;
      pragma Import (C, Termattrs, "termattrs");

      Ch : constant Attributed_Character := Chtype_To_AttrChar (Termattrs);
   begin
      return Ch.Attr;
   end Supported_Attributes;

   procedure Long_Name (Name : out String)
   is
      function Longname return chars_ptr;
      pragma Import (C, Longname, "longname");
   begin
      Fill_String (Longname, Name);
   end Long_Name;

   function Long_Name return String
   is
      function Longname return chars_ptr;
      pragma Import (C, Longname, "longname");
   begin
      return Fill_String (Longname);
   end Long_Name;

   procedure Terminal_Name (Name : out String)
   is
      function Termname return chars_ptr;
      pragma Import (C, Termname, "termname");
   begin
      Fill_String (Termname, Name);
   end Terminal_Name;

   function Terminal_Name return String
   is
      function Termname return chars_ptr;
      pragma Import (C, Termname, "termname");
   begin
      return Fill_String (Termname);
   end Terminal_Name;
------------------------------------------------------------------------------
   procedure Init_Pair (Pair : in Redefinable_Color_Pair;
                        Fore : in Color_Number;
                        Back : in Color_Number)
   is
      function Initpair (Pair : C_Short;
                         Fore : C_Short;
                         Back : C_Short) return C_Int;
      pragma Import (C, Initpair, "init_pair");
   begin
      if Integer (Pair) >= Number_Of_Color_Pairs then
         raise Constraint_Error;
      end if;
      if Integer (Fore) >= Number_Of_Colors or else
        Integer (Back) >= Number_Of_Colors then raise Constraint_Error;
      end if;
      if Initpair (C_Short (Pair), C_Short (Fore), C_Short (Back))
        = Curses_Err then
         raise Curses_Exception;
      end if;
   end Init_Pair;

   procedure Pair_Content (Pair : in Color_Pair;
                           Fore : out Color_Number;
                           Back : out Color_Number)
   is
      type C_Short_Access is access all C_Short;
      function Paircontent (Pair : C_Short;
                            Fp   : C_Short_Access;
                            Bp   : C_Short_Access) return C_Int;
      pragma Import (C, Paircontent, "pair_content");

      F, B : aliased C_Short;
   begin
      if Paircontent (C_Short (Pair), F'Access, B'Access) = Curses_Err then
         raise Curses_Exception;
      else
         Fore := Color_Number (F);
         Back := Color_Number (B);
      end if;
   end Pair_Content;

   function Has_Colors return Boolean
   is
      function Hascolors return Curses_Bool;
      pragma Import (C, Hascolors, "has_colors");
   begin
      if Hascolors = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Has_Colors;

   procedure Init_Color (Color : in Color_Number;
                         Red   : in RGB_Value;
                         Green : in RGB_Value;
                         Blue  : in RGB_Value)
   is
      function Initcolor (Col   : C_Short;
                          Red   : C_Short;
                          Green : C_Short;
                          Blue  : C_Short) return C_Int;
      pragma Import (C, Initcolor, "init_color");
   begin
      if Initcolor (C_Short (Color), C_Short (Red), C_Short (Green),
                    C_Short (Blue)) = Curses_Err then
            raise Curses_Exception;
      end if;
   end Init_Color;

   function Can_Change_Color return Boolean
   is
      function Canchangecolor return Curses_Bool;
      pragma Import (C, Canchangecolor, "can_change_color");
   begin
      if Canchangecolor = Curses_Bool_False then
         return False;
      else
         return True;
      end if;
   end Can_Change_Color;

   procedure Color_Content (Color : in  Color_Number;
                            Red   : out RGB_Value;
                            Green : out RGB_Value;
                            Blue  : out RGB_Value)
   is
      type C_Short_Access is access all C_Short;

      function Colorcontent (Color : C_Short; R, G, B : C_Short_Access)
                             return C_Int;
      pragma Import (C, Colorcontent, "color_content");

      R, G, B : aliased C_Short;
   begin
      if Colorcontent (C_Short (Color), R'Access, G'Access, B'Access) =
        Curses_Err then
         raise Curses_Exception;
      else
         Red   := RGB_Value (R);
         Green := RGB_Value (G);
         Blue  := RGB_Value (B);
      end if;
   end Color_Content;

------------------------------------------------------------------------------
   procedure Save_Curses_Mode (Mode : in Curses_Mode)
   is
      function Def_Prog_Mode return C_Int;
      pragma Import (C, Def_Prog_Mode, "def_prog_mode");
      function Def_Shell_Mode return C_Int;
      pragma Import (C, Def_Shell_Mode, "def_shell_mode");

      Err : C_Int;
   begin
      case Mode is
         when Curses => Err := Def_Prog_Mode;
         when Shell  => Err := Def_Shell_Mode;
      end case;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Save_Curses_Mode;

   procedure Reset_Curses_Mode (Mode : in Curses_Mode)
   is
      function Reset_Prog_Mode return C_Int;
      pragma Import (C, Reset_Prog_Mode, "reset_prog_mode");
      function Reset_Shell_Mode return C_Int;
      pragma Import (C, Reset_Shell_Mode, "reset_shell_mode");

      Err : C_Int;
   begin
      case Mode is
         when Curses => Err := Reset_Prog_Mode;
         when Shell  => Err := Reset_Shell_Mode;
      end case;
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Reset_Curses_Mode;

   procedure Save_Terminal_State
   is
      function Savetty return C_Int;
      pragma Import (C, Savetty, "savetty");
   begin
      if Savetty = Curses_Err then
         raise Curses_Exception;
      end if;
   end Save_Terminal_State;

   procedure Reset_Terminal_State
   is
      function Resetty return C_Int;
      pragma Import (C, Resetty, "resetty");
   begin
      if Resetty = Curses_Err then
         raise Curses_Exception;
      end if;
   end Reset_Terminal_State;

   procedure Rip_Off_Lines (Lines : in Integer;
                            Proc  : in Stdscr_Init_Proc)
   is
      function Ripoffline (Lines : C_Int;
                           Proc  : Stdscr_Init_Proc) return C_Int;
      pragma Import (C, Ripoffline, "_nc_ripoffline");
   begin
      if Ripoffline (C_Int (Lines), Proc) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Rip_Off_Lines;

   procedure Set_Cursor_Visibility (Visibility : in out Cursor_Visibility)
   is
      function Curs_Set (Curs : C_Int) return C_Int;
      pragma Import (C, Curs_Set, "curs_set");

      Res : C_Int;
   begin
      Res := Curs_Set (Cursor_Visibility'Pos (Visibility));
      if Res /= Curses_Err then
         Visibility := Cursor_Visibility'Val (Res);
      end if;
   end Set_Cursor_Visibility;

   procedure Nap_Milli_Seconds (Ms : in Natural)
   is
      function Napms (Ms : C_Int) return C_Int;
      pragma Import (C, Napms, "napms");
   begin
      if Napms (C_Int (Ms)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Nap_Milli_Seconds;
------------------------------------------------------------------------------

   function Standard_Window return Window
   is
      Stdscr : Window;
      pragma Import (C, Stdscr, "stdscr");
   begin
      return Stdscr;
   end Standard_Window;

   function Lines return Line_Count
   is
      C_Lines : C_Int;
      pragma Import (C, C_Lines, "LINES");
   begin
      return Line_Count (C_Lines);
   end Lines;

   function Columns return Column_Count
   is
      C_Columns : C_Int;
      pragma Import (C, C_Columns, "COLS");
   begin
      return Column_Count (C_Columns);
   end Columns;

   function Tab_Size return Natural
   is
      C_Tab_Size : C_Int;
      pragma Import (C, C_Tab_Size, "TABSIZE");
   begin
      return Natural (C_Tab_Size);
   end Tab_Size;

   function Number_Of_Colors return Natural
   is
      C_Number_Of_Colors : C_Int;
      pragma Import (C, C_Number_Of_Colors, "COLORS");
   begin
      return Natural (C_Number_Of_Colors);
   end Number_Of_Colors;

   function Number_Of_Color_Pairs return Natural
   is
      C_Number_Of_Color_Pairs : C_Int;
      pragma Import (C, C_Number_Of_Color_Pairs, "COLOR_PAIRS");
   begin
      return Natural (C_Number_Of_Color_Pairs);
   end Number_Of_Color_Pairs;
------------------------------------------------------------------------------
   procedure Transform_Coordinates
     (W      : in Window := Standard_Window;
      Line   : in out Line_Position;
      Column : in out Column_Position;
      Dir    : in Transform_Direction := From_Screen)
   is
      type Int_Access is access all C_Int;
      function Transform (W    : Window;
                          Y, X : Int_Access;
                          Dir  : Curses_Bool) return C_Int;
      pragma Import (C, Transform, "wmouse_trafo");

      X : aliased C_Int := C_Int (Column);
      Y : aliased C_Int := C_Int (Line);
      D : Curses_Bool := Curses_Bool_False;
      R : C_Int;
   begin
      if Dir = To_Screen then
         D := 1;
      end if;
      R := Transform (W, Y'Access, X'Access, D);
      if R = Curses_False then
         raise Curses_Exception;
      else
         Line   := Line_Position (Y);
         Column := Column_Position (X);
      end if;
   end Transform_Coordinates;
------------------------------------------------------------------------------
   procedure Use_Default_Colors is
      function C_Use_Default_Colors return C_Int;
      pragma Import (C, C_Use_Default_Colors, "use_default_colors");
      Err : constant C_Int := C_Use_Default_Colors;
   begin
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Use_Default_Colors;

   procedure Assume_Default_Colors (Fore : Color_Number := Default_Color;
                                    Back : Color_Number := Default_Color)
   is
      function C_Assume_Default_Colors (Fore : C_Int;
                                        Back : C_Int) return C_Int;
      pragma Import (C, C_Assume_Default_Colors, "assume_default_colors");

      Err : constant C_Int := C_Assume_Default_Colors (C_Int (Fore),
                                                       C_Int (Back));
   begin
      if Err = Curses_Err then
         raise Curses_Exception;
      end if;
   end Assume_Default_Colors;
------------------------------------------------------------------------------
   function Curses_Version return String
   is
      function curses_versionC return chars_ptr;
      pragma Import (C, curses_versionC, "curses_version");
      Result : constant chars_ptr := curses_versionC;
   begin
      return Fill_String (Result);
   end Curses_Version;
------------------------------------------------------------------------------
   function Use_Extended_Names (Enable : Boolean) return Boolean
   is
      function use_extended_namesC (e : Curses_Bool) return C_Int;
      pragma Import (C, use_extended_namesC, "use_extended_names");

      Res : constant C_Int :=
         use_extended_namesC (Curses_Bool (Boolean'Pos (Enable)));
   begin
      if Res = C_Int (Curses_Bool_False) then
         return False;
      else
         return True;
      end if;
   end Use_Extended_Names;
------------------------------------------------------------------------------
   procedure Screen_Dump_To_File (Filename : in String)
   is
      function scr_dump (f : char_array) return C_Int;
      pragma Import (C, scr_dump, "scr_dump");
      Txt    : char_array (0 .. Filename'Length);
      Length : size_t;
   begin
      To_C (Filename, Txt, Length);
      if Curses_Err = scr_dump (Txt) then
         raise Curses_Exception;
      end if;
   end Screen_Dump_To_File;

   procedure Screen_Restore_From_File (Filename : in String)
   is
      function scr_restore (f : char_array) return C_Int;
      pragma Import (C, scr_restore, "scr_restore");
      Txt    : char_array (0 .. Filename'Length);
      Length : size_t;
   begin
      To_C (Filename, Txt, Length);
      if Curses_Err = scr_restore (Txt)  then
         raise Curses_Exception;
      end if;
   end Screen_Restore_From_File;

   procedure Screen_Init_From_File (Filename : in String)
   is
      function scr_init (f : char_array) return C_Int;
      pragma Import (C, scr_init, "scr_init");
      Txt    : char_array (0 .. Filename'Length);
      Length : size_t;
   begin
      To_C (Filename, Txt, Length);
      if Curses_Err = scr_init (Txt) then
         raise Curses_Exception;
      end if;
   end Screen_Init_From_File;

   procedure Screen_Set_File (Filename : in String)
   is
      function scr_set (f : char_array) return C_Int;
      pragma Import (C, scr_set, "scr_set");
      Txt    : char_array (0 .. Filename'Length);
      Length : size_t;
   begin
      To_C (Filename, Txt, Length);
      if Curses_Err = scr_set (Txt) then
         raise Curses_Exception;
      end if;
   end Screen_Set_File;
------------------------------------------------------------------------------
   procedure Resize (Win               : Window := Standard_Window;
                     Number_Of_Lines   : Line_Count;
                     Number_Of_Columns : Column_Count) is
      function wresize (win     : Window;
                        lines   : C_Int;
                        columns : C_Int) return C_Int;
      pragma Import (C, wresize);
   begin
      if wresize (Win,
                  C_Int (Number_Of_Lines),
                  C_Int (Number_Of_Columns)) = Curses_Err then
         raise Curses_Exception;
      end if;
   end Resize;
------------------------------------------------------------------------------

end Terminal_Interface.Curses;


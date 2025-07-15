//
// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2023 LucKey Productions.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Object.h"

namespace Dry
{

/// Global mouse click in the UI. Sent by the UI subsystem.
DRY_EVENT(E_UIMOUSECLICK, UIMouseClick)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Global mouse click end in the UI. Sent by the UI subsystem.
DRY_EVENT(E_UIMOUSECLICKEND, UIMouseClickEnd)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_BEGINELEMENT, BeginElement);    // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Global mouse double click in the UI. Sent by the UI subsystem.
DRY_EVENT(E_UIMOUSEDOUBLECLICK, UIMouseDoubleClick)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_XBEGIN, XBegin);                // int
    DRY_PARAM(P_YBEGIN, YBegin);                // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse click on a UI element. Parameters are same as in UIMouseClick event, but is sent by the element.
DRY_EVENT(E_CLICK, Click)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse click end on a UI element. Parameters are same as in UIMouseClickEnd event, but is sent by the element.
DRY_EVENT(E_CLICKEND, ClickEnd)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_BEGINELEMENT, BeginElement);    // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse double click on a UI element. Parameters are same as in UIMouseDoubleClick event, but is sent by the element.
DRY_EVENT(E_DOUBLECLICK, DoubleClick)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_XBEGIN, XBegin);                // int
    DRY_PARAM(P_YBEGIN, YBegin);                // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Drag and drop test.
DRY_EVENT(E_DRAGDROPTEST, DragDropTest)
{
    DRY_PARAM(P_SOURCE, Source);                // UIElement pointer
    DRY_PARAM(P_TARGET, Target);                // UIElement pointer
    DRY_PARAM(P_ACCEPT, Accept);                // bool
}

/// Drag and drop finish.
DRY_EVENT(E_DRAGDROPFINISH, DragDropFinish)
{
    DRY_PARAM(P_SOURCE, Source);                // UIElement pointer
    DRY_PARAM(P_TARGET, Target);                // UIElement pointer
    DRY_PARAM(P_ACCEPT, Accept);                // bool
}

/// Focus element changed.
DRY_EVENT(E_FOCUSCHANGED, FocusChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_CLICKEDELEMENT, ClickedElement); // UIElement pointer
}

/// UI element name changed.
DRY_EVENT(E_NAMECHANGED, NameChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// UI element resized.
DRY_EVENT(E_RESIZED, Resized)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_WIDTH, Width);                  // int
    DRY_PARAM(P_HEIGHT, Height);                // int
    DRY_PARAM(P_DX, DX);                        // int
    DRY_PARAM(P_DY, DY);                        // int
}

/// UI element positioned.
DRY_EVENT(E_POSITIONED, Positioned)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
}

/// UI element visibility changed.
DRY_EVENT(E_VISIBLECHANGED, VisibleChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_VISIBLE, Visible);              // bool
}

/// UI element focused.
DRY_EVENT(E_FOCUSED, Focused)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_BYKEY, ByKey);                  // bool
}

/// UI element defocused.
DRY_EVENT(E_DEFOCUSED, Defocused)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// UI element layout updated.
DRY_EVENT(E_LAYOUTUPDATED, LayoutUpdated)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// UI button pressed.
DRY_EVENT(E_PRESSED, Pressed)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// UI button was pressed, then released.
DRY_EVENT(E_RELEASED, Released)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// UI checkbox toggled.
DRY_EVENT(E_TOGGLED, Toggled)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_STATE, State);                  // bool
}

/// UI slider value changed
DRY_EVENT(E_SLIDERCHANGED, SliderChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_VALUE, Value);                  // float
}

/// UI slider being paged.
DRY_EVENT(E_SLIDERPAGED, SliderPaged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_OFFSET, Offset);                // int
    DRY_PARAM(P_PRESSED, Pressed);              // bool
}

/// UI progressbar value changed
DRY_EVENT(E_PROGRESSBARCHANGED, ProgressBarChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_VALUE, Value);                  // float
}

/// UI scrollbar value changed.
DRY_EVENT(E_SCROLLBARCHANGED, ScrollBarChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_VALUE, Value);                  // float
}

/// UI scrollview position changed.
DRY_EVENT(E_VIEWCHANGED, ViewChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
}

/// UI modal changed (currently only Window has modal flag).
DRY_EVENT(E_MODALCHANGED, ModalChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_MODAL, Modal);                  // bool
}

/// Text entry into a LineEdit. The text can be modified in the event data.
DRY_EVENT(E_TEXTENTRY, TextEntry)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_TEXT, Text);                    // String [in/out]
}

/// Editable text changed
DRY_EVENT(E_TEXTCHANGED, TextChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_TEXT, Text);                    // String
}

/// Text editing finished (enter pressed on a LineEdit)
DRY_EVENT(E_TEXTFINISHED, TextFinished)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_TEXT, Text);                    // String
    DRY_PARAM(P_VALUE, Value);                  // Float
}

/// Menu selected.
DRY_EVENT(E_MENUSELECTED, MenuSelected)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// Listview or DropDownList item selected.
DRY_EVENT(E_ITEMSELECTED, ItemSelected)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_SELECTION, Selection);          // int
}

/// Listview item deselected.
DRY_EVENT(E_ITEMDESELECTED, ItemDeselected)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_SELECTION, Selection);          // int
}

/// Listview selection change finished.
DRY_EVENT(E_SELECTIONCHANGED, SelectionChanged)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// Listview item clicked. If this is a left-click, also ItemSelected event will be sent. If this is a right-click, only this event is sent.
DRY_EVENT(E_ITEMCLICKED, ItemClicked)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_ITEM, Item);                    // UIElement pointer
    DRY_PARAM(P_SELECTION, Selection);          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Listview item double clicked.
DRY_EVENT(E_ITEMDOUBLECLICKED, ItemDoubleClicked)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_ITEM, Item);                    // UIElement pointer
    DRY_PARAM(P_SELECTION, Selection);          // int
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// LineEdit or ListView unhandled key pressed.
DRY_EVENT(E_UNHANDLEDKEY, UnhandledKey)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_KEY, Key);                      // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Fileselector choice.
DRY_EVENT(E_FILESELECTED, FileSelected)
{
    DRY_PARAM(P_FILENAME, FileName);            // String
    DRY_PARAM(P_FILTER, Filter);                // String
    DRY_PARAM(P_OK, OK);                        // bool
}

/// MessageBox acknowlegement.
DRY_EVENT(E_MESSAGEACK, MessageACK)
{
    DRY_PARAM(P_OK, OK);                        // bool
}

/// A child element has been added to an element. Sent by the UI root element, or element-event-sender if set.
DRY_EVENT(E_ELEMENTADDED, ElementAdded)
{
    DRY_PARAM(P_ROOT, Root);                    // UIElement pointer
    DRY_PARAM(P_PARENT, Parent);                // UIElement pointer
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// A child element is about to be removed from an element. Sent by the UI root element, or element-event-sender if set.
DRY_EVENT(E_ELEMENTREMOVED, ElementRemoved)
{
    DRY_PARAM(P_ROOT, Root);                    // UIElement pointer
    DRY_PARAM(P_PARENT, Parent);                // UIElement pointer
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// Hovering on an UI element has started
DRY_EVENT(E_HOVERBEGIN, HoverBegin)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int
    DRY_PARAM(P_ELEMENTY, ElementY);            // int
}

/// Hovering on an UI element has ended
DRY_EVENT(E_HOVEREND, HoverEnd)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
}

/// Drag behavior of a UI Element has started
DRY_EVENT(E_DRAGBEGIN, DragBegin)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int
    DRY_PARAM(P_ELEMENTY, ElementY);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag behavior of a UI Element when the input device has moved
DRY_EVENT(E_DRAGMOVE, DragMove)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_DX, DX);                        // int
    DRY_PARAM(P_DY, DY);                        // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int
    DRY_PARAM(P_ELEMENTY, ElementY);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag behavior of a UI Element has finished
DRY_EVENT(E_DRAGEND, DragEnd)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int
    DRY_PARAM(P_ELEMENTY, ElementY);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// Drag of a UI Element was canceled by pressing ESC
DRY_EVENT(E_DRAGCANCEL, DragCancel)
{
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int
    DRY_PARAM(P_ELEMENTY, ElementY);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_NUMBUTTONS, NumButtons);        // int
}

/// A file was drag-dropped into the application window. Includes also coordinates and UI element if applicable
DRY_EVENT(E_UIDROPFILE, UIDropFile)
{
    DRY_PARAM(P_FILENAME, FileName);            // String
    DRY_PARAM(P_ELEMENT, Element);              // UIElement pointer
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_ELEMENTX, ElementX);            // int (only if element is non-null)
    DRY_PARAM(P_ELEMENTY, ElementY);            // int (only if element is non-null)
}

}

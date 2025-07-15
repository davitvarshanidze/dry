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
#include "../Input/InputConstants.h"


namespace Dry
{

/// Mouse button pressed.
DRY_EVENT(E_MOUSEBUTTONDOWN, MouseButtonDown)
{
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
    DRY_PARAM(P_CLICKS, Clicks);                // int
}

/// Mouse button released.
DRY_EVENT(E_MOUSEBUTTONUP, MouseButtonUp)
{
    DRY_PARAM(P_BUTTON, Button);                // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse moved.
DRY_EVENT(E_MOUSEMOVE, MouseMove)
{
    DRY_PARAM(P_X, X);                          // int (only when mouse visible)
    DRY_PARAM(P_Y, Y);                          // int (only when mouse visible)
    DRY_PARAM(P_DX, DX);                        // int
    DRY_PARAM(P_DY, DY);                        // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Mouse wheel moved.
DRY_EVENT(E_MOUSEWHEEL, MouseWheel)
{
    DRY_PARAM(P_WHEEL, Wheel);                  // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Key pressed.
DRY_EVENT(E_KEYDOWN, KeyDown)
{
    DRY_PARAM(P_KEY, Key);                      // int
    DRY_PARAM(P_SCANCODE, Scancode);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
    DRY_PARAM(P_REPEAT, Repeat);                // bool
}

/// Key released.
DRY_EVENT(E_KEYUP, KeyUp)
{
    DRY_PARAM(P_KEY, Key);                      // int
    DRY_PARAM(P_SCANCODE, Scancode);            // int
    DRY_PARAM(P_BUTTONS, Buttons);              // int
    DRY_PARAM(P_QUALIFIERS, Qualifiers);        // int
}

/// Text input event.
DRY_EVENT(E_TEXTINPUT, TextInput)
{
    DRY_PARAM(P_TEXT, Text);                    // String
}

/// Text editing event.
DRY_EVENT(E_TEXTEDITING, TextEditing)
{
    DRY_PARAM(P_COMPOSITION, Composition);      // String
    DRY_PARAM(P_CURSOR, Cursor);                // int
    DRY_PARAM(P_SELECTION_LENGTH, SelectionLength);  // int
}

/// Joystick connected.
DRY_EVENT(E_JOYSTICKCONNECTED, JoystickConnected)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
}

/// Joystick disconnected.
DRY_EVENT(E_JOYSTICKDISCONNECTED, JoystickDisconnected)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
}

/// Joystick button pressed.
DRY_EVENT(E_JOYSTICKBUTTONDOWN, JoystickButtonDown)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
    DRY_PARAM(P_BUTTON, Button);                // int
}

/// Joystick button released.
DRY_EVENT(E_JOYSTICKBUTTONUP, JoystickButtonUp)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
    DRY_PARAM(P_BUTTON, Button);                // int
}

/// Joystick axis moved.
DRY_EVENT(E_JOYSTICKAXISMOVE, JoystickAxisMove)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
    DRY_PARAM(P_AXIS, Axis);                    // int
    DRY_PARAM(P_POSITION, Position);            // float
}

/// Joystick POV hat moved.
DRY_EVENT(E_JOYSTICKHATMOVE, JoystickHatMove)
{
    DRY_PARAM(P_JOYSTICKID, JoystickID);        // int
    DRY_PARAM(P_HAT, Hat);                      // int
    DRY_PARAM(P_POSITION, Position);            // int
}

/// Finger pressed on the screen.
DRY_EVENT(E_TOUCHBEGIN, TouchBegin)
{
    DRY_PARAM(P_TOUCHID, TouchID);              // int
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_PRESSURE, Pressure);            // float
}

/// Finger released from the screen.
DRY_EVENT(E_TOUCHEND, TouchEnd)
{
    DRY_PARAM(P_TOUCHID, TouchID);              // int
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
}

/// Finger moved on the screen.
DRY_EVENT(E_TOUCHMOVE, TouchMove)
{
    DRY_PARAM(P_TOUCHID, TouchID);              // int
    DRY_PARAM(P_X, X);                          // int
    DRY_PARAM(P_Y, Y);                          // int
    DRY_PARAM(P_DX, DX);                        // int
    DRY_PARAM(P_DY, DY);                        // int
    DRY_PARAM(P_PRESSURE, Pressure);            // float
}

/// A touch gesture finished recording.
DRY_EVENT(E_GESTURERECORDED, GestureRecorded)
{
    DRY_PARAM(P_GESTUREID, GestureID);          // unsigned
}

/// A recognized touch gesture was input by the user.
DRY_EVENT(E_GESTUREINPUT, GestureInput)
{
    DRY_PARAM(P_GESTUREID, GestureID);          // unsigned
    DRY_PARAM(P_CENTERX, CenterX);              // int
    DRY_PARAM(P_CENTERY, CenterY);              // int
    DRY_PARAM(P_NUMFINGERS, NumFingers);        // int
    DRY_PARAM(P_ERROR, Error);                  // float
}

/// Pinch/rotate multi-finger touch gesture motion update.
DRY_EVENT(E_MULTIGESTURE, MultiGesture)
{
    DRY_PARAM(P_CENTERX, CenterX);              // int
    DRY_PARAM(P_CENTERY, CenterY);              // int
    DRY_PARAM(P_NUMFINGERS, NumFingers);        // int
    DRY_PARAM(P_DTHETA, DTheta);                // float (degrees)
    DRY_PARAM(P_DDIST, DDist);                  // float
}

/// A file was drag-dropped into the application window.
DRY_EVENT(E_DROPFILE, DropFile)
{
    DRY_PARAM(P_FILENAME, FileName);            // String
}

/// Application input focus or minimization changed.
DRY_EVENT(E_INPUTFOCUS, InputFocus)
{
    DRY_PARAM(P_FOCUS, Focus);                  // bool
    DRY_PARAM(P_MINIMIZED, Minimized);          // bool
}

/// OS mouse cursor visibility changed.
DRY_EVENT(E_MOUSEVISIBLECHANGED, MouseVisibleChanged)
{
    DRY_PARAM(P_VISIBLE, Visible);              // bool
}

/// Mouse mode changed.
DRY_EVENT(E_MOUSEMODECHANGED, MouseModeChanged)
{
    DRY_PARAM(P_MODE, Mode);                    // MouseMode
    DRY_PARAM(P_MOUSELOCKED, MouseLocked);      // bool
}

/// Application exit requested.
DRY_EVENT(E_EXITREQUESTED, ExitRequested)
{
}

/// Raw SDL input event.
DRY_EVENT(E_SDLRAWINPUT, SDLRawInput)
{
    DRY_PARAM(P_SDLEVENT, SDLEvent);           // SDL_Event*
    DRY_PARAM(P_CONSUMED, Consumed);           // bool
}

/// Input handling begins.
DRY_EVENT(E_INPUTBEGIN, InputBegin)
{
}

/// Input handling ends.
DRY_EVENT(E_INPUTEND, InputEnd)
{
}

}

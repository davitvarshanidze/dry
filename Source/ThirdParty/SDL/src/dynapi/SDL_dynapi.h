/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// Modified by Lasse Oorni for Urho3D

#ifndef SDL_dynapi_h_
#define SDL_dynapi_h_

/* IMPORTANT:
   This is the master switch to disabling the dynamic API. We made it so you
   have to hand-edit an internal source file in SDL to turn it off; you
   can do it if you want it badly enough, but hopefully you won't want to.
   You should understand the ramifications of turning this off: it makes it
   hard to update your SDL in the field, and impossible if you've statically
   linked SDL into your app. Understand that platforms change, and if we can't
   drop in an updated SDL, your application can definitely break some time
   in the future, even if it's fine today.
   To be sure, as new system-level video and audio APIs are introduced, an
   updated SDL can transparently take advantage of them, but your program will
   not without this feature. Think hard before turning it off.
*/
#ifdef SDL_DYNAMIC_API  /* Tried to force it on the command line? */
#error Nope, you have to edit this file to force this off.
#endif

// Dry: disabled dynamic API
#define SDL_DYNAMIC_API 0

#endif

/* vi: set ts=4 sw=4 expandtab: */

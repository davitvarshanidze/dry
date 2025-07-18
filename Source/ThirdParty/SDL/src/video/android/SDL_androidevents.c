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

#include "../../SDL_internal.h"

#if SDL_VIDEO_DRIVER_ANDROID

#include "SDL_androidevents.h"
#include "SDL_events.h"
#include "SDL_androidkeyboard.h"
#include "SDL_androidwindow.h"

/* Can't include sysaudio "../../audio/android/SDL_androidaudio.h"
 * because of THIS redefinition */

#if !SDL_AUDIO_DISABLED && SDL_AUDIO_DRIVER_ANDROID
extern void ANDROIDAUDIO_ResumeDevices(void);
extern void ANDROIDAUDIO_PauseDevices(void);
#else
static void ANDROIDAUDIO_ResumeDevices(void) {}
static void ANDROIDAUDIO_PauseDevices(void) {}
#endif

#if !SDL_AUDIO_DISABLED && SDL_AUDIO_DRIVER_OPENSLES
extern void openslES_ResumeDevices(void);
extern void openslES_PauseDevices(void);
#else
static void openslES_ResumeDevices(void) {}
static void openslES_PauseDevices(void) {}
#endif

/* Number of 'type' events in the event queue */
static int
SDL_NumberOfEvents(Uint32 type)
{
    return SDL_PeepEvents(NULL, 0, SDL_PEEKEVENT, type, type);
}

static void
android_egl_context_restore(SDL_Window *window)
{
    if (window) {
        SDL_Event event;
        SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
        // Dry: make sure there is a valid stored context to restore
        if (data->egl_context && SDL_GL_MakeCurrent(window, (SDL_GLContext) data->egl_context) < 0) {
            // Dry: if the old context could not be restored, leave it to the Graphics subsystem to create a new one
            data->egl_context = NULL;
        }
    }
}

static void
android_egl_context_backup(SDL_Window *window)
{
    if (window) {
        /* Keep a copy of the EGL Context so we can try to restore it when we resume */
        SDL_WindowData *data = (SDL_WindowData *) window->driverdata;
        data->egl_context = SDL_GL_GetCurrentContext();
        /* We need to do this so the EGLSurface can be freed */
        SDL_GL_MakeCurrent(window, NULL);
    }
}


/*
 * Android_ResumeSem and Android_PauseSem are signaled from Java_org_libsdl_app_SDLActivity_nativePause and Java_org_libsdl_app_SDLActivity_nativeResume
 * When the pause semaphore is signaled, if Android_PumpEvents_Blocking is used, the event loop will block until the resume signal is emitted.
 *
 * No polling necessary
 */

void
Android_PumpEvents_Blocking(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;

    if (videodata->isPaused) {

        /* Make sure this is the last thing we do before pausing */
        SDL_LockMutex(Android_ActivityMutex);
        android_egl_context_backup(Android_Window);
        SDL_UnlockMutex(Android_ActivityMutex);

        ANDROIDAUDIO_PauseDevices();
        openslES_PauseDevices();

        if (SDL_SemWait(Android_ResumeSem) == 0) {

            videodata->isPaused = 0;

            ANDROIDAUDIO_ResumeDevices();
            openslES_ResumeDevices();

            /* Restore the GL Context from here, as this operation is thread dependent */
            if (!SDL_HasEvent(SDL_QUIT)) {
                SDL_LockMutex(Android_ActivityMutex);
                android_egl_context_restore(Android_Window);
                SDL_UnlockMutex(Android_ActivityMutex);
            }

            /* Make sure SW Keyboard is restored when an app becomes foreground */
            if (SDL_IsTextInputActive()) {
                Android_StartTextInput(_this); /* Only showTextInput */
            }
        }
    } else {
        if (videodata->isPausing || SDL_SemTryWait(Android_PauseSem) == 0) {
            /* We've been signaled to pause (potentially several times), but before we block ourselves,
             * we need to make sure that the very last event (of the first pause sequence, if several)
             * has reached the app */
            if (SDL_NumberOfEvents(SDL_APP_DIDENTERBACKGROUND) > SDL_SemValue(Android_PauseSem)) {
                videodata->isPausing = 1;
            } else {
                videodata->isPausing = 0;
                videodata->isPaused = 1;
            }
        }
    }
}

void
Android_PumpEvents_NonBlocking(_THIS)
{
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
    static int backup_context = 0;

    if (videodata->isPaused) {

        if (backup_context) {

            SDL_LockMutex(Android_ActivityMutex);
            android_egl_context_backup(Android_Window);
            SDL_UnlockMutex(Android_ActivityMutex);

            ANDROIDAUDIO_PauseDevices();
            openslES_PauseDevices();

            backup_context = 0;
        }


        if (SDL_SemTryWait(Android_ResumeSem) == 0) {

            videodata->isPaused = 0;

            ANDROIDAUDIO_ResumeDevices();
            openslES_ResumeDevices();

            /* Restore the GL Context from here, as this operation is thread dependent */
            if (!SDL_HasEvent(SDL_QUIT)) {
                SDL_LockMutex(Android_ActivityMutex);
                android_egl_context_restore(Android_Window);
                SDL_UnlockMutex(Android_ActivityMutex);
            }

            /* Make sure SW Keyboard is restored when an app becomes foreground */
            if (SDL_IsTextInputActive()) {
                Android_StartTextInput(_this); /* Only showTextInput */
            }
        }
    } else {
        if (videodata->isPausing || SDL_SemTryWait(Android_PauseSem) == 0) {
            /* We've been signaled to pause (potentially several times), but before we block ourselves,
             * we need to make sure that the very last event (of the first pause sequence, if several)
             * has reached the app */
            if (SDL_NumberOfEvents(SDL_APP_DIDENTERBACKGROUND) > SDL_SemValue(Android_PauseSem)) {
                videodata->isPausing = 1;
            } else {
                videodata->isPausing = 0;
                videodata->isPaused = 1;
                backup_context = 1;
            }
        }
    }
}

#endif /* SDL_VIDEO_DRIVER_ANDROID */

/* vi: set ts=4 sw=4 expandtab: */

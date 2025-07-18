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

#ifdef DRY_ANGELSCRIPT
#include <Dry/AngelScript/ScriptFile.h>
#include <Dry/AngelScript/Script.h>
#endif
#include <Dry/Core/Main.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Engine/EngineDefs.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/IO/Log.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Resource/ResourceEvents.h>

#include "DryPlayer.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(DryPlayer);

DryPlayer::DryPlayer(Context* context) :
    Application(context),
    commandLineRead_(false)
{
}

void DryPlayer::Setup()
{
    // Web platform depends on the resource system to read any data files. Skip parsing the command line file now
    // and try later when the resource system is live
#ifndef __EMSCRIPTEN__
    // Read command line from a file if no arguments given. This is primarily intended for mobile platforms.
    // Note that the command file name uses a hardcoded path that does not utilize the resource system
    // properly (including resource path prefix), as the resource system is not yet initialized at this point
    auto* filesystem = GetSubsystem<FileSystem>();
    const String commandFileName = filesystem->GetProgramDir() + "Data/CommandLine.txt";
    if (GetArguments().IsEmpty() && filesystem->FileExists(commandFileName))
    {
        SharedPtr<File> commandFile(new File(context_, commandFileName));
        if (commandFile->IsOpen())
        {
            commandLineRead_ = true;
            String commandLine = commandFile->ReadLine();
            commandFile->Close();
            ParseArguments(commandLine, false);
            // Reparse engine startup parameters now
            engineParameters_ = Engine::ParseParameters(GetArguments());
        }
    }
#endif

    // Check for script file name from the arguments
    GetScriptFileName();

#ifndef __EMSCRIPTEN__
    // Show usage if not found
    if ((GetArguments().Size() || commandLineRead_) && scriptFileName_.IsEmpty())
    {
        ErrorExit("Usage: DryPlayer <scriptfile> [options]\n\n"
            "The script file should implement the function void Start() for initializing the "
            "application and subscribing to all necessary events, such as the frame update.\n"
            #ifndef _WIN32
            "\nCommand line options:\n"
            "-x <res>     Horizontal resolution\n"
            "-y <res>     Vertical resolution\n"
            "-m <level>   Enable hardware multisampling\n"
            "-v           Enable vertical sync\n"
            "-t           Enable triple buffering\n"
            "-w           Start in windowed mode\n"
            "-s           Enable resizing when in windowed mode\n"
            "-q           Enable quiet mode which does not log to standard output stream\n"
            "-b <length>  Sound buffer length in milliseconds\n"
            "-r <freq>    Sound mixing frequency in Hz\n"
            "-pp <paths>  Resource prefix path(s), separated by semicolons, default to executable path\n"
            "The resource prefix paths can also be defined using DRY_PREFIX_PATH env - var\n"
            "When both are defined, the paths set by -pp takes higher precedence\n"
            "-p <paths>   Resource path(s) to use, separated by semicolons, default to 'Data;CoreData'\n"
            "-pf <files>  Resource package file to use, separated by semicolons, default to none\n"
            "-ap <paths>  Resource autoload path(s), separated by semicolons, default to 'AutoLoad'\n"
            "-log <level> Change the log level, valid 'level' values: 'debug', 'info', 'warning', 'error'\n"
            "-ds <file>   Dump used shader variations to a file for precaching\n"
            "-mq <level>  Material quality level, default 2 (high)\n"
            "-tq <level>  Texture quality level, default 2 (high)\n"
            "-tf <level>  Texture filter mode, default 2 (trilinear)\n"
            "-af <level>  Texture anisotropy level, default 4. Also sets anisotropic filter mode\n"
            "-gl2         Force OpenGL 2 use even if OpenGL 3 is available\n"
            "-borderless  Borderless window mode\n"
            "-lowdpi      Force low DPI mode on Retina display\n"
            "-headless    Headless mode. No application window will be created\n"
            "-landscape   Use landscape orientations (iOS only, default)\n"
            "-portrait    Use portrait orientations (iOS only)\n"
            "-monitor <num> Monitor number to use\n"
            "-hz <freq>   Monitor refresh rate to use\n"
            "-prepass     Use light pre-pass rendering\n"
            "-deferred    Use deferred rendering\n"
            "-renderpath <name> Use the named renderpath (must enter full resource name)\n"
            "-lqshadows   Use low-quality (1-sample) shadow filtering\n"
            "-noshadows   Disable shadow rendering\n"
            "-nolimit     Disable frame limiter\n"
            "-nothreads   Disable worker threads\n"
            "-nosound     Disable sound output\n"
            "-noip        Disable sound mixing interpolation\n"
            "-touch       Touch emulation on desktop platform\n"
            #endif
        );
    }
    else
    {
        // Use the script file name as the base name for the log file
        engineParameters_[EP_LOG_NAME] = filesystem->GetAppPreferencesDir("dry", "logs") + GetFileNameAndExtension(scriptFileName_) + ".log";
    }
#else
    // On Web platform setup a default windowed resolution similar to the executable samples
    engineParameters_[EP_FULL_SCREEN]  = false;
#endif

    // Construct a search path to find the resource prefix with two entries:
    // The first entry is an empty path which will be substituted with program/bin directory -- this entry is for binary when it is still in build tree
    // The second and third entries are possible relative paths from the installed program/bin directory to the asset directory -- these entries are for binary when it is in the Dry SDK installation location
    if (!engineParameters_.Contains(EP_RESOURCE_PREFIX_PATHS))
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";../share/Resources;../share/Dry/Resources";
}

void DryPlayer::Start()
{
    // Reattempt reading the command line from the resource system now if not read before
    // Note that the engine can not be reconfigured at this point; only the script name can be specified
    if (GetArguments().IsEmpty() && !commandLineRead_)
    {
        SharedPtr<File> commandFile = GetSubsystem<ResourceCache>()->GetFile("CommandLine.txt", false);
        if (commandFile)
        {
            String commandLine = commandFile->ReadLine();
            commandFile->Close();
            ParseArguments(commandLine, false);
        }

        GetScriptFileName();
    }

    if (scriptFileName_.IsEmpty())
    {
        ErrorExit("Script file name not specified; cannot proceed");
        return;
    }

    String extension = GetExtension(scriptFileName_);
    if (extension != ".lua" && extension != ".luc")
    {
#ifdef DRY_ANGELSCRIPT
        // Instantiate and register the AngelScript subsystem
        context_->RegisterSubsystem(new Script(context_));

        // Hold a shared pointer to the script file to make sure it is not unloaded during runtime
        scriptFile_ = GetSubsystem<ResourceCache>()->GetResource<ScriptFile>(scriptFileName_);

        // If script loading is successful, proceed to main loop
        if (scriptFile_ && scriptFile_->Execute("void Start()"))
        {
            // Subscribe to script's reload event to allow live-reload of the application
            SubscribeToEvent(scriptFile_, E_RELOADSTARTED, DRY_HANDLER(DryPlayer, HandleScriptReloadStarted));
            SubscribeToEvent(scriptFile_, E_RELOADFINISHED, DRY_HANDLER(DryPlayer, HandleScriptReloadFinished));
            SubscribeToEvent(scriptFile_, E_RELOADFAILED, DRY_HANDLER(DryPlayer, HandleScriptReloadFailed));
            return;
        }
#else
        ErrorExit("AngelScript is not enabled!");
        return;
#endif
    }

    // The script was not successfully loaded. Show the last error message and do not run the main loop
    ErrorExit();
}

void DryPlayer::Stop()
{
#ifdef DRY_ANGELSCRIPT
    if (scriptFile_)
    {
        // Execute the optional stop function
        if (scriptFile_->GetFunction("void Stop()"))
            scriptFile_->Execute("void Stop()");
    }
#endif
}

void DryPlayer::HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData)
{
#ifdef DRY_ANGELSCRIPT
    if (scriptFile_->GetFunction("void Stop()"))
        scriptFile_->Execute("void Stop()");
#endif
}

void DryPlayer::HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData)
{
#ifdef DRY_ANGELSCRIPT
    // Restart the script application after reload
    if (!scriptFile_->Execute("void Start()"))
    {
        scriptFile_.Reset();
        ErrorExit();
    }
#endif
}

void DryPlayer::HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData)
{
#ifdef DRY_ANGELSCRIPT
    scriptFile_.Reset();
    ErrorExit();
#endif
}

void DryPlayer::GetScriptFileName()
{
    const Vector<String>& arguments = GetArguments();
    if (arguments.Size() && arguments[0][0] != '-')
        scriptFileName_ = GetInternalPath(arguments[0]);
}

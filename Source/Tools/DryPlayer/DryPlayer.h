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

#include <Dry/Engine/Application.h>

using namespace Dry;

/// DryPlayer application runs a script specified on the command line.
class DryPlayer : public Application
{
    DRY_OBJECT(DryPlayer, Application);

public:
    /// Construct.
    explicit DryPlayer(Context* context);

    /// Setup before engine initialization. Verify that a script file has been specified.
    void Setup() override;
    /// Setup after engine initialization. Load the script and execute its start function.
    void Start() override;
    /// Cleanup after the main loop. Run the script's stop function if it exists.
    void Stop() override;

private:
    /// Handle reload start of the script file.
    void HandleScriptReloadStarted(StringHash eventType, VariantMap& eventData);
    /// Handle reload success of the script file.
    void HandleScriptReloadFinished(StringHash eventType, VariantMap& eventData);
    /// Handle reload failure of the script file.
    void HandleScriptReloadFailed(StringHash eventType, VariantMap& eventData);
    /// Parse script file name from the first argument.
    void GetScriptFileName();

    /// Script file name.
    String scriptFileName_;
    /// Flag whether CommandLine.txt was already successfully read.
    bool commandLineRead_;

#ifdef DRY_ANGELSCRIPT
    /// Script file.
    SharedPtr<ScriptFile> scriptFile_;
#endif
};

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

#include "../Core/Context.h"
#include "../Core/Main.h"
#include "../Engine/Engine.h"

namespace Dry
{

class Engine;

/// Base class for creating applications which initialize the Dry engine and run a main loop until exited.
class DRY_API Application : public Object
{
    DRY_OBJECT(Application, Object);

public:
    /// Construct. Parse default engine parameters from the command line, and create the engine in an uninitialized state.
    explicit Application(Context* context);

    /// Setup before engine initialization. This is a chance to eg. modify the engine parameters. Call ErrorExit() to terminate without initializing the engine. Called by Application.
    virtual void Setup() { }

    /// Setup after engine initialization and before running the main loop. Call ErrorExit() to terminate without running the main loop. Called by Application.
    virtual void Start() { }

    /// Cleanup after the main loop. Called by Application.
    virtual void Stop() { }

    /// Initialize the engine and run the main loop, then return the application exit code. Catch out-of-memory exceptions while running.
    int Run();
    /// Show an error message (last log message if empty), terminate the main loop, and set failure exit code.
    void ErrorExit(const String& message = String::EMPTY);

protected:
    /// Handle log message.
    void HandleLogMessage(StringHash eventType, VariantMap& eventData);

    /// Dry engine.
    SharedPtr<Engine> engine_;
    /// Engine parameters map.
    VariantMap engineParameters_;
    /// Collected startup error log messages.
    String startupErrors_;
    /// Application exit code.
    int exitCode_;
};

// Macro for defining a main function which creates a Context and the application, then runs it
#if !defined(IOS) && !defined(TVOS)
#define DRY_DEFINE_APPLICATION_MAIN(className) \
int RunApplication() \
{ \
    Dry::SharedPtr<Dry::Context> context(new Dry::Context()); \
    Dry::SharedPtr<className> application(new className(context)); \
    return application->Run(); \
} \
DRY_DEFINE_MAIN(RunApplication())
#else
// On iOS/tvOS we will let this function exit, so do not hold the context and application in SharedPtr's
#define DRY_DEFINE_APPLICATION_MAIN(className) \
int RunApplication() \
{ \
    Dry::Context* context = new Dry::Context(); \
    className* application = new className(context); \
    return application->Run(); \
} \
DRY_DEFINE_MAIN(RunApplication());
#endif

}

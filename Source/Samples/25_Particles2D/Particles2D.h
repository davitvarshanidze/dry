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

#include "Sample.h"

namespace Dry
{
    class Node;
    class Scene;
}

// 2D particle example.
// This sample demonstrates:
//     - Creating a 2D scene with particle
//     - Displaying the scene using the Renderer subsystem
//     - Handling mouse move to move particle
class Particles2D: public Sample
{
    DRY_OBJECT(Particles2D, Sample);

public:
    /// Construct.
    explicit Particles2D(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">"
        "        <attribute name=\"Is Visible\" value=\"false\" />"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the scene content.
    void CreateScene();
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update events.
    void SubscribeToEvents();
    /// Handle mouse move event.
    void HandleMouseMove(StringHash eventType, VariantMap& eventData);

    /// Particle scene node.
    SharedPtr<Node> particleNode_;
};

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

#include <Dry/Core/CoreEvents.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UIEvents.h>

#include "UIDrag.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(UIDrag)

UIDrag::UIDrag(Context* context): Sample(context)
{
}

void UIDrag::Start()
{
    // Execute base class startup
    Sample::Start();

    // Set mouse visible
    String platform{ GetPlatform() };

    if (platform != "Android" && platform != "iOS")
        GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the UI content
    CreateGUI();

    CreateInstructions("Press SPACE to show/hide tagged UI elements\n"
                       "Drag the buttons to move them around\n"
                       "Touch input also allows multi-drag");

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void UIDrag::CreateGUI()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Graphics* graphics{ GetSubsystem<Graphics>() };
    UI* ui{ GetSubsystem<UI>() };
    UIElement* root{ ui->GetRoot() };

    // Load the style sheet from xml
    root->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    for (int i{ 0 }; i < 10; i++)
    {
        const int halfWidth{ graphics->GetWidth() / 2 };
        Button* b{ new Button{ context_ } };
        root->AddChild(b);

        // Reference a style from the style sheet loaded earlier:
        b->SetStyleAuto();
        b->SetMinWidth(250);
        b->SetPosition(IntVector2{ halfWidth - b->GetMinWidth() / 2 + (16 * i) * (i % 2) - (8 * i),
                                   50 * (i + 1) });

        // Enable the bring-to-front flag and set the initial priority
        b->SetBringToFront(true);
        b->SetPriority(i);

        // Set the layout mode to make the child text elements aligned vertically
        b->SetLayout(LM_VERTICAL, 20, { 40, 40, 40, 40 });

        for (const String& name: { "Num Touch", "Text", "Event Touch" })
            b->CreateChild<Text>(name)->SetStyleAuto();

        if (i % 2 == 0)
            b->AddTag("SomeTag");

        SubscribeToEvent(b, E_CLICK, DRY_HANDLER(UIDrag, HandleClick));
        SubscribeToEvent(b, E_DRAGMOVE, DRY_HANDLER(UIDrag, HandleDragMove));
        SubscribeToEvent(b, E_DRAGBEGIN, DRY_HANDLER(UIDrag, HandleDragBegin));
        SubscribeToEvent(b, E_DRAGCANCEL, DRY_HANDLER(UIDrag, HandleDragCancel));
    }

    for (int i{ 0 }; i < 10; ++i)
    {
        Text* t{ new Text(context_) };
        root->AddChild(t);
        t->SetStyleAuto();
        t->SetName("Touch "+ String(i));
        t->SetVisible(false);
        t->SetPriority(100);     // Make sure it has higher priority than the buttons
    }
}

void UIDrag::SubscribeToEvents()
{
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(UIDrag, HandleUpdate));
}

void UIDrag::HandleClick(StringHash eventType, VariantMap& eventData)
{
    Button* element{ (Button*)eventData[Click::P_ELEMENT].GetVoidPtr() };

    element->BringToFront();
}

void UIDrag::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;

    Button* element{ (Button*)eventData[P_ELEMENT].GetVoidPtr() };

    const int lx{ eventData[P_X].GetInt() };
    const int ly{ eventData[P_Y].GetInt() };
    const IntVector2 p{ element->GetPosition() };

    element->SetVar("START", p);
    element->SetVar("DELTA", IntVector2(p.x_ - lx, p.y_ - ly));

    const int buttons{ eventData[P_BUTTONS].GetInt() };
    element->SetVar("BUTTONS", buttons);

    Text* t{ element->GetChildStaticCast<Text>("Text", false) };
    t->SetText("Drag Begin Buttons: " + String{ buttons });

    t = element->GetChildStaticCast<Text>("Num Touch", false);
    t->SetText("Number of buttons: " + String{ eventData[P_NUMBUTTONS].GetInt() });
}

void UIDrag::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;

    Button* element{ (Button*)eventData[P_ELEMENT].GetVoidPtr() };
    const int buttons{ eventData[P_BUTTONS].GetInt() };
    const IntVector2 d{ element->GetVar("DELTA").GetIntVector2() };
    int X{ eventData[P_X].GetInt() + d.x_ };
    const int Y{ eventData[P_Y].GetInt() + d.y_ };
    const int BUTTONS{ element->GetVar("BUTTONS").GetInt() };

    Text* t{ element->GetChildStaticCast<Text>("Event Touch", false) };
    t->SetText("Drag Move Buttons: " + String{ buttons });

    if (buttons == BUTTONS)
        element->SetPosition(IntVector2{ X, Y });
}

void UIDrag::HandleDragCancel(StringHash eventType, VariantMap& eventData)
{
    using namespace DragBegin;

    Button* element{ (Button*)eventData[P_ELEMENT].GetVoidPtr() };
    const IntVector2 P{ element->GetVar("START").GetIntVector2() };

    element->SetPosition(P);
}

void UIDrag::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    UI* ui{ GetSubsystem<UI>() };
    UIElement* root{ ui->GetRoot() };
    Input* input{ GetSubsystem<Input>() };
    const unsigned n{ input->GetNumTouches() };

    for (unsigned i{ 0 }; i < n; ++i)
    {
        Text* t{ (Text*)root->GetChild("Touch " + String{ i }) };
        TouchState* ts{ input->GetTouch(i) };
        t->SetText("Touch " + String{ ts->touchID_ });

        IntVector2 pos{ ts->position_ };
        pos.y_ -= 30;

        t->SetPosition(pos);
        t->SetVisible(true);
    }

    for (unsigned i{ n }; i < 10; ++i)
    {
        Text* t{ (Text*)root->GetChild("Touch " + String{ i }) };
        t->SetVisible(false);
    }

    if (input->GetKeyPress(KEY_SPACE))
    {
        PODVector<UIElement*> elements;
        root->GetChildrenWithTag(elements, "SomeTag");

        for (auto i{ elements.Begin() }; i != elements.End(); ++i)
        {
            UIElement* element{ *i };
            element->SetVisible(!element->IsVisible());
        }
    }
}

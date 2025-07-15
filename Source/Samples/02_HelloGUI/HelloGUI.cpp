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
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/Texture2D.h>
#include <Dry/Input/Input.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/CheckBox.h>
#include <Dry/UI/LineEdit.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/ToolTip.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>
#include <Dry/UI/Window.h>

#include "HelloGUI.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(HelloGUI)

HelloGUI::HelloGUI(Context* context): Sample(context),
    uiRoot_(GetSubsystem<UI>()->GetRoot()),
    dragBeginPosition_(IntVector2::ZERO)
{
}

void HelloGUI::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    XMLFile* style{ cache->GetResource<XMLFile>("UI/DefaultStyle.xml") };

    // Set the loaded style as default style
    uiRoot_->SetDefaultStyle(style);

    // Initialize Window
    InitWindow();

    // Create and add some controls to the Window
    InitControls();

    // Create a draggable Emblem
    CreateDraggableEmblem();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void HelloGUI::InitControls()
{
    // Create a CheckBox
    CheckBox* checkBox{ new CheckBox{ context_ } };
    checkBox->SetName("CheckBox");

    // Create a Button
    Button* button{ new Button{ context_ } };
    button->SetName("Button");
    button->SetMinHeight(24);

    // Create a LineEdit
    LineEdit* lineEdit{ new LineEdit{ context_ } };
    lineEdit->SetName("LineEdit");
    lineEdit->SetMinHeight(24);

    // Add controls to Window
    window_->AddChild(checkBox);
    window_->AddChild(button);
    window_->AddChild(lineEdit);

    // Apply previously set default style
    checkBox->SetStyleAuto();
    button->SetStyleAuto();
    lineEdit->SetStyleAuto();
}

void HelloGUI::InitWindow()
{
    // Create the Window and add it to the UI's root node
    window_ = new Window{ context_ };
    uiRoot_->AddChild(window_);

    // Set Window size and layout settings
    window_->SetMinWidth(384);
    window_->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window_->SetAlignment(HA_CENTER, VA_CENTER);
    window_->SetName("Window");

    // Create Window 'titlebar' container
    UIElement* titleBar{ new UIElement{ context_ } };
    titleBar->SetMinSize(0, 24);
    titleBar->SetVerticalAlignment(VA_TOP);
    titleBar->SetLayoutMode(LM_HORIZONTAL);

    // Create the Window title Text
    Text* windowTitle{ new Text{ context_ } };
    windowTitle->SetName("WindowTitle");
    windowTitle->SetText("Hello GUI!");

    // Create the Window's close button
    Button* buttonClose{ new Button{ context_ } };
    buttonClose->SetName("CloseButton");

    // Add the controls to the title bar
    titleBar->AddChild(windowTitle);
    titleBar->AddChild(buttonClose);

    // Add the title bar to the Window
    window_->AddChild(titleBar);

    // Apply styles
    window_->SetStyleAuto();
    windowTitle->SetStyleAuto();
    buttonClose->SetStyle("CloseButton");

    // Subscribe to buttonClose release (following a 'press') events
    SubscribeToEvent(buttonClose, E_RELEASED, DRY_HANDLER(HelloGUI, HandleClosePressed));

    // Subscribe also to all UI mouse clicks just to see where we have clicked
    SubscribeToEvent(E_UIMOUSECLICK, DRY_HANDLER(HelloGUI, HandleControlClicked));
}

void HelloGUI::CreateDraggableEmblem()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Graphics* graphics{ GetSubsystem<Graphics>() };

    // Create a draggable Emblem button
    Button* draggableEmblem{ new Button{ context_ } };
    draggableEmblem->SetTexture(cache->GetResource<Texture2D>("Textures/DryDecal.png")); // Set texture
    draggableEmblem->SetBlendMode(BLEND_ADD);
    draggableEmblem->SetSize(128, 128);
    draggableEmblem->SetPosition((graphics->GetWidth() - draggableEmblem->GetWidth()) / 2, 200);
    draggableEmblem->SetName("Emblem");
    uiRoot_->AddChild(draggableEmblem);

    // Add a tooltip to Emblem button
    ToolTip* toolTip{ new ToolTip{ context_ } };
    draggableEmblem->AddChild(toolTip);
    toolTip->SetPosition(IntVector2(draggableEmblem->GetWidth() + 5, draggableEmblem->GetWidth() / 2)); // slightly offset from close button
    BorderImage* textHolder{ new BorderImage{ context_ } };
    toolTip->AddChild(textHolder);
    textHolder->SetStyle("ToolTipBorderImage");
    Text* toolTipText{ new Text{ context_ } };
    textHolder->AddChild(toolTipText);
    toolTipText->SetStyle("ToolTipText");
    toolTipText->SetText("Please drag me!");

    // Subscribe draggableEmblem to Drag Events (in order to make it draggable)
    // See "Event list" in documentation's Main Page for reference on available Events and their eventData
    SubscribeToEvent(draggableEmblem, E_DRAGBEGIN, DRY_HANDLER(HelloGUI, HandleDragBegin));
    SubscribeToEvent(draggableEmblem, E_DRAGMOVE, DRY_HANDLER(HelloGUI, HandleDragMove));
    SubscribeToEvent(draggableEmblem, E_DRAGEND, DRY_HANDLER(HelloGUI, HandleDragEnd));
}

void HelloGUI::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    // Get UIElement relative position where input (touch or click) occurred (top-left = IntVector2(0,0))
    dragBeginPosition_ = IntVector2{ eventData["ElementX"].GetInt(),
                                     eventData["ElementY"].GetInt() };
}

void HelloGUI::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    const IntVector2 dragCurrentPosition{ eventData["X"].GetInt(), eventData["Y"].GetInt() };
    UIElement* draggedElement{ static_cast<UIElement*>(eventData["Element"].GetPtr()) };
    draggedElement->SetPosition(dragCurrentPosition - dragBeginPosition_);
}

void HelloGUI::HandleDragEnd(StringHash eventType, VariantMap& eventData) // For reference (not used here)
{
}

void HelloGUI::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    if (GetPlatform() != "Web")
        engine_->Exit();
}

void HelloGUI::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    // Get the Text control acting as the Window's title
    Text* windowTitle{ window_->GetChildStaticCast<Text>("WindowTitle", true) };

    // Get control that was clicked
    UIElement* clicked{ static_cast<UIElement*>(eventData[UIMouseClick::P_ELEMENT].GetPtr()) };

    String name = "...?";

    if (clicked)
    {
        // Get the name of the control that was clicked
        name = clicked->GetName();
    }

    // Update the Window's title text
    windowTitle->SetText("Hello " + name + "!");
}

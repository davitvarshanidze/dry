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
#include <Dry/Graphics/Zone.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Technique.h>
#include <Dry/Graphics/Octree.h>
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
#include <Dry/UI/ListView.h>
#include <Dry/UI/UIComponent.h>

#include "Hello3DUI.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(Hello3DUI)

Hello3DUI::Hello3DUI(Context* context): Sample(context),
    uiRoot_{ GetSubsystem<UI>()->GetRoot() },
    dragBeginPosition_{ IntVector2::ZERO },
    animateCube_{ true },
    renderOnCube_{ false },
    drawDebug_{ false }
{
}

void Hello3DUI::Start()
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

    // Initialize Scene
    InitScene();

    // Initialize Window
    InitWindow();

    // Create and add some controls to the Window
    InitControls();

    // Create a draggable Emblem
    CreateDraggableEmblem();

    // Create 3D UI rendered on a cube.
    Init3DUI();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Hello3DUI::InitControls()
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

    CreateInstructions("Press Tab to toggle between rendering on screen or cube\n"
                       "Space toggles cube rotation");
}

void Hello3DUI::InitWindow()
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

    // Create a list.
    ListView* list{ window_->CreateChild<ListView>() };
    list->SetSelectOnClickEnd(true);
    list->SetHighlightMode(HM_ALWAYS);
    list->SetMinHeight(200);

    for (int i{ 0 }; i < 32; ++i)
    {
        Text* text{ new Text{ context_ } };
        text->SetStyleAuto();
        text->SetText(ToString("List item %d", i));
        text->SetName(ToString("Item %d", i));
        list->AddItem(text);
    }

    // Apply styles
    window_->SetStyleAuto();
    list->SetStyleAuto();
    windowTitle->SetStyleAuto();
    buttonClose->SetStyle("CloseButton");

    // Subscribe to buttonClose release (following a 'press') events
    SubscribeToEvent(buttonClose, E_RELEASED, DRY_HANDLER(Hello3DUI, HandleClosePressed));

    // Subscribe also to all UI mouse clicks just to see where we have clicked
    SubscribeToEvent(E_UIMOUSECLICK, DRY_HANDLER(Hello3DUI, HandleControlClicked));
}

void Hello3DUI::InitScene()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    scene_ = new Scene{ context_ };
    scene_->CreateComponent<Octree>();
    Zone* zone{ scene_->CreateComponent<Zone>() };
    zone->SetBoundingBox(BoundingBox{ -1000.0f, 1000.0f });
    zone->SetFogColor(Color::GRAY);
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a child scene node (at world origin) and a StaticModel component into it.
    Node* boxNode{ scene_->CreateChild("Box") };
    boxNode->SetScale({ 5.0f, 5.0f, 5.0f });
    boxNode->SetRotation(Quaternion{ 90.0f, Vector3::LEFT });

    // Create a box model and hide it initially.
    StaticModel* boxModel{ boxNode->CreateComponent<StaticModel>() };
    boxModel->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    boxNode->SetEnabled(false);

    // Create a camera.
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node.
    cameraNode_->SetPosition({ 0.0f, 0.0f, -10.0f });

    // Set up a viewport so 3D scene can be visible.
    Renderer* renderer{ GetSubsystem<Renderer>() };
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);

    // Subscribe to update event and animate cube and handle input.
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(Hello3DUI, HandleUpdate));
}

void Hello3DUI::CreateDraggableEmblem()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Graphics* graphics{ GetSubsystem<Graphics>() };

    // Create a draggable Emblem button
    Button* draggableEmblem{ new Button{ context_ } };
    draggableEmblem->SetTexture(cache->GetResource<Texture2D>("Textures/DryDecalAlpha.png")); // Set texture
    draggableEmblem->SetBlendMode(BLEND_ALPHA);
    draggableEmblem->SetSize(128, 128);
    draggableEmblem->SetPosition((graphics->GetWidth() - draggableEmblem->GetWidth()) / 2, 200);
    draggableEmblem->SetName("Emblem");
    uiRoot_->AddChild(draggableEmblem);

    // Add a tooltip to Emblem button
    ToolTip* toolTip{ new ToolTip{ context_ } };
    draggableEmblem->AddChild(toolTip);
    toolTip->SetPosition(IntVector2{ draggableEmblem->GetWidth() + 5, draggableEmblem->GetWidth() / 2 }); // slightly offset from close button
    BorderImage* textHolder{ new BorderImage{ context_ } };
    toolTip->AddChild(textHolder);
    textHolder->SetStyle("ToolTipBorderImage");
    Text* toolTipText{ new Text{ context_ } };
    textHolder->AddChild(toolTipText);
    toolTipText->SetStyle("ToolTipText");
    toolTipText->SetText("Please drag me!");

    // Subscribe draggableEmblem to Drag Events (in order to make it draggable)
    // See "Event list" in documentation's Main Page for reference on available Events and their eventData
    SubscribeToEvent(draggableEmblem, E_DRAGBEGIN, DRY_HANDLER(Hello3DUI, HandleDragBegin));
    SubscribeToEvent(draggableEmblem, E_DRAGMOVE, DRY_HANDLER(Hello3DUI, HandleDragMove));
    SubscribeToEvent(draggableEmblem, E_DRAGEND, DRY_HANDLER(Hello3DUI, HandleDragEnd));
}

void Hello3DUI::HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    // Get UIElement relative position where input (touch or click) occurred (top-left = IntVector2(0,0))
    dragBeginPosition_ = IntVector2(eventData["ElementX"].GetInt(), eventData["ElementY"].GetInt());
}

void Hello3DUI::HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    const IntVector2 dragCurrentPosition{ IntVector2{ eventData["X"].GetInt(), eventData["Y"].GetInt() } };
    UIElement* draggedElement{ static_cast<UIElement*>(eventData["Element"].GetPtr()) };

    draggedElement->SetPosition(dragCurrentPosition - dragBeginPosition_);
}

void Hello3DUI::HandleDragEnd(StringHash eventType, VariantMap& eventData) // For reference (not used here)
{
}

void Hello3DUI::HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    if (GetPlatform() != "Web")
        engine_->Exit();
}

void Hello3DUI::HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    // Get the Text control acting as the Window's title
    Text* windowTitle{ window_->GetChildStaticCast<Text>("WindowTitle", true) };

    // Get control that was clicked
    UIElement* clicked{ static_cast<UIElement*>(eventData[UIMouseClick::P_ELEMENT].GetPtr()) };
    String name = "...?";

    // Get the name of the control that was clicked
    if (clicked)
        name = clicked->GetName();

    // Update the Window's title text
    windowTitle->SetText("Hello " + name + "!");
}

void Hello3DUI::Init3DUI()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };

    // Node that will get UI rendered on it.
    Node* boxNode{ scene_->GetChild("Box") };
    // Create a component that sets up UI rendering. It sets material to StaticModel of the node.
    UIComponent* component{ boxNode->CreateComponent<UIComponent>() };
    // Optionally modify material. Technique is changed so object is visible without any lights.
    component->GetMaterial()->SetTechnique(0, cache->GetResource<Technique>("Techniques/DiffUnlitAlpha.xml"));
    // Save root element of texture UI for later use.
    textureRoot_ = component->GetRoot();
    // Set size of root element. This is size of texture as well.
    textureRoot_->SetSize(512, 512);
}

void Hello3DUI::HandleUpdate(StringHash, VariantMap& eventData)
{
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };
    Input* input{ GetSubsystem<Input>() };
    Node* node{ scene_->GetChild("Box") };

    if (!current_.IsNull() && drawDebug_)
        GetSubsystem<UI>()->DebugDraw(current_);

    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        current_ = GetSubsystem<UI>()->GetElementAt(input->GetMousePosition());

    if (input->GetKeyPress(KEY_TAB))
    {
        renderOnCube_ = !renderOnCube_;
        // Toggle between rendering on screen or to texture.
        if (renderOnCube_)
        {
            node->SetEnabled(true);
            textureRoot_->AddChild(window_);
        }
        else
        {
            node->SetEnabled(false);
            uiRoot_->AddChild(window_);
        }
    }

    if (input->GetKeyPress(KEY_SPACE))
        animateCube_ = !animateCube_;

    if (input->GetKeyPress(KEY_F2))
        drawDebug_ = !drawDebug_;

    if (animateCube_)
    {
        node->Yaw(6.0f * timeStep * 1.5f);
        node->Roll(-6.0f * timeStep * 1.5f);
        node->Pitch(-6.0f * timeStep * 1.5f);
    }
}

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
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Resource/Localization.h>
#include <Dry/Resource/ResourceEvents.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/Text3D.h>
#include <Dry/UI/UIEvents.h>
#include <Dry/UI/Window.h>

#include "L10n.h"

#include <Dry/DebugNew.h>

DRY_DEFINE_APPLICATION_MAIN(L10n)

L10n::L10n(Context* context): Sample(context)
{
}

void L10n::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable and center OS cursor
    Input* input{ GetSubsystem<Input>() };
    input->SetMouseVisible(true);
    input->CenterMousePosition();

    // Load strings from JSON files and subscribe to the change language event
    InitLocalizationSystem();

    // Init the 3D space
    CreateScene();

    // Init the user interface
    CreateGUI();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void L10n::InitLocalizationSystem()
{
    Localization* l10n{ GetSubsystem<Localization>() };
    // JSON files must be in UTF8 encoding without BOM
    // The first found language will be set as current
    l10n->LoadJSONFile("StringsEnRu.json");
    // You can load multiple files
    l10n->LoadJSONFile("StringsDe.json");
    l10n->LoadJSONFile("StringsLv.json", "lv");
    // Hook up to the change language
    SubscribeToEvent(E_CHANGELANGUAGE, DRY_HANDLER(L10n, HandleChangeLanguage));
}

void L10n::CreateGUI()
{
    // Get localization subsystem
    Localization* l10n{ GetSubsystem<Localization>() };

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    UIElement* root{ GetSubsystem<UI>()->GetRoot() };
    root->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    Window* window{ new Window{ context_ } };
    root->AddChild(window);
    window->SetMinSize(384, 192);
    window->SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window->SetAlignment(HA_CENTER, VA_CENTER);
    window->SetStyleAuto();

    Text* windowTitle{ new Text(context_) };
    windowTitle->SetName("WindowTitle");
    windowTitle->SetStyleAuto();
    window->AddChild(windowTitle);

    // In this place the current language is "en" because it was found first when loading the JSON files
    const String langName{ l10n->GetLanguage() };
    // Languages are numbered in the loading order
    const int langIndex{ l10n->GetLanguageIndex() }; // == 0 at the beginning
    // Get string with identifier "title" in the current language
    const String localizedString{ l10n->Get("title") };
    // Localization::Get returns String::EMPTY if the id is empty.
    // Localization::Get returns the id if translation is not found and will be added a warning into the log.

    windowTitle->SetText(localizedString + " (" + String(langIndex) + " " + langName + ")");

    Button* b{ new Button(context_) };
    window->AddChild(b);
    b->SetStyle("Button");
    b->SetMinHeight(24);

    Text* t{ b->CreateChild<Text>("ButtonTextChangeLang") };
    // The showing text value will automatically change when language is changed
    t->SetAutoLocalizable(true);
    // The text value used as a string identifier in this mode.
    // Remember that a letter case of the id and of the lang name is important.
    t->SetText("Press this button");

    t->SetAlignment(HA_CENTER, VA_CENTER);
    t->SetStyle("Text");
    SubscribeToEvent(b, E_RELEASED, DRY_HANDLER(L10n, HandleChangeLangButtonPressed));

    b = new Button{ context_ };
    window->AddChild(b);
    b->SetStyle("Button");
    b->SetMinHeight(24);
    t = b->CreateChild<Text>("ButtonTextQuit");
    t->SetAlignment(HA_CENTER, VA_CENTER);
    t->SetStyle("Text");

    // Manually set text in the current language
    t->SetText(l10n->Get("quit"));

    SubscribeToEvent(b, E_RELEASED, DRY_HANDLER(L10n, HandleQuitButtonPressed));
}

void L10n::CreateScene()
{
    // Get localization subsystem
    Localization* l10n{ GetSubsystem<Localization>() };

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    scene_ = new Scene{ context_ };
    scene_->CreateComponent<Octree>();

    Zone* zone{ scene_->CreateComponent<Zone>() };
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor({ 0.1f, 0.05f, 0.4f });
    zone->SetFogColor({ 0.125f, 0.0f, 0.15f });
    zone->SetFogStart(1.0f);
    zone->SetFogEnd(100.0f);

    Node* planeNode{ scene_->CreateChild("Plane") };
    planeNode->SetScale(Vector3(300.0f, 1.0f, 300.0f));
    StaticModel* planeObject{ planeNode->CreateComponent<StaticModel>() };
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/MudLeavesTiled.xml"));

    Node* lightNode{ scene_->CreateChild("DirectionalLight") };
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    Light* light{ lightNode->CreateComponent<Light>() };
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetColor({ 0.8f, 0.8f, 0.8f });

    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();
    cameraNode_->SetPosition(Vector3(0.0f, 10.0f, -30.0f));

    Node* text3DNode{ scene_->CreateChild("Text3D") };
    text3DNode->SetPosition({ 0.0f, 0.1f, 30.0f });
    text3DNode->SetScale(5);
    Text3D* text3D{ text3DNode->CreateComponent<Text3D>() };

    // Manually set text in the current language.
    text3D->SetText(l10n->Get("lang"));
    text3D->SetFont(cache->GetResource<Font>("Fonts/Days.ttf"), 42);
    text3D->SetColor({ 0.42f, 0.9f, 0.23f, 0.9f });
    text3D->SetAlignment(HA_CENTER, VA_BOTTOM);

    Renderer* renderer{ GetSubsystem<Renderer>() };
    SharedPtr<Viewport> viewport{ new Viewport{ context_, scene_, cameraNode_->GetComponent<Camera>() } };
    renderer->SetViewport(0, viewport);

    SubscribeToEvent(E_UPDATE, DRY_HANDLER(L10n, HandleUpdate));
}

void L10n::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };
    Input* input{ GetSubsystem<Input>() };
    const float MOUSE_SENSITIVITY{ 0.1f };
    const IntVector2 mouseMove{ input->GetMouseMove() };
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
}

void L10n::HandleChangeLangButtonPressed(StringHash eventType, VariantMap& eventData)
{
    Localization* l10n{ GetSubsystem<Localization>() };
    // Languages are numbered in the loading order
    int lang{ l10n->GetLanguageIndex() };
    lang++;

    if (lang >= l10n->GetNumLanguages())
        lang = 0;

    l10n->SetLanguage(lang);
}

void L10n::HandleQuitButtonPressed(StringHash eventType, VariantMap& eventData)
{
    if (GetPlatform() != "Web")
        engine_->Exit();
}

// You can manually change texts, sprites and other aspects of the game when language is changed
void L10n::HandleChangeLanguage(StringHash eventType, VariantMap& eventData)
{
    Localization* l10n{ GetSubsystem<Localization>() };
    UIElement* uiRoot{ GetSubsystem<UI>()->GetRoot() };

    Text* windowTitle{ uiRoot->GetChildStaticCast<Text>("WindowTitle", true) };
    windowTitle->SetText(l10n->Get("title") + " (" + String{ l10n->GetLanguageIndex() } + " " + l10n->GetLanguage() + ")");

    Text* buttonText{ uiRoot->GetChildStaticCast<Text>("ButtonTextQuit", true) };
    buttonText->SetText(l10n->Get("quit"));

    Text3D* text3D{ scene_->GetChild("Text3D")->GetComponent<Text3D>() };
    text3D->SetText(l10n->Get("lang"));

    // A text on the button "Press this button" changes automatically
}

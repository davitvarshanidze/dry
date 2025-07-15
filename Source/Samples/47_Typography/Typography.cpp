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
#include <Dry/Core/ProcessUtils.h>
#include <Dry/Graphics/RenderPath.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/UI/CheckBox.h>
#include <Dry/UI/DropDownList.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>

#include "Typography.h"

#include <Dry/DebugNew.h>

// Expands to this example's entry-point
DRY_DEFINE_APPLICATION_MAIN(Typography)

namespace
{
    // Tag used to find all Text elements
    const char* TEXT_TAG{ "Typography_text_tag" };
}

Typography::Typography(Context* context): Sample(context)
{
}

void Typography::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Load XML file containing default UI style sheet
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    XMLFile* style{ cache->GetResource<XMLFile>("UI/DefaultStyle.xml") };

    // Set the loaded style as default style
    UI* ui{ GetSubsystem<UI>() };
    UIElement* root{ ui->GetRoot() };
    root->SetDefaultStyle(style);

    // Create a UIElement to hold all our content
    // (Don't modify the root directly, as the base Sample class uses it)
    uielement_ = new UIElement{ context_ };
    uielement_->SetAlignment(HA_CENTER, VA_CENTER);
    uielement_->SetLayout(LM_VERTICAL, 10, IntRect{ 20, 40, 20, 40 });
    root->AddChild(uielement_);

    // Add some sample text.
    CreateText();

    // Add a checkbox to toggle the background color.
    CreateCheckbox("White background", DRY_HANDLER(Typography, HandleWhiteBackground))
        ->SetChecked(false);

    // Add a checkbox to toggle SRGB output conversion (if available).
    // This will give more correct text output for FreeType fonts, as the FreeType rasterizer
    // outputs linear coverage values rather than SRGB values. However, this feature isn't
    // available on all platforms.
    CreateCheckbox("Graphics::SetSRGB", DRY_HANDLER(Typography, HandleSRGB))
        ->SetChecked(GetSubsystem<Graphics>()->GetSRGB());

    // Add a checkbox for the global ForceAutoHint setting. This affects character spacing.
    CreateCheckbox("UI::SetForceAutoHint", DRY_HANDLER(Typography, HandleForceAutoHint))
        ->SetChecked(ui->GetForceAutoHint());

    // Add a drop-down menu to control the font hinting level.
    const char* levels[] = {
        "FONT_HINT_LEVEL_NONE",
        "FONT_HINT_LEVEL_LIGHT",
        "FONT_HINT_LEVEL_NORMAL",
        nullptr
    };
    CreateMenu("UI::SetFontHintLevel", levels, DRY_HANDLER(Typography, HandleFontHintLevel))
        ->SetSelection(ui->GetFontHintLevel());

    // Add a drop-down menu to control the subpixel threshold.
    const char* thresholds[] = {
        "0",
        "3",
        "6",
        "9",
        "12",
        "15",
        "18",
        "21",
        nullptr
    };
    CreateMenu("UI::SetFontSubpixelThreshold", thresholds, DRY_HANDLER(Typography, HandleFontSubpixel))
        ->SetSelection(ui->GetFontSubpixelThreshold() / 3);

    // Add a drop-down menu to control oversampling.
    const char* limits[] = {
        "1",
        "2",
        "3",
        "4",
        "5",
        "6",
        "7",
        "8",
        nullptr
    };
    CreateMenu("UI::SetFontOversampling", limits, DRY_HANDLER(Typography, HandleFontOversampling))
        ->SetSelection(ui->GetFontOversampling() - 1);

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void Typography::CreateText()
{
    SharedPtr<UIElement> container(new UIElement(context_));
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_VERTICAL);
    uielement_->AddChild(container);

    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };

    for (int size2x{ 2 }; size2x <= 36; ++size2x)
    {
        float size{ size2x / 2.0f };

        SharedPtr<Text> text{ new Text{ context_ } };
        text->SetText(String{ "The quick brown fox jumps over the lazy dog (" } + String{ size } + String{ "pt)" });
        text->SetFont(font, size);
        text->AddTag(TEXT_TAG);
        container->AddChild(text);
    }
}

SharedPtr<CheckBox> Typography::CreateCheckbox(const String& label, EventHandler* handler)
{
    SharedPtr<UIElement> container{ new UIElement{ context_ } };
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_HORIZONTAL, 8);
    uielement_->AddChild(container);

    SharedPtr<CheckBox> box{ new CheckBox{ context_ } };
    container->AddChild(box);
    box->SetStyleAuto();

    SharedPtr<Text> text{ new Text{ context_ } };
    container->AddChild(text);
    text->SetText(label);
    text->SetStyleAuto();
    text->AddTag(TEXT_TAG);

    SubscribeToEvent(box, E_TOGGLED, handler);
    return box;
}

SharedPtr<DropDownList> Typography::CreateMenu(const String& label, const char** items, EventHandler* handler)
{
    SharedPtr<UIElement> container{ new UIElement{ context_ } };
    container->SetAlignment(HA_LEFT, VA_TOP);
    container->SetLayout(LM_HORIZONTAL, 8);
    uielement_->AddChild(container);

    SharedPtr<Text> text{ new Text{ context_ } };
    container->AddChild(text);
    text->SetText(label);
    text->SetStyleAuto();
    text->AddTag(TEXT_TAG);

    SharedPtr<DropDownList> list{ new DropDownList{ context_ } };
    container->AddChild(list);
    list->SetStyleAuto();

    for (int i{ 0 }; items[i]; ++i)
    {
        SharedPtr<Text> item{ new Text{ context_ } };
        list->AddItem(item);
        item->SetText(items[i]);
        item->SetStyleAuto();
        item->SetMinWidth(item->GetRowWidth(0) + 10);
        item->AddTag(TEXT_TAG);
    }

    text->SetMaxWidth(text->GetRowWidth(0));

    SubscribeToEvent(list, E_ITEMSELECTED, handler);

    return list;
}

void Typography::HandleWhiteBackground(StringHash eventType, VariantMap& eventData)
{
    CheckBox* box{ static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const bool checked{ box->IsChecked() };

    const Color fg{ (checked ? Color::BLACK : Color::WHITE) };
    const Color bg{ (checked ? Color::WHITE : Color::BLACK) };

    Renderer* renderer{ GetSubsystem<Renderer>() };
    Zone* zone{ renderer->GetDefaultZone() };
    zone->SetFogColor(bg);

    PODVector<UIElement*> text{ uielement_->GetChildrenWithTag(TEXT_TAG, true) };

    for (int i{ 0 }; i < text.Size(); ++i)
        text[i]->SetColor(fg);
}

void Typography::HandleForceAutoHint(StringHash eventType, VariantMap& eventData)
{
    CheckBox* box{ static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const bool checked{ box->IsChecked() };

    GetSubsystem<UI>()->SetForceAutoHint(checked);
}

void Typography::HandleSRGB(StringHash eventType, VariantMap& eventData)
{
    Graphics* graphics{ GetSubsystem<Graphics>() };

    CheckBox* box{ static_cast<CheckBox*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const bool checked{ box->IsChecked() };

    if (graphics->GetSRGBWriteSupport())
        graphics->SetSRGB(checked);
    else
        DRY_LOGWARNING("Graphics::GetSRGBWriteSupport returned false");
        // Note: PostProcess/GammaCorrection.xml implements SRGB conversion.
        // However, post-processing filters don't affect the UI layer.
}

void Typography::HandleFontHintLevel(StringHash eventType, VariantMap& eventData)
{
    DropDownList* list{ static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const unsigned i{ list->GetSelection() };

    GetSubsystem<UI>()->SetFontHintLevel(static_cast<FontHintLevel>(i));
}

void Typography::HandleFontSubpixel(StringHash eventType, VariantMap& eventData)
{
    DropDownList* list{ static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const unsigned i{ list->GetSelection() };

    GetSubsystem<UI>()->SetFontSubpixelThreshold(i * 3);
}

void Typography::HandleFontOversampling(StringHash eventType, VariantMap& eventData)
{
    DropDownList* list{ static_cast<DropDownList*>(eventData[Toggled::P_ELEMENT].GetPtr()) };
    const unsigned i{ list->GetSelection() };

    GetSubsystem<UI>()->SetFontOversampling(i + 1);
}

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

#include <Dry/Audio/Audio.h>
#include <Dry/Audio/AudioEvents.h>
#include <Dry/Audio/Sound.h>
#include <Dry/Audio/SoundSource.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/Log.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Scene/Scene.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Slider.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>

#include "SoundEffects.h"

#include <Dry/DebugNew.h>

// Custom variable identifier for storing sound effect name within the UI element
static const StringHash VAR_SOUNDRESOURCE("SoundResource");
static const unsigned NUM_SOUNDS = 3;

static const char* soundNames[] = {
    "Fist",
    "Explosion",
    "Power-up"
};

static const char* soundResourceNames[] = {
    "Sounds/PlayerFistHit.wav",
    "Sounds/BigExplosion.wav",
    "Sounds/Powerup.wav"
};

DRY_DEFINE_APPLICATION_MAIN(SoundEffects)

SoundEffects::SoundEffects(Context* context) :
    Sample(context),
    musicSource_(nullptr)
{
}

void SoundEffects::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void SoundEffects::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create a scene which will not be actually rendered, but is used to hold SoundSource components while they play sounds
    scene_ = new Scene{ context_ };

    // Create music sound source
    musicSource_ = scene_->CreateComponent<SoundSource>();
    // Set the sound type to music so that master volume control works correctly
    musicSource_->SetSoundType(SOUND_MUSIC);

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the user interface
    CreateUI();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void SoundEffects::CreateUI()
{
    UIElement* root{ GetSubsystem<UI>()->GetRoot() };
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    XMLFile* uiStyle{ cache->GetResource<XMLFile>("UI/DefaultStyle.xml") };
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    // Create buttons for playing back sounds
    for (unsigned i{ 0 }; i < NUM_SOUNDS; ++i)
    {
        Button* button{ CreateButton(i * 140 + 20, 20, 120, 40, soundNames[i]) };
        // Store the sound effect resource name as a custom variable into the button
        button->SetVar(VAR_SOUNDRESOURCE, soundResourceNames[i]);
        SubscribeToEvent(button, E_PRESSED, DRY_HANDLER(SoundEffects, HandlePlaySound));
    }

    // Create buttons for playing/stopping music
    Button* button{ CreateButton(20, 80, 120, 40, "Play Music") };
    SubscribeToEvent(button, E_RELEASED, DRY_HANDLER(SoundEffects, HandlePlayMusic));

    button = CreateButton(160, 80, 120, 40, "Stop Music");
    SubscribeToEvent(button, E_RELEASED, DRY_HANDLER(SoundEffects, HandleStopMusic));

    Audio* audio{ GetSubsystem<Audio>() };

    // Create sliders for controlling sound and music master volume
    Slider* slider{ CreateSlider(20, 140, 200, 20, "Sound Volume") };
    slider->SetValue(audio->GetMasterGain(SOUND_EFFECT));
    SubscribeToEvent(slider, E_SLIDERCHANGED, DRY_HANDLER(SoundEffects, HandleSoundVolume));

    slider = CreateSlider(20, 200, 200, 20, "Music Volume");
    slider->SetValue(audio->GetMasterGain(SOUND_MUSIC));
    SubscribeToEvent(slider, E_SLIDERCHANGED, DRY_HANDLER(SoundEffects, HandleMusicVolume));
}

Button* SoundEffects::CreateButton(int x, int y, int xSize, int ySize, const String& text)
{
    UIElement* root{ GetSubsystem<UI>()->GetRoot() };
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };

    // Create the button and center the text onto it
    Button* button{ root->CreateChild<Button>() };
    button->SetStyleAuto();
    button->SetPosition(x, y);
    button->SetSize(xSize, ySize);

    Text* buttonText{ button->CreateChild<Text>() };
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetFont(font, 12);
    buttonText->SetText(text);

    return button;
}

Slider* SoundEffects::CreateSlider(int x, int y, int xSize, int ySize, const String& text)
{
    UIElement* root{ GetSubsystem<UI>()->GetRoot() };
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };

    // Create text and slider below it
    Text* sliderText{ root->CreateChild<Text>() };
    sliderText->SetPosition(x, y);
    sliderText->SetFont(font, 12);
    sliderText->SetText(text);

    Slider* slider{ root->CreateChild<Slider>() };
    slider->SetStyleAuto();
    slider->SetPosition(x, y + 20);
    slider->SetSize(xSize, ySize);
    // Use 0-1 range for controlling sound/music master volume
    slider->SetRange(1.0f);

    return slider;
}

void SoundEffects::HandlePlaySound(StringHash eventType, VariantMap& eventData)
{
    Button* button{ static_cast<Button*>(GetEventSender()) };
    const String& soundResourceName{ button->GetVar(VAR_SOUNDRESOURCE).GetString() };

    // Get the sound resource
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Sound* sound{ cache->GetResource<Sound>(soundResourceName) };

    if (sound)
    {
        // Create a SoundSource component for playing the sound. The SoundSource component plays
        // non-positional audio, so its 3D position in the scene does not matter. For positional sounds the
        // SoundSource3D component would be used instead
        SoundSource* soundSource{ scene_->CreateComponent<SoundSource>() };
        // Component will automatically remove itself when the sound finished playing
        soundSource->SetAutoRemoveMode(REMOVE_COMPONENT);
        soundSource->Play(sound);
        // In case we also play music, set the sound volume below maximum so that we don't clip the output
        soundSource->SetGain(0.75f);
    }
}

void SoundEffects::HandlePlayMusic(StringHash eventType, VariantMap& eventData)
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Sound* music{ cache->GetResource<Sound>("Music/Ninja Gods.ogg") };
    // Set the song to loop
    music->SetLooped(true);

    musicSource_->Play(music);
}

void SoundEffects::HandleStopMusic(StringHash eventType, VariantMap& eventData)
{
    // Remove the music player node from the scene
    musicSource_->Stop();
}

void SoundEffects::HandleSoundVolume(StringHash eventType, VariantMap& eventData)
{
    const float newVolume{ eventData[SliderChanged::P_VALUE].GetFloat() };
    GetSubsystem<Audio>()->SetMasterGain(SOUND_EFFECT, newVolume);
}

void SoundEffects::HandleMusicVolume(StringHash eventType, VariantMap& eventData)
{
    const float newVolume{ eventData[SliderChanged::P_VALUE].GetFloat() };
    GetSubsystem<Audio>()->SetMasterGain(SOUND_MUSIC, newVolume);
}

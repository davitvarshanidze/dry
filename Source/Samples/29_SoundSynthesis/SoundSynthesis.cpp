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

#include <Dry/Audio/BufferedSoundStream.h>
#include <Dry/Audio/SoundSource.h>
#include <Dry/Core/CoreEvents.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/Log.h>
#include <Dry/Scene/Node.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>

#include "SoundSynthesis.h"

#include <Dry/DebugNew.h>

// Expands to this example's entry-point
DRY_DEFINE_APPLICATION_MAIN(SoundSynthesis)

SoundSynthesis::SoundSynthesis(Context* context): Sample(context),
    filter_{ 0.42f },
    accumulator_{ 0.0f },
    osc1_{ 0.f },
    osc2_{ 0.5f },
    harmonic_{ PT_HARMONIC_SIN }
{
    // Create harmonic oscillator
    harmonic_.SetCoefficients({ .0f, .3f, .1f, -.05f, .2f, -.1f });
    harmonic_.SetSlope({ 0.f, 23.f });
}

void SoundSynthesis::Setup()
{
    // Modify engine startup parameters
    Sample::Setup();
    engineParameters_[EP_SOUND] = true;
}

void SoundSynthesis::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the sound stream & start playback
    CreateSound();

    // Create the UI content
    CreateInstructions();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void SoundSynthesis::CreateSound()
{
    // Sound source needs a node so that it is considered enabled
    node_ = new Node{ context_ };
    SoundSource* source{ node_->CreateComponent<SoundSource>() };

    soundStream_ = new BufferedSoundStream{};
    // Set format: 44100 Hz, sixteen bit, mono
    soundStream_->SetFormat(44100, true, false);

    // Start playback. We don't have data in the stream yet, but the SoundSource will wait until there is data,
    // as the stream is by default in the "don't stop at end" mode
    source->Play(soundStream_);
}

void SoundSynthesis::UpdateSound()
{
    // Try to keep 1/10 seconds of sound in the buffer, to avoid both dropouts and unnecessary latency
    float targetLength{ 1.0f / 10.0f };
    float requiredLength{ targetLength - soundStream_->GetBufferLength() };

    if (requiredLength < 0.0f)
        return;

    unsigned numSamples{ static_cast<unsigned>(soundStream_->GetFrequency() * requiredLength) };

    if (!numSamples)
        return;

    // Allocate a new buffer and fill it with a simple two-oscillator algorithm. The sound is over-amplified
    // (distorted), clamped to the 16-bit range, and finally lowpass-filtered according to the coefficient
    SharedArrayPtr<signed short> newData{ new signed short[numSamples] };

    for (unsigned i{ 0 }; i < numSamples; ++i)
    {
        osc1_ = fmodf(osc1_ + 1.f/36000.f, 1.f/harmonic_.GetSlope().y_);
        osc2_ = fmodf(osc2_ + 1.f/35850.f, 1.f/harmonic_.GetSlope().y_);

        const float newValue{ Clamp(harmonic_.Solve(osc1_) * harmonic_.Solve(osc2_) * 100000.0f, -32767.0f, 32767.0f) };
        accumulator_ = Lerp(accumulator_, newValue, Pow(1.f - filter_, 8.f));
        newData[i] = static_cast<int>(accumulator_);
    }

    // Queue buffer to the stream for playback
    soundStream_->AddData(newData, numSamples * sizeof(signed short));
}

void SoundSynthesis::CreateInstructions()
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    UI* ui{ GetSubsystem<UI>() };

    // Construct new Text object, set string to display and font to use
    instructionText_ = ui->GetRoot()->CreateChild<Text>();
    instructionText_->SetText("Use cursor up and down to control sound filtering");
    instructionText_->SetFont(cache->GetResource<Font>("Fonts/Philosopher.ttf"), 15);

    // Position the text relative to the screen center
    instructionText_->SetTextAlignment(HA_CENTER);
    instructionText_->SetHorizontalAlignment(HA_CENTER);
    instructionText_->SetVerticalAlignment(VA_CENTER);
    instructionText_->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void SoundSynthesis::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, DRY_HANDLER(SoundSynthesis, HandleUpdate));
}

void SoundSynthesis::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    const float timeStep{ eventData[Update::P_TIMESTEP].GetFloat() };

    // Use keys to control the filter constant
    Input* input{ GetSubsystem<Input>() };



    if (input->GetKeyDown(KEY_UP))
        filter_ += timeStep * 0.5f;
    if (input->GetKeyDown(KEY_DOWN))
        filter_ -= timeStep * 0.5f;

    if (input->GetKeyDown(KEY_RIGHT))
        harmonic_.GetSlope().y_ += timeStep * (3.f + Sqrt(harmonic_.GetSlope().y_));
    if (input->GetKeyDown(KEY_LEFT))
        harmonic_.GetSlope().y_ -= timeStep * (3.f + Sqrt(harmonic_.GetSlope().y_));

    harmonic_.GetSlope().y_ = Clamp(harmonic_.GetSlope().y_, 0.01f, 512.f);

    filter_ = Clamp(filter_, .0f, .999f);

    instructionText_->SetText("Use cursor up and down to control sound filtering\nLeft and right to change pitch\n"
        "Coefficient: " + String(filter_));

    UpdateSound();
}

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

#include "../Precompiled.h"

#include "../Audio/Audio.h"
#include "../Audio/Sound.h"
#include "../Audio/SoundListener.h"
#include "../Audio/SoundSource3D.h"
#include "../Core/Context.h"
#include "../Graphics/DebugRenderer.h"
#include "../Scene/Node.h"

namespace Dry
{

static const float DEFAULT_NEARDISTANCE = 0.0f;
static const float DEFAULT_FARDISTANCE = 100.0f;
static const float DEFAULT_ROLLOFF = 2.0f;
static const float DEFAULT_ANGLE = 360.0f;
static const float MIN_ROLLOFF = 0.1f;
static const Color INNER_COLOR(1.0f, 0.5f, 1.0f);
static const Color OUTER_COLOR(1.0f, 0.0f, 1.0f);

extern const char* DRY_AUDIO_CATEGORY;

SoundSource3D::SoundSource3D(Context* context) :
    SoundSource(context),
    nearDistance_(DEFAULT_NEARDISTANCE),
    farDistance_(DEFAULT_FARDISTANCE),
    innerAngle_(DEFAULT_ANGLE),
    outerAngle_(DEFAULT_ANGLE),
    rolloffFactor_(DEFAULT_ROLLOFF)
{
    // Start from zero volume until attenuation properly calculated
    attenuation_ = 0.f;
}

void SoundSource3D::RegisterObject(Context* context)
{
    context->RegisterFactory<SoundSource3D>(DRY_AUDIO_CATEGORY);

    DRY_COPY_BASE_ATTRIBUTES(SoundSource);
    // Remove Attenuation and Panning as attribute as they are constantly being updated
    DRY_REMOVE_ATTRIBUTE("Attenuation");
    DRY_REMOVE_ATTRIBUTE("Panning");
    DRY_ATTRIBUTE("Near Distance", float, nearDistance_, DEFAULT_NEARDISTANCE, AM_DEFAULT);
    DRY_ATTRIBUTE("Far Distance", float, farDistance_, DEFAULT_FARDISTANCE, AM_DEFAULT);
    DRY_ATTRIBUTE("Inner Angle", float, innerAngle_, DEFAULT_ANGLE, AM_DEFAULT);
    DRY_ATTRIBUTE("Outer Angle", float, outerAngle_, DEFAULT_ANGLE, AM_DEFAULT);
    DRY_ATTRIBUTE("Rolloff Factor", float, rolloffFactor_, DEFAULT_ROLLOFF, AM_DEFAULT);
}

void SoundSource3D::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!debug || !node_ || !IsEnabledEffective())
        return;

    const Matrix3x4& worldTransform{ node_->GetWorldTransform() };
    const Vector3 worldPosition{ worldTransform.Translation() };
    const Quaternion worldRotation{ worldTransform.Rotation() };

    // Draw cones for directional sounds, or spheres for non-directional
    if (innerAngle_ < DEFAULT_ANGLE && outerAngle_ > 0.0f)
    {
        const Quaternion rotation = worldRotation * Quaternion(Vector3::UP, Vector3::FORWARD);
        debug->AddSphereSector(Sphere(worldPosition, nearDistance_), rotation, innerAngle_, false, INNER_COLOR, depthTest);
        debug->AddSphereSector(Sphere(worldPosition, nearDistance_), rotation, outerAngle_, false, OUTER_COLOR, depthTest);
        debug->AddSphereSector(Sphere(worldPosition, farDistance_), rotation, innerAngle_, true, INNER_COLOR, depthTest);
        debug->AddSphereSector(Sphere(worldPosition, farDistance_), rotation, outerAngle_, true, OUTER_COLOR, depthTest);
    }
    else
    {
        debug->AddSphere(Sphere(worldPosition, nearDistance_), INNER_COLOR, depthTest);
        debug->AddSphere(Sphere(worldPosition, farDistance_), OUTER_COLOR, depthTest);
    }
}

void SoundSource3D::Update(float timeStep)
{
    CalculateAttenuation();
    SoundSource::Update(timeStep);
}

void SoundSource3D::SetDistanceAttenuation(float nearDistance, float farDistance, float rolloffFactor)
{
    nearDistance_ = Max(nearDistance, 0.0f);
    farDistance_ = Max(farDistance, 0.0f);
    rolloffFactor_ = Max(rolloffFactor, MIN_ROLLOFF);
    MarkNetworkUpdate();
}

void SoundSource3D::SetAngleAttenuation(float innerAngle, float outerAngle)
{
    innerAngle_ = Clamp(innerAngle, 0.0f, DEFAULT_ANGLE);
    outerAngle_ = Clamp(outerAngle, 0.0f, DEFAULT_ANGLE);
    MarkNetworkUpdate();
}

void SoundSource3D::SetFarDistance(float distance)
{
    farDistance_ = Max(distance, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource3D::SetNearDistance(float distance)
{
    nearDistance_ = Max(distance, 0.0f);
    MarkNetworkUpdate();
}

void SoundSource3D::SetInnerAngle(float angle)
{
    innerAngle_ = Clamp(angle, 0.0f, DEFAULT_ANGLE);
    MarkNetworkUpdate();
}

void SoundSource3D::SetOuterAngle(float angle)
{
    outerAngle_ = Clamp(angle, 0.0f, DEFAULT_ANGLE);
    MarkNetworkUpdate();
}

void SoundSource3D::SetRolloffFactor(float factor)
{
    rolloffFactor_ = Max(factor, MIN_ROLLOFF);
    MarkNetworkUpdate();
}

void SoundSource3D::CalculateAttenuation()
{
    if (!audio_)
        return;

    attenuation_ = 0.f;
    const float interval{ farDistance_ - nearDistance_ };

    if (node_)
    {
        PODVector<Pair<float, float> > panning{};

        for (SoundListener* listener: audio_->GetListeners())
        {
            float attenuation{ 0.f };

            // Listener must either be sceneless or in the same scene, else attenuate sound to silence
            if (listener && listener->IsEnabledEffective() && (!listener->GetScene() || listener->GetScene() == GetScene()))
            {
                Node* listenerNode{ listener->GetNode() };
                const Vector3 relativePos{ listenerNode->GetWorldRotation().Inverse() *
                            (node_->GetWorldPosition() - listenerNode->GetWorldPosition()) };
                const float distance{ relativePos.Length() };

                // Distance attenuation
                if (interval > 0.f)
                    attenuation = powf(1.f - Clamp(distance - nearDistance_, 0.f, interval) / interval, rolloffFactor_);
                else
                    attenuation = (distance <= nearDistance_ ? 1.f : 0.f);

                // Panning
                panning.Push({ relativePos.Normalized().x_, attenuation });

                // Angle attenuation
                if (innerAngle_ < DEFAULT_ANGLE && outerAngle_ > 0.f)
                {
                    const Vector3 listenerRelativePos{ node_->GetWorldRotation().Inverse() *
                                (listenerNode->GetWorldPosition() - node_->GetWorldPosition()) };
                    const float listenerDot  { Vector3::FORWARD.DotProduct(listenerRelativePos.Normalized()) };
                    const float listenerAngle{ acosf(listenerDot) * M_RADTODEG * 2.f };
                    const float angleInterval{ Max(outerAngle_ - innerAngle_, 0.f) };
                    float angleAttenuation{ 1.f };

                    if (angleInterval > 0.f)
                    {
                        if (listenerAngle > innerAngle_)
                        {
                            angleAttenuation = powf(1.f - Clamp(listenerAngle - innerAngle_, 0.f, angleInterval) / angleInterval,
                                                    rolloffFactor_);
                        }
                    }
                    else
                    {
                        angleAttenuation = listenerAngle <= innerAngle_ ? 1.f : 0.f;
                    }

                    attenuation *= angleAttenuation;
                }
            }

            attenuation_ = Max(attenuation_, attenuation);
        }

        float accumulativePan{};
        float totalWeight{};
        for (Pair<float, float> pan: panning)
        {
            accumulativePan += pan.first_ * pan.second_;
            totalWeight += pan.second_;
        }

        if (totalWeight > 0.f)
            panning_ = accumulativePan / totalWeight;
        else
            panning_ = 0.f;
    }
}
}

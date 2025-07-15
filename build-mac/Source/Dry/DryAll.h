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

#include <Dry/Dry.h>

#if DRY_2D
#include <Dry/2D/2D.h>
#include <Dry/2D/AnimatedSprite2D.h>
#include <Dry/2D/AnimationSet2D.h>
#include <Dry/2D/CollisionBox2D.h>
#include <Dry/2D/CollisionChain2D.h>
#include <Dry/2D/CollisionCircle2D.h>
#include <Dry/2D/CollisionEdge2D.h>
#include <Dry/2D/CollisionPolygon2D.h>
#include <Dry/2D/CollisionShape2D.h>
#include <Dry/2D/Constraint2D.h>
#include <Dry/2D/ConstraintDistance2D.h>
#include <Dry/2D/ConstraintFriction2D.h>
#include <Dry/2D/ConstraintGear2D.h>
#include <Dry/2D/ConstraintMotor2D.h>
#include <Dry/2D/ConstraintMouse2D.h>
#include <Dry/2D/ConstraintPrismatic2D.h>
#include <Dry/2D/ConstraintPulley2D.h>
#include <Dry/2D/ConstraintRevolute2D.h>
#include <Dry/2D/ConstraintRope2D.h>
#include <Dry/2D/ConstraintWeld2D.h>
#include <Dry/2D/ConstraintWheel2D.h>
#include <Dry/2D/Drawable2D.h>
#include <Dry/2D/ParticleEffect2D.h>
#include <Dry/2D/ParticleEmitter2D.h>
#include <Dry/2D/ParticleEvents2D.h>
#include <Dry/2D/PhysicsEvents2D.h>
#include <Dry/2D/PhysicsUtils2D.h>
#include <Dry/2D/PhysicsWorld2D.h>
#include <Dry/2D/Renderer2D.h>
#include <Dry/2D/RigidBody2D.h>
#include <Dry/2D/Sprite2D.h>
#include <Dry/2D/SpriteSheet2D.h>
#include <Dry/2D/SpriterData2D.h>
#include <Dry/2D/SpriterInstance2D.h>
#include <Dry/2D/StaticSprite2D.h>
#include <Dry/2D/StretchableSprite2D.h>
#include <Dry/2D/TileMap2D.h>
#include <Dry/2D/TileMapDefs2D.h>
#include <Dry/2D/TileMapLayer2D.h>
#include <Dry/2D/TmxFile2D.h>
#endif
#if DRY_ANGELSCRIPT
#include <Dry/AngelScript/APITemplates.h>
#include <Dry/AngelScript/Addons.h>
#include <Dry/AngelScript/Script.h>
#include <Dry/AngelScript/ScriptAPI.h>
#include <Dry/AngelScript/ScriptEventListener.h>
#include <Dry/AngelScript/ScriptFile.h>
#include <Dry/AngelScript/ScriptInstance.h>
#endif
#include <Dry/Audio/Audio.h>
#include <Dry/Audio/AudioDefs.h>
#include <Dry/Audio/AudioEvents.h>
#include <Dry/Audio/BufferedSoundStream.h>
#include <Dry/Audio/OggVorbisSoundStream.h>
#include <Dry/Audio/Sound.h>
#include <Dry/Audio/SoundListener.h>
#include <Dry/Audio/SoundSource.h>
#include <Dry/Audio/SoundSource3D.h>
#include <Dry/Audio/SoundStream.h>
#include <Dry/Base/Algorithm.h>
#include <Dry/Base/Iter.h>
#include <Dry/Container/Allocator.h>
#include <Dry/Container/ArrayPtr.h>
#include <Dry/Container/FlagSet.h>
#include <Dry/Container/ForEach.h>
#include <Dry/Container/Hash.h>
#include <Dry/Container/HashBase.h>
#include <Dry/Container/HashMap.h>
#include <Dry/Container/HashSet.h>
#include <Dry/Container/LinkedList.h>
#include <Dry/Container/List.h>
#include <Dry/Container/ListBase.h>
#include <Dry/Container/Pair.h>
#include <Dry/Container/Ptr.h>
#include <Dry/Container/RefCounted.h>
#include <Dry/Container/Sort.h>
#include <Dry/Container/Str.h>
#include <Dry/Container/Swap.h>
#include <Dry/Container/Vector.h>
#include <Dry/Container/VectorBase.h>
#include <Dry/Core/Attribute.h>
#include <Dry/Core/Condition.h>
#include <Dry/Core/Context.h>
#include <Dry/Core/CoreEvents.h>
#include <Dry/Core/EventProfiler.h>
#include <Dry/Core/Main.h>
#include <Dry/Core/MiniDump.h>
#include <Dry/Core/Mutex.h>
#include <Dry/Core/Object.h>
#include <Dry/Core/ProcessUtils.h>
#include <Dry/Core/Profiler.h>
#include <Dry/Core/Spline.h>
#include <Dry/Core/StringHashRegister.h>
#include <Dry/Core/StringUtils.h>
#include <Dry/Core/Thread.h>
#include <Dry/Core/Timer.h>
#include <Dry/Core/Variant.h>
#include <Dry/Core/WorkQueue.h>
#if DRY_DATABASE
#include <Dry/Database/Database.h>
#include <Dry/Database/DatabaseEvents.h>
#include <Dry/Database/DbConnection.h>
#include <Dry/Database/DbResult.h>
#endif
#include <Dry/Engine/Application.h>
#include <Dry/Engine/Console.h>
#include <Dry/Engine/DebugHud.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Engine/EngineDefs.h>
#include <Dry/Engine/EngineEvents.h>
#include <Dry/Graphics/AnimatedModel.h>
#include <Dry/Graphics/Animation.h>
#include <Dry/Graphics/AnimationController.h>
#include <Dry/Graphics/AnimationState.h>
#include <Dry/Graphics/Batch.h>
#include <Dry/Graphics/BillboardSet.h>
#include <Dry/Graphics/Camera.h>
#include <Dry/Graphics/ConstantBuffer.h>
#include <Dry/Graphics/CustomGeometry.h>
#include <Dry/Graphics/DebugRenderer.h>
#include <Dry/Graphics/DecalSet.h>
#include <Dry/Graphics/Drawable.h>
#include <Dry/Graphics/DrawableEvents.h>
#include <Dry/Graphics/GPUObject.h>
#include <Dry/Graphics/Geometry.h>
#include <Dry/Graphics/Graphics.h>
#include <Dry/Graphics/GraphicsDefs.h>
#include <Dry/Graphics/GraphicsEvents.h>
#include <Dry/Graphics/IndexBuffer.h>
#include <Dry/Graphics/Light.h>
#include <Dry/Graphics/Material.h>
#include <Dry/Graphics/Model.h>
#include <Dry/Graphics/OcclusionBuffer.h>
#include <Dry/Graphics/Octree.h>
#include <Dry/Graphics/OctreeQuery.h>
#include <Dry/Graphics/ParticleEffect.h>
#include <Dry/Graphics/ParticleEmitter.h>
#include <Dry/Graphics/ReflectionProbe.h>
#include <Dry/Graphics/RenderPath.h>
#include <Dry/Graphics/RenderSurface.h>
#include <Dry/Graphics/Renderer.h>
#include <Dry/Graphics/RibbonTrail.h>
#include <Dry/Graphics/Shader.h>
#include <Dry/Graphics/ShaderPrecache.h>
#include <Dry/Graphics/ShaderProgram.h>
#include <Dry/Graphics/ShaderVariation.h>
#include <Dry/Graphics/Skeleton.h>
#include <Dry/Graphics/Skybox.h>
#include <Dry/Graphics/StaticModel.h>
#include <Dry/Graphics/StaticModelGroup.h>
#include <Dry/Graphics/Tangent.h>
#include <Dry/Graphics/Technique.h>
#include <Dry/Graphics/Terrain.h>
#include <Dry/Graphics/TerrainPatch.h>
#include <Dry/Graphics/Texture.h>
#include <Dry/Graphics/Texture2D.h>
#include <Dry/Graphics/Texture2DArray.h>
#include <Dry/Graphics/Texture3D.h>
#include <Dry/Graphics/TextureCube.h>
#include <Dry/Graphics/VertexBuffer.h>
#include <Dry/Graphics/VertexDeclaration.h>
#include <Dry/Graphics/View.h>
#include <Dry/Graphics/Viewport.h>
#include <Dry/Graphics/Zone.h>
#if DRY_IK
#include <Dry/IK/IK.h>
#include <Dry/IK/IKConstraint.h>
#include <Dry/IK/IKEffector.h>
#include <Dry/IK/IKEvents.h>
#include <Dry/IK/IKSolver.h>
#endif
#include <Dry/IO/AbstractFile.h>
#include <Dry/IO/Compression.h>
#include <Dry/IO/Deserializer.h>
#include <Dry/IO/File.h>
#include <Dry/IO/FileSystem.h>
#include <Dry/IO/FileWatcher.h>
#include <Dry/IO/IOEvents.h>
#include <Dry/IO/Log.h>
#include <Dry/IO/MacFileWatcher.h>
#include <Dry/IO/MemoryBuffer.h>
#include <Dry/IO/NamedPipe.h>
#include <Dry/IO/PackageFile.h>
#include <Dry/IO/RWOpsWrapper.h>
#include <Dry/IO/Serializer.h>
#include <Dry/IO/VectorBuffer.h>
#include <Dry/Input/Controls.h>
#include <Dry/Input/Input.h>
#include <Dry/Input/InputConstants.h>
#include <Dry/Input/InputEvents.h>
#include <Dry/LibraryInfo.h>
#include <Dry/Math/AreaAllocator.h>
#include <Dry/Math/BoundingBox.h>
#include <Dry/Math/Color.h>
#include <Dry/Math/Frustum.h>
#include <Dry/Math/MathDefs.h>
#include <Dry/Math/Matrix2.h>
#include <Dry/Math/Matrix3.h>
#include <Dry/Math/Matrix3x4.h>
#include <Dry/Math/Matrix4.h>
#include <Dry/Math/Plane.h>
#include <Dry/Math/Polyhedron.h>
#include <Dry/Math/Polynomial.h>
#include <Dry/Math/Quaternion.h>
#include <Dry/Math/Random.h>
#include <Dry/Math/Ray.h>
#include <Dry/Math/Rect.h>
#include <Dry/Math/Sphere.h>
#include <Dry/Math/StringHash.h>
#include <Dry/Math/Vector2.h>
#include <Dry/Math/Vector3.h>
#include <Dry/Math/Vector4.h>
#if DRY_NAVIGATION
#include <Dry/Navigation/CrowdAgent.h>
#include <Dry/Navigation/CrowdManager.h>
#include <Dry/Navigation/DynamicNavigationMesh.h>
#include <Dry/Navigation/NavArea.h>
#include <Dry/Navigation/NavBuildData.h>
#include <Dry/Navigation/Navigable.h>
#include <Dry/Navigation/NavigationEvents.h>
#include <Dry/Navigation/NavigationMesh.h>
#include <Dry/Navigation/Obstacle.h>
#include <Dry/Navigation/OffMeshConnection.h>
#endif
#if DRY_NETWORK
#include <Dry/Network/Connection.h>
#include <Dry/Network/HttpRequest.h>
#include <Dry/Network/Network.h>
#include <Dry/Network/NetworkEvents.h>
#include <Dry/Network/NetworkPriority.h>
#include <Dry/Network/Protocol.h>
#endif
#if DRY_PHYSICS
#include <Dry/Physics/CollisionShape.h>
#include <Dry/Physics/Constraint.h>
#include <Dry/Physics/KinematicCharacterController.h>
#include <Dry/Physics/PhysicsEvents.h>
#include <Dry/Physics/PhysicsUtils.h>
#include <Dry/Physics/PhysicsWorld.h>
#include <Dry/Physics/RaycastVehicle.h>
#include <Dry/Physics/RigidBody.h>
#endif
#include <Dry/Resource/BackgroundLoader.h>
#include <Dry/Resource/Decompress.h>
#include <Dry/Resource/Image.h>
#include <Dry/Resource/JSONFile.h>
#include <Dry/Resource/JSONValue.h>
#include <Dry/Resource/Localization.h>
#include <Dry/Resource/PListFile.h>
#include <Dry/Resource/Resource.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/Resource/ResourceEvents.h>
#include <Dry/Resource/XMLElement.h>
#include <Dry/Resource/XMLFile.h>
#include <Dry/Scene/Animatable.h>
#include <Dry/Scene/AnimationDefs.h>
#include <Dry/Scene/Component.h>
#include <Dry/Scene/LogicComponent.h>
#include <Dry/Scene/Node.h>
#include <Dry/Scene/ObjectAnimation.h>
#include <Dry/Scene/ReplicationState.h>
#include <Dry/Scene/Scene.h>
#include <Dry/Scene/SceneEvents.h>
#include <Dry/Scene/SceneResolver.h>
#include <Dry/Scene/Serializable.h>
#include <Dry/Scene/SmoothedTransform.h>
#include <Dry/Scene/SplinePath.h>
#include <Dry/Scene/UnknownComponent.h>
#include <Dry/Scene/ValueAnimation.h>
#include <Dry/Scene/ValueAnimationInfo.h>
#include <Dry/UI/BorderImage.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/CheckBox.h>
#include <Dry/UI/Cursor.h>
#include <Dry/UI/DropDownList.h>
#include <Dry/UI/FileSelector.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/FontFace.h>
#include <Dry/UI/FontFaceBitmap.h>
#include <Dry/UI/FontFaceFreeType.h>
#include <Dry/UI/LineEdit.h>
#include <Dry/UI/ListView.h>
#include <Dry/UI/Menu.h>
#include <Dry/UI/MessageBox.h>
#include <Dry/UI/ProgressBar.h>
#include <Dry/UI/ScrollBar.h>
#include <Dry/UI/ScrollView.h>
#include <Dry/UI/Slider.h>
#include <Dry/UI/Sprite.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/Text3D.h>
#include <Dry/UI/ToolTip.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIBatch.h>
#include <Dry/UI/UIComponent.h>
#include <Dry/UI/UIElement.h>
#include <Dry/UI/UIEvents.h>
#include <Dry/UI/UISelectable.h>
#include <Dry/UI/View3D.h>
#include <Dry/UI/Window.h>

#include <Dry/DebugNew.h>

using namespace Dry;

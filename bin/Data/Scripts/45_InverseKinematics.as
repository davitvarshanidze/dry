// Inverse Kinematics
// This sample demonstrates how to use the IK solver to create "grounders" for a walking character on a slope.

#include "Scripts/Utilities/Sample.as"

AnimationController@ ozomAnimCtrl_;
Node@ cameraRotateNode_;
Node@ floorNode_;
Node@ leftFoot_;
Node@ rightFoot_;
Node@ ozomNode_;
IKEffector@ leftEffector_;
IKEffector@ rightEffector_;
IKSolver@ solver_;
float floorPitch_ = 0.0f;
float floorRoll_ = 0.0f;
bool drawDebug_ = false;

void Start()
{
    cache.autoReloadResources = true;

    // Execute the common startup for samples
    SampleStart();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Drag LMB to look around\n"
                       "Drag RMB to change incline\n"
                       "Press Space to reset floor\n"
                       "D to toggle debug geometry");

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Set the mouse mode to use in the sample
    SampleInitMouseMode(MM_RELATIVE);

    // Hook up to the frame update events
    SubscribeToEvents();
}

void CreateScene()
{
    scene_ = Scene();

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    scene_.CreateComponent("Octree");
    scene_.CreateComponent("DebugRenderer");
    scene_.CreateComponent("PhysicsWorld");

    // Create scene node & StaticModel component for showing a static plane
    floorNode_ = scene_.CreateChild("Plane");
    floorNode_.scale = Vector3(50.0f, 1.0f, 50.0f);
    StaticModel@ planeObject = floorNode_.CreateComponent("StaticModel");
    planeObject.model = cache.GetResource("Model", "Models/Plane.mdl");
    planeObject.material = cache.GetResource("Material", "Materials/StoneTiled.xml");

    // Set up collision, we need to raycast to determine foot height
    floorNode_.CreateComponent("RigidBody");
    CollisionShape@ col = floorNode_.CreateComponent("CollisionShape");
    col.SetBox(Vector3(1, 0, 1));

    // Create a directional light to the world.
    Node@ lightNode = scene_.CreateChild("DirectionalLight");
    lightNode.direction = Vector3(0.6f, -1.0f, 0.8f); // The direction vector does not need to be normalized
    Light@ light = lightNode.CreateComponent("Light");
    light.lightType = LIGHT_DIRECTIONAL;
    light.castShadows = true;
    light.shadowBias = BiasParameters(0.00005f, 0.5f);
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light.shadowCascade = CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f);

    // Load Ozom animated model
    ozomNode_ = scene_.CreateChild("Ozom");
    ozomNode_.rotation = Quaternion(0.0f, 270.0f, 0.0f);
    ozomNode_.position = Vector3(0.0f, -0.1f, 0.0f);
    AnimatedModel@ ozom = ozomNode_.CreateComponent("AnimatedModel");
    ozom.model = cache.GetResource("Model", "Ghotiland/Models/Ozom.mdl");
    ozom.material = cache.GetResource("Material", "Ghotiland/Materials/Ozom.xml");
    ozom.castShadows = true;

    // Create animation controller and play walk animation
    ozomAnimCtrl_ = ozomNode_.CreateComponent("AnimationController");
    ozomAnimCtrl_.PlayExclusive("Ghotiland/Anim/Ozom/Walk.ani", 0, true, 0.0f);

    // We need to attach two inverse kinematic effectors to Ozom's feet to
    // control the grounding.
    leftFoot_  = ozomNode_.GetChild("Toes.L", true);
    rightFoot_ = ozomNode_.GetChild("Toes.R", true);
    leftEffector_  = leftFoot_.CreateComponent("IKEffector");
    rightEffector_ = rightFoot_.CreateComponent("IKEffector");
    // Control 2 segments up to the hips
    leftEffector_.chainLength = 3;
    rightEffector_.chainLength = 3;

    // For the effectors to work, an IKSolver needs to be attached to one of
    // the parent nodes. Typically, you want to place the solver as close as
    // possible to the effectors for optimal performance. Since in this case
    // we're solving the legs only, we can place the solver at the spine.
    Node@ spine = ozomNode_.GetChild("Hips", true);
    solver_ = spine.CreateComponent("IKSolver");

    // Two-bone solver is more efficient and more stable than FABRIK (but only
    // works for two bones, obviously).
    solver_.algorithm = IKAlgorithm::TWO_BONE;

    // Disable auto-solving, which means we can call Solve() manually.
    solver_.AUTO_SOLVE = false;

    // Only enable this so the debug draw shows us the pose before solving.
    // This should NOT be enabled for any other reason (it does nothing and is
    // a waste of performance).
    solver_.UPDATE_ORIGINAL_POSE = true;

    // Create the camera.
    cameraRotateNode_ = scene_.CreateChild("CameraRotate");
    cameraNode = cameraRotateNode_.CreateChild("Camera");
    cameraNode.CreateComponent("Camera");

    // Set an initial position for the camera scene node above the plane
    cameraNode.position = Vector3(0.0f, 0.0f, -4.0f);
    cameraRotateNode_.position = Vector3(0.0f, 0.4f, 0.0f);
    pitch = 20.0f;
    yaw = 50.0f;
}

void SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    Viewport@ viewport = Viewport(scene_, cameraNode.GetComponent("Camera"));
    renderer.viewports[0] = viewport;
}

void UpdateCameraAndFloor(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (ui.focusElement !is null)
        return;

    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    if (input.mouseButtonDown[MOUSEB_LEFT])
    {
        IntVector2 mouseMove = input.mouseMove;
        yaw += MOUSE_SENSITIVITY * mouseMove.x;
        pitch += MOUSE_SENSITIVITY * mouseMove.y;
        pitch = Clamp(pitch, -90.0f, 90.0f);
    }

    if (input.mouseButtonDown[MOUSEB_RIGHT])
    {
        IntVector2 mouseMoveInt = input.mouseMove;
        Vector2 mouseMove = Matrix2(
            -Cos(yaw), Sin(yaw),
            Sin(yaw),  Cos(yaw)
        ) * Vector2(mouseMoveInt.y, -mouseMoveInt.x);
        floorPitch_ += MOUSE_SENSITIVITY * mouseMove.x;
        floorPitch_ = Clamp(floorPitch_, -90.0f, 90.0f);
        floorRoll_ += MOUSE_SENSITIVITY * mouseMove.y;
    }

    if (input.keyPress[KEY_SPACE])
    {
        floorPitch_ = 0.0f;
        floorRoll_ = 0.0f;
    }

    if (input.keyPress[KEY_D])
    {
        drawDebug_ = !drawDebug_;
    }

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraRotateNode_.rotation = Quaternion(pitch, yaw, 0.0f);
    floorNode_.rotation = Quaternion(floorPitch_, 0.0f, floorRoll_);
}

void SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent("Update", "HandleUpdate");
    SubscribeToEvent("PostRenderUpdate", "HandlePostRenderUpdate");
    SubscribeToEvent("SceneDrawableUpdateFinished", "HandleSceneDrawableUpdateFinished");
}

void HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    float timeStep = eventData["TimeStep"].GetFloat();

    // Move the camera, scale movement with time step
    UpdateCameraAndFloor(timeStep);
}

void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (drawDebug_)
        solver_.DrawDebugGeometry(false);
}

void HandleSceneDrawableUpdateFinished(StringHash eventType, VariantMap& eventData)
{
    Vector3 leftFootPosition = leftFoot_.worldPosition;
    Vector3 rightFootPosition = rightFoot_.worldPosition;

    // Cast ray down to get the normal of the underlying surface
    PhysicsRaycastResult result = scene_.physicsWorld.RaycastSingle(Ray(leftFootPosition + Vector3(0, 1, 0), Vector3(0, -1, 0)), 2);
    if (result.body !is null)
    {
        // Cast again, but this time along the normal. Set the target position
        // to the ray intersection
        result = scene_.physicsWorld.RaycastSingle(Ray(leftFootPosition + result.normal, -result.normal), 2);
        // The foot node has an offset relative to the root node
        float footOffset = leftFoot_.worldPosition.y - ozomNode_.worldPosition.y;
        leftEffector_.targetPosition = result.position + result.normal * footOffset;
        // Rotate foot according to normal
        leftFoot_.Rotate(Quaternion(Vector3(0, 1, 0), result.normal), TS_WORLD);
    }

    // Same deal with the right foot
    result = scene_.physicsWorld.RaycastSingle(Ray(rightFootPosition + Vector3(0, 1, 0), Vector3(0, -1, 0)), 2);
    if (result.body !is null)
    {
        result = scene_.physicsWorld.RaycastSingle(Ray(rightFootPosition + result.normal, -result.normal), 2);
        float footOffset = rightFoot_.worldPosition.y - ozomNode_.worldPosition.y;
        rightEffector_.targetPosition = result.position + result.normal * footOffset;
        rightFoot_.Rotate(Quaternion(Vector3(0, 1, 0), result.normal), TS_WORLD);
    }

    solver_.Solve();
}

// Create XML patch instructions for screen joystick layout specific to this sample app
String patchInstructions = "";

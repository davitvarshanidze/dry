// Signed distance field text example.
// This sample demonstrates.
//     - Creating a 3D scene with static content
//     - Creating a 3D text use SDF Font
//     - Displaying the scene using the Renderer subsystem
//     - Handling keyboard and mouse input to move a freelook camera

#include "Scripts/Utilities/Sample.as"

void Start()
{
    // Execute the common startup for samples
    SampleStart();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions("Use WASDQE keys and mouse to move");

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

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_.CreateComponent("Octree");

    // Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
    // plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
    // (100 x 100 world units)
    Node@ planeNode = scene_.CreateChild("Plane");
    planeNode.scale = Vector3(100.0f, 1.0f, 100.0f);
    StaticModel@ planeObject = planeNode.CreateComponent("StaticModel");
    planeObject.model = cache.GetResource("Model", "Models/Plane.mdl");
    planeObject.material = cache.GetResource("Material", "Materials/StoneTiled.xml");

    // Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    // light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    // The light will use default settings (white light, no shadows)
    Node@ lightNode = scene_.CreateChild("DirectionalLight");
    lightNode.direction = Vector3(0.6f, -1.0f, 0.8f); // The direction vector does not need to be normalized
    Light@ light = lightNode.CreateComponent("Light");
    light.lightType = LIGHT_DIRECTIONAL;

    // Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
    // quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
    // LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
    // see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
    // same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
    // scene.
    const uint NUM_OBJECTS = 200;
    for (uint i = 0; i < NUM_OBJECTS; ++i)
    {
        Node@ mushroomNode = scene_.CreateChild("Mushroom");
        mushroomNode.position = Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f);
        mushroomNode.SetScale(0.5f + Random(2.0f));
        StaticModel@ mushroomObject = mushroomNode.CreateComponent("StaticModel");
        mushroomObject.model = cache.GetResource("Model", "Models/Mushroom.mdl");
        mushroomObject.material = cache.GetResource("Material", "Materials/Mushroom.xml");

        Node@ mushroomTitleNode = mushroomNode.CreateChild("MushroomTitle");
        mushroomTitleNode.position = Vector3(0.0f, 1.2f, 0.0f);
        Text3D@ mushroomTitleText = mushroomTitleNode.CreateComponent("Text3D");
        mushroomTitleText.text = "Mushroom " + i;

        mushroomTitleText.SetFont(cache.GetResource("Font", "Fonts/BlueHighway.sdf"), 24);
        mushroomTitleText.color = Color(1.0f, 0.0f, 0.0f);

        if (i % 3 == 1)
        {
            mushroomTitleText.color = Color(0.0f, 1.0f, 0.0f);
            mushroomTitleText.textEffect = TE_SHADOW;
            mushroomTitleText.effectColor = Color(0.5f, 0.5f, 0.5f);
        }
        else if (i % 3 == 2)
        {
            mushroomTitleText.color = Color(1.0f, 1.0f, 0.0f);
            mushroomTitleText.textEffect = TE_STROKE;
            mushroomTitleText.effectColor = Color(0.5f, 0.5f, 0.5f);
        }

        mushroomTitleText.SetAlignment(HA_CENTER, VA_CENTER);
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode = scene_.CreateChild("Camera");
    cameraNode.CreateComponent("Camera");

    // Set an initial position for the camera scene node above the plane
    cameraNode.position = Vector3(0.0f, 5.0f, 0.0f);
}


void SetupViewport()
{
    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    Viewport@ viewport = Viewport(scene_, cameraNode.GetComponent("Camera"));
    renderer.viewports[0] = viewport;
}

void MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (ui.focusElement !is null)
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    const IntVector2 mouseMove = input.mouseMove;
    yaw += MOUSE_SENSITIVITY * mouseMove.x;
    pitch += MOUSE_SENSITIVITY * mouseMove.y;
    pitch = Clamp(pitch, -89.0f, 89.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode.rotation = Quaternion(pitch, yaw, 0.0f);

    // Read WASDQE keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the Translate() function (default local space) to move relative to the node's orientation.
    if (input.keyDown[KEY_W])
        cameraNode.Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input.keyDown[KEY_S])
        cameraNode.Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input.keyDown[KEY_A])
        cameraNode.Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input.keyDown[KEY_D])
        cameraNode.Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
    if (input.keyDown[KEY_E])
        cameraNode.Translate(Vector3::UP * MOVE_SPEED * timeStep, TS_WORLD);
    if (input.keyDown[KEY_Q])
        cameraNode.Translate(Vector3::DOWN * MOVE_SPEED * timeStep, TS_WORLD);
}

void SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent("Update", "HandleUpdate");
}

void HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // Take the frame time step, which is stored as a float
    float timeStep = eventData["TimeStep"].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

// Create XML patch instructions for screen joystick layout specific to this sample app
String patchInstructions = "";

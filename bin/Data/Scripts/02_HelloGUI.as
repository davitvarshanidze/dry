// A simple 'HelloWorld' GUI created purely from code.
// This sample demonstrates:
//     - Creation of controls and building a UI hierarchy
//     - Loading UI style from XML and applying it to controls
//     - Handling of global and per-control events
// For more advanced users (beginners can skip this section):
//     - Dragging UIElements
//     - Displaying tooltips
//     - Accessing available Events data (eventData)

#include "Scripts/Utilities/Sample.as"

Window@ window;
IntVector2 dragBeginPosition = IntVector2::ZERO;

void Start()
{
    // Execute the common startup for samples
    SampleStart();

    // Enable OS cursor
    input.mouseVisible = true;

    // Load XML file containing default UI style sheet
    XMLFile@ style = cache.GetResource("XMLFile", "UI/DefaultStyle.xml");

    // Set the loaded style as default style
    ui.root.defaultStyle = style;

    // Initialize Window
    InitWindow();

    // Create and add some controls to the Window
    InitControls();

    // Create a draggable Emblem
    CreateDraggableEmblem();

    // Set the mouse mode to use in the sample
    SampleInitMouseMode(MM_FREE);
}

void InitControls()
{
    // Create a CheckBox
    CheckBox@ checkBox = CheckBox();
    checkBox.name = "CheckBox";

    // Create a Button
    Button@ button = Button();
    button.name = "Button";
    button.minHeight = 24;

    // Create a LineEdit
    LineEdit@ lineEdit = LineEdit();
    lineEdit.name = "LineEdit";
    lineEdit.minHeight = 24;

    // Add controls to Window
    window.AddChild(checkBox);
    window.AddChild(button);
    window.AddChild(lineEdit);

    // Apply previously set default style
    checkBox.SetStyleAuto();
    button.SetStyleAuto();
    lineEdit.SetStyleAuto();
}

void InitWindow()
{
    // Create the Window and add it to the UI's root node
    window = Window();
    ui.root.AddChild(window);

    // Set Window size and layout settings
    window.minWidth = 384;
    window.SetLayout(LM_VERTICAL, 6, IntRect(6, 6, 6, 6));
    window.SetAlignment(HA_CENTER, VA_CENTER);
    window.name = "Window";

    // Create Window 'titlebar' container
    UIElement@ titleBar = UIElement();
    titleBar.SetMinSize(0, 24);
    titleBar.verticalAlignment = VA_TOP;
    titleBar.layoutMode = LM_HORIZONTAL;

    // Create the Window title Text
    Text@ windowTitle = Text();
    windowTitle.name = "WindowTitle";
    windowTitle.text = "Hello GUI!";

    // Create the Window's close button
    Button@ buttonClose = Button();
    buttonClose.name = "CloseButton";

    // Add the controls to the title bar
    titleBar.AddChild(windowTitle);
    titleBar.AddChild(buttonClose);

    // Add the title bar to the Window
    window.AddChild(titleBar);

    // Apply styles
    window.SetStyleAuto();
    windowTitle.SetStyleAuto();
    buttonClose.style = "CloseButton";

    // Subscribe to buttonClose release (following a 'press') events
    SubscribeToEvent(buttonClose, "Released", "HandleClosePressed");

    // Subscribe also to all UI mouse clicks just to see where we have clicked
    SubscribeToEvent("UIMouseClick", "HandleControlClicked");
}

void CreateDraggableEmblem()
{
    // Create a draggable Emblem button
    Button@ draggableEmblem = ui.root.CreateChild("Button", "Emblem");
    draggableEmblem.texture = cache.GetResource("Texture2D", "Textures/DryDecal.png"); // Set texture
    draggableEmblem.blendMode = BLEND_ADD;
    draggableEmblem.SetSize(128, 128);
    draggableEmblem.SetPosition((graphics.width - draggableEmblem.width) / 2, 200);

    // Add a tooltip to Emblem button
    ToolTip@ toolTip = draggableEmblem.CreateChild("ToolTip");
    toolTip.position = IntVector2(draggableEmblem.width + 5, draggableEmblem.width/2); // slightly offset from emblem
    BorderImage@ textHolder = toolTip.CreateChild("BorderImage");
    textHolder.SetStyle("ToolTipBorderImage");
    Text@ toolTipText = textHolder.CreateChild("Text");
    toolTipText.SetStyle("ToolTipText");
    toolTipText.text = "Please drag me!";

    // Subscribe draggableEmblem to Drag Events (in order to make it draggable)
    // See "Event list" in documentation's Main Page for reference on available Events and their eventData
    SubscribeToEvent(draggableEmblem, "DragBegin", "HandleDragBegin");
    SubscribeToEvent(draggableEmblem, "DragMove", "HandleDragMove");
    SubscribeToEvent(draggableEmblem, "DragEnd", "HandleDragEnd");
}

void HandleDragBegin(StringHash eventType, VariantMap& eventData)
{
    // Get UIElement relative position where input (touch or click) occurred (top-left = IntVector2(0,0))
    dragBeginPosition = IntVector2(eventData["ElementX"].GetInt(), eventData["ElementY"].GetInt());
}

void HandleDragMove(StringHash eventType, VariantMap& eventData)
{
    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetInt(), eventData["Y"].GetInt());
    // Get the element (emblem) that is being dragged. GetPtr() returns a RefCounted handle which can be cast implicitly
    UIElement@ draggedElement = eventData["Element"].GetPtr();
    draggedElement.position = dragCurrentPosition - dragBeginPosition;
}

void HandleDragEnd(StringHash eventType, VariantMap& eventData) // For reference (not used here)
{
}

void HandleClosePressed(StringHash eventType, VariantMap& eventData)
{
    engine.Exit();
}

void HandleControlClicked(StringHash eventType, VariantMap& eventData)
{
    // Get the Text control acting as the Window's title
    Text@ windowTitle = window.GetChild("WindowTitle", true);

    // Get control that was clicked
    UIElement@ clicked = eventData["Element"].GetPtr();

    String name = "...?";
    if (clicked !is null)
    {
        // Get the name of the control that was clicked
        name = clicked.name;
    }

    // Update the Window's title text
    windowTitle.text = "Hello " + name + "!";
}

// Create XML patch instructions for screen joystick layout specific to this sample app
String patchInstructions =
        "<patch>" +
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Hat0']]\">" +
        "        <attribute name=\"Is Visible\" value=\"false\" />" +
        "    </add>" +
        "</patch>";

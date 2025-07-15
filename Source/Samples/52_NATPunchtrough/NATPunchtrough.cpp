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
#include <Dry/Audio/Sound.h>
#include <Dry/Engine/Engine.h>
#include <Dry/Graphics/Zone.h>
#include <Dry/Input/Input.h>
#include <Dry/IO/Log.h>
#include <Dry/IO/MemoryBuffer.h>
#include <Dry/IO/VectorBuffer.h>
#include <Dry/Network/Network.h>
#include <Dry/Network/NetworkEvents.h>
#include <Dry/Resource/ResourceCache.h>
#include <Dry/UI/Button.h>
#include <Dry/UI/Font.h>
#include <Dry/UI/LineEdit.h>
#include <Dry/UI/Text.h>
#include <Dry/UI/UI.h>
#include <Dry/UI/UIEvents.h>

#include <SLikeNet/types.h>

#include "NATPunchtrough.h"

// Undefine Windows macro, as our Connection class has a function called SendMessage
#ifdef SendMessage
#undef SendMessage
#endif

DRY_DEFINE_APPLICATION_MAIN(NATPunchtrough)

NATPunchtrough::NATPunchtrough(Context* context): Sample(context)
{
}

void NATPunchtrough::Start()
{
    // Execute base class startup
    Sample::Start();

    // Enable OS cursor
    GetSubsystem<Input>()->SetMouseVisible(true);

    // Create the user interface
    CreateUI();

    // Subscribe to UI and network events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void NATPunchtrough::CreateUI()
{
    SetLogoVisible(true); // We need the full rendering window

    UIElement* root{ GetSubsystem<UI>()->GetRoot() };
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    XMLFile* uiStyle{ cache->GetResource<XMLFile>("UI/DefaultStyle.xml") };
    // Set style to the UI root so that elements will inherit it
    root->SetDefaultStyle(uiStyle);

    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };
    logHistoryText_ = root->CreateChild<Text>();
    logHistoryText_->SetFont(font, 12);
    logHistoryText_->SetPosition(20, -20);
    logHistoryText_->SetVerticalAlignment(VA_BOTTOM);
    logHistory_.Resize(20);

    // Create NAT server config fields
    int marginTop{ 40 };
    CreateLabel("1. Run NAT server somewhere, enter NAT server info and press 'Save NAT settings'", IntVector2{ 20, marginTop - 20 });
    const StringVector defaultNatServerInfo{ String{ GetSubsystem<Network>()->GetNATServerInfo()->ToString() }.Split('|') };
    natServerAddress_ = CreateLineEdit(defaultNatServerInfo.Front(), 200, IntVector2{ 20, marginTop });
    natServerPort_ = CreateLineEdit(defaultNatServerInfo.Back(), 100, IntVector2{ 240, marginTop });
    saveNatSettingsButton_ = CreateButton("Save NAT settings", 160, IntVector2{ 360, marginTop });

    // Create server start button
    marginTop = 120;
    CreateLabel("2. Create server and give others your server GUID", IntVector2{ 20, marginTop - 20 });
    guid_ = CreateLineEdit("Your server GUID", 200, IntVector2{ 20, marginTop });
    startServerButton_ = CreateButton("Start server", 160, IntVector2{ 240, marginTop });

    // Create client connection related fields
    marginTop = 200;
    CreateLabel("3. Input local or remote server GUID", IntVector2{ 20, marginTop - 20 });
    serverGuid_ = CreateLineEdit("Remote server GUID", 200, IntVector2(20, marginTop));
    connectButton_ = CreateButton("Connect", 160, IntVector2(240, marginTop));

    // No viewports or scene is defined. However, the default zone's fog color controls the fill color
    GetSubsystem<Renderer>()->GetDefaultZone()->SetFogColor({ 0.0f, 0.0f, 0.1f });
}

void NATPunchtrough::SubscribeToEvents()
{
    SubscribeToEvent(E_SERVERCONNECTED, DRY_HANDLER(NATPunchtrough, HandleServerConnected));
    SubscribeToEvent(E_SERVERDISCONNECTED, DRY_HANDLER(NATPunchtrough, HandleServerDisconnected));
    SubscribeToEvent(E_CONNECTFAILED, DRY_HANDLER(NATPunchtrough, HandleConnectFailed));

    // NAT server connection related events
    SubscribeToEvent(E_NATMASTERCONNECTIONFAILED, DRY_HANDLER(NATPunchtrough, HandleNatConnectionFailed));
    SubscribeToEvent(E_NATMASTERCONNECTIONSUCCEEDED, DRY_HANDLER(NATPunchtrough, HandleNatConnectionSucceeded));
    SubscribeToEvent(E_NATMASTERDISCONNECTED, DRY_HANDLER(NATPunchtrough, HandleNatDisconnected));

    // NAT punchtrough request events
    SubscribeToEvent(E_NETWORKNATPUNCHTROUGHSUCCEEDED, DRY_HANDLER(NATPunchtrough, HandleNatPunchtroughSucceeded));
    SubscribeToEvent(E_NETWORKNATPUNCHTROUGHFAILED, DRY_HANDLER(NATPunchtrough, HandleNatPunchtroughFailed));

    SubscribeToEvent(E_CLIENTCONNECTED, DRY_HANDLER(NATPunchtrough, HandleClientConnected));
    SubscribeToEvent(E_CLIENTDISCONNECTED, DRY_HANDLER(NATPunchtrough, HandleClientDisconnected));

    SubscribeToEvent(saveNatSettingsButton_, E_RELEASED, DRY_HANDLER(NATPunchtrough, HandleSaveNatSettings));
    SubscribeToEvent(startServerButton_, E_RELEASED, DRY_HANDLER(NATPunchtrough, HandleStartServer));
    SubscribeToEvent(connectButton_, E_RELEASED, DRY_HANDLER(NATPunchtrough, HandleConnect));
}

Button* NATPunchtrough::CreateButton(const String& text, int width, IntVector2 position)
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    Font* font{ cache->GetResource<Font>("Fonts/Philosopher.ttf") };

    Button* button{ GetSubsystem<UI>()->GetRoot()->CreateChild<Button>() };
    button->SetStyleAuto();
    button->SetFixedWidth(width);
    button->SetFixedHeight(30);
    button->SetPosition(position);

    Text* buttonText{ button->CreateChild<Text>() };
    buttonText->SetFont(font, 12);
    buttonText->SetAlignment(HA_CENTER, VA_CENTER);
    buttonText->SetText(text);

    return button;
}

LineEdit* NATPunchtrough::CreateLineEdit(const String& placeholder, int width, IntVector2 pos)
{
    LineEdit* textEdit{ GetSubsystem<UI>()->GetRoot()->CreateChild<LineEdit>("") };
    textEdit->SetStyleAuto();
    textEdit->SetFixedWidth(width);
    textEdit->SetFixedHeight(30);
    textEdit->SetText(placeholder);
    textEdit->SetPosition(pos);

    return textEdit;
}

void NATPunchtrough::CreateLabel(const String& text, IntVector2 pos)
{
    ResourceCache* cache{ GetSubsystem<ResourceCache>() };
    // Create log element to view latest logs from the system
    Font* font{ cache->GetResource<Font>("Fonts/Days.ttf") };
    Text* label{ GetSubsystem<UI>()->GetRoot()->CreateChild<Text>() };
    label->SetFont(font, 12);
    label->SetColor({ 0.0f, 1.0f, 0.0f });
    label->SetPosition(pos);
    label->SetText(text);
}

void NATPunchtrough::ShowLogMessage(const String& row)
{
    logHistory_.Erase(0);
    logHistory_.Push(row);

    // Concatenate all the rows in history
    String allRows;

    for (unsigned i{ 0 }; i < logHistory_.Size(); ++i)
        allRows += logHistory_[i] + "\n";

    logHistoryText_->SetText(allRows);
}

void NATPunchtrough::HandleSaveNatSettings(StringHash eventType, VariantMap& eventData)
{
    // Save NAT server configuration
    GetSubsystem<Network>()->SetNATServerInfo(natServerAddress_->GetText(), ToInt(natServerPort_->GetText()));
    ShowLogMessage("Saving NAT settings: " + natServerAddress_->GetText() + ":" + natServerPort_->GetText());
}

void NATPunchtrough::HandleServerConnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Server connected!");
}

void NATPunchtrough::HandleServerDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Server disconnected!");
}

void NATPunchtrough::HandleConnectFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Client: Connection failed!");
}

void NATPunchtrough::HandleNatDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Disconnected from NAT master server");
}

void NATPunchtrough::HandleStartServer(StringHash eventType, VariantMap& eventData)
{
    GetSubsystem<Network>()->StartServer(SERVER_PORT);
    ShowLogMessage("Server: Server started on port: " + String(SERVER_PORT));

    // Connect to the NAT server
    GetSubsystem<Network>()->StartNATClient();
    ShowLogMessage("Server: Starting NAT client for server...");

    // Output our assigned GUID which others will use to connect to our server
    guid_->SetText(GetSubsystem<Network>()->GetGUID());
}

void NATPunchtrough::HandleConnect(StringHash eventType, VariantMap& eventData)
{
    VariantMap userData;
    userData["Name"] = "Dry";

    // Attempt connecting to server using custom GUID, Scene = null as a second parameter and user identity is passed as third parameter
    GetSubsystem<Network>()->AttemptNATPunchtrough(serverGuid_->GetText(), nullptr, userData);
    ShowLogMessage("Client: Attempting NAT punchtrough to guid: " + serverGuid_->GetText());
}

void NATPunchtrough::HandleNatConnectionFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Connection to NAT master server failed!");
}

void NATPunchtrough::HandleNatConnectionSucceeded(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Connection to NAT master server succeeded!");
}

void NATPunchtrough::HandleNatPunchtroughSucceeded(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("NAT punchtrough succeeded!");
}

void NATPunchtrough::HandleNatPunchtroughFailed(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("NAT punchtrough failed!");
}

void NATPunchtrough::HandleClientConnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Server: Client connected!");
}

void NATPunchtrough::HandleClientDisconnected(StringHash eventType, VariantMap& eventData)
{
    ShowLogMessage("Server: Client disconnected!");
}

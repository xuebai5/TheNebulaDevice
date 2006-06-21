//------------------------------------------------------------------------------
//  application/guistatehandler.cc
//  (C) 2005 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "application/guistatehandler.h"
#include "foundation/factory.h"
#include "application/guieventhandler.h"
#include "ui/server.h"
#include "gui/nguiserver.h"
#include "video/nvideoserver.h"
#include "particle/nparticleserver.h"
#include "particle/nparticleserver2.h"
#include "vfx/server.h"
#include "managers/setupmanager.h"

namespace Application
{
ImplementRtti(Application::GuiStateHandler, Application::StateHandler);
ImplementFactory(Application::GuiStateHandler);

//------------------------------------------------------------------------------
/**
*/
GuiStateHandler::GuiStateHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
GuiStateHandler::~GuiStateHandler()
{
    n_assert(!this->eventHandler.isvalid());
}

//------------------------------------------------------------------------------
/**
    Handle an UI event generated by our user interface. Override this method
    in a subclass.
*/
void
GuiStateHandler::OnEvent(UI::Event* event)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Called when this application state is activated. This will create
    and activate our GUI and GUI event handler.
*/
void
GuiStateHandler::OnStateEnter(const nString& prevState)
{
    Managers::SetupManager::Instance()->SetupEmptyWorld();
    this->eventHandler = GuiEventHandler::Create();
    this->eventHandler->SetGuiStateHandler(this);
    UI::Server::Instance()->DisplayGui(this->resName, this->eventHandler);
#ifdef MANGALORE_USE_CEGUI
    CEUI::Server::Instance()->DisplayGui(this->resName);
#endif
}

//------------------------------------------------------------------------------
/**
    Called when this application state is deactivated. This will destroy
    our GUI and GUI event handler.
*/
void
GuiStateHandler::OnStateLeave(const nString& nextState)
{
    UI::Server::Instance()->HideGui();
#ifdef MANGALORE_USE_CEGUI
    CEUI::Server::Instance()->HideGui();
#endif
    this->eventHandler = 0;
    Managers::SetupManager::Instance()->CleanupWorld();
}

//------------------------------------------------------------------------------
/**
    The per-frame handler method.
*/
nString
GuiStateHandler::OnFrame()
{
    nScriptServer* scriptServer = Foundation::Server::Instance()->GetScriptServer();
    bool running = true;

    // distribute timestamps to interested subsystems
    nTime time = App::Instance()->GetTime();
    nTime frameTime = App::Instance()->GetFrameTime();
    Input::Server::Instance()->SetTime(time);
    VFX::Server::Instance()->SetTime(time);
    Audio::Server::Instance()->SetTime(time);
    UI::Server::Instance()->SetTime(time);
    UI::Server::Instance()->SetFrameTime(frameTime);
#ifdef MANGALORE_USE_CEGUI
    CEUI::Server::Instance()->SetTime(time);
    CEUI::Server::Instance()->SetFrameTime(frameTime);
#endif
    nGuiServer::Instance()->SetTime(time);

    // trigger subsystem and Nebula servers
    nVideoServer::Instance()->Trigger();
    Input::Server::Instance()->Trigger();
    running &= scriptServer->Trigger();

    // trigger the audio and game subsystems
    Audio::Server::Instance()->BeginScene();
    VFX::Server::Instance()->BeginScene();
    nParticleServer::Instance()->Trigger();
    nParticleServer2::Instance()->Trigger();

    running &= Graphics::Server::Instance()->Trigger();
    if (Graphics::Server::Instance()->BeginRender())
    {
        UI::Server::Instance()->Render();
        Graphics::Server::Instance()->Render();
#ifdef MANGALORE_USE_CEGUI
        CEUI::Server::Instance()->Render();
#endif
        Graphics::Server::Instance()->EndRender();
    }
    VFX::Server::Instance()->EndScene();
    Audio::Server::Instance()->EndScene();

    // trigger kernel server
    nKernelServer::Instance()->Trigger();

    if (running)
    {
        return this->GetName();
    }
    else
    {
        return "Exit";
    }
}

} // namespace Application

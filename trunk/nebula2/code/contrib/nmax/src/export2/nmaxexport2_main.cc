 //-----------------------------------------------------------------------------
//  nmaxexport2_main.cc
//
//  (c)2004 Kim, Hyoun Woo
//-----------------------------------------------------------------------------
#include "export2/nmax.h"
#include "base/nmaxlistener.h"
#include "export2/nmaxexport2.h"
#include "tools/nmeshbuilder.h"
#include "export2/nmaxscene.h"
#include "export2/nmaxoptions.h"
#include "pluginlibs/nmaxdlg.h"
#include "pluginlibs/nmaxlogdlg.h"
#include "pluginlibs/nmaxvieweroptions.h"

#include "tools/napplauncher.h"
#include "tools/nwinmaincmdlineargs.h"

//-----------------------------------------------------------------------------
/**
*/
nMaxExport2::nMaxExport2()
{
}

//-----------------------------------------------------------------------------
/**
*/
nMaxExport2::~nMaxExport2()
{
}

//-----------------------------------------------------------------------------
/**
    Export a scene.
*/
int	nMaxExport2::DoExport(const TCHAR *name, 
    ExpInterface *ei, 
    Interface *inf, 
    BOOL suppressPrompts, 
    DWORD options)
{
    // it exports root node in the scene.
    return ExportScene(name, inf, 0, 0/*no use preview*/);
}

//-----------------------------------------------------------------------------
/**
    Launch preview application.  
*/
static
bool LaunchViewer(const char* sceneFile)
{
    // read viewer options.
    nMaxViewerOptions viewerOptions;

    if (nMaxOptions::Instance()->UseDefaultViewer())
        viewerOptions.SetViewerType(nMaxViewerOptions::ViewerType::Default);
    else
        viewerOptions.SetViewerType(nMaxViewerOptions::ViewerType::Custom);

    viewerOptions.SetSceneFileName(sceneFile);

    if (!viewerOptions.Read())
    {
        n_message("Failed to read viewer option.");
        return false;
    }

    nString appArgs = viewerOptions.GetArguments();
    
    //nString appArgs;

    //appArgs += "-projdir ";
    //appArgs += nMaxOptions::Instance()->GetHomePath();
    //appArgs += " ";
    //appArgs += "-view ";
    //appArgs += >
    //appArgs += sceneFile;
    //appArgs += " ";

    nAppLauncher appLauncher(nKernelServer::Instance());

    //appLauncher.SetExecutable("nviewer.exe");
    appLauncher.SetExecutable(viewerOptions.GetExecutable());

    nString cwd; 
    cwd += nMaxOptions::Instance()->GetHomePath(); 
#ifdef _DEBUG
    cwd += "\\bin\\win32d";
#else
    cwd += "\\bin\\win32";
#endif

    appLauncher.SetWorkingDirectory(cwd.Get());
    appLauncher.SetArguments(appArgs.Get());

    return appLauncher.Launch();
}

//-----------------------------------------------------------------------------
/**
    Delete any exist singleton class instances.

    - 20-Feb-05 kims Removed release of nMaxOption instance. The option can be
                     globally available not just only for export.
*/
static
void ReleaseSingletons()
{
    nMaxInterface* intf    = nMaxInterface::Instance();
    nMaxLogDlg*    logDlg  = nMaxLogDlg::Instance();

    n_delete(intf);

    // idle until user click 'ok' button of log dialog.
    logDlg->Wait();
    n_delete(logDlg);
}

//-----------------------------------------------------------------------------
/**
    Export the given scene or node.

    @param name filename to save a exported scene(or node).
    @param inf  pointer to Interface class instance.
    @param node INode which to export. If Null is given, it exports scene root node.

    @return Return 0, if it failed to export.
*/
int ExportScene(const TCHAR* name, Interface* inf, INode* inode, int previewMode)
{
    n_assert(inf);

    //init the maxLogHandler to redirect error message of nebula to 3dsmax.
    nLogHandler* logHandler = nKernelServer::Instance()->GetLogHandler();
    static_cast<nMaxLogHandler*>(logHandler)->SetLogSys(inf->Log());

    nMaxLogDlg* logDlg = nMaxLogDlg::Instance();
    logDlg->Create();

    // create max interface.
    nMaxInterface* intf = nMaxInterface::Instance();

    if (!intf->Create(inf))
    {
        n_maxlog(Error, "Failed to create nMaxInterface class instance.");

        ReleaseSingletons();

        return 0;
    }

    //// read options from .ini file.
    nMaxOptions* expOptions = nMaxOptions::Instance();
    if (!expOptions->Initialize())
    {
        n_maxlog(Error, "Faile to read '%s' file from 'plugcfg' directory.", 
            N_MAXEXPORT_INIFILE);

        ReleaseSingletons();

        return 0;
    }
    expOptions->SetSaveFileName(nString(name));

    // specifies preview mode.
    if (-1 == previewMode )
        expOptions->SetPreviewMode(true);
    else
        expOptions->SetPreviewMode(false);

    // export scene.
    nMaxScene scene;
    if (!scene.Export(inode))
    {
        n_maxlog(Error, "Failed to export scene.");

        ReleaseSingletons();

        return 0;
    }

    // run nmaxpreviewer.
    if (expOptions->RunViewer())
    {
        nString saveFile = name;
        nString sceneFile = saveFile.ExtractFileName();

        if (LaunchViewer(sceneFile.Get()))
        {
            n_maxlog(Medium, "Launched preview application.");
        }
        else
        {
            n_maxlog(Medium, "Failed to launch preview application.");
        }
    }

    ReleaseSingletons();

    return 1;
}

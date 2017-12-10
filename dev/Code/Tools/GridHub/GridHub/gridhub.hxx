/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#ifndef GRIDHUB_H
#define GRIDHUB_H

#include <QWidget>
#include <QSystemTrayIcon>

#include <GridHub/ui_GridHub.h>

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/debug/TraceMessageBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    class ComponentApplication;
};

class GridHubComponent;

class GridHub
    : public QWidget
    , public AZ::Debug::TraceMessageBus::Handler
{
    Q_OBJECT

public:
    GridHub(AZ::ComponentApplication* componentApp, GridHubComponent* hubComponent, QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~GridHub();

public slots:
    void OnStartStopSession();	
    void SetSessionPort(int port);
    void SetSessionSlots(int numberOfSlots);
    void SetHubName(const QString & name);
    void EnableDisconnectDetection(int state);
    void AddToStartupFolder(int state);
    void LogToFile(int state);
    void OnDisconnectTimeOutChange(int value);    

protected:
    void SanityCheckDetectionTimeout();

    void timerEvent(QTimerEvent *event);
    void closeEvent(QCloseEvent *event);
    void SystemTick();

private:
    //////////////////////////////////////////////////////////////////////////
    // TraceMessagesBus
    virtual bool	OnOutput(const char* window, const char* message);
    //////////////////////////////////////////////////////////////////////////

    void UpdateOutput();
    void UpdateMembers();


    QSystemTrayIcon*	m_trayIcon;
    QMenu*				m_trayIconMenu;

    //QAction *			m_minimizeAction;
    //QAction *			m_maximizeAction;
    QAction *			m_restoreAction;
    QAction *			m_quitAction;
    AZ::ComponentApplication*	m_componentApp;
    GridHubComponent*	m_hubComponent;

    AZStd::string		m_output;
    AZStd::mutex		m_outputMutex;

    Ui::GridHubClass ui;
};

//////////////////////////////////////////////////////////////////////////
// TODO MOVE TO A SEPARATE FILE
#include <AzCore/Component/Component.h>
#include <GridMate/Session/Session.h>

class GridHubComponent
    : public AZ::Component
    , public AZ::SystemTickBus::Handler
    , public GridMate::SessionEventBus::Handler
    , public AZ::Debug::TraceMessageBus::Handler
{
    friend class GridHubComponentFactory;
public:
    AZ_COMPONENT(GridHubComponent, "{11E4BB35-F135-4720-A890-979195A6B74E}");

    GridHubComponent();
    virtual ~GridHubComponent();

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    virtual void Init();
    virtual void Activate();
    virtual void Deactivate();
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AZ::SystemTickBus
    void OnSystemTick() override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // TraceMessagesBus
    virtual bool	OnOutput(const char* window, const char* message);
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Session Events
    /// Callback that is called when the Session service is ready to process sessions.
    virtual void OnSessionServiceReady()															{}
    /// Callback that notifies the title when a game search query have completed.  
    virtual void OnGridSearchComplete(GridMate::GridSearch* gridSearch)									{ (void)gridSearch; }
    /// Callback that notifies the title when a new member joins the game session.  
    virtual void OnMemberJoined(GridMate::GridSession* session, GridMate::GridMember* member);
    /// Callback that notifies the title that a member is leaving the game session. member pointer is NOT valid after the callback returns.
    virtual void OnMemberLeaving(GridMate::GridSession* session, GridMate::GridMember* member);
    // \todo a better way will be (after we solve migration) is to supply a reason to OnMemberLeaving... like the member was kicked. 
    // this will require that we actually remove the replica at the same moment.
    /// Callback that host decided to kick a member. You will receive a OnMemberLeaving when the actual member leaves the session.
    virtual void OnMemberKicked(GridMate::GridSession* session, GridMate::GridMember* member)			{ (void)session;(void)member; }	
    /// After this callback it is safe to access session features. If host session is fully operational if client wait for OnSessionJoined.
    virtual void OnSessionCreated(GridMate::GridSession* session);
    /// Called on client machines to indicate that we join successfully.
    virtual void OnSessionJoined(GridMate::GridSession* session)										{ (void)session; }
    /// Callback that notifies the title when a session will be left. session pointer is NOT valid after the callback returns.
    virtual void OnSessionDelete(GridMate::GridSession* session);
    /// Called when a session error occurs.
    virtual void OnSessionError(GridMate::GridSession* session, const GridMate::string& errorMsg )		{ (void)session; (void)errorMsg; }
    /// Called when the actual game(match) starts
    virtual void OnSessionStart(GridMate::GridSession* session)											{ (void)session; }
    /// Called when the actual game(match) ends
    virtual void OnSessionEnd(GridMate::GridSession* session)											{ (void)session; }
    /// Called when we start a host migration. 
    virtual void OnMigrationStart(GridMate::GridSession* session)										{ (void)session; }
    /// Called so the user can select a member that should be the new Host. Value will be ignored if NULL, current host or the member has invalid connection id.
    virtual void OnMigrationElectHost(GridMate::GridSession* session,GridMate::GridMember*& newHost)	{ (void)session;(void)newHost; }
    /// Called when the host migration has completed.
    virtual void OnMigrationEnd(GridMate::GridSession* session,GridMate::GridMember* newHost)			{ (void)session;(void)newHost; }
    //////////////////////////////////////////////////////////////////////////

    void SetUI(GridHub* ui)	{ m_ui = ui; }

    bool StartSession(bool isRestarting = false);
    void StopSession(bool isRestarting = false);
    void RestartSession();
    bool IsInSession() const						{ return m_session != nullptr; } 
    GridMate::GridSession*	GetSession()			{ return m_session; }
    void SetSessionPort(unsigned short port)		{ m_sessionPort = port; }
    int  GetSessionPort() const						{ return m_sessionPort; }
    void SetSessionSlots(unsigned char numSlots)	{ m_numberOfSlots = numSlots; }
    int GetSessionSlots() const						{ return m_numberOfSlots; }
    void SetHubName(AZStd::string& hubName)			{ m_hubName = hubName; }
    void EnableDisconnectDetection(bool en);
    bool IsEnableDisconnectDetection() const		{ return m_isDisconnectDetection; }
    void SetDisconnectionTimeOut(int timeout)       { m_disconnectionTimeout = timeout; }
    int GetDisconnectionTimeout() const             { return m_disconnectionTimeout; }
    void AddToStartupFolder(bool isAdd)				{ m_isAddToStartupFolder = isAdd; }
    bool IsAddToStartupFolder() const				{ return m_isAddToStartupFolder; }
    const AZStd::string& GetHubName() const			{ return m_hubName; }		

    void LogToFile(bool enable);
    bool IsLogToFile() const						{ return m_isLogToFile; }

    

    static void Reflect(AZ::ReflectContext* reflection);
private:    

    GridHub*				m_ui;
    GridMate::IGridMate*	m_gridMate;
    GridMate::GridSession*  m_session;

    unsigned short			m_sessionPort;
    unsigned char			m_numberOfSlots;
    AZStd::string			m_hubName;
    bool					m_isDisconnectDetection;
    int                     m_disconnectionTimeout;
    bool					m_isAddToStartupFolder;
    bool					m_isLogToFile;

    AZ::IO::SystemFile		m_logFile;
    
    /**
     * Contain information about members and titles that we monitor for exit. Only enabled if 
     * we have disconnection detection off.
     */	
    struct ExternalProcessMonitor
    {
        ExternalProcessMonitor()
            : m_memberId(0)
            , m_localProcess(INVALID_HANDLE_VALUE)
        {
        }

        GridMate::MemberIDCompact	m_memberId;			///< Pointer to member ID in the session.
        HANDLE						m_localProcess;		///< Local process ID used to monitor local applocations.

    };

    AZStd::vector<ExternalProcessMonitor> m_monitored;

};

#endif // GRIDHUB_H

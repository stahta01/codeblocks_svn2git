/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include <sdk.h>
#include "cdb_driver.h"
#include "cdb_commands.h"

#include <backtracedlg.h>
#include <manager.h>
#include <configmanager.h>
#include <globals.h>
#include <infowindow.h>

#define CDB_PROMPT0 _T("0:000>")
#define CDB_PROMPT1 _T("0:001>")

static wxRegEx reBP(_T("Breakpoint ([0-9]+) hit"));
// one stack frame (to access current file; is there another way???)
//  # ChildEBP RetAddr
// 00 0012fe98 00401426 Win32GUI!WinMain+0x89 [c:\devel\tmp\win32 test\main.cpp @ 55]
static wxRegEx reFile(_T("[ \t]([A-z]+.*)[ \t]+\\[([A-z]:)(.*) @ ([0-9]+)\\]"));

CDB_driver::CDB_driver(DebuggerGDB* plugin)
    : DebuggerDriver(plugin),
    m_IsStarted(false)
{
    //ctor
}

CDB_driver::~CDB_driver()
{
    //dtor
}

wxString CDB_driver::GetCommandLine(const wxString& debugger, const wxString& debuggee)
{
    wxString cmd;
    cmd << debugger;
//    cmd << _T(" -g"); // ignore starting breakpoint
    cmd << _T(" -G"); // ignore ending breakpoint
    cmd << _T(" -lines"); // line info

    if (m_Dirs.GetCount() > 0)
    {
        // add symbols dirs
        cmd << _T(" -y ");
        for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i)
        {
            cmd << m_Dirs[i] << wxPATH_SEP;
        }

        // add source dirs
        cmd << _T(" -srcpath ");
        for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i)
        {
            cmd << m_Dirs[i] << wxPATH_SEP;
        }
    }

    // finally, add the program to debug
    cmd << _T(' ') << debuggee;

    if (!m_WorkingDir.IsEmpty())
        wxSetWorkingDirectory(m_WorkingDir);

    return cmd;
}

wxString CDB_driver::GetCommandLine(const wxString& debugger, int pid)
{
    wxString cmd;
    cmd << debugger;
//    cmd << _T(" -g"); // ignore starting breakpoint
    cmd << _T(" -G"); // ignore ending breakpoint
    cmd << _T(" -lines"); // line info

    if (m_Dirs.GetCount() > 0)
    {
        // add symbols dirs
        cmd << _T(" -y ");
        for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i)
        {
            cmd << m_Dirs[i] << wxPATH_SEP;
        }

        // add source dirs
        cmd << _T(" -srcpath ");
        for (unsigned int i = 0; i < m_Dirs.GetCount(); ++i)
        {
            cmd << m_Dirs[i] << wxPATH_SEP;
        }
    }

    // finally, add the PID
    cmd << _T(" -p ") << wxString::Format(_T("%d"), pid);

    if (!m_WorkingDir.IsEmpty())
        wxSetWorkingDirectory(m_WorkingDir);

    return cmd;
}

void CDB_driver::Prepare(ProjectBuildTarget* /*target*/, bool /*isConsole*/)
{
    // default initialization
}

void CDB_driver::Start(bool /*breakOnEntry*/)
{
    // start the process
    QueueCommand(new DebuggerCmd(this, _T("l+t"))); // source mode
    QueueCommand(new DebuggerCmd(this, _T("l+s"))); // show source lines
    QueueCommand(new DebuggerCmd(this, _T("l+o"))); // only source lines

    if (!Manager::Get()->GetConfigManager(_T("debugger"))->ReadBool(_T("do_not_run"), false))
    {
        QueueCommand(new DebuggerCmd(this, _T("g")));
        m_IsStarted = true;
    }
}

void CDB_driver::Stop()
{
    ResetCursor();
    QueueCommand(new DebuggerCmd(this, _T("q")));
    m_IsStarted = false;
}

void CDB_driver::Continue()
{
    ResetCursor();
    QueueCommand(new DebuggerCmd(this, _T("g")));
    m_IsStarted = true;
}

void CDB_driver::Step()
{
    ResetCursor();
    QueueCommand(new DebuggerCmd(this, _T("p")));
    // print a stack frame to find out about the file we 've stopped
    QueueCommand(new CdbCmd_SwitchFrame(this, -1));
}

void CDB_driver::StepInstruction()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::StepIn()
{
    ResetCursor();
    QueueCommand(new DebuggerCmd(this, _T("t")));
    Step();
}

void CDB_driver::StepOut()
{
    ResetCursor();
    QueueCommand(new DebuggerCmd(this, _T("gu")));
    Step();
}

void CDB_driver::SetNextStatement(const wxString& filename, int line)
{
    NOT_IMPLEMENTED();
}

void CDB_driver::Backtrace()
{
    DoBacktrace(false);
}
void CDB_driver::DoBacktrace(bool switchToFirst)
{
    if (Manager::Get()->GetDebuggerManager()->UpdateBacktrace())
        QueueCommand(new CdbCmd_Backtrace(this, switchToFirst));
}

void CDB_driver::Disassemble()
{
    QueueCommand(new CdbCmd_DisassemblyInit(this));
}

void CDB_driver::CPURegisters()
{
    QueueCommand(new CdbCmd_InfoRegisters(this));
}

void CDB_driver::SwitchToFrame(size_t number)
{
    ResetCursor();
    QueueCommand(new CdbCmd_SwitchFrame(this, number));
}

void CDB_driver::SetVarValue(const wxString& /*var*/, const wxString& /*value*/)
{
    NOT_IMPLEMENTED();
}

void CDB_driver::MemoryDump()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::RunningThreads()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::InfoFrame()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::InfoDLL()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::InfoFiles()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::InfoFPU()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::InfoSignals()
{
    NOT_IMPLEMENTED();
}

void CDB_driver::AddBreakpoint(DebuggerBreakpoint* bp)
{
    QueueCommand(new CdbCmd_AddBreakpoint(this, bp));
}

void CDB_driver::RemoveBreakpoint(DebuggerBreakpoint* bp)
{
    QueueCommand(new CdbCmd_RemoveBreakpoint(this, bp));
}

void CDB_driver::EvaluateSymbol(const wxString& symbol, const wxRect& tipRect)
{
    QueueCommand(new CdbCmd_TooltipEvaluation(this, symbol, tipRect));
}

void CDB_driver::UpdateWatches(bool doLocals, bool /*doArgs*/, WatchesContainer &watches)
{
    for (WatchesContainer::iterator it = watches.begin(); it != watches.end(); ++it)
        QueueCommand(new CdbCmd_Watch(this, *it));

    QueueCommand(new DbgCmd_UpdateWatchesTree(this));

    // FIXME (obfuscated#): reimplement this code
//    // start updating watches tree
//    tree->BeginUpdateTree();
//
//    // locals before args because of precedence
//    if (doLocals)
//        QueueCommand(new CdbCmd_InfoLocals(this, tree));
////    if (doArgs)
////        QueueCommand(new CdbCmd_InfoArguments(this, tree));
//    for (unsigned int i = 0; i < tree->GetWatches().GetCount(); ++i)
//    {
//        Watch& w = tree->GetWatches()[i];
//        QueueCommand(new CdbCmd_Watch(this, tree, &w));
//    }
//
//    // run this action-only command to update the tree
//    QueueCommand(new DbgCmd_UpdateWatchesTree(this, tree));
}

void CDB_driver::UpdateWatch(GDBWatch::Pointer const &watch)
{
    QueueCommand(new CdbCmd_Watch(this, watch));
    QueueCommand(new DbgCmd_UpdateWatchesTree(this));
}

void CDB_driver::Attach(int /*pid*/)
{
    // FIXME (obfuscated#): implement this
}

void CDB_driver::Detach()
{
    QueueCommand(new CdbCmd_Detach(this));
}

void CDB_driver::ParseOutput(const wxString& output)
{
    m_Cursor.changed = false;
    static wxString buffer;
    buffer << output << _T('\n');

    m_pDBG->DebugLog(output);

    int idx = buffer.First(CDB_PROMPT0);
    if (idx == wxNOT_FOUND)
        idx = buffer.First(CDB_PROMPT1);
    if (idx != wxNOT_FOUND)
    {
        m_ProgramIsStopped = true;
        m_QueueBusy = false;
        DebuggerCmd* cmd = CurrentCommand();
        if (cmd)
        {
//            Log(_T("Command parsing output: ") + buffer.Left(idx));
            RemoveTopCommand(false);
            buffer.Remove(idx);
            if (buffer[buffer.Length() - 1] == _T('\n'))
                buffer.Remove(buffer.Length() - 1);
            cmd->ParseOutput(buffer.Left(idx));
            delete cmd;
            RunQueue();
        }
    }
    else
    {
        m_ProgramIsStopped = false;
        return; // come back later
    }

    bool notifyChange = false;

    // non-command messages (e.g. breakpoint hits)
    // break them up in lines
    wxArrayString lines = GetArrayFromString(buffer, _T('\n'));
    for (unsigned int i = 0; i < lines.GetCount(); ++i)
    {
//            Log(_T("DEBUG: ") + lines[i]); // write it in the full debugger log

        if (lines[i].StartsWith(_T("Cannot execute ")))
        {
            Log(lines[i]);
        }

        else if (lines[i].Contains(_T("Access violation")))
        {
            Log(lines[i]);
            m_pDBG->BringCBToFront();

            Manager::Get()->GetDebuggerManager()->ShowBacktraceDialog();
            DoBacktrace(true);
            InfoWindow::Display(_("Access violation"), lines[i]);
            break;
        }
        else if (notifyChange)
            continue;

        // Breakpoint 0 hit
        // >   38:     if (!RegisterClassEx (&wincl))
        else if (reBP.Matches(lines[i]))
        {
            Log(lines[i]);

            long int bpNum;
            reBP.GetMatch(lines[i], 1).ToLong(&bpNum);
            DebuggerBreakpoint* bp = m_pDBG->GetState().GetBreakpointByNumber(bpNum);
            if (bp)
            {
                // force cursor notification because we don't have an actual address
                // available...
                m_Cursor.address = _T("deadbeef");

                m_Cursor.file = bp->filename;
                m_Cursor.line = bp->line + 1;
//                if (bp->temporary)
//                    m_pDBG->GetState().RemoveBreakpoint(bp->index);
            }
            else
                Log(wxString::Format(_T("Breakpoints inconsistency detected!\nNothing known about breakpoint %ld"), bpNum));
            m_Cursor.changed = true;
            notifyChange = true;
        }
    }

    if (notifyChange)
        NotifyCursorChanged();

    buffer.Clear();
}

bool CDB_driver::IsDebuggingStarted() const
{
    return m_IsStarted;
}

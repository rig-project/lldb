//===-- SBFrame.cpp ---------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/API/SBFrame.h"

#include <string>
#include <algorithm>

#include "lldb/lldb-types.h"

#include "lldb/Core/Address.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/ValueObjectRegister.h"
#include "lldb/Core/ValueObjectVariable.h"
#include "lldb/Expression/ClangUserExpression.h"
#include "lldb/Host/Host.h"
#include "lldb/Symbol/Block.h"
#include "lldb/Symbol/Function.h"
#include "lldb/Symbol/Symbol.h"
#include "lldb/Symbol/SymbolContext.h"
#include "lldb/Symbol/VariableList.h"
#include "lldb/Symbol/Variable.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/StackID.h"
#include "lldb/Target/Thread.h"

#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBValue.h"
#include "lldb/API/SBAddress.h"
#include "lldb/API/SBExpressionOptions.h"
#include "lldb/API/SBStream.h"
#include "lldb/API/SBSymbolContext.h"
#include "lldb/API/SBThread.h"

using namespace lldb;
using namespace lldb_private;


SBFrame::SBFrame () :
    m_opaque_sp (new ExecutionContextRef())
{
}

SBFrame::SBFrame (const StackFrameSP &lldb_object_sp) :
    m_opaque_sp (new ExecutionContextRef (lldb_object_sp))
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    if (log)
    {
        SBStream sstr;
        GetDescription (sstr);
        log->Printf ("SBFrame::SBFrame (sp=%p) => SBFrame(%p): %s", 
                     lldb_object_sp.get(), lldb_object_sp.get(), sstr.GetData());
                     
    }
}

SBFrame::SBFrame(const SBFrame &rhs) :
    m_opaque_sp (new ExecutionContextRef (*rhs.m_opaque_sp))
{
}

const SBFrame &
SBFrame::operator = (const SBFrame &rhs)
{
    if (this != &rhs)
        *m_opaque_sp = *rhs.m_opaque_sp;
    return *this;
}

SBFrame::~SBFrame()
{
}

StackFrameSP
SBFrame::GetFrameSP() const
{
    if (m_opaque_sp)
        return m_opaque_sp->GetFrameSP();
    return StackFrameSP();
}

void
SBFrame::SetFrameSP (const StackFrameSP &lldb_object_sp)
{
    return m_opaque_sp->SetFrameSP(lldb_object_sp);
}

bool
SBFrame::IsValid() const
{
    return GetFrameSP().get() != NULL;
}

SBSymbolContext
SBFrame::GetSymbolContext (uint32_t resolve_scope) const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBSymbolContext sb_sym_ctx;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_sym_ctx.SetSymbolContext(&frame->GetSymbolContext (resolve_scope));
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetVariables () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetSymbolContext () => error: process is running");
        }
    }

    if (log)
        log->Printf ("SBFrame(%p)::GetSymbolContext (resolve_scope=0x%8.8x) => SBSymbolContext(%p)", 
                     frame, resolve_scope, sb_sym_ctx.get());

    return sb_sym_ctx;
}

SBModule
SBFrame::GetModule () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBModule sb_module;
    ModuleSP module_sp;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                module_sp = frame->GetSymbolContext (eSymbolContextModule).module_sp;
                sb_module.SetSP (module_sp);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetModule () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetModule () => error: process is running");
        }
    }

    if (log)
        log->Printf ("SBFrame(%p)::GetModule () => SBModule(%p)", 
                     frame, module_sp.get());

    return sb_module;
}

SBCompileUnit
SBFrame::GetCompileUnit () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBCompileUnit sb_comp_unit;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_comp_unit.reset (frame->GetSymbolContext (eSymbolContextCompUnit).comp_unit);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetCompileUnit () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetCompileUnit () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetCompileUnit () => SBCompileUnit(%p)", 
                     frame, sb_comp_unit.get());

    return sb_comp_unit;
}

SBFunction
SBFrame::GetFunction () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBFunction sb_function;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_function.reset(frame->GetSymbolContext (eSymbolContextFunction).function);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetFunction () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetFunction () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetFunction () => SBFunction(%p)", 
                     frame, sb_function.get());

    return sb_function;
}

SBSymbol
SBFrame::GetSymbol () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBSymbol sb_symbol;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_symbol.reset(frame->GetSymbolContext (eSymbolContextSymbol).symbol);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetSymbol () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetSymbol () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetSymbol () => SBSymbol(%p)", 
                     frame, sb_symbol.get());
    return sb_symbol;
}

SBBlock
SBFrame::GetBlock () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBBlock sb_block;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_block.SetPtr (frame->GetSymbolContext (eSymbolContextBlock).block);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetBlock () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame(%p)::GetBlock () => error: process is running", frame);
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetBlock () => SBBlock(%p)", 
                     frame, sb_block.GetPtr());
    return sb_block;
}

SBBlock
SBFrame::GetFrameBlock () const
{
    SBBlock sb_block;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_block.SetPtr(frame->GetFrameBlock ());
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetFrameBlock () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetFrameBlock () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetFrameBlock () => SBBlock(%p)", 
                     frame, sb_block.GetPtr());
    return sb_block;    
}

SBLineEntry
SBFrame::GetLineEntry () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBLineEntry sb_line_entry;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_line_entry.SetLineEntry (frame->GetSymbolContext (eSymbolContextLineEntry).line_entry);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetLineEntry () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetLineEntry () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetLineEntry () => SBLineEntry(%p)", 
                     frame, sb_line_entry.get());
    return sb_line_entry;
}

uint32_t
SBFrame::GetFrameID () const
{
    uint32_t frame_idx = UINT32_MAX;
    
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    if (frame)
        frame_idx = frame->GetFrameIndex ();
    
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (log)
        log->Printf ("SBFrame(%p)::GetFrameID () => %u", 
                     frame, frame_idx);
    return frame_idx;
}

addr_t
SBFrame::GetPC () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    addr_t addr = LLDB_INVALID_ADDRESS;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                addr = frame->GetFrameCodeAddress().GetOpcodeLoadAddress (target);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetPC () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetPC () => error: process is running");
        }
    }

    if (log)
        log->Printf ("SBFrame(%p)::GetPC () => 0x%" PRIx64, frame, addr);

    return addr;
}

bool
SBFrame::SetPC (addr_t new_pc)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    bool ret_val = false;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                ret_val = frame->GetRegisterContext()->SetPC (new_pc);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::SetPC () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::SetPC () => error: process is running");
        }
    }

    if (log)
        log->Printf ("SBFrame(%p)::SetPC (new_pc=0x%" PRIx64 ") => %i",
                     frame, new_pc, ret_val);

    return ret_val;
}

addr_t
SBFrame::GetSP () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    addr_t addr = LLDB_INVALID_ADDRESS;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                addr = frame->GetRegisterContext()->GetSP();
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetSP () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetSP () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetSP () => 0x%" PRIx64, frame, addr);

    return addr;
}


addr_t
SBFrame::GetFP () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    addr_t addr = LLDB_INVALID_ADDRESS;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                addr = frame->GetRegisterContext()->GetFP();
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetFP () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetFP () => error: process is running");
        }
    }

    if (log)
        log->Printf ("SBFrame(%p)::GetFP () => 0x%" PRIx64, frame, addr);
    return addr;
}


SBAddress
SBFrame::GetPCAddress () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBAddress sb_addr;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                sb_addr.SetAddress (&frame->GetFrameCodeAddress());
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetPCAddress () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetPCAddress () => error: process is running");
        }
    }
    if (log)
        log->Printf ("SBFrame(%p)::GetPCAddress () => SBAddress(%p)", frame, sb_addr.get());
    return sb_addr;
}

void
SBFrame::Clear()
{
    m_opaque_sp->Clear();
}

lldb::SBValue
SBFrame::GetValueForVariablePath (const char *var_path)
{
    SBValue sb_value;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    if (frame && target)
    {
        lldb::DynamicValueType  use_dynamic = frame->CalculateTarget()->GetPreferDynamicValue();
        sb_value = GetValueForVariablePath (var_path, use_dynamic);
    }
    return sb_value;
}

lldb::SBValue
SBFrame::GetValueForVariablePath (const char *var_path, DynamicValueType use_dynamic)
{
    SBValue sb_value;
    Mutex::Locker api_locker;
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    if (var_path == NULL || var_path[0] == '\0')
    {
        if (log)
            log->Printf ("SBFrame::GetValueForVariablePath called with empty variable path.");
        return sb_value;
    }
    
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                VariableSP var_sp;
                Error error;
                ValueObjectSP value_sp (frame->GetValueForVariableExpressionPath (var_path,
                                                                                  eNoDynamicValues,
                                                                                  StackFrame::eExpressionPathOptionCheckPtrVsMember | StackFrame::eExpressionPathOptionsAllowDirectIVarAccess,
                                                                                  var_sp,
                                                                                  error));
                sb_value.SetSP(value_sp, use_dynamic);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetValueForVariablePath () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetValueForVariablePath () => error: process is running");
        }
    }
    return sb_value;
}

SBValue
SBFrame::FindVariable (const char *name)
{
    SBValue value;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    if (frame && target)
    {
        lldb::DynamicValueType  use_dynamic = frame->CalculateTarget()->GetPreferDynamicValue();
        value = FindVariable (name, use_dynamic);
    }
    return value;
}
                                    

SBValue
SBFrame::FindVariable (const char *name, lldb::DynamicValueType use_dynamic)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    VariableSP var_sp;
    SBValue sb_value;

    if (name == NULL || name[0] == '\0')
    {
        if (log)
            log->Printf ("SBFrame::FindVariable called with empty name");
        return sb_value;
    }
    
    ValueObjectSP value_sp;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                VariableList variable_list;
                SymbolContext sc (frame->GetSymbolContext (eSymbolContextBlock));

                if (sc.block)
                {
                    const bool can_create = true;
                    const bool get_parent_variables = true;
                    const bool stop_if_block_is_inlined_function = true;
                    
                    if (sc.block->AppendVariables (can_create, 
                                                   get_parent_variables,
                                                   stop_if_block_is_inlined_function,
                                                   &variable_list))
                    {
                        var_sp = variable_list.FindVariable (ConstString(name));
                    }
                }

                if (var_sp)
                {
                    value_sp = frame->GetValueObjectForFrameVariable(var_sp, eNoDynamicValues);
                    sb_value.SetSP(value_sp, use_dynamic);
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::FindVariable () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::FindVariable () => error: process is running");
        }
    }
    
    if (log)
        log->Printf ("SBFrame(%p)::FindVariable (name=\"%s\") => SBValue(%p)", 
                     frame, name, value_sp.get());

    return sb_value;
}

SBValue
SBFrame::FindValue (const char *name, ValueType value_type)
{
    SBValue value;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    if (frame && target)
    {
        lldb::DynamicValueType use_dynamic = frame->CalculateTarget()->GetPreferDynamicValue();
        value = FindValue (name, value_type, use_dynamic);
    }
    return value;
}

SBValue
SBFrame::FindValue (const char *name, ValueType value_type, lldb::DynamicValueType use_dynamic)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    SBValue sb_value;
    
    if (name == NULL || name[0] == '\0')
    {
        if (log)
            log->Printf ("SBFrame::FindValue called with empty name.");
        return sb_value;
    }
    
    ValueObjectSP value_sp;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                switch (value_type)
                {
                case eValueTypeVariableGlobal:      // global variable
                case eValueTypeVariableStatic:      // static variable
                case eValueTypeVariableArgument:    // function argument variables
                case eValueTypeVariableLocal:       // function local variables
                    {
                        VariableList *variable_list = frame->GetVariableList(true);

                        SymbolContext sc (frame->GetSymbolContext (eSymbolContextBlock));

                        const bool can_create = true;
                        const bool get_parent_variables = true;
                        const bool stop_if_block_is_inlined_function = true;

                        if (sc.block && sc.block->AppendVariables (can_create, 
                                                                   get_parent_variables,
                                                                   stop_if_block_is_inlined_function,
                                                                   variable_list))
                        {
                            ConstString const_name(name);
                            const uint32_t num_variables = variable_list->GetSize();
                            for (uint32_t i = 0; i < num_variables; ++i)
                            {
                                VariableSP variable_sp (variable_list->GetVariableAtIndex(i));
                                if (variable_sp && 
                                    variable_sp->GetScope() == value_type &&
                                    variable_sp->GetName() == const_name)
                                {
                                    value_sp = frame->GetValueObjectForFrameVariable (variable_sp, eNoDynamicValues);
                                    sb_value.SetSP (value_sp, use_dynamic);
                                    break;
                                }
                            }
                        }
                    }
                    break;

                case eValueTypeRegister:            // stack frame register value
                    {
                        RegisterContextSP reg_ctx (frame->GetRegisterContext());
                        if (reg_ctx)
                        {
                            const uint32_t num_regs = reg_ctx->GetRegisterCount();
                            for (uint32_t reg_idx = 0; reg_idx < num_regs; ++reg_idx)
                            {
                                const RegisterInfo *reg_info = reg_ctx->GetRegisterInfoAtIndex (reg_idx);
                                if (reg_info && 
                                    ((reg_info->name && strcasecmp (reg_info->name, name) == 0) ||
                                     (reg_info->alt_name && strcasecmp (reg_info->alt_name, name) == 0)))
                                {
                                    value_sp = ValueObjectRegister::Create (frame, reg_ctx, reg_idx);
                                    sb_value.SetSP (value_sp);
                                    break;
                                }
                            }
                        }
                    }
                    break;

                case eValueTypeRegisterSet:         // A collection of stack frame register values
                    {
                        RegisterContextSP reg_ctx (frame->GetRegisterContext());
                        if (reg_ctx)
                        {
                            const uint32_t num_sets = reg_ctx->GetRegisterSetCount();
                            for (uint32_t set_idx = 0; set_idx < num_sets; ++set_idx)
                            {
                                const RegisterSet *reg_set = reg_ctx->GetRegisterSet (set_idx);
                                if (reg_set && 
                                    ((reg_set->name && strcasecmp (reg_set->name, name) == 0) ||
                                     (reg_set->short_name && strcasecmp (reg_set->short_name, name) == 0)))
                                {
                                    value_sp = ValueObjectRegisterSet::Create (frame, reg_ctx, set_idx);
                                    sb_value.SetSP (value_sp);
                                    break;
                                }
                            }
                        }
                    }
                    break;

                case eValueTypeConstResult:         // constant result variables
                    {
                        ConstString const_name(name);
                        ClangExpressionVariableSP expr_var_sp (target->GetPersistentVariables().GetVariable (const_name));
                        if (expr_var_sp)
                        {
                            value_sp = expr_var_sp->GetValueObject();
                            sb_value.SetSP (value_sp, use_dynamic);
                        }
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::FindValue () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::FindValue () => error: process is running");
        }
    }
    
    if (log)
        log->Printf ("SBFrame(%p)::FindVariableInScope (name=\"%s\", value_type=%i) => SBValue(%p)", 
                     frame, name, value_type, value_sp.get());

    
    return sb_value;
}

bool
SBFrame::IsEqual (const SBFrame &that) const
{
    lldb::StackFrameSP this_sp = GetFrameSP();
    lldb::StackFrameSP that_sp = that.GetFrameSP();
    return (this_sp && that_sp && this_sp->GetStackID() == that_sp->GetStackID());
}

bool
SBFrame::operator == (const SBFrame &rhs) const
{
    return IsEqual(rhs);
}

bool
SBFrame::operator != (const SBFrame &rhs) const
{
    return !IsEqual(rhs);
}

SBThread
SBFrame::GetThread () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    ExecutionContext exe_ctx(m_opaque_sp.get());
    ThreadSP thread_sp (exe_ctx.GetThreadSP());
    SBThread sb_thread (thread_sp);

    if (log)
    {
        SBStream sstr;
        sb_thread.GetDescription (sstr);
        log->Printf ("SBFrame(%p)::GetThread () => SBThread(%p): %s", 
                     exe_ctx.GetFramePtr(), 
                     thread_sp.get(), 
                     sstr.GetData());
    }

    return sb_thread;
}

const char *
SBFrame::Disassemble () const
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    const char *disassembly = NULL;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                disassembly = frame->Disassemble();
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::Disassemble () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::Disassemble () => error: process is running");
        }            
    }

    if (log)
        log->Printf ("SBFrame(%p)::Disassemble () => %s", frame, disassembly);

    return disassembly;
}


SBValueList
SBFrame::GetVariables (bool arguments,
                       bool locals,
                       bool statics,
                       bool in_scope_only)
{
    SBValueList value_list;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    if (frame && target)
    {
        lldb::DynamicValueType use_dynamic = frame->CalculateTarget()->GetPreferDynamicValue();
        value_list = GetVariables (arguments, locals, statics, in_scope_only, use_dynamic);
    }
    return value_list;
}

SBValueList
SBFrame::GetVariables (bool arguments,
                       bool locals,
                       bool statics,
                       bool in_scope_only,
                       lldb::DynamicValueType  use_dynamic)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    SBValueList value_list;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();

    if (log)
        log->Printf ("SBFrame::GetVariables (arguments=%i, locals=%i, statics=%i, in_scope_only=%i)", 
                     arguments,
                     locals,
                     statics,
                     in_scope_only);
    
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                size_t i;
                VariableList *variable_list = NULL;
                variable_list = frame->GetVariableList(true);
                if (variable_list)
                {
                    const size_t num_variables = variable_list->GetSize();
                    if (num_variables)
                    {
                        for (i = 0; i < num_variables; ++i)
                        {
                            VariableSP variable_sp (variable_list->GetVariableAtIndex(i));
                            if (variable_sp)
                            {
                                bool add_variable = false;
                                switch (variable_sp->GetScope())
                                {
                                case eValueTypeVariableGlobal:
                                case eValueTypeVariableStatic:
                                    add_variable = statics;
                                    break;

                                case eValueTypeVariableArgument:
                                    add_variable = arguments;
                                    break;

                                case eValueTypeVariableLocal:
                                    add_variable = locals;
                                    break;

                                default:
                                    break;
                                }
                                if (add_variable)
                                {
                                    if (in_scope_only && !variable_sp->IsInScope(frame))
                                        continue;

                                    ValueObjectSP valobj_sp(frame->GetValueObjectForFrameVariable (variable_sp, eNoDynamicValues));
                                    SBValue value_sb;
                                    value_sb.SetSP(valobj_sp,use_dynamic);
                                    value_list.Append(value_sb);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetVariables () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetVariables () => error: process is running");
        }            
    }

    if (log)
    {
        log->Printf ("SBFrame(%p)::GetVariables (...) => SBValueList(%p)", frame, value_list.opaque_ptr());
    }

    return value_list;
}

SBValueList
SBFrame::GetRegisters ()
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    SBValueList value_list;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                RegisterContextSP reg_ctx (frame->GetRegisterContext());
                if (reg_ctx)
                {
                    const uint32_t num_sets = reg_ctx->GetRegisterSetCount();
                    for (uint32_t set_idx = 0; set_idx < num_sets; ++set_idx)
                    {
                        value_list.Append(ValueObjectRegisterSet::Create (frame, reg_ctx, set_idx));
                    }
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetRegisters () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetRegisters () => error: process is running");
        }            
    }

    if (log)
        log->Printf ("SBFrame(%p)::GetRegisters () => SBValueList(%p)", frame, value_list.opaque_ptr());

    return value_list;
}

SBValue
SBFrame::FindRegister (const char *name)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));

    SBValue result;
    ValueObjectSP value_sp;
    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                RegisterContextSP reg_ctx (frame->GetRegisterContext());
                if (reg_ctx)
                {
                    const uint32_t num_regs = reg_ctx->GetRegisterCount();
                    for (uint32_t reg_idx = 0; reg_idx < num_regs; ++reg_idx)
                    {
                        const RegisterInfo *reg_info = reg_ctx->GetRegisterInfoAtIndex (reg_idx);
                        if (reg_info && 
                            ((reg_info->name && strcasecmp (reg_info->name, name) == 0) ||
                             (reg_info->alt_name && strcasecmp (reg_info->alt_name, name) == 0)))
                        {
                            value_sp = ValueObjectRegister::Create (frame, reg_ctx, reg_idx);
                            result.SetSP (value_sp);
                            break;
                        }
                    }
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::FindRegister () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::FindRegister () => error: process is running");
        }            
    }

    if (log)
        log->Printf ("SBFrame(%p)::FindRegister () => SBValue(%p)", frame, value_sp.get());

    return result;
}

bool
SBFrame::GetDescription (SBStream &description)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    Stream &strm = description.ref();

    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    StackFrame *frame;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                frame->DumpUsingSettingsFormat (&strm);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetDescription () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetDescription () => error: process is running");
        }            

    }
    else
        strm.PutCString ("No value");

    return true;
}

SBValue
SBFrame::EvaluateExpression (const char *expr)
{
    SBValue result;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = exe_ctx.GetFramePtr();
    Target *target = exe_ctx.GetTargetPtr();
    if (frame && target)
    {
        SBExpressionOptions options;
        lldb::DynamicValueType fetch_dynamic_value = frame->CalculateTarget()->GetPreferDynamicValue();
        options.SetFetchDynamicValue (fetch_dynamic_value);
        options.SetUnwindOnError (true);
        return EvaluateExpression (expr, options);
    }
    return result;
}

SBValue
SBFrame::EvaluateExpression (const char *expr, lldb::DynamicValueType fetch_dynamic_value)
{
    SBExpressionOptions options;
    options.SetFetchDynamicValue (fetch_dynamic_value);
    options.SetUnwindOnError (true);
    return EvaluateExpression (expr, options);
}

SBValue
SBFrame::EvaluateExpression (const char *expr, lldb::DynamicValueType fetch_dynamic_value, bool unwind_on_error)
{
    SBExpressionOptions options;
    options.SetFetchDynamicValue (fetch_dynamic_value);
    options.SetUnwindOnError (unwind_on_error);
    return EvaluateExpression (expr, options);
}

lldb::SBValue
SBFrame::EvaluateExpression (const char *expr, const SBExpressionOptions &options)
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    
    Log *expr_log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_EXPRESSIONS));

    ExecutionResults exe_results = eExecutionSetupError;
    SBValue expr_result;
    
    if (expr == NULL || expr[0] == '\0')
    {
        if (log)
            log->Printf ("SBFrame::EvaluateExpression called with an empty expression");
        return expr_result;
    }
    
    ValueObjectSP expr_value_sp;

    Mutex::Locker api_locker;
    ExecutionContext exe_ctx (m_opaque_sp.get(), api_locker);

    if (log)
        log->Printf ("SBFrame()::EvaluateExpression (expr=\"%s\")...", expr);

    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                if (target->GetDisplayExpressionsInCrashlogs())
                {
                    StreamString frame_description;
                    frame->DumpUsingSettingsFormat (&frame_description);
                    Host::SetCrashDescriptionWithFormat ("SBFrame::EvaluateExpression (expr = \"%s\", fetch_dynamic_value = %u) %s",
                                                         expr, options.GetFetchDynamicValue(), frame_description.GetString().c_str());
                }
                
                exe_results = target->EvaluateExpression (expr,
                                                          frame,
                                                          expr_value_sp,
                                                          options.ref());
                expr_result.SetSP(expr_value_sp, options.GetFetchDynamicValue());

                if (target->GetDisplayExpressionsInCrashlogs())
                    Host::SetCrashDescription (NULL);
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::EvaluateExpression () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::EvaluateExpression () => error: process is running");
        }            
    }

#ifndef LLDB_DISABLE_PYTHON
    if (expr_log)
        expr_log->Printf("** [SBFrame::EvaluateExpression] Expression result is %s, summary %s **", 
                         expr_result.GetValue(), 
                         expr_result.GetSummary());
    
    if (log)
        log->Printf ("SBFrame(%p)::EvaluateExpression (expr=\"%s\") => SBValue(%p) (execution result=%d)", 
                     frame, 
                     expr, 
                     expr_value_sp.get(),
                     exe_results);
#endif

    return expr_result;
}

bool
SBFrame::IsInlined()
{
    Log *log(GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {

                Block *block = frame->GetSymbolContext(eSymbolContextBlock).block;
                if (block)
                    return block->GetContainingInlinedBlock () != NULL;
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::IsInlined () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::IsInlined () => error: process is running");
        }            

    }
    return false;
}

const char *
SBFrame::GetFunctionName()
{
    Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_API));
    const char *name = NULL;
    ExecutionContext exe_ctx(m_opaque_sp.get());
    StackFrame *frame = NULL;
    Target *target = exe_ctx.GetTargetPtr();
    Process *process = exe_ctx.GetProcessPtr();
    if (target && process)
    {
        Process::StopLocker stop_locker;
        if (stop_locker.TryLock(&process->GetRunLock()))
        {
            frame = exe_ctx.GetFramePtr();
            if (frame)
            {
                SymbolContext sc (frame->GetSymbolContext(eSymbolContextFunction | eSymbolContextBlock | eSymbolContextSymbol));
                if (sc.block)
                {
                    Block *inlined_block = sc.block->GetContainingInlinedBlock ();
                    if (inlined_block)
                    {
                        const InlineFunctionInfo* inlined_info = inlined_block->GetInlinedFunctionInfo();
                        name = inlined_info->GetName().AsCString();
                    }
                }
                
                if (name == NULL)
                {
                    if (sc.function)
                        name = sc.function->GetName().GetCString();
                }

                if (name == NULL)
                {
                    if (sc.symbol)
                        name = sc.symbol->GetName().GetCString();
                }
            }
            else
            {
                if (log)
                    log->Printf ("SBFrame::GetFunctionName () => error: could not reconstruct frame object for this SBFrame.");
            }
        }
        else
        {
            if (log)
                log->Printf ("SBFrame::GetFunctionName() => error: process is running");

        }
    }
    return name;
}


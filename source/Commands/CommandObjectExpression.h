//===-- CommandObjectExpression.h -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_CommandObjectExpression_h_
#define liblldb_CommandObjectExpression_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Core/IOHandler.h"
#include "lldb/Interpreter/CommandObject.h"
#include "lldb/Interpreter/OptionGroupFormat.h"
#include "lldb/Interpreter/OptionGroupValueObjectDisplay.h"
#include "lldb/Target/ExecutionContext.h"

namespace lldb_private {

class CommandObjectExpression :
    public CommandObjectRaw,
    public IOHandlerDelegate
{
public:

    class CommandOptions : public OptionGroup
    {
    public:

        CommandOptions ();

        virtual
        ~CommandOptions ();

        virtual uint32_t
        GetNumDefinitions ();
        
        virtual const OptionDefinition*
        GetDefinitions ();
        
        virtual Error
        SetOptionValue (CommandInterpreter &interpreter,
                        uint32_t option_idx,
                        const char *option_value);
        
        virtual void
        OptionParsingStarting (CommandInterpreter &interpreter);

        // Options table: Required for subclasses of Options.

        static OptionDefinition g_option_table[];
        bool        unwind_on_error;
        bool        ignore_breakpoints;
        bool        show_types;
        bool        show_summary;
        bool        debug;
        uint32_t    timeout;
        bool        try_all_threads;
        LanguageRuntimeDescriptionDisplayVerbosity m_verbosity;
    };

    CommandObjectExpression (CommandInterpreter &interpreter);

    virtual
    ~CommandObjectExpression ();

    virtual
    Options *
    GetOptions ();

protected:
    
    //------------------------------------------------------------------
    // IOHandler::Delegate functions
    //------------------------------------------------------------------
    virtual void
    IOHandlerActivated (IOHandler &io_handler);

    virtual void
    IOHandlerInputComplete (IOHandler &io_handler,
                            std::string &line);

    virtual LineStatus
    IOHandlerLinesUpdated (IOHandler &io_handler,
                           StringList &lines,
                           uint32_t line_idx,
                           Error &error);
    virtual bool
    DoExecute (const char *command,
               CommandReturnObject &result);

    bool
    EvaluateExpression (const char *expr,
                        Stream *output_stream,
                        Stream *error_stream,
                        CommandReturnObject *result = NULL);

    OptionGroupOptions m_option_group;
    OptionGroupFormat m_format_options;
    OptionGroupValueObjectDisplay m_varobj_options;
    CommandOptions m_command_options;
    uint32_t m_expr_line_count;
    std::string m_expr_lines; // Multi-line expression support
};

} // namespace lldb_private

#endif  // liblldb_CommandObjectExpression_h_

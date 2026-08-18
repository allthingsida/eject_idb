// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

// Eject IDB by Elias Bachaalany(c) AllThingsIDA

#include <memory>
#include <string>

#ifdef __NT__
    #include <windows.h>
#else
    #include <unistd.h>
#endif

// Include BEFORE the IDA headers: pro.h poisons identifiers (e.g. `wait` in
// SDK 9.2) that C++20 std headers pulled in here (<atomic>, <thread>) use.
#include <libidacpp/ipc/named_semaphore.hpp>

#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

#include "utils.hpp"

// TODO:
// - use on_event/modern mechanism
// - delay event handler installation with a timer. give time to other plugins to install their handlers; come in last

//--------------------------------------------------------------------------
struct plugin_ctx_t : public plugmod_t
{
    libidacpp::ipc::semaphore_waiter_t waiter;

    bool disable_ui = false;
    static ssize_t idaapi ui_callback(void* ud, int notification_code, va_list va)
    {
        plugin_ctx_t* ctx = (plugin_ctx_t*)ud;
        return ctx->disable_ui ? 1 : 0;
    }

    void do_eject()
    {
        qstring p = get_idb_path();
        auto new_name = derive_ejected_path(p.c_str());
        if (!new_name)
            return;

        // unfortunately, `save_database` calls the main thread/UI to display success/failure messages
        // thus, before 'ejecting', let's disable/disallow all UI messages from being processed.
        disable_ui = true;
        flush_buffers();
        save_database(new_name->c_str(), DBFL_BAK);
        // Now it is safe to kill IDA.
#ifdef __NT__
        if (MessageBoxW(NULL, L"IDB has been ejected. Do you want to forcefully exit IDA?", L"eject_idb", MB_YESNO | MB_ICONINFORMATION) == IDYES)
            ExitProcess(0);
#else
        // IDA's UI is presumed hung, so no dialogs: print to the IDA console
        // (may not repaint) and to the terminal IDA was launched from.
        // (qeprintf = the SDK's stderr printf; CRT fprintf/stderr are poisoned.)
        msg("eject_idb: IDB ejected to %s\n", new_name->c_str());
        qeprintf("eject_idb: IDB ejected to %s\n"
                 "IDA may be hung -- you may now kill it: kill -9 %d\n",
                 new_name->c_str(), (int)getpid());
#endif
        disable_ui = false;
    }

    plugin_ctx_t()
    {
        std::string sem_name = make_semaphore_name(get_idb_path().c_str());
        if (waiter.start(sem_name.c_str(), [this]() { do_eject(); }))
        {
            msg("eject_idb installed. call the 'eject_idb \"%s\"' command line tool to eject this database!\n",
                get_idb_path().c_str());
        }
        else
        {
            // e.g. sandboxed macOS denying sem_open: stay loaded but inert.
            msg("eject_idb: could not create the eject semaphore; plugin is inactive.\n");
        }
        hook_to_notification_point(HT_UI, ui_callback, this);
    }

    // This hanging mode (only if poll is true) can be 'ejected'
    static bool hang_with_loop(bool poll=false)
    {
        while (true)
        {
            if (poll)
                user_cancelled();
            qsleep(1000);
        }
        return true;
    }

    bool idaapi run(size_t arg) override
    {
        switch (arg)
        {
            case 0:
                if (ask_yn(1, "HIDECANCEL\nDo you want to simulate a non-responsive UI hang?") == ASKBTN_YES)
                    hang_with_loop(false);
                break;
            case 1:
                if (ask_yn(1, "HIDECANCEL\nDo you want to simulate a responsive UI hang?") == ASKBTN_YES)
                    hang_with_loop(true);
                break;
        }
        return true;
    }

    ~plugin_ctx_t() override
    {
        waiter.stop();
        unhook_from_notification_point(HT_UI, ui_callback, this);
    }
};

//--------------------------------------------------------------------------
plugin_t PLUGIN =
{
    IDP_INTERFACE_VERSION,
    PLUGIN_MULTI,
    []()->plugmod_t* { return new plugin_ctx_t; },
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    "eject_idb: simulate an infinite loop",
    nullptr
};

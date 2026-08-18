// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

/*
eject_core: pure helpers shared by the plugin and the standalone signaler.
No IDA SDK, no platform headers — headlessly testable.
*/

#pragma once

#include <cstdint>
#include <optional>
#include <string>

/*
 * Hash function (djb2) by Dan Bernstein.
 * Source: Originally described in comp.lang.c
 * Accumulates in uint32_t so the value is identical on every platform
 * (unsigned long is 64-bit on LP64 Linux/macOS, which would diverge from
 * Windows for the same input).
 */
inline uint32_t djb2(const char* str)
{
    uint32_t hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + (uint32_t)c; /* hash * 33 + c */
    return hash;
}

// Base name of the eject semaphore for a given IDB path (no slashes; platform
// naming rules are applied inside libidacpp::ipc::named_semaphore_t). The hash
// is formatted by hand ("%08x") because the IDA SDK poisons CRT snprintf and
// this header is shared with the non-IDA signaler.
inline std::string make_semaphore_name(const char* idb_path)
{
    static const char hexdig[] = "0123456789abcdef";
    uint32_t hash = djb2(idb_path);

    std::string name = "ejectidb_";
    for (int shift = 28; shift >= 0; shift -= 4)
        name += hexdig[(hash >> shift) & 0xF];
    return name;
}

// Derive "<stem>.ejected.<ext>" from an IDB path. The extension dot must be in
// the filename component (a dot in a directory name does not count). Returns
// nullopt when the filename has no extension (including dotfile-only names).
inline std::optional<std::string> derive_ejected_path(const std::string& path)
{
    const size_t sep = path.find_last_of("/\\");
    const size_t name_begin = (sep == std::string::npos) ? 0 : sep + 1;

    const size_t dot = path.rfind('.');
    if (dot == std::string::npos || dot <= name_begin)
        return std::nullopt;   // no extension, or dotfile-only name

    return path.substr(0, dot) + ".ejected" + path.substr(dot);
}

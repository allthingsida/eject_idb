// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

#pragma once

#include "eject_core.hpp"

#ifdef __IDP__
#include <pro.h>
#include <loader.hpp>

inline qstring get_idb_path() { return get_path(PATH_TYPE_IDB); }

#endif

// Copyright (c) 2019-2026 Elias Bachaalany
// SPDX-License-Identifier: LicenseRef-Human-Origin-Source-1.0
//
// This file is licensed under the Human-Origin Source License v1.0.
// See LICENSE.

#include <cstdio>

#include <libidacpp/ipc/named_semaphore.hpp>

#include "eject_core.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Usage: %s <IDB file path>\n", argv[0]);
        return 1;
    }

#ifdef __TESTING__
    char input[100]; // buffer for user input
    printf("eject_idb waiting...press ENTER to continue\n");
    fgets(input, sizeof(input), stdin); // read a line, user must press enter
#endif

    std::string sem_name = make_semaphore_name(argv[1]);

    libidacpp::ipc::named_semaphore_t sem;
    if (sem.open(sem_name.c_str()) && sem.post())
    {
        printf("Ejected %s\n", argv[1]);
        return 0;
    }

    printf("Failed to eject %s\n", argv[1]);
    return 1;
}

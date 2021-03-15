// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#ifndef _GOBJ_TRACKER_H_
#define _GOBJ_TRACKER_H_

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

extern inline char *__orbit_tracker_file_name(char *buf);
extern inline void __orbit_track_gobj(char *addr, size_t size);
extern inline void *__orbit_alloc_gobj(size_t size);
void __orbit_gobj_tracker_init();
bool __orbit_gobj_tracker_dump();
void __orbit_gobj_tracker_finish();
#ifdef __cplusplus
}  // extern "C"
#endif

#endif /* _GOBJ_TRACKER_H_ */

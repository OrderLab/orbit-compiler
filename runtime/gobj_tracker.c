// The Obi-wan Project
//
// Copyright (c) 2021, Johns Hopkins University - Order Lab.
//
//    All rights reserved.
//    Licensed under the Apache License, Version 2.0 (the "License");
//

#include "gobj_tracker.h"

FILE *__orbit_tracker_file;
#define MAX_FILE_NAME_SIZE 64

char *__orbit_tracker_file_name(char *buf) {
  snprintf(buf, 64, "orbit_gobj_pid_%d.dat", getpid());
  return buf;
}

void termination_handler(int signum) {
  fclose(__orbit_tracker_file);
  exit(-1);
}

void __orbit_gobj_tracker_init() {
  char filename_buf[MAX_FILE_NAME_SIZE];
  char *filename = __orbit_tracker_file_name(filename_buf);
  fprintf(stderr, "opening orbit tracker output file %s\n", filename);
  __orbit_tracker_file = fopen(filename, "w");

  struct sigaction new_action, old_action;
  new_action.sa_handler = termination_handler;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags = 0;
  new_action.sa_sigaction = &termination_handler;

  sigaction(SIGINT, &new_action, NULL);
  sigaction(SIGFPE, &new_action, NULL);
  sigaction(SIGTERM, &new_action, NULL);
  sigaction(SIGSEGV, &new_action, NULL);
  sigaction(SIGILL, &new_action, NULL);
  sigaction(SIGABRT, &new_action, NULL);
  sigaction(SIGBUS, &new_action, NULL);
  sigaction(SIGSTKFLT, &new_action, NULL);
}

inline void __orbit_track_gobj(char *addr, size_t size) {
   fprintf(__orbit_tracker_file, "%zu => %p\n", size, addr);
}

bool __orbit_gobj_tracker_dump() {
  fflush(__orbit_tracker_file);
  return true;
}

void __orbit_gobj_tracker_finish() {
  // close the tracker file
  fflush(__orbit_tracker_file);
  fclose(__orbit_tracker_file);
}

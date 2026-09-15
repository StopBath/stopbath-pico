/*
 * The vendored Waveshare drivers include this name and call Debug(...) for
 * their trace lines. Upstream's version prints through stdio; this project
 * enables no stdio (firmware/DEV_Config.h says why), so the macro expands to
 * nothing and the trace lines cost nothing.
 */
#pragma once

#define Debug(...) ((void)0)

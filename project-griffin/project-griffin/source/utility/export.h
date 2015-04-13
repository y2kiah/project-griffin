#pragma once
#ifndef GRIFFIN_EXPORT_H_
#define GRIFFIN_EXPORT_H_

#ifdef _WIN32

#ifdef __GNUC__
#define GRIFFIN_EXPORT __attribute__ ((dllexport))
#else
#define GRIFFIN_EXPORT __declspec(dllexport)
#endif

#else

#if __GNUC__ >= 4
#define GRIFFIN_EXPORT __attribute__ ((visibility ("default")))
#else
#define GRIFFIN_EXPORT
#endif
#endif

#endif
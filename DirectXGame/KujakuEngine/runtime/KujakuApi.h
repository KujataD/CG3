#pragma once

#if defined(_WIN32)
#if defined(KUJAKU_ENGINE_EXPORTS)
#define KUJAKU_API __declspec(dllexport)
#else
#define KUJAKU_API __declspec(dllimport)
#endif
#else
#define KUJAKU_API
#endif

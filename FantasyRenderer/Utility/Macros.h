#pragma once

#ifndef CLASS_NO_COPY
#define CLASS_NO_COPY(ClassName)                        \
public:                                                 \
    ClassName(const ClassName&) = delete;               \
    ClassName(ClassName&&) = delete;                    \
    ClassName& operator=(const ClassName&) = delete;    \
    ClassName& operator=(ClassName&&) = delete;
#endif

#ifndef CLASS_DEFAULT_COPY
#define CLASS_DEFAULT_COPY(ClassName)                   \
public:                                                 \
    ClassName(const ClassName&) = default;              \
    ClassName(ClassName&&) = default;                   \
    ClassName& operator=(const ClassName&) = default;   \
    ClassName& operator=(ClassName&&) = default;
#endif


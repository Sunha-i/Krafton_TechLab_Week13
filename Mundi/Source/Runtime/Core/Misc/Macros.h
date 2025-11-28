#pragma once

// 기타 유틸 매크로 정의하려고 따로 만듬

#if defined(_MSC_VER)
    #include <intrin.h>
    #define DEBUG_BREAK() __debugbreak()
#else
    #define DEBUG_BREAK() ((void)0)
#endif

// 디버그 모드일 때만 check 매크로 사용
#ifdef _DEBUG
    #define CHECK(expr)\
        do\
        {\
            if(!(expr))\
            {\
                DEBUG_BREAK();\
            }\
        } while(false)
    #define CHECK_MSG(expr, msg)\
        do\
        {\
            if(!(expr))\
            {\
                UE_LOG(msg); \
                DEBUG_BREAK();\
            }\
        } while(false)
#else
// 릴리즈때는 아무것도 안함
    #define CHECK(expr) ((void)0)
    #define CHECK_MSG(expr, msg) ((void)0)
#endif

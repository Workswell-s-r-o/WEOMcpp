#ifndef VERIFY_H
#define VERIFY_H


#if !defined(VERIFY)
    #if !defined(NDEBUG)
        #define VERIFY assert
    #else
        #define VERIFY(expr)  \
        do                    \
        {                     \
            (void) (expr);    \
        } while (0)
    #endif
#endif

#endif // VERIFY_H

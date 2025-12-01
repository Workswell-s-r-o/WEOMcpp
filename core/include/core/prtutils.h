#pragma once

#define derefPtr(Expr) \
    [](auto _nullptr_) -> auto& { assert(_nullptr_); return *_nullptr_; }(Expr)

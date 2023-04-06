//
// Created by Sreejith Krishnan R on 23/02/23.
//

#pragma once

#include "common.h"


#define kcmod_verify(condition)                                                                                 \
    if (!(condition)) {                                                                                         \
        throw FatalError{"verify condition {} failed at {}:{}", #condition, __FILE__, __LINE__}; \
    }                                                                                                           \
    0


#define kcmod_not_reachable() throw FatalError{"not reachable at {}:{}", __FILE__, __LINE__}

#define kcmod_todo() throw FatalError{"todo at {}:{}", __FILE__, __LINE__}

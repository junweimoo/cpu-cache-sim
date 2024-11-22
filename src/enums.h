#ifndef ENUMS_H
#define ENUMS_H

enum BusMessage {
    ReadExclusive,
    Read,
    WriteBack,
    Invalidate
};

enum BusResponse {
    NoResponse,
    HasCopy,
    IsNotShared
};

enum CacheState {
    Modified,
    Exclusive,
    Shared,
    Invalid,
    NotPresent
};

#endif //ENUMS_H

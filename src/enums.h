#ifndef ENUMS_H
#define ENUMS_H

enum BusMessage {
    // MESI
    ReadExclusive,
    Read,
    WriteBack,

    // Dragon
    ReadDragon,
    BusUpdate,

    // Common
    NoMessage,
};

enum BusResponse {
    NoResponse,
    HasCopy,
};

enum CacheState {
    // MESI
    Modified,
    Exclusive,
    Shared,
    Invalid,

    // Dragon
    ExclusiveDragon,
    SharedClean,
    SharedModified,
    Dirty,

    // Common
    NotPresent,
};

enum ProcessorAction {
    PrWrite,
    PrRead,
};

enum Protocol {
    MESI,
    Dragon,
};

#endif //ENUMS_H

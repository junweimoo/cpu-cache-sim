#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
     constexpr int CACHE_HIT_TIME = 1;
     constexpr int SEND_WORD_TIME = 2;
     constexpr int MEM_FETCH_TIME = 100;
     constexpr int MEM_FLUSH_TIME = 100;
     constexpr int ADDRESS_BITS = 32;
     constexpr int WORD_SIZE_BITS = 32;
}

#endif //CONFIG_H

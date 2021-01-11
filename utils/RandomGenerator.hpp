#ifndef ANONYMOUSP2P_RANDOMGENERATOR_HPP
#define ANONYMOUSP2P_RANDOMGENERATOR_HPP

class RandomGenerator {
public:
    unsigned long seed;
    RandomGenerator(unsigned long _seed): seed(_seed) {}
    RandomGenerator(): RandomGenerator(0) {}

    void srand(unsigned long _seed) {
        seed = _seed;
    }
    
    unsigned long rand() {
        return (((seed = seed * 214013UL + 2531011UL) >> 16) & 0x7fffffffUL);
    }
};

#endif //ANONYMOUSP2P_ORDEREDVECTOR_HPP

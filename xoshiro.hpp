/* Code was taken from here:
   https://prng.di.unimi.it
   And wrapped as a bit generator for C++'s random module' */

/*  Written in 2019 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */
#ifndef XOSHIRO_H
#define XOSHIRO_H

#include <random>
#include <iostream>
#include <cstring>
#include <cstdint>

namespace Xoshiro {


#if __cplusplus >= 202001L
#   define HAS_CPP20
#endif

static inline std::uint64_t rotl64(const std::uint64_t x, const int k) {
    return (x << k) | (x >> (64 - k));
}

static inline std::uint32_t rotl32(const std::uint32_t x, const int k) {
    return (x << k) | (x >> (32 - k));
}

/* these are in order to avoid gcc warnings about 'strict aliasing rules' */
static inline std::uint32_t extract_32bits_from64_left(const std::uint64_t x)
{
    std::uint32_t out;
    std::memcpy(&out, reinterpret_cast<const std::uint32_t*>(&x), sizeof(std::uint32_t));
    return out;
}

static inline std::uint32_t extract_32bits_from64_right(const std::uint64_t x)
{
    std::uint32_t out;
    std::memcpy(&out, reinterpret_cast<const std::uint32_t*>(&x) + 1, sizeof(std::uint32_t));
    return out;
}

static inline void assign_32bits_to64_left(std::uint64_t assign_to, const std::uint32_t take_from)
{
    std::memcpy(reinterpret_cast<std::uint32_t*>(&assign_to), &take_from, sizeof(std::uint32_t));
}

static inline void assign_32bits_to64_right(std::uint64_t assign_to, const std::uint32_t take_from)
{
    std::memcpy(reinterpret_cast<std::uint32_t*>(&assign_to) + 1, &take_from, sizeof(std::uint32_t));
}

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and 
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state. */
static inline std::uint64_t splitmix64(const std::uint64_t seed)
{
    std::uint64_t z = (seed + 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

/* This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
   It has excellent (sub-ns) speed, a state (256 bits) that is large
   enough for any parallel application, and it passes all tests we are
   aware of.

   For generating just floating-point numbers, xoshiro256+ is even faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
class Xoshiro256PP
{
public:
    using result_type = std::uint64_t;
    std::uint64_t state[4] = {0x3d23dce41c588f8c, 0x10c770bb8da027b0, 0xc7a4c5e87c63ba25, 0xa830f83239465a2e};

    constexpr result_type min() const
    {
        return 0;
    }

    constexpr result_type max() const
    {
        return UINT64_MAX;
    }

    Xoshiro256PP() = default;

    void seed(const std::uint64_t seed)
    {
        this->state[0] = splitmix64(splitmix64(seed));
        this->state[1] = splitmix64(this->state[0]);
        this->state[2] = splitmix64(this->state[1]);
        this->state[3] = splitmix64(this->state[2]);
    }

    void seed(const std::uint64_t seed[4])
    {
        std::memcpy(this->state, seed, 4*sizeof(std::uint64_t));
    }

    template<class Sseq>
    void seed(Sseq& seq)
    {
        seq.generate(reinterpret_cast<std::uint32_t*>(&this->state[0]),
                     reinterpret_cast<std::uint32_t*>(&this->state[0] + 4));
    }

    Xoshiro256PP(const std::uint64_t seed)
    {
        this->seed(seed);
    }

    Xoshiro256PP(const std::uint64_t seed[4])
    {
        this->seed(seed);
    }

    template<class Sseq>
    Xoshiro256PP(Sseq& seq)
    {
        this->seed(seq);
    }

    result_type operator()()
    {
        const std::uint64_t result = rotl64(this->state[0] + this->state[3], 23) + this->state[0];
        const std::uint64_t t = this->state[1] << 17;
        this->state[2] ^= this->state[0];
        this->state[3] ^= this->state[1];
        this->state[1] ^= this->state[2];
        this->state[0] ^= this->state[3];
        this->state[2] ^= t;
        this->state[3] = rotl64(this->state[3], 45);
        return result;
    }

    void discard(unsigned long long z)
    {
        for (unsigned long long ix = 0; ix < z; ix++)
            this->operator()();
    }

    bool operator==(const Xoshiro256PP &rhs)
    {
        return std::memcmp(this->state, rhs.state, 4*sizeof(std::uint64_t));
    }

    #ifndef HAS_CPP20
    bool operator!=(const Xoshiro256PP &rhs)
    {
        return !std::memcmp(this->state, rhs.state, 4*sizeof(std::uint64_t));
    }
    #endif

    template< class CharT, class Traits >
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& ost, const Xoshiro256PP& e)
    {
        ost.write(reinterpret_cast<const char*>(&e.state[0]), sizeof(std::uint64_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[1]), sizeof(std::uint64_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[2]), sizeof(std::uint64_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[3]), sizeof(std::uint64_t));
        return ost;
    }
    template< class CharT, class Traits >
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& ist, Xoshiro256PP& e)
    {
        ist.read(reinterpret_cast<char*>(&e.state[0]), sizeof(std::uint64_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[1]), sizeof(std::uint64_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[2]), sizeof(std::uint64_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[3]), sizeof(std::uint64_t));
        return ist;
    }
};

/* This is xoshiro128++ 1.0, one of our 32-bit all-purpose, rock-solid
   generators. It has excellent speed, a state size (128 bits) that is
   large enough for mild parallelism, and it passes all tests we are aware
   of.

   For generating just single-precision (i.e., 32-bit) floating-point
   numbers, xoshiro128+ is even faster.

   The state must be seeded so that it is not everywhere zero. */
class Xoshiro128PP
{
public:
    using result_type = std::uint32_t;
    std::uint32_t state[4] = {0x1c588f8c, 0x3d23dce4, 0x8da027b0, 0x10c770bb};

    constexpr result_type min() const
    {
        return 0;
    }

    constexpr result_type max() const
    {
        return UINT32_MAX;
    }

    Xoshiro128PP() = default;


    void seed(const std::uint64_t seed)
    {
        const auto t1 = splitmix64(seed);
        const auto t2 = splitmix64(t1);
        this->state[0] = splitmix64(extract_32bits_from64_left(t1));
        this->state[1] = splitmix64(extract_32bits_from64_right(t1));
        this->state[2] = splitmix64(extract_32bits_from64_left(t2));
        this->state[3] = splitmix64(extract_32bits_from64_right(t2));
    }

    void seed(const std::uint32_t seed)
    {
        std::uint64_t temp;
        assign_32bits_to64_left(temp, seed);
        assign_32bits_to64_right(temp, seed);
        this->seed(temp);
    }

    void seed(const std::uint64_t seed[2])
    {
        std::memcpy(this->state, seed, 4*sizeof(std::uint32_t));
    }

    void seed(const std::uint32_t seed[4])
    {
        std::memcpy(this->state, seed, 4*sizeof(std::uint32_t));
    }

    template<class Sseq>
    void seed(Sseq& seq)
    {
        seq.generate(&this->state[0], &this->state[0] + 4);
    }

    Xoshiro128PP(const std::uint32_t seed)
    {
        this->seed(seed);
    }

    Xoshiro128PP(const std::uint64_t seed)
    {
        this->seed(seed);
    }

    Xoshiro128PP(const std::uint32_t seed[4])
    {
        this->seed(seed);
    }

    Xoshiro128PP(const std::uint64_t seed[2])
    {
        this->seed(seed);
    }

    template<class Sseq>
    Xoshiro128PP(Sseq& seq)
    {
        this->seed(seq);
    }

    result_type operator()()
    {
        const std::uint32_t result = rotl32(this->state[0] + this->state[3], 7) + this->state[0];
        const std::uint32_t t = this->state[1] << 9;
        this->state[2] ^= this->state[0];
        this->state[3] ^= this->state[1];
        this->state[1] ^= this->state[2];
        this->state[0] ^= this->state[3];
        this->state[2] ^= t;
        this->state[3] = rotl32(this->state[3], 11);
        return result;
    }

    void discard(unsigned long long z)
    {
        for (unsigned long long ix = 0; ix < z; ix++)
            this->operator()();
    }

    bool operator==(const Xoshiro128PP &rhs)
    {
        return std::memcmp(this->state, rhs.state, 4*sizeof(std::uint32_t));
    }

    #ifndef HAS_CPP20
    bool operator!=(const Xoshiro128PP &rhs)
    {
        return !std::memcmp(this->state, rhs.state, 4*sizeof(std::uint32_t));
    }
    #endif

    template< class CharT, class Traits >
    friend std::basic_ostream<CharT,Traits>&
    operator<<(std::basic_ostream<CharT,Traits>& ost, const Xoshiro128PP& e)
    {
        ost.write(reinterpret_cast<const char*>(&e.state[0]), sizeof(std::uint32_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[1]), sizeof(std::uint32_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[2]), sizeof(std::uint32_t));
        ost.put(' ');
        ost.write(reinterpret_cast<const char*>(&e.state[3]), sizeof(std::uint32_t));
        return ost;
    }
    template< class CharT, class Traits >
    friend std::basic_istream<CharT,Traits>&
    operator>>(std::basic_istream<CharT,Traits>& ist, Xoshiro128PP& e)
    {
        ist.read(reinterpret_cast<char*>(&e.state[0]), sizeof(std::uint32_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[1]), sizeof(std::uint32_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[2]), sizeof(std::uint32_t));
        ist.get();
        ist.read(reinterpret_cast<char*>(&e.state[3]), sizeof(std::uint32_t));
        return ist;
    }
};

}

#endif

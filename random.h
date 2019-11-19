// Stolen from https://gist.github.com/jrandom/64c8972b438bf8f1d0dd

//
//  Random.h
//

#ifndef Tools_Random_h
#define Tools_Random_h


// ================================================================================ Standard Includes
// Standard Includes
// --------------------------------------------------------------------------------
#include <cstdint>
#include <limits>
#include <random>
#include <utility>


// ================================================================================ Numeric Pairs
// Numeric Pairs
// --------------------------------------------------------------------------------
using int8_pair_t   = std::pair<   int_fast8_t,   int_fast8_t >;
using int16_pair_t  = std::pair<  int_fast16_t,  int_fast16_t >;
using int32_pair_t  = std::pair<  int_fast32_t,  int_fast32_t >;
using int64_pair_t  = std::pair<  int_fast64_t,  int_fast64_t >;

using uint8_pair_t  = std::pair<  uint_fast8_t,  uint_fast8_t >;
using uint16_pair_t = std::pair< uint_fast16_t, uint_fast16_t >;
using uint32_pair_t = std::pair< uint_fast32_t, uint_fast32_t >;
using uint64_pair_t = std::pair< uint_fast64_t, uint_fast64_t >;

using size_t_pair_t = std::pair<   std::size_t,   std::size_t >;

using float_pair_t  = std::pair<         float,         float >;
using double_pair_t = std::pair<        double,        double >;



namespace Random
{
    // ============================================================================ Generator
    // Generator
    // ----------------------------------------------------------------------------
    class Generator
    {
    private:
        // -------------------------------------------------------------------- Engine State
        std::mt19937_64                        _engine;
        std::uniform_real_distribution<float > _float_01_distribution;
        std::uniform_real_distribution<double> _double_01_distribution;
        
        
    public:
        // ==================================================================== Construct / Destruct
        // Construct / Destruct
        // -------------------------------------------------------------------- Construct (seed)
        explicit Generator( uint_fast64_t seed )
        
            : _engine                ( seed       ),
              _float_01_distribution ( 0.0f, 1.0f ),
              _double_01_distribution( 0.0,  1.0  ) {}
        
        // -------------------------------------------------------------------- Construct (random seed)
        Generator()
        
            : Generator(   uint_fast64_t(std::random_device{}()) << 32
                         | uint_fast64_t(std::random_device{}())       ) {}
        
        
        
    public:
        // ==================================================================== Random Number Generation
        // Random Number Generation
        // -------------------------------------------------------------------- Bits
        uint_fast8_t  Bits_8 () { return std::uniform_int_distribution< uint8_t  >(0, std::numeric_limits< uint8_t  >::max())(_engine); }
        uint_fast16_t Bits_16() { return std::uniform_int_distribution< uint16_t >(0, std::numeric_limits< uint16_t >::max())(_engine); }
        uint_fast32_t Bits_32() { return std::uniform_int_distribution< uint32_t >(0, std::numeric_limits< uint32_t >::max())(_engine); }
        uint_fast64_t Bits_64() { return std::uniform_int_distribution< uint64_t >(0, std::numeric_limits< uint64_t >::max())(_engine); }
        
        
        // -------------------------------------------------------------------- Integers
        int_fast8_t   Int_8   ( int8_t      low, int8_t      high ) { return std::uniform_int_distribution< int8_t   >(low, high)(_engine); }
        int_fast16_t  Int_16  ( int16_t     low, int16_t     high ) { return std::uniform_int_distribution< int16_t  >(low, high)(_engine); }
        int_fast32_t  Int_32  ( int32_t     low, int32_t     high ) { return std::uniform_int_distribution< int32_t  >(low, high)(_engine); }
        int_fast64_t  Int_64  ( int64_t     low, int64_t     high ) { return std::uniform_int_distribution< int64_t  >(low, high)(_engine); }
        
        uint_fast8_t  Uint_8  ( uint8_t     low, uint8_t     high ) { return std::uniform_int_distribution< uint8_t  >(low, high)(_engine); }
        uint_fast16_t Uint_16 ( uint16_t    low, uint16_t    high ) { return std::uniform_int_distribution< uint16_t >(low, high)(_engine); }
        uint_fast32_t Uint_32 ( uint32_t    low, uint32_t    high ) { return std::uniform_int_distribution< uint32_t >(low, high)(_engine); }
        uint_fast64_t Uint_64 ( uint64_t    low, uint64_t    high ) { return std::uniform_int_distribution< uint64_t >(low, high)(_engine); }
        
        std::size_t   Size_T  ( std::size_t low, std::size_t high ) { return std::uniform_int_distribution< std::size_t >(low, high)(_engine); }
        
        
        // -------------------------------------------------------------------- Integers (pairs)
        int_fast8_t   Int_8   ( const int8_pair_t   range ) { return Int_8  ( range.first, range.second ); }
        int_fast16_t  Int_16  ( const int16_pair_t  range ) { return Int_16 ( range.first, range.second ); }
        int_fast32_t  Int_32  ( const int32_pair_t  range ) { return Int_32 ( range.first, range.second ); }
        int_fast64_t  Int_64  ( const int64_pair_t  range ) { return Int_64 ( range.first, range.second ); }
        
        uint_fast8_t  Uint_8  ( const uint8_pair_t  range ) { return Uint_8 ( range.first, range.second ); }
        uint_fast16_t Uint_16 ( const uint16_pair_t range ) { return Uint_16( range.first, range.second ); }
        uint_fast32_t Uint_32 ( const uint32_pair_t range ) { return Uint_32( range.first, range.second ); }
        uint_fast64_t Uint_64 ( const uint64_pair_t range ) { return Uint_64( range.first, range.second ); }
        
        std::size_t   Size_T  ( const size_t_pair_t range ) { return Size_T ( range.first, range.second ); }
        
        
        // -------------------------------------------------------------------- Reals
        float  Float_01  ()                          { return _float_01_distribution (_engine); }
        double Double_01 ()                          { return _double_01_distribution(_engine); }
        
        float  Float     ( float  low, float  high ) { return std::uniform_real_distribution<float >(low, high)(_engine); }
        double Double    ( double low, double high ) { return std::uniform_real_distribution<double>(low, high)(_engine); }
        
        
        // -------------------------------------------------------------------- Reals (pairs)
        float  Float  ( const float_pair_t  range ) { return Float ( range.first, range.second ); }
        double Double ( const double_pair_t range ) { return Double( range.first, range.second ); }
        
        // -------------------------------------------------------------------- Overloaded
        int_fast8_t   Number ( int8_t      low, int8_t      high ) { return Int_8 (low, high); }
        int_fast16_t  Number ( int16_t     low, int16_t     high ) { return Int_16(low, high); }
        int_fast32_t  Number ( int32_t     low, int32_t     high ) { return Int_32(low, high); }
        int_fast64_t  Number ( int64_t     low, int64_t     high ) { return Int_64(low, high); }
        
        uint_fast8_t  Number ( uint8_t     low, uint8_t     high ) { return Uint_8 (low, high); }
        uint_fast16_t Number ( uint16_t    low, uint16_t    high ) { return Uint_16(low, high); }
        uint_fast32_t Number ( uint32_t    low, uint32_t    high ) { return Uint_32(low, high); }
        uint_fast64_t Number ( uint64_t    low, uint64_t    high ) { return Uint_64(low, high); }
        
        //std::size_t   Number ( std::size_t low, std::size_t high ) { return Size_T (low, high); }

        float         Number ( float       low, float       high ) { return Float  (low, high); }
        double        Number ( double      low, double      high ) { return Double (low, high); }

        
        // -------------------------------------------------------------------- Templated
        template < typename value_t >
        
        value_t Number( std::pair<value_t, value_t> range ) { return Number(range.first, range.second); }
        
        
        // -------------------------------------------------------------------- Utility
        bool Chance ( float probability ) { return Float_01() < probability; }
    };
}

namespace dhtsim {
	static Random::Generator global_rng;
}
#endif

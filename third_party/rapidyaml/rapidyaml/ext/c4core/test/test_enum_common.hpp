#ifndef _C4_ENUM_COMMON_HPP_
#define _C4_ENUM_COMMON_HPP_

#include <c4/enum.hpp>

typedef enum {
    FOO = 0,
    BAR,
    BAZ,
} MyEnum;

namespace c4 {
template<>
inline const EnumSymbols<MyEnum> esyms<MyEnum>()
{
    static const EnumSymbols<MyEnum>::Sym rs[] =
    {
        {FOO, "FOO"},
        {BAR, "BAR"},
        {BAZ, "BAZ"},
    };
    EnumSymbols<MyEnum> r(rs);
    return r;
}
} // namespace c4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

enum class MyEnumClass {
    FOO = 0,
    BAR,
    BAZ,
};


namespace c4 {
template<>
inline const EnumSymbols<MyEnumClass> esyms<MyEnumClass>()
{
    static const EnumSymbols<MyEnumClass>::Sym rs[] =
    {
        {MyEnumClass::FOO, "MyEnumClass::FOO"},
        {MyEnumClass::BAR, "MyEnumClass::BAR"},
        {MyEnumClass::BAZ, "MyEnumClass::BAZ"},
    };
    EnumSymbols<MyEnumClass> r(rs);
    return r;
}


template<>
inline size_t eoffs_cls<MyEnumClass>()
{
    return 13; // same as strlen("MyEnumClass::")
}
} // namespace c4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

typedef enum {
    BM_NONE = 0,
    BM_FOO = 1 << 0,
    BM_BAR = 1 << 1,
    BM_BAZ = 1 << 2,
    BM_FOO_BAR = BM_FOO|BM_BAR,
    BM_FOO_BAR_BAZ = BM_FOO|BM_BAR|BM_BAZ,
} MyBitmask;

namespace c4 {
template<>
inline const EnumSymbols<MyBitmask> esyms<MyBitmask>()
{
    static const EnumSymbols<MyBitmask>::Sym rs[] =
    {
            {BM_NONE, "BM_NONE"},
            {BM_FOO, "BM_FOO"},
            {BM_BAR, "BM_BAR"},
            {BM_BAZ, "BM_BAZ"},
            {BM_FOO_BAR, "BM_FOO_BAR"},
            {BM_FOO_BAR_BAZ, "BM_FOO_BAR_BAZ"},
    };
    EnumSymbols<MyBitmask> r(rs);
    return r;
}

template<>
inline size_t eoffs_pfx<MyBitmask>()
{
    return 3; // same as strlen("BM_")
}
} // namespace c4



typedef enum {
    // no null value
    BM_KABOOM = 1,
    BM_PAFF = 2,
    BM_PEW = 4,
    BM_POW = 7,
} BmWithoutNull;


namespace c4 {
template<>
inline const c4::EnumSymbols<BmWithoutNull> esyms<BmWithoutNull>()
{
    static const EnumSymbols<BmWithoutNull>::Sym rs[] =
    {
        {BM_KABOOM, "KABOOM"},
        {BM_PAFF  , "PAFF"},
        {BM_PEW   , "PEW"},
        {BM_POW   , "POW"},
    };
    EnumSymbols<BmWithoutNull> r(rs);
    return r;
}

template<>
inline size_t eoffs_pfx<BmWithoutNull>()
{
    return 3; // same as strlen("BM_")
}
} // namespace c4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

enum class MyBitmaskClass {
    BM_NONE = 0,
    BM_FOO = 1 << 0,
    BM_BAR = 1 << 1,
    BM_BAZ = 1 << 2,
    BM_FOO_BAR = BM_FOO|BM_BAR,
    BM_FOO_BAR_BAZ = BM_FOO|BM_BAR|BM_BAZ,
};

namespace c4 {

template<>
inline const EnumSymbols<MyBitmaskClass> esyms<MyBitmaskClass>()
{
    static const EnumSymbols<MyBitmaskClass>::Sym rs[] =
    {
        {MyBitmaskClass::BM_NONE,        "MyBitmaskClass::BM_NONE"},
        {MyBitmaskClass::BM_FOO,         "MyBitmaskClass::BM_FOO"},
        {MyBitmaskClass::BM_BAR,         "MyBitmaskClass::BM_BAR"},
        {MyBitmaskClass::BM_BAZ,         "MyBitmaskClass::BM_BAZ"},
        {MyBitmaskClass::BM_FOO_BAR,     "MyBitmaskClass::BM_FOO_BAR"},
        {MyBitmaskClass::BM_FOO_BAR_BAZ, "MyBitmaskClass::BM_FOO_BAR_BAZ"},
    };
    EnumSymbols<MyBitmaskClass> r(rs);
    return r;
}

template<> inline size_t eoffs_cls< MyBitmaskClass >()
{
    return 16; // same as strlen("MyBitmaskClass::")
}
template<> inline size_t eoffs_pfx< MyBitmaskClass >()
{
    return 19; // same as strlen("MyBitmaskClass::BM_")
}

} // namespace c4


enum class BmClassWithoutNull {
    // no null value
    BM_KABOOM = 1,
    BM_PAFF = 2,
    BM_PEW = 4,
    BM_POW = 7,
};


namespace c4 {
template<>
inline const c4::EnumSymbols<BmClassWithoutNull> esyms<BmClassWithoutNull>()
{
    static const EnumSymbols<BmClassWithoutNull>::Sym rs[] =
    {
        {BmClassWithoutNull::BM_KABOOM, "BmClassWithoutNull::BM_KABOOM"},
        {BmClassWithoutNull::BM_PAFF  , "BmClassWithoutNull::BM_PAFF"},
        {BmClassWithoutNull::BM_PEW   , "BmClassWithoutNull::BM_PEW"},
        {BmClassWithoutNull::BM_POW   , "BmClassWithoutNull::BM_POW"},
    };
    EnumSymbols<BmClassWithoutNull> r(rs);
    return r;
}

template<> inline size_t eoffs_cls<BmClassWithoutNull>()
{
    return strlen("BmClassWithoutNull::");
}
template<> inline size_t eoffs_pfx<BmClassWithoutNull>()
{
    return strlen("BmClassWithoutNull::BM_");
}
} // namespace c4


#endif /* _C4_ENUM_COMMON_HPP_ */

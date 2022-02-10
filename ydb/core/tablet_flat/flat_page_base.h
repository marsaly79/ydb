#pragma once

#include <util/generic/ptr.h>
#include <iterator>
#include "flat_update_op.h"
#include "flat_part_scheme.h"
#include "flat_row_nulls.h"
#include "util_deref.h" 

namespace NKikimr {
namespace NTable {
namespace NPage {

using TRecIdx = ui32; // context: page

template <class T> struct TPgSizeOf { static constexpr TPgSize Value = sizeof(T); };
struct TEmpty { };
template <> struct TPgSizeOf<TEmpty> { static constexpr TPgSize Value = 0; };

template <class T>
inline void* NextPtr(T* t) {
    return (char*)t + TPgSizeOf<T>::Value;
}

template <class T>
inline const void* NextPtr(const T* t) {
    return (const char*)t + TPgSizeOf<T>::Value;
}


template <typename TPage, typename TRecord>
class TPageIterator {
public:
    typedef std::random_access_iterator_tag  iterator_category;
    typedef std::ptrdiff_t                   difference_type;
    typedef TRecord          value_type;
    typedef TRecord const*   pointer;
    typedef TRecord const&   reference;

    using TIterator = TPageIterator<TPage, TRecord>;

    TPageIterator() = default;

    TPageIterator(const TPage* page, TRecIdx on, TRecIdx end)
        : Page(page)
        , Upper(end)
        , Offset(Min(on, end))
    {
        Sync();
    }

    TRecIdx Off() const noexcept
    {
        return Offset;
    }

    TRecIdx End() const noexcept 
    { 
        return Upper; 
    } 
 
    const TRecord* GetRecord() const noexcept 
    { 
        return Record; 
    } 
 
    bool operator==(const TIterator& o) const {
        return Offset == o.Offset;
    }

    bool operator!=(const TIterator& o) const {
        return Offset != o.Offset;
    }

    bool operator<(const TIterator& rhs) const { 
        return Offset < rhs.Offset; 
    } 
 
    bool operator<=(const TIterator& rhs) const { 
        return Offset <= rhs.Offset; 
    } 
 
    bool operator>=(const TIterator& rhs) const { 
        return Offset >= rhs.Offset; 
    } 
 
    bool operator>(const TIterator& rhs) const { 
        return Offset > rhs.Offset; 
    } 
 
    reference operator*() const {
        return *Record;
    }

    pointer operator->() const {
        return Record;
    }

    TIterator& operator++() {
        return *this += 1;
    }

    TIterator& operator--() {
        return *this -= 1;
    }

    TIterator operator++(int) { 
        TIterator copy(*this); 
        *this += 1; 
        return copy; 
    } 
 
    TIterator operator--(int) { 
        TIterator copy(*this); 
        *this -= 1; 
        return copy; 
    } 
 
    TIterator& operator+=(size_t n) {
        Offset += Min(TRecIdx(n), Upper - Offset);
        Sync();
        return *this;
    }

    TIterator& operator-=(size_t n) {
        Offset -= Min(TRecIdx(n), Offset);
        Sync();
        return *this;
    }

    TIterator operator+(size_t n) const {
        TIterator ret(*this);
        return ret += n;
    }

    TIterator operator-(size_t n) const {
        TIterator ret(*this);
        return ret -= n;
    }

    difference_type operator-(const TIterator& rhs) const {
        return (difference_type)Offset - (difference_type)rhs.Offset;
    }

    explicit operator bool() const noexcept {
        return Offset < Upper;
    }

private:
    void Sync() {
        Record = *this ? Page->Record(Offset) : nullptr;
    }

private:
    const TPage * Page = nullptr;
    pointer Record = nullptr;
    TRecIdx Upper = 0;
    TRecIdx Offset = 0;
};

#pragma pack(push,1)

struct TDataRef {
    TPgSize Offset;
    TPgSize Size;
} Y_PACKED;

struct TRecordsHeader {
    TRecIdx Records;
} Y_PACKED;

struct TRecordsEntry { 
    TPgSize Offset; 
} Y_PACKED;

template <class TRecord, class TItem>
struct TDataPageRecord {
    void* Base() {
        return NextPtr((TRecord*)this);
    }

    const void* Base() const {
        return NextPtr((const TRecord*)this);
    }

    TItem* GetItem(const TPartScheme::TColumn& info) {
        return TDeref<TItem>::At(Base(), info.Offset); 
    }

    const TItem* GetItem(const TPartScheme::TColumn& info) const {
        return TDeref<TItem>::At(Base(), info.Offset); 
    }

    template <class T>
    const T* GetTail(const TPartScheme::TGroupInfo& group) const { 
        return TDeref<T>::At(Base(), group.FixedSize); 
    } 
 
    template <class T> 
    T* GetFixed(TItem* item) const {
        return reinterpret_cast<T*>(NextPtr(item));
    }

    template <class T>
    const T* GetFixed(const TItem* item) const {
        return reinterpret_cast<const T*>(NextPtr(item));
    }

    TCellOp GetOp(const TPartScheme::TColumn &info) const
    {
        return GetItem(info)->GetOp(info.IsKey());
    }

    TCell Cell(const TPartScheme::TColumn& info) const
    {
        const TItem* item = GetItem(info);
        const auto op = item->GetOp(info.IsKey());

        if (op == ECellOp::Empty || op == ECellOp::Null || op == ECellOp::Reset) {
            return { };
        } else if (info.IsFixed) {
            return { GetFixed<const char>(item), info.FixedSize };
        } else if (op == ELargeObj::Inline) {
            auto *ref = GetFixed<TDataRef>(item);

            return { TDeref<const char>::At(Base(), ref->Offset), ref->Size }; 
        } else { /* ELargeObj::Extern or other link to attached blob */
            return { GetFixed<const char>(item), sizeof(ui64) };

            static_assert(sizeof(TDataRef) == sizeof(ui64), "");
        }
    }
} Y_PACKED;

#pragma pack(pop)

static_assert(sizeof(TDataRef) == 8, "Invalid TDataRef size"); 
static_assert(sizeof(TRecordsHeader) == 4, "Invalid TRecordsHeader size"); 
static_assert(sizeof(TRecordsEntry) == 4, "Invalid TRecordsEntry size"); 
 
template <typename TRecord> 
struct TBlockWithRecords { 
    using TSelf = TBlockWithRecords<TRecord>; 
    using TIterator = TPageIterator<TSelf, TRecord>; 
 
    const TRecord* Record(TRecIdx idx) const noexcept 
    { 
        return TDeref<TRecord>::At(Base, Array[idx].Offset); 
    } 
 
    TIterator Begin() const noexcept 
    { 
        return TIterator(this, 0, Records); 
    } 
 
    TIterator End() const noexcept 
    { 
        return TIterator(this, Records, Records); 
    } 
 
    const void *Base  = nullptr; 
    const TRecordsEntry *Array = nullptr; 
    ui32 Records = 0; 
}; 
 
using TCells = TArrayRef<const TCell>;

template <typename TRecord>
struct TCompare {
    using TColumns = TArrayRef<const TPartScheme::TColumn>;

    TCompare(TColumns keys, const TKeyNulls &nulls) 
        : Info(keys)
        , Nulls(nulls)
    {
        Y_VERIFY(Nulls->size() >= Info.size());
    }

    bool operator()(const TRecord &record, TCells key) const noexcept
    {
        return Compare(record, key) < 0;
    }

    bool operator()(TCells key, const TRecord &record) const noexcept
    {
        return Compare(record, key) > 0;
    }

    int Compare(const TRecord &rec, const TCells key) const noexcept
    {
        for (TPos it = 0; it < Min(key.size(), Nulls->size()); it++) {
            const TCell left = it < Info.size() ? rec.Cell(Info[it]) : Nulls[it];

            if (int cmp = CompareTypedCells(left, key[it], Nulls.Types[it]))
                return cmp;
        }

        return  key.size() < Nulls->size() ? -1 : 0;
    }

private:
    const TColumns Info;
    const TKeyNulls &Nulls; 
};


}
}
}

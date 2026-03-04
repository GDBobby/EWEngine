#pragma once

namespace Reflect{
    template <typename Struct>
    concept IsStruct = std::is_class_v<Struct>;

    struct StructMember{
        std::string_view name;
        std::size_t size;
        std::size_t offset;
        std::size_t alignment;

        std::size_t bit_size;
        std::size_t bit_offset;
    };

    template<IsStruct S>
    consteval auto BuildStructData() {
        constexpr auto members = std::define_static_array(std::meta::nonstatic_data_members_of(^^E, std::meta::access_context::current()));
        constexpr auto member_count = std::meta::nonstatic_data_members_of(^^E, std::meta::access_context::current()).size();
        std::array<StructMember, member_count> result;
        
        template for (constexpr auto i : std::views::iota(0u, member_count)) {
            result[i] = {
                .name = std::meta::identifier_of(members[i]),
                .size = std::meta::size_of(members[i]),
                .offset = std::meta::offset_of(members[i]).bytes,
                .alignment = std::meta::alignment_of(members[i]),
                .bit_size = std::meta::std::meta::bit_size_of(members[i]),
                .bit_offset = std::meta::offset_of(members[i]).total_bits()
            };
        }

        return result;
    }

    template<IsStruct S>
    struct S{
        
        static constexpr auto members = BuildStructData<S>();
        
        template<std::size_t Index>
        static constexpr auto GetMember(S& s){
            static constexpr members = std::meta::nonstatic_data_members_of(^^S)
            return s.
        }
    };

}
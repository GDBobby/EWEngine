#pragma once

#include <meta>

namespace Reflection{
  static constexpr std::string_view null_str_v{"nullstr"};

  template<std::meta::info T>
  consteval std::string_view AttemptToGetName(){
    if constexpr(requires { std::meta::identifier_of(T);}){
      return std::meta::identifier_of(T);
    }
    else{
      return null_str_v;
    }
  }

  template<std::meta::info T>
  consteval std::string_view SafeAttemptName(){
    try{
      return AttemptToGetName<T>();
    }
    catch(std::meta::exception& e) {
      //returning e.what() doesn't work
      //exception currently thrown on - T == (^^operator new) and every other new/delete variation
      //return e.what(); 
      return null_str_v;
    }
  }

  static constexpr std::meta::info invalid_handle = {}; 

  template <auto MetaFunc, std::meta::info T>
  consteval std::size_t Safe_Reflect_Size() {
      try {
          return MetaFunc(T, std::meta::access_context::current()).size();
      } catch (...) {
          return 0;
      }
  }

  template <auto MetaFunc, std::meta::info T>
  consteval std::size_t Safe_Reflect_Size_NoContext() {
      try {
          return MetaFunc(T).size();
      } catch (...) {
          return 0;
      }
  }

  template<auto MetaFunc, std::meta::info T, std::size_t Index>
  consteval auto GetReflectedInfo() {
    static_assert(Index < MetaFunc(T, std::meta::access_context::current()).size()); //is this even necessary? it'll be an error regardless

    return MetaFunc(T, std::meta::access_context::current())[Index];
  }

  template<std::meta::info T = invalid_handle>
  struct Properties{
    
  };


#define META_MACRO(name) static constexpr auto name = std::meta::name(T)

  template<std::meta::info T = invalid_handle>
  struct ReflectedInfo{

    consteval ReflectedInfo(){}

    //i suspect that a user could use reflection info to reflect on functions in std::meta to create this struct, 
    //and it would even update itself along with std::meta
    //but i havent seen how to do that so im gonna hand type it

    //theres also other considerations. is_noexcept isn't gonna be useful for non-function types, is_enumerable won't be valuable for functions, and so on
    //it would make sense to put it in a specialized struct
    //and i think that would be a bit more difficult, or at least meta-programming heavy if i just generate this from ALL funcs

    //static constexpr std::string_view type_name = std::meta::display_string_of(T);

    //using Type = std::meta::extract<T>();

    static constexpr std::string_view name = SafeAttemptName<T>();

    static constexpr auto source_location = std::meta::source_location_of(T);
    //struct Properties{
    META_MACRO(is_public);
    META_MACRO(is_protected);
    META_MACRO(is_private);
    META_MACRO(is_virtual);
    META_MACRO(is_pure_virtual);
    META_MACRO(is_override);
    META_MACRO(is_final);
    META_MACRO(is_deleted);
    META_MACRO(is_defaulted);
    META_MACRO(is_explicit);
    META_MACRO(is_noexcept);
    META_MACRO(is_bit_field);
    META_MACRO(is_enumerator);
    META_MACRO(is_const);
    META_MACRO(is_volatile);
    META_MACRO(is_mutable_member);
    META_MACRO(is_lvalue_reference_qualified);
    META_MACRO(is_rvalue_reference_qualified);
    META_MACRO(has_static_storage_duration);
    META_MACRO(has_thread_storage_duration);
    META_MACRO(has_automatic_storage_duration);
    META_MACRO(has_internal_linkage);
    META_MACRO(has_module_linkage);
    META_MACRO(has_external_linkage);
    META_MACRO(has_linkage);
    META_MACRO(is_class_member);
    META_MACRO(is_namespace_member);
    META_MACRO(is_nonstatic_data_member);
    META_MACRO(is_static_member);
    META_MACRO(is_base);
    META_MACRO(is_data_member_spec);
    META_MACRO(is_namespace);
    META_MACRO(is_function);
    META_MACRO(is_variable);
    META_MACRO(is_type);
    META_MACRO(is_type_alias);
    META_MACRO(is_namespace_alias);
    META_MACRO(is_complete_type);
    META_MACRO(is_enumerable_type);
    META_MACRO(is_template);
    META_MACRO(is_function_template);
    META_MACRO(is_variable_template);
    META_MACRO(is_class_template);
    META_MACRO(is_alias_template);
    META_MACRO(is_conversion_function_template);
    META_MACRO(is_operator_function_template);
    META_MACRO(is_literal_operator_template);
    META_MACRO(is_constructor_template);
    META_MACRO(is_concept);
    META_MACRO(is_structured_binding);
    META_MACRO(is_value);
    META_MACRO(is_object);
    META_MACRO(has_template_arguments);
    META_MACRO(has_default_member_initializer);

    META_MACRO(is_special_member_function);
    META_MACRO(is_conversion_function);
    META_MACRO(is_operator_function);
    META_MACRO(is_literal_operator);
    META_MACRO(is_constructor);
    META_MACRO(is_default_constructor);
    META_MACRO(is_copy_constructor);
    META_MACRO(is_move_constructor);
    META_MACRO(is_assignment);
    META_MACRO(is_copy_assignment);
    META_MACRO(is_move_assignment);
    META_MACRO(is_destructor);
    META_MACRO(is_user_provided);
    META_MACRO(is_user_declared);
    //};
    //static constexpr Properties properties;

    static constexpr auto template_arguments_count = Safe_Reflect_Size_NoContext<std::meta::template_arguments_of, T>();
    static constexpr auto enumerators_count = Safe_Reflect_Size_NoContext<std::meta::enumerators_of, T>();

    static constexpr auto members_count = Safe_Reflect_Size<std::meta::members_of, T>();

    static constexpr auto static_members_count = Safe_Reflect_Size<std::meta::static_data_members_of, T>();
    static constexpr auto nonstatic_members_count = Safe_Reflect_Size<std::meta::nonstatic_data_members_of, T>();


    static constexpr auto bases_count = Safe_Reflect_Size<std::meta::bases_of, T>();


  };

  template<>
  struct ReflectedInfo<invalid_handle> {

    static constexpr std::string_view name = "invalid handle";

    consteval ReflectedInfo() {}
  };


#undef META_MACRO
} //namespace Reflection
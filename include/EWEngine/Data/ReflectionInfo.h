#pragma once

#include <meta>

namespace Reflection{



  template <typename E>
  requires std::is_enum_v<E>
  constexpr std::string_view enum_to_string(E value) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
      if (value == [:e:]) {
        return std::meta::identifier_of(e);
      }
    }

    return "<unnamed>";
  }

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

  template<auto MetaFunc, std::meta::info T>
  consteval auto Safe_Enumerate_Members(){

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

  enum class FunctionType{
    None =              0,
    Basic =             1 << 0,
    Template =          1 << 2,
    ConversionTemplate =1 << 3,
    OperatorTemplate =  1 << 4,
    SpecialMember =     1 << 5,
    Conversion =        1 << 6,
    Operator =          1 << 7
  };

  enum class TemplateType{
    None,
    Basic,
    Function,
    Variable,
    Class,
    Alias,
  };

  enum class AccessLevel{
    NA,
    Public,
    Protected,
    Private,
  };
  enum class Polymorphism{
    None,
    Virtual,
    PureVirtual,
    Override,
    Final
  };
  enum class FunctionModifiers{
    None,
    Deleted,
    Defaulted,
    Explicit,
    Noexcept,
    Const,
    Volatile,
  };
  enum class MemberFunctionInfo{
    None,
    Constructor,
    DefaultConstructor,
    CopyConstructor,
    MoveConstructor,
    Assignment,
    CopyAssignment,
    MoveAssignment,
    Destructor
  };

  enum class MetaType{ //potentially multiple of these?
    Invalid,
    Namespace,
    Object,
    Type,
    CompleteType,
    Enumerator,
    Concept,
    Function,
    Value,
    Variable,
    Class,
    CompleteClass,
    ClassMember,
  };

  enum class Linkage{
    None,
    Linked,
    Internal,
    Module,
    Static
  };

  enum class StorageDuration{
    Automatic,
    Static,
    Thread,
  };

  enum class MemberType{
    NoParent,
    Mutable,
    Class,
    Namespace,
    NonStaticData,
    Static,


    Invalid
  };

  template<std::meta::info T>
  consteval MemberType GetMemberType(){
    if constexpr(std::meta::has_parent(T)){
      if constexpr(std::meta::is_mutable_member(T)){
        return MemberType::Mutable;
      }
      else if constexpr(std::meta::is_class_member(T)){
        return MemberType::Class;
      }
      else if constexpr(std::meta::is_static_member(T)){
        return MemberType::Static;
      }
      else if constexpr(std::meta::is_nonstatic_data_member(T)){
        return MemberType::NonStaticData;
      }
      else if constexpr(std::meta::is_namespace_member(T)){
        return MemberType::Namespace;
      }
      else{
        return MemberType::Invalid;
      }
    }
    else{
      return MemberType::NoParent;
    }
  }

  template<std::meta::info T>
  consteval MetaType GetMetaType(){
    if constexpr(std::meta::is_namespace(T)){
      return MetaType::Namespace;
    }
    else if constexpr(std::meta::is_object(T)){
      return MetaType::Object;
    }
    else if constexpr(std::meta::is_enumerator(T)){
      return MetaType::Enumerator;
    }
    else if constexpr(std::meta::is_concept(T)){
      return MetaType::Concept;
    }
    else if constexpr(std::meta::is_function(T)){
      return MetaType::Function;
    }
    else if constexpr(std::meta::is_value(T)){
      return MetaType::Value;
    }
    else if constexpr(std::meta::is_variable(T)){
      return MetaType::Variable;
    }
    else if constexpr(std::meta::is_complete_type(T)){
      if constexpr(std::meta::is_class_type(T)){
        return MetaType::CompleteClass;
      }
      else{
        return MetaType::CompleteType;
      }
    }
    else if constexpr(std::meta::is_type(T)){
      if constexpr(std::meta::is_class_type(T)){
        return MetaType::Class;
      }
      else{
        return MetaType::Type;
      }
    }
    else if constexpr(std::meta::is_class_member(T)){
      return MetaType::ClassMember;
    }
    else{
      return MetaType::Invalid;
    }
  }


  template<std::meta::info T>
  consteval TemplateType GetTemplateType(){
    if constexpr(std::meta::is_template(T)){
      if constexpr(std::meta::is_function_template(T)){
        return TemplateType::Function;
      }
      else if constexpr(std::meta::is_variable_template(T)){
        return TemplateType::Variable;
      }
      else if constexpr(std::meta::is_class_template(T)){
        return TemplateType::Class;
      }
      else if constexpr(std::meta::is_alias_template(T)){
        return TemplateType::Alias;
      }
      else{
        return TemplateType::Basic;
      }
    }
    else{
      return TemplateType::None;
    }
  }

  template<std::meta::info T>
  consteval FunctionType GetFunctionType(){
    if constexpr (std::meta::is_function(T)){
      return FunctionType::Basic;
    }
    else if constexpr(std::meta::is_function_template(T)){
      return FunctionType::Template;
    }
    else if constexpr(std::meta::is_conversion_function_template(T)) {
      return FunctionType::ConversionTemplate;
    }
    else if constexpr(std::meta::is_operator_function_template(T)){
      return FunctionType::OperatorTemplate;
    }
    else if constexpr(std::meta::is_special_member_function(T)){
      return FunctionType::SpecialMember;
    }
    else if constexpr(std::meta::is_conversion_function(T)){
      return FunctionType::Conversion;
    }
    else if constexpr(std::meta::is_operator_function(T)){
      return FunctionType::Operator;
    }
    else{
      return FunctionType::None;
    }
  }

  template<std::meta::info T>
  struct Properties{ //this struct is currently getting symbol mangled (hopefully a compiler bug and not a permanent issue)
    consteval Properties(){} 

    static constexpr std::string_view name = SafeAttemptName<T>();
    static constexpr MetaType meta_type = GetMetaType<T>();
    static constexpr FunctionType function_type = GetFunctionType<T>();
    static constexpr TemplateType template_type = GetTemplateType<T>();

    static constexpr bool is_value = std::meta::is_value(T);
    static constexpr bool meta_enumerable = ((meta_type == MetaType::CompleteClass)) || (meta_type == MetaType::Namespace);
    static constexpr bool is_type = std::meta::is_type(T);
    static constexpr bool is_object = std::meta::is_object(T);

    static consteval bool GetIfEnum(){
        //the splice [::] pops off too early, and throws an error.
        //static constexpr bool is_an_enum = (meta_type == Reflection::MetaType::CompleteType) && (std::is_enum_v<typename[:member:]>);
        
        //the splice pops off too early here as well
        //static constexpr bool is_an_enum = std::conditional_t<
        //        meta_type == Reflection::MetaType::CompleteType, //condition
        //        std::is_enum<typename[:member:]>, //is true
        //            
        //        std::false_type //if false
        //    >::value;

        if constexpr(meta_type == Reflection::MetaType::CompleteType){
            if constexpr(std::is_enum_v<typename[:T:]>){
                return true;
            }
        }
        return false;
    }
    static constexpr bool is_enum = GetIfEnum();
  };

  template<std::meta::info T>
  struct ReflectedInfo{
    using Props = Properties<T>;

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

    template<auto MetaFunc, bool access_context, bool conditional>
    static consteval auto GetMetaSpan(){
      if constexpr(conditional) {
        if constexpr (access_context){
          return std::span{
            std::define_static_array(MetaFunc(T, std::meta::access_context::current())).data(),
            MetaFunc(T, std::meta::access_context::current()).size()
          };
        }
        else{
          return std::span{
            std::define_static_array(MetaFunc(T)).data(),
            MetaFunc(T).size()
          };
        }
      }
      else{
        return std::span<const std::meta::info, 0>{};
        
      }
    }

    static consteval auto GetTempEmptySpan(){
        return std::span<const std::meta::info, 0>{};
    }

    //these aren't callable during runtime, consteval only
    static constexpr auto template_args = GetMetaSpan<
        std::meta::template_arguments_of, false, 
        (Props::template_type != TemplateType::None) 
        && ((Props::meta_type == MetaType::CompleteType) || (Props::meta_type == MetaType::CompleteClass))>();
    static constexpr auto enumerators = GetMetaSpan<std::meta::enumerators_of, false, Props::is_enum>();
    static constexpr auto members = GetMetaSpan<std::meta::members_of, true, Props::meta_enumerable>();
    static constexpr auto static_members = GetMetaSpan<std::meta::static_data_members_of, true, Props::meta_type == MetaType::CompleteClass>();
    static constexpr auto nonstatic_members = GetMetaSpan<std::meta::nonstatic_data_members_of, true, Props::meta_type == MetaType::CompleteClass>();
    static constexpr auto bases = GetMetaSpan<std::meta::bases_of, true, Props::meta_type == MetaType::CompleteClass>();

    //by separating the size from the span, these are callable during runtime
    static constexpr std::size_t template_arg_count = template_args.size();
    static constexpr std::size_t enumerator_count = enumerators.size();//Safe_Reflect_Size_NoContext<std::meta::enumerators_of, T>();
    static constexpr std::size_t members_count = members.size();
    static constexpr std::size_t static_member_count = static_members.size();
    static constexpr std::size_t nonstatic_member_count = nonstatic_members.size();
    static constexpr std::size_t base_count = bases.size();

  };

  template<>
  struct ReflectedInfo<invalid_handle> {

    static constexpr std::string_view name = "invalid handle";

    consteval ReflectedInfo() {}
  };


} //namespace Reflection
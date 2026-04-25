#pragma once

#include <string>
#include <vector>

namespace EWE{


namespace Asset{
    template<typename Resource>
    inline constexpr std::span<std::string_view> acceptable_extensions;


} //namespace Asset

// ~~~~~~ forward declares
    //struct Buffer;//not written to file
    //struct Image; //special handling
    //struct ImageView; //not written to file
namespace Command{
    struct InstructionPackage;
    struct ObjectPackage;
    struct PackageRecord;
} //namespace Command
    struct RasterPackage;
    //struct Sampler; //not written to file
    struct DescriptorImageInfo;
    struct SubmissionTask;

    struct Shader;
    struct RenderGraph;


namespace Helper{
    static constexpr std::string_view instpkg_exts[] = {".eip"};
    static constexpr std::string_view objpkg_exts[] = {".eop"};
    static constexpr std::string_view pkgrec_exts[] = {".epr"};
    static constexpr std::string_view rasterpkg_exts[] = {".erp"};
    static constexpr std::string_view dii_exts[] = {".edi"};
    static constexpr std::string_view subtasks_exts[] = {".est"};

    static constexpr std::string_view shader_exts[] = { ".spv" };
    static constexpr std::string_view rg_exts[] = {".erg"};

} //namespace Helper

namespace Asset{

    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<Command::InstructionPackage> = Helper::instpkg_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<Command::ObjectPackage> = Helper::objpkg_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<Command::PackageRecord> = Helper::pkgrec_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<RasterPackage> = Helper::rasterpkg_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<DescriptorImageInfo> = Helper::dii_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<SubmissionTask> = Helper::subtasks_exts;

    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<Shader> = Helper::shader_exts;
    template<> inline constexpr std::span<const std::string_view> 
    acceptable_extensions<RenderGraph> = Helper::rg_exts;


} //namesace Asset

} //namespace EWE
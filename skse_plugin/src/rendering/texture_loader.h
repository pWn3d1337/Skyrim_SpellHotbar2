#pragma once
#include <filesystem>

namespace SpellHotbar::TextureLoader
{
    bool fromFile(const char* filename, REX::W32::ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
    bool fromDDSFile(const std::filesystem::path& path, REX::W32::ID3D11ShaderResourceView** out_srv, int* out_width,
        int* out_height);
}
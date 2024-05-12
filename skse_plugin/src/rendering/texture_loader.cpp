#include "texture_loader.h"
#include "../logger/logger.h"
#include "render_manager.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <d3d11.h>

namespace SpellHotbar::TextureLoader {
    // Simple helper function to load an image into a DX11 texture with common settings
    bool fromFile(const char* filename, REX::W32::ID3D11ShaderResourceView** out_srv, int* out_width,
                             int* out_height) {

        auto* renderer = RE::BSGraphics::Renderer::GetSingleton();
        if (!renderer) {
            logger::error("Cannot find Skyrim Renderer. texture loading failed");
            return false;
        }
        auto& forwarder = renderer->data.forwarder;

        // Load from disk into a raw RGBA buffer
        int image_width = 0;
        int image_height = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
        if (image_data == nullptr) {
            logger::error("stbi_load failed for '{}'", filename);
            return false;
        }

        // Create texture
        REX::W32::D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.width = image_width;
        desc.height = image_height;
        desc.mipLevels = 1;
        desc.arraySize = 1;
        desc.format = REX::W32::DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.sampleDesc.count = 1;
        desc.usage = REX::W32::D3D11_USAGE_DEFAULT;
        desc.bindFlags = REX::W32::D3D11_BIND_SHADER_RESOURCE;
        desc.cpuAccessFlags = 0;

        REX::W32::ID3D11Texture2D* pTexture = nullptr;
        REX::W32::D3D11_SUBRESOURCE_DATA subResource;
        subResource.sysMem = image_data;
        subResource.sysMemPitch = desc.width * 4;
        subResource.sysMemSlicePitch = 0;
        RenderManager::device->CreateTexture2D(&desc, &subResource, &pTexture);

        // Create texture view
        REX::W32::D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.format = REX::W32::DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.viewDimension = REX::W32::D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.texture2D.mipLevels = desc.mipLevels;
        srvDesc.texture2D.mostDetailedMip = 0;
        forwarder->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
        pTexture->Release();

        *out_width = image_width;
        *out_height = image_height;
        stbi_image_free(image_data);

        return true;
    }
}
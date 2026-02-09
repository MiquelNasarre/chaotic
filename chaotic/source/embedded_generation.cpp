#include "embedded_resources.h"

#ifdef _INCLUDE_EMBEDDED_GENERATION
#include "Error/_erDefault.h"
#include <cstdio>
#include <cstdint>

// Stores the blob names as they appear in the enum.
// They will also be used to generate the lists.
const char* blob_names[] =
{
    "BLOB_DEFAULT_ICON",
    "BLOB_BACKGROUND_PS",
    "BLOB_BACKGROUND_VS",
    "BLOB_COLOR_CURVE_VS",
    "BLOB_CUBE_TEXTURE_PS",
    "BLOB_CURVE_VS",
    "BLOB_DYNAMIC_BG_PS",
    "BLOB_DYNAMIC_BG_VS",
    "BLOB_GLOBAL_COLOR_PS",
    "BLOB_GLOBAL_COLOR_VS",
    "BLOB_LIGHT_PS",
    "BLOB_LIGHT_VS",
    "BLOB_OIT_CUBE_TEXTURE_PS",
    "BLOB_OIT_GLOBAL_COLOR_PS",
    "BLOB_OIT_RESOLVE_PS",
    "BLOB_OIT_RESOLVE_VS",
    "BLOB_OIT_UNLIT_CUBE_TEXTURE_PS",
    "BLOB_OIT_UNLIT_GLOBAL_COLOR_PS",
    "BLOB_OIT_UNLIT_VERTEX_COLOR_PS",
    "BLOB_OIT_UNLIT_VERTEX_TEXTURE_PS",
    "BLOB_OIT_VERTEX_COLOR_PS",
    "BLOB_OIT_VERTEX_TEXTURE_PS",
    "BLOB_UNLIT_CUBE_TEXTURE_PS",
    "BLOB_UNLIT_GLOBAL_COLOR_PS",
    "BLOB_UNLIT_VERTEX_COLOR_PS",
    "BLOB_UNLIT_VERTEX_TEXTURE_PS",
    "BLOB_VERTEX_COLOR_PS",
    "BLOB_VERTEX_COLOR_VS",
    "BLOB_VERTEX_TEXTURE_PS",
    "BLOB_VERTEX_TEXTURE_VS",
};

// Stores the filepath of the resources to embed.
const char* blob_files[] =
{
    SOLUTION_DIR "chaotic/resources/Icon.ico",
    SOLUTION_DIR "chaotic/shaders/BackgroundPS.cso",
    SOLUTION_DIR "chaotic/shaders/BackgroundVS.cso",
    SOLUTION_DIR "chaotic/shaders/ColorCurveVS.cso",
    SOLUTION_DIR "chaotic/shaders/CubeTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/CurveVS.cso",
    SOLUTION_DIR "chaotic/shaders/DynamicBgPS.cso",
    SOLUTION_DIR "chaotic/shaders/DynamicBgVS.cso",
    SOLUTION_DIR "chaotic/shaders/GlobalColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/GlobalColorVS.cso",
    SOLUTION_DIR "chaotic/shaders/LightPS.cso",
    SOLUTION_DIR "chaotic/shaders/LightVS.cso",
    SOLUTION_DIR "chaotic/shaders/OITCubeTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/OITGlobalColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/OITresolvePS.cso",
    SOLUTION_DIR "chaotic/shaders/OITresolveVS.cso",
    SOLUTION_DIR "chaotic/shaders/OITUnlitCubeTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/OITUnlitGlobalColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/OITUnlitVertexColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/OITUnlitVertexTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/OITVertexColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/OITVertexTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/UnlitCubeTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/UnlitGlobalColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/UnlitVertexColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/UnlitVertexTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/VertexColorPS.cso",
    SOLUTION_DIR "chaotic/shaders/VertexColorVS.cso",
    SOLUTION_DIR "chaotic/shaders/VertexTexturePS.cso",
    SOLUTION_DIR "chaotic/shaders/VertexTextureVS.cso",
};

// Function used to generate the embedded resources.

void generateEmbedded(const char* out_filename)
{
    FILE* out;
    fopen_s(&out, out_filename, "w");
    USER_CHECK(out,
        "Could not generate file to print embeddings to.\n" 
        "Make sure the specified folder path is valid."
    );

    fprintf(out, "%s",
        R"(#include "embedded_resources.h")" "\n"
        R"(#include <cstdint>)" "\n"
        "\n"
    );

    for (unsigned i = 0u; i < sizeof(blob_files) / sizeof(const char*); i++)
    {
        fprintf(out,
            "alignas(16) static const uint8_t %s[] =\n" 
            "{ "
            , blob_names[i] + 5   // Remove initial "BLOB_" to avoid confusion
        );

        FILE* resource;
        fopen_s(&resource, blob_files[i], "rb");
        USER_CHECK(resource,
            blob_files[i]
        );

        uint8_t byte;
        while (fread(&byte, 1, 1, resource))
            fprintf(out, "0x%02X, ", byte);

        fprintf(out, 
            "};\n"
            "\n"
        );

        fclose(resource);
    }

    fprintf(out, "%s",
        R"(// Returns a pointer to the bytecode data of the blobs.)" "\n"
        "\n"
        R"(const void* getBlobFromId(BLOB_ID id) noexcept)" "\n"
        R"({)" "\n"
        "\t"  R"(switch (id))" "\n"
        "\t"  R"({)" "\n"
    );

    for (unsigned i = 0u; i < sizeof(blob_files) / sizeof(const char*); i++)
        fprintf(out,
            "\t" "case BLOB_ID::%s:" "\n"
            "\t" "\t" "return %s;" "\n"
            "\n"
            ,
            blob_names[i],
            blob_names[i] + 5
        );

    fprintf(out, "%s",

        "\t"  R"(default:)" "\n"
        "\t" "\t"  R"(return nullptr;)" "\n"
        "\t"  R"(};)" "\n"
        R"(})" "\n"
        "\n"
        R"(// Returns the size in bytes of the blob data.)" "\n"
        "\n"
        R"(unsigned long long getBlobSizeFromId(BLOB_ID id) noexcept)" "\n"
        R"({)" "\n"
        "\t"  R"(switch (id))" "\n"
        "\t"  R"({)" "\n"
    );

    for (unsigned i = 0u; i < sizeof(blob_files) / sizeof(const char*); i++)
        fprintf(out,
            "\t" "case BLOB_ID::%s:" "\n"
            "\t" "\t" "return sizeof(%s);" "\n"
            "\n"
            ,
            blob_names[i],
            blob_names[i] + 5
        );

    fprintf(out, "%s",

        "\t"  R"(default:)" "\n"
        "\t" "\t"  R"(return 0;)" "\n"
        "\t"  R"(};)" "\n"
        R"(})" "\n"
    );

    fclose(out);

    USER_ERROR("Embedded resources correctly generated.");
}
#endif
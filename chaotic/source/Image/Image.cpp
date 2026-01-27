#include "Image/Image.h"

#include "Exception/_exDefault.h"

#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*
-------------------------------------------------------------------------------------------------------
 Helper functions to read and write BMPs
-------------------------------------------------------------------------------------------------------
*/

static inline void write_le16(FILE* f, uint16_t v) 
{
    uint8_t b[2] = { (uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF) };
    if (fwrite(b, 1, 2, f) != 2) 
        throw INFO_EXCEPT("Failed to write in file, possible corruption");
}
static inline void write_le32(FILE* f, uint32_t v) 
{
    uint8_t b[4] = {
        (uint8_t)(v & 0xFF),
        (uint8_t)((v >> 8) & 0xFF),
        (uint8_t)((v >> 16) & 0xFF),
        (uint8_t)((v >> 24) & 0xFF)
    };
    if (fwrite(b, 1, 4, f) != 4) throw INFO_EXCEPT("Failed to write in file, possible corruption");
}
static inline uint16_t read_le16(FILE* f) 
{
    uint8_t b[2]; 
    if (fread(b, 1, 2, f) != 2) 
        throw INFO_EXCEPT("Failed to read from file, possible corruption");

    return (uint16_t)(b[0] | (b[1] << 8));
}
static inline uint32_t read_le32(FILE* f) 
{
    uint8_t b[4]; 
    if (fread(b, 1, 4, f) != 4) 
        throw INFO_EXCEPT("Failed to read from file, possible corruption");

    return (uint32_t)(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
}

/*
-------------------------------------------------------------------------------------------------------
 Internal Functions
-------------------------------------------------------------------------------------------------------
*/

// Internal function to format strings for the templates

const char* Image::internal_formatting(const char* fmt_filename, ...)
{
    // Sanity check
    if (!fmt_filename)
        return nullptr;

    // Load args
    va_list ap;

    // Get string
    thread_local static char filename[512];

    // Format
    va_start(ap, fmt_filename);
    if (vsnprintf(filename, 507, fmt_filename, ap) < 0)
        return nullptr;
    va_end(ap);

    // Make sure it ends and return
    filename[507] = '\0';

    return filename;
}

/*
-------------------------------------------------------------------------------------------------------
 Constructors / Destructors
-------------------------------------------------------------------------------------------------------
*/

// Initializes the image as stored in the bitmap file.

Image::Image(const char* filename)
{
    if (!load(filename))
        throw INFO_EXCEPT("Could not create image from file");
}

// Copies the other image

Image::Image(const Image& other)
	:width_(other.width_), height_(other.height_)
{
	pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));
	memcpy(pixels_, other.pixels_, width_ * height_ * sizeof(Color));
}

// Copies the other image

Image& Image::operator=(const Image& other)
{
    if (&other == this) 
        return *this;

    if (pixels_)
        free(pixels_);

    width_ = other.width_;
    height_ = other.height_;
    pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));
    memcpy(pixels_, other.pixels_, width_ * height_ * sizeof(Color));

    return *this;
}

// Stores a copy of the color pointer

Image::Image(Color* pixels, unsigned width, unsigned height)
    :width_{ width }, height_{ height }, pixels_{ (Color*)calloc(width * height, sizeof(Color)) }
{
    memcpy(pixels_, pixels, width * height * sizeof(Color));
}

// Creates an image with the specified size and color

Image::Image(unsigned width, unsigned height, Color color)
{
    reset(width, height, color);
}

// Frees the pixel pointer

Image::~Image()
{
    if (pixels_)
	    free(pixels_);
}

// Resets the image to the new dimensions and color.

void Image::reset(unsigned width, unsigned height, Color color)
{
    // Free pixels if they exist.
    if (pixels_)
        free(pixels_);

    width_ = width;
    height_ = height;

    pixels_ = (Color*)calloc(width_ * height_, sizeof(Color));

    if (color != Color::Transparent)
        for (unsigned int i = 0; i < width_ * height_; i++)
            pixels_[i] = color;
}

/*
-------------------------------------------------------------------------------------------------------
 BMP image files functions
-------------------------------------------------------------------------------------------------------
*/

// Loads an image from the specified file path

bool Image::save(const char* filename_) const
{
    if (!filename_ || !*filename_)
        return 0;

    char filename[512];
    strncpy_s(filename, filename_, 507);
    filename[507] = '\0';

    // Force the correct extension
    char* base = filename;
    if (char* p = strrchr(filename, '/'))  base = p + 1;
    if (char* p = strrchr(filename, '\\')) if (p + 1 > base) base = p + 1;

    char* dot = strrchr(base, '.');
    char* end = dot ? dot : filename + strlen(filename);

    end[0] = '.';
    end[1] = 'b';
    end[2] = 'm';
    end[3] = 'p';
    end[4] = '\0';

    FILE* file = nullptr;
    fopen_s(&file, filename, "wb");
    if (!file) 
        return false;

    const uint16_t bpp = 32u;
    const uint32_t bytes_per_pixel = 4u;

    // BMP rows are aligned to 4 bytes. For 32bpp stride == width*4 already aligned.
    const uint32_t row_stride = width_ * bytes_per_pixel;
    const uint32_t pixel_data_size = row_stride * height_;

    // --- FILE HEADER (14 bytes) ---
    // Signature "BM"
    if (fwrite("BM", sizeof(uint8_t), 2, file) != 2)
    {
        fclose(file);
        return false;
    }

    const uint32_t bfOffBits = 14u + 40u; // file header + BITMAPINFOHEADER
    const uint32_t bfSize = bfOffBits + pixel_data_size;
    write_le32(file, bfSize);
    write_le16(file, 0); // bfReserved1
    write_le16(file, 0); // bfReserved2
    write_le32(file, bfOffBits);

    // --- DIB HEADER: BITMAPINFOHEADER (40 bytes) ---
    write_le32(file, 40u);                 // biSize
    write_le32(file, (uint32_t)width_);    // biWidth
    write_le32(file, (uint32_t)height_);   // biHeight (positive => bottom-up)
    write_le16(file, 1u);                  // biPlanes
    write_le16(file, bpp);                 // biBitCount
    write_le32(file, 0u);                  // biCompression (BI_RGB = 0)
    write_le32(file, pixel_data_size);     // biSizeImage
    write_le32(file, 2835u);               // biXPelsPerMeter (~72 DPI)
    write_le32(file, 2835u);               // biYPelsPerMeter
    write_le32(file, 0u);                  // biClrUsed
    write_le32(file, 0u);                  // biClrImportant

    // --- Pixel data (bottom-up BGR[A]) ---

    uint8_t* row = (uint8_t*)calloc(row_stride, sizeof(uint8_t));
    for (int y = (int)height_ - 1; y >= 0; --y)
    {
        uint8_t* out = row;
        const Color* in = &pixels_[(size_t)y * width_];

            // BGRA
        for (unsigned x = 0; x < width_; ++x)
        {
            *out++ = in[x].B;
            *out++ = in[x].G;
            *out++ = in[x].R;
            *out++ = in[x].A;
        }

        if (fwrite(row, sizeof(uint8_t), row_stride, file) != row_stride)
        {
            free(row);
            fclose(file);
            return false;
        }
    }

    free(row);
    fclose(file);
    return true;
}

// Saves the image to the specified file path

bool Image::load(const char* filename_) 
{
    if (!filename_ || !*filename_)
        return 0;

    char filename[512];
    strncpy_s(filename, filename_, 507);
    filename[507] = '\0';

    // Force the correct extension
    char* base = filename;
    if (char* p = strrchr(filename, '/'))  base = p + 1;
    if (char* p = strrchr(filename, '\\')) if (p + 1 > base) base = p + 1;

    char* dot = strrchr(base, '.');
    char* end = dot ? dot : filename + strlen(filename);

    end[0] = '.';
    end[1] = 'b';
    end[2] = 'm';
    end[3] = 'p';
    end[4] = '\0';

    FILE* file = nullptr;
    fopen_s(&file, filename, "rb");
    if (!file) return false;

    // Check signature
    char sig[2];
    if (fread(sig, 1, 2, file) != 2 || sig[0] != 'B' || sig[1] != 'M')
    {
        fclose(file);
        return false;
    }

    (void)read_le32(file); // bfSize
    (void)read_le16(file); // bfReserved1
    (void)read_le16(file); // bfReserved2
    uint32_t bfOffBits = read_le32(file);

    uint32_t biSize = read_le32(file);
    if (biSize < 40u) 
    { 
        fclose(file); 
        return false; 
    } // we expect BITMAPINFOHEADER or larger

    int32_t biWidth = (int32_t)read_le32(file);
    int32_t biHeight = (int32_t)read_le32(file);
    uint16_t biPlanes = read_le16(file);
    uint16_t biBitCount = read_le16(file);
    uint32_t biCompression = read_le32(file);
    (void)read_le32(file); // biSizeImage
    (void)read_le32(file); // biXPelsPerMeter
    (void)read_le32(file); // biYPelsPerMeter
    (void)read_le32(file); // biClrUsed
    (void)read_le32(file); // biClrImportant

    if (biPlanes != 1 || (biBitCount != 24 && biBitCount != 32) || (biCompression != 0 && biCompression != 3))
    {
        fclose(file);
        return false; // only uncompressed 24/32-bit
    }

    // Some BMPs have extra header bytes; seek to pixel array
    if (fseek(file, (long)bfOffBits, SEEK_SET) != 0) 
    { 
        fclose(file); 
        return false; 
    }

    const bool top_down = (biHeight < 0);
    const uint32_t w = (uint32_t)biWidth;
    const uint32_t h = (uint32_t)(top_down ? -biHeight : biHeight);
    const uint32_t bytes_per_pixel = (biBitCount == 32) ? 4u : 3u;
    const uint32_t row_stride = ((w * bytes_per_pixel + 3u) / 4u) * 4u;

    // allocate
    if (pixels_)
        free(pixels_);

    width_ = w; height_ = h;
    pixels_ = (Color*)calloc(w * h, sizeof(Color));

    uint8_t* row = (uint8_t*)calloc(row_stride, sizeof(uint8_t));
    for (uint32_t y = 0; y < h; ++y)
    {
        uint8_t* in = row;
        if (fread(in, sizeof(uint8_t), row_stride, file) != row_stride) 
        { 
            free(row); 
            fclose(file); 
            return false; 
        }

        uint32_t dst_y = top_down ? y : (h - 1 - y);
        Color* out = &pixels_[dst_y * w];

        if (biBitCount == 32)
            for (uint32_t x = 0; x < w; ++x)
            {
                out[x].B = *(in++);
                out[x].G = *(in++);
                out[x].R = *(in++);
                out[x].A = *(in++);
            }

        else
            for (uint32_t x = 0; x < w; ++x)
            {
                out[x].B = *(in++);
                out[x].G = *(in++);
                out[x].R = *(in++);
                out[x].A = 255;
            }
    }

    free(row);
    fclose(file);
    return true;
}

/*
-------------------------------------------------------------------------------------------------------
 To Cube functions
-------------------------------------------------------------------------------------------------------
*/

// Helper functions, they take spherical coordinates and a formatted image and sample
// the color from the specified coordinates in the specified spherical format.

static inline Color interpolate_color(float u, float v, const Image& image)
{
    // Set the image coordinates to [0,1)x[0,1)
    u -= floorf(u);
    if (v > 0.9999f) v = 0.9999f;
    if (v < 0.f) v = 0.f;

    // Map to image pixels
    float x = u * (image.width() - 1.f);
    float y = v * (image.height() - 1.f);

    // Get neerest pixel coordinates
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Get pixel distances
    float tx = x - (float)x0;
    float ty = y - (float)y0;

    // Get pixel colors
    _float4color c00 = image(y0, x0).getColor4();
    _float4color c10 = image(y0, x1).getColor4();
    _float4color c01 = image(y1, x0).getColor4();
    _float4color c11 = image(y1, x1).getColor4();

    // Interpolation function
    static auto intrp = [](_float4color c0, _float4color c1, float t) {
        return _float4color
        (
            c0.r * (1.f - t) + c1.r * t,
            c0.g * (1.f - t) + c1.g * t,
            c0.b * (1.f - t) + c1.b * t,
            c0.a * (1.f - t) + c1.a * t
        );
    };

    // Interpolate sideways
    _float4color c0 = intrp(c00, c10, tx);
    _float4color c1 = intrp(c01, c11, tx);

    // Interpolate vertically
    return Color(intrp(c0, c1, ty));
}
static inline Color sample_from_equirect(float x, float y, float z, const Image& equirect)
{
    // Normalize the coordinates
    const float norm = sqrtf(x * x + y * y + z * z);
    x /= norm;
    y /= norm;
    z /= norm;

    // Obtain latitude and longitude from the normalized values
    float theta = atan2f(y, x);
    float phi = asinf(z);

    // Divide by 2*PI and add 1/2 to get it in range (0,1)
    float u = theta * 0.1591549430918953f + 0.5f;
    // Divide by PI and subtract from 1/2 to get it in range (0,1)
    float v = 0.5f - phi * 0.3183098861837907f;

    // Interpolate the obtained image coordinates
    return interpolate_color(u, v, equirect);
}
static inline Color sample_from_fisheye(float x, float y, float z, const Image& fisheye, ToCube::FISHEYE_TYPE type)
{
    // Normalize the coordinates
    const float norm = sqrtf(x * x + y * y + z * z);
    x /= norm;
    y /= norm;
    z /= norm;

    // Get the spherical coordinates
    float theta = atan2f(sqrtf(x * x + y * y), z);
    float alpha = atan2f(y, x);

    // Transform the angular distance to radius depending on the type
    float rho;
    switch (type)
    {
    case ToCube::FISHEYE_EQUIDISTANT:
        // Divide by pi
        rho = theta * 0.3183098861837907f;
        break;

    case ToCube::FISHEYE_EQUISOLID:
        // Transform to radius (0,1)
        rho = sinf(theta / 2.f);
        break;

    case ToCube::FISHEYE_STEREOGRAPHIC:
        // Transform to radius (0,1)
        rho = tanf(theta / 2.f) / ToCube::STEREOGRAPHIC_DIV;
        // If out of image bounds return
        if (rho > 1.f) return ToCube::STEREOGRAPHIC_FILL;
        break;

    default:
        return Color::Transparent;
    }

    // Get the circle coordinates in range (0,1)x(0,1)
    float u = 0.5f + rho * cosf(alpha) / 2.f;
    float v = 0.5f + rho * sinf(alpha) / 2.f;

    // Interpolate the obtained image coordinates
    return interpolate_color(u, v, fisheye);
}

// Equirectangular projections are extremely common, they follow the typical
// latitude longitude coordinate system and are widely used for all kinds of
// applications. This function allows you to convert equirectangular projection
// images to texture cubes.

Image* ToCube::from_equirect(const Image& equirect, unsigned cube_width)
{
    if (!equirect.width() || !equirect.height())
        return nullptr;

    Image& cube = *new Image(cube_width, 6u * cube_width);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 0u * cube_width, col) = sample_from_equirect(1.f - 2.f * float(col + 0.5f) / cube_width,  1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, equirect);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 1u * cube_width, col) = sample_from_equirect(2.f * float(col + 0.5f) / cube_width - 1.f, -1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, equirect);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 2u * cube_width, col) = sample_from_equirect(2.f * float(row + 0.5f) / cube_width - 1.f, 2.f * float(col + 0.5f) / cube_width - 1.f,  1.f, equirect);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 3u * cube_width, col) = sample_from_equirect(1.f - 2.f * float(row + 0.5f) / cube_width, 2.f * float(col + 0.5f) / cube_width - 1.f, -1.f, equirect);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 4u * cube_width, col) = sample_from_equirect( 1.f, 2.f * float(col + 0.5f) / cube_width - 1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, equirect);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 5u * cube_width, col) = sample_from_equirect(-1.f, 1.f - 2.f * float(col + 0.5f) / cube_width, 1.f - 2.f * float(row + 0.5f) / cube_width, equirect);

    return &cube;
}

// Fish-eye or circular panoramic pictures are common as a result of 360º cameras
// that usually take this format when taking their pictures. This function allows
// you to convert your fish-eye 360º pictures into texture cubes.

Image* ToCube::from_fisheye(const Image& fisheye, unsigned cube_width, FISHEYE_TYPE type)
{
    if (!fisheye.width() || !fisheye.height())
        return nullptr;

    Image& cube = *new Image(cube_width, 6u * cube_width);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 0u * cube_width, col) = sample_from_fisheye(1.f - 2.f * float(col + 0.5f) / cube_width,  1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, fisheye, type);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 1u * cube_width, col) = sample_from_fisheye(2.f * float(col + 0.5f) / cube_width - 1.f, -1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, fisheye, type);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 2u * cube_width, col) = sample_from_fisheye(2.f * float(row + 0.5f) / cube_width - 1.f, 2.f * float(col + 0.5f) / cube_width - 1.f,  1.f, fisheye, type);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 3u * cube_width, col) = sample_from_fisheye(1.f - 2.f * float(row + 0.5f) / cube_width, 2.f * float(col + 0.5f) / cube_width - 1.f, -1.f, fisheye, type);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 4u * cube_width, col) = sample_from_fisheye( 1.f, 2.f * float(col + 0.5f) / cube_width - 1.f, 1.f - 2.f * float(row + 0.5f) / cube_width, fisheye, type);

    for (unsigned row = 0u; row < cube_width; row++)
        for (unsigned col = 0u; col < cube_width; col++)
            cube(row + 5u * cube_width, col) = sample_from_fisheye(-1.f, 1.f - 2.f * float(col + 0.5f) / cube_width, 1.f - 2.f * float(row + 0.5f) / cube_width, fisheye, type);

    return &cube;
}

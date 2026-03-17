#include "generation/Noise.h"
#include <cmath>
#include <algorithm>

static const uint8_t kPerm[256] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

Noise::Noise(uint32_t seed) {
    for (int i = 0; i < 256; ++i) perm[i] = kPerm[i];

    if (seed != 0) {
        uint32_t s = seed;
        for (int i = 255; i > 0; --i) {
            s = s * 1103515245u + 12345u;
            int j = static_cast<int>((s >> 16) % static_cast<uint32_t>(i + 1));
            std::swap(perm[i], perm[j]);
        }
    }
    for (int i = 0; i < 256; ++i) perm[i + 256] = perm[i];
}

float Noise::fade(float t)                    { return t*t*t*(t*(t*6.0f-15.0f)+10.0f); }
float Noise::lerp(float t, float a, float b)  { return a + t*(b - a); }

float Noise::grad2(int hash, float x, float y) {
    int h = hash & 7;
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float Noise::grad3(int hash, float x, float y, float z) {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float Noise::perlin2D(float x, float y) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    float u = fade(x), v = fade(y);

    int A = perm[X]   + Y;
    int B = perm[X+1] + Y;

    return lerp(v,
        lerp(u, grad2(perm[A],   x,   y),   grad2(perm[B],   x-1, y)),
        lerp(u, grad2(perm[A+1], x,   y-1), grad2(perm[B+1], x-1, y-1)));
}

float Noise::perlin3D(float x, float y, float z) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;
    int Z = static_cast<int>(std::floor(z)) & 255;
    x -= std::floor(x);
    y -= std::floor(y);
    z -= std::floor(z);
    float u = fade(x), v = fade(y), w = fade(z);

    int A  = perm[X]   + Y;
    int AA = perm[A]   + Z;
    int AB = perm[A+1] + Z;
    int B  = perm[X+1] + Y;
    int BA = perm[B]   + Z;
    int BB = perm[B+1] + Z;

    return lerp(w,
        lerp(v,
            lerp(u, grad3(perm[AA],   x,   y,   z),   grad3(perm[BA],   x-1, y,   z)),
            lerp(u, grad3(perm[AB],   x,   y-1, z),   grad3(perm[BB],   x-1, y-1, z))),
        lerp(v,
            lerp(u, grad3(perm[AA+1], x,   y,   z-1), grad3(perm[BA+1], x-1, y,   z-1)),
            lerp(u, grad3(perm[AB+1], x,   y-1, z-1), grad3(perm[BB+1], x-1, y-1, z-1))));
}

float Noise::fbm2D(float x, float y, int oct, float lac, float gain) const {
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < oct; ++i) {
        sum    += perlin2D(x*freq, y*freq) * amp;
        maxVal += amp;
        amp    *= gain;
        freq   *= lac;
    }
    return sum / maxVal;
}

float Noise::fbm3D(float x, float y, float z, int oct, float lac, float gain) const {
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f;
    for (int i = 0; i < oct; ++i) {
        sum    += perlin3D(x*freq, y*freq, z*freq) * amp;
        maxVal += amp;
        amp    *= gain;
        freq   *= lac;
    }
    return sum / maxVal;
}

float Noise::ridgeNoise2D(float x, float y, int oct, float lac, float gain) const {
    float sum = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f, prev = 1.0f;
    for (int i = 0; i < oct; ++i) {
        float n = 1.0f - std::fabs(perlin2D(x*freq, y*freq));
        n *= n;
        n *= prev;
        prev    = n;
        sum    += n * amp;
        maxVal += amp;
        amp    *= gain;
        freq   *= lac;
    }
    return sum / maxVal;
}

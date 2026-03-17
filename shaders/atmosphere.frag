#version 460 core

// Physically-based atmosphere with Rayleigh + Mie scattering.
// Renders sky into empty (far-depth) pixels of the HDR buffer.

in vec2 vUV;

uniform sampler2D uDepth;
uniform mat4  uInvVP;
uniform float uTime;
uniform vec3  uSunDir;
uniform float uDaylight;

out vec4 FragColor;

const float PI = 3.14159265359;

// Atmosphere parameters
const float EARTH_RADIUS       = 6371000.0;
const float ATMO_RADIUS        = 6471000.0; // 100km atmosphere
const vec3  RAYLEIGH_COEFF     = vec3(5.8e-6, 13.5e-6, 33.1e-6); // wavelength-dependent
const float MIE_COEFF          = 21e-6;
const float RAYLEIGH_SCALE_H   = 8500.0;
const float MIE_SCALE_H        = 1200.0;
const float MIE_G              = 0.76;  // anisotropy

const int   VIEW_SAMPLES       = 16;
const int   LIGHT_SAMPLES      = 8;

// Ray-sphere intersection
vec2 raySphere(vec3 ro, vec3 rd, float radius) {
    float b = 2.0 * dot(ro, rd);
    float c = dot(ro, ro) - radius * radius;
    float d = b * b - 4.0 * c;
    if (d < 0.0) return vec2(-1.0);
    d = sqrt(d);
    return vec2((-b - d) * 0.5, (-b + d) * 0.5);
}

vec3 atmosphere(vec3 rd, vec3 sunDir) {
    vec3 ro = vec3(0, EARTH_RADIUS + 200.0, 0); // camera at 200m altitude

    vec2 atmoHit = raySphere(ro, rd, ATMO_RADIUS);
    if (atmoHit.y < 0.0) return vec3(0);

    float tMax = atmoHit.y;
    // Check ground intersection
    vec2 groundHit = raySphere(ro, rd, EARTH_RADIUS);
    if (groundHit.x > 0.0) tMax = groundHit.x;

    float stepSize = tMax / float(VIEW_SAMPLES);
    float t = 0.0;

    vec3 totalRayleigh = vec3(0);
    vec3 totalMie      = vec3(0);
    float opticalDepthR = 0.0;
    float opticalDepthM = 0.0;

    for (int i = 0; i < VIEW_SAMPLES; ++i) {
        vec3 pos = ro + rd * (t + stepSize * 0.5);
        float height = length(pos) - EARTH_RADIUS;

        float hr = exp(-height / RAYLEIGH_SCALE_H) * stepSize;
        float hm = exp(-height / MIE_SCALE_H) * stepSize;
        opticalDepthR += hr;
        opticalDepthM += hm;

        // Light ray toward sun
        vec2 sunHit = raySphere(pos, sunDir, ATMO_RADIUS);
        float sunStepSize = sunHit.y / float(LIGHT_SAMPLES);
        float sunOpticalR = 0.0, sunOpticalM = 0.0;

        bool blocked = false;
        for (int j = 0; j < LIGHT_SAMPLES; ++j) {
            vec3 sunPos = pos + sunDir * ((float(j) + 0.5) * sunStepSize);
            float sunH = length(sunPos) - EARTH_RADIUS;
            if (sunH < 0.0) { blocked = true; break; }
            sunOpticalR += exp(-sunH / RAYLEIGH_SCALE_H) * sunStepSize;
            sunOpticalM += exp(-sunH / MIE_SCALE_H) * sunStepSize;
        }

        if (!blocked) {
            vec3 tau = RAYLEIGH_COEFF * (opticalDepthR + sunOpticalR) +
                       MIE_COEFF * (opticalDepthM + sunOpticalM);
            vec3 attenuation = exp(-tau);
            totalRayleigh += attenuation * hr;
            totalMie      += attenuation * hm;
        }

        t += stepSize;
    }

    // Phase functions
    float cosTheta = dot(rd, sunDir);
    float rayleighPhase = 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
    float g = MIE_G;
    float miePhase = 3.0 / (8.0 * PI) * ((1.0 - g*g) * (1.0 + cosTheta*cosTheta)) /
                     ((2.0 + g*g) * pow(1.0 + g*g - 2.0*g*cosTheta, 1.5));

    vec3 sunIntensity = vec3(20.0); // sun brightness
    return sunIntensity * (totalRayleigh * RAYLEIGH_COEFF * rayleighPhase +
                           totalMie * MIE_COEFF * miePhase);
}

// Procedural stars
float stars(vec3 rd) {
    vec3 p = rd * 200.0;
    vec3 id = floor(p);
    vec3 fd = fract(p) - 0.5;
    float seed = dot(id, vec3(127.1, 311.7, 74.7));
    float rnd = fract(sin(seed) * 43758.5453);
    float star = smoothstep(0.97, 0.99, rnd);
    float dist = length(fd);
    return star * smoothstep(0.5, 0.1, dist);
}

void main() {
    float depth = texture(uDepth, vUV).r;

    // Only draw sky in empty pixels (depth == 1.0)
    if (depth < 0.9999) {
        discard;
    }

    // Reconstruct world ray direction
    vec2 ndc = vUV * 2.0 - 1.0;
    vec4 worldDir = uInvVP * vec4(ndc, 1.0, 1.0);
    vec3 rd = normalize(worldDir.xyz / worldDir.w);

    vec3 color = atmosphere(rd, normalize(uSunDir));

    // Add stars at night
    float nightFactor = 1.0 - smoothstep(0.0, 0.3, uDaylight);
    if (nightFactor > 0.0 && rd.y > 0.0) {
        color += vec3(stars(rd)) * nightFactor * 2.0;
    }

    // Sun disc
    float sunDot = dot(rd, normalize(uSunDir));
    if (sunDot > 0.9995) {
        color += vec3(50.0, 40.0, 30.0) * smoothstep(0.9995, 0.9999, sunDot);
    }

    FragColor = vec4(max(color, 0.0), 1.0);
}

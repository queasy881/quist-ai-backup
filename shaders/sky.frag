#version 460 core

in vec2 vUV;

uniform mat4  uInvVP;
uniform float uTime;
uniform float uDayLength;
uniform vec3  uSunDir;
uniform float uDaylight;
uniform vec3  uSkyColor;

out vec4 FragColor;

// Pseudo-random for stars
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

void main() {
    // Reconstruct view direction from screen UV
    vec4 clipNear = vec4(vUV * 2.0 - 1.0, -1.0, 1.0);
    vec4 clipFar  = vec4(vUV * 2.0 - 1.0,  1.0, 1.0);
    vec4 wNear    = uInvVP * clipNear; wNear /= wNear.w;
    vec4 wFar     = uInvVP * clipFar;  wFar  /= wFar.w;
    vec3 dir      = normalize(wFar.xyz - wNear.xyz);

    // Sky gradient (horizon to zenith)
    float vert = max(dir.y, 0.0);
    vec3 horizon = uSkyColor * 1.15;
    vec3 zenith  = uSkyColor * 0.7;
    vec3 sky     = mix(horizon, zenith, pow(vert, 0.5));

    // Sun disc
    float sunDot = dot(dir, normalize(uSunDir));
    float sunDisc = smoothstep(0.9993, 0.9997, sunDot);
    vec3 sunColor = vec3(1.0, 0.95, 0.85) * sunDisc * uDaylight;
    sky += sunColor;

    // Sunset glow at horizon
    float horizonGlow = pow(max(1.0 - abs(dir.y), 0.0), 8.0);
    float sunsetFactor = max(1.0 - uDaylight * 1.5, 0.0);
    sky += vec3(0.8, 0.3, 0.1) * horizonGlow * sunsetFactor * 0.6;

    // Stars at night
    float nightFactor = 1.0 - uDaylight;
    if (nightFactor > 0.1 && dir.y > 0.0) {
        vec3 starPos = floor(dir * 300.0);
        float star   = hash(starPos);
        float twinkle = sin(uTime * 2.0 + star * 100.0) * 0.5 + 0.5;
        if (star > 0.990) {
            sky += vec3(0.8, 0.85, 1.0) * nightFactor * twinkle;
        }
    }

    // Below-horizon fallback
    if (dir.y < 0.0)
        sky = mix(sky, horizon * 0.5, smoothstep(0.0, -0.3, dir.y));

    FragColor = vec4(sky, 1.0);
}

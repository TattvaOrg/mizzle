in vec2 vUv;
out vec4 fragColor;

uniform float uTime;
uniform vec2 uResolution;
uniform int uBgStyle; // 0 = Alpine Lake Dusk (default motion), 1 = Optics Lab (stripes + orbs)

vec3 hsl2rgb(float h, float s, float l) {
    vec3 rgb = clamp(abs(mod(h * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return l + s * (rgb - 0.5) * (1.0 - abs(2.0 * l - 1.0));
}

float hash11(float p) {
    p = fract(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

float noise1(float x) {
    float i = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash11(i), hash11(i + 1.0), f);
}

float fbm1(float x) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; ++i) {
        v += a * noise1(x);
        x *= 2.03;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 p = vUv;
    float aspect = uResolution.x / max(uResolution.y, 1.0);
    vec2 ap = vec2((p.x - 0.5) * aspect, p.y - 0.5);
    float t = uTime * 0.22;

    if (uBgStyle == 1) {
        // Optics Lab: diagonal architectural stripes + drifting orbs + bottom prism band
        float diag = (p.x * aspect + p.y) * 9.5 + sin(t * 0.6) * 0.15;
        float stripe = smoothstep(-0.04, 0.04, sin(diag * 3.14159));
        vec3 col = mix(vec3(0.08, 0.10, 0.14), vec3(0.19, 0.22, 0.28), stripe);

        if (p.y < 0.20) {
            float barX = p.x * 28.0;
            float barShade = 0.80 + 0.20 * smoothstep(-0.05, 0.05, sin(barX * 3.14159));
            vec3 rainbow = hsl2rgb(fract(p.x * 0.65 + 0.02), 0.86, 0.54) * barShade;
            float blendY = smoothstep(0.20, 0.16, p.y);
            col = mix(col, rainbow, blendY * step(0.42, p.x));
        }

        vec2 orb1 = vec2(0.32 * aspect + sin(uTime * 0.55) * 0.34, 0.10 + cos(uTime * 0.42) * 0.22);
        vec2 orb2 = vec2(-0.34 * aspect + cos(uTime * 0.48) * 0.28, -0.08 + sin(uTime * 0.52) * 0.20);
        col += vec3(0.16, 0.56, 1.00) * exp(-length(ap - orb1) * 7.5) * 1.10;
        col += vec3(0.85, 0.94, 1.00) * exp(-length(ap - orb1) * 22.0) * 0.85;
        col += vec3(1.00, 0.58, 0.14) * exp(-length(ap - orb2) * 7.0) * 1.00;
        fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
        return;
    }

    // Style 0: Serene alpine lake at twilight (slate sky, warm amber clouds, distant ridge, rippling water)
    float horizon = 0.46;
    vec3 skyTop = vec3(0.11, 0.17, 0.29);
    vec3 skyMid = vec3(0.20, 0.27, 0.39);
    vec3 skyGlow = vec3(0.62, 0.43, 0.36);

    float skyT = clamp((p.y - horizon) / (1.0 - horizon), 0.0, 1.0);
    vec3 sky = mix(skyMid, skyTop, pow(skyT, 0.65));

    // Warm sunset cloud band on the right
    float cloudBand = exp(-pow((p.y - 0.62) * 4.8, 2.0)) * smoothstep(0.20, 0.95, p.x);
    float cloudTex = fbm1(p.x * 6.0 + t * 0.25) * 0.7 + fbm1(p.x * 14.0 - t * 0.15) * 0.3;
    sky = mix(sky, skyGlow * (0.75 + 0.45 * cloudTex), cloudBand * (0.45 + 0.40 * cloudTex));

    // Distant mountain silhouettes
    float ridge1 = horizon + 0.085 * fbm1(p.x * 3.8 + 1.4) + 0.03 * sin(p.x * 5.0);
    float ridge2 = horizon + 0.055 * fbm1(p.x * 6.5 + 4.2);

    vec3 col = sky;
    if (p.y < ridge1 && p.y >= horizon) {
        col = mix(vec3(0.12, 0.16, 0.24), vec3(0.22, 0.26, 0.35), (p.y - horizon) * 6.0);
    }
    if (p.y < ridge2 && p.y >= horizon) {
        col = vec3(0.08, 0.12, 0.19);
    }

    // Calm reflective water below horizon with subtle liquid ripples
    if (p.y < horizon) {
        float waterDepth = (horizon - p.y) / horizon;
        float ripple = sin(p.y * 190.0 - uTime * 1.4 + fbm1(p.x * 18.0) * 5.0) * (0.004 + 0.014 * waterDepth);
        float reflY = clamp(horizon + (horizon - p.y) * 0.85 + ripple, horizon, 1.0);
        float reflT = clamp((reflY - horizon) / (1.0 - horizon), 0.0, 1.0);
        vec3 reflSky = mix(skyMid, skyTop, pow(reflT, 0.65));
        float reflCloud = exp(-pow((reflY - 0.62) * 4.8, 2.0)) * smoothstep(0.20, 0.95, p.x);
        reflSky = mix(reflSky, skyGlow * 0.85, reflCloud * 0.55);

        vec3 deepWater = vec3(0.05, 0.10, 0.17);
        col = mix(reflSky * 0.72, deepWater, smoothstep(0.0, 0.85, waterDepth) * 0.68);
        // Subtle horizon water sheen
        col += vec3(0.10, 0.16, 0.22) * exp(-waterDepth * 9.0) * (0.5 + 0.5 * sin(p.x * 40.0 + uTime));
    }

    fragColor = vec4(clamp(col, 0.0, 1.0), 1.0);
}

in vec2 vUv;
out vec4 fragColor;

uniform sampler2D uTex;
uniform int uFlipY;

void main() {
    vec2 uv = uFlipY != 0 ? vec2(vUv.x, 1.0 - vUv.y) : vUv;
    fragColor = texture(uTex, uv);
}

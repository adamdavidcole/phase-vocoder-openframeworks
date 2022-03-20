// code source/inspiration:
// https://github.com/Seungme/glitch-opengl/blob/master/src/shaders/fragment.shd


#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_res;
uniform vec2 u_mouse;
uniform float u_red;
//uniform sampler2DRect inputTexture;
//uniform sampler2DRect tex1;
//
uniform sampler2DRect inputTexture;
uniform sampler2DRect feedbackTexture;

in vec2 varyingtexcoord;


// BEGIN BLEND MODES
float blendLighten(float base, float blend) {
    return max(blend,base);
}

vec3 blendLighten(vec3 base, vec3 blend) {
    return vec3(blendLighten(base.r,blend.r),blendLighten(base.g,blend.g),blendLighten(base.b,blend.b));
}

vec3 blendLighten(vec3 base, vec3 blend, float opacity) {
    return (blendLighten(base, blend) * opacity + base * (1.0 - opacity));
}

float blendDarken(float base, float blend) {
    return min(blend,base);
}

vec3 blendDarken(vec3 base, vec3 blend) {
    return vec3(blendDarken(base.r,blend.r),blendDarken(base.g,blend.g),blendDarken(base.b,blend.b));
}

vec3 blendDarken(vec3 base, vec3 blend, float opacity) {
    return (blendDarken(base, blend) * opacity + base * (1.0 - opacity));
}


float blendPinLight(float base, float blend) {
    return (blend<0.5)?blendDarken(base,(2.0*blend)):blendLighten(base,(2.0*(blend-0.5)));
}

vec3 blendPinLight(vec3 base, vec3 blend) {
    return vec3(blendPinLight(base.r,blend.r),blendPinLight(base.g,blend.g),blendPinLight(base.b,blend.b));
}

vec3 blendPinLight(vec3 base, vec3 blend, float opacity) {
    return (blendPinLight(base, blend) * opacity + base * (1.0 - opacity));
}
// END BLEND MODES



float map(float value, float min1, float max1, float min2, float max2) {
  return min2 + (value - min1) * (max2 - min2) / (max1 - min1);
}

float rand(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float rand() {
    return rand(vec2(u_time));
}

float random (vec2 position) {
    //generate random variable
  return fract(sin(dot(position, vec2(int( rand()*100.  ) % 50, int( rand()*100. ) % 100))));
}

float noise(vec2 p){
    vec2 ip = floor(p);
    vec2 u = fract(p);
    u = u*u*(3.0-2.0*u);
    
    float res = mix(
        mix(rand(ip),rand(ip+vec2(1.0,0.0)),u.x),
        mix(rand(ip+vec2(0.0,1.0)),rand(ip+vec2(1.0,1.0)),u.x),u.y);
    return res*res;
}

// BEGIN PERLIN NOISE
//float rand(vec2 c){
//    return fract(sin(dot(c.xy ,vec2(12.9898,78.233))) * 43758.5453);
//}

//    Classic Perlin 3D Noise
//    by Stefan Gustavson
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}
vec3 fade(vec3 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

float cnoise(vec3 P){
  vec3 Pi0 = floor(P); // Integer part for indexing
  vec3 Pi1 = Pi0 + vec3(1.0); // Integer part + 1
  Pi0 = mod(Pi0, 289.0);
  Pi1 = mod(Pi1, 289.0);
  vec3 Pf0 = fract(P); // Fractional part for interpolation
  vec3 Pf1 = Pf0 - vec3(1.0); // Fractional part - 1.0
  vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
  vec4 iy = vec4(Pi0.yy, Pi1.yy);
  vec4 iz0 = Pi0.zzzz;
  vec4 iz1 = Pi1.zzzz;

  vec4 ixy = permute(permute(ix) + iy);
  vec4 ixy0 = permute(ixy + iz0);
  vec4 ixy1 = permute(ixy + iz1);

  vec4 gx0 = ixy0 / 7.0;
  vec4 gy0 = fract(floor(gx0) / 7.0) - 0.5;
  gx0 = fract(gx0);
  vec4 gz0 = vec4(0.5) - abs(gx0) - abs(gy0);
  vec4 sz0 = step(gz0, vec4(0.0));
  gx0 -= sz0 * (step(0.0, gx0) - 0.5);
  gy0 -= sz0 * (step(0.0, gy0) - 0.5);

  vec4 gx1 = ixy1 / 7.0;
  vec4 gy1 = fract(floor(gx1) / 7.0) - 0.5;
  gx1 = fract(gx1);
  vec4 gz1 = vec4(0.5) - abs(gx1) - abs(gy1);
  vec4 sz1 = step(gz1, vec4(0.0));
  gx1 -= sz1 * (step(0.0, gx1) - 0.5);
  gy1 -= sz1 * (step(0.0, gy1) - 0.5);

  vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
  vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
  vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
  vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
  vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
  vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
  vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
  vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);

  vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
  g000 *= norm0.x;
  g010 *= norm0.y;
  g100 *= norm0.z;
  g110 *= norm0.w;
  vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
  g001 *= norm1.x;
  g011 *= norm1.y;
  g101 *= norm1.z;
  g111 *= norm1.w;

  float n000 = dot(g000, Pf0);
  float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
  float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
  float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
  float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
  float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
  float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
  float n111 = dot(g111, Pf1);

  vec3 fade_xyz = fade(Pf0);
  vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
  vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
  float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x);
  return 2.2 * n_xyz;
}
// END PERLIN NOISE


vec4 lightning() {
    float r = rand() * 100.0;
    vec2 uv = varyingtexcoord / u_res;
    vec2 n = floor(uv * fract(u_time) * r * 0.1);

    vec4 currColor = texture2D(inputTexture, varyingtexcoord);

    vec3 textured = mod(vec3(currColor), vec4(uv, n).xyz);

    if (r > 95) {
        return vec4(textured, 1.0);
    }

    return currColor;
}

vec4 full_noise() {
    vec3 noisePattern = vec3(fract(sin(dot(fract(vec2(random(varyingtexcoord * 50))), vec2(28, 312)))));

//    randomNoise
//    float noise = rand(varyingtexcoord + rand(vec2(u_time * rand())));

    return vec4(noisePattern, 1.0);
}

vec4 horizontal_lines() {
    float r = rand() * 100.0;
    vec2 uv = varyingtexcoord / u_res;

    //generate black horizontal lines
    if (mod(uv.y, 0.9) < cos(r) + 0.05 && mod(uv.y, 0.95) > cos(r) && fract(u_time * 0.1) > -0.01 && uv.x < cos(r) + 0.6 && uv.x > cos(r) - 0.6)
    {
        return texture(inputTexture, vec2(cos(u_time)*u_res.x, floor(r) * u_res.y));
    }
    return texture2D(inputTexture, varyingtexcoord);
}

vec4 vertical_lines() {
    //generate black vertical lines
    float r = rand() * 100.0;
    vec2 uv = varyingtexcoord / u_res;
    if (mod(uv.x, 0.9) < cos(r) + 0.04 && mod(uv.x, 0.95) > cos(r) && r > 80) {
        return texture2D(inputTexture, vec2(cos(u_time) * u_res.x, floor(u_time * 0.05) * u_res.y));
    }
    return texture2D(inputTexture, varyingtexcoord);
}


vec4 rgbShift() {
    float r = rand();
    vec2 noiseInput = vec2(u_time, u_time);
    float n = noise(noiseInput);
    float shiftX = r * (50.0 * n);

    float shiftY = rand(vec2(r, r)) * 20.0 * n;

    vec4 currColor = texture2D(inputTexture, varyingtexcoord);
    vec4 prevColor = texture2D(inputTexture, vec2(varyingtexcoord.x - shiftX, varyingtexcoord.y + shiftY));
    vec4 nextColor = texture2D(inputTexture, vec2(varyingtexcoord.x + shiftX, varyingtexcoord.y + shiftY));
//
//    return vec4(prevColor.r, currColor.g, nextColor.b, 1.0);
    return vec4(prevColor.r, currColor.g, nextColor.b, 1.0);
}

vec4 vertical_shift() {
    vec2 newCoords = varyingtexcoord;
    if ((varyingtexcoord.y/u_res.y)  < (1 - fract(u_time * 0.8)) && rand() < 0.1) {
        newCoords.x += 0.01 * u_res.x;
    }
    return texture2D(inputTexture, newCoords);
}

vec4 glitchColor() {
    //    GLITCH BEHAVIOR
    vec4 outputColor = vec4(0.0);
    float r = rand(vec2(rand(), rand()));

    if (r < 0.3) {
        outputColor = rgbShift();
    } else if (r < 0.5) {
        outputColor = vertical_shift();
    } else if (r < 0.51) {
        outputColor = full_noise();
    } else if (r < 0.6) {
        outputColor = horizontal_lines();
    } else if (r < 0.75) {
        outputColor = vertical_lines();
    } else if (r < 0.8) {
        outputColor = lightning();
    } else {
        outputColor = texture2D(inputTexture, varyingtexcoord);
    }

    return outputColor;
}

void main(){
//    vec2 coord = gl_FragCoord.xy;
//    vec2 uv = coord / u_resolution;
//    vec2 mouse_uv = u_mouse / u_resolution;
//    vec2 ratio =  u_resolution / u_res;
    
 
    vec2 inputCoord = varyingtexcoord;
    vec2 feedbackCoord = vec2(u_res.x - inputCoord.x, inputCoord.y);
    
    float feedbackNoise = cnoise(vec3(feedbackCoord.xy / 100.0, u_time/10.0));
    vec2 feedbackNoisePos = feedbackCoord + feedbackNoise * 10.0;
    float clampMargin = 20.0;
    vec2 feedbackNoisePosClamped = vec2(
        clamp(feedbackNoisePos.x, clampMargin, u_resolution.x - clampMargin),
        clamp(feedbackNoisePos.y, clampMargin, u_resolution.y - clampMargin)
    );
    
    vec4 inputColor = texture2D(inputTexture, inputCoord);
    vec4 feedbackColor = texture2D(feedbackTexture, feedbackNoisePosClamped);
    vec4 glitchColor = glitchColor();
    
    vec3 classicCombo = mix(inputColor.xyz, feedbackColor.xyz, .9);
    
    vec3  comboA = blendPinLight(inputColor.xyz, feedbackColor.xyz, 1.0);
    vec3 combo = mix(inputColor.xyz, comboA.xyz, .999999);
    
    
    gl_FragColor = vec4(classicCombo.xyz, 1.0);
}

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
uniform sampler2DRect tex0;

in vec2 varyingtexcoord;


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

vec4 lightning() {
    float r = rand() * 100.0;
    vec2 uv = varyingtexcoord / u_res;
    vec2 n = floor(uv * fract(u_time) * r * 0.1);
    
    vec4 currColor = texture2D(tex0, varyingtexcoord);
    
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
        return texture(tex0, vec2(cos(u_time)*u_res.x, floor(r) * u_res.y));
    }
    return texture2D(tex0, varyingtexcoord);
}

vec4 vertical_lines() {
    //generate black vertical lines
    float r = rand() * 100.0;
    vec2 uv = varyingtexcoord / u_res;
    if (mod(uv.x, 0.9) < cos(r) + 0.04 && mod(uv.x, 0.95) > cos(r) && r > 80) {
        return texture2D(tex0, vec2(cos(u_time) * u_res.x, floor(u_time * 0.05) * u_res.y));
    }
    return texture2D(tex0, varyingtexcoord);
}


vec4 rgbShift() {
    float r = rand();
    vec2 noiseInput = vec2(u_time, u_time);
    float n = noise(noiseInput);
    float shiftX = r * (50.0 * n);
    
    float shiftY = rand(vec2(r, r)) * 20.0 * n;

    vec4 currColor = texture2D(tex0, varyingtexcoord);
    vec4 prevColor = texture2D(tex0, vec2(varyingtexcoord.x - shiftX, varyingtexcoord.y + shiftY));
    vec4 nextColor = texture2D(tex0, vec2(varyingtexcoord.x + shiftX, varyingtexcoord.y + shiftY));
//
//    return vec4(prevColor.r, currColor.g, nextColor.b, 1.0);
    return vec4(prevColor.r, currColor.g, nextColor.b, 1.0);
}

vec4 vertical_shift() {
    vec2 newCoords = varyingtexcoord;
    if ((varyingtexcoord.y/u_res.y)  < (1 - fract(u_time * 0.8)) && rand() < 0.1) {
        newCoords.x += 0.01 * u_res.x;
    }
    return texture2D(tex0, newCoords);
}

void main(){
    vec2 coord = gl_FragCoord.xy;
    vec3 color = vec3(0.0, 0.5, 0.5);
    
    vec2 uv = coord / u_resolution;
    vec2 mouse_uv = u_mouse / u_resolution;
    
    vec2 ratio =  u_resolution / u_res;
    
    
//    gl_FragColor = rgbShift();
//    gl_FragColor = vertical_shift();
//    gl_FragColor = full_noise();
//    gl_FragColor = horizontal_lines();
//    gl_FragColor = vertical_lines();
//    gl_FragColor = lightning();
//    gl_FragColor = (vertical_lines() + vertical_shift() + rgbShift() + lightning()) /4.0;
    
    float r = rand(vec2(rand(), rand()));
    
    if (r < 0.3) {
        gl_FragColor = rgbShift();
    } else if (r < 0.5) {
        gl_FragColor = vertical_shift();
    } else if (r < 0.51) {
        gl_FragColor = full_noise();
    } else if (r < 0.6) {
        gl_FragColor = horizontal_lines();
    } else if (r < 0.75) {
        gl_FragColor = vertical_lines();
    } else if (r < 0.8) {
        gl_FragColor = lightning();
    } else {
        gl_FragColor = texture2D(tex0, varyingtexcoord);
    }
}

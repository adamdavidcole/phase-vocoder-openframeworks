#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 modelViewProjectionMatrix;
uniform vec2 u_mouse;
uniform vec2 u_resolution;
uniform vec2 u_res;
uniform float u_time;

attribute vec4 position;

in vec2 texcoord;
out vec2 varyingtexcoord;

float rand(vec2 n) {
    return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
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

void main(){
    float n = noise(texcoord + u_time) * 0.0;
    varyingtexcoord = vec2(
        texcoord.x,
        texcoord.y
    );
    gl_Position = modelViewProjectionMatrix * position;
}

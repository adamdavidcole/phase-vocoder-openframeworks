#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;
uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_red;
uniform sampler2DRect tex0;

in vec2 varyingtexcoord;


void main(){
    vec2 coord = gl_FragCoord.xy;
    vec3 color = vec3(0.0, 0.5, 0.5);
    vec2 uv = coord / u_resolution;
    
    vec2 mouse_uv = u_mouse / u_resolution;
    vec4 text = texture2D(tex0, coord);

    gl_FragColor = vec4(text.rgb, 1.0);
    
//    gl_FragColor = vec4(uv.x, uv.y, 0.0, 1.0);
}

#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 modelViewProjectionMatrix;

attribute vec4 position;

in vec2 texcoord;
out vec2 varyingtexcoord;

void main(){
    varyingtexcoord = vec2(texcoord.x, texcoord.y);
    gl_Position = modelViewProjectionMatrix * position;
}

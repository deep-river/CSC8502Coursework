# version 400

uniform mat4 modelMatrix ;
uniform mat4 viewMatrix ;
uniform mat4 projMatrix ;
uniform sampler2D blendMap;


in vec3 position ;
in vec2 texCoord ;

 out Vertex {
 vec2 texCoord ;
 } OUT ;

 void main ( void ) {

 vec4 blendMapColour = texture(blendMap, texCoord);

 vec4 localPos = vec4 ( position , 1.0f );

 gl_Position = projMatrix * viewMatrix * modelMatrix * localPos;

 OUT.texCoord = texCoord ;
}
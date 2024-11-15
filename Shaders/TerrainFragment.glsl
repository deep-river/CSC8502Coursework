#version 330 core

uniform sampler2D rockTex;	
uniform sampler2D rockBump;	

uniform sampler2D grassTex;
uniform sampler2D grassBump;

uniform sampler2D sandTex;
uniform sampler2D sandBump;

uniform sampler2D snowTex;
uniform sampler2D snowBump;

uniform float worldHeight;

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 tangent;
    vec3 binormal;
    vec3 worldPos;
} IN;

out vec4 fragColour[2];	// Final outputted colours

void main(void) {
    mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal)); // Form tangent binormal normal matrix
    vec3 normal = texture2D(rockBump, IN.texCoord).rgb * 2.0 - 1.0;
	normal = normalize(TBN * normalize(normal));

    float NYPos = IN.worldPos.y / worldHeight; // Normalised Y pos
    /*
    if(NYPos <= 0.3) {
        fragColour[0] = texture2D(sandTex, IN.texCoord);
        vec3 normal = texture2D(sandBump, IN.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalize(normal));
        fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
    } else if (NYPos <= 0.7) {
        fragColour[0] = texture2D(rockTex, IN.texCoord);
        vec3 normal = texture2D(rockBump, IN.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalize(normal));
        fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
    } else {
        fragColour[0] = texture2D(grassTex, IN.texCoord);
        vec3 normal = texture2D(grassBump, IN.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalize(normal));
        fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
    }
    */
    if(NYPos <= 0.5) {
        fragColour[0] = texture2D(sandTex, IN.texCoord);
        vec3 normal = texture2D(sandBump, IN.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalize(normal));
        fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
    } else {
        fragColour[0] = texture2D(grassTex, IN.texCoord);
        vec3 normal = texture2D(grassBump, IN.texCoord).rgb * 2.0 - 1.0;
        normal = normalize(TBN * normalize(normal));
        fragColour[1] = vec4(normal.xyz * 0.5 + 0.5, 1.0);
    }
}
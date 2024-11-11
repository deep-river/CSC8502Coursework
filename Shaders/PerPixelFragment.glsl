#version 330 core

/*
start off with a uniform texture sampler so we
can sample the incoming mesh's texture.
further 4 uniform values - 3 to represent the point 
light's attributes, and 1 to keep the camera's
world space position.
*/
uniform sampler2D diffuseTex;
uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex {
    vec4 colour;
    vec2 texCoord;
    vec3 normal;
    vec3 worldPos;
} IN;

out vec4 fragColour;


void main(void) {
    /*
    most light models need the incident, view, and half-angle
    vectors, so the main func's first task is to compute these.

    since we get the world space position of each fragment 
    coming in as an interpolated value, this is as simple as 
    subtracting it from our camera and light positions, and 
    normalising the results.
    */
    vec3 incident = normalize(lightPos - IN.worldPos);
    vec3 viewDir = normalize(cameraPos - IN.worldPos);

    /*
    the half-angle is just the sum of the incident and 
    view dir, re-normalised to keep it unit length.
    */
    vec3 halfDir = normalize(incident + viewDir);
    /*
    diffuse light needs the colour of the surface of the 
    object, which we're representing via a texture, so 
    this is sampled, too.
    */
    vec4 diffuse = texture(diffuseTex, IN.texCoord);

    /*
    lambert's cosine law can give us the amount of light 
    hitting a surface, it's a simple dot product with the normal, clamped. 
    */
    float lambert = max(dot(incident, IN.normal), 0.0f);
    /*
    attenuation of the light source at current fragment. 
    the distance between the current fragment and the light position is 
    easily calculated. Remember, worldPos will be an interpolated value 
    calculated on a per-fragment vasis from the vertices that make up the 
    geometry.
    */
    float distance = length(lightPos - IN.worldPos);
    float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

    /*
    next up is calculating the specularity lighting 
    component of the fragment. Using the half-angle, 
    we can work out how close to a perfect reflection 
    angle the camera is, by taking another dot product,
     between the normal and half-angle-if they align 
    perfectly, it'll be 1.0.
    */
    float specFactor = clamp(dot(halfDir, IN.normal), 0.0, 1.0);
    /*
    we can then work out the final specularity value by 
    raising the specular reflection value to a power 
    representing how shiny the surface is - the higher 
    this power, the more perfectly shiny the surface is,
     and the tigher out specularity will be.
    */
    specFactor = pow(specFactor, 60.0);

    /*
    calculating the final colour of our fragment.
    */
     vec3 surface = (diffuse.rgb * lightColour.rgb);
    fragColour.rgb = surface * lambert * attenuation;
    fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
    fragColour.rgb += surface * 0.1f; //ambient!
    fragColour.a = diffuse.a;
}
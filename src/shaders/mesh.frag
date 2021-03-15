#version 330
#extension GL_ARB_explicit_attrib_location : require

// Uniform constants
uniform float u_gamma; // enable gamma correction
uniform float u_envMapping; // enable environment mapping

uniform vec3 u_diffuseColor;
uniform float u_ambientEnabled;
uniform float u_diffuseEnabled;
uniform float u_specularEnabled;
uniform float u_specularPower;

uniform samplerCube u_cubemap;

uniform sampler2D u_texture; // texture sampler
uniform float u_texCoordinates; // enable visualization of texture coordinates
uniform float u_texMapping; // enable texture mapping
uniform float u_lighting;

uniform float u_toonEnabled; // enable toon shading
uniform sampler1D u_quantization; 
uniform sampler2D u_outline;

in vec3 N; // view space normal vector
in vec3 L; // view space light direction vector
in vec3 V; // view vector

in vec2 v_texcoord; // interpolated texture coordinate

out vec4 frag_color;

vec3 gammaCorrect(vec3 color) { // gamma correction
    if(u_gamma > 0.5) return pow(color, vec3(1 / 2.2));
    return color;
}

void main() {
    // blinn-phong lighting calculations
    float lambertian = max(dot(L, N), 0.0);
    vec3 H = normalize(L + V);
    vec3 R = reflect(-V, N);
    float specAngle = max(dot(H, N), 0.0);
    float specular = pow(specAngle, u_specularPower);
    specular = ((u_specularPower + 8) / 8) * specular; // normalize specular lighting

    vec3 color = vec3(0.0);

    vec3 ambientColor = u_diffuseColor * vec3(0.4);
    vec3 diffuseColor = u_diffuseColor;
    vec3 specularColor = vec3(0.1);

    if(u_texMapping > 0.5) { // get diffuse base color if texture mapping is turned on
        diffuseColor = texture(u_texture, v_texcoord).rgb; // get diffuse base color
        ambientColor = diffuseColor * vec3(0.4);
    }

    // make ambient, diffuse and specular lighting toggable from GUI
    if(u_ambientEnabled > 0.5)  color = color + ambientColor;
    if(u_diffuseEnabled > 0.5)  color = color + diffuseColor * L * lambertian;
    if(u_specularEnabled > 0.5) color = color + specularColor * L * specular;

    if(u_envMapping > 0.5) { // environment mapping
        vec3 color = texture(u_cubemap, R).rgb;
        
        frag_color = vec4(gammaCorrect(color), 1.0);
    } else if(u_texCoordinates > 0.5) { // texture coordinate visualization
        
        frag_color = vec4(v_texcoord, 0.0, 0.0);
    } else if (u_texMapping > 0.5) { // texture mapping
        vec4 textureColor = texture(u_texture, v_texcoord).rgba;
        if(u_lighting > 0.5) textureColor = textureColor * vec4(gammaCorrect(color), 1.0);
        
        frag_color = textureColor;
    } else if (u_toonEnabled > 0.5) { // toon shading
        vec3 toonColor = color;
        float colorScale = texture(u_quantization, lambertian).r;
        toonColor = toonColor * colorScale;

        // frag_color = vec4(gammaCorrect(toonColor), 1.0);

        // outline
        vec4 top = texture(u_outline, vec2(v_texcoord.x, v_texcoord.y + 1.0 / 200.0));
        vec4 bottom = texture(u_outline, vec2(v_texcoord.x, v_texcoord.y - 1.0 / 200.0));
        vec4 left = texture(u_outline, vec2(v_texcoord.x - 1.0 / 300.0, v_texcoord.y));
        vec4 right = texture(u_outline, vec2(v_texcoord.x + 1.0 / 300.0, v_texcoord.y));
        vec4 topLeft = texture(u_outline, vec2(v_texcoord.x - 1.0 / 300.0, v_texcoord.y + 1.0 / 200.0));
        vec4 topRight = texture(u_outline, vec2(v_texcoord.x + 1.0 / 300.0, v_texcoord.y + 1.0 / 200.0));
        vec4 bottomLeft = texture(u_outline, vec2(v_texcoord.x - 1.0 / 300.0, v_texcoord.y - 1.0 / 200.0));
        vec4 bottomRight = texture(u_outline, vec2(v_texcoord.x + 1.0 / 300.0, v_texcoord.y - 1.0 / 200.0));
        vec4 sx = -topLeft - 2 * left - bottomLeft + topRight   + 2 * right  + bottomRight;
        vec4 sy = -topLeft - 2 * top  - topRight   + bottomLeft + 2 * bottom + bottomRight;
        vec4 sobel = sqrt(sx * sx + sy * sy);
        frag_color = sobel;
    } else {
        frag_color = vec4(gammaCorrect(color), 1.0);
    }
}

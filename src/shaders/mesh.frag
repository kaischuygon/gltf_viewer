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
uniform sampler2D u_depthMap;
uniform float u_viewDepthMap;
const float offset = 1.0 / 300.0;

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
        vec2 offsets[9] = vec2[](
            vec2(-offset, offset),  // top-left
            vec2( 0.0, offset),     // top-center
            vec2( offset, offset),  // top-right
            vec2(-offset, 0.0),     // center-left
            vec2( 0.0, 0.0),        // center-center
            vec2( offset, 0.0),     // center-right
            vec2(-offset, -offset), // bottom-left
            vec2( 0.0, -offset),    // bottom-center
            vec2( offset, -offset)  // bottom-right    
        );

        float sobelX[9] = float[](
            1, 0, -1,
            2, 0, -2,
            1, 0, -1
        );

        float sobelY[9] = float[](
             1,  2,  1,
             0,  0,  0,
            -4, -2, -1
        );

        vec3 sampleTex[9];
        for(int i = 0; i < 9; i++)
            sampleTex[i] = vec3(texture(u_depthMap, v_texcoord.st + offsets[i]));

        vec3 gx = vec3(0.0);
        vec3 gy = vec3(0.0);
        for(int i = 0; i < 9; i++) {
            gx += sampleTex[i] * sobelX[i];
            gy += sampleTex[i] * sobelY[i];
        }

        vec3 g = sqrt(gx * gx + gy * gy);
        
        frag_color = vec4(gammaCorrect(toonColor - g), 1.0);
    } else if (u_viewDepthMap > 0.5) {
        float depthValue = texture(u_depthMap, v_texcoord).r;
        frag_color = vec4(vec3(depthValue), 1.0);
    } else {
        frag_color = vec4(gammaCorrect(color), 1.0);
    }
}

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
uniform float u_viewTextureCoords; // enable visualization of texture coordinates
uniform float u_texMapping; // enable texture mapping
uniform float u_lighting;

uniform float u_quantizationEnabled; // enable quantization
uniform sampler1D u_quantization; 
uniform sampler2D u_depthTexture;
uniform float u_viewDepth;
uniform sampler2D u_normalTexture;
uniform float u_viewNormals;
uniform float u_viewOutline;
uniform float u_outlineIntensity;

in vec3 N; // view space normal vector
in vec3 L; // view space light direction vector
in vec3 V; // view vector

in vec2 texcoord; // interpolated texture coordinate
in vec2 outlineTexcoord;
out vec4 frag_color;

vec3 gammaCorrect(vec3 color) { // gamma correction
    if(u_gamma > 0.5) return pow(color, vec3(1 / 2.2));
    return color;
}

float sobelFilter(sampler2D tex) {
    vec4 top         = texture(tex, vec2(outlineTexcoord.x, outlineTexcoord.y + 1.0 / 200.0));
    vec4 bottom      = texture(tex, vec2(outlineTexcoord.x, outlineTexcoord.y - 1.0 / 200.0));
    vec4 left        = texture(tex, vec2(outlineTexcoord.x - 1.0 / 300.0, outlineTexcoord.y));
    vec4 right       = texture(tex, vec2(outlineTexcoord.x + 1.0 / 300.0, outlineTexcoord.y));
    vec4 topLeft     = texture(tex, vec2(outlineTexcoord.x - 1.0 / 300.0, outlineTexcoord.y + 1.0 / 200.0));
    vec4 topRight    = texture(tex, vec2(outlineTexcoord.x + 1.0 / 300.0, outlineTexcoord.y + 1.0 / 200.0));
    vec4 bottomLeft  = texture(tex, vec2(outlineTexcoord.x - 1.0 / 300.0, outlineTexcoord.y - 1.0 / 200.0));
    vec4 bottomRight = texture(tex, vec2(outlineTexcoord.x + 1.0 / 300.0, outlineTexcoord.y - 1.0 / 200.0));
    vec4 sx = -topLeft - 2 * left - bottomLeft + topRight   + 2 * right  + bottomRight;
    vec4 sy = -topLeft - 2 * top  - topRight   + bottomLeft + 2 * bottom + bottomRight;
    vec4 sobel = sqrt(sx * sx + sy * sy);

    float average = (sobel.r + sobel.g + sobel.b) / 3;
    return average;
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
        diffuseColor = texture(u_texture, texcoord).rgb; // get diffuse base color
        ambientColor = diffuseColor * vec3(0.4);
    }

    // make ambient, diffuse and specular lighting toggable from GUI
    if(u_ambientEnabled > 0.5)  color = color + ambientColor;
    if(u_diffuseEnabled > 0.5)  color = color + diffuseColor * L * lambertian;
    if(u_specularEnabled > 0.5) color = color + specularColor * L * specular;

    if(u_envMapping > 0.5) { // environment mapping
        vec3 color = texture(u_cubemap, R).rgb;
        
        frag_color = vec4(gammaCorrect(color), 1.0);
    } else if (u_viewNormals > 0.5) {
        vec3 rgb_normal = texture(u_normalTexture, outlineTexcoord).rgb;

        frag_color = vec4(rgb_normal, 1.0);
    } else if (u_viewDepth > 0.5) {
        float depthValue = texture(u_depthTexture, outlineTexcoord).x; // gl_FragCoord.z;

        frag_color = vec4(vec3(depthValue), 1.0);
    } else if(u_viewTextureCoords > 0.5) { // texture coordinate visualization
        
        if(texcoord.x > 0 || texcoord.y > 0) // for lpshead model, who has different texture coordinates
            frag_color = vec4(texcoord, 0.0, 0.0);
        else
            frag_color = vec4(outlineTexcoord, 0.0, 0.0);
    } else if (u_texMapping > 0.5) { // texture mapping
        vec4 textureColor = texture(u_texture, texcoord).rgba;
        if(u_lighting > 0.5) textureColor = textureColor * vec4(gammaCorrect(color), 1.0);
        
        frag_color = textureColor;
    } else if (u_quantizationEnabled > 0.5) { // toon shading
        vec3 toonColor = color;
        float colorScale = texture(u_quantization, lambertian).r;
        toonColor = toonColor * colorScale;

        if(u_viewOutline > 0.5f) {
            float depthSobel = sobelFilter(u_depthTexture);
            float normalSobel = sobelFilter(u_normalTexture);
            float sobelIntensity = depthSobel + normalSobel / 2;
            if(sobelIntensity > u_outlineIntensity)
                toonColor = vec3(0.0);
        }
        frag_color = vec4(gammaCorrect(toonColor), 1.0);
    } else {
        frag_color = vec4(gammaCorrect(color), 1.0);
    }
}

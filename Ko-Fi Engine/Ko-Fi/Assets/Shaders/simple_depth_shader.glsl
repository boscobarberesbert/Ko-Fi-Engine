#shader vertex
#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 model_matrix;
uniform mat4 lightSpaceMatrix;

void main()
{
    gl_Position = lightSpaceMatrix * model_matrix * vec4(position, 1.0);
} 

#shader fragment
#version 330 core

void main()
{
    gl_FragDepth = gl_FragCoord.z;
}
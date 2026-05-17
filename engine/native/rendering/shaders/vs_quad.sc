$input a_position, a_texcoord0, a_color0
$output v_texcoord0, v_color0

#include <bgfx_shader.sh>

void main()
{
    // Grab the actual screen dimensions automatically tracked by bgfx
    float width  = u_viewRect.z;
    float height = u_viewRect.w;

    // Convert pixel coordinates [0, width] and [0, height] directly to NDC [-1, 1]
    float ndcX = (a_position.x / width) * 2.0 - 1.0;
    float ndcY = 1.0 - (a_position.y / height) * 2.0; // Flip Y so 0 is top of the screen

    // Lock Z to 0.0 and W to 1.0
    gl_Position = vec4(ndcX, ndcY, 0.0, 1.0);

    v_texcoord0 = a_texcoord0;
    v_color0    = a_color0;
}
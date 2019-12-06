// ENTRY POINT
float4 main(PixelInputType input) : SV_TARGET
{
    float2 projectTexCoord;
    float depthValue;
    float lightDepthValue;
    //float4 lightColor = float4(0,0,0,0);
    float4 lightColor = float4(0.05,0.05,0.05,1);

    // Set the bias value for fixing the floating point precision issues.
    float bias = 0.001f;

    //////////////// SHADOWING LOOP ////////////////
    for(int i = 0; i < NUM_LIGHTS; ++i)
    {
    // Calculate the projected texture coordinates.
    projectTexCoord.x =  input.vertex_ProjLightSpace[i].x / input.vertex_ProjLightSpace[i].w / 2.0f + 0.5f;
    projectTexCoord.y = -input.vertex_ProjLightSpace[i].y / input.vertex_ProjLightSpace[i].w / 2.0f + 0.5f;

    if((saturate(projectTexCoord.x) == projectTexCoord.x) && (saturate(projectTexCoord.y) == projectTexCoord.y))
    {
        // Sample the shadow map depth value from the depth texture using the sampler at the projected texture coordinate location.
        depthValue = depthTextures[i].Sample(SampleTypeClamp, projectTexCoord).r;

        // Calculate the depth of the light.
        lightDepthValue = input.vertex_ProjLightSpace[i].z / input.vertex_ProjLightSpace[i].w;

        // Subtract the bias from the lightDepthValue.
        lightDepthValue = lightDepthValue - bias;

        // Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
        // If the light is in front of the object then light the pixel, if not then shadow this pixel since an object (occluder) is casting a shadow on it.
            if(lightDepthValue < depthValue)
            {
                // Calculate the amount of light on this pixel.
                float lightIntensity = saturate(dot(input.normal, normalize(input.lightPos_LS[i])));

                if(lightIntensity > 0.0f)
                {
                    float spotlightIntensity = CalculateSpotLightIntensity(input.lightPos_LS[i], cb_lights[i].lightDirection, input.normal);
                    //lightColor += (float4(1.0f, 1.0f, 1.0f, 1.0f) * lightIntensity) * .3f; // spotlight
                    lightColor += float4(1.0f, 1.0f, 1.0f, 1.0f) /** lightIntensity*/ * spotlightIntensity * .3f; // spotlight
                }
            }
        }
    }

    return saturate(lightColor);
}
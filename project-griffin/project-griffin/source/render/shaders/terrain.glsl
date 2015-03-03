#ifdef _VERTEX_
	
	void main()
	{
	}

#endif

#ifdef _FRAGMENT_

	/*float3 blend(float4 texture1, float a1, float4 texture2, float a2)
	{
		return texture1.a + a1 > texture2.a + a2 ? texture1.rgb : texture2.rgb;
	}*/

	float3 blend(float4 texture1, float a1, float4 texture2, float a2)
	{
		float depth = 0.2;
		float ma = max(texture1.a + a1, texture2.a + a2) - depth;

		float b1 = max(texture1.a + a1 - ma, 0);
		float b2 = max(texture2.a + a2 - ma, 0);

		return (texture1.rgb * b1 + texture2.rgb * b2) / (b1 + b2);
	}

	void main()
	{
	}
	
#endif
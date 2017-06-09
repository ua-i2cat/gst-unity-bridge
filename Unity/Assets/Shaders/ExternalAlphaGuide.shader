// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "GUB/TransitionsBase" {

	Properties{
			_MainTex("Texture", 2D) = "white" {}
			_AlphaTex("Guide (RGB)", 2D) = "white" {}
			_Color("Color Tint", Color) = (1,1,1,1)
			_TransitionProgress("TransitionProgress",Range(0,1)) = 1
			[KeywordEnum(Disable,Enable)] _Inverse("Inverse", Float) = 0
			[KeywordEnum(Solid, Transparency, Threshold, Video)] _Solid("BodySolid", Float) = 3
			_TimeFactor("TimeFactor", Float) = 1

		}

		SubShader{

			Tags{ "Queue" = "Transparent" "RenderType" = "Transparent" }
			LOD 200
			
			Pass
			{
				Blend srcalpha oneminussrcalpha
				CGPROGRAM
				#pragma vertex vert
				#pragma fragment frag

				#include "UnityCG.cginc"

				struct appdata
				{
					float4 vertex : POSITION;
					float2 uv : TEXCOORD0;
				};

				struct v2f
				{
					float2 uv : TEXCOORD0;
					float4 vertex : SV_POSITION;
				};

				sampler2D _MainTex;
				sampler2D _AlphaTex;
				float4 _Color;
				float _TransitionProgress;
				float _Inverse;
				float _Solid;
				float _TimeFactor;
				
				float4 _MainTex_ST;

				v2f vert(appdata v)
				{
					v2f o;
					o.vertex = UnityObjectToClipPos(v.vertex);
					o.uv = TRANSFORM_TEX(v.uv, _MainTex);
					return o;
				}

				fixed4 frag(v2f i) : SV_Target
				{
					fixed4 col = tex2D(_MainTex, i.uv);
					fixed4 g = tex2D(_AlphaTex, i.uv);
					col.rgb = col.rgb * _Color.rgb;

					half activeValue = 1;
					half inactiveValue = 0;
					half transparency = _TransitionProgress * _TimeFactor;

					if (_Solid == 2)
					{
						activeValue = transparency;
					}

					if (_Inverse)
					{
						activeValue = 0;
						inactiveValue = 1;
						transparency = 1 - transparency;

						if (_Solid == 2)
						{
							activeValue = transparency;
						}
					}

					if (_Solid == 3)
					{
						col.a = g.r;
					}
					else if (_Solid == 1)
					{
						col.a = transparency;
					}
					else if ((g.r + g.g + g.b)*0.33333f <= _TransitionProgress)
					{
						col.a = activeValue;
					}
					else
					{
						col.a = inactiveValue;
					}

					return col;

				}
			ENDCG
		}
	}
}

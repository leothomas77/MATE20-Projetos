#version 430

out vec4 FragColor;

in VS_OUT {
	vec3 Normal;
    vec3 FragPos;
    vec2 TexCoords;
} fs_in; 

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform bool debug;
uniform bool showNormal;

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv ) {
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx( p );
    vec3 dp2 = dFdy( p );
    vec2 duv1 = dFdx( uv );
    vec2 duv2 = dFdy( uv );
 
    // solve the linear system
    vec3 dp2perp = cross( dp2, N );
    vec3 dp1perp = cross( N, dp1 );
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
 
    // construct a scale-invariant frame 
    float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
    return mat3( T * invmax, B * invmax, N );
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord, vec3 texcolor) {
 	vec3 map = texcolor;
	mat3 TBN = cotangent_frame( N, -V, texcoord);
    return normalize( TBN * map );
}

void main() {    
    vec3 N = normalize(fs_in.Normal);    
    vec3 V = normalize(fs_in.FragPos - viewPos); 
	
	vec3 normalRGB = texture( normalMap, fs_in.TexCoords).rgb;	//mapa de normais em textura RGB
	vec3 map = normalRGB * 2.0f - 1.0f;							// expande de 0..255 para -1..1

    vec3 pN; 

	if (debug) {
		pN = N;
	} else {
		pN = perturb_normal( N, V, fs_in.TexCoords, map);
	}
	// phong
	// ambient
	vec3 texcolor; 
	if (showNormal) {
		texcolor = normalRGB;
	} else {
		texcolor = texture( diffuseMap, fs_in.TexCoords).rgb; 
	}
	vec3 ambient = 0.5 * texcolor;

	// diffuse 
	vec3 lightDir = normalize(lightPos - fs_in.FragPos);
	float cTheta = max(dot(-lightDir, pN), 0.0);
	vec3 diffuse = cTheta * texcolor;

	// specular
	vec3 viewDir = normalize(viewPos - fs_in.FragPos);

	vec3 reflectDir = reflect(-lightDir, pN);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
	float spec = pow(max(dot(pN, halfwayDir), 0.0), 32.0);

	vec3 specular = vec3(0.2) * spec;

	FragColor = vec4(ambient + diffuse + specular, 1.0);
}
//

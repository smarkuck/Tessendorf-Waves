#version 330 core 

in vec3 o_pos;
in vec3 o_normal;

uniform int isColor;
uniform samplerCube CubeMap;
uniform vec3 cameraPosition;

vec3 sunDirection = vec3(0.96, -0.09, 0.45);

vec3 oceanColor = vec3(0, 0.2, 0.3);
vec3 skyColor = vec3(0.69,0.84,1);

float exposure = 0.35;

vec3 hdr (vec3 color, float exposure) {
    return 1.0 - exp(-color * exposure);
}

void main (void) {
    vec3 normal = o_normal;

	sunDirection = normalize(sunDirection);

	vec3 view = normalize(-o_pos);
    float fresnel = 0.02 + 0.98 * pow(1.0 - dot(normal, view), 5.0);
    vec3 sky = skyColor*fresnel;

    float diffuse = clamp(dot(normal, sunDirection), 0.0, 1.0);
    vec3 water = (1.0 - fresnel) * oceanColor * skyColor * diffuse;

	//odleg³oœæ od linii kamera - s³oñce
	vec3 p = vec3(o_pos.x, o_pos.y, o_pos.z);
	vec3 n = sunDirection;

	float dist = clamp(length( p - dot(p,n)*n )*0.02, 0, 1);

	float sunRate = dot(o_normal, vec3(sunDirection.x, -sunDirection.y, sunDirection.z));
	if(dist < 1 && sunRate > 0.05)
		sunRate *= 50*(1-dist)*(1-dist);
	else
		sunRate = 0;
	vec3 sun = vec3(sunRate, sunRate*0.2, 0);

	vec3 color = sky + water;

	vec3 reflection = reflect(vec3(o_pos.x, -o_pos.yz), o_normal);

	if(isColor == 1)
		gl_FragColor = vec4(0, 0.5, 1, 1);
	else
		gl_FragColor = texture(CubeMap,  reflection)*0.25 + vec4(hdr(color, exposure)+sun, 1.0);
}
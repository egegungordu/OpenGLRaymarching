#version 330 core

out vec4 outputColor;

uniform vec2 windowSize;
uniform float time;
uniform vec3 pos;
uniform mat4 viewToWorld;

float sX = 1f;
float sY = 1f;
float sZ = 1f;
float sR = 1.5f + 0.2 * sin(time);

float Iterations = 16;
float Power = 4 + (2 * sin(time/3));
float Bailout = 2;

float distToOrigin(vec3 pos)
{
	return pos.x * pos.x + pos.y * pos.y + pos.z * pos.z;
}

mat2x3 DE(vec3 pos) {
	vec3 z = pos;
	float dr = 1;
	float r = 0;
	float minDist = 9999;
	float maxDist = 0;
	float maxX = 0;
	float maxY = 0;
	float maxZ = 0;
	bool first = true;
	float oldR = 0.0;
	for (int i = 0; i < Iterations; i++) {
		oldR = r;
		r = length(z);
		if (r > Bailout) break;

		// convert to polar coordinates
		float theta = acos(z.z / r);
		float phi = atan(z.y, z.x);
		dr = pow(r, Power - 1.0) * (Power - 1) * dr + 1.0;

		// scale and rotate the point
		float zr = pow(r, Power);
		theta = theta * Power;
		phi = phi * Power;

		// convert back to cartesian coordinates
		z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
		
		float tempDist = distToOrigin(z);
		
		first = false;
		maxDist = max(maxDist, tempDist);
		maxX = max(maxX, z.x);
		maxY = max(maxY, z.y);
		maxZ = max(maxZ, z.z);
		minDist = min(minDist, tempDist);
		z += pos;
	}
	return mat2x3(vec3(0.5 * log(r) * r / dr, minDist, maxDist), vec3(maxX, maxY, maxZ));
}

void main()
{
	float normalX = gl_FragCoord.x - windowSize.x / 2;
	float normalY = gl_FragCoord.y - windowSize.y / 2;
	float minParam = min(windowSize.x, windowSize.y);
	float distToCenter = (minParam - sqrt(normalX * normalX + normalY * normalY ) + 300) / minParam;

	normalX /= minParam;
	normalY /= minParam;

	float frustumLength = 0.5;

	vec3 v = normalize(vec3(normalX, normalY, frustumLength));

	vec3 eye = pos;

	vec3 worldDir = (viewToWorld * vec4(v, 0.0)).xyz;

	float minDist = 999;

	for (int i = 0; i < 400; ++i)
	{
		mat2x3 data = DE(eye);
		if (data[0][0] < minDist) {
			minDist = data[0][0];
		}
		vec3 transform = worldDir * data[0][0];
		eye = eye + transform;
		if (data[0][0] < 0.001)
		{
			float lightFactor = 1;
			float dist = min(1 / length(eye - pos), 1);
			float orbit = sqrt(data[0][1]);
			float orbit2 = sqrt(data[1][0]);
			float orbit3 = sqrt(data[1][1]);
			float orbit4 = sqrt(data[1][2]);
			float orbit5 = sqrt(data[0][2]);
			float adjusted = (200 - i) / 200;
			outputColor = mix(vec4(0.6, 0.6, 0.1 + 0.7, 1.0f) * distToCenter,
				vec4(1 * orbit5,1 * orbit3,1 * orbit4, 1 )  * distToCenter * orbit * lightFactor, dist);
			return;
		}
	}

	// fog
	float dist = min(1 / length(pos), 1);
	outputColor = mix(vec4(0.6, 0.6, 0.1 + 0.7, 1.0f) * distToCenter, 
		vec4(0.6, 0.6, 0.1 + 0.7, 1.0f) * distToCenter, dist);
};
// Karis-style energy-conserving Marschner hair BSDF.
// References:
//   Marschner et al. "Light Scattering from Human Hair Fibers" (SIGGRAPH 2003)
//   Karis "Physically Based Hair Shading in Unreal" (SIGGRAPH 2016 course notes)
//   d'Eon et al. "An Energy-Conserving Hair Reflectance Model" (EGSR 2011)
//
// We do not have a per-vertex shading path in the deferred lighting pass, so all
// of the Marschner angle scalars (sin/cosThetaI, sin/cosThetaR, cosThetaD,
// cosPhiD, cosHalfPhi) are derived per-fragment from the world-space tangent
// stored in the GBuffer normal slot together with the light and view directions.

#define HAIR_SQRT_TWO_PI_INV 0.39894228040143267794

// Tunable Marschner parameters. These are reasonable defaults for human hair;
// the existing pipeline does not have authored hair material parameters yet so
// they live as constants here.
const float HairMarschnerLongitudinalSigma = 0.10;   // longitudinal Gaussian sigma (radians)
const float HairMarschnerCuticleShift      = 0.035;  // cuticle tilt alpha_R (radians, ~2 deg)
const float HairMarschnerIor               = 1.55;   // index of refraction of hair
const float HairMarschnerScaleR            = 0.05;   // primary specular weight
const float HairMarschnerScaleTT           = 0.55;   // forward scattering weight
const float HairMarschnerScaleTRT          = 0.18;   // secondary specular weight
const float HairMarschnerScaleDiffuse      = 0.22;   // wrap-around diffuse weight
const float HairMarschnerDiffuseFalloff    = 0.7;
const float HairMarschnerDiffuseAzimFalloff = 0.7;

vec3 SafeHairNormalize(vec3 value, vec3 fallback)
{
    float len2 = dot(value, value);
    if (len2 <= 1e-8)
    {
        return fallback;
    }
    return value * inversesqrt(len2);
}

float HairLongitudinalLobe(float sinThetaI, float sinThetaR, float alpha, float beta)
{
    float x = sinThetaI + sinThetaR - alpha;
    float invBeta = 1.0 / max(beta, 1e-4);
    return exp(-0.5 * x * x * invBeta * invBeta) * HAIR_SQRT_TWO_PI_INV * invBeta;
}

float HairSchlickFresnel(float cosAngle, float ior)
{
    float r0 = (1.0 - ior) / (1.0 + ior);
    r0 *= r0;
    float c = clamp(1.0 - cosAngle, 0.0, 1.0);
    float c2 = c * c;
    return r0 + (1.0 - r0) * c2 * c2 * c;
}

vec3 HairShade(Light light, Surface surface, vec3 fragPos, vec3 viewPos, float ambientOcclusion)
{
    vec3 surfaceToLight = light.position - fragPos;
    float distSq = dot(surfaceToLight, surfaceToLight);
    float attenuation = GetAttenuationFactor(distSq, light.range);

    vec3 lightDir = normalize(surfaceToLight);
    vec3 viewDir  = normalize(viewPos - fragPos);
    vec3 tangent  = SafeHairNormalize(surface.Normal, vec3(0.0, 1.0, 0.0));

    // Marschner angle scalars (Karis 2016, derived per-fragment).
    float sinThetaI = clamp(dot(lightDir, tangent), -1.0, 1.0);
    float sinThetaR = clamp(dot(viewDir,  tangent), -1.0, 1.0);
    float cosThetaI = sqrt(max(1.0 - sinThetaI * sinThetaI, 0.0));
    float cosThetaR = sqrt(max(1.0 - sinThetaR * sinThetaR, 0.0));
    float cosThetaD = (1.0 + cosThetaI * cosThetaR + sinThetaI * sinThetaR) * 0.5;
    cosThetaD = max(cosThetaD, 1e-3);

    vec3 lightPerp = lightDir - sinThetaI * tangent;
    vec3 viewPerp  = viewDir  - sinThetaR * tangent;
    float perpLen2 = max(dot(lightPerp, lightPerp) * dot(viewPerp, viewPerp), 1e-8);
    float cosPhiD = clamp(dot(lightPerp, viewPerp) * inversesqrt(perpLen2), -1.0, 1.0);
    // cos(phi/2) via half-angle identity (always non-negative for phi in [0, pi]).
    float cosHalfPhi = sqrt(max(0.5 + 0.5 * cosPhiD, 0.0));

    // Longitudinal lobes (Gaussian around shifted cuticle angle)
    float alphaR   =  HairMarschnerCuticleShift;
    float alphaTT  = -HairMarschnerCuticleShift * 0.5;
    float alphaTRT = -HairMarschnerCuticleShift * 1.5;
    float betaR   = HairMarschnerLongitudinalSigma;
    float betaTT  = HairMarschnerLongitudinalSigma * 0.5;
    float betaTRT = HairMarschnerLongitudinalSigma * 2.0;

    float Mr   = HairLongitudinalLobe(sinThetaI, sinThetaR, alphaR,   betaR);
    float Mtt  = HairLongitudinalLobe(sinThetaI, sinThetaR, alphaTT,  betaTT);
    float Mtrt = HairLongitudinalLobe(sinThetaI, sinThetaR, alphaTRT, betaTRT);

    vec3 hairColor = max(surface.Albedo, vec3(1e-3));

    // Azimuthal R lobe
    float fR = HairSchlickFresnel(sqrt(max(0.5 + 0.5 * dot(lightDir, viewDir), 0.0)), HairMarschnerIor);
    float Nr = 0.25 * cosHalfPhi * fR;

    // Azimuthal TT lobe
    // a = 1/eta', Karis approximation of the modified IOR.
    float a = 1.55 / (HairMarschnerIor * (1.19 / cosThetaD + 0.36 * cosThetaD));
    float h = clamp((1.0 + a * (0.6 - 0.8 * cosPhiD)) * cosHalfPhi, -1.0, 1.0);
    float oneMinusHaSq = max(1.0 - h * h * a * a, 0.0);
    // Karis: T = color^(sqrt(1 - h^2 a^2) / (2 cos(thetaD)))
    vec3  Ttt = pow(hairColor, vec3(sqrt(oneMinusHaSq) / (2.0 * cosThetaD)));
    float Dtt = exp(-3.65 * cosPhiD - 3.98);
    float fTT = HairSchlickFresnel(cosThetaD * sqrt(max(1.0 - h * h, 0.0)), HairMarschnerIor);
    vec3  Att = (1.0 - fTT) * (1.0 - fTT) * Ttt;
    vec3  Ntt = 0.5 * Att * Dtt;

    // Azimuthal TRT lobe
    vec3  Ttrt = pow(hairColor, vec3(0.8 / cosThetaD));
    float Dtrt = exp(17.0 * cosPhiD - 16.78);
    float fTRT = HairSchlickFresnel(cosThetaD * 0.5, HairMarschnerIor);
    vec3  Atrt = (1.0 - fTRT) * (1.0 - fTRT) * fTRT * Ttrt * Ttrt;
    vec3  Ntrt = 0.5 * Atrt * Dtrt;

    // Cylindrical Lambert factor: light projected perpendicular to the fiber.
    float sinL = sqrt(max(1.0 - sinThetaI * sinThetaI, 0.0));

    vec3 marschner = vec3(Mr * Nr) * HairMarschnerScaleR
                   + Mtt  * Ntt    * HairMarschnerScaleTT
                   + Mtrt * Ntrt   * HairMarschnerScaleTRT;

    // Wrap-around diffuse — hair scatters light from all azimuths, so we use a
    // soft falloff against cosThetaI / cosHalfPhi instead of a hard N.L term.
    float diffuseWrap = mix(1.0, cosThetaI,   HairMarschnerDiffuseFalloff) * mix(1.0, cosHalfPhi,  HairMarschnerDiffuseAzimFalloff);
    vec3 diffuse = hairColor * diffuseWrap * HairMarschnerScaleDiffuse * sinL * ambientOcclusion;

    return (diffuse + marschner) * attenuation * light.color;
}

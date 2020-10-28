export const name = "Fire"
export const interval = 33 // ms

let m_lightness = []
let m_center = 0

const Cooling = 8
const SparkMax = 160
const SparkMin = 100
const DefaultLightness = 127

function randomBounded(max) {
	return Math.floor(Math.random() * Math.floor(max))
}

export function tick(baseColor, colors)
{
	if (colors.length < 2)
		return colors

	if (colors.length > m_lightness.length) {
		for (let i = m_lightness.length; i < colors.length; ++i)
			m_lightness.push(DefaultLightness)
	}

	// heavily inspired by FastLED Fire2012 demo
	// https://github.com/FastLED/FastLED/blob/master/examples/Fire2012/Fire2012.ino

	const centerMax = Math.round(colors.length / 4)
	const middleLed = Math.floor(colors.length / 2)
	const sparkCount = Math.round(colors.length / 12)

	m_center += randomBounded(2) ? -1 : 1
	m_center = Math.max(-centerMax, Math.min(centerMax, m_center))

	for (let i = 0; i < middleLed + m_center; ++i) {
		const minLightnessReduction = Cooling * Math.pow(i / (middleLed + m_center), 3)
		const maxLightnessReduction = minLightnessReduction * 2 // useless?
		const lightnessReduction = minLightnessReduction + randomBounded(Math.max(1, maxLightnessReduction - minLightnessReduction)) + Cooling / 3
		m_lightness[i] = Math.round(Math.max(0, m_lightness[i] - lightnessReduction)) % 256
	}

	for (let i = colors.length - 1; i >= middleLed + m_center; --i) {
		const minLightnessReduction = Cooling * Math.pow((colors.length - 1 - i) / (middleLed - m_center), 3)
		const maxLightnessReduction = minLightnessReduction * 2 // useless?
		const lightnessReduction = minLightnessReduction + randomBounded(Math.max(1, maxLightnessReduction - minLightnessReduction)) + Cooling / 3
		m_lightness[i] = Math.round(Math.max(0, m_lightness[i] - lightnessReduction)) % 256
	}

	for (let k = middleLed + m_center; k > 1; --k)
		m_lightness[k] = Math.round((m_lightness[k - 1] + m_lightness[k - 2] * 2) / 3) % 256

	for (let k = middleLed + m_center; k < colors.length - 2; ++k)
		m_lightness[k] = Math.round((m_lightness[k + 1] + m_lightness[k + 2] * 2) / 3) % 256


	if (randomBounded(2) == 0) {
		const y = randomBounded(Math.max(1, sparkCount))
		m_lightness[y] = Math.max(SparkMax, m_lightness[y] + (SparkMin + randomBounded(SparkMax - SparkMin))) % 256
	}
	if (randomBounded(2) == 0) {
		const z = colors.length - 1 - randomBounded(Math.max(1, sparkCount))
		m_lightness[z] = Math.max(SparkMax, m_lightness[z] + (SparkMin + randomBounded(SparkMax - SparkMin))) % 256
	}

	let color = new QColor(baseColor)
	for (let i = 0; i < colors.length; i++)
	{
		color.setHsl(color.hslHue(), color.hslSaturation(), m_lightness[i])
		colors[i] = color.rgb()
	}

	return colors
}

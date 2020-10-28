export const name = "RGB is Life"
export const interval = 33 // ms
const speed = 1.5

let m_frames = 0

export default function tick(baseColor, colors)
{
	baseColor = new QColor(baseColor)
	const degrees = 360.0 / colors.length
	const step = speed * m_frames++
	for (let i = 0; i < colors.length; i++)
	{
		let color = new QColor(colors[i])
		color.setHsl(baseColor.hslHue() + degrees * i + step, baseColor.hslSaturation(), baseColor.lightness())
		colors[i] = color.rgb()
	}
	if (step > 360)
		m_frames = 0

	return colors
}

export const name = "Random noise"
export const interval = 50 // ms

export function tick(baseColor, colors)
{
	let color = new QColor(baseColor)
	for (let i = 0; i < colors.length; i++) {
		color.setRgb(Math.random() * 0xFFFFFF)
		colors[i] = color.rgb()
	}
	return colors
}

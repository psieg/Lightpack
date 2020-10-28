export const name = "Breathing"
export const interval = 40 // ms

let lightnessInc = -1
let lightness = -1

export function tick(baseColor, colors)
{
	baseColor = new QColor(baseColor)
	if (lightness == -1)
		lightness = baseColor.lightness()
	let color = new QColor()
	if (lightness <= baseColor.lightness() / 2 && lightnessInc == -1)
		lightnessInc = 1
	else if (lightness >= baseColor.lightness() - 1 && lightnessInc == 1)
		lightnessInc = -1

	lightness += lightnessInc

	color.setHsl(baseColor.hslHue(), baseColor.hslSaturation(), lightness)
	return colors.fill(color.rgb())
}
